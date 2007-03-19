/***************************************************************************

  nascom1.c

  Functions to emulate the video hardware of the nascom1.

***************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "includes/nascom1.h"

VIDEO_UPDATE( nascom1 )
{
	int	sy, sx;

	for (sx = 0; sx < 48; sx++)
	{
		drawgfx (bitmap, Machine->gfx[0], videoram[0x03ca + sx],
			1, 0, 0, sx * 8, 0, &Machine->screen[0].visarea,
			TRANSPARENCY_NONE, 0);
	}

	for (sy = 0; sy < 15; sy++)
	{
		for (sx = 0; sx < 48; sx++)
		{
			drawgfx (bitmap, Machine->gfx[0], videoram[0x000a + (sy * 64) + sx],
				1, 0, 0, sx * 8, (sy + 1) * 16, &Machine->screen[0].visarea,
				TRANSPARENCY_NONE, 0);
		}
	}
	return 0;
}

VIDEO_UPDATE( nascom2 )
{
	int	sy, sx;

	for (sx = 0; sx < 48; sx++)
	{
		drawgfx (bitmap, Machine->gfx[0], videoram[0x03ca + sx],
			1, 0, 0, sx * 8, 0, &Machine->screen[0].visarea,
			TRANSPARENCY_NONE, 0);
	}

	for (sy = 0; sy < 15; sy++)
	{
		for (sx = 0; sx < 48; sx++)
		{
			drawgfx (bitmap, Machine->gfx[0], videoram[0x000a + (sy * 64) + sx],
				1, 0, 0, sx * 8, (sy + 1) * 14, &Machine->screen[0].visarea,
				TRANSPARENCY_NONE, 0);
		}
	}
	return 0;
}

