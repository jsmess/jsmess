/***************************************************************************

  MOS video interface chip 6560
  PeT mess@utanet.at

***************************************************************************/

#include <math.h>
#include <stdio.h>
#include "emu.h"
#include "utils.h"

#include "vic6560.h"
#include "sound/dac.h"

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

static const unsigned char vic6560_palette[] =
{
/* ripped from vice, a very excellent emulator */
/* black, white, red, cyan */
	0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0xf0, 0xf0,
/* purple, green, blue, yellow */
	0x60, 0x00, 0x60, 0x00, 0xa0, 0x00, 0x00, 0x00, 0xf0, 0xd0, 0xd0, 0x00,
/* orange, light orange, pink, light cyan, */
	0xc0, 0xa0, 0x00, 0xff, 0xa0, 0x00, 0xf0, 0x80, 0x80, 0x00, 0xff, 0xff,
/* light violett, light green, light blue, light yellow */
	0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0x00, 0xa0, 0xff, 0xff, 0xff, 0x00
};

UINT8 vic6560[16];

/* 2008-05 FP: lightpen code needs to read input port from vc20.c */

#define LIGHTPEN_BUTTON		(((input_port_read(space->machine, "CTRLSEL") & 0xf0) == 0x20) && (input_port_read(space->machine, "JOY") & 0x40))
#define LIGHTPEN_X_VALUE	(input_port_read(space->machine, "LIGHTX") & ~0x01)
#define LIGHTPEN_Y_VALUE	(input_port_read(space->machine, "LIGHTY") & ~0x01)

/* lightpen delivers values from internal counters
 * they do not start with the visual area or frame area */
#define VIC6560_X_BEGIN 38
#define VIC6560_Y_BEGIN -6			   /* first 6 lines after retrace not for lightpen! */
#define VIC6561_X_BEGIN 38
#define VIC6561_Y_BEGIN -6
#define VIC656X_X_BEGIN (vic6560_pal?VIC6561_X_BEGIN:VIC6560_X_BEGIN)
#define VIC656X_Y_BEGIN (vic6560_pal?VIC6561_Y_BEGIN:VIC6560_Y_BEGIN)
/* lightpen behaviour in pal or mono multicolor not tested */
#define VIC6560_X_VALUE ((LIGHTPEN_X_VALUE+VIC656X_X_BEGIN \
			  +VIC656X_MAME_XPOS)/2)
#define VIC6560_Y_VALUE ((LIGHTPEN_Y_VALUE+VIC656X_Y_BEGIN \
			  +VIC656X_MAME_YPOS)/2)

#define INTERLACE (vic6560&0x80)	   /*only ntsc version */
/* ntsc 1 - 8 */
/* pal 5 - 19 */
#define XPOS (((int)vic6560[0]&0x7f)*4)
#define YPOS ((int)vic6560[1]*2)

/* ntsc values >= 31 behave like 31 */
/* pal value >= 32 behave like 32 */
#define CHARS_X ((int)vic6560[2]&0x7f)
#define CHARS_Y (((int)vic6560[3]&0x7e)>>1)

#define MATRIX8X16 (vic6560[3]&1)	   /* else 8x8 */
#define CHARHEIGHT (MATRIX8X16?16:8)
#define XSIZE (CHARS_X*8)
#define YSIZE (CHARS_Y*CHARHEIGHT)

/* colorram and backgroundcolor are changed */
#define INVERTED (!(vic6560[0x0f]&8))

#define CHARGENADDR (((int)vic6560[5]&0xf)<<10)
#define VIDEOADDR ( ( ((int)vic6560[5]&0xf0)<<(10-4))\
		    | ( ((int)vic6560[2]&0x80)<<(9-7)) )
#define VIDEORAMSIZE (YSIZE*XSIZE)
#define CHARGENSIZE (256*HEIGHTPIXEL)

#define HELPERCOLOR (vic6560[0xe]>>4)
#define BACKGROUNDCOLOR (vic6560[0xf]>>4)
#define FRAMECOLOR (vic6560[0xf]&7)

int vic6560_pal;

static bitmap_t *vic6560_bitmap;
static int rasterline = 0, lastline = 0;
static void vic6560_drawlines (running_machine *machine, int start, int last);

static int (*vic_dma_read) (running_machine *machine, int);
static int (*vic_dma_read_color) (running_machine *machine, int);

static int vic656x_xsize, vic656x_ysize, vic656x_lines, vic656x_vretracerate;
static int charheight, matrix8x16, inverted;
static int chars_x, chars_y;
static int xsize, ysize, xpos, ypos;
static int chargenaddr, videoaddr;

/* values in videoformat */
static UINT16 backgroundcolor, framecolor, helpercolor;

/* arrays for bit to color conversion without condition checking */
static UINT16 mono[2], monoinverted[2], multi[4], multiinverted[4];

static void vic656x_init (void)
{
	vic656x_xsize = VIC656X_XSIZE;
	vic656x_ysize = VIC656X_YSIZE;
	vic656x_lines = VIC656X_LINES;
	vic656x_vretracerate = VIC656X_VRETRACERATE;
}

void vic6560_init (int (*dma_read) (running_machine *machine, int), int (*dma_read_color) (running_machine *machine, int))
{
	vic6560_pal = FALSE;
	vic_dma_read = dma_read;
	vic_dma_read_color = dma_read_color;
	vic656x_init ();
}

void vic6561_init (int (*dma_read) (running_machine *machine, int), int (*dma_read_color) (running_machine *machine, int))
{
	vic6560_pal = TRUE;
	vic_dma_read = dma_read;
	vic_dma_read_color = dma_read_color;
	vic656x_init ();
}


static VIDEO_START( vic6560 )
{
	const device_config *screen = video_screen_first(machine->config);
	int width = video_screen_get_width(screen);
	int height = video_screen_get_height(screen);

	vic6560_bitmap = auto_bitmap_alloc(machine, width, height, BITMAP_FORMAT_INDEXED16);
}

WRITE8_HANDLER ( vic6560_port_w )
{
	running_machine *machine = space->machine;
	DBG_LOG (1, "vic6560_port_w", ("%.4x:%.2x\n", offset, data));
	switch (offset)
	{
	case 0xa:
	case 0xb:
	case 0xc:
	case 0xd:
	case 0xe:
		vic6560_soundport_w (space->machine, offset, data);
		break;
	}
	if (vic6560[offset] != data)
	{
		switch (offset)
		{
		case 0:
		case 1:
		case 2:
		case 3:
		case 5:
		case 0xe:
		case 0xf:
			vic6560_drawlines (space->machine, lastline, rasterline);
			break;
		}
		vic6560[offset] = data;
		switch (offset)
		{
		case 0:
			xpos = XPOS;
			break;
		case 1:
			ypos = YPOS;
			break;
		case 2:
			/* ntsc values >= 31 behave like 31 */
			/* pal value >= 32 behave like 32 */
			chars_x = CHARS_X;
			videoaddr = VIDEOADDR;
			xsize = XSIZE;
			break;
		case 3:
			matrix8x16 = MATRIX8X16;
			charheight = CHARHEIGHT;
			chars_y = CHARS_Y;
			ysize = YSIZE;
			break;
		case 5:
			chargenaddr = CHARGENADDR;
			videoaddr = VIDEOADDR;
			break;
		case 0xe:
			multi[3] = multiinverted[3] = helpercolor = HELPERCOLOR;
			break;
		case 0xf:
			inverted = INVERTED;
			multi[1] = multiinverted[1] = framecolor = FRAMECOLOR;
			mono[0] = monoinverted[1] =
				multi[0] = multiinverted[2] = backgroundcolor = BACKGROUNDCOLOR;
			break;
		}
	}
}

READ8_HANDLER ( vic6560_port_r )
{
	running_machine *machine = space->machine;
	static double lightpenreadtime = 0.0;
	int val;

	switch (offset)
	{
	case 3:
		val = ((rasterline & 1) << 7) | (vic6560[offset] & 0x7f);
		break;
	case 4:						   /*rasterline */
		vic6560_drawlines (space->machine, lastline, rasterline);
		val = (rasterline / 2) & 0xff;
		break;
	case 6:						   /*lightpen horizontal */
	case 7:						   /*lightpen vertical */
		if (LIGHTPEN_BUTTON
			&& ((attotime_to_double(timer_get_time(space->machine)) - lightpenreadtime) * VIC6560_VRETRACERATE >= 1))
		{
			/* only 1 update each frame */
			/* and diode must recognize light */
			if (1)
			{
				vic6560[6] = VIC6560_X_VALUE;
				vic6560[7] = VIC6560_Y_VALUE;
			}
			lightpenreadtime = attotime_to_double(timer_get_time(space->machine));
		}
		val = vic6560[offset];
		break;
	case 8:						   /* poti 1 */
		val = input_port_read(space->machine, "PADDLE0");
		break;
	case 9:						   /* poti 2 */
		val = input_port_read(space->machine, "PADDLE1");
		break;
	default:
		val = vic6560[offset];
		break;
	}
	DBG_LOG (3, "vic6560_port_r", ("%.4x:%.2x\n", offset, val));
	return val;
}

static void vic6560_draw_character (running_machine *machine, int ybegin, int yend,
									int ch, int yoff, int xoff,
									UINT16 *color)
{
	int y, code;

	for (y = ybegin; y <= yend; y++)
	{
		code = vic_dma_read (machine, (chargenaddr + ch * charheight + y) & 0x3fff);
		*BITMAP_ADDR16(vic6560_bitmap, y + yoff, xoff + 0) = color[code >> 7];
		*BITMAP_ADDR16(vic6560_bitmap, y + yoff, xoff + 1) = color[(code >> 6) & 1];
		*BITMAP_ADDR16(vic6560_bitmap, y + yoff, xoff + 2) = color[(code >> 5) & 1];
		*BITMAP_ADDR16(vic6560_bitmap, y + yoff, xoff + 3) = color[(code >> 4) & 1];
		*BITMAP_ADDR16(vic6560_bitmap, y + yoff, xoff + 4) = color[(code >> 3) & 1];
		*BITMAP_ADDR16(vic6560_bitmap, y + yoff, xoff + 5) = color[(code >> 2) & 1];
		*BITMAP_ADDR16(vic6560_bitmap, y + yoff, xoff + 6) = color[(code >> 1) & 1];
		*BITMAP_ADDR16(vic6560_bitmap, y + yoff, xoff + 7) = color[code & 1];
	}
}

static void vic6560_draw_character_multi (running_machine *machine, int ybegin, int yend,
										  int ch, int yoff, int xoff, UINT16 *color)
{
	int y, code;

	for (y = ybegin; y <= yend; y++)
	{
		code = vic_dma_read (machine, (chargenaddr + ch * charheight + y) & 0x3fff);
		*BITMAP_ADDR16(vic6560_bitmap, y + yoff, xoff + 0) =
			*BITMAP_ADDR16(vic6560_bitmap, y + yoff, xoff + 1) = color[code >> 6];
		*BITMAP_ADDR16(vic6560_bitmap, y + yoff, xoff + 2) =
			*BITMAP_ADDR16(vic6560_bitmap, y + yoff, xoff + 3) = color[(code >> 4) & 3];
		*BITMAP_ADDR16(vic6560_bitmap, y + yoff, xoff + 4) =
			*BITMAP_ADDR16(vic6560_bitmap, y + yoff, xoff + 5) = color[(code >> 2) & 3];
		*BITMAP_ADDR16(vic6560_bitmap, y + yoff, xoff + 6) =
			*BITMAP_ADDR16(vic6560_bitmap, y + yoff, xoff + 7) = color[code & 3];
	}
}


static void vic6560_drawlines (running_machine *machine, int first, int last)
{
	int line, vline;
	int offs, yoff, xoff, ybegin, yend, i;
	int attr, ch;

	lastline = last;
	if (first >= last)
		return;

	for (line = first; (line < ypos) && (line < last); line++)
	{
		memset16 (BITMAP_ADDR16(vic6560_bitmap, line, 0), framecolor, vic656x_xsize);
	}

	for (vline = line - ypos; (line < last) && (line < ypos + ysize);)
	{
		if (matrix8x16)
		{
			offs = (vline >> 4) * chars_x;
			yoff = (vline & ~0xf) + ypos;
			ybegin = vline & 0xf;
			yend = (vline + 0xf < last - ypos) ? 0xf : ((last - line) & 0xf) + ybegin;
		}
		else
		{
			offs = (vline >> 3) * chars_x;
			yoff = (vline & ~7) + ypos;
			ybegin = vline & 7;
			yend = (vline + 7 < last - ypos) ? 7 : ((last - line) & 7) + ybegin;
		}

		if (xpos > 0)
		{
			for (i = ybegin; i <= yend; i++)
				memset16 (BITMAP_ADDR16(vic6560_bitmap, yoff + i, 0), framecolor, xpos);
		}
		for (xoff = xpos; (xoff < xpos + xsize) && (xoff < vic656x_xsize); xoff += 8, offs++)
		{
			ch = vic_dma_read (machine, (videoaddr + offs) & 0x3fff);
			attr = (vic_dma_read_color (machine, (videoaddr + offs) & 0x3fff)) & 0xf;
			if (inverted)
			{
				if (attr & 8)
				{
					multiinverted[0] = attr & 7;
					vic6560_draw_character_multi (machine, ybegin, yend, ch, yoff, xoff,
												  multiinverted);
				}
				else
				{
					monoinverted[0] = attr;
					vic6560_draw_character (machine, ybegin, yend, ch, yoff, xoff, monoinverted);
				}
			}
			else
			{
				if (attr & 8)
				{
					multi[2] = attr & 7;
					vic6560_draw_character_multi (machine, ybegin, yend, ch, yoff, xoff, multi);
				}
				else
				{
					mono[1] = attr;
					vic6560_draw_character (machine, ybegin, yend, ch, yoff, xoff, mono);
				}
			}
		}
		if (xoff < vic656x_xsize)
		{
			for (i = ybegin; i <= yend; i++)
			{
				memset16 (BITMAP_ADDR16(vic6560_bitmap, yoff + i, xoff),
					framecolor, vic656x_xsize - xoff);
			}
		}
		if (matrix8x16)
		{
			vline = (vline + 16) & ~0xf;
			line = vline + ypos;
		}
		else
		{
			vline = (vline + 8) & ~7;
			line = vline + ypos;
		}
	}

	for (; line < last; line++)
	{
		memset16 (BITMAP_ADDR16(vic6560_bitmap, line, 0), framecolor, vic656x_xsize);
	}
}

INTERRUPT_GEN( vic656x_raster_interrupt )
{
	rasterline++;
	if (rasterline >= vic656x_lines)
	{
		rasterline = 0;
		vic6560_drawlines (device->machine, lastline, vic656x_lines);
		lastline = 0;
	}
}

static VIDEO_UPDATE( vic6560 )
{
	copybitmap(bitmap, vic6560_bitmap, 0, 0, 0, 0, cliprect);
	return 0;
}

static PALETTE_INIT( vic6560 )
{
	int i;

	for ( i = 0; i < sizeof(vic6560_palette) / 3; i++ ) {
		palette_set_color_rgb(machine, i, vic6560_palette[i*3], vic6560_palette[i*3+1], vic6560_palette[i*3+2]);
	}
}

MACHINE_DRIVER_START( vic6560_video )
    /* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(VIC6560_VRETRACERATE)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE((VIC6560_XSIZE + 7) & ~7, VIC6560_YSIZE)
	MDRV_SCREEN_VISIBLE_AREA(VIC6560_MAME_XPOS, VIC6560_MAME_XPOS + VIC6560_MAME_XSIZE - 1, VIC6560_MAME_YPOS, VIC6560_MAME_YPOS + VIC6560_MAME_YSIZE - 1)
	
	MDRV_PALETTE_LENGTH(16)
	MDRV_PALETTE_INIT(vic6560)

	MDRV_VIDEO_START(vic6560)
	MDRV_VIDEO_UPDATE(vic6560)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("custom", VIC6560, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD("dac", DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( vic6561_video )
    /* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(VIC6561_VRETRACERATE)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE((VIC6561_XSIZE + 7) & ~7, VIC6561_YSIZE)
	MDRV_SCREEN_VISIBLE_AREA(VIC6561_MAME_XPOS, VIC6561_MAME_XPOS + VIC6561_MAME_XSIZE - 1, VIC6561_MAME_YPOS, VIC6561_MAME_YPOS + VIC6561_MAME_YSIZE - 1)
	
	MDRV_PALETTE_LENGTH(16)
	MDRV_PALETTE_INIT(vic6560)

	MDRV_VIDEO_START(vic6560)
	MDRV_VIDEO_UPDATE(vic6560)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("custom", VIC6560, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD("dac", DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END
