
#include "driver.h"


/***********************************************************

	Video

************************************************************/

PALETTE_INIT( kaypro )
{
	palette_set_color(machine,0,RGB_BLACK); /* black */
	palette_set_color(machine,1,MAKE_RGB(0, 220, 0)); /* green */	
}

VIDEO_UPDATE( kayproii )
{
/* The display consists of 80 columns and 24 rows. Each row is allocated 128 bytes of ram,
	but only the first 80 are used. The total video ram therefore is 0x0c00 bytes.
	There is one video attribute: bit 7 causes blinking. The first half of the
	character generator is blank, with the visible characters in the 2nd half.
	During the "off" period of blanking, the first half is used. Only 5 pixels are
	connected from the rom to the shift register, the remaining pixels are held high.
	A high pixel is black and a low pixel is green. */

	static UINT8 framecnt=0;
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=0,x;
	UINT8 *FNT = memory_region(screen->machine, "gfx1");

	framecnt++;

	for (y = 0; y < 24; y++)
	{
		for (ra = 0; ra < 10; ra++)
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = ma; x < ma + 80; x++)
			{
				if (ra < 8)
				{
					chr = videoram[x]^0x80;

					/* Take care of flashing characters */
					if ((chr < 0x80) && (framecnt & 0x08))
						chr |= 0x80;

					/* get pattern of pixels for that character scanline */
					gfx = FNT[(chr<<3) | ra ];
				}
				else
					gfx = 0xff;

				/* Display a scanline of a character (7 pixels) */
				*p = 0; p++;
				*p = ( gfx & 0x10 ) ? 0 : 1; p++;
				*p = ( gfx & 0x08 ) ? 0 : 1; p++;
				*p = ( gfx & 0x04 ) ? 0 : 1; p++;
				*p = ( gfx & 0x02 ) ? 0 : 1; p++;
				*p = ( gfx & 0x01 ) ? 0 : 1; p++;
				*p = 0; p++;
			}
		}
		ma+=128;
	}
	return 0;
}

VIDEO_UPDATE( omni2 )
{
	static UINT8 framecnt=0;
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=0,x;
	UINT8 *FNT = memory_region(screen->machine, "gfx1");

	framecnt++;

	for (y = 0; y < 24; y++)
	{
		for (ra = 0; ra < 10; ra++)
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = ma; x < ma + 80; x++)
			{
				if (ra < 8)
				{
					chr = videoram[x];

					/* Take care of flashing characters */
					if ((chr > 0x7f) && (framecnt & 0x08))
						chr |= 0x80;

					/* get pattern of pixels for that character scanline */
					gfx = FNT[(chr<<3) | ra ];
				}
				else
					gfx = 0xff;

				/* Display a scanline of a character (7 pixels) */
				*p = ( gfx & 0x40 ) ? 0 : 1; p++;
				*p = ( gfx & 0x20 ) ? 0 : 1; p++;
				*p = ( gfx & 0x10 ) ? 0 : 1; p++;
				*p = ( gfx & 0x08 ) ? 0 : 1; p++;
				*p = ( gfx & 0x04 ) ? 0 : 1; p++;
				*p = ( gfx & 0x02 ) ? 0 : 1; p++;
				*p = ( gfx & 0x01 ) ? 0 : 1; p++;
			}
		}
		ma+=128;
	}
	return 0;
}
#if 0
const mc6845_interface kaypro2x_crtc = {
	"screen",			/* name of screen */
	7,				/* number of dots per character */
	NULL,
	kaypro2x_update_row,		/* handler to display a scanline */
	NULL,
	NULL,
	NULL,
	NULL
};
#endif
READ8_HANDLER( kaypro_videoram_r )
{
	return videoram[offset];
}

WRITE8_HANDLER( kaypro_videoram_w )
{
	videoram[offset] = data;
}
