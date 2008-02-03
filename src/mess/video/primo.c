/***************************************************************************

  primo.c

  Functions to emulate the video hardware of Primo.

  Krzysztof Strzecha

***************************************************************************/

#include "driver.h"
#include "deprecat.h"
#include "includes/primo.h"

UINT16 primo_video_memory_base;


static void primo_draw_scanline(mame_bitmap *bitmap, int primo_scanline)
{
	int x, i;
	UINT8 data;

	/* set up scanline */
	UINT16 *scanline = BITMAP_ADDR16(bitmap, primo_scanline, 0);

	/* address of current line in Primo video memory */
	const UINT8* primo_video_ram_line = memory_get_read_ptr(0, ADDRESS_SPACE_PROGRAM, primo_video_memory_base + 32*primo_scanline);

	for (x=0; x<256; x+=8)
	{
		data = primo_video_ram_line[x/8];

		for (i=0; i<8; i++)
			scanline[x+i]=Machine->pens[(data & (0x80>>i)) ? 1 : 0];

	}
}


VIDEO_UPDATE( primo )
{
	int primo_scanline;

	for (primo_scanline=0; primo_scanline<192; primo_scanline++)
		primo_draw_scanline(bitmap, primo_scanline);
	return 0;
}
