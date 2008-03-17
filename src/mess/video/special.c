/***************************************************************************

		Specialist video driver by Miodrag Milanovic

		15/03/2008 Preliminary driver.
		     
****************************************************************************/


#include "driver.h"
  
#define SPECIALIST_VIDEO_MEMORY		0x9000  
    

VIDEO_START( special )
{
}
 
VIDEO_UPDATE( special )
{
  UINT8 code;
	int y, x, b;
	
	for (x = 0; x < 48; x++)
	{			
		for (y = 0; y < 256; y++)
		{
			code = program_read_byte(SPECIALIST_VIDEO_MEMORY + y + x*256);
			for (b = 7; b >= 0; b--)
			{								
				*BITMAP_ADDR16(bitmap, y, x*8+(7-b)) =  (code >> b) & 0x01;
			}
		}				
	}
	return 0;
}

