/***************************************************************************

        LLC driver by Miodrag Milanovic

        17/04/2009 Preliminary driver.

****************************************************************************/


#include "emu.h"
#include "includes/llc.h"


VIDEO_START( llc1 )
{
}

SCREEN_UPDATE( llc1 )
{
	llc_state *state = screen->machine().driver_data<llc_state>();
	UINT8 code,disp;
	int y, x, b,c,inv;
	UINT8 *gfx = screen->machine().region("gfx1")->base();

	for (x = 0; x < 64; x++)
	{
		for (y = 0; y < 16; y++)
		{
			code = state->m_video_ram[x + y*64];
			inv = code & 0x80; // highest bit is invert flag
			code &= 0x7f;
			for (b = 0; b < 8; b++)
			{
				for (c = 0; c < 8; c++)
				{
					disp = gfx[code + b*0x80];
					if (inv==0x80) disp = disp ^ 0xff;
					*BITMAP_ADDR16(bitmap, y*8+b, x*8+c) =  ((disp<< c) & 0x80) ? 1 : 0;
				}
			}
		}
	}
	return 0;
}

VIDEO_START( llc2 )
{
}

SCREEN_UPDATE( llc2 )
{
	llc_state *state = screen->machine().driver_data<llc_state>();
	UINT8 code,disp;
	int y, x, b,c;
	UINT8 *gfx = screen->machine().region("gfx1")->base();

	for (x = 0; x < 64; x++)
	{
		for (y = 0; y < 32; y++)
		{
			code = state->m_video_ram[x + y*64];
			for (b = 0; b < 8; b++)
			{
				disp = gfx[code * 8 + b];
				for (c = 0; c < 8; c++)
				{
					*BITMAP_ADDR16(bitmap, y*8+b, x*8+c) =  ((disp<< c) & 0x80) ? 1 : 0;
				}
			}
		}
	}
	return 0;
}
