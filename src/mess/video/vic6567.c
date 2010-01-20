/***************************************************************************

    Video Interface Chip (6567R8 for NTSC system and 6569 for PAL system)
    PeT mess@utanet.at

    A part of the code (cycle routine and drawing routines) is a modified version of the vic ii emulation used in
    commodore 64 emulator "frodo" by Christian Bauer

    http://frodo.cebix.net/
    The rights on the source code remain at the author.
    It may not - not even in parts - used for commercial purposes without explicit written permission by the author.
    Permission to use it for non-commercial purposes is hereby granted als long as my copyright notice remains in the program.
    You are not allowed to use the source to create and distribute a modified version of Frodo.

***************************************************************************/
/* mos videochips
  vic (6560 NTSC, 6561 PAL)
  used in commodore vic20

  vic II
   6566 NTSC
    no dram refresh?
   6567 NTSC
   6569 PAL-B
   6572 PAL-N
   6573 PAL-M
   8562 NTSC
   8565 PAL
  used in commodore c64
  complete different to vic

  ted
   7360/8360 (NTSC-M, PAL-B by same chip ?)
   8365 PAL-N
   8366 PAL-M
  used in c16 c116 c232 c264 plus4 c364
  based on the vic II
  but no sprites and not register compatible
  includes timers, input port, sound generators
  memory interface, dram refresh, clock generation

  vic IIe
   8564 NTSC-M
   8566 PAL-B
   8569 PAL-N
  used in commodore c128
  vic II with some additional features
   3 programmable output pins k0 k1 k2

  vic III
   4567
  used in commodore c65 prototype
  vic II compatible (mode only?)
  several additional features
   different resolutions, more colors, ...
   (maybe like in the amiga graphic chip docu)

  vdc
   8563
   8568 (composite video and composite sync)
  second graphic chip in c128
  complete different to above chips
*/

#include <math.h>
#include <stdio.h>
#include <assert.h>

#include "emu.h"
#include "vic6567.h"
#include "vic4567.h"
#include "utils.h"

#ifdef UNUSED_FUNCTION
static TIMER_CALLBACK( line_timer_callback );
#endif
static TIMER_CALLBACK( PAL_timer_callback );
static TIMER_CALLBACK( NTSC_timer_callback );


/* lightpen values */
#include "includes/c64.h"

#define VERBOSE_LEVEL 0
#define DBG_LOG(N,M,A) \
	do { \
		if(VERBOSE_LEVEL >= N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s", attotime_to_double(timer_get_time(machine)), (char*) M ); \
			logerror A; \
		} \
	} while (0)

// Display
#define ROW25_YSTART 0x33
#define ROW25_YSTOP 0xfb
#define ROW24_YSTART 0x37
#define ROW24_YSTOP 0xf7

static UINT16 dy_start = ROW24_YSTART;
static UINT16 dy_stop = ROW24_YSTOP;

// GFX
static UINT8 draw_this_line = 0;
static UINT8 is_bad_line = 0;
static UINT8 bad_lines_enabled = 0;
static UINT8 display_state = 0;
static UINT8 char_data = 0;
static UINT8 gfx_data = 0;
static UINT8 color_data = 0;
static UINT8 last_char_data = 0;
static UINT8 matrix_line[40];						// Buffer for video line, read in Bad Lines
static UINT8 color_line[40];						// Buffer for color line, read in Bad Lines
static UINT8 vblanking = 0;
static UINT16 ml_index = 0;
static UINT8 rc = 0;
static UINT16 vc = 0;
static UINT16 vc_base = 0;
static UINT8 ref_cnt = 0;

// Sprites
static UINT8 spr_coll_buf[0x400];					// Buffer for sprite-sprite collisions and priorities
static UINT8 fore_coll_buf[0x400];					// Buffer for foreground-sprite collisions and priorities
static UINT8 spr_draw_data[8][4];					// Sprite data for drawing
static UINT8 spr_exp_y = 0;
static UINT8 spr_dma_on;
static UINT8 spr_draw = 0;
static UINT8 spr_disp_on = 0;
static UINT16 spr_ptr[8];
static UINT8 spr_data[8][4];
static UINT16 mc_base[8];						// Sprite data counter bases
static UINT16 mc[8];							// Sprite data counters

// Border
static UINT8 border_on = 0;
static UINT8 ud_border_on = 0;
static UINT8 border_on_sample[5];
static UINT8 border_color_sample[0x400 / 8];			// Samples of border color at each "displayed" cycle

// Cycles
static UINT64 first_ba_cycle;
static UINT8 cpu_suspended = 0;

#define VREFRESHINLINES			28

#define VIC2_YPOS				50
#define RASTERLINE_2_C64(a)		(a)
#define C64_2_RASTERLINE(a)		(a)
#define XPOS				(VIC2_STARTVISIBLECOLUMNS + (VIC2_VISIBLECOLUMNS - VIC2_HSIZE) / 2)
#define YPOS				(VIC2_STARTVISIBLELINES /* + (VIC2_VISIBLELINES - VIC2_VSIZE) / 2 */)
#define FIRSTLINE				10 /* 36 ((VIC2_VISIBLELINES - VIC2_VSIZE)/2) */
#define FIRSTCOLUMN			50

/* 2008-05 FP: lightpen code needs to read input port from c64.c and cbmb.c */
#define LIGHTPEN_BUTTON			(input_port_read(machine, "OTHER") & 0x04)
#define LIGHTPEN_X_VALUE		(input_port_read(machine, "LIGHTX") & ~0x01)
#define LIGHTPEN_Y_VALUE		(input_port_read(machine, "LIGHTY") & ~0x01)

/* lightpen delivers values from internal counters; they do not start with the visual area or frame area */
#define VIC2_MAME_XPOS			0
#define VIC2_MAME_YPOS			0
#define VIC6567_X_BEGIN			38
#define VIC6567_Y_BEGIN			-6			   /* first 6 lines after retrace not for lightpen! */
#define VIC6569_X_BEGIN			38
#define VIC6569_Y_BEGIN			-6
#define VIC2_X_BEGIN			(vic2.pal ? VIC6569_X_BEGIN : VIC6567_X_BEGIN)
#define VIC2_Y_BEGIN			(vic2.pal ? VIC6569_Y_BEGIN : VIC6567_Y_BEGIN)
#define VIC2_X_VALUE			((LIGHTPEN_X_VALUE+VIC2_X_BEGIN + VIC2_MAME_XPOS) / 2)
#define VIC2_Y_VALUE			((LIGHTPEN_Y_VALUE+VIC2_Y_BEGIN + VIC2_MAME_YPOS))

#define VIC2E_K0_LEVEL			(vic2.reg[0x2f] & 0x01)
#define VIC2E_K1_LEVEL			(vic2.reg[0x2f] & 0x02)
#define VIC2E_K2_LEVEL			(vic2.reg[0x2f] & 0x04)

/*#define VIC3_P5_LEVEL (vic2.reg[0x30] & 0x20) */
#define VIC3_BITPLANES			(vic2.reg[0x31] & 0x10)
#define VIC3_80COLUMNS			(vic2.reg[0x31] & 0x80)
#define VIC3_LINES			((vic2.reg[0x31] & 0x19) == 0x19 ? 400 : 200)
#define VIC3_BITPLANES_WIDTH 		(vic2.reg[0x31] & 0x80 ? 640 : 320)

/*#define VIC2E_TEST (vic2[0x30] & 2) */
#define DOUBLE_CLOCK			(vic2.reg[0x30] & 0x01)

/* sprites 0 .. 7 */
#define SPRITE_BASE_X_SIZE		24
#define SPRITE_BASE_Y_SIZE		21
#define SPRITEON(nr)			(vic2.reg[0x15] & (1 << nr))
#define SPRITE_Y_EXPAND(nr)		(vic2.reg[0x17] & (1 << nr))
#define SPRITE_Y_SIZE(nr)		(SPRITE_Y_EXPAND(nr) ? 2 * 21 : 21)
#define SPRITE_X_EXPAND(nr)		(vic2.reg[0x1d] & (1 << nr))
#define SPRITE_X_SIZE(nr)		(SPRITE_X_EXPAND(nr) ? 2 * 24 : 24)
#define SPRITE_X_POS(nr)		((vic2.reg[(nr) * 2] | (vic2.reg[0x10] & (1 << (nr)) ? 0x100 : 0)) - 24 + XPOS)
#define SPRITE_X_POS2(nr)		(vic2.reg[(nr) * 2] | (vic2.reg[0x10] & (1 << (nr)) ? 0x100 : 0))
#define SPRITE_Y_POS(nr)		(vic2.reg[1+2*(nr)] - 50 + YPOS)
#define SPRITE_Y_POS2(nr)		(vic2.reg[1 + 2 * (nr)])
#define SPRITE_MULTICOLOR(nr)		(vic2.reg[0x1c] & (1 << nr))
#define SPRITE_PRIORITY(nr)		(vic2.reg[0x1b] & (1 << nr))
#define SPRITE_MULTICOLOR1		(vic2.reg[0x25] & 0x0f)
#define SPRITE_MULTICOLOR2		(vic2.reg[0x26] & 0x0f)
#define SPRITE_COLOR(nr)		(vic2.reg[0x27+nr] & 0x0f)
#define SPRITE_ADDR(nr)			(vic2.videoaddr | 0x3f8 | nr)
#define SPRITE_BG_COLLISION(nr)	(vic2.reg[0x1f] & (1 << nr))
#define SPRITE_COLLISION(nr)		(vic2.reg[0x1e] & (1 << nr))
#define SPRITE_SET_BG_COLLISION(nr) (vic2.reg[0x1f] |= (1 << nr))
#define SPRITE_SET_COLLISION(nr)	(vic2.reg[0x1e] |= (1 << nr))
#define SPRITE_COLL			(vic2.reg[0x1e])
#define SPRITE_BG_COLL			(vic2.reg[0x1f])

#define GFXMODE				((vic2.reg[0x11] & 0x60) | (vic2.reg[0x16] & 0x10)) >> 4
#define SCREENON				(vic2.reg[0x11] & 0x10)
#define VERTICALPOS			(vic2.reg[0x11] & 0x07)
#define HORIZONTALPOS			(vic2.reg[0x16] & 0x07)
#define ECMON				(vic2.reg[0x11] & 0x40)
#define HIRESON				(vic2.reg[0x11] & 0x20)
#define MULTICOLORON			(vic2.reg[0x16] & 0x10)
#define LINES25				(vic2.reg[0x11] & 0x08)		   /* else 24 Lines */
#define LINES				(LINES25 ? 25 : 24)
#define YSIZE				(LINES * 8)
#define COLUMNS40				(vic2.reg[0x16] & 0x08)		   /* else 38 Columns */
#define COLUMNS				(COLUMNS40 ? 40 : 38)
#define XSIZE				(COLUMNS * 8)

#define VIDEOADDR				((vic2.reg[0x18] & 0xf0) << (10 - 4))
#define CHARGENADDR			((vic2.reg[0x18] & 0x0e) << 10)
#define BITMAPADDR			((data & 0x08) << 10)

#define RASTERLINE			(((vic2.reg[0x11] & 0x80) << 1) | vic2.reg[0x12])

#define FRAMECOLOR			(vic2.reg[0x20] & 0x0f)
#define BACKGROUNDCOLOR			(vic2.reg[0x21] & 0x0f)
#define MULTICOLOR1			(vic2.reg[0x22] & 0x0f)
#define MULTICOLOR2			(vic2.reg[0x23] & 0x0f)
#define FOREGROUNDCOLOR			(vic2.reg[0x24] & 0x0f)

const unsigned char vic2_palette[] =
{
/* black, white, red, cyan */
/* purple, green, blue, yellow */
/* orange, brown, light red, dark gray, */
/* medium gray, light green, light blue, light gray */
/* taken from the vice emulator */
	0x00, 0x00, 0x00,  0xfd, 0xfe, 0xfc,  0xbe, 0x1a, 0x24,  0x30, 0xe6, 0xc6,
	0xb4, 0x1a, 0xe2,  0x1f, 0xd2, 0x1e,  0x21, 0x1b, 0xae,  0xdf, 0xf6, 0x0a,
	0xb8, 0x41, 0x04,  0x6a, 0x33, 0x04,  0xfe, 0x4a, 0x57,  0x42, 0x45, 0x40,
	0x70, 0x74, 0x6f,  0x59, 0xfe, 0x59,  0x5f, 0x53, 0xfe,  0xa4, 0xa7, 0xa2
};

static struct {
	UINT8 reg[0x80];
	int pal;
	int vic2e;								/* version with some port lines */
	int vic3;
	int on;								/* rastering of the screen */

	int (*dma_read)(running_machine *, int);
	int (*dma_read_color)(running_machine *, int);
	void (*interrupt) (running_machine *, int);
	void (*port_changed)(running_machine *, int);

	int lines;

	UINT16 chargenaddr, videoaddr, bitmapaddr;

	bitmap_t *bitmap;
	int x_begin, x_end;
	int y_begin, y_end;

	UINT16 c64_bitmap[2], bitmapmulti[4], mono[2], multi[4], ecmcolor[2], colors[4], spritemulti[4];

	int lastline, rasterline, raster_mod;
	UINT64 rasterline_start_cpu_cycles, totalcycles, cycles_counter;
	UINT8 rasterX, cycle;
	UINT16 raster_x;
	UINT16 graphic_x;

	/* background/foreground for sprite collision */
	UINT8 *screen[216], shift[216];

	/* convert multicolor byte to background/foreground for sprite collision */
	UINT8 foreground[256];
	UINT16 expandx[256];
	UINT16 expandx_multi[256];

	/* converts sprite multicolor info to info for background collision checking */
	UINT8 multi_collision[256];

	struct
	{
		int on, x, y, xexpand, yexpand;

		int repeat;							/* expand, line once drawn */
		int line;							/* 0 not painting, else painting */

		/* buffer for currently painted line */
		int paintedline[8];
		UINT8 bitmap[8][SPRITE_BASE_X_SIZE * 2 / 8 + 1	/*for simplier sprite collision detection*/];
	} sprites[8];
} vic2;

INLINE int vic2_getforeground (register int y, register int x)
{
    return ((vic2.screen[y][x >> 3] << 8)
	    | (vic2.screen[y][(x >> 3) + 1])) >> (8 - (x & 7));
}

INLINE int vic2_getforeground16 (register int y, register int x)
{
	return ((vic2.screen[y][x >> 3] << 16)
			| (vic2.screen[y][(x >> 3) + 1] << 8)
			| (vic2.screen[y][(x >> 3) + 2])) >> (8 - (x & 7));
}

#if 0
static void vic2_setforeground (int y, int x, int value)
{
	vic2.screen[y][x >> 3] = value;
}
#endif

static void vic2_drawlines (running_machine *machine, int first, int last, int start_x, int end_x);

void vic6567_init (int chip_vic2e, int pal,
				   int (*dma_read) (running_machine *, int), int (*dma_read_color) (running_machine *, int),
				   void (*irq) (running_machine *, int))
{
	memset(&vic2, 0, sizeof(vic2));

	vic2.cycles_counter = 0;				// total cycles
	vic2.cycle = 63;						// first cycle
	vic2.pal = pal;
	vic2.lines = VIC2_LINES;

	vic2.dma_read = dma_read;
	vic2.dma_read_color = dma_read_color;
	vic2.interrupt = irq;
	vic2.vic2e = chip_vic2e;

	vic2.on = TRUE;
}

void vic2_set_rastering(int onoff)
{
	vic2.on=onoff;
}

int vic2e_k0_r (void)
{
	return VIC2E_K0_LEVEL;
}

int vic2e_k1_r (void)
{
	return VIC2E_K1_LEVEL;
}

int vic2e_k2_r (void)
{
	return VIC2E_K2_LEVEL;
}

#if 0
int vic3_p5_r (void)
{
	return VIC3_P5_LEVEL;
}
#endif

static void vic2_set_interrupt(running_machine *machine, int mask)
{
	if (((vic2.reg[0x19] ^ mask) & vic2.reg[0x1a] & 0xf))
	{
		if (!(vic2.reg[0x19] & 0x80))
		{
			DBG_LOG (2, "vic2", ("irq start %.2x\n", mask));
			vic2.reg[0x19] |= 0x80;
			vic2.interrupt (machine, 1);
		}
	}
	vic2.reg[0x19] |= mask;
}

static void vic2_clear_interrupt (running_machine *machine, int mask)
{
	vic2.reg[0x19] &= ~mask;
	if ((vic2.reg[0x19] & 0x80) && !(vic2.reg[0x19] & vic2.reg[0x1a] & 0xf))
	{
		DBG_LOG (2, "vic2", ("irq end %.2x\n", mask));
		vic2.reg[0x19] &= ~0x80;
		vic2.interrupt (machine, 0);
	}
}

void vic2_lightpen_write (int level)
{
	/* calculate current position, write it and raise interrupt */
}

static TIMER_CALLBACK(vic2_timer_timeout)
{
	int which = param;
	DBG_LOG (3, "vic2 ", ("timer %d timeout\n", which));
	switch (which)
	{
	case 1:						   /* light pen */
		/* and diode must recognize light */
		if (1)
		{
			vic2.reg[0x13] = VIC2_X_VALUE;
			vic2.reg[0x14] = VIC2_Y_VALUE;
		}
		vic2_set_interrupt(machine, 8);
		break;
	}
}

INTERRUPT_GEN( vic2_frame_interrupt )
{
}

WRITE8_HANDLER ( vic2_port_w )
{
	running_machine *machine = space->machine;
	DBG_LOG (2, "vic write", ("%.2x:%.2x\n", offset, data));
	offset &= 0x3f;

	switch (offset)
	{
	case 0x01:
	case 0x03:
	case 0x05:
	case 0x07:
	case 0x09:
	case 0x0b:
	case 0x0d:
	case 0x0f:
									/* sprite y positions */
		if (vic2.reg[offset] != data)
		{
			vic2.reg[offset] = data;
			vic2.sprites[offset / 2].y = SPRITE_Y_POS(offset / 2);
		}
		break;

	case 0x00:
	case 0x02:
	case 0x04:
	case 0x06:
	case 0x08:
	case 0x0a:
	case 0x0c:
	case 0x0e:
									/* sprite x positions */
		if (vic2.reg[offset] != data)
		{
			vic2.reg[offset] = data;
			vic2.sprites[offset / 2].x = SPRITE_X_POS(offset / 2);
		}
		break;

	case 0x10:							/* sprite x positions */
		if (vic2.reg[offset] != data)
		{
			vic2.reg[offset] = data;
			vic2.sprites[0].x = SPRITE_X_POS(0);
			vic2.sprites[1].x = SPRITE_X_POS(1);
			vic2.sprites[2].x = SPRITE_X_POS(2);
			vic2.sprites[3].x = SPRITE_X_POS(3);
			vic2.sprites[4].x = SPRITE_X_POS(4);
			vic2.sprites[5].x = SPRITE_X_POS(5);
			vic2.sprites[6].x = SPRITE_X_POS(6);
			vic2.sprites[7].x = SPRITE_X_POS(7);
		}
		break;

	case 0x17:							/* sprite y size */
		spr_exp_y |= ~data;
		if (vic2.reg[offset] != data)
		{
			vic2.reg[offset] = data;
		}
		break;

	case 0x1d:							/* sprite x size */
		if (vic2.reg[offset] != data)
		{
			vic2.reg[offset] = data;
		}
		break;

	case 0x1b:							/* sprite background priority */
		if (vic2.reg[offset] != data)
		{
			vic2.reg[offset] = data;
		}
		break;

	case 0x1c:							/* sprite multicolor mode select */
		if (vic2.reg[offset] != data)
		{
			vic2.reg[offset] = data;
		}
		break;

	case 0x27:
	case 0x28:
	case 0x29:
	case 0x2a:
	case 0x2b:
	case 0x2c:
	case 0x2d:
	case 0x2e:
									/* sprite colors */
		if (vic2.reg[offset] != data)
		{
			vic2.reg[offset] = data;
		}
		break;

	case 0x25:							/* sprite multicolor */
		if (vic2.reg[offset] != data)
		{
			vic2.reg[offset] = data;
			vic2.spritemulti[1] = SPRITE_MULTICOLOR1;
		}
		break;

	case 0x26:							/* sprite multicolor */
		if (vic2.reg[offset] != data)
		{
			vic2.reg[offset] = data;
			vic2.spritemulti[3] = SPRITE_MULTICOLOR2;
		}
		break;

	case 0x19:
		vic2_clear_interrupt(space->machine, data & 0x0f);
		break;

	case 0x1a:							/* irq mask */
		vic2.reg[offset] = data;
		vic2_set_interrupt(space->machine, 0); 	// beamrider needs this
		break;

	case 0x11:
		if (vic2.reg[offset] != data)
		{
			vic2.reg[offset] = data;
			if (LINES25)
			{
				vic2.y_begin = 0;
				vic2.y_end = vic2.y_begin + 200;
			}
			else
			{
				vic2.y_begin = 4;
				vic2.y_end = vic2.y_begin + 192;
			}
			if (data & 8) {
				dy_start = ROW25_YSTART;
				dy_stop = ROW25_YSTOP;
			} else {
				dy_start = ROW24_YSTART;
				dy_stop = ROW24_YSTOP;
			}
		}
		break;

	case 0x12:
		if (data != vic2.reg[offset])
		{
			vic2.reg[offset] = data;
		}
		break;

	case 0x16:
		if (vic2.reg[offset] != data)
		{
			vic2.reg[offset] = data;
			vic2.x_begin = HORIZONTALPOS;
			vic2.x_end = vic2.x_begin + 320;
		}
		break;

	case 0x18:
		if (vic2.reg[offset] != data)
		{
			vic2.reg[offset] = data;
			vic2.videoaddr = VIDEOADDR;
			vic2.chargenaddr = CHARGENADDR;
			vic2.bitmapaddr = BITMAPADDR;
		}
		break;

	case 0x21:							/* background color */
		if (vic2.reg[offset] != data)
		{
			vic2.reg[offset] = data;
			vic2.mono[0] = vic2.bitmapmulti[0] = vic2.multi[0] = vic2.colors[0] = BACKGROUNDCOLOR;
		}
		break;

	case 0x22:							/* background color 1 */
		if (vic2.reg[offset] != data)
		{
			vic2.reg[offset] = data;
			vic2.multi[1] = vic2.colors[1] = MULTICOLOR1;
		}
		break;

	case 0x23:							/* background color 2 */
		if (vic2.reg[offset] != data)
		{
			vic2.reg[offset] = data;
			vic2.multi[2] = vic2.colors[2] = MULTICOLOR2;
		}
		break;

	case 0x24:							/* background color 3 */
		if (vic2.reg[offset] != data)
		{
			vic2.reg[offset] = data;
			vic2.colors[3] = FOREGROUNDCOLOR;
		}
		break;

	case 0x20:							/* framecolor */
		if (vic2.reg[offset] != data)
		{
			vic2.reg[offset] = data;
		}
		break;

	case 0x2f:
		if (vic2.vic2e)
		{
			DBG_LOG (2, "vic write", ("%.2x:%.2x\n", offset, data));
			vic2.reg[offset] = data;
		}
		break;

	case 0x30:
		if (vic2.vic2e)
		{
			vic2.reg[offset] = data;
		}
		break;

	case 0x31:
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
	case 0x38:
	case 0x39:
	case 0x3a:
	case 0x3b:
	case 0x3c:
	case 0x3d:
	case 0x3e:
	case 0x3f:
		vic2.reg[offset] = data;
		DBG_LOG (2, "vic write", ("%.2x:%.2x\n", offset, data));
		break;

	default:
		vic2.reg[offset] = data;
		break;
	}
}

READ8_HANDLER ( vic2_port_r )
{
	running_machine *machine = space->machine;
	int val = 0;
	offset &= 0x3f;

	switch (offset)
	{
	case 0x11:
		val = (vic2.reg[offset] & ~0x80) | ((vic2.rasterline & 0x100) >> 1);
		break;

	case 0x12:
		val = vic2.rasterline & 0xff;
		break;

	case 0x16:
		val = vic2.reg[offset] | 0xc0;
		break;

	case 0x18:
		val = vic2.reg[offset] | 0x01;
		break;

	case 0x19:							/* interrupt flag register */
		/* vic2_clear_interrupt(0xf); */
		val = vic2.reg[offset] | 0x70;
		break;

	case 0x1a:
		val = vic2.reg[offset] | 0xf0;
		break;

	case 0x1e:							/* sprite to sprite collision detect */
		val = vic2.reg[offset];
		vic2.reg[offset] = 0;
		vic2_clear_interrupt(space->machine, 4);
		break;

	case 0x1f:							/* sprite to background collision detect */
		val = vic2.reg[offset];
		vic2.reg[offset] = 0;
		vic2_clear_interrupt(space->machine, 2);
		break;

	case 0x20:
	case 0x21:
	case 0x22:
	case 0x23:
	case 0x24:
		val = vic2.reg[offset];
		break;

	case 0x00:
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
	case 0x08:
	case 0x09:
	case 0x0a:
	case 0x0b:
	case 0x0c:
	case 0x0d:
	case 0x0e:
	case 0x0f:
	case 0x10:
	case 0x17:
	case 0x1b:
	case 0x1c:
	case 0x1d:
	case 0x25:
	case 0x26:
	case 0x27:
	case 0x28:
	case 0x29:
	case 0x2a:
	case 0x2b:
	case 0x2c:
	case 0x2d:
	case 0x2e:
		val = vic2.reg[offset];
		break;

	case 0x2f:
	case 0x30:
		if (vic2.vic2e)
		{
			val = vic2.reg[offset];
			DBG_LOG (2, "vic read", ("%.2x:%.2x\n", offset, val));
		}
		else
			val = 0xff;
		break;

	case 0x31:
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
	case 0x38:
	case 0x39:
	case 0x3a:
	case 0x3b:
	case 0x3c:
	case 0x3d:
	case 0x3e:
	case 0x3f:							/* not used */
		// val = vic2.reg[offset]; //
		val = 0xff;
		DBG_LOG (2, "vic read", ("%.2x:%.2x\n", offset, val));
		break;

	default:
		val = vic2.reg[offset];
	}

	if ((offset != 0x11) && (offset != 0x12))
		DBG_LOG (2, "vic read", ("%.2x:%.2x\n", offset, val));
	return val;
}

VIDEO_START( vic2 )
{
	int i;
	const device_config *screen = video_screen_first(machine->config);
	int width = video_screen_get_width(screen);
	int height = video_screen_get_height(screen);

	for (i = 0; i < ARRAY_LENGTH(mc); i++)
		mc[i] = 63;

	vic2.bitmap = auto_bitmap_alloc(machine, width, height, BITMAP_FORMAT_INDEXED16);

	if (vic2.vic3) {
		vic2.screen[0] = auto_alloc_array(machine, UINT8, 216 * 656 / 8);

		for (i = 1; i < 216; i++)
			vic2.screen[i] = vic2.screen[i - 1] + 656 / 8;
	} else {
		vic2.screen[0] = auto_alloc_array(machine, UINT8, 216 * 336 / 8);

		for (i = 1; i < 216; i++)
			vic2.screen[i] = vic2.screen[i - 1] + 336 / 8;
	}

	for (i = 0; i < 256; i++)
	{
		vic2.foreground[i] = 0;
		if ((i & 3) > 1)
			vic2.foreground[i] |= 0x3;
		if ((i & 0xc) > 0x4)
			vic2.foreground[i] |= 0xc;
		if ((i & 0x30) > 0x10)
			vic2.foreground[i] |= 0x30;
		if ((i & 0xc0) > 0x40)
			vic2.foreground[i] |= 0xc0;
	}
	for (i = 0; i < 256; i++)
	{
		vic2.expandx[i] = 0;
		if (i & 1)
			vic2.expandx[i] |= 3;
		if (i & 2)
			vic2.expandx[i] |= 0xc;
		if (i & 4)
			vic2.expandx[i] |= 0x30;
		if (i & 8)
			vic2.expandx[i] |= 0xc0;
		if (i & 0x10)
			vic2.expandx[i] |= 0x300;
		if (i & 0x20)
			vic2.expandx[i] |= 0xc00;
		if (i & 0x40)
			vic2.expandx[i] |= 0x3000;
		if (i & 0x80)
			vic2.expandx[i] |= 0xc000;
	}
	for (i = 0; i < 256; i++)
	{
		vic2.expandx_multi[i] = 0;
		if (i & 1)
			vic2.expandx_multi[i] |= 5;
		if (i & 2)
			vic2.expandx_multi[i] |= 0xa;
		if (i & 4)
			vic2.expandx_multi[i] |= 0x50;
		if (i & 8)
			vic2.expandx_multi[i] |= 0xa0;
		if (i & 0x10)
			vic2.expandx_multi[i] |= 0x500;
		if (i & 0x20)
			vic2.expandx_multi[i] |= 0xa00;
		if (i & 0x40)
			vic2.expandx_multi[i] |= 0x5000;
		if (i & 0x80)
			vic2.expandx_multi[i] |= 0xa000;
	}
	for (i = 0; i < 256; i++)
	{
		vic2.multi_collision[i] = 0;
		if (i & 3)
			vic2.multi_collision[i] |= 3;
		if (i & 0xc)
			vic2.multi_collision[i] |= 0xc;
		if (i & 0x30)
			vic2.multi_collision[i] |= 0x30;
		if (i & 0xc0)
			vic2.multi_collision[i] |= 0xc0;
	}

	// from 0 to 311 (0 first, PAL) or from 0 to 261 (? first, NTSC 6567R56A) or from 0 to 262 (? first, NTSC 6567R8)
	vic2.rasterline = 0; // VIC2_LINES - 1;

	// from 1 to 63 (PAL) or from 1 to 65 (NTSC)
	vic2.rasterX = 63;

	// immediately call the timer to handle the first line
	if (vic2.pal)
		timer_set(machine, cputag_clocks_to_attotime(machine, "maincpu", 0), NULL, 0, PAL_timer_callback);
	else
		timer_set(machine, cputag_clocks_to_attotime(machine, "maincpu", 0), NULL, 0, NTSC_timer_callback);
}

static void vic2_draw_character (running_machine *machine, int ybegin, int yend, int ch, int yoff, int xoff, UINT16 *color, int start_x, int end_x)
{
	int y, code;

	for (y = ybegin; y <= yend; y++)
	{
		code = vic2.dma_read(machine, vic2.chargenaddr + ch * 8 + y);
		vic2.screen[y + yoff][xoff >> 3] = code;
		if ((xoff + 0 > start_x) && (xoff + 0 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 0) = color[code >> 7];
		if ((xoff + 1 > start_x) && (xoff + 0 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 1) = color[(code >> 6) & 1];
		if ((xoff + 2 > start_x) && (xoff + 0 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 2) = color[(code >> 5) & 1];
		if ((xoff + 3 > start_x) && (xoff + 0 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 3) = color[(code >> 4) & 1];
		if ((xoff + 4 > start_x) && (xoff + 0 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 4) = color[(code >> 3) & 1];
		if ((xoff + 5 > start_x) && (xoff + 0 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 5) = color[(code >> 2) & 1];
		if ((xoff + 6 > start_x) && (xoff + 0 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 6) = color[(code >> 1) & 1];
		if ((xoff + 7 > start_x) && (xoff + 0 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 7) = color[code & 1];
	}
}

static void vic2_draw_character_multi(running_machine *machine, int ybegin, int yend, int ch, int yoff, int xoff, int start_x, int end_x )
{
	int y, code;

	for (y = ybegin; y <= yend; y++)
	{
		code = vic2.dma_read(machine, vic2.chargenaddr + ch * 8 + y);
		vic2.screen[y + yoff][xoff >> 3] = vic2.foreground[code];
		if ((xoff + 0 > start_x) && (xoff + 0 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 0) = vic2.multi[code >> 6];
		if ((xoff + 1 > start_x) && (xoff + 1 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 1) = vic2.multi[code >> 6];
		if ((xoff + 2 > start_x) && (xoff + 2 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 2) = vic2.multi[(code >> 4) & 3];
		if ((xoff + 3 > start_x) && (xoff + 3 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 3) = vic2.multi[(code >> 4) & 3];
		if ((xoff + 4 > start_x) && (xoff + 4 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 4) = vic2.multi[(code >> 2) & 3];
		if ((xoff + 5 > start_x) && (xoff + 5 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 5) = vic2.multi[(code >> 2) & 3];
		if ((xoff + 6 > start_x) && (xoff + 6 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 6) = vic2.multi[code & 3];
		if ((xoff + 7 > start_x) && (xoff + 7 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 7) = vic2.multi[code & 3];
	}
}

static void vic2_draw_bitmap(running_machine *machine, int ybegin, int yend, int ch, int yoff, int xoff, int start_x, int end_x)
{
	int y, code;

	for (y = ybegin; y <= yend; y++)
	{
		code = vic2.dma_read(machine, (vic2.chargenaddr&0x2000) + ch * 8 + y);
		vic2.screen[y + yoff][xoff >> 3] = code;
		if ((xoff + 0 > start_x) && (xoff + 0 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 0) = vic2.c64_bitmap[code >> 7];
		if ((xoff + 1 > start_x) && (xoff + 1 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 1) = vic2.c64_bitmap[(code >> 6) & 1];
		if ((xoff + 2 > start_x) && (xoff + 2 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 2) = vic2.c64_bitmap[(code >> 5) & 1];
		if ((xoff + 3 > start_x) && (xoff + 3 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 3) = vic2.c64_bitmap[(code >> 4) & 1];
		if ((xoff + 4 > start_x) && (xoff + 4 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 4) = vic2.c64_bitmap[(code >> 3) & 1];
		if ((xoff + 5 > start_x) && (xoff + 5 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 5) = vic2.c64_bitmap[(code >> 2) & 1];
		if ((xoff + 6 > start_x) && (xoff + 6 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 6) = vic2.c64_bitmap[(code >> 1) & 1];
		if ((xoff + 7 > start_x) && (xoff + 7 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 7) = vic2.c64_bitmap[code & 1];
	}
}

static void vic2_draw_bitmap_multi(running_machine *machine, int ybegin, int yend, int ch, int yoff, int xoff, int start_x, int end_x)
{
	int y, code;

	for (y = ybegin; y <= yend; y++)
	{
		code = vic2.dma_read(machine, (vic2.chargenaddr&0x2000) + ch * 8 + y);
		vic2.screen[y + yoff][xoff >> 3] = vic2.foreground[code];
		if ((xoff + 0 > start_x) && (xoff + 0 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 0) = vic2.bitmapmulti[code >> 6];
		if ((xoff + 1 > start_x) && (xoff + 1 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 1) = vic2.bitmapmulti[code >> 6];
		if ((xoff + 2 > start_x) && (xoff + 2 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 2) = vic2.bitmapmulti[(code >> 4) & 3];
		if ((xoff + 3 > start_x) && (xoff + 3 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 3) = vic2.bitmapmulti[(code >> 4) & 3];
		if ((xoff + 4 > start_x) && (xoff + 4 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 4) = vic2.bitmapmulti[(code >> 2) & 3];
		if ((xoff + 5 > start_x) && (xoff + 5 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 5) = vic2.bitmapmulti[(code >> 2) & 3];
		if ((xoff + 6 > start_x) && (xoff + 6 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 6) = vic2.bitmapmulti[code & 3];
		if ((xoff + 7 > start_x) && (xoff + 7 < end_x)) *BITMAP_ADDR16(vic2.bitmap, y + yoff + FIRSTLINE, xoff + 7) = vic2.bitmapmulti[code & 3];
	}
}

static void vic2_draw_sprite_code_multi(running_machine *machine, int y, int xbegin, int code, int prior, int start_x, int end_x)
{
	register int x, mask, shift;

	if ((y < YPOS) || (y >= (VIC2_STARTVISIBLELINES + VIC2_VISIBLELINES)) || (xbegin <= 1) || (xbegin >= (VIC2_STARTVISIBLECOLUMNS + VIC2_VISIBLECOLUMNS)))
		{
			return;
		}

	for (x = 0, mask = 0xc0, shift = 6; x < 8; x += 2, mask >>= 2, shift -= 2)
	{
		if (code & mask)
		{
			switch ((prior & mask) >> shift)
			{
			case 1:
				if ((xbegin + x + 1 > start_x) && (xbegin + x + 1 < end_x))
					*BITMAP_ADDR16(vic2.bitmap, y + FIRSTLINE, xbegin + x + 1) = vic2.spritemulti[(code >> shift) & 3];
				break;
			case 2:
				if ((xbegin + x > start_x) && (xbegin + x < end_x))
					*BITMAP_ADDR16(vic2.bitmap, y + FIRSTLINE, xbegin + x) = vic2.spritemulti[(code >> shift) & 3];
				break;
			case 3:
				if ((xbegin + x > start_x) && (xbegin + x < end_x))
					*BITMAP_ADDR16(vic2.bitmap, y + FIRSTLINE, xbegin + x) = vic2.spritemulti[(code >> shift) & 3];
				if ((xbegin + x + 1> start_x) && (xbegin + x + 1< end_x))
					*BITMAP_ADDR16(vic2.bitmap, y + FIRSTLINE, xbegin + x + 1) = vic2.spritemulti[(code >> shift) & 3];
				break;
			}
		}
	}
}

static void vic2_draw_sprite_code (int y, int xbegin, int code, int color, int start_x, int end_x)
{
	register int mask, x;

	if ((y < YPOS) || (y >= (VIC2_STARTVISIBLELINES + VIC2_VISIBLELINES)) || (xbegin <= 1) || (xbegin >= (VIC2_STARTVISIBLECOLUMNS + VIC2_VISIBLECOLUMNS)))
		return;

	for (x = 0, mask = 0x80; x < 8; x++, mask >>= 1)
	{
		if (code & mask)
		{
			if ((xbegin + x > start_x) && (xbegin + x < end_x))
				*BITMAP_ADDR16(vic2.bitmap, y + FIRSTLINE, xbegin + x) = color;
		}
	}
}

static void vic2_sprite_collision (running_machine *machine, int nr, int y, int x, int mask)
{
	int i, value, xdiff;

	for (i = 7; i > nr; i--)
	{
		if (!SPRITEON (i)
			|| !vic2.sprites[i].paintedline[y]
			|| (SPRITE_COLLISION (i) && SPRITE_COLLISION (nr)))
			continue;
		if ((x + 7 < SPRITE_X_POS (i))
			|| (x >= SPRITE_X_POS (i) + SPRITE_X_SIZE (i)))
			continue;
		xdiff = x - SPRITE_X_POS (i);
		if ((x & 7) == (SPRITE_X_POS (i) & 7))
			value = vic2.sprites[i].bitmap[y][xdiff >> 3];
		else if (xdiff < 0)
			value = vic2.sprites[i].bitmap[y][0] >> (-xdiff);
		else {
			UINT8 *vp = vic2.sprites[i].bitmap[y]+(xdiff>>3);
			value = ((vp[1] | (*vp << 8)) >> (8 - (xdiff&7) )) & 0xff;
		}
		if (value & mask)
		{
			SPRITE_SET_COLLISION (i);
			SPRITE_SET_COLLISION (nr);
			vic2_set_interrupt(machine, 4);
		}
	}
}

static void vic2_draw_sprite_multi(running_machine *machine, int nr, int yoff, int ybegin, int yend, int start_x, int end_x)
{
	int y, i, prior, addr, xbegin, collision;
	int value, value2, value3 = 0, bg, color[2];

	xbegin = SPRITE_X_POS (nr);
	addr = vic2.dma_read(machine, SPRITE_ADDR (nr)) << 6;
	vic2.spritemulti[2] = SPRITE_COLOR (nr);
	prior = SPRITE_PRIORITY (nr);
	collision = SPRITE_BG_COLLISION (nr);
	color[0] = 0;
	color[1] = 1;

	if (SPRITE_X_EXPAND (nr))
	{
		for (y = ybegin; y <= yend; y++)
		{
			vic2.sprites[nr].paintedline[y] = 1;
			for (i = 0; i < 3; i++)
			{
				value = vic2.expandx_multi[bg = vic2.dma_read(machine, addr + vic2.sprites[nr].line * 3 + i)];
				value2 = vic2.expandx[vic2.multi_collision[bg]];
				vic2.sprites[nr].bitmap[y][i*2] = value2 >> 8;
				vic2.sprites[nr].bitmap[y][i*2+1] = value2 & 0xff;
				vic2_sprite_collision(machine, nr, y, xbegin + i * 16, value2 >> 8);
				vic2_sprite_collision(machine, nr, y, xbegin + i * 16 + 8, value2 & 0xff);
				if (prior || !collision)
				{
					value3 = vic2_getforeground16 (yoff + y, xbegin + i * 16 - 7);
				}
				if (!collision && (value2 & value3))
				{
					collision = 1;
					SPRITE_SET_BG_COLLISION (nr);
					vic2_set_interrupt(machine, 2);
				}
				if (prior)
				{
					vic2_draw_sprite_code_multi (machine, yoff + y, xbegin + i * 16, value >> 8, (value3 >> 8) ^ 0xff, start_x, end_x);
					vic2_draw_sprite_code_multi (machine, yoff + y, xbegin + i * 16 + 8, value & 0xff, (value3 & 0xff) ^ 0xff, start_x, end_x);
				}
				else
				{
					vic2_draw_sprite_code_multi (machine, yoff + y, xbegin + i * 16, value >> 8, 0xff, start_x, end_x);
					vic2_draw_sprite_code_multi (machine, yoff + y, xbegin + i * 16 + 8, value & 0xff, 0xff, start_x, end_x);
				}
			}
			vic2.sprites[nr].bitmap[y][i*2]=0; //easier sprite collision detection
			if (SPRITE_Y_EXPAND (nr))
			{
				if (vic2.sprites[nr].repeat)
				{
					vic2.sprites[nr].line++;
					vic2.sprites[nr].repeat = 0;
				}
				else
					vic2.sprites[nr].repeat = 1;
			}
			else
			{
				vic2.sprites[nr].line++;
			}
		}
	}
	else
	{
		for (y = ybegin; y <= yend; y++)
		{
			vic2.sprites[nr].paintedline[y] = 1;
			for (i = 0; i < 3; i++)
			{
				value = vic2.dma_read(machine, addr + vic2.sprites[nr].line * 3 + i);
				vic2.sprites[nr].bitmap[y][i] =
					value2 = vic2.multi_collision[value];
				vic2_sprite_collision(machine, nr, y, xbegin + i * 8, value2);
				if (prior || !collision)
				{
					value3 = vic2_getforeground (yoff + y, xbegin + i * 8 - 7);
				}
				if (!collision && (value2 & value3))
				{
					collision = 1;
					SPRITE_SET_BG_COLLISION (nr);
					vic2_set_interrupt(machine, 2);
				}
				if (prior)
				{
					vic2_draw_sprite_code_multi(machine, yoff + y, xbegin + i * 8, value, value3 ^ 0xff, start_x, end_x);
				}
				else
				{
					vic2_draw_sprite_code_multi(machine, yoff + y, xbegin + i * 8, value, 0xff, start_x, end_x);
				}
			}
			vic2.sprites[nr].bitmap[y][i]=0; //easier sprite collision detection
			if (SPRITE_Y_EXPAND (nr))
			{
				if (vic2.sprites[nr].repeat)
				{
					vic2.sprites[nr].line++;
					vic2.sprites[nr].repeat = 0;
				}
				else
					vic2.sprites[nr].repeat = 1;
			}
			else
			{
				vic2.sprites[nr].line++;
			}
		}
	}
}

static void vic2_draw_sprite(running_machine *machine, int nr, int yoff, int ybegin, int yend, int start_x, int end_x)
{
	int y, i, addr, xbegin, color, prior, collision;
	int value, value3 = 0;

	xbegin = SPRITE_X_POS (nr);
	addr = vic2.dma_read(machine, SPRITE_ADDR (nr)) << 6;
	color = SPRITE_COLOR (nr);
	prior = SPRITE_PRIORITY (nr);
	collision = SPRITE_BG_COLLISION (nr);

	if (SPRITE_X_EXPAND (nr))
	{
		for (y = ybegin; y <= yend; y++)
		{
			vic2.sprites[nr].paintedline[y] = 1;
			for (i = 0; i < 3; i++)
			{
				value = vic2.expandx[vic2.dma_read(machine, addr + vic2.sprites[nr].line * 3 + i)];
				vic2.sprites[nr].bitmap[y][i*2] = value>>8;
				vic2.sprites[nr].bitmap[y][i*2+1] = value&0xff;
				vic2_sprite_collision(machine, nr, y, xbegin + i * 16, value >> 8);
				vic2_sprite_collision(machine, nr, y, xbegin + i * 16 + 8, value & 0xff);
				if (prior || !collision)
					value3 = vic2_getforeground16 (yoff + y, xbegin + i * 16 - 7);
				if (!collision && (value & value3))
				{
					collision = 1;
					SPRITE_SET_BG_COLLISION (nr);
					vic2_set_interrupt(machine, 2);
				}
				if (prior)
					value &= ~value3;
				vic2_draw_sprite_code (yoff + y, xbegin + i * 16, value >> 8, color, start_x, end_x);
				vic2_draw_sprite_code (yoff + y, xbegin + i * 16 + 8, value & 0xff, color, start_x, end_x);
			}
			vic2.sprites[nr].bitmap[y][i*2]=0; //easier sprite collision detection
			if (SPRITE_Y_EXPAND (nr))
			{
				if (vic2.sprites[nr].repeat)
				{
					vic2.sprites[nr].line++;
					vic2.sprites[nr].repeat = 0;
				}
				else
					vic2.sprites[nr].repeat = 1;
			}
			else
			{
				vic2.sprites[nr].line++;
			}
		}
	}
	else
	{
		for (y = ybegin; y <= yend; y++)
		{
			vic2.sprites[nr].paintedline[y] = 1;
			for (i = 0; i < 3; i++)
			{
				value = vic2.dma_read(machine, addr + vic2.sprites[nr].line * 3 + i);
				vic2.sprites[nr].bitmap[y][i] = value;
				vic2_sprite_collision(machine, nr, y, xbegin + i * 8, value);
				if (prior || !collision)
					value3 = vic2_getforeground (yoff + y, xbegin + i * 8 - 7);
				if (!collision && (value & value3))
				{
					collision = 1;
					SPRITE_SET_BG_COLLISION (nr);
					vic2_set_interrupt(machine, 2);
				}
				if (prior)
					value &= ~value3;
				vic2_draw_sprite_code (yoff + y, xbegin + i * 8, value, color, start_x, end_x);
			}
			vic2.sprites[nr].bitmap[y][i]=0; //easier sprite collision detection
			if (SPRITE_Y_EXPAND (nr))
			{
				if (vic2.sprites[nr].repeat)
				{
					vic2.sprites[nr].line++;
					vic2.sprites[nr].repeat = 0;
				}
				else
					vic2.sprites[nr].repeat = 1;
			}
			else
			{
				vic2.sprites[nr].line++;
			}
		}
	}
}

static void vic3_drawlines (running_machine *machine, int first, int last, int start_x, int end_x);

static void vic2_drawlines (running_machine *machine, int first, int last, int start_x, int end_x)
{
	int line, vline, end;
	int attr, ch, ecm;
	int syend;
	int offs, yoff, xoff, ybegin, yend, xbegin, xend;
	int i;

	if (vic2.vic3 && VIC3_BITPLANES) return ;

	/* temporary allowing vic3 displaying 80 columns */
	if (vic2.vic3 && (vic2.reg[0x31] & 0x80))
	{
		vic3_drawlines(machine, first, first + 1, start_x, end_x);
		return;
	}

	if (video_skip_this_frame ())
		return;

	/* top part of display not rastered */
	first -= VIC2_YPOS - YPOS;

	xbegin = VIC2_STARTVISIBLECOLUMNS;
	xend = xbegin + VIC2_VISIBLECOLUMNS;
	if (!SCREENON)
	{
		xbegin = VIC2_STARTVISIBLECOLUMNS;
		xend = xbegin + VIC2_VISIBLECOLUMNS;
		if ((start_x <= xbegin) && (end_x >= xend))
			plot_box(vic2.bitmap, xbegin, first + FIRSTLINE, xend - xbegin, 1, FRAMECOLOR);
		if ((start_x > xbegin) && (end_x >= xend))
			plot_box(vic2.bitmap, start_x - VIC2_STARTVISIBLECOLUMNS, first + FIRSTLINE, xend - start_x, 1, FRAMECOLOR);
		if ((start_x <= xbegin) && (end_x < xend))
			plot_box(vic2.bitmap, xbegin, first + FIRSTLINE, end_x - xbegin , 1, FRAMECOLOR);
		if ((start_x > xbegin) && (end_x < xend))
			plot_box(vic2.bitmap, start_x - VIC2_STARTVISIBLECOLUMNS, first + FIRSTLINE, end_x - start_x, 1, FRAMECOLOR);
		return;
	}

	if (COLUMNS40)
	{
		xbegin = XPOS;
		xend = xbegin + 320;
	}
	else
	{
		xbegin = XPOS + 7;
		xend = xbegin + 304;
	}

	if (first + 1 < vic2.y_begin)
		end = first + 1;
	else
		end = vic2.y_begin + YPOS;

	line = first;
	// top border
	if (line < end)
	{
		if ((start_x <= xbegin) && (end_x >= xend))
			plot_box(vic2.bitmap, xbegin, first + FIRSTLINE, xend - xbegin, 1, FRAMECOLOR);
		if ((start_x > xbegin) && (end_x >= xend))
			plot_box(vic2.bitmap, start_x - VIC2_STARTVISIBLECOLUMNS, first + FIRSTLINE, xend - start_x, 1, FRAMECOLOR);
		if ((start_x <= xbegin) && (end_x < xend))
			plot_box(vic2.bitmap, xbegin, first + FIRSTLINE, end_x - xbegin , 1, FRAMECOLOR);
		if ((start_x > xbegin) && (end_x < xend))
			plot_box(vic2.bitmap, start_x - VIC2_STARTVISIBLECOLUMNS, first + FIRSTLINE, end_x - start_x, 1, FRAMECOLOR);
		line = end;
	}

	vline = line - YPOS + 3 - VERTICALPOS;

	if (first + 1 < vic2.y_end + YPOS)
		end = first + 1;
	else
		end = vic2.y_end + YPOS;

	if (line < end)
	{
		offs = (vline >> 3) * 40;
		ybegin = vline & 7;
		yoff = line - ybegin;
		yend = (yoff + 7 < end) ? 7 : (end - yoff - 1);

		/* rendering 39 characters */
		/* left and right borders are overwritten later */

		vic2.shift[line] = HORIZONTALPOS;
		for (xoff = vic2.x_begin + XPOS; xoff < vic2.x_end + XPOS; xoff += 8, offs++)
		{
			ch = vic2.dma_read(machine, vic2.videoaddr + offs);
#if 0
			attr = vic2.dma_read_color(machine, vic2.videoaddr + offs);
#else
			/* temporaery until vic3 finished */
			attr = vic2.dma_read_color(machine, (vic2.videoaddr + offs)&0x3ff)&0x0f;
#endif
			if (HIRESON)
			{
				vic2.bitmapmulti[1] = vic2.c64_bitmap[1] = ch >> 4;
				vic2.bitmapmulti[2] = vic2.c64_bitmap[0] = ch & 0xf;
				if (MULTICOLORON)
				{
				    vic2.bitmapmulti[3] = attr;
					vic2_draw_bitmap_multi(machine, ybegin, yend, offs, yoff, xoff, start_x, end_x);
				}
				else
				{
					vic2_draw_bitmap(machine, ybegin, yend, offs, yoff, xoff, start_x, end_x);
				}
			}
			else if (ECMON)
			{
				ecm = ch >> 6;
				vic2.ecmcolor[0] = vic2.colors[ecm];
				vic2.ecmcolor[1] = attr;
				vic2_draw_character(machine, ybegin, yend, ch & ~0xC0, yoff, xoff, vic2.ecmcolor, start_x, end_x);
			}
			else if (MULTICOLORON && (attr & 8))
			{
				vic2.multi[3] = attr & 7;
				vic2_draw_character_multi(machine, ybegin, yend, ch, yoff, xoff, start_x, end_x);
			}
			else
			{
				vic2.mono[1] = attr;
				vic2_draw_character(machine, ybegin, yend, ch, yoff, xoff, vic2.mono, start_x, end_x);
			}
		}

		/* sprite priority, sprite overwrites lowerprior pixels */
		for (i = 7; i >= 0; i--)
		{
			if (SPRITEON (i) &&
					(yoff + ybegin >= SPRITE_Y_POS (i)) &&
					(yoff + ybegin - SPRITE_Y_POS (i) < (SPRITE_Y_EXPAND (i)? 21 * 2 : 21 )) &&
					(SPRITE_Y_POS (i) < 0))
			{
				int wrapped = - SPRITE_Y_POS (i) + 6;

				syend = yend;

				if (SPRITE_Y_EXPAND (i))
				{
					if (wrapped & 1) vic2.sprites[i].repeat = 1;
					wrapped >>= 1;
					syend = 21 * 2 - 1 - wrapped * 2;
					if (syend > (yend - ybegin)) syend = yend - ybegin;
				}
				else
				{
					syend = 21 - 1 - wrapped;
					if (syend > (yend - ybegin)) syend = yend - ybegin;
				}

				vic2.sprites[i].line = wrapped;

				if (SPRITE_MULTICOLOR (i))
					vic2_draw_sprite_multi(machine, i, 0, 0 , syend, start_x, end_x);
				else
					vic2_draw_sprite(machine, i, 0, 0 , syend, start_x, end_x);
			}
			else if 	(SPRITEON (i) &&
					(yoff + ybegin >= SPRITE_Y_POS (i)) &&
					(yoff + ybegin - SPRITE_Y_POS (i) < (SPRITE_Y_EXPAND (i)? 21 * 2 : 21 )) &&
					(SPRITE_Y_POS (i) >= 0))
			{
				int wrapped = yoff + ybegin - SPRITE_Y_POS (i);

				syend = yend;

				if (SPRITE_Y_EXPAND (i))
				{
					if (wrapped & 1) vic2.sprites[i].repeat = 1;
					wrapped >>= 1;
					syend = 21 * 2 - 1 - wrapped * 2;
					if (syend > (yend - ybegin)) syend = yend - ybegin;
				}
				else
				{
					syend = 21 - 1 - wrapped;
					if (syend > (yend - ybegin)) syend = yend - ybegin;
				}

				vic2.sprites[i].line = wrapped;

				if (SPRITE_MULTICOLOR (i))
					vic2_draw_sprite_multi(machine, i, yoff + ybegin, 0, 0, start_x, end_x);
				else
					vic2_draw_sprite(machine, i, yoff + ybegin, 0, 0, start_x, end_x);
			}
			else
			{
				memset (vic2.sprites[i].paintedline, 0, sizeof (vic2.sprites[i].paintedline));
			}
		}
		line += 1 + yend - ybegin;
	}

	// left border
	if ((start_x <= VIC2_STARTVISIBLECOLUMNS) && (end_x >= xbegin))
		plot_box(vic2.bitmap, VIC2_STARTVISIBLECOLUMNS, first + FIRSTLINE, xbegin - VIC2_STARTVISIBLECOLUMNS, 1, FRAMECOLOR);
	else if ((start_x > VIC2_STARTVISIBLECOLUMNS) && (end_x >= xbegin))
		plot_box(vic2.bitmap, start_x, first + FIRSTLINE, xbegin - start_x, 1, FRAMECOLOR);
	else if ((start_x <= VIC2_STARTVISIBLECOLUMNS) && (end_x < xbegin))
		plot_box(vic2.bitmap, VIC2_STARTVISIBLECOLUMNS, first + FIRSTLINE, end_x, 1, FRAMECOLOR);
	else if ((start_x > VIC2_STARTVISIBLECOLUMNS) && (end_x < xbegin))
		plot_box(vic2.bitmap, start_x, first + FIRSTLINE, end_x - start_x, 1, FRAMECOLOR);

	// right border
	if ((start_x <= xend) && (end_x >= VIC2_STARTVISIBLECOLUMNS + VIC2_VISIBLECOLUMNS))
		plot_box(vic2.bitmap, xend, first + FIRSTLINE, VIC2_STARTVISIBLECOLUMNS + VIC2_VISIBLECOLUMNS - xend, 1, FRAMECOLOR);
	else if ((start_x > xend) && (end_x >= VIC2_STARTVISIBLECOLUMNS + VIC2_VISIBLECOLUMNS))
		plot_box(vic2.bitmap, start_x, first + FIRSTLINE, VIC2_STARTVISIBLECOLUMNS + VIC2_VISIBLECOLUMNS - start_x, 1, FRAMECOLOR);
	else if ((start_x <= xend) && (end_x < VIC2_STARTVISIBLECOLUMNS + VIC2_VISIBLECOLUMNS))
		plot_box(vic2.bitmap, xend, first + FIRSTLINE, end_x - xend, 1, FRAMECOLOR);
	else if ((start_x > VIC2_STARTVISIBLECOLUMNS) && (end_x < xbegin))
		plot_box(vic2.bitmap, start_x, first + FIRSTLINE, end_x - start_x, 1, FRAMECOLOR);

	if (first + 1 < vic2.bitmap->height)
		end = first + 1;
	else
		end = vic2.bitmap->height;

	// bottom border
	if (line < end)
	{
		if ((start_x <= xbegin) && (end_x >= xend))
			plot_box(vic2.bitmap, xbegin, first + FIRSTLINE, xend - xbegin, 1, FRAMECOLOR);
		if ((start_x > xbegin) && (end_x >= xend))
			plot_box(vic2.bitmap, start_x - VIC2_STARTVISIBLECOLUMNS, first + FIRSTLINE, xend - start_x, 1, FRAMECOLOR);
		if ((start_x <= xbegin) && (end_x < xend))
			plot_box(vic2.bitmap, xbegin, first + FIRSTLINE, end_x - xbegin , 1, FRAMECOLOR);
		if ((start_x > xbegin) && (end_x < xend))
			plot_box(vic2.bitmap, start_x - VIC2_STARTVISIBLECOLUMNS, first + FIRSTLINE, end_x - start_x, 1, FRAMECOLOR);
		line = end;
	}
}

#include "vic4567.c"

#ifdef UNUSED_FUNCTION
static TIMER_CALLBACK( line_timer_callback )
{
	int i,j;

	switch(vic2.rasterX)
	{

	case 1:
		if (vic2.rasterline == vic2.lines)
		{
			vic2.rasterline = 0;

			for (i = 0; i < 8; i++)
			{
				vic2.sprites[i].repeat = vic2.sprites[i].line = 0;
				for (j = 0; j < 256; j++)
					vic2.sprites[i].paintedline[j] = 0;
			}

			if (LIGHTPEN_BUTTON)
			{
				/* lightpen timer starten */
				timer_set (machine, attotime_make(0, 0), NULL, 1, vic2_timer_timeout);
			}
		}

		if (vic2.rasterline == RASTERLINE)
		{
			vic2_set_interrupt(machine, 1);
		}

		vic2.rasterX++;
		break;

	case 60:
		vic2.rasterX++;

		if (!c64_pal)
			if (vic2.rasterline < VIC2_FIRSTRASTERLINE + VIC2_VISIBLELINES - vic2.lines)
				vic2_drawlines (machine, vic2.rasterline + vic2.lines, vic2.rasterline + vic2.lines + 1, VIC2_STARTVISIBLECOLUMNS, VIC2_STARTVISIBLECOLUMNS + VIC2_VISIBLECOLUMNS);

		if (vic2.on)
			if ((vic2.rasterline >= VIC6569_STARTVISIBLELINES ) && (vic2.rasterline < VIC6569_STARTVISIBLELINES + VIC2_VISIBLELINES))
				vic2_drawlines (machine, vic2.rasterline, vic2.rasterline + 1, VIC2_STARTVISIBLECOLUMNS, VIC2_STARTVISIBLECOLUMNS + VIC2_VISIBLECOLUMNS);

		break;

	case 63:
		vic2.rasterline++;
		vic2.rasterX = 1;

		break;

	default:
		vic2.rasterX++;

		break;
	}

	timer_set(machine, cputag_clocks_to_attotime(machine, "maincpu", 1), NULL, 0, line_timer_callback);
}
#endif

// modified VIC II emulation by Christian Bauer starts here...

// Idle access
#define IdleAccess \
	vic2.dma_read(machine, 0x3fff)

// Fetch sprite data pointer
#define SprPtrAccess(num) \
	spr_ptr[num] = vic2.dma_read(machine, SPRITE_ADDR(num)) << 6

// Fetch sprite data, increment data counter
#define SprDataAccess(num, bytenum) \
	if (spr_dma_on & (1 << num)) \
	{ \
		spr_data[num][bytenum] = vic2.dma_read(machine, (mc[num] & 0x3f) | spr_ptr[num]); \
		mc[num]++; \
	} \
	else \
		if (bytenum == 1) \
			IdleAccess;

// Turn on display if Bad Line
#define DisplayIfBadLine \
	if (is_bad_line) \
		display_state = 1;

// Suspend CPU
#define SuspendCPU \
	{ \
		if (cpu_suspended == 0) \
		{ \
			first_ba_cycle = vic2.cycles_counter; \
			if (input_port_read(machine, "CTRLSEL") & 0x08) cpu_suspend(machine->firstcpu, SUSPEND_REASON_SPIN, 0); \
			cpu_suspended = 1; \
		} \
	}

// Resume CPU
#define ResumeCPU \
	{ \
		if (cpu_suspended == 1) \
		{ \
			if (input_port_read(machine, "CTRLSEL") & 0x08) cpu_resume(machine->firstcpu, SUSPEND_REASON_SPIN); \
			cpu_suspended = 0; \
		} \
	}


// Refresh access
#define RefreshAccess \
	{ \
		vic2.dma_read(machine, 0x3f00 | ref_cnt--); \
	}

#define FetchIfBadLine \
	if (is_bad_line) \
	{ \
		display_state = 1; \
	}

// Turn on display and matrix access and reset RC if Bad Line
#define RCIfBadLine \
	if (is_bad_line) \
	{ \
		display_state = 1; \
		rc = 0; \
	}

// Sample border color and increment vic2.graphic_x
#define SampleBorder \
	if (draw_this_line) \
	{ \
		if (border_on) \
			border_color_sample[vic2.cycle - 13] = FRAMECOLOR; \
		vic2.graphic_x += 8; \
	}

// Turn on sprite DMA if necessary
#define CheckSpriteDMA \
	mask = 1; \
	for (i = 0; i < 8; i++, mask <<= 1) \
		if (SPRITEON(i) && ((vic2.rasterline & 0xff) == SPRITE_Y_POS2(i))) \
		{ \
			spr_dma_on |= mask; \
			mc_base[i] = 0; \
			if (SPRITE_Y_EXPAND(i)) \
				spr_exp_y &= ~mask; \
		}

// Video matrix access
#define MatrixAccess \
{ \
	if (cpu_suspended == 1) \
	{ \
		if ((vic2.cycles_counter - first_ba_cycle) < 0) \
			matrix_line[ml_index] = color_line[ml_index] = 0xff; \
		else \
		{ \
			UINT16 adr = (vc & 0x03ff) | VIDEOADDR; \
			matrix_line[ml_index] = vic2.dma_read(machine, adr); \
			color_line[ml_index] = vic2.dma_read_color(machine, (adr & 0x03ff)); \
		} \
	} \
}

// Graphics data access
#define GraphicsAccess \
{ \
	if (display_state == 1) \
	{ \
		UINT16 adr; \
		if (HIRESON) \
			adr = ((vc & 0x03ff) << 3) | vic2.bitmapaddr | rc; \
		else \
			adr = (matrix_line[ml_index] << 3) | vic2.chargenaddr | rc; \
		if (ECMON) \
			adr &= 0xf9ff; \
		gfx_data = vic2.dma_read(machine, adr); \
		char_data = matrix_line[ml_index]; \
		color_data = color_line[ml_index]; \
		ml_index++; \
		vc++; \
	} \
	else \
	{ \
		gfx_data = vic2.dma_read(machine, (ECMON ? 0x39ff : 0x3fff)); \
		char_data = 0; \
	} \
}

#define DrawBackground \
	if (draw_this_line) \
	{ \
		UINT8 c; \
		switch (GFXMODE) \
		{ \
			case 0: \
			case 1: \
			case 3: \
				c = vic2.colors[0]; \
				break; \
			case 2: \
				c = last_char_data & 0x0f; \
				break; \
			case 4: \
				if (last_char_data & 0x80) \
					if (last_char_data & 0x40) \
						c = vic2.colors[3]; \
					else \
						c = vic2.colors[2]; \
				else \
					if (last_char_data & 0x40) \
						c = vic2.colors[1]; \
					else \
						c = vic2.colors[0]; \
				break; \
			default: \
				c = 0; \
				break; \
		} \
		plot_box(vic2.bitmap, vic2.graphic_x, VIC2_RASTER_2_EMU(vic2.rasterline), 8, 1, c); \
	}

#define DrawMono \
	data = gfx_data; \
	*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 7) = c[data & 1]; \
	fore_coll_buf[p + 7] = data & 1; data >>= 1; \
	*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 6) = c[data & 1]; \
	fore_coll_buf[p + 6] = data & 1; data >>= 1; \
	*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 5) = c[data & 1]; \
	fore_coll_buf[p + 5] = data & 1; data >>= 1; \
	*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 4) = c[data & 1]; \
	fore_coll_buf[p + 4] = data & 1; data >>= 1; \
	*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 3) = c[data & 1]; \
	fore_coll_buf[p + 3] = data & 1; data >>= 1; \
	*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 2) = c[data & 1]; \
	fore_coll_buf[p + 2] = data & 1; data >>= 1; \
	*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 1) = c[data & 1]; \
	fore_coll_buf[p + 1] = data & 1; data >>= 1; \
	*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 0) = c[data]; \
	fore_coll_buf[p + 0] = data & 1; \

#define DrawMulti \
	data = gfx_data; \
	*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 7) = c[data & 3]; \
	fore_coll_buf[p + 7] = data & 2; \
	*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 6) = c[data & 3]; \
	fore_coll_buf[p + 6] = data & 2; data >>= 2; \
	*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 5) = c[data & 3]; \
	fore_coll_buf[p + 5] = data & 2; \
	*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 4) = c[data & 3]; \
	fore_coll_buf[p + 4] = data & 2; data >>= 2; \
	*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 3) = c[data & 3]; \
	fore_coll_buf[p + 3] = data & 2; \
	*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 2) = c[data & 3]; \
	fore_coll_buf[p + 2] = data & 2; data >>= 2; \
	*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 1) = c[data]; \
	fore_coll_buf[p + 1] = data & 2; \
	*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 0) = c[data]; \
	fore_coll_buf[p + 0] = data & 2; \

// Graphics display (8 pixels)
#define DrawGraphics \
	if (draw_this_line == 0) \
	{ \
		UINT16 p = vic2.graphic_x + HORIZONTALPOS; \
		fore_coll_buf[p + 7] = 0; \
		fore_coll_buf[p + 6] = 0; \
		fore_coll_buf[p + 5] = 0; \
		fore_coll_buf[p + 4] = 0; \
		fore_coll_buf[p + 3] = 0; \
		fore_coll_buf[p + 2] = 0; \
		fore_coll_buf[p + 1] = 0; \
		fore_coll_buf[p + 0] = 0; \
	} \
	else if (ud_border_on) \
	{ \
		UINT16 p = vic2.graphic_x + HORIZONTALPOS; \
		fore_coll_buf[p + 7] = 0; \
		fore_coll_buf[p + 6] = 0; \
		fore_coll_buf[p + 5] = 0; \
		fore_coll_buf[p + 4] = 0; \
		fore_coll_buf[p + 3] = 0; \
		fore_coll_buf[p + 2] = 0; \
		fore_coll_buf[p + 1] = 0; \
		fore_coll_buf[p + 0] = 0; \
		DrawBackground; \
	} \
	else \
	{ \
		UINT8 c[4], data; \
		UINT16 p = vic2.graphic_x + HORIZONTALPOS; \
		switch (GFXMODE) \
		{ \
			case 0: \
				c[0] = vic2.colors[0]; \
				c[1] = color_data & 0x0f; \
				DrawMono; \
				break; \
			case 1: \
				if (color_data & 0x08) \
				{ \
					c[0] = vic2.colors[0]; \
					c[1] = vic2.colors[1]; \
					c[2] = vic2.colors[2]; \
					c[3] = color_data & 0x07; \
					DrawMulti; \
				} \
				else \
				{ \
					c[0] = vic2.colors[0]; \
					c[1] = color_data & 0x0f; \
					DrawMono; \
				} \
				break; \
			case 2: \
				c[0] = char_data & 0x0f; \
				c[1] = char_data >> 4; \
				DrawMono; \
				break; \
			case 3: \
				c[0] = vic2.colors[0]; \
				c[1] = char_data >> 4; \
				c[2] = char_data & 0x0f; \
				c[3] = color_data & 0x0f; \
				DrawMulti; \
				break; \
			case 4: \
				if (char_data & 0x80) \
					if (char_data & 0x40) \
						c[0] = vic2.colors[3]; \
					else \
						c[0] = vic2.colors[2]; \
				else \
					if (char_data & 0x40) \
						c[0] = vic2.colors[1]; \
					else \
						c[0] = vic2.colors[0]; \
				c[1] = color_data & 0x0f; \
				DrawMono; \
				break; \
			case 5: \
				*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 7) = 0; \
				fore_coll_buf[p + 7] = 0; \
				*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 6) = 0; \
				fore_coll_buf[p + 6] = 0; \
				*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 5) = 0; \
				fore_coll_buf[p + 5] = 0; \
				*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 4) = 0; \
				fore_coll_buf[p + 4] = 0; \
				*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 3) = 0; \
				fore_coll_buf[p + 3] = 0; \
				*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 2) = 0; \
				fore_coll_buf[p + 2] = 0; \
				*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 1) = 0; \
				fore_coll_buf[p + 1] = 0; \
				*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 0) = 0; \
				fore_coll_buf[p + 0] = 0; \
				break; \
			case 6: \
				*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 7) = 0; \
				fore_coll_buf[p + 7] = 0; \
				*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 6) = 0; \
				fore_coll_buf[p + 6] = 0; \
				*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 5) = 0; \
				fore_coll_buf[p + 5] = 0; \
				*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 4) = 0; \
				fore_coll_buf[p + 4] = 0; \
				*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 3) = 0; \
				fore_coll_buf[p + 3] = 0; \
				*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 2) = 0; \
				fore_coll_buf[p + 2] = 0; \
				*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 1) = 0; \
				fore_coll_buf[p + 1] = 0; \
				*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 0) = 0; \
				fore_coll_buf[p + 0] = 0; \
				break; \
			case 7: \
				*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 7) = 0; \
				fore_coll_buf[p + 7] = 0; \
				*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 6) = 0; \
				fore_coll_buf[p + 6] = 0; \
				*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 5) = 0; \
				fore_coll_buf[p + 5] = 0; \
				*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 4) = 0; \
				fore_coll_buf[p + 4] = 0; \
				*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 3) = 0; \
				fore_coll_buf[p + 3] = 0; \
				*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 2) = 0; \
				fore_coll_buf[p + 2] = 0; \
				*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 1) = 0; \
				fore_coll_buf[p + 1] = 0; \
				*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + 0) = 0; \
				fore_coll_buf[p + 0] = 0; \
				break; \
		} \
	}

#define DrawSprites \
	{ \
		int i; \
		UINT8 snum, sbit; \
		UINT8 spr_coll = 0, gfx_coll = 0; \
		UINT32 plane0_l, plane0_r, plane1_l, plane1_r; \
		UINT32 sdata_l = 0, sdata_r = 0; \
		for (i = 0; i < 0x400; i++) \
			spr_coll_buf[i] = 0; \
		for (snum = 0, sbit = 1; snum < 8; snum++, sbit <<= 1) \
		{ \
			if ((spr_draw & sbit) && (SPRITE_X_POS2(snum) <= (403 - (VIC2_FIRSTCOLUMN + 1)))) \
			{ \
				UINT16     p = 	SPRITE_X_POS2(snum) + VIC2_X_2_EMU(0) + 8; \
				UINT8  color = 	SPRITE_COLOR(snum); \
				UINT32 sdata =	(spr_draw_data[snum][0] << 24) | \
							(spr_draw_data[snum][1] << 16) | \
							(spr_draw_data[snum][2] << 8); \
				if (SPRITE_X_EXPAND(snum)) \
				{ \
					if (SPRITE_X_POS2(snum) > (403 - 24 - (VIC2_FIRSTCOLUMN + 1))) \
						continue; \
					if (SPRITE_MULTICOLOR(snum)) \
					{ \
						sdata_l = (vic2.expandx_multi[(sdata >> 24) & 0xff] << 16) | vic2.expandx_multi[(sdata >> 16) & 0xff]; \
						sdata_r = vic2.expandx_multi[(sdata >> 8) & 0xff] << 16; \
						plane0_l = (sdata_l & 0x55555555) | (sdata_l & 0x55555555) << 1; \
						plane1_l = (sdata_l & 0xaaaaaaaa) | (sdata_l & 0xaaaaaaaa) >> 1; \
						plane0_r = (sdata_r & 0x55555555) | (sdata_r & 0x55555555) << 1; \
						plane1_r = (sdata_r & 0xaaaaaaaa) | (sdata_r & 0xaaaaaaaa) >> 1; \
						for (i = 0; i < 32; i++, plane0_l <<= 1, plane1_l <<= 1) \
						{ \
							UINT8 col; \
							if (plane1_l & 0x80000000) \
							{ \
								if (fore_coll_buf[p + i]) \
								{ \
									gfx_coll |= sbit; \
								} \
								if (plane0_l & 0x80000000) \
									col = vic2.spritemulti[3]; \
								else \
									col = color; \
							} \
							else \
							{ \
								if (plane0_l & 0x80000000) \
								{ \
									if (fore_coll_buf[p + i]) \
									{ \
										gfx_coll |= sbit; \
									} \
									col = vic2.spritemulti[1]; \
								} \
								else \
									continue; \
							} \
							if (spr_coll_buf[p + i]) \
								spr_coll |= spr_coll_buf[p + i] | sbit; \
							else \
							{ \
								if (SPRITE_PRIORITY(snum)) \
								{ \
									if (fore_coll_buf[p + i] == 0) \
										*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + i) = col; \
									spr_coll_buf[p + i] = sbit; \
								} \
								else \
								{ \
									*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + i) = col; \
									spr_coll_buf[p + i] = sbit; \
								} \
							} \
						} \
						for (; i < 48; i++, plane0_r <<= 1, plane1_r <<= 1) \
						{ \
							UINT8 col; \
							if(plane1_r & 0x80000000) \
							{ \
								if (fore_coll_buf[p + i]) \
								{ \
									gfx_coll |= sbit; \
								} \
								if (plane0_r & 0x80000000) \
									col = vic2.spritemulti[3]; \
								else \
									col = color; \
							} \
							else \
							{ \
								if (plane0_r & 0x80000000) \
								{ \
									if (fore_coll_buf[p + i]) \
									{ \
										gfx_coll |= sbit; \
									} \
									col =  vic2.spritemulti[1]; \
								} \
								else \
									continue; \
							} \
							if (spr_coll_buf[p + i]) \
								spr_coll |= spr_coll_buf[p + i] | sbit; \
							else \
							{ \
								if (SPRITE_PRIORITY(snum)) \
								{ \
									if (fore_coll_buf[p + i] == 0) \
										*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + i) = col; \
									spr_coll_buf[p + i] = sbit; \
								} \
								else \
								{ \
									*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + i) = col; \
									spr_coll_buf[p + i] = sbit; \
								} \
							} \
						} \
					} \
					else \
					{ \
						sdata_l = (vic2.expandx[(sdata >> 24) & 0xff] << 16) | vic2.expandx[(sdata >> 16) & 0xff]; \
						sdata_r = vic2.expandx[(sdata >> 8) & 0xff] << 16; \
						for (i = 0; i < 32; i++, sdata_l <<= 1) \
							if (sdata_l & 0x80000000) \
							{ \
								if (fore_coll_buf[p + i]) \
								{ \
									gfx_coll |= sbit; \
								} \
								if(spr_coll_buf[p + i]) \
									spr_coll |= spr_coll_buf[p + i] | sbit; \
								else \
								{ \
									if (SPRITE_PRIORITY(snum)) \
									{ \
										if (fore_coll_buf[p + i] == 0) \
											*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + i) = color; \
										spr_coll_buf[p + i] = sbit; \
									} \
									else \
									{ \
										*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + i) = color; \
										spr_coll_buf[p + i] = sbit; \
									} \
								} \
							} \
						for (; i < 48; i++, sdata_r <<= 1) \
							if (sdata_r & 0x80000000) \
							{ \
								if (fore_coll_buf[p + i]) \
								{ \
									gfx_coll |= sbit; \
								} \
								if(spr_coll_buf[p + i]) \
									spr_coll |= spr_coll_buf[p + i] | sbit; \
								else \
								{ \
									if (SPRITE_PRIORITY(snum)) \
									{ \
										if (fore_coll_buf[p + i] == 0) \
											*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + i) = color; \
										spr_coll_buf[p + i] = sbit; \
									} \
									else \
									{ \
										*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + i) = color; \
										spr_coll_buf[p + i] = sbit; \
									} \
								} \
							} \
					} \
				} \
				else \
				{ \
					if (SPRITE_MULTICOLOR(snum)) \
					{ \
						UINT32 plane0, plane1; \
						plane0 = (sdata & 0x55555555) | (sdata & 0x55555555) << 1; \
						plane1 = (sdata & 0xaaaaaaaa) | (sdata & 0xaaaaaaaa) >> 1; \
						for (i = 0; i < 24; i++, plane0 <<= 1, plane1 <<= 1) \
						{ \
							UINT8 col; \
							if (plane1 & 0x80000000) \
							{ \
								if (fore_coll_buf[p + i]) \
								{ \
									gfx_coll |= sbit; \
								} \
								if (plane0 & 0x80000000) \
									col = vic2.spritemulti[3]; \
								else \
									col = color; \
							} \
							else \
							{ \
								if (fore_coll_buf[p + i]) \
								{ \
									gfx_coll |= sbit; \
								} \
								if (plane0 & 0x80000000) \
									col = vic2.spritemulti[1]; \
								else \
									continue; \
							} \
							if (spr_coll_buf[p + i]) \
								spr_coll |= spr_coll_buf[p + i] | sbit; \
							else \
							{ \
								if (SPRITE_PRIORITY(snum)) \
								{ \
									if (fore_coll_buf[p + i] == 0) \
										*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + i) = col; \
									spr_coll_buf[p + i] = sbit; \
								} \
								else \
								{ \
									*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + i) = col; \
									spr_coll_buf[p + i] = sbit; \
								} \
							} \
						} \
					} \
					else \
					{ \
						for (i = 0; i < 24; i++, sdata <<= 1) \
						{ \
							if (sdata & 0x80000000) \
							{ \
								if (fore_coll_buf[p + i]) \
								{ \
									gfx_coll |= sbit; \
								} \
								if (spr_coll_buf[p + i]) \
								{ \
									spr_coll |= spr_coll_buf[p + i] | sbit; \
								} \
								else \
								{ \
									if (SPRITE_PRIORITY(snum)) \
									{ \
										if (fore_coll_buf[p + i] == 0) \
											*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + i) = color; \
										spr_coll_buf[p + i] = sbit; \
									} \
									else \
									{ \
										*BITMAP_ADDR16(vic2.bitmap, VIC2_RASTER_2_EMU(vic2.rasterline), p + i) = color; \
										spr_coll_buf[p + i] = sbit; \
									} \
								} \
							} \
						} \
					} \
				} \
			} \
		} \
		if (SPRITE_COLL) \
			SPRITE_COLL |= spr_coll; \
		else \
		{ \
			SPRITE_COLL = spr_coll; \
			if (SPRITE_COLL) vic2_set_interrupt(machine, 4); \
		} \
		if (SPRITE_BG_COLL) \
			SPRITE_BG_COLL |= gfx_coll; \
		else \
		{ \
			SPRITE_BG_COLL = gfx_coll; \
			if (SPRITE_BG_COLL) vic2_set_interrupt(machine, 2); \
		} \
	}

static TIMER_CALLBACK( PAL_timer_callback )
{
	int i;
	UINT8 mask;
	vic2.cycles_counter++;

	switch(vic2.cycle)
	{

	// Sprite 3, raster counter, raster IRQ, bad line
	case 1:
		if (vic2.rasterline == (VIC2_LINES - 1))
		{
			vblanking = 1;

			if (LIGHTPEN_BUTTON)
			{
				/* lightpen timer starten */
				timer_set (machine, attotime_make(0, 0), NULL, 1, vic2_timer_timeout);
			}
		}
		else
		{
			vic2.rasterline++;

			if (vic2.rasterline == RASTERLINE)
			{
				vic2_set_interrupt(machine, 1);
			}

			if (vic2.rasterline == VIC2_FIRST_DMA_LINE)
				bad_lines_enabled = SCREENON;

			is_bad_line = 	((vic2.rasterline >= VIC2_FIRST_DMA_LINE) &&
						(vic2.rasterline <= VIC2_LAST_DMA_LINE) &&
						((vic2.rasterline & 0x07) == VERTICALPOS) &&
						bad_lines_enabled);

			draw_this_line =	((VIC2_RASTER_2_EMU(vic2.rasterline) >= VIC2_RASTER_2_EMU(VIC2_FIRST_DISP_LINE)) &&
						(VIC2_RASTER_2_EMU(vic2.rasterline ) <= VIC2_RASTER_2_EMU(VIC2_LAST_DISP_LINE)));
		}

		border_on_sample[0] = border_on;
		SprPtrAccess(3);
		SprDataAccess(3, 0);
		DisplayIfBadLine;

		if(spr_dma_on & 0x08)
			SuspendCPU
		else
			ResumeCPU

		vic2.cycle++;
		break;

	// Sprite 3
	case 2:
		if (vblanking)
		{
			// Vertical blank, reset counters
			vic2.rasterline = vc_base = 0;
			ref_cnt = 0xff;
			vblanking = 0;

			// Trigger raster IRQ if IRQ in line 0
			if (RASTERLINE == 0)
			{
				vic2_set_interrupt(machine, 1);
			}
		}

		vic2.graphic_x = VIC2_X_2_EMU(0);

		SprDataAccess(3, 1);
		SprDataAccess(3, 2);
		DisplayIfBadLine;

		vic2.cycle++;
		break;

	// Sprite 4
	case 3:
		SprPtrAccess(4);
		SprDataAccess(4, 0);
		DisplayIfBadLine;

		if(spr_dma_on & 0x10)
			SuspendCPU
		else
			ResumeCPU

		vic2.cycle++;
		break;

	// Sprite 4
	case 4:
		SprDataAccess(4, 1);
		SprDataAccess(4, 2);
		DisplayIfBadLine;

		vic2.cycle++;
		break;

	// Sprite 5
	case 5:
		SprPtrAccess(5);
		SprDataAccess(5, 0);
		DisplayIfBadLine;

		if(spr_dma_on & 0x20)
			SuspendCPU
		else
			ResumeCPU

		vic2.cycle++;
		break;

	// Sprite 5
	case 6:
		SprDataAccess(5, 1);
		SprDataAccess(5, 2);
		DisplayIfBadLine;

		vic2.cycle++;
		break;

	// Sprite 6
	case 7:
		SprPtrAccess(6);
		SprDataAccess(6, 0);
		DisplayIfBadLine;

		if(spr_dma_on & 0x40)
			SuspendCPU
		else
			ResumeCPU

		vic2.cycle++;
		break;

	// Sprite 6
	case 8:
		SprDataAccess(6, 1);
		SprDataAccess(6, 2);
		DisplayIfBadLine;

		vic2.cycle++;
		break;

	// Sprite 7
	case 9:
		SprPtrAccess(7);
		SprDataAccess(7, 0);
		DisplayIfBadLine;

		if(spr_dma_on & 0x80)
			SuspendCPU
		else
			ResumeCPU

		vic2.cycle++;
		break;

	// Sprite 7
	case 10:
		SprDataAccess(7, 1);
		SprDataAccess(7, 2);
		DisplayIfBadLine;

		vic2.cycle++;
		break;

	// Refresh
	case 11:
		RefreshAccess;
		DisplayIfBadLine;

		ResumeCPU

		vic2.cycle++;
		break;

	// Refresh, fetch if bad line
	case 12:
		RefreshAccess;
		FetchIfBadLine;

		vic2.cycle++;
		break;

	// Refresh, fetch if bad line, raster_x
	case 13:
		DrawBackground;
		SampleBorder;
		RefreshAccess;
		FetchIfBadLine;

		vic2.raster_x = 0xfffc;

		vic2.cycle++;
		break;

	// Refresh, fetch if bad line, RC, VC
	case 14:
		DrawBackground;
		SampleBorder;
		RefreshAccess;
		RCIfBadLine;

		vc = vc_base;

		vic2.cycle++;
		break;

	// Refresh, fetch if bad line, sprite y expansion
	case 15:
		DrawBackground;
		SampleBorder;
		RefreshAccess;
		FetchIfBadLine;

		for (i = 0; i < 8; i++)
			if (spr_exp_y & (1 << i))
				mc_base[i] += 2;

		if (is_bad_line)
			SuspendCPU

		ml_index = 0;
		MatrixAccess;

		vic2.cycle++;
		break;

	// Graphics, sprite y expansion, sprite DMA
	case 16:
		DrawBackground;
		SampleBorder;
		GraphicsAccess;
		FetchIfBadLine;

		mask = 1;
		for (i = 0; i < 8; i++, mask <<= 1)
		{
			if (spr_exp_y & mask)
				mc_base[i]++;
			if ((mc_base[i] & 0x3f) == 0x3f)
				spr_dma_on &= ~mask;
		}

		MatrixAccess;

		vic2.cycle++;
		break;

	// Graphics, check border
	case 17:
		if (COLUMNS40)
		{
			if (vic2.rasterline == dy_stop)
				ud_border_on = 1;
			else
			{
				if (SCREENON)
				{
					if (vic2.rasterline == dy_start)
						border_on = ud_border_on = 0;
					else
						if (ud_border_on == 0)
							border_on = 0;
				} else
					if (ud_border_on == 0)
						border_on = 0;
			}
		}

		// Second sample of border state
		border_on_sample[1] = border_on;

		DrawBackground;
		DrawGraphics;
		SampleBorder;
		GraphicsAccess;
		FetchIfBadLine;
		MatrixAccess;

		vic2.cycle++;
		break;

	// Check border
	case 18:
		if (!COLUMNS40)
		{
			if (vic2.rasterline == dy_stop)
				ud_border_on = 1;
			else
			{
				if (SCREENON)
				{
					if (vic2.rasterline == dy_start)
						border_on = ud_border_on = 0;
					else
						if (ud_border_on == 0)
							border_on = 0;
				} else
					if (ud_border_on == 0)
						border_on = 0;
			}
		}

		// Third sample of border state
		border_on_sample[2] = border_on;

	// Graphics

	case 19:
	case 20:
	case 21:
	case 22:
	case 23:
	case 24:
	case 25:
	case 26:
	case 27:
	case 28:
	case 29:
	case 30:
	case 31:
	case 32:
	case 33:
	case 34:
	case 35:
	case 36:
	case 37:
	case 38:
	case 39:
	case 40:
	case 41:
	case 42:
	case 43:
	case 44:
	case 45:
	case 46:
	case 47:
	case 48:
	case 49:
	case 50:
	case 51:
	case 52:
	case 53:
	case 54:
		DrawGraphics;
		SampleBorder;
		GraphicsAccess;
		FetchIfBadLine;
		MatrixAccess;
		last_char_data = char_data;

		vic2.cycle++;
		break;

	// Graphics, sprite y expansion, sprite DMA
	case 55:
		DrawGraphics;
		SampleBorder;
		GraphicsAccess;
		DisplayIfBadLine;

		// sprite y expansion
		mask = 1;
		for (i = 0; i < 8; i++, mask <<= 1)
			if (SPRITE_Y_EXPAND (i))
				spr_exp_y ^= mask;

		CheckSpriteDMA;

		ResumeCPU

		vic2.cycle++;
		break;

	// Check border, sprite DMA
	case 56:
		if (!COLUMNS40)
			border_on = 1;

		// Fourth sample of border state
		border_on_sample[3] = border_on;

		DrawGraphics;
		SampleBorder;
		IdleAccess;
		DisplayIfBadLine;
		CheckSpriteDMA;

		vic2.cycle++;
		break;

	// Check border, sprites
	case 57:
		if (COLUMNS40)
			border_on = 1;

		// Fifth sample of border state
		border_on_sample[4] = border_on;

		// Sample spr_disp_on and spr_data for sprite drawing
		spr_draw = spr_disp_on;
		if (spr_draw)
			memcpy(spr_draw_data, spr_data, 8 * 4);

		mask = 1;
		for (i = 0; i < 8; i++, mask <<= 1)
			if ((spr_disp_on & mask) && !(spr_dma_on & mask))
				spr_disp_on &= ~mask;

		DrawBackground;
		SampleBorder;
		IdleAccess;
		DisplayIfBadLine;

		vic2.cycle++;
		break;

	// Sprite 0, sprite DMA, MC, RC
	case 58:
		DrawBackground;
		SampleBorder;

		mask = 1;
		for (i = 0; i < 8; i++, mask <<= 1)
		{
			mc[i] = mc_base[i];
			if ((spr_dma_on & mask) && ((vic2.rasterline & 0xff) == SPRITE_Y_POS2(i)))
				spr_disp_on |= mask;
		}

		SprPtrAccess(0);
		SprDataAccess(0, 0);

		if (rc == 7)
		{
			vc_base = vc;
			display_state = 0;
		}

		if (is_bad_line || display_state)
		{
			display_state = 1;
			rc = (rc + 1) & 7;
		}

		if(spr_dma_on & 0x01)
			SuspendCPU
		else
			ResumeCPU

		vic2.cycle++;
		break;

	// Sprite 0
	case 59:
		DrawBackground;
		SampleBorder;
		SprDataAccess(0, 1);
		SprDataAccess(0, 2);
		DisplayIfBadLine;

		vic2.cycle++;
		break;

	// Sprite 1, draw
	case 60:
		DrawBackground;
		SampleBorder;

		if (draw_this_line)
		{
			DrawSprites;

			if (border_on_sample[0])
				for (i = 0; i < 4; i++)
					plot_box(vic2.bitmap, VIC2_X_2_EMU(i * 8), VIC2_RASTER_2_EMU(vic2.rasterline), 8, 1, border_color_sample[i]);

			if (border_on_sample[1])
				plot_box(vic2.bitmap, VIC2_X_2_EMU(4 * 8), VIC2_RASTER_2_EMU(vic2.rasterline), 8, 1, border_color_sample[4]);

			if (border_on_sample[2])
				for (i = 5; i < 43; i++)
					plot_box(vic2.bitmap, VIC2_X_2_EMU(i * 8), VIC2_RASTER_2_EMU(vic2.rasterline), 8, 1, border_color_sample[i]);

			if (border_on_sample[3])
				plot_box(vic2.bitmap, VIC2_X_2_EMU(43 * 8), VIC2_RASTER_2_EMU(vic2.rasterline), 8, 1, border_color_sample[43]);

			if (border_on_sample[4])
			{
				for (i = 44; i < 48; i++)
					plot_box(vic2.bitmap, VIC2_X_2_EMU(i * 8), VIC2_RASTER_2_EMU(vic2.rasterline), 8, 1, border_color_sample[i]);
				for (i = 48; i < 51; i++)
					plot_box(vic2.bitmap, VIC2_X_2_EMU(i * 8), VIC2_RASTER_2_EMU(vic2.rasterline), 8, 1, border_color_sample[47]);
			}
		}

		SprPtrAccess(1);
		SprDataAccess(1, 0);
		DisplayIfBadLine;

		if(spr_dma_on & 0x02)
			SuspendCPU
		else
			ResumeCPU

		vic2.cycle++;
		break;

	// Sprite 1
	case 61:
		SprDataAccess(1, 1);
		SprDataAccess(1, 2);
		DisplayIfBadLine;

		vic2.cycle++;
		break;

	// Sprite 2
	case 62:
		SprPtrAccess(2);
		SprDataAccess(2, 0);
		DisplayIfBadLine;

		if(spr_dma_on & 0x04)
			SuspendCPU
		else
			ResumeCPU

		vic2.cycle++;
		break;

	// Sprite 2
	case 63:
		SprDataAccess(2, 1);
		SprDataAccess(2, 2);
		DisplayIfBadLine;

		if (vic2.rasterline == dy_stop)
			ud_border_on = 1;
		else
			if (SCREENON && (vic2.rasterline == dy_start))
				ud_border_on = 0;

		// Last cycle
		vic2.cycle = 1;
	}

	vic2.raster_x += 8;
	timer_set(machine, cputag_clocks_to_attotime(machine, "maincpu", 1), NULL, 0, PAL_timer_callback);
}

static TIMER_CALLBACK( NTSC_timer_callback )
{
	int i;
	UINT8 mask;
	vic2.cycles_counter++;

	switch(vic2.cycle)
	{

	// Sprite 3, raster counter, raster IRQ, bad line
	case 1:
		if (vic2.rasterline == (VIC2_LINES - 1))
		{
			vblanking = 1;

			if (LIGHTPEN_BUTTON)
			{
				/* lightpen timer starten */
				timer_set (machine, attotime_make(0, 0), NULL, 1, vic2_timer_timeout);
			}
		}
		else
		{
			vic2.rasterline++;

			if (vic2.rasterline == RASTERLINE)
			{
				vic2_set_interrupt(machine, 1);
			}

			if (vic2.rasterline == VIC2_FIRST_DMA_LINE)
				bad_lines_enabled = SCREENON;

			is_bad_line = 	((vic2.rasterline >= VIC2_FIRST_DMA_LINE) &&
						(vic2.rasterline <= VIC2_LAST_DMA_LINE) &&
						((vic2.rasterline & 0x07) == VERTICALPOS) &&
						bad_lines_enabled);

			draw_this_line =	((VIC2_RASTER_2_EMU(vic2.rasterline) >= VIC2_RASTER_2_EMU(VIC2_FIRST_DISP_LINE)) &&
						(VIC2_RASTER_2_EMU(vic2.rasterline ) <= VIC2_RASTER_2_EMU(VIC2_LAST_DISP_LINE)));
		}

		border_on_sample[0] = border_on;
		SprPtrAccess(3);
		SprDataAccess(3, 0);
		DisplayIfBadLine;

		if(spr_dma_on & 0x08)
			SuspendCPU
		else
			ResumeCPU

		vic2.cycle++;
		break;

	// Sprite 3
	case 2:
		if (vblanking)
		{
			// Vertical blank, reset counters
			vic2.rasterline = vc_base = 0;
			ref_cnt = 0xff;
			vblanking = 0;

			// Trigger raster IRQ if IRQ in line 0
			if (RASTERLINE == 0)
			{
				vic2_set_interrupt(machine, 1);
			}
		}

		vic2.graphic_x = VIC2_X_2_EMU(0);

		SprDataAccess(3, 1);
		SprDataAccess(3, 2);
		DisplayIfBadLine;

		vic2.cycle++;
		break;

	// Sprite 4
	case 3:
		SprPtrAccess(4);
		SprDataAccess(4, 0);
		DisplayIfBadLine;

		if(spr_dma_on & 0x10)
			SuspendCPU
		else
			ResumeCPU

		vic2.cycle++;
		break;

	// Sprite 4
	case 4:
		SprDataAccess(4, 1);
		SprDataAccess(4, 2);
		DisplayIfBadLine;

		vic2.cycle++;
		break;

	// Sprite 5
	case 5:
		SprPtrAccess(5);
		SprDataAccess(5, 0);
		DisplayIfBadLine;

		if(spr_dma_on & 0x20)
			SuspendCPU
		else
			ResumeCPU

		vic2.cycle++;
		break;

	// Sprite 5
	case 6:
		SprDataAccess(5, 1);
		SprDataAccess(5, 2);
		DisplayIfBadLine;

		vic2.cycle++;
		break;

	// Sprite 6
	case 7:
		SprPtrAccess(6);
		SprDataAccess(6, 0);
		DisplayIfBadLine;

		if(spr_dma_on & 0x40)
			SuspendCPU
		else
			ResumeCPU

		vic2.cycle++;
		break;

	// Sprite 6
	case 8:
		SprDataAccess(6, 1);
		SprDataAccess(6, 2);
		DisplayIfBadLine;

		vic2.cycle++;
		break;

	// Sprite 7
	case 9:
		SprPtrAccess(7);
		SprDataAccess(7, 0);
		DisplayIfBadLine;

		if(spr_dma_on & 0x80)
			SuspendCPU
		else
			ResumeCPU

		vic2.cycle++;
		break;

	// Sprite 7
	case 10:
		SprDataAccess(7, 1);
		SprDataAccess(7, 2);
		DisplayIfBadLine;

		vic2.cycle++;
		break;

	// Refresh
	case 11:
		RefreshAccess;
		DisplayIfBadLine;

		ResumeCPU

		vic2.cycle++;
		break;

	// Refresh, fetch if bad line
	case 12:
		RefreshAccess;
		FetchIfBadLine;

		vic2.cycle++;
		break;

	// Refresh, fetch if bad line, raster_x
	case 13:
		DrawBackground;
		SampleBorder;
		RefreshAccess;
		FetchIfBadLine;

		vic2.raster_x = 0xfffc;

		vic2.cycle++;
		break;

	// Refresh, fetch if bad line, RC, VC
	case 14:
		DrawBackground;
		SampleBorder;
		RefreshAccess;
		RCIfBadLine;

		vc = vc_base;

		vic2.cycle++;
		break;

	// Refresh, fetch if bad line, sprite y expansion
	case 15:
		DrawBackground;
		SampleBorder;
		RefreshAccess;
		FetchIfBadLine;

		for (i = 0; i < 8; i++)
			if (spr_exp_y & (1 << i))
				mc_base[i] += 2;

		if (is_bad_line)
			SuspendCPU

		ml_index = 0;
		MatrixAccess;

		vic2.cycle++;
		break;

	// Graphics, sprite y expansion, sprite DMA
	case 16:
		DrawBackground;
		SampleBorder;
		GraphicsAccess;
		FetchIfBadLine;

		mask = 1;
		for (i = 0; i < 8; i++, mask <<= 1)
		{
			if (spr_exp_y & mask)
				mc_base[i]++;
			if ((mc_base[i] & 0x3f) == 0x3f)
				spr_dma_on &= ~mask;
		}

		MatrixAccess;

		vic2.cycle++;
		break;

	// Graphics, check border
	case 17:
		if (COLUMNS40)
		{
			if (vic2.rasterline == dy_stop)
				ud_border_on = 1;
			else
			{
				if (SCREENON)
				{
					if (vic2.rasterline == dy_start)
						border_on = ud_border_on = 0;
					else
						if (ud_border_on == 0)
							border_on = 0;
				} else
					if (ud_border_on == 0)
						border_on = 0;
			}
		}

		// Second sample of border state
		border_on_sample[1] = border_on;

		DrawBackground;
		DrawGraphics;
		SampleBorder;
		GraphicsAccess;
		FetchIfBadLine;
		MatrixAccess;

		vic2.cycle++;
		break;

	// Check border
	case 18:
		if (!COLUMNS40)
		{
			if (vic2.rasterline == dy_stop)
				ud_border_on = 1;
			else
			{
				if (SCREENON)
				{
					if (vic2.rasterline == dy_start)
						border_on = ud_border_on = 0;
					else
						if (ud_border_on == 0)
							border_on = 0;
				} else
					if (ud_border_on == 0)
						border_on = 0;
			}
		}

		// Third sample of border state
		border_on_sample[2] = border_on;

	// Graphics

	case 19:
	case 20:
	case 21:
	case 22:
	case 23:
	case 24:
	case 25:
	case 26:
	case 27:
	case 28:
	case 29:
	case 30:
	case 31:
	case 32:
	case 33:
	case 34:
	case 35:
	case 36:
	case 37:
	case 38:
	case 39:
	case 40:
	case 41:
	case 42:
	case 43:
	case 44:
	case 45:
	case 46:
	case 47:
	case 48:
	case 49:
	case 50:
	case 51:
	case 52:
	case 53:
	case 54:
		DrawGraphics;
		SampleBorder;
		GraphicsAccess;
		FetchIfBadLine;
		MatrixAccess;
		last_char_data = char_data;

		vic2.cycle++;
		break;

	// Graphics, sprite y expansion, sprite DMA
	case 55:
		DrawGraphics;
		SampleBorder;
		GraphicsAccess;
		DisplayIfBadLine;

		// sprite y expansion
		mask = 1;
		for (i = 0; i < 8; i++, mask <<= 1)
			if (SPRITE_Y_EXPAND (i))
				spr_exp_y ^= mask;

		CheckSpriteDMA;

		ResumeCPU

		vic2.cycle++;
		break;

	// Check border, sprite DMA
	case 56:
		if (!COLUMNS40)
			border_on = 1;

		// Fourth sample of border state
		border_on_sample[3] = border_on;

		DrawGraphics;
		SampleBorder;
		IdleAccess;
		DisplayIfBadLine;
		CheckSpriteDMA;

		vic2.cycle++;
		break;

	// Check border, sprites
	case 57:
		if (COLUMNS40)
			border_on = 1;

		// Fifth sample of border state
		border_on_sample[4] = border_on;

		// Sample spr_disp_on and spr_data for sprite drawing
		spr_draw = spr_disp_on;
		if (spr_draw)
			memcpy(spr_draw_data, spr_data, 8 * 4);

		mask = 1;
		for (i = 0; i < 8; i++, mask <<= 1)
			if ((spr_disp_on & mask) && !(spr_dma_on & mask))
				spr_disp_on &= ~mask;

		DrawBackground;
		SampleBorder;
		IdleAccess;
		DisplayIfBadLine;

		vic2.cycle++;
		break;

	// for NTSC 6567R8
	case 58:
		DrawBackground;
		SampleBorder;
		IdleAccess;
		DisplayIfBadLine;

		vic2.cycle++;
		break;

	// for NTSC 6567R8
	case 59:
		DrawBackground;
		SampleBorder;
		IdleAccess;
		DisplayIfBadLine;

		vic2.cycle++;
		break;

	// Sprite 0, sprite DMA, MC, RC
	case 60:
		DrawBackground;
		SampleBorder;

		mask = 1;
		for (i = 0; i < 8; i++, mask <<= 1)
		{
			mc[i] = mc_base[i];
			if ((spr_dma_on & mask) && ((vic2.rasterline & 0xff) == SPRITE_Y_POS2(i)))
				spr_disp_on |= mask;
		}

		SprPtrAccess(0);
		SprDataAccess(0, 0);

		if (rc == 7)
		{
			vc_base = vc;
			display_state = 0;
		}

		if (is_bad_line || display_state)
		{
			display_state = 1;
			rc = (rc + 1) & 7;
		}

		if(spr_dma_on & 0x01)
			SuspendCPU
		else
			ResumeCPU

		vic2.cycle++;
		break;

	// Sprite 0
	case 61:
		DrawBackground;
		SampleBorder;
		SprDataAccess(0, 1);
		SprDataAccess(0, 2);
		DisplayIfBadLine;

		vic2.cycle++;
		break;

	// Sprite 1, draw
	case 62:
		DrawBackground;
		SampleBorder;

		if (draw_this_line)
		{
			DrawSprites;

			if (border_on_sample[0])
				for (i = 0; i < 4; i++)
					plot_box(vic2.bitmap, VIC2_X_2_EMU(i * 8), VIC2_RASTER_2_EMU(vic2.rasterline), 8, 1, border_color_sample[i]);

			if (border_on_sample[1])
				plot_box(vic2.bitmap, VIC2_X_2_EMU(4 * 8), VIC2_RASTER_2_EMU(vic2.rasterline), 8, 1, border_color_sample[4]);

			if (border_on_sample[2])
				for (i = 5; i < 43; i++)
					plot_box(vic2.bitmap, VIC2_X_2_EMU(i * 8), VIC2_RASTER_2_EMU(vic2.rasterline), 8, 1, border_color_sample[i]);

			if (border_on_sample[3])
				plot_box(vic2.bitmap, VIC2_X_2_EMU(43 * 8), VIC2_RASTER_2_EMU(vic2.rasterline), 8, 1, border_color_sample[43]);

			if (border_on_sample[4])
			{
				for (i = 44; i < 48; i++)
					plot_box(vic2.bitmap, VIC2_X_2_EMU(i * 8), VIC2_RASTER_2_EMU(vic2.rasterline), 8, 1, border_color_sample[i]);
				for (i = 48; i < 53; i++)
					plot_box(vic2.bitmap, VIC2_X_2_EMU(i * 8), VIC2_RASTER_2_EMU(vic2.rasterline), 8, 1, border_color_sample[47]);
			}
		}

		SprPtrAccess(1);
		SprDataAccess(1, 0);
		DisplayIfBadLine;

		if(spr_dma_on & 0x02)
			SuspendCPU
		else
			ResumeCPU

		vic2.cycle++;
		break;

	// Sprite 1
	case 63:
		SprDataAccess(1, 1);
		SprDataAccess(1, 2);
		DisplayIfBadLine;

		vic2.cycle++;
		break;

	// Sprite 2
	case 64:
		SprPtrAccess(2);
		SprDataAccess(2, 0);
		DisplayIfBadLine;

		if(spr_dma_on & 0x04)
			SuspendCPU
		else
			ResumeCPU

		vic2.cycle++;
		break;

	// Sprite 2
	case 65:
		SprDataAccess(2, 1);
		SprDataAccess(2, 2);
		DisplayIfBadLine;

		if (vic2.rasterline == dy_stop)
			ud_border_on = 1;
		else
			if (SCREENON && (vic2.rasterline == dy_start))
				ud_border_on = 0;

		// Last cycle
		vic2.cycle = 1;
	}

	vic2.raster_x += 8;
	timer_set(machine, cputag_clocks_to_attotime(machine, "maincpu", 1), NULL, 0, NTSC_timer_callback);
}

VIDEO_UPDATE( vic2 )
{
	if (vic2.on)
		copybitmap(bitmap, vic2.bitmap, 0, 0, 0, 0, cliprect);
	return 0;
}

static PALETTE_INIT( vic2 )
{
	int i;

	for ( i = 0; i < sizeof(vic2_palette) / 3; i++ ) {
		palette_set_color_rgb(machine, i, vic2_palette[i*3], vic2_palette[i*3+1], vic2_palette[i*3+2]);
	}
}

MACHINE_DRIVER_START( vh_vic2 )
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(VIC6567_COLUMNS, VIC6567_LINES)
	MDRV_SCREEN_VISIBLE_AREA(0, VIC6567_VISIBLECOLUMNS - 1, 0, VIC6567_VISIBLELINES - 1)
	MDRV_PALETTE_INIT( vic2 )
	MDRV_PALETTE_LENGTH(ARRAY_LENGTH(vic2_palette) / 3)
	MDRV_VIDEO_START( vic2 )
	MDRV_VIDEO_UPDATE( vic2 )
MACHINE_DRIVER_END

MACHINE_DRIVER_START( vh_vic2_pal )
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(VIC6569_COLUMNS, VIC6569_LINES)
	MDRV_SCREEN_VISIBLE_AREA(0, VIC6569_VISIBLECOLUMNS - 1, 0, VIC6569_VISIBLELINES - 1)
	MDRV_PALETTE_INIT( vic2 )
	MDRV_PALETTE_LENGTH(ARRAY_LENGTH(vic2_palette) / 3)
	MDRV_VIDEO_START( vic2 )
	MDRV_VIDEO_UPDATE( vic2 )
MACHINE_DRIVER_END

