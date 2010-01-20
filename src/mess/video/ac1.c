/***************************************************************************

        AC1 video driver by Miodrag Milanovic

        15/01/2009 Preliminary driver.

****************************************************************************/

#include "emu.h"
#include "includes/ac1.h"

#define AC1_VIDEO_MEMORY		0x1000

const gfx_layout ac1_charlayout =
{
	6, 8,				/* 6x8 characters */
	256,				/* 256 characters */
	1,				    /* 1 bits per pixel */
	{0},				/* no bitplanes; 1 bit per pixel */
	{7, 6, 5, 4, 3, 2},
	{0 * 8, 1 * 8, 2 * 8, 3 * 8, 4 * 8, 5 * 8, 6 * 8, 7 * 8},
	8*8					/* size of one char */
};

VIDEO_START( ac1 )
{
}

VIDEO_UPDATE( ac1 )
{
	int x,y;
	const address_space *space = cputag_get_address_space(screen->machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	for(y = 0; y < 16; y++ )
	{
		for(x = 0; x < 64; x++ )
		{
			int code = memory_read_byte(space, AC1_VIDEO_MEMORY + x + y*64);
			drawgfx_opaque(bitmap, NULL, screen->machine->gfx[0],  code , 0, 0,0, 63*6-x*6,15*8-y*8);
		}
	}
	return 0;
}

VIDEO_UPDATE( ac1_32 )
{
	int x,y;
	const address_space *space = cputag_get_address_space(screen->machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	for(y = 0; y < 32; y++ )
	{
		for(x = 0; x < 64; x++ )
		{
			int code = memory_read_byte(space, AC1_VIDEO_MEMORY + x + y*64);
			drawgfx_opaque(bitmap, NULL, screen->machine->gfx[0],  code , 0, 0,0, 63*6-x*6,31*8-y*8);
		}
	}
	return 0;
}


