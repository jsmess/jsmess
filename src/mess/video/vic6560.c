/***************************************************************************

  MOS video interface chip 6560
  PeT mess@utanet.at

***************************************************************************/

#include <math.h>
#include <stdio.h>
#include "driver.h"
#include "utils.h"
#include "sound/custom.h"

#define VERBOSE_DBG 0
#include "includes/cbm.h"
#include "includes/vc20.h"
#include "includes/cbmserb.h"
#include "includes/vc1541.h"
#include "includes/vc20tape.h"

#include "includes/vic6560.h"

/* usage of GFX system for lightpenpointer drawing,
 * needs little more testing with working dipswitches */
/*#define GFX */

unsigned char vic6560_palette[] =
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

struct CustomSound_interface vic6560_sound_interface =
{
	vic6560_custom_start
};

UINT8 vic6560[16];

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

bool vic6560_pal;

static mame_bitmap *vic6560_bitmap;
static int rasterline = 0, lastline = 0;
static void vic6560_drawlines (int start, int last);

static int (*vic_dma_read) (int);
static int (*vic_dma_read_color) (int);

static int vic656x_xsize, vic656x_ysize, vic656x_lines, vic656x_vretracerate;
static int charheight, matrix8x16, inverted;
static int chars_x, chars_y;
static int xsize, ysize, xpos, ypos;
static int chargenaddr, videoaddr;

/* values in videoformat */
static UINT16 backgroundcolor, framecolor, helpercolor, white, black;

/* arrays for bit to color conversion without condition checking */
static UINT16 mono[2], monoinverted[2], multi[4], multiinverted[4];

/* transparent, white, black */
static UINT32 pointercolortable[3] =
{0};
static gfx_layout pointerlayout =
{
	8, 8,
	1,
	2,
	{0, 64},
	{0, 1, 2, 3, 4, 5, 6, 7},
	{0 * 8, 1 * 8, 2 * 8, 3 * 8, 4 * 8, 5 * 8, 6 * 8, 7 * 8},
	8 * 8
};

static UINT8 pointermask[] =
{
	0x00, 0x70, 0x60, 0x50, 0x08, 0x04, 0x00, 0x00,		/* blackmask */
	0xf0, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00	/* whitemask */
};

static gfx_element *pointerelement;

static void vic656x_init (void)
{
	vic656x_xsize = VIC656X_XSIZE;
	vic656x_ysize = VIC656X_YSIZE;
	vic656x_lines = VIC656X_LINES;
	vic656x_vretracerate = VIC656X_VRETRACERATE;
}

void vic6560_init (int (*dma_read) (int), int (*dma_read_color) (int))
{
	vic6560_pal = FALSE;
	vic_dma_read = dma_read;
	vic_dma_read_color = dma_read_color;
	vic656x_init ();
}

void vic6561_init (int (*dma_read) (int), int (*dma_read_color) (int))
{
	vic6560_pal = TRUE;
	vic_dma_read = dma_read;
	vic_dma_read_color = dma_read_color;
	vic656x_init ();
}

static void vic6560_video_stop(running_machine *machine)
{
	freegfx(pointerelement);
}

VIDEO_START( vic6560 )
{
	black = Machine->pens[0];
	white = Machine->pens[1];
	pointerelement = allocgfx(&pointerlayout);
	decodegfx(pointerelement, pointermask, 0, 1);
	pointerelement->colortable = pointercolortable;
	pointercolortable[1] = Machine->pens[1];
	pointercolortable[2] = Machine->pens[0];
	pointerelement->total_colors = 3;
	vic6560_bitmap = auto_bitmap_alloc(Machine->screen[0].width, Machine->screen[0].height, BITMAP_FORMAT_INDEXED16);
	add_exit_callback(machine, vic6560_video_stop);
	return 0;
}

WRITE8_HANDLER ( vic6560_port_w )
{
	DBG_LOG (1, "vic6560_port_w", (errorlog, "%.4x:%.2x\n", offset, data));
	switch (offset)
	{
	case 0xa:
	case 0xb:
	case 0xc:
	case 0xd:
	case 0xe:
		vic6560_soundport_w (offset, data);
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
			vic6560_drawlines (lastline, rasterline);
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
			multi[3] = multiinverted[3] = helpercolor = Machine->pens[HELPERCOLOR];
			break;
		case 0xf:
			inverted = INVERTED;
			multi[1] = multiinverted[1] = framecolor = Machine->pens[FRAMECOLOR];
			mono[0] = monoinverted[1] =
				multi[0] = multiinverted[2] = backgroundcolor = Machine->pens[BACKGROUNDCOLOR];
			break;
		}
	}
}

 READ8_HANDLER ( vic6560_port_r )
{
	static double lightpenreadtime = 0.0;
	int val;

	switch (offset)
	{
	case 3:
		val = ((rasterline & 1) << 7) | (vic6560[offset] & 0x7f);
		break;
	case 4:						   /*rasterline */
		vic6560_drawlines (lastline, rasterline);
		val = (rasterline / 2) & 0xff;
		break;
	case 6:						   /*lightpen horicontal */
	case 7:						   /*lightpen vertical */
		if (LIGHTPEN_BUTTON
			&& ((timer_get_time () - lightpenreadtime) * VIC6560_VRETRACERATE >= 1))
		{
			/* only 1 update each frame */
			/* and diode must recognize light */
			if (1)
			{
				vic6560[6] = VIC6560_X_VALUE;
				vic6560[7] = VIC6560_Y_VALUE;
			}
			lightpenreadtime = timer_get_time ();
		}
		val = vic6560[offset];
		break;
	case 8:						   /* poti 1 */
		val = PADDLE1_VALUE;
		break;
	case 9:						   /* poti 2 */
		val = PADDLE2_VALUE;
		break;
	default:
		val = vic6560[offset];
		break;
	}
	DBG_LOG (3, "vic6560_port_r", (errorlog, "%.4x:%.2x\n", offset, val));
	return val;
}

static int DOCLIP (rectangle *r1, const rectangle *r2)
{
	if (r1->min_x > r2->max_x)
		return 0;
	if (r1->max_x < r2->min_x)
		return 0;
	if (r1->min_y > r2->max_y)
		return 0;
	if (r1->max_y < r2->min_y)
		return 0;
	if (r1->min_x < r2->min_x)
		r1->min_x = r2->min_x;
	if (r1->max_x > r2->max_x)
		r1->max_x = r2->max_x;
	if (r1->min_y < r2->min_y)
		r1->min_y = r2->min_y;
	if (r1->max_y > r2->max_y)
		r1->max_y = r2->max_y;
	return 1;
}

static void vic6560_draw_character (int ybegin, int yend,
									int ch, int yoff, int xoff,
									UINT16 *color)
{
	int y, code;

	for (y = ybegin; y <= yend; y++)
	{
		code = vic_dma_read ((chargenaddr + ch * charheight + y) & 0x3fff);
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

static void vic6560_draw_character_multi (int ybegin, int yend,
										  int ch, int yoff, int xoff, UINT16 *color)
{
	int y, code;

	for (y = ybegin; y <= yend; y++)
	{
		code = vic_dma_read ((chargenaddr + ch * charheight + y) & 0x3fff);
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


#ifndef GFX
INLINE void vic6560_draw_pointer (mame_bitmap *bitmap,
								  rectangle *visible, int xoff, int yoff)
{
	/* this is a a static graphical object */
	/* should be easy to convert to gfx_element!? */
	static UINT8 blackmask[] =
	{0x00, 0x70, 0x60, 0x50, 0x08, 0x04, 0x00, 0x00};
	static UINT8 whitemask[] =
	{0xf0, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00};
	int i, j, y, x;

	for (y = visible->min_y, j = yoff; y <= visible->max_y; y++, j++)
	{
		for (x = visible->min_x, i = xoff; x <= visible->max_x; x++, i++)
		{
			if ((blackmask[j] << i) & 0x80)
				*BITMAP_ADDR16(bitmap, y, x) = black;
			else if ((whitemask[j] << (i & ~1)) & 0x80)
				*BITMAP_ADDR16(bitmap, y, x) = white;
		}
	}
}
#endif

static void vic6560_drawlines (int first, int last)
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
			ch = vic_dma_read ((videoaddr + offs) & 0x3fff);
			attr = (vic_dma_read_color ((videoaddr + offs) & 0x3fff)) & 0xf;
			if (inverted)
			{
				if (attr & 8)
				{
					multiinverted[0] = Machine->pens[attr & 7];
					vic6560_draw_character_multi (ybegin, yend, ch, yoff, xoff,
												  multiinverted);
				}
				else
				{
					monoinverted[0] = Machine->pens[attr];
					vic6560_draw_character (ybegin, yend, ch, yoff, xoff, monoinverted);
				}
			}
			else
			{
				if (attr & 8)
				{
					multi[2] = Machine->pens[attr & 7];
					vic6560_draw_character_multi (ybegin, yend, ch, yoff, xoff, multi);
				}
				else
				{
					mono[1] = Machine->pens[attr];
					vic6560_draw_character (ybegin, yend, ch, yoff, xoff, mono);
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
	rectangle r;

	rasterline++;
	if (rasterline >= vic656x_lines)
	{
		rasterline = 0;
		vic6560_drawlines (lastline, vic656x_lines);
		lastline = 0;

		if (LIGHTPEN_POINTER)
		{

			r.min_x = LIGHTPEN_X_VALUE - 1 + VIC656X_MAME_XPOS;
			r.max_x = r.min_x + 8 - 1;
			r.min_y = LIGHTPEN_Y_VALUE - 1 + VIC656X_MAME_YPOS;
			r.max_y = r.min_y + 8 - 1;

			if (DOCLIP (&r, &Machine->screen[0].visarea))
			{
#ifndef GFX
				vic6560_draw_pointer (vic6560_bitmap, &r,
									  r.min_x - (LIGHTPEN_X_VALUE + VIC656X_MAME_XPOS - 1),
									  r.min_y - (LIGHTPEN_Y_VALUE + VIC656X_MAME_YPOS - 1));
#else
				drawgfx (vic6560_bitmap, pointerelement, 0, 0, 0, 0,
						 r.min_x - 1, r.min_y - 1, &r, TRANSPARENCY_PEN, 0);
#endif
			}
		}
	}
}

VIDEO_UPDATE( vic6560 )
{
	copybitmap(bitmap, vic6560_bitmap, 0, 0, 0, 0, cliprect, TRANSPARENCY_NONE, 0);
	return 0;
}
