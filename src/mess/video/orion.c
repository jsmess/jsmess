/***************************************************************************

		Orion video driver by Miodrag Milanovic

		02/04/2008 Preliminary driver.
		     
****************************************************************************/


#include "driver.h"
  

VIDEO_START( orion128 )
{
}
 
extern UINT8 orion128_video_mode;
extern UINT8 orion128_video_page; 

VIDEO_UPDATE( orion128 )
{
	UINT8 code1,code2,color;
	int y, x,b;
		
	int part1addr = (3-orion128_video_page) * 0x4000;
	int part2addr = (3-orion128_video_page) * 0x4000 + 0x10000;			
	for (x = 0; x < 48; x++)
	{			
		for (y = 0; y < 256; y++)
		{
			code1 = mess_ram[part1addr + y + x*256];
			code2 = mess_ram[part2addr + y + x*256];
			color = 0;
			for (b = 7; b >= 0; b--)			
			{								
				switch(orion128_video_mode) {
					case 0 : color = ((code1 >> b) & 0x01) ? 10 : 0; break;
					case 1 : color = ((code1 >> b) & 0x01) ? 17 : 16; break;
					case 4 : 
					case 6 :									 		
					case 7 :
									 color = ((code1 >> b) & 0x01) ? (code2 & 0x0f) : (code2 >> 4); break;						
				}
				*BITMAP_ADDR16(bitmap, y, x*8+(7-b)) = color;
			}
		}				
	}
	
	return 0;
}

const rgb_t orion128_palette[18] = {
	MAKE_RGB(0x00, 0x00, 0x00), // 0
	MAKE_RGB(0x00, 0x00, 0xc0), // 1
	MAKE_RGB(0x00, 0xc0, 0x00), // 2 
	MAKE_RGB(0x00, 0xc0, 0xc0), // 3  
	MAKE_RGB(0xc0, 0x00, 0x00), // 4  
	MAKE_RGB(0xc0, 0x00, 0xc0), // 5  
	MAKE_RGB(0xc0, 0xc0, 0x00), // 6  
	MAKE_RGB(0xc0, 0xc0, 0xc0), // 7  
	MAKE_RGB(0x80, 0x80, 0x80), // 8  
	MAKE_RGB(0x00, 0x00, 0xff), // 9  
	MAKE_RGB(0x00, 0xff, 0x00), // A  
	MAKE_RGB(0x00, 0xff, 0xff), // B  
	MAKE_RGB(0xff, 0x00, 0x00), // C  
	MAKE_RGB(0xff, 0x00, 0xff), // D  
	MAKE_RGB(0xff, 0xff, 0x00), // E  
	MAKE_RGB(0xff, 0xff, 0xff),	// F  
	MAKE_RGB(0xc8, 0xb4, 0x28),	// 10 
	MAKE_RGB(0x32, 0xfa, 0xfa)	// 11  
};

PALETTE_INIT( orion128 )
{
	palette_set_colors(machine, 0, orion128_palette, ARRAY_LENGTH(orion128_palette));
}

