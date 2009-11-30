/**********************************************************************

    p2000m.c

    Functions to emulate video hardware of the p2000m

**********************************************************************/

#include "driver.h"
#include "includes/p2000t.h"


static INT8 frame_count;


VIDEO_START( p2000m )
{
	frame_count = 0;
}


VIDEO_UPDATE( p2000m )
{
	int offs, sx, sy, code, loop;

	for (offs = 0; offs < 80 * 24; offs++)
	{
		sy = (offs / 80) * 10;
		sx = (offs % 80) * 6;

		if ((frame_count > 25) && (screen->machine->generic.videoram.u8[offs + 2048] & 0x40))
			code = 32;
		else
		{
			code = screen->machine->generic.videoram.u8[offs];
			if ((screen->machine->generic.videoram.u8[offs + 2048] & 0x01) && (code & 0x20))
			{
				code += (code & 0x40) ? 64 : 96;
			} else {
				code &= 0x7f;
			}
			if (code < 32) code = 32;
		}

		drawgfx_opaque (bitmap, NULL, screen->machine->gfx[0], code,
			screen->machine->generic.videoram.u8[offs + 2048] & 0x08 ? 0 : 1, 0, 0, sx, sy);

		if (screen->machine->generic.videoram.u8[offs] & 0x80)
		{
			for (loop = 0; loop < 6; loop++)
			{
				*BITMAP_ADDR16(bitmap, sy + 9, sx + loop) = 0;	/* cursor */
			}
		}
	}

	return 0;
}
