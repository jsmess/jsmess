//============================================================
//
//  blit.c - Win32 blit handling
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// MAME headers
#include "osdepend.h"
#include "driver.h"
#include "video/vector.h"
#include "x86drc.h"

// MAMEOS headers
#include "blit.h"
#include "winmain.h"
#include "video.h"
#include "window.h"



//============================================================
//  PARAMETERS
//============================================================

#define DEBUG_BLITTERS		0
#define MAX_SCREEN_DIM		2048
#define SUPPORT_MMX			1
#define SUPPORT_SSE			1
#define SUPPORT_SSE2		1



//============================================================
//  TYPE DEFINITIONS
//============================================================

typedef void (*blitter_func)(void);

typedef struct _blit_state blit_state;
struct _blit_state
{
	const win_blit_params *blit;
	int offset;
	int update;
	int has_used_mmx;
	int has_used_non_temporal;
	int row_count;
};

typedef enum _brightness brightness;
enum _brightness
{
	BRIGHTNESS_25,
	BRIGHTNESS_50,
	BRIGHTNESS_75,
	BRIGHTNESS_100
};



//============================================================
//  GLOBAL VARIABLES
//============================================================

void *asmblit_srcdata;
UINT32 asmblit_srcheight;
void *asmblit_srclookup;

void *asmblit_dstdata;
UINT32 asmblit_dstpitch;


//============================================================
//  LOCAL VARIABLES
//============================================================

// blitter cache
static UINT8 *				active_fast_blitter;
static UINT8 *				active_update_blitter;

// current parameters
static win_blit_params		active_blitter_params;

// MMX/SSE/SSE2 supported?
static int					use_mmx;
static int					use_sse;
static int					use_sse2;

static drc_core *			drc;
static int					pixoffset[16];



//============================================================
//  PROTOTYPES
//============================================================

static void generate_blitter(const win_blit_params *blit);



//============================================================
//  win_blit_exit
//============================================================

static void win_blit_exit(running_machine *machine)
{
	drc_exit(drc);
}



//============================================================
//  win_blit_init
//============================================================

void win_blit_init(running_machine *machine)
{
	drc_config drconfig;
	UINT32 features;

	// initialize DRC
	memset(&drconfig, 0, sizeof(drconfig));
	drconfig.cache_size = (16 * 1024 * 1024);
	drc = drc_init(~0, &drconfig);
	if (!drc)
		fatalerror("Failed to init blit");

	// determine MMX/XMM support
	if (sizeof(void *) == 8)
	{
		// 64-bit always has these
		use_mmx = TRUE;
		use_sse = TRUE;
		use_sse2 = TRUE;
	}
	else
	{
		features = drc_x86_get_features();
		use_mmx = (features & (1 << 23));
		use_sse = (features & (1 << 25));
		use_sse2 = (features & (1 << 26));
	}

	if (use_sse2)
		verbose_printf("blit: SSE2 supported\n");
	else if (use_sse)
		verbose_printf("blit: SSE supported\n");
	else if (use_mmx)
		verbose_printf("blit: MMX supported\n");
	else
		verbose_printf("blit: MMX not supported\n");

	add_exit_callback(win_blit_exit);
}



//============================================================
//  win_perform_blit
//============================================================

int win_perform_blit(const win_blit_params *blit, int update)
{
	int srcdepth = (blit->srcdepth + 7) / 8;
	int dstdepth = (blit->dstdepth + 7) / 8;
	rectangle temprect;
	blitter_func blitter;
	int srcx, srcy;

	// determine the starting source X/Y
	temprect.min_x = blit->srcxoffs;
	temprect.min_y = blit->srcyoffs;
	temprect.max_x = blit->srcxoffs + blit->srcwidth - 1;
	temprect.max_y = blit->srcyoffs + blit->srcheight - 1;

	if (blit->swapxy || blit->flipx || blit->flipy)
		win_disorient_rect(&temprect);

	if (!blit->swapxy)
	{
		srcx = blit->flipx ? (temprect.max_x + 1) : temprect.min_x;
		srcy = blit->flipy ? (temprect.max_y + 1) : temprect.min_y;
	}
	else
	{
		srcx = blit->flipy ? (temprect.max_x + 1) : temprect.min_x;
		srcy = blit->flipx ? (temprect.max_y + 1) : temprect.min_y;
	}

	// if anything important has changed, fix it
	if (blit->srcwidth != active_blitter_params.srcwidth ||
		blit->srcdepth != active_blitter_params.srcdepth ||
		blit->srcpitch != active_blitter_params.srcpitch ||
		blit->dstdepth != active_blitter_params.dstdepth ||
		blit->dstpitch != active_blitter_params.dstpitch ||
		blit->dstyskip != active_blitter_params.dstyskip ||
		blit->dstxscale != active_blitter_params.dstxscale ||
		blit->dstyscale != active_blitter_params.dstyscale ||
		blit->dsteffect != active_blitter_params.dsteffect ||
		blit->flipx != active_blitter_params.flipx ||
		blit->flipy != active_blitter_params.flipy ||
		blit->swapxy != active_blitter_params.swapxy)
	{
		generate_blitter(blit);
		active_blitter_params = *blit;
	}

	// copy data to the globals
	asmblit_srcdata = (UINT8 *)blit->srcdata + blit->srcpitch * srcy + srcdepth * srcx;
	asmblit_srcheight = blit->srcheight;
	asmblit_srclookup = blit->srclookup;

	asmblit_dstdata = (UINT8 *)blit->dstdata + blit->dstpitch * blit->dstyoffs + dstdepth * blit->dstxoffs;
	asmblit_dstpitch = blit->dstpitch;

	// pick the blitter
	blitter = update ? (blitter_func)active_update_blitter : (blitter_func)active_fast_blitter;
	(*blitter)();
	return 1;
}



//============================================================
//  has_mmx
//  has_sse
//============================================================

// we don't need to use the use_mmx/use_sse variables if we are a
// 64-bit compile
static int has_mmx(void)	{ return SUPPORT_MMX && ((sizeof(void *) == 8) || use_mmx); }
static int has_sse(void)	{ return SUPPORT_SSE && ((sizeof(void *) == 8) || use_sse); }
static int has_sse2(void)	{ return SUPPORT_SSE2 && ((sizeof(void *) == 8) || use_sse2); }



//============================================================
//  output_r8
//  output_r16
//  output_r32
//  output_mmx
//  shift_r32
//  shift_mmx_d
//============================================================

static void output_r8(blit_state *state, int reg)
{
	int row;
	for (row = 0; row < state->row_count; row++)
		_mov_m8bd_r8(REG_EDI, state->offset + row * state->blit->dstpitch, reg);
	state->offset += 1;
}

static void output_r16(blit_state *state, int reg)
{
	int row;
	for (row = 0; row < state->row_count; row++)
		_mov_m16bd_r16(REG_EDI, state->offset + row * state->blit->dstpitch, reg);
	state->offset += 2;
}

static void output_r32(blit_state *state, int reg)
{
	int row;
	for (row = 0; row < state->row_count; row++)
	{
		if (has_sse2())
			_movnti_m32bd_r32(REG_EDI, state->offset + row * state->blit->dstpitch, reg);
		else
			_mov_m32bd_r32(REG_EDI, state->offset + row * state->blit->dstpitch, reg);
	}
	state->offset += 4;

	if (has_sse2())
		state->has_used_non_temporal = TRUE;
}

static void output_mmx(blit_state *state, int reg)
{
	int row;
	for (row = 0; row < state->row_count; row++)
	{
		if (has_sse())
			_movntq_m64bd_mmx(REG_EDI, state->offset + row * state->blit->dstpitch, reg);
		else
			_movq_m64bd_mmx(REG_EDI, state->offset + row * state->blit->dstpitch, reg);
	}
	state->offset += 8;

	state->has_used_mmx = TRUE;
	if (has_sse())
		state->has_used_non_temporal = TRUE;
}

static void shift_r32(int reg, int shift)
{
	if (shift > 0)
		_shl_r32_imm(reg, shift);
	else if (shift < 0)
		_shr_r32_imm(reg, -shift);
}

static void shift_mmx_d(int reg, int shift)
{
	if (shift > 0)
		_pssld_mmx_imm(reg, shift);
	else if (shift < 0)
		_pssrd_mmx_imm(reg, -shift);
}



//============================================================
//  adjust_brightness_r16
//  adjust_brightness_r32
//============================================================

static void adjust_brightness_r16(int reg, brightness b)
{
	UINT16 shift1_mask;
	UINT16 shift2_mask;

	shift1_mask	= ~(1 << (win_color16_bdst_shift - 1))
				& ~(1 << (win_color16_bdst_shift + win_color16_gdst_shift - 1))
				& ~(1 << (win_color16_bdst_shift + win_color16_gdst_shift - win_color16_rdst_shift));
	shift2_mask = shift1_mask & (shift1_mask >> 1);

	switch(b)
	{
		case BRIGHTNESS_25:
			_shr_r16_imm(reg, 2);
			_and_r16_imm(reg, shift2_mask);
			break;

		case BRIGHTNESS_50:
			_shr_r16_imm(reg, 1);
			_and_r16_imm(reg, shift1_mask);
			break;

		case BRIGHTNESS_75:
			_mov_r16_r16(REG_DX, reg);
			_shr_r16_imm(reg, 2);
			_shr_r16_imm(REG_DX, 1);
			_and_r16_imm(reg, shift2_mask);
			_and_r16_imm(REG_DX, shift1_mask);
			_add_r16_r16(reg, REG_DX);
			break;

		case BRIGHTNESS_100:
			// no reduction necessary
			break;
	}
}

static void adjust_brightness_r32(int reg, brightness b)
{
	switch(b)
	{
		case BRIGHTNESS_25:
			_shr_r32_imm(reg, 2);
			_and_r32_imm(reg, 0x3F3F3F);
			break;

		case BRIGHTNESS_50:
			_shr_r32_imm(reg, 1);
			_and_r32_imm(reg, 0x7F7F7F);
			break;

		case BRIGHTNESS_75:
			_mov_r32_r32(REG_EDX, reg);
			_shr_r32_imm(reg, 2);
			_shr_r32_imm(REG_EDX, 1);
			_and_r32_imm(reg, 0x3F3F3F);
			_and_r32_imm(REG_EDX, 0x7F7F7F);
			_add_r32_r32(reg, REG_EDX);
			break;

		case BRIGHTNESS_100:
			// no reduction necessary
			break;
	}
}



//============================================================
//  emit_val16_from_val32
//============================================================

static void emit_val16_from_val32(int reg)
{
	_mov_r32_r32(REG_ECX, reg);
	shift_r32(REG_ECX,
		win_color16_rdst_shift - win_color32_rdst_shift - win_color16_rsrc_shift);
	_and_r32_imm(REG_ECX, (0xFF >> win_color16_rsrc_shift) << win_color16_rdst_shift);

	_mov_r32_r32(REG_EDX, reg);
	shift_r32(REG_EDX,
		win_color16_gdst_shift - win_color32_gdst_shift - win_color16_gsrc_shift);
	_and_r32_imm(REG_EDX, (0xFF >> win_color16_gsrc_shift) << win_color16_gdst_shift);
	_or_r32_r32(REG_ECX, REG_EDX);

	shift_r32(reg,
		win_color16_bdst_shift - win_color32_bdst_shift - win_color16_bsrc_shift);
	_and_r32_imm(reg, (0xFF >> win_color16_bsrc_shift) << win_color16_bdst_shift);
	_or_r32_r32(reg, REG_ECX);
}



//============================================================
//  expand_blitter
//============================================================

static void expand_blitter(int count, blit_state *state,
	int row_start, int row_count, brightness b)
{
	int i, j, x, is_contiguous, span, block;
	const win_blit_params *blit = state->blit;

	state->offset = blit->dstpitch * row_start;
	state->row_count = row_count;

	// determine if contiguous or not
	is_contiguous = TRUE;
	for (i = 1; i < count; i++)
	{
		if ((pixoffset[0] + i * ((blit->srcdepth + 7) / 8)) != pixoffset[i])
		{
			is_contiguous = FALSE;
			break;
		}
	}

	if (has_mmx()
		&& ((count % REGCOUNT_MMX) == 0)
		&& ((blit->srcdepth == 15) || (blit->srcdepth == 16))
		&& ((blit->dstdepth == 16) || (blit->dstdepth == 32))
		&& ((blit->dstxscale == 1) || (blit->dstxscale == 2) || (blit->dstxscale == 3))
		&& (b == BRIGHTNESS_100))
	{
		// MMX, mod REGCOUNT_MMX, src 16 bit, dest 16 bit, scale 1/2
		switch(blit->dstxscale)
		{
			case 1:		span = 2;	break;
			case 2:		span = 2;	break;
			case 3:		span = 4;	break;
			default:	fatalerror("Invalid blit->dstxscale");	break;
		}

		for (i = 0; i < count; i += block * 2)
		{
			block = MIN(count - i, REGCOUNT_MMX / span);

			for (j = 0; j < block; j++)
			{
				_movzx_r32_m16bd(REG_EAX, REG_ESI, pixoffset[i + j * 2 + 0]);
				_movzx_r32_m16bd(REG_EBX, REG_ESI, pixoffset[i + j * 2 + 1]);
				_movd_mmx_m32bisd(REG_MM0 + j * span, REG_ECX, REG_EAX, 4, 0);
				_movd_mmx_m32bisd(REG_MM1 + j * span, REG_ECX, REG_EBX, 4, 0);
			}

			switch(blit->dstdepth * 100 + blit->dstxscale)
			{
				case 1601:
					for (j = 0; j < block; j++)
						_punpcklwd_mmx_mmx(REG_MM0 + j * span, REG_MM1 + j * span);
					for (j = 0; j < block; j += 2)
						_punpckldq_mmx_mmx(REG_MM0 + j * span, REG_MM0 + (j + 1) * span);
					for (j = 0; j < block; j += 2)
						output_mmx(state, REG_MM0 + j * span);
					break;

				case 1602:
					for (j = 0; j < block; j++)
					{
						_punpcklwd_mmx_mmx(REG_MM0 + j * span, REG_MM0 + j * span);
						_punpcklwd_mmx_mmx(REG_MM1 + j * span, REG_MM1 + j * span);
					}
					for (j = 0; j < block; j++)
						_punpckldq_mmx_mmx(REG_MM0 + j * span, REG_MM1 + j * span);
					for (j = 0; j < block; j++)
						output_mmx(state, REG_MM0 + j * span);
					break;

				case 1603:
					for (j = 0; j < block; j++)
						_movq_mmx_mmx(REG_MM2 + j * span, REG_MM0 + j * span);
					for (j = 0; j < block; j += 2)
						_punpcklwd_mmx_mmx(REG_MM0 + j * span, REG_MM0 + j * span);
					for (j = 0; j < block; j += 2)
						_punpcklwd_mmx_mmx(REG_MM1 + j * span, REG_MM1 + j * span);
					for (j = 0; j < block; j += 2)
						_punpcklwd_mmx_mmx(REG_MM4 + j * span, REG_MM5 + j * span);
					for (j = 0; j < block; j += 2)
						_punpcklwd_mmx_mmx(REG_MM4 + j * span, REG_MM5 + j * span);
					for (j = 0; j < block; j += 2)
						_punpcklwd_mmx_mmx(REG_MM6 + j * span, REG_MM6 + j * span);
					for (j = 0; j < block; j += 2)
						_punpcklwd_mmx_mmx(REG_MM2 + j * span, REG_MM1 + j * span);
					for (j = 0; j < block; j += 2)
						_punpckldq_mmx_mmx(REG_MM0 + j * span, REG_MM2 + j * span);
					for (j = 0; j < block; j += 2)
						_punpckldq_mmx_mmx(REG_MM1 + j * span, REG_MM6 + j * span);
					for (j = 0; j < block; j += 2)
						_punpckldq_mmx_mmx(REG_MM4 + j * span, REG_MM5 + j * span);

					for (j = 0; j < block; j += 2)
					{
						output_mmx(state, REG_MM0 + j * span);
						output_mmx(state, REG_MM1 + j * span);
						output_mmx(state, REG_MM4 + j * span);
					}
					break;

				case 3201:
					for (j = 0; j < block; j++)
						_punpckldq_mmx_mmx(REG_MM0 + j * span, REG_MM1 + j * span);
					for (j = 0; j < block; j++)
						output_mmx(state, REG_MM0 + j * span);
					break;

				case 3202:
					for (j = 0; j < block; j++)
						_punpckldq_mmx_mmx(REG_MM0 + j * span, REG_MM1 + j * span);
					for (j = 0; j < block; j++)
						output_mmx(state, REG_MM0 + j * span);
					break;

				case 3203:
					for (j = 0; j < block; j++)
						_movq_mmx_mmx(REG_MM2 + j * span, REG_MM0 + j * span);
					for (j = 0; j < block; j++)
						_punpckldq_mmx_mmx(REG_MM0 + j * span, REG_MM0 + j * span);
					for (j = 0; j < block; j++)
						_punpckldq_mmx_mmx(REG_MM2 + j * span, REG_MM1 + j * span);
					for (j = 0; j < block; j++)
						_punpckldq_mmx_mmx(REG_MM1 + j * span, REG_MM1 + j * span);
					for (j = 0; j < block; j++)
					{
						output_mmx(state, REG_MM0 + j * span);
						output_mmx(state, REG_MM2 + j * span);
						output_mmx(state, REG_MM1 + j * span);
					}
					break;

				default:
					fatalerror("Invalid blit->dstxdepth/blit->dstxscale");
					break;
			}
		}
	}
	else if (has_mmx()
		&& ((count % REGCOUNT_MMX) == 0)
		&& (blit->srcdepth == 32)
		&& (blit->dstdepth == 32)
		&& ((blit->dstxscale == 1) || (blit->dstxscale == 2))
		&& (b == BRIGHTNESS_100))
	{
		// MMX, mod REGCOUNT_MMX, src 32 bit, dest 32 bit, scale 1
		switch(blit->dstxscale)
		{
			case 1:		span = 1;	break;
			case 2:		span = 2;	break;
			default:	fatalerror("Invalid blit->dstxscale");	break;
		}
		span = MAX(span, is_contiguous ? 1 : 2);

		for (i = 0; i < count; i += block * 2)
		{
			block = MIN(count - i, REGCOUNT_MMX / span);

			if (is_contiguous)
			{
				for (j = 0; j < block; j++)
					_movq_mmx_m64bd(REG_MM0 + (j * span), REG_ESI, pixoffset[i + j * 2]);
			}
			else
			{
				for (j = 0; j < block; j++)
				{
					_movd_mmx_m32bd(REG_MM0 + (j * span), REG_ESI, pixoffset[i + j * 2 + 0]);
					_movd_mmx_m32bd(REG_MM1 + (j * span), REG_ESI, pixoffset[i + j * 2 + 1]);
				}
				for (j = 0; j < block; j++)
					_punpckldq_mmx_mmx(REG_MM0 + (j * span), REG_MM1 + (j * span));
			}

			switch(blit->dstxscale)
			{
				case 1:
					for (j = 0; j < block; j++)
						output_mmx(state, REG_MM0 + (j * span));
					break;

				case 2:
					for (j = 0; j < block; j++)
						_movq_mmx_mmx(REG_MM1 + (j * span), REG_MM0 + (j * span));
					for (j = 0; j < block; j++)
					{
						_punpckldq_mmx_mmx(REG_MM0 + (j * span), REG_MM0 + (j * span));
						_punpckhdq_mmx_mmx(REG_MM1 + (j * span), REG_MM1 + (j * span));
					}
					for (j = 0; j < block; j++)
					{
						output_mmx(state, REG_MM0 + (j * span));
						output_mmx(state, REG_MM1 + (j * span));
					}
					break;

				default:
					fatalerror("Invalid blit->dstxscale");
					break;
			}
		}
	}
	else if (has_mmx()
		&& ((count % (REGCOUNT_MMX / 2)) == 0)
		&& (blit->srcdepth == 32)
		&& (blit->dstdepth == 16)
		&& (blit->dstxscale == 1)
		&& (b == BRIGHTNESS_100))
	{
		// MMX, mod REGCOUNT_MMX/2, src 32 bit, dest 16 bit, scale 1
		static PAIR64 rmmx_shift;
		static PAIR64 gmmx_shift;
		static PAIR64 bmmx_shift;

		rmmx_shift.d.h = rmmx_shift.d.l = (UINT16) ((0xFF >> win_color16_rsrc_shift) << win_color16_rdst_shift);
		gmmx_shift.d.h = gmmx_shift.d.l = (UINT16) ((0xFF >> win_color16_gsrc_shift) << win_color16_gdst_shift);
		bmmx_shift.d.h = bmmx_shift.d.l = (UINT16) ((0xFF >> win_color16_bsrc_shift) << win_color16_bdst_shift);

		for (i = 0; i < count; i += (REGCOUNT_MMX / 2))
		{
			if (is_contiguous)
			{
				for (j = 0; j < REGCOUNT_MMX; j += 4)
					_movq_mmx_m64bd(REG_MM0 + j, REG_ESI, pixoffset[i + j / 2]);
			}
			else
			{
				for (j = 0; j < REGCOUNT_MMX; j += 2)
					_movd_mmx_m32bd(REG_MM0 + j, REG_ESI, pixoffset[i + j / 2]);
				for (j = 0; j < REGCOUNT_MMX; j += 4)
					_punpckldq_mmx_mmx(REG_MM0 + j, REG_MM2 + j);
			}

			for (j = 0; j < REGCOUNT_MMX; j += 4)
				_movq_mmx_mmx(REG_MM1 + j, REG_MM0 + j);
			for (j = 0; j < REGCOUNT_MMX; j += 4)
				shift_mmx_d(REG_MM1 + j, win_color16_rdst_shift - win_color32_rdst_shift - win_color16_rsrc_shift);
			for (j = 0; j < REGCOUNT_MMX; j += 4)
				_pand_mmx_m64abs(REG_MM1 + j, &rmmx_shift);

			for (j = 0; j < REGCOUNT_MMX; j += 4)
				_movq_mmx_mmx(REG_MM2 + j, REG_MM0 + j);
			for (j = 0; j < REGCOUNT_MMX; j += 4)
				shift_mmx_d(REG_MM2 + j, win_color16_gdst_shift - win_color32_gdst_shift - win_color16_gsrc_shift);
			for (j = 0; j < REGCOUNT_MMX; j += 4)
				_pand_mmx_m64abs(REG_MM2 + j, &gmmx_shift);

			for (j = 0; j < REGCOUNT_MMX; j += 4)
				shift_mmx_d(REG_MM0 + j, win_color16_bdst_shift - win_color32_bdst_shift - win_color16_bsrc_shift);
			for (j = 0; j < REGCOUNT_MMX; j += 4)
				_pand_mmx_m64abs(REG_MM0 + j, &bmmx_shift);

			for (j = 0; j < REGCOUNT_MMX; j += 4)
				_por_mmx_mmx(REG_MM0 + j, REG_MM1 + j);
			for (j = 0; j < REGCOUNT_MMX; j += 4)
				_por_mmx_mmx(REG_MM0 + j, REG_MM2 + j);

			if (has_sse())
			{
				for (j = 0; j < REGCOUNT_MMX; j += 4)
					_pshufw_mmx_mmx_imm(REG_MM0 + j, REG_MM0 + j, 0x08);
				for (j = 0; j < REGCOUNT_MMX; j += 8)
					_punpckldq_mmx_mmx(REG_MM0 + j, REG_MM4 + j);
				for (j = 0; j < REGCOUNT_MMX; j += 8)
					output_mmx(state, REG_MM0 + j);
			}
			else
			{
				for (j = 0; j < REGCOUNT_MMX; j += 4)
					_movq_mmx_mmx(REG_MM1 + j, REG_MM0 + j);
				for (j = 0; j < REGCOUNT_MMX; j += 4)
					_punpckhdq_mmx_mmx(REG_MM1 + j, REG_MM1 + j);
				for (j = 0; j < REGCOUNT_MMX; j += 4)
					_punpcklwd_mmx_mmx(REG_MM0 + j, REG_MM1 + j);
				for (j = 0; j < REGCOUNT_MMX; j += 8)
					_punpckldq_mmx_mmx(REG_MM0 + j, REG_MM4 + j);
				for (j = 0; j < REGCOUNT_MMX; j += 8)
					output_mmx(state, REG_MM0 + j);
			}
		}
	}
	else if ((count % 2) == 0)
	{
		// No MMX/SSE, even count
		for (i = 0; i < count; i += 2)
		{
			switch(blit->srcdepth)
			{
				case 15:
				case 16:
					_movzx_r32_m16bd(REG_EAX, REG_ESI, pixoffset[i + 0]);
					_movzx_r32_m16bd(REG_EBX, REG_ESI, pixoffset[i + 1]);
					_mov_r32_m32bisd(REG_EAX, REG_ECX, REG_EAX, 4, 0);
					_mov_r32_m32bisd(REG_EBX, REG_ECX, REG_EBX, 4, 0);
					break;

				case 32:
					_mov_r32_m32bd(REG_EAX, REG_ESI, pixoffset[i + 0]);
					_mov_r32_m32bd(REG_EBX, REG_ESI, pixoffset[i + 1]);
					break;

				default:
					fatalerror("Invalid blit->srcdepth");
					break;
			}

			// if we are converting 32 bit values to 16 bit values, we
			// must shuffle the bits around
			if ((blit->srcdepth == 32) && (blit->dstdepth == 16))
			{
				emit_val16_from_val32(REG_EAX);
				emit_val16_from_val32(REG_EBX);
			}

			// adjust brightness if necessary
			switch(blit->dstdepth)
			{
				case 16:
					adjust_brightness_r16(REG_AX, b);
					adjust_brightness_r16(REG_BX, b);
					break;

				case 24:
				case 32:
					adjust_brightness_r32(REG_EAX, b);
					adjust_brightness_r32(REG_EBX, b);
					break;
			}

			switch(blit->dstdepth)
			{
				case 16:
					for (x = 0; x < blit->dstxscale; x++)
						output_r16(state, REG_AX);
					for (x = 0; x < blit->dstxscale; x++)
						output_r16(state, REG_BX);
					break;

				case 24:
					_mov_r32_r32(REG_EDX, REG_EAX);
					_shr_r32_imm(REG_EDX, 16);
					for (x = 0; x < blit->dstxscale; x++)
					{
						output_r16(state, REG_AX);
						output_r8(state, REG_DL);
					}

					_mov_r32_r32(REG_EDX, REG_EBX);
					_shr_r32_imm(REG_EDX, 16);
					for (x = 0; x < blit->dstxscale; x++)
					{
						output_r16(state, REG_BX);
						output_r8(state, REG_DL);
					}
					break;

				case 32:
					for (x = 0; x < blit->dstxscale; x++)
						output_r32(state, REG_EAX);
					for (x = 0; x < blit->dstxscale; x++)
						output_r32(state, REG_EBX);
					break;

				default:
					fatalerror("Invalid blit->dstdepth");
					break;
			}
		}
	}
	else
	{
		// No MMX/SSE, odd count
		for (i = 0; i < count; i++)
		{
			switch(blit->srcdepth)
			{
				case 15:
				case 16:
					_movzx_r32_m16bd(REG_EAX, REG_ESI, pixoffset[i]);
					_mov_r32_m32bisd(REG_EAX, REG_ECX, REG_EAX, 4, 0);
					break;

				case 32:
					_mov_r32_m32bd(REG_EAX, REG_ESI, pixoffset[i]);
					break;

				default:
					fatalerror("Invalid blit->srcdepth");
					break;
			}

			// if we are converting 32 bit values to 16 bit values, we
			// must shuffle the bits around
			if ((blit->srcdepth == 32) && (blit->dstdepth == 16))
				emit_val16_from_val32(REG_EAX);

			for (x = 0; x < blit->dstxscale; x++)
			{
				switch(blit->dstdepth)
				{
					case 16:
						adjust_brightness_r16(REG_AX, b);
						output_r16(state, REG_AX);
						break;

					case 24:
						adjust_brightness_r32(REG_EAX, b);
						_mov_r32_r32(REG_EDX, REG_EAX);
						_shr_r32_imm(REG_EDX, 16);
						output_r16(state, REG_AX);
						output_r8(state, REG_DL);
						break;

					case 32:
						adjust_brightness_r32(REG_EAX, b);
						output_r32(state, REG_EAX);
						break;

					default:
						fatalerror("Invalid blit->dstdepth");
						break;
				}
			}
		}
	}
}



//============================================================
//  generate_blitter
//============================================================

static void generate_single_blitter(const win_blit_params *blit, int update)
{
	int middle, last;
	int srcxsize = (blit->srcdepth + 7) / 8;
	int srcysize = blit->srcpitch;
	int srcbytes, srcadvance;
	int pix;
	int row, row_count;
	brightness b;
	void *xloop_base;
	void *yloop_base;
	blit_state state;

	memset(&state, 0, sizeof(state));
	state.blit = blit;
	state.update = update;

	if (blit->swapxy)
	{
		int temp = srcxsize;
		srcxsize = srcysize;
		srcysize = temp;
	}

	if (!blit->flipx)
	{
		// no flipping
		if (!blit->flipy)
		{
			srcbytes = srcxsize;
			srcadvance = srcysize;
			for (pix = 0; pix < 16; pix++)
				pixoffset[pix] = pix * srcxsize;
		}

		// Y flipping
		else
		{
			srcbytes = srcxsize;
			srcadvance = -srcysize;
			for (pix = 0; pix < 16; pix++)
				pixoffset[pix] = pix * srcxsize - srcysize;
		}
	}

	else
	{
		// X flipping
		if (!blit->flipy)
		{
			srcbytes = -srcxsize;
			srcadvance = srcysize;
			for (pix = 0; pix < 16; pix++)
				pixoffset[pix] = -(pix + 1) * srcxsize;
		}

		// X and Y flipping
		else
		{
			srcbytes = -srcxsize;
			srcadvance = -srcysize;
			for (pix = 0; pix < 16; pix++)
				pixoffset[pix] = -(pix + 1) * srcxsize - srcysize;
		}
	}

	// determine how many pixels to do at the middle, and end
	middle = blit->srcwidth / 16;
	last = blit->srcwidth % 16;

	_pushad();

	// load the source/dest pointers
	_mov_r32_m32abs(REG_ESI, &asmblit_srcdata);
	_mov_r32_m32abs(REG_EDI, &asmblit_dstdata);

	if (blit->srcdepth == 15 || blit->srcdepth == 16)
	{
		// load the palette pointer
		_mov_r32_m32abs(REG_ECX, &asmblit_srclookup);
	}

	_push_r32(REG_ESI);
	_push_r32(REG_EDI);

	// top of yloop
	yloop_base = drc->cache_top;

	row = 0;
	while(row < blit->dstyscale)
	{
		// retrieve EDI/ESI
		_mov_r32_m32bd(REG_EDI, REG_ESP, 0);
		_mov_r32_m32bd(REG_ESI, REG_ESP, 4);

		// evaluate how many lines we are doing in this pass, depending
		// on the blit effect
		switch(blit->dsteffect)
		{
			case EFFECT_SCANLINE_25:
				b = (row == 0) ? BRIGHTNESS_100 : BRIGHTNESS_25;
				row_count = (row == 0) ? blit->dstyscale - 1 : 1;
				break;

			case EFFECT_SCANLINE_50:
				b = (row == 0) ? BRIGHTNESS_100 : BRIGHTNESS_50;
				row_count = (row == 0) ? blit->dstyscale - 1 : 1;
				break;

			case EFFECT_SCANLINE_75:
				b = (row == 0) ? BRIGHTNESS_100 : BRIGHTNESS_75;
				row_count = (row == 0) ? blit->dstyscale - 1 : 1;
				break;

			default:
				b = BRIGHTNESS_100;
				row_count = blit->dstyscale;
				break;
		}

		// top of middle (X) loop
		if (middle > 0)
		{
			// determine the number of 16-byte chunks to blit
			_mov_r32_imm(REG_EBP, middle);

			xloop_base = drc->cache_top;

			if (has_sse() && (row == 0))
			{
				// prefetch instructions
				_prefetch_m8bd(0, REG_ESI, srcbytes * 16 + 64);
			}

			expand_blitter(16, &state, row, row_count, b);

			// middlexloop_bottom
			_sub_or_dec_r32_imm(REG_EBP, 1);
			_lea_r32_m32bd(REG_ESI, REG_ESI, srcbytes * 16);
			_lea_r32_m32bd(REG_EDI, REG_EDI, ((blit->dstdepth + 7) / 8) * blit->dstxscale * 16);
			_jcc(COND_NE, xloop_base);
		}

		// top of last (X) loop
		if (last > 0)
		{
			_mov_r32_imm(REG_EBP, last);

			xloop_base = drc->cache_top;

			expand_blitter(1, &state, row, row_count, b);

			// lastxloop_bottom
			_sub_or_dec_r32_imm(REG_EBP, 1);
			_lea_r32_m32bd(REG_ESI, REG_ESI, srcbytes);
			_lea_r32_m32bd(REG_EDI, REG_EDI, ((blit->dstdepth + 7) / 8) * blit->dstxscale);
			_jcc(COND_NE, xloop_base);
		}

		row += row_count;
	}

	if (update && blit->dstyskip)
	{
		// need to emit code to blank out scanline
		UINT32 size = blit->dstpitch * blit->dstyskip;
		UINT32 increment;
		void *skiploop_base;

		_mov_r32_m32bd(REG_EDI, REG_ESP, 0);
		_lea_r32_m32bd(REG_EDI, REG_EDI, blit->dstpitch * blit->dstyscale);

		if (has_mmx() && ((size % 8) == 0))
		{
			increment = 8;
			_mov_r32_imm(REG_EBP, size / 8);
			_pxor_mmx_mmx(REG_MM7, REG_MM7);

			skiploop_base = drc->cache_top;
			if (has_sse())
				_movntq_m64bd_mmx(REG_EDI, 0, REG_MM7);
			else
				_movq_m64bd_mmx(REG_EDI, 0, REG_MM7);

			state.has_used_mmx = TRUE;
			if (has_sse())
				state.has_used_non_temporal = TRUE;
		}
		else if ((size % 4) == 0)
		{
			increment = 4;
			_mov_r32_imm(REG_EBP, size / 4);
			_xor_r32_r32(REG_EAX, REG_EAX);

			skiploop_base = drc->cache_top;
			if (has_sse2())
				_movnti_m32bd_r32(REG_EDI, 0, REG_EAX);
			else
				_mov_m32bd_r32(REG_EDI, 0, REG_EAX);

			if (has_sse2())
				state.has_used_non_temporal = TRUE;
		}
		else
		{
			increment = 1;
			_mov_r32_imm(REG_EBP, size / 1);
			_mov_r8_imm(REG_AL, 0);

			skiploop_base = drc->cache_top;
			_mov_m8bd_r8(REG_EDI, 0, REG_AL);
		}

		_sub_or_dec_r32_imm(REG_EBP, 1);
		_lea_r32_m32bd(REG_EDI, REG_EDI, increment);
		_jcc(COND_NE, skiploop_base);
	}

	// advance source and destinations
	_add_m32bd_imm(REG_ESP, 0, blit->dstpitch * (blit->dstyscale + blit->dstyskip));
	_add_m32bd_imm(REG_ESP, 4, srcadvance);

	// end of the loop
	_sub_or_dec_m32abs_imm(&asmblit_srcheight, 1);
	_jcc(COND_NE, yloop_base);

	// function footer
	if (state.has_used_mmx)
		_emms();
	if (state.has_used_non_temporal)
		_sfence();
	_add_r32_imm(REG_ESP, 8);
	_popad();
	_ret();
}



static void generate_blitter(const win_blit_params *blit)
{
	const UINT8 *fast_end;
	const UINT8 *update_end;

	if (DEBUG_BLITTERS)
		fprintf(stderr, "Generating blitter\n");

	drc_cache_reset(drc);

	active_fast_blitter = drc->cache_top;
	generate_single_blitter(blit, FALSE);
	fast_end = drc->cache_top;

	if (blit->dstyskip == 0)
	{
		// if dstyskip is zero, then the fast and update blits are identical
		active_update_blitter = active_fast_blitter;
		update_end = fast_end;
	}
	else
	{
		// generate a separate update blitter
		active_update_blitter = drc->cache_top;
		generate_single_blitter(blit, TRUE);
		update_end = drc->cache_top;
	}

	// generate files with the results; use ndisasmw to disassemble them
	if (DEBUG_BLITTERS)
	{
		FILE *out;

		out = fopen("fast.com", "wb");
		fwrite(active_fast_blitter, 1, fast_end - active_fast_blitter, out);
		fclose(out);

		out = fopen("update.com", "wb");
		fwrite(active_update_blitter, 1, update_end - active_update_blitter, out);
		fclose(out);

		out = fopen("fast.dasm", "w");
		drc_dasm(out, active_fast_blitter, (void *) fast_end);
		fclose(out);

		out = fopen("update.dasm", "w");
		drc_dasm(out, active_update_blitter, (void *) update_end);
		fclose(out);
	}
}
