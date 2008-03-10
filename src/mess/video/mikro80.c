/***************************************************************************

		Mikro-80 video driver by Miodrag Milanovic

		10/03/2008 Preliminary driver.
		     
****************************************************************************/


#include "driver.h"
  
#define MIKRO80_VIDEO_MEMORY		0xE800  
#define MIKRO80_CURSOR_MEMORY		0xE000  
    
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
			int code = program_read_byte(MIKRO80_VIDEO_MEMORY + x + y*64);		
			drawgfx(bitmap, machine->gfx[0],  code , 0, 0,0, x*8,y*8,
				NULL, TRANSPARENCY_NONE, 0);
		}
	}

	for(y = 0; y < 32; y++ )
	{
		for(x = 0; x < 64; x++ )
		{
			int code = program_read_byte(MIKRO80_CURSOR_MEMORY + x + y*64);		
			if (code == 0x80 ) {
				drawgfx(bitmap, machine->gfx[0],  0xff , 0, 0,0, x*8,y*8,
					NULL, TRANSPARENCY_NONE, 0);
			}
		}
	}
	
	return 0;
}
