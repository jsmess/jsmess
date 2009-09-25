/***************************************************************************

		Bashkiria-2M video driver by Miodrag Milanovic

		28/03/2008 Preliminary driver.
		     
****************************************************************************/


#include "driver.h"
#include "includes/b2m.h"
  
VIDEO_START( b2m )
{
}
 
VIDEO_UPDATE( b2m )
{
 	UINT8 code1;
 	UINT8 code2;
 	UINT8 col;
	int y, x, b;
	b2m_state *state = screen->machine->driver_data;
		
	for (x = 0; x < 48; x++)
	{			
		for (y = 0; y < 256; y++)
		{
			if (state->b2m_video_page==0) {
				code1 = mess_ram[0x11000 + x*256 + ((y + state->b2m_video_scroll) & 0xff)];
				code2 = mess_ram[0x15000 + x*256 + ((y + state->b2m_video_scroll) & 0xff)];
			} else {
				code1 = mess_ram[0x19000 + x*256 + ((y + state->b2m_video_scroll) & 0xff)];
				code2 = mess_ram[0x1d000 + x*256 + ((y + state->b2m_video_scroll) & 0xff)];
			}			
			for (b = 7; b >= 0; b--)
			{								
				col = (((code2 >> b) & 0x01)<<1) + ((code1 >> b) & 0x01);
				*BITMAP_ADDR16(bitmap, y, x*8+b) =  col;
			}
		}				
	}
	
	return 0;
}

static const rgb_t b2m_palette[4] = {
	MAKE_RGB(0x00, 0x00, 0x00), // 0
	MAKE_RGB(0x00, 0x00, 0x00), // 1
	MAKE_RGB(0x00, 0x00, 0x00), // 2 
	MAKE_RGB(0x00, 0x00, 0x00), // 3  
};

PALETTE_INIT( b2m )
{
	palette_set_colors(machine, 0, b2m_palette, ARRAY_LENGTH(b2m_palette));
}
