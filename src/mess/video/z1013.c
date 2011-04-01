/***************************************************************************

        Z1013 driver by Miodrag Milanovic

        22/04/2009 Preliminary driver.

****************************************************************************/


#include "emu.h"
#include "includes/z1013.h"


VIDEO_START( z1013 )
{
}

SCREEN_UPDATE( z1013 )
{
	z1013_state *state = screen->machine().driver_data<z1013_state>();
	UINT8 code, line;
	int y, x, b,i;
	UINT8 *gfx = screen->machine().region("gfx1")->base();

	for(y = 0; y < 32; y++ )
	{
		for(x = 0; x < 32; x++ )
		{
			code = state->m_video_ram[x + y*32];
			for (i = 0; i < 8; i++)
			{
				line = gfx[code*8 + i];
				for (b = 0; b < 8; b++)
				{
					*BITMAP_ADDR16(bitmap, y*8+i, x*8+b) =  ((line << b) & 0x80) ? 1 : 0;
				}
			}

		}
	}
	return 0;
}

