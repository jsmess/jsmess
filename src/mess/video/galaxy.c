/***************************************************************************

  galaxy.c

  Functions to emulate the video hardware of the Galaksija.
  
  01/03/2008 - Update by Miodrag Milanovic to make Galaksija video work with new SVN code

***************************************************************************/

#include "driver.h"
#include "includes/galaxy.h"
#include "cpu/z80/z80.h"

static int horizontal_pos = 0x0b;

const gfx_layout galaxy_charlayout =
{
	8, 13,				/* 8x8 characters */
	128,				/* 128 characters */
	1,				/* 1 bits per pixel */
	{0},				/* no bitplanes; 1 bit per pixel */
	{7, 6, 5, 4, 3, 2, 1, 0},
	{0*128*8, 1*128*8,  2*128*8,  3*128*8,
	 4*128*8, 5*128*8,  6*128*8,  7*128*8,
	 8*128*8, 9*128*8, 10*8*128, 11*128*8, 12*128*8},
	8 				/* each character takes 1 consecutive byte */
};

const unsigned char galaxy_palette[2*3] =
{
	0xff, 0xff, 0xff,		/* White */
	0x00, 0x00, 0x00		/* Black */
};

PALETTE_INIT( galaxy )
{
	int i;

	for ( i = 0; i < sizeof(galaxy_palette) / 3; i++ ) {
		palette_set_color_rgb(machine, i, galaxy_palette[i*3], galaxy_palette[i*3+1], galaxy_palette[i*3+2]);
	}
}

VIDEO_START( galaxy )
{
}

VIDEO_UPDATE( galaxy )
{
	int offs;
	rectangle black_area = {0,0,0,16*13};
	static int fast_mode = FALSE;
	int full_refresh = 1;

	UINT8* videoram = mess_ram;

	if (!galaxy_interrupts_enabled)
	{
		black_area.min_x = 0;
		black_area.max_x = 32*8-1;
		black_area.min_y = 0;
		black_area.max_y = 16*13-1;
		fillbitmap(bitmap, machine->pens[1], &black_area);
		fast_mode = TRUE;
		return 0;
	}

	if (horizontal_pos!=program_read_byte(0x2ba8))
	{
		full_refresh=1;
		horizontal_pos = program_read_byte(0x2ba8);
		if (horizontal_pos > 0x0b)
		{
			black_area.min_x =  0;
			black_area.max_x =  8*(horizontal_pos-0x0b)-1;
		}
		if (horizontal_pos < 0x0b)
		{
			black_area.min_x = 8*(21+horizontal_pos);
			black_area.max_x = 32*8-1;
		}
		if (horizontal_pos == 0x0b)
			black_area.min_x =  black_area.max_x = 0;
		fillbitmap(bitmap, machine->pens[1], &black_area);
	}

	for( offs = 0; offs < 512; offs++ )
	{
		int sx, sy;
		int code = videoram[offs];

		sx = (offs % 32) * 8 + horizontal_pos*8-88;

		if (sx>=0 && sx<32*8)
		{
			if ((code>63 && code<96) || (code>127 && code<192))
				code-=64;
			if (code>191)
				code-=128;
			sy = (offs / 32) * 13;
			drawgfx(bitmap, machine->gfx[0], code & 0x7f, 0, 0,0, sx,sy,
				NULL, TRANSPARENCY_NONE, 0);
		}
	}

	galaxy_interrupts_enabled = FALSE;
	return 0;
}
