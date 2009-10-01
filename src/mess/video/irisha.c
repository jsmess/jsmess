/***************************************************************************

        Irisha video driver by Miodrag Milanovic

        27/03/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "includes/irisha.h"


VIDEO_START( irisha )
{
}

VIDEO_UPDATE( irisha )
{
 	UINT8 code1, code2;
 	UINT8 col;
	int y, x, b;
	const address_space *space = cputag_get_address_space(screen->machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	// draw image
	for (y = 0; y < 200; y++)
	{
		for (x = 0; x < 40; x++)
		{
			code1 = memory_read_byte(space, 0xe000 + x + y * 40);
			code2 = memory_read_byte(space, 0xc000 + x + y * 40);
			for (b = 0; b < 8; b++)
			{
				col = ((code1 >> b) & 0x01);
				*BITMAP_ADDR16(bitmap, y, x * 8 + (7 - b)) =  col;
			}
		}
	}


	return 0;
}

