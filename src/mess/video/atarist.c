/*

	TODO:

	- double screen width to allow for medium resolution
	- STe pixelofs
	- blitter hog
	- high resolution

*/

#include "driver.h"
#include "video/generic.h"
#include "cpu/m68000/m68k.h"
#include "cpu/m68000/m68000.h"
#include "machine/68901mfp.h"
#include "video/atarist.h"

static struct SHIFTER
{
	UINT32 base;
	UINT32 ofs;
	UINT8 sync;
	UINT8 mode;
	UINT16 palette[16];
	UINT8 lineofs;  // STe
	UINT8 pixelofs; // STe
	UINT16 rr[4];
	UINT16 ir[4];
	int bitplane;
	int shift;
	int h, v;
	int de;
	int x_start, x_end, y_start, y_end, hblank_start, vblank_start;
} shifter;

static emu_timer *atarist_glue_timer;
static emu_timer *atarist_shifter_timer;

INLINE pen_t atarist_shift_mode_0(void)
{
	int color = (BIT(shifter.rr[3], 15) << 3) | (BIT(shifter.rr[2], 15) << 2) | (BIT(shifter.rr[1], 15) << 1) | BIT(shifter.rr[0], 15);

	shifter.rr[0] <<= 1;
	shifter.rr[1] <<= 1;
	shifter.rr[2] <<= 1;
	shifter.rr[3] <<= 1;

	return Machine->pens[color];
}

INLINE pen_t atarist_shift_mode_1(void)
{
	int color = (BIT(shifter.rr[1], 15) << 1) | BIT(shifter.rr[0], 15);

	shifter.rr[0] <<= 1;
	shifter.rr[1] <<= 1;
	shifter.shift++;

	if (shifter.shift == 16)
	{
		shifter.rr[0] = shifter.rr[2];
		shifter.rr[1] = shifter.rr[3];
		shifter.rr[2] = shifter.rr[3] = 0;
		shifter.shift = 0;
	}

	return Machine->pens[color];
}

INLINE pen_t atarist_shift_mode_2(void)
{
	int color = BIT(shifter.rr[0], 15);

	shifter.rr[0] <<= 1;
	shifter.shift++;

	switch (shifter.shift)
	{
	case 16:
		shifter.rr[0] = shifter.rr[1];
		shifter.rr[1] = shifter.rr[2];
		shifter.rr[2] = shifter.rr[3];
		shifter.rr[3] = 0;
		break;

	case 32:
		shifter.rr[0] = shifter.rr[1];
		shifter.rr[1] = shifter.rr[2];
		shifter.rr[2] = 0;
		break;

	case 48:
		shifter.rr[0] = shifter.rr[1];
		shifter.rr[1] = 0;
		shifter.shift = 0;
		break;
	}

	return Machine->pens[color];
}

static TIMER_CALLBACK(atarist_shifter_tick)
{
	int y = video_screen_get_vpos(0);
	int x = video_screen_get_hpos(0);

	pen_t pen;

	switch (shifter.mode)
	{
	case 0:
		pen = atarist_shift_mode_0();
		break;

	case 1:
		pen = atarist_shift_mode_1();
		break;

	case 2:
		pen = atarist_shift_mode_2();
		break;

	default:
		pen = get_black_pen(machine);
		break;
	}

	*BITMAP_ADDR32(tmpbitmap, y, x) = pen;
}

INLINE void atarist_shifter_load(void)
{
	UINT8 *RAM = memory_region(REGION_CPU1) + shifter.ofs;

	shifter.ir[shifter.bitplane] = (RAM[1] << 8) | RAM[0];
	shifter.bitplane++;
	shifter.ofs += 2;
	
	if (shifter.bitplane == 4)
	{
		shifter.bitplane = 0;
		
		shifter.rr[0] = shifter.ir[0];
		shifter.rr[1] = shifter.ir[1];
		shifter.rr[2] = shifter.ir[2];
		shifter.rr[3] = shifter.ir[3];
	}
}

static TIMER_CALLBACK(atarist_glue_tick)
{
	int y = video_screen_get_vpos(0);
	int x = video_screen_get_hpos(0);

	int v = (y >= shifter.y_start) && (y < shifter.y_end);
	int h = (x >= shifter.x_start) && (x < shifter.x_end);

	int de = h && v;

	if (de != shifter.de)
	{
		mfp68901_tbi_w(0, de);
		shifter.de = de;
	}

	if (de)
	{
		atarist_shifter_load();
	}

	if ((y == shifter.vblank_start) && (x == 0))
	{
		cpunum_set_input_line(0, MC68000_IRQ_4, HOLD_LINE);
		shifter.ofs = shifter.base;
	}

	if (x == shifter.hblank_start)
	{
		cpunum_set_input_line(0, MC68000_IRQ_2, HOLD_LINE);
		shifter.ofs += (shifter.lineofs * 2); // STe
	}
}

static void atarist_set_screen_parameters(void)
{
	if (shifter.sync & 0x02)
	{
		shifter.x_start = ATARIST_HBDEND_PAL;
		shifter.x_end = ATARIST_HBDSTART_PAL;
		shifter.y_start = ATARIST_VBDEND_PAL;
		shifter.y_end = ATARIST_VBDSTART_PAL;
		shifter.hblank_start = ATARIST_HBSTART_PAL;
		shifter.vblank_start = ATARIST_VBSTART_PAL;
	}
	else
	{
		shifter.x_start = ATARIST_HBDEND_NTSC;
		shifter.x_end = ATARIST_HBDSTART_NTSC;
		shifter.y_start = ATARIST_VBDEND_NTSC;
		shifter.y_end = ATARIST_VBDSTART_NTSC;
		shifter.hblank_start = ATARIST_HBSTART_NTSC;
		shifter.vblank_start = ATARIST_VBSTART_NTSC;
	}
}

/* Atari ST Shifter */

READ16_HANDLER( atarist_shifter_base_r )
{
	switch (offset)
	{
	case 0x00:
		return (shifter.base >> 16) & 0x3f;
	case 0x01:
		return (shifter.base >> 8) & 0xff;
	}
	
	return 0;
}

WRITE16_HANDLER( atarist_shifter_base_w )
{
	switch (offset)
	{
	case 0x00:
		shifter.base = (shifter.base & 0x00ff00) | (data & 0x3f) << 16;
		logerror("SHIFTER Video Base Address %06x\n", shifter.base);
		break;
	case 0x01:
		shifter.base = (shifter.base & 0x3f0000) | (data & 0xff) << 8;
		logerror("SHIFTER Video Base Address %06x\n", shifter.base);
		break;
	}
}

READ16_HANDLER( atarist_shifter_counter_r )
{
	switch (offset)
	{
	case 0x00:
		return (shifter.ofs >> 16) & 0x3f;
	case 0x01:
		return (shifter.ofs >> 8) & 0xff;
	case 0x02:
		return shifter.ofs & 0xff;
	}

	return 0;
}

READ16_HANDLER( atarist_shifter_sync_r )
{
	return shifter.sync;
}

WRITE16_HANDLER( atarist_shifter_sync_w )
{
	shifter.sync = data >> 8;
	logerror("SHIFTER Sync %x\n", shifter.sync);
	atarist_set_screen_parameters();
}

READ16_HANDLER( atarist_shifter_mode_r )
{
	return shifter.mode << 8;
}

WRITE16_HANDLER( atarist_shifter_mode_w )
{
	shifter.mode = data >> 8;
	logerror("SHIFTER Mode %x\n", shifter.mode);
}

READ16_HANDLER( atarist_shifter_palette_r )
{
	return shifter.palette[offset];
}
		
WRITE16_HANDLER( atarist_shifter_palette_w )
{
	shifter.palette[offset] = data;
	logerror("SHIFTER Palette[%x] = %x\n", offset, data);

	palette_set_color_rgb(Machine, offset, pal3bit(data >> 8), pal3bit(data >> 4), pal3bit(data));
}

/* Atari STe Shifter */

READ16_HANDLER( atariste_shifter_base_low_r )
{
	return shifter.base & 0xfe;
}

WRITE16_HANDLER( atariste_shifter_base_low_w )
{
	shifter.base = (shifter.base & 0x3fff00) | (data & 0xfe);
	logerror("SHIFTER Video Base Address %06x\n", shifter.base);
}

READ16_HANDLER( atariste_shifter_counter_r )
{
	switch (offset)
	{
	case 0x00:
		return (shifter.ofs >> 16) & 0x3f;
	case 0x01:
		return (shifter.ofs >> 8) & 0xff;
	case 0x02:
		return shifter.ofs & 0xfe;
	}

	return 0;
}

WRITE16_HANDLER( atariste_shifter_counter_w )
{
	switch (offset)
	{
	case 0x00:
		shifter.ofs = (shifter.ofs & 0x00fffe) | (data & 0x3f) << 16;
		logerror("SHIFTER Video Address Counter %06x\n", shifter.ofs);
		break;
	case 0x01:
		shifter.ofs = (shifter.ofs & 0x3f00fe) | (data & 0xff) << 8;
		logerror("SHIFTER Video Address Counter %06x\n", shifter.ofs);
		break;
	case 0x02:
		shifter.ofs = (shifter.ofs & 0x3fff00) | (data & 0xfe);
		logerror("SHIFTER Video Address Counter %06x\n", shifter.ofs);
		break;
	}
}

WRITE16_HANDLER( atariste_shifter_palette_w )
{
	int r = ((data >> 7) & 0x0e) | BIT(data, 11);
	int g = ((data >> 3) & 0x0e) | BIT(data, 7);
	int b = ((data << 1) & 0x0e) | BIT(data, 3);

	shifter.palette[offset] = data;
	logerror("SHIFTER palette %x = %x\n", offset, data);

	palette_set_color_rgb(Machine, offset, r, g, b);
}

READ16_HANDLER( atariste_shifter_lineofs_r )
{
	return shifter.lineofs;
}

WRITE16_HANDLER( atariste_shifter_lineofs_w )
{
	shifter.lineofs = data & 0xff;
	logerror("SHIFTER Line Offset %x\n", shifter.lineofs);
}

READ16_HANDLER( atariste_shifter_pixelofs_r )
{
	return shifter.pixelofs;
}

WRITE16_HANDLER( atariste_shifter_pixelofs_w )
{
	shifter.pixelofs = data & 0x0f;
	logerror("SHIFTER Pixel Offset %x\n", shifter.pixelofs);
}

/* Atari ST Blitter */

static const int BLITTER_NOPS[16][4] = 
{
	{ 1, 1, 1, 1 },
	{ 2, 2, 3, 3 },
	{ 2, 2, 3, 3 },
	{ 1, 1, 2, 2 },
	{ 2, 2, 3, 3 },
	{ 2, 2, 2, 2 },
	{ 2, 2, 3, 3 },
	{ 2, 2, 3, 3 },
	{ 2, 2, 3, 3 },
	{ 2, 2, 3, 3 },
	{ 2, 2, 2, 2 },
	{ 2, 2, 3, 3 },
	{ 1, 1, 2, 2 },
	{ 2, 2, 3, 3 },
	{ 2, 2, 3, 3 },
	{ 1, 1, 1, 1 }
};

static struct BLITTER
{
	UINT16 halftone[16];
	INT16 src_inc_x, src_inc_y, dst_inc_x, dst_inc_y;
	UINT32 src, dst;
	UINT16 endmask1, endmask2, endmask3;
	UINT16 xcount, ycount, xcountl;
	UINT8 hop, op, ctrl, skew;
	UINT32 srcbuf;
} blitter;

static void atarist_blitter_source(void)
{
	UINT8 *RAM = memory_region(REGION_CPU1) + blitter.src;

	if (blitter.src_inc_x < 0)
	{
		blitter.srcbuf = (RAM[1] << 24) | (RAM[0] << 16) | (blitter.srcbuf >> 16);
	}
	else
	{
		blitter.srcbuf = (blitter.srcbuf << 16) | (RAM[1] << 8) | RAM[0];
	}
}

static UINT16 atarist_blitter_hop(void)
{
	UINT16 source = blitter.srcbuf >> (blitter.skew & 0x0f);
	UINT16 halftone = blitter.halftone[blitter.ctrl & 0x0f];

	if (blitter.ctrl & ATARIST_BLITTER_CTRL_SMUDGE)
	{
		halftone = blitter.halftone[source & 0x0f];
	}

	switch (blitter.hop)
	{
	case 0:
		return 0xffff;
	case 1:
		return halftone;
	case 2:
		return source;
	case 3:
		return source & halftone;
	}

	return 0;
}

static void atarist_blitter_op(UINT16 s, UINT32 dstaddr, UINT16 mask)
{
	UINT8 *dst = memory_region(REGION_CPU1) + dstaddr;
	UINT16 d = (dst[1] << 8) + dst[0];
	UINT16 result = 0;

	if (blitter.op & 0x08) result = (~s & ~d);
	if (blitter.op & 0x04) result |= (~s & d);
	if (blitter.op & 0x02) result |= (s & ~d);
	if (blitter.op & 0x01) result |= (s & d);

	dst[0] = result & 0xff;
	dst[1] = (result >> 8) & 0xff;
}

static TIMER_CALLBACK( atarist_blitter_tick )
{
	do
	{
		if (blitter.skew & ATARIST_BLITTER_SKEW_FXSR)
		{
			atarist_blitter_source();
			blitter.src += blitter.src_inc_x;
		}

		atarist_blitter_source();
		atarist_blitter_op(atarist_blitter_hop(), blitter.dst, blitter.endmask1);
		blitter.xcount--;

		while (blitter.xcount > 0)
		{
			blitter.src += blitter.src_inc_x;
			blitter.dst += blitter.dst_inc_x;
			
			if (blitter.xcount == 1)
			{
				if (!(blitter.skew & ATARIST_BLITTER_SKEW_NFSR))
				{
					atarist_blitter_source();
				}

				atarist_blitter_op(atarist_blitter_hop(), blitter.dst, blitter.endmask3);
			}
			else
			{
				atarist_blitter_source();
				atarist_blitter_op(atarist_blitter_hop(), blitter.dst, blitter.endmask2);
			}

			blitter.xcount--;
		}

		blitter.src += blitter.src_inc_y;
		blitter.dst += blitter.dst_inc_y;

		if (blitter.dst_inc_y < 0)
		{
			blitter.ctrl = (blitter.ctrl & 0xf0) | (((blitter.ctrl & 0x0f) - 1) & 0x0f);
		}
		else
		{
			blitter.ctrl = (blitter.ctrl & 0xf0) | (((blitter.ctrl & 0x0f) + 1) & 0x0f);
		}

		blitter.xcount = blitter.xcountl;
		blitter.ycount--;
	}
	while (blitter.ycount > 0);

	blitter.ctrl &= 0x7f;
}

READ16_HANDLER( atarist_blitter_halftone_r )
{
	return blitter.halftone[offset];
}

READ16_HANDLER( atarist_blitter_src_inc_x_r )
{
	return blitter.src_inc_x;
}

READ16_HANDLER( atarist_blitter_src_inc_y_r )
{
	return blitter.src_inc_y;
}

READ16_HANDLER( atarist_blitter_src_r )
{
	switch (offset)
	{
	case 0:
		return (blitter.src >> 16) & 0xff;
	case 1:
		return blitter.src & 0xfffe;
	}

	return 0;
}

READ16_HANDLER( atarist_blitter_end_mask_r )
{
	switch (offset)
	{
	case 0:
		return blitter.endmask1;
	case 1:
		return blitter.endmask2;
	case 2:
		return blitter.endmask3;
	}

	return 0;
}

READ16_HANDLER( atarist_blitter_dst_inc_x_r )
{
	return blitter.dst_inc_x;
}

READ16_HANDLER( atarist_blitter_dst_inc_y_r )
{
	return blitter.dst_inc_y;
}

READ16_HANDLER( atarist_blitter_dst_r )
{
	switch (offset)
	{
	case 0:
		return (blitter.dst >> 16) & 0xff;
	case 1:
		return blitter.dst & 0xfffe;
	}

	return 0;
}

READ16_HANDLER( atarist_blitter_count_x_r )
{
	return blitter.xcount;
}

READ16_HANDLER( atarist_blitter_count_y_r )
{
	return blitter.ycount;
}

READ16_HANDLER( atarist_blitter_op_r )
{
	if (ACCESSING_LSB16)
	{
		return blitter.hop;
	}
	else
	{
		return blitter.op;
	}
}

READ16_HANDLER( atarist_blitter_ctrl_r )
{
	if (ACCESSING_LSB16)
	{
		return blitter.ctrl;
	}
	else
	{
		return blitter.skew;
	}
}

WRITE16_HANDLER( atarist_blitter_halftone_w )
{
	blitter.halftone[offset] = data;
}

WRITE16_HANDLER( atarist_blitter_src_inc_x_w )
{
	blitter.src_inc_x = data & 0xfffe;
}

WRITE16_HANDLER( atarist_blitter_src_inc_y_w )
{
	blitter.src_inc_y = data & 0xfffe;
}

WRITE16_HANDLER( atarist_blitter_src_w )
{
	switch (offset)
	{
	case 0:
		blitter.src = (data & 0xff) | (blitter.src & 0xfffe);
	case 1:
		blitter.src = (blitter.src & 0xff0000) | (data & 0xfffe);
	}
}

WRITE16_HANDLER( atarist_blitter_end_mask_w )
{
	switch (offset)
	{
	case 0:
		blitter.endmask1 = data;
	case 1:
		blitter.endmask2 = data;
	case 2:
		blitter.endmask3 = data;
	}
}

WRITE16_HANDLER( atarist_blitter_dst_inc_x_w )
{
	blitter.dst_inc_x = data & 0xfffe;
}

WRITE16_HANDLER( atarist_blitter_dst_inc_y_w )
{
	blitter.dst_inc_y = data & 0xfffe;
}

WRITE16_HANDLER( atarist_blitter_dst_w )
{
	switch (offset)
	{
	case 0:
		blitter.dst = (data & 0xff) | (blitter.dst & 0xfffe);
	case 1:
		blitter.dst = (blitter.dst & 0xff0000) | (data & 0xfffe);
	}
}

WRITE16_HANDLER( atarist_blitter_count_x_w )
{
	blitter.xcount = data;
}

WRITE16_HANDLER( atarist_blitter_count_y_w )
{
	blitter.ycount = data;
}

WRITE16_HANDLER( atarist_blitter_op_w )
{
	if (ACCESSING_LSB16)
	{
		blitter.hop = (data >> 8) & 0x03;
	}
	else
	{
		blitter.op = data & 0x0f;
	}
}

WRITE16_HANDLER( atarist_blitter_ctrl_w )
{
	if (ACCESSING_LSB16)
	{
		blitter.ctrl = (data >> 8) & 0xef;

		if (!(blitter.ctrl & ATARIST_BLITTER_CTRL_BUSY))
		{
			if ((data >> 8) & ATARIST_BLITTER_CTRL_BUSY)
			{
				int nops = BLITTER_NOPS[blitter.op][blitter.hop]; // each NOP takes 4 cycles
				timer_set(ATTOTIME_IN_HZ((Y2/4)/(4*nops)), NULL, 0, atarist_blitter_tick);
			}
		}
	}
	else
	{
		blitter.skew = data & 0xcf;
	}
}

/* Video */

VIDEO_START( atarist )
{
	atarist_shifter_timer = timer_alloc(atarist_shifter_tick, NULL);
	atarist_glue_timer = timer_alloc(atarist_glue_tick, NULL);

	timer_adjust(atarist_shifter_timer, video_screen_get_time_until_pos(0,0,0), 0, ATTOTIME_IN_HZ(Y2/4)); // 125 ns
	timer_adjust(atarist_glue_timer, video_screen_get_time_until_pos(0,0,0), 0, ATTOTIME_IN_HZ(Y2/16)); // 500 ns

	memset(&shifter, 0, sizeof(shifter));

	state_save_register_global(shifter.base);
	state_save_register_global(shifter.ofs);
	state_save_register_global(shifter.sync);
	state_save_register_global(shifter.mode);
	state_save_register_global_array(shifter.palette);
	state_save_register_global(shifter.lineofs);
	state_save_register_global(shifter.pixelofs);
	state_save_register_global_array(shifter.rr);
	state_save_register_global_array(shifter.ir);
	state_save_register_global(shifter.bitplane);
	state_save_register_global(shifter.shift);
	state_save_register_global(shifter.h);
	state_save_register_global(shifter.v);
	state_save_register_global(shifter.de);

	state_save_register_global_array(blitter.halftone);
	state_save_register_global(blitter.src_inc_x);
	state_save_register_global(blitter.src_inc_y);
	state_save_register_global(blitter.dst_inc_x);
	state_save_register_global(blitter.dst_inc_y);
	state_save_register_global(blitter.src);
	state_save_register_global(blitter.dst);
	state_save_register_global(blitter.endmask1);
	state_save_register_global(blitter.endmask2);
	state_save_register_global(blitter.endmask3);
	state_save_register_global(blitter.xcount);
	state_save_register_global(blitter.ycount);
	state_save_register_global(blitter.xcountl);
	state_save_register_global(blitter.hop);
	state_save_register_global(blitter.op);
	state_save_register_global(blitter.ctrl);
	state_save_register_global(blitter.skew);

	atarist_set_screen_parameters();

	video_start_generic_bitmapped(machine);
}
