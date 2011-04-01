/***************************************************************************

        MZ80 driver by Miodrag Milanovic

        22/11/2008 Preliminary driver.

****************************************************************************/

#include "emu.h"
#include "machine/i8255a.h"
#include "machine/pit8253.h"
#include "includes/mz80.h"

const gfx_layout mz80k_charlayout =
{
	8, 8,				/* 8x8 characters */
	256,				/* 256 characters */
	1,				  /* 1 bits per pixel */
	{0},				/* no bitplanes; 1 bit per pixel */
	{0, 1, 2, 3, 4, 5, 6, 7},
	{0 * 8, 1 * 8, 2 * 8, 3 * 8, 4 * 8, 5 * 8, 6 * 8, 7 * 8},
	8*8					/* size of one char */
};

const gfx_layout mz80kj_charlayout =
{
	8, 8,				/* 8x8 characters */
	256,				/* 256 characters */
	1,				  /* 1 bits per pixel */
	{0},				/* no bitplanes; 1 bit per pixel */
	{7, 6, 5, 4, 3, 2, 1, 0},
	{0 * 8, 1 * 8, 2 * 8, 3 * 8, 4 * 8, 5 * 8, 6 * 8, 7 * 8},
	8*8					/* size of one char */
};

/* Video hardware */
VIDEO_START( mz80k )
{
}

SCREEN_UPDATE( mz80k )
{
	mz80_state *state = screen->machine().driver_data<mz80_state>();
	int x,y;
	address_space *space = screen->machine().device("maincpu")->memory().space(AS_PROGRAM);
	state->m_mz80k_vertical = state->m_mz80k_vertical ? 0 : 1;
	state->m_mz80k_cursor_cnt++;
	if (state->m_mz80k_cursor_cnt==64) state->m_mz80k_cursor_cnt = 0;

	for(y = 0; y < 25; y++ )
	{
		for(x = 0; x < 40; x++ )
		{
			int code = space->read_byte(0xD000 + x + y*40);
			drawgfx_opaque(bitmap, NULL, screen->machine().gfx[0],  code , 0, 0,0, x*8,y*8);
		}
	}
	return 0;
}
