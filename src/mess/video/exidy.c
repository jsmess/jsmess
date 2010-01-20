/***************************************************************************

  video/exidy.c

  Functions to emulate the video hardware of the Exidy Sorcerer

***************************************************************************/

#include "emu.h"
#include "includes/exidy.h"


VIDEO_UPDATE( exidy )
{
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=0xf080,x;
	UINT8 *RAM = memory_region(screen->machine, "maincpu");

	for (y = 0; y < 30; y++)
	{
		for (ra = 0; ra < 8; ra++)
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = ma; x < ma+64; x++)
			{
				chr = RAM[x];

				/* get pattern of pixels for that character scanline */
				gfx = RAM[0xf800 | (chr<<3) | ra];

				/* Display a scanline of a character (8 pixels) */
				*p = ( gfx & 0x80 ) ? 1 : 0; p++;
				*p = ( gfx & 0x40 ) ? 1 : 0; p++;
				*p = ( gfx & 0x20 ) ? 1 : 0; p++;
				*p = ( gfx & 0x10 ) ? 1 : 0; p++;
				*p = ( gfx & 0x08 ) ? 1 : 0; p++;
				*p = ( gfx & 0x04 ) ? 1 : 0; p++;
				*p = ( gfx & 0x02 ) ? 1 : 0; p++;
				*p = ( gfx & 0x01 ) ? 1 : 0; p++;
			}
		}
		ma+=64;
	}
	return 0;
}
