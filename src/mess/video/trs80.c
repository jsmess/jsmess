/***************************************************************************

  trs80.c

  Functions to emulate the video hardware of the TRS80.

***************************************************************************/
#include "driver.h"
#include "includes/trs80.h"

#define FW  TRS80_FONT_W
#define FH  TRS80_FONT_H

/* Bit assignment for "trs80_mode"
	d7 Page select
	d3 Invert characters with bit 7 set (1=invert)
	d2 80/40 or 64/32 characters per line (1=80)
	d0 80/64 or 40/32 characters per line (1=32) */

static UINT16 start_address=0;
static UINT8 crtc_reg;

WRITE8_HANDLER( trs80m4_88_w )
{
/* This is for the programming of the CRTC registers.
	However this CRTC is mask-programmed, and only the
	start address register can be used. The cursor and
	light-pen facilities are ignored. The character clock
	is changed depending on the screen size chosen.
	It is assumed that character size of 6x12 is used in
	all screen sizes, however decapping is needed to be
	certain. Therefore it is easier to use normal
	coding rather than the mc6845 device. */

	if (!offset) crtc_reg = data & 0x1f;

	if (offset) switch (crtc_reg)
	{
		case 12:
			start_address = (start_address & 0x00ff) | (data << 8);
			break;
		case 13:
			start_address = (start_address & 0xff00) | data;
	}
}

/***************************************************************************
  Statics for this module
***************************************************************************/
static int width_store = 10;  // bodgy value to force an initial resize

VIDEO_UPDATE( trs80 )
{
	int width = 64 - ((trs80_mode & 1) << 5);
	int skip = 3 - (width >> 5);
	int i=0,x,y,chr;
	int adj=input_port_read(screen->machine, "CONFIG")&0x40;

	if (width != width_store)
	{
		width_store = width;
		video_screen_set_visarea(screen, 0, width*FW-1, 0, 16*FH-1);
	}

	for (y = 0; y < 16; y++)
	{
		for (x = 0; x < 64; x+=skip)
		{
			i = (y << 6) + x;
			chr=videoram[i];
			if ((adj) && (chr < 32)) chr+=64;			// 7-bit video handler
			drawgfx( bitmap,screen->machine->gfx[0],chr,0,0,0,x/skip*FW,y*FH,
				NULL,TRANSPARENCY_NONE,0);
		}
	}
	return 0;
}

/***************************************************************************
  Write to video ram
***************************************************************************/

READ8_HANDLER( trs80_videoram_r )
{
	if (trs80_mode & 0x80) offset |= 0x400;
	return videoram[offset];
}

WRITE8_HANDLER( trs80_videoram_w )
{
	if (trs80_mode & 0x80) offset |= 0x400;
	videoram[offset] = data;
}

