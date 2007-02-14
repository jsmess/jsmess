/***************************************************************************

  Attack ufo video emulation
  based on MOS 6560 emulator by
  PeT mess@utanet.at

  Differences between 6560 and Attack Ufo chip:
  - no invert mode
  - no multicolor
  - 16 col chars

***************************************************************************/

#include "driver.h"
#include "sound/custom.h"
#include "includes/attckufo.h"

#define MAX_LINES 261

unsigned char attckufo_palette[] =
{
/* ripped from vice, a very excellent emulator */
	0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0xf0, 0xf0,
	0x60, 0x00, 0x60, 0x00, 0xa0, 0x00, 0x00, 0x00, 0xf0, 0xd0, 0xd0, 0x00,
	0xc0, 0xa0, 0x00, 0xff, 0xa0, 0x00, 0xf0, 0x80, 0x80, 0x00, 0xff, 0xff,
	0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0x00, 0xa0, 0xff, 0xff, 0xff, 0x00
};



UINT8 attckufo_regs[16];

#define CHARS_X ((int)attckufo_regs[2]&0x7f)
#define CHARS_Y (((int)attckufo_regs[3]&0x7e)>>1)


#define XSIZE (CHARS_X*8)
#define YSIZE (CHARS_Y*8)

#define CHARGENADDR (((int)attckufo_regs[5]&0xf)<<10)
#define VIDEOADDR ( ( ((int)attckufo_regs[5]&0xf0)<<(10-4))\
		    | ( ((int)attckufo_regs[2]&0x80)<<(9-7)) )
#define VIDEORAMSIZE (YSIZE*XSIZE)
#define CHARGENSIZE (256*HEIGHTPIXEL)

static int rasterline = 0, lastline = 0;
static void attckufo_drawlines (int start, int last);



static int chars_x, chars_y;
static int xsize, ysize ;
static int chargenaddr, videoaddr;

static UINT16 mono[2];

VIDEO_START( attckufo )
{
	mono[0]=0;
	return video_start_generic_bitmapped(machine);
}


WRITE8_HANDLER ( attckufo_port_w )
{
	switch (offset)
	{
	case 0xa:
	case 0xb:
	case 0xc:
	case 0xd:
	case 0xe:
		attckufo_soundport_w (offset, data);
		break;
	}
	if (attckufo_regs[offset] != data)
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
			attckufo_drawlines (lastline, rasterline);
			break;
		}
		attckufo_regs[offset] = data;
		switch (offset)
		{

		case 2:
			/* ntsc values >= 31 behave like 31 */
			chars_x = CHARS_X;
			videoaddr = VIDEOADDR;
			xsize = XSIZE;
			break;
		case 3:
			chars_y = CHARS_Y;
			ysize = YSIZE;
			break;
		case 5:
			chargenaddr = CHARGENADDR;
			videoaddr = VIDEOADDR;
			break;
		}
	}
}

 READ8_HANDLER ( attckufo_port_r )
{
	int val;

	switch (offset)
	{
	case 3:
		val = ((rasterline & 1) << 7) | (attckufo_regs[offset] & 0x7f);
		break;
	case 4:						   /*rasterline */
		attckufo_drawlines (lastline, rasterline);
		val = (rasterline / 2) & 0xff;
		break;
	default:
		val = attckufo_regs[offset];
		break;
	}
	return val;
}

static void attckufo_draw_character (int ybegin, int yend,
									int ch, int yoff, int xoff,
									UINT16 *color)
{
	int y, code;

	{
		for (y = ybegin; y <= yend; y++)
		{
			code = attckufo_dma_read ((chargenaddr + ch * 8 + y) );
			*BITMAP_ADDR16(tmpbitmap, y + yoff, xoff + 0) = color[code >> 7];
			*BITMAP_ADDR16(tmpbitmap, y + yoff, xoff + 1) = color[(code >> 6) & 1];
			*BITMAP_ADDR16(tmpbitmap, y + yoff, xoff + 2) = color[(code >> 5) & 1];
			*BITMAP_ADDR16(tmpbitmap, y + yoff, xoff + 3) = color[(code >> 4) & 1];
			*BITMAP_ADDR16(tmpbitmap, y + yoff, xoff + 4) = color[(code >> 3) & 1];
			*BITMAP_ADDR16(tmpbitmap, y + yoff, xoff + 5) = color[(code >> 2) & 1];
			*BITMAP_ADDR16(tmpbitmap, y + yoff, xoff + 6) = color[(code >> 1) & 1];
			*BITMAP_ADDR16(tmpbitmap, y + yoff, xoff + 7) = color[code & 1];
		}
	}
}

static void attckufo_drawlines (int first, int last)
{
	int line, vline;
	int offs, yoff, xoff, ybegin, yend;
	int attr, ch;

	lastline = last;
	if (first >= last)
		return;


	line=0;
	for (vline = line ; (line < last) && (line <  ysize);)
	{

		{
			offs = (vline >> 3) * chars_x;
			yoff = (vline & ~7) ;
			ybegin = vline & 7;
			yend = (vline + 7 < last ) ? 7 : ((last - line) & 7) + ybegin;
		}

		for (xoff = 0; (xoff <  xsize) && (xoff < 22*8); xoff += 8, offs++)
		{
			ch = attckufo_dma_read ((videoaddr + offs) & 0x3fff);
			attr = (attckufo_dma_read_color ((videoaddr + offs) & 0x3fff)) & 0xf;
			mono[1] = Machine->pens[attr];
			attckufo_draw_character (ybegin, yend, ch, yoff, xoff, mono);
		}

		vline = (vline + 8) & ~7;
		line = vline ;
	}
}

INTERRUPT_GEN( attckufo_raster_interrupt )
{
	rasterline++;
	if (rasterline >= MAX_LINES)
	{
		rasterline = 0;
		attckufo_drawlines (lastline, MAX_LINES);
		lastline = 0;
	}
}
