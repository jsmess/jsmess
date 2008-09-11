/***************************************************************************

        SAPI-1 driver by Miodrag Milanovic

        09/09/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "includes/sapi1.h"

VIDEO_START( sapi1 )
{
}

VIDEO_UPDATE( sapi1 )
{
 	int x,y,j,b;
	UINT8 *gfx = memory_region(screen->machine, "gfx1");

	for(y = 0; y < 32; y++ )
	{
		for(x = 0; x < 64; x++ )
		{
			int code = program_read_byte(0x3800+ x + y*64);
			for(j = 0; j < 12; j++ )
			{
				for(b = 0; b < 6; b++ )
			  {
			  	*BITMAP_ADDR16(bitmap, y*12+j, x*6+b ) = (gfx[code*16+(11-j)] >> (7-b)) & 1;
			  }
			}
		}
	}
	return 0;
}

