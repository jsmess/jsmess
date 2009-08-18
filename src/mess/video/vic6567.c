/***************************************************************************

	Video Interface Chip 6567 (6566 6569 and some other)
	PeT mess@utanet.at

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

#include "driver.h"
#include "vic6567.h"
#include "vic4567.h"
#include "utils.h"

/* lightpen values */
#include "includes/c64.h"

// emu_timer *vicii_scanline_timer;

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



/*#define GFX */

#define VREFRESHINLINES 28

#define VIC2_YPOS 50
#define RASTERLINE_2_C64(a) (a)
#define C64_2_RASTERLINE(a) (a)
#define XPOS (VIC2_STARTVISIBLECOLUMNS + (VIC2_VISIBLECOLUMNS - VIC2_HSIZE)/2)
#define YPOS (VIC2_STARTVISIBLELINES /* + (VIC2_VISIBLELINES - VIC2_VSIZE)/2 */)
#define FIRSTLINE 36 /* ((VIC2_VISIBLELINES - VIC2_VSIZE)/2) */

/* 2008-05 FP: lightpen code needs to read input port from c64.c and cbmb.c */
#define LIGHTPEN_BUTTON		(input_port_read(machine, "OTHER") & 0x04)
#define LIGHTPEN_X_VALUE	(input_port_read(machine, "LIGHTX") & ~0x01)
#define LIGHTPEN_Y_VALUE	(input_port_read(machine, "LIGHTY") & ~0x01)

/* lightpen delivers values from internal counters
 * they do not start with the visual area or frame area */
#define VIC2_MAME_XPOS 0
#define VIC2_MAME_YPOS 0
#define VIC6567_X_BEGIN 38
#define VIC6567_Y_BEGIN -6			   /* first 6 lines after retrace not for lightpen! */
#define VIC6569_X_BEGIN 38
#define VIC6569_Y_BEGIN -6
#define VIC2_X_BEGIN (vic2.pal?VIC6569_X_BEGIN:VIC6567_X_BEGIN)
#define VIC2_Y_BEGIN (vic2.pal?VIC6569_Y_BEGIN:VIC6567_Y_BEGIN)
#define VIC2_X_VALUE ((LIGHTPEN_X_VALUE+VIC2_X_BEGIN \
                          +VIC2_MAME_XPOS)/2)
#define VIC2_Y_VALUE ((LIGHTPEN_Y_VALUE+VIC2_Y_BEGIN \
                          +VIC2_MAME_YPOS))

#define VIC2E_K0_LEVEL (vic2.reg[0x2f]&1)
#define VIC2E_K1_LEVEL (vic2.reg[0x2f]&2)
#define VIC2E_K2_LEVEL (vic2.reg[0x2f]&4)

/*#define VIC3_P5_LEVEL (vic2.reg[0x30]&0x20) */
#define VIC3_BITPLANES (vic2.reg[0x31]&0x10)
#define VIC3_80COLUMNS (vic2.reg[0x31]&0x80)
#define VIC3_LINES	   ((vic2.reg[0x31]&0x19)==0x19?400:200)
#define VIC3_BITPLANES_WIDTH (vic2.reg[0x31]&0x80?640:320)

/*#define VIC2E_TEST (vic2[0x30]&2) */
#define DOUBLE_CLOCK (vic2.reg[0x30]&1)

/* sprites 0 .. 7 */
#define SPRITE_BASE_X_SIZE 24
#define SPRITE_BASE_Y_SIZE 21
#define SPRITEON(nr) (vic2.reg[0x15]&(1<<nr))
#define SPRITE_Y_EXPAND(nr) (vic2.reg[0x17]&(1<<nr))
#define SPRITE_Y_SIZE(nr) (SPRITE_Y_EXPAND(nr)?2*21:21)
#define SPRITE_X_EXPAND(nr) (vic2.reg[0x1d]&(1<<nr))
#define SPRITE_X_SIZE(nr) (SPRITE_X_EXPAND(nr)?2*24:24)
#define SPRITE_X_POS(nr) ( (vic2.reg[(nr)*2]|(vic2.reg[0x10]&(1<<(nr))?0x100:0))-24+XPOS )
#define SPRITE_Y_POS(nr) (vic2.reg[1+2*(nr)]-50+YPOS)
#define SPRITE_MULTICOLOR(nr) (vic2.reg[0x1c]&(1<<nr))
#define SPRITE_PRIORITY(nr) (vic2.reg[0x1b]&(1<<nr))
#define SPRITE_MULTICOLOR1 (vic2.reg[0x25]&0xf)
#define SPRITE_MULTICOLOR2 (vic2.reg[0x26]&0xf)
#define SPRITE_COLOR(nr) (vic2.reg[0x27+nr]&0xf)
#define SPRITE_ADDR(nr) (vic2.videoaddr+0x3f8+nr)
#define SPRITE_BG_COLLISION(nr) (vic2.reg[0x1f]&(1<<nr))
#define SPRITE_COLLISION(nr) (vic2.reg[0x1e]&(1<<nr))
#define SPRITE_SET_BG_COLLISION(nr) (vic2.reg[0x1f]|=(1<<nr))
#define SPRITE_SET_COLLISION(nr) (vic2.reg[0x1e]|=(1<<nr))

// for sprite's changes in the middle of a raster line
#define SPRITE_X_POS_BUFFER(nr) ( (vic2.reg_buffer[(nr)*2]|(vic2.reg_buffer[0x10]&(1<<(nr))?0x100:0))-24+XPOS )
#define SPRITE_Y_POS_BUFFER(nr) (vic2.reg_buffer[1+2*(nr)]-50+YPOS)
#define SPRITE_MULTICOLOR1_BUFFER (vic2.reg_buffer[0x25]&0xf)
#define SPRITE_MULTICOLOR2_BUFFER (vic2.reg_buffer[0x26]&0xf)

#define SCREENON (vic2.reg[0x11]&0x10)
#define VERTICALPOS (vic2.reg[0x11]&7)
#define HORIZONTALPOS (vic2.reg[0x16]&7)
#define ECMON (vic2.reg[0x11]&0x40)
#define HIRESON (vic2.reg[0x11]&0x20)
#define MULTICOLORON (vic2.reg[0x16]&0x10)
#define LINES25 (vic2.reg[0x11]&8)		   /* else 24 Lines */
#define LINES (LINES25?25:24)
#define YSIZE (LINES*8)
#define COLUMNS40 (vic2.reg[0x16]&8)	   /* else 38 Columns */
#define COLUMNS (COLUMNS40?40:38)
#define XSIZE (COLUMNS*8)

#define VIDEOADDR ( (vic2.reg[0x18]&0xf0)<<(10-4) )
#define CHARGENADDR ((vic2.reg[0x18]&0xe)<<10)

#define RASTERLINE ( ((vic2.reg[0x11]&0x80)<<1)|vic2.reg[0x12])

#define BACKGROUNDCOLOR (vic2.reg[0x21]&0xf)
#define MULTICOLOR1 (vic2.reg[0x22]&0xf)
#define MULTICOLOR2 (vic2.reg[0x23]&0xf)
#define FOREGROUNDCOLOR (vic2.reg[0x24]&0xf)
#define FRAMECOLOR (vic2.reg[0x20]&0xf)

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
	UINT8 reg[0x80],reg_buffer[0x80]; 	// buffer for sprite's changes in a middle of a rasterline
	int pal;
	int vic2e;		     /* version with some port lines */
	int vic3;
	int on; /* rastering of the screen */

	int (*dma_read)(running_machine *, int);
	int (*dma_read_color)(running_machine *, int);
	void (*interrupt) (running_machine *, int);
	void (*port_changed)(running_machine *, int);

	int lines;

	int chargenaddr, videoaddr;

	bitmap_t *bitmap;
	int x_begin, x_end;
	int y_begin, y_end;

	UINT16 c64_bitmap[2], bitmapmulti[4], mono[2], multi[4], ecmcolor[2], colors[4], spritemulti[4], spritemulti_buffer[4];

	int lastline, rasterline, raster_mod;
	UINT64 rasterline_start_cpu_cycles, totalcycles;
	UINT8 rasterX;

	/* background/foreground for sprite collision */
	UINT8 *screen[216], shift[216];

	/* convert multicolor byte to background/foreground for sprite collision */
	UINT8 foreground[256];
	UINT16 expandx[256];
	UINT16 expandx_multi[256];

	/* converts sprite multicolor info to info for background collision checking */
	UINT8 multi_collision[256];

	struct {
		int on, x, y, xexpand, yexpand;

		int repeat;						   /* y expand, line once drawn */
		int line;						   /* 0 not painting, else painting */

		/* buffer for currently painted line */
		int paintedline[8];
	    UINT8 bitmap[8][SPRITE_BASE_X_SIZE * 2 / 8 + 1/*for simplier sprite collision detection*/];
	}
	sprites[8]; // buffer for sprite's changes in a middle of a rasterline
	struct {
		int on, x, y, xexpand, yexpand;
	}
	sprites_buffer[8];
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

	vic2.totalcycles = 63;
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
	int cycles, realx;
	DBG_LOG (2, "vic write", ("%.2x:%.2x\n", offset, data));
	offset &= 0x3f;
	switch (offset)
	{
	case 1:
	case 3:
	case 5:
	case 7:
	case 9:
	case 0xb:
	case 0xd:
	case 0xf:
		/* sprite y positions */
		if (vic2.reg[offset] != data)
		{
			vic2.reg_buffer[offset] = data;
			vic2.sprites_buffer[offset/2].y = SPRITE_Y_POS_BUFFER(offset/2);
		}
		break;
	case 0:
	case 2:
	case 4:
	case 6:
	case 8:
	case 0xa:
	case 0xc:
	case 0xe:
		/* sprite x positions */
		if (vic2.reg[offset] != data)
		{
			vic2.reg_buffer[offset] = data;
			vic2.sprites_buffer[offset/2].x = SPRITE_X_POS_BUFFER(offset/2);
		}
		break;
	case 0x10:
		/* sprite x positions */
		if (vic2.reg[offset] != data)
		{
			vic2.reg_buffer[offset] = data;
			vic2.sprites_buffer[0].x = SPRITE_X_POS_BUFFER(0);
			vic2.sprites_buffer[1].x = SPRITE_X_POS_BUFFER(1);
			vic2.sprites_buffer[2].x = SPRITE_X_POS_BUFFER(2);
			vic2.sprites_buffer[3].x = SPRITE_X_POS_BUFFER(3);
			vic2.sprites_buffer[4].x = SPRITE_X_POS_BUFFER(4);
			vic2.sprites_buffer[5].x = SPRITE_X_POS_BUFFER(5);
			vic2.sprites_buffer[6].x = SPRITE_X_POS_BUFFER(6);
			vic2.sprites_buffer[7].x = SPRITE_X_POS_BUFFER(7);
		}
		break;
	case 0x17:						   /* sprite y size */
	case 0x1d:						   /* sprite x size */
	case 0x1b:						   /* sprite background priority */
	case 0x1c:						   /* sprite multicolor mode select */
	case 0x27:
	case 0x28:						   /* sprite colors */
	case 0x29:
	case 0x2a:
	case 0x2b:
	case 0x2c:
	case 0x2d:
	case 0x2e:
		if (vic2.reg[offset] != data)
		{
			vic2.reg_buffer[offset] = data;
		}
		break;
	case 0x25:
		if (vic2.reg[offset] != data)
		{
			vic2.reg_buffer[offset] = data;
			vic2.spritemulti_buffer[1] = SPRITE_MULTICOLOR1_BUFFER;
		}
		break;
	case 0x26:
		if (vic2.reg[offset] != data)
		{
			vic2.reg_buffer[offset] = data;
			vic2.spritemulti_buffer[3] = SPRITE_MULTICOLOR2_BUFFER;
		}
		break;
	case 0x19:
		vic2_clear_interrupt(space->machine, data & 0xf);
		break;
	case 0x1a:						   /* irq mask */
		vic2.reg[offset] = data;
		vic2_set_interrupt(space->machine, 0); 			//beamrider needs this
		break;
	case 0x11:
		if (vic2.reg[offset] != data)
		{
//			vic2.reg_buffer[offset] = data;

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
//			vic2.reg_buffer[offset] = data;

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
		}
		break;
	case 0x21:						   /* background color */
		if (vic2.reg[offset] != data)
		{
			vic2.reg[offset] = data;
			vic2.mono[0] = vic2.bitmapmulti[0] = vic2.multi[0] =
				vic2.colors[0] = BACKGROUNDCOLOR;
		}
		break;
	case 0x22:						   /* background color 1 */
		if (vic2.reg[offset] != data)
		{
			vic2.reg[offset] = data;
			vic2.multi[1] = vic2.colors[1] = MULTICOLOR1;
		}
		break;
	case 0x23:						   /* background color 2 */
		if (vic2.reg[offset] != data)
		{
			vic2.reg[offset] = data;
			vic2.multi[2] = vic2.colors[2] = MULTICOLOR2;
		}
		break;
	case 0x24:						   /* background color 3 */
		if (vic2.reg[offset] != data)
		{
			vic2.reg[offset] = data;
			vic2.colors[3] = FOREGROUNDCOLOR;
		}
		break;
	case 0x20:						   /* framecolor */
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

	cycles = (int)(cpu_get_total_cycles(space->cpu) - vic2.rasterline_start_cpu_cycles);

	realx = VIC2_FIRST_X + cycles * 8 * (VIC2_CYCLESPERLINE / vic2.totalcycles);
	if (realx > VIC2_MAX_X) realx -= VIC2_MAX_X;

	// sprites chenges are active only from the next raster line

	if (vic2.on)
		if ((vic2.rasterline >= VIC2_FIRSTRASTERLINE) && (vic2.rasterline < VIC2_FIRSTRASTERLINE + VIC2_VISIBLELINES))
			{
				if ((realx >= 0) && (realx <= (VIC2_LAST_VISIBLE_X - VIC2_FIRST_VISIBLE_X)))
				{
					vic2_drawlines (machine, vic2.rasterline, vic2.rasterline + 1, realx, VIC2_LAST_VISIBLE_X - VIC2_FIRST_VISIBLE_X);
				}
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
		val = vic2.reg[offset] | 1;
		break;
	case 0x19:						   /* interrupt flag register */
		/*    vic2_clear_interrupt(0xf); */
		val = vic2.reg[offset] | 0x70;
		break;
	case 0x1a:
		val = vic2.reg[offset] | 0xf0;
		break;
	case 0x1e:						   /* sprite to sprite collision detect */
		val = vic2.reg[offset];
		vic2.reg[offset] = 0;
		vic2_clear_interrupt(space->machine, 4);
		break;
	case 0x1f:						   /* sprite to background collision detect */
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
		val = vic2.reg_buffer[offset];
		break;
	case 0x2f:
	case 0x30:
		if (vic2.vic2e) {
			val = vic2.reg[offset];
			DBG_LOG (2, "vic read", ("%.2x:%.2x\n", offset, val));
		} else
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
	case 0x3f:						   /* not used */
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
	vic2.rasterline = 0;

	// from 1 to 63 (PAL) or from 1 to 65 (NTSC)
	vic2.rasterX = 1; 

	// immediately call the timer to handle the first line
	timer_set(machine, cputag_clocks_to_attotime(machine, "maincpu", 0), NULL, 0, rz_timer_callback);
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
static int is_bad_line = 0;

TIMER_CALLBACK( rz_timer_callback )
{
	int i,j;

	switch(vic2.rasterX)
	{

	case 1:
		is_bad_line = ((vic2.rasterline >= 0x30) && (vic2.rasterline <= 0xf7) && ((vic2.rasterline & 0x07) == VERTICALPOS) && SCREENON);

		vic2.rasterline_start_cpu_cycles = cpu_get_total_cycles(machine->cpu[0]);

		// update sprites registers
		for (i=0x00; i <= 0x10; i++)
			vic2.reg[i] = vic2.reg_buffer[i];
		vic2.reg[0x17] = vic2.reg_buffer[0x17];
		for (i=0x1b; i <= 0x1d; i++)
			vic2.reg[i] = vic2.reg_buffer[i];
		for (i=0x25; i <= 0x2e; i++)
			vic2.reg[i] = vic2.reg_buffer[i];	
		for (i=0; i < 8; i++)
		{
			vic2.sprites[i].x = vic2.sprites_buffer[i].x;
			vic2.sprites[i].y = vic2.sprites_buffer[i].y;
		}
		for (i=0; i < 4; i++)
			vic2.spritemulti[i] = vic2.spritemulti_buffer[i];

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

		if (vic2.rasterline == C64_2_RASTERLINE(RASTERLINE))
		{
			vic2_set_interrupt(machine, 1);
		}

		vic2.rasterX++;
		break;

	case 15:
		vic2.rasterX++;
		if (is_bad_line) cpu_suspend(machine->cpu[0], SUSPEND_REASON_SPIN, 0);

		if (!c64_pal)
			if (vic2.rasterline < VIC2_FIRSTRASTERLINE + VIC2_VISIBLELINES - vic2.lines)
				vic2_drawlines (machine, vic2.rasterline + vic2.lines, vic2.rasterline + vic2.lines + 1, VIC2_STARTVISIBLECOLUMNS, VIC2_STARTVISIBLECOLUMNS + VIC2_VISIBLECOLUMNS);

		if (vic2.on)
			if ((vic2.rasterline >= VIC6569_STARTVISIBLELINES ) && (vic2.rasterline < VIC6569_STARTVISIBLELINES + VIC2_VISIBLELINES))
				vic2_drawlines (machine, vic2.rasterline, vic2.rasterline + 1, VIC2_STARTVISIBLECOLUMNS, VIC2_STARTVISIBLECOLUMNS + VIC2_VISIBLECOLUMNS);

		break;

	case 55:
		vic2.rasterX++;

		if (is_bad_line) cpu_resume(machine->cpu[0], SUSPEND_REASON_SPIN);

		break;


	case 63:
		vic2.rasterline++;
		vic2.rasterX = 1;

		break;

	default:
		vic2.rasterX++;
		
		break;
	}

	timer_set(machine, cputag_clocks_to_attotime(machine, "maincpu", 1), NULL, 0, rz_timer_callback);
}

VIDEO_UPDATE( vic2 )
{
	//vic2.rasterline = 0;
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
	MDRV_SCREEN_VISIBLE_AREA(VIC6567_STARTVISIBLECOLUMNS ,VIC6567_STARTVISIBLECOLUMNS + VIC6567_VISIBLECOLUMNS - 1, VIC6567_STARTVISIBLELINES, VIC6567_STARTVISIBLELINES + VIC6567_VISIBLELINES - 1)
	MDRV_PALETTE_INIT( vic2 )
	MDRV_PALETTE_LENGTH(sizeof (vic2_palette) / sizeof (vic2_palette[0]) / 3)
	MDRV_VIDEO_START( vic2 )
	MDRV_VIDEO_UPDATE( vic2 )
MACHINE_DRIVER_END

MACHINE_DRIVER_START( vh_vic2_pal )
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(VIC6569_COLUMNS, VIC6569_LINES)
	MDRV_SCREEN_VISIBLE_AREA(VIC6569_STARTVISIBLECOLUMNS, VIC6569_STARTVISIBLECOLUMNS + VIC6569_VISIBLECOLUMNS - 1, VIC6569_STARTVISIBLELINES, VIC6569_STARTVISIBLELINES + VIC6569_VISIBLELINES - 1)
	MDRV_PALETTE_INIT( vic2 )
	MDRV_PALETTE_LENGTH(sizeof (vic2_palette) / sizeof (vic2_palette[0]) / 3)
	MDRV_VIDEO_START( vic2 )
	MDRV_VIDEO_UPDATE( vic2 )
MACHINE_DRIVER_END
