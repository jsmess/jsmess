/***************************************************************************

		Orao video driver by Miodrag Milanovic

		01/03/2008 Updated to work with latest SVN code
		22/02/2008 Preliminary driver.
		     
****************************************************************************/

#include "driver.h"

#define ORAO_VIDEO_MEMORY		0x6000
  
 
const rgb_t orao_palette[2] =
{
	MAKE_RGB(0x00, 0x00, 0x00),		/* Black */
	MAKE_RGB(0xff, 0xff, 0xff)		/* White */
};

PALETTE_INIT( orao )
{
	palette_set_colors(machine, 0, orao_palette, ARRAY_LENGTH(orao_palette));
}

VIDEO_START( orao )
{
}
 
VIDEO_UPDATE( orao )
{
	UINT8 code;
	int y, x, b;
	
	int addr = ORAO_VIDEO_MEMORY;	
	for (y = 0; y < 256; y++)
	{
		int horpos = 0;
		for (x = 0; x < 32; x++)
		{			
			code = program_read_byte(addr++);
			for (b = 0; b < 8; b++)
			{				
				*BITMAP_ADDR16(bitmap, y, horpos++) =  (code >> b) & 0x01;
			}
		}
	}
	return 0;
}


