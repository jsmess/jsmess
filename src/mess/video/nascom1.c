/***************************************************************************

  nascom1.c

  Functions to emulate the video hardware of the nascom1.

***************************************************************************/

#include "emu.h"
#include "includes/nascom1.h"

VIDEO_UPDATE( nascom1 )
{
	UINT8 *videoram = screen->machine->generic.videoram.u8;
	int	sy, sx;

	for (sx = 0; sx < 48; sx++)
	{
		drawgfx_opaque (bitmap, NULL,
			screen->machine->gfx[0], videoram[0x03ca + sx],
			1, 0, 0, sx * 8, 0);
	}

	for (sy = 0; sy < 15; sy++)
	{
		for (sx = 0; sx < 48; sx++)
		{
			drawgfx_opaque (bitmap, NULL,
				screen->machine->gfx[0], videoram[0x000a + (sy * 64) + sx],
				1, 0, 0, sx * 8, (sy + 1) * 16);
		}
	}
	return 0;
}

VIDEO_UPDATE( nascom2 )
{
	UINT8 *videoram = screen->machine->generic.videoram.u8;
	int	sy, sx;

	for (sx = 0; sx < 48; sx++)
	{
		drawgfx_opaque (bitmap, NULL,
			screen->machine->gfx[0], videoram[0x03ca + sx],
			1, 0, 0, sx * 8, 0);
	}

	for (sy = 0; sy < 15; sy++)
	{
		for (sx = 0; sx < 48; sx++)
		{
			drawgfx_opaque (bitmap, NULL,
				screen->machine->gfx[0], videoram[0x000a + (sy * 64) + sx],
				1, 0, 0, sx * 8, (sy + 1) * 14);
		}
	}
	return 0;
}

