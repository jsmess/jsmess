/***************************************************************************

  jupiter.c

  Functions to emulate the video hardware of the Jupiter Ace.

***************************************************************************/

#include "driver.h"
#include "deprecat.h"
#include "includes/jupiter.h"
#include "mslegacy.h"


UINT8 *jupiter_charram;
size_t jupiter_charram_size;


VIDEO_START( jupiter )
{
	VIDEO_START_CALL(generic);
}


WRITE8_HANDLER( jupiter_vh_charram_w )
{
    if(data == jupiter_charram[offset])
		return; /* no change */

    jupiter_charram[offset] = data;

    /* decode character graphics again */
	decodechar(Machine->gfx[0], offset / 8, jupiter_charram);
}


VIDEO_UPDATE( jupiter )
{
	int offs;

    for(offs = 0; offs < videoram_size; offs++)
	{
        int code = videoram[offs];
		int sx, sy;

		sy = (offs / 32) * 8;
		sx = (offs % 32) * 8;

		drawgfx(bitmap, machine->gfx[0], code & 0x7f, (code & 0x80) ? 1 : 0, 0,0, sx,sy,
			&machine->screen[0].visarea, TRANSPARENCY_NONE, 0);
	}

	return 0;
}
