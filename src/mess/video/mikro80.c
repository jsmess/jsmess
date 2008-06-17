/***************************************************************************

		Mikro-80 video driver by Miodrag Milanovic

		10/03/2008 Preliminary driver.
		     
****************************************************************************/


#include "driver.h"
#include "includes/mikro80.h"
  
UINT8 *mikro80_video_ram;
UINT8 *mikro80_cursor_ram;
    
const gfx_layout mikro80_charlayout =
{
	8, 8,				/* 8x8 characters */
	256,				/* 256 characters */
	1,				  /* 1 bits per pixel */
	{0},				/* no bitplanes; 1 bit per pixel */	
	{0, 1, 2, 3, 4, 5, 6, 7},
	{0 * 8, 1 * 8, 2 * 8, 3 * 8, 4 * 8, 5 * 8, 6 * 8, 7 * 8},
	8*8					/* size of one char */
};

PALETTE_INIT( mikro80 )
{
	palette_set_color(machine,0,RGB_BLACK); /* black */	
	palette_set_color(machine,1,RGB_WHITE); /* white */
	palette_set_color(machine,2,RGB_WHITE); /* white */	
	palette_set_color(machine,3,RGB_BLACK); /* black */
}

VIDEO_START( mikro80 )
{
}
 
VIDEO_UPDATE( mikro80 )
{
	int x,y;

	for(y = 0; y < 32; y++ )
	{
		for(x = 0; x < 64; x++ )
		{
			UINT8 code = mikro80_video_ram [x + y*64 ];
			UINT8 attr = mikro80_cursor_ram[x+1 + y*64 ] & 0x80 ? 1 : 0;
			drawgfx(bitmap, screen->machine->gfx[0],  code, attr, 0,0, x*8,y*8,
				NULL, TRANSPARENCY_NONE, 0);
		}
	}
	return 0;
}
