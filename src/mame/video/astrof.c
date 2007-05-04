/***************************************************************************

  video.c

  Functions to emulate the video hardware of Astro Fighter hardware games.

  Lee Taylor 28/11/1997

***************************************************************************/

#include "driver.h"
#include "includes/astrof.h"


UINT8 *tomahawk_protection;
UINT8 *astrof_color;

static UINT8 palette_bank, red_on;


/***************************************************************************

  The palette PROMs are connected to the RGB output this way:

  bit 0 -- RED
        -- RED
        -- GREEN
        -- GREEN
        -- BLUE
  bit 5 -- BLUE

  I couldn't really determine the resistances, (too many resistors and capacitors)
  so the value below might be off a tad. But since there is also a variable
  resistor for each color gun, this is one of the concievable settings

***************************************************************************/

static void get_pens(pen_t *pens)
{
	offs_t i;

	for (i = 0; i < memory_region_length(REGION_PROMS); i++)
	{
		UINT8 bit0, bit1, r, g, b;

		UINT8 data = memory_region(REGION_PROMS)[i];

		bit0 = ((data >> 0) & 0x01) | red_on;
		bit1 = ((data >> 1) & 0x01) | red_on;
		r = 0xc0 * bit0 + 0x3f * bit1;

		bit0 = ( data >> 2) & 0x01;
		bit1 = ( data >> 3) & 0x01;
		g = 0xc0 * bit0 + 0x3f * bit1;

		bit0 = ( data >> 4) & 0x01;
		bit1 = ( data >> 5) & 0x01;
		b = 0xc0 * bit0 + 0x3f * bit1;

		pens[i] = MAKE_RGB(r, g, b);
	}
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( astrof )
{
	colorram = auto_malloc(videoram_size);

	palette_bank = 0;
	red_on = 0;

	return 0;
}



WRITE8_HANDLER( astrof_videoram_w )
{
	// Astro Fighter's palette is set in astrof_video_control2_w, D0 is unused
	videoram[offset] = data;
	colorram[offset] = *astrof_color & 0x0e;
}


WRITE8_HANDLER( tomahawk_videoram_w )
{
	// Tomahawk's palette is set per byte
	videoram[offset] = data;
	colorram[offset] = (*astrof_color & 0x0e) | ((*astrof_color & 0x01) << 4);
}


WRITE8_HANDLER( astrof_video_control1_w )
{
	// Video control register 1
	//
	// Bit 0     = Flip screen
	// Bit 1     = Shown in schematics as what appears to be a screen clear
	//             bit, but it's always zero in Astro Fighter
	// Bit 2     = Not hooked up in the schematics, but at one point the game
	//             sets it to 1.
	// Bit 3-7   = Not hooked up

	if (input_port_2_r(0) & 0x02) /* Cocktail mode */
	{
		flip_screen_set(data & 0x01);
	}
}


// Video control register 2
//
// Bit 0     = Hooked up to a connector called OUT0, don't know what it does
// Bit 1     = Hooked up to a connector called OUT1, don't know what it does
// Bit 2     = Palette select in Astro Fighter, unused in Tomahawk
// Bit 3     = Turns on RED color gun regardless of what the value is
//             in the color PROM
// Bit 4-7   = Not hooked up

WRITE8_HANDLER( astrof_video_control2_w )
{
	palette_bank = (data >> 2) & 0x01;

	red_on = (data >> 3) & 0x01;
}


WRITE8_HANDLER( tomahawk_video_control2_w )
{
	palette_bank = 0;

	red_on = (data >> 3) & 0x01;

	video_screen_update_partial(0, video_screen_get_vpos(0));
}


READ8_HANDLER( tomahawk_protection_r )
{
	/* flip the byte */

	return BITSWAP8(*tomahawk_protection,0,1,2,3,4,5,6,7);
}


VIDEO_UPDATE( astrof )
{
	pen_t pens[0x100];
	offs_t offs;

	get_pens(pens);

	for (offs = 0; offs < videoram_size; offs++)
	{
		int i;

		UINT8 data = videoram[offs];
		UINT8 color = colorram[offs];

		pen_t back_pen = pens[(palette_bank << 4) | color | 0x00];
		pen_t fore_pen = pens[(palette_bank << 4) | color | 0x01];

		UINT8 y = ~offs;
		UINT8 x = offs >> 8 << 3;

		for (i = 0; i < 8; i++)
		{
			pen_t pen = (data & 0x01) ? fore_pen : back_pen;

			if (flip_screen)
				*BITMAP_ADDR32(bitmap, 255 - y, 255 - x) = pen;
			else
				*BITMAP_ADDR32(bitmap, y, x) = pen;

			x = x + 1;
			data = data >> 1;
		}
	}

	return 0;
}
