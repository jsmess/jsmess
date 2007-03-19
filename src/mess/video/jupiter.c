/***************************************************************************

  jupiter.c

  Functions to emulate the video hardware of the Jupiter Ace.

***************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "includes/jupiter.h"

unsigned char *jupiter_charram;
size_t jupiter_charram_size;

VIDEO_START( jupiter )
{
	if( video_start_generic(machine) )
		return 1;
    return 0;
}

WRITE8_HANDLER( jupiter_vh_charram_w )
{
	int chr = offset / 8, offs;

    if( data == jupiter_charram[offset] )
		return; /* no change */

    jupiter_charram[offset] = data;

    /* decode character graphics again */
	decodechar(Machine->gfx[0], chr, jupiter_charram, &jupiter_charlayout);

    /* mark all visible characters with that code dirty */
    for( offs = 0; offs < videoram_size; offs++ )
	{
		if( videoram[offs] == chr )
			dirtybuffer[offs] = 1;
	}
}

VIDEO_UPDATE( jupiter )
{
	int offs;
	int full_refresh = 1;

	/* do we need a full refresh? */
    if( full_refresh )
		memset(dirtybuffer, 1, videoram_size);

    for( offs = 0; offs < videoram_size; offs++ )
	{
        if( dirtybuffer[offs]  )
		{
            int code = videoram[offs];
			int sx, sy;

			sy = (offs / 32) * 8;
			sx = (offs % 32) * 8;

			drawgfx(bitmap, Machine->gfx[0], code & 0x7f, (code & 0x80) ? 1 : 0, 0,0, sx,sy,
				&Machine->screen[0].visarea, TRANSPARENCY_NONE, 0);

            dirtybuffer[offs] = 0;
		}
	}
	return 0;
}
