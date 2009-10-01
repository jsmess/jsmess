/***************************************************************************

        Homelab video driver by Miodrag Milanovic

        31/08/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "includes/homelab.h"


VIDEO_START( homelab )
{
}

VIDEO_UPDATE( homelab )
{
	int x,y,j,b;
	UINT8 *gfx = memory_region(screen->machine, "gfx1");
	const address_space *space = cputag_get_address_space(screen->machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	for(y = 0; y < 25; y++ )
	{
		for(x = 0; x < 40; x++ )
		{
			int code = memory_read_byte(space, 0xc000 + x + y*40);
			for(j = 0; j < 8; j++ )
			{
				for(b = 0; b < 8; b++ )
			  {
			  	*BITMAP_ADDR16(bitmap, y * 8 + j, x * 8 + b ) = (gfx[code + j * 256] >> (7 - b)) & 1;
			  }
			}
		}
	}
	return 0;
}

VIDEO_UPDATE( homelab3 )
{
	int x,y,j,b;
	UINT8 *gfx = memory_region(screen->machine, "gfx1");
	const address_space *space = cputag_get_address_space(screen->machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	for(y = 0; y < 25; y++ )
	{
		for(x = 0; x < 80; x++ )
		{
			int code = memory_read_byte(space, 0xf000 + x + y * 80);
			for(j = 0; j < 8; j++ )
			{
				for(b = 0; b < 8; b++ )
			  {
			  	*BITMAP_ADDR16(bitmap, y * 8 + j, x * 8 + b ) = (gfx[code + j * 256] >> (7 - b)) & 1;
			  }
			}
		}
	}
	return 0;
}

