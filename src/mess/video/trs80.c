/***************************************************************************

  trs80.c

  Functions to emulate the video hardware of the TRS80.

***************************************************************************/
#include "emu.h"
#include "includes/trs80.h"

/* Bit assignment for "trs80_mode"
    d7 Page select
    d6 LNW80 switch to graphics ram
    d5 LNW80 colour or monochrome (1=colour)
    d4 LNW80 lores or hires (1=hires) also does 64 or 80 chars per line
    d3 LNW80 invert entire screen / Model III/4 Invert characters with bit 7 set (1=invert)
    d2 80/40 or 64/32 characters per line (1=80)
    d1 7 or 8 bit video (1=requires 7-bit, 0=don't care)
    d0 80/64 or 40/32 characters per line (1=32) */


WRITE8_HANDLER( trs80m4_88_w )
{
	trs80_state *state = space->machine().driver_data<trs80_state>();
/* This is for the programming of the CRTC registers.
    However this CRTC is mask-programmed, and only the
    start address register can be used. The cursor and
    light-pen facilities are ignored. The character clock
    is changed depending on the screen size chosen.
    Therefore it is easier to use normal
    coding rather than the mc6845 device. */

	if (!offset) state->m_crtc_reg = data & 0x1f;

	if (offset) switch (state->m_crtc_reg)
	{
		case 12:
			state->m_start_address = (state->m_start_address & 0x00ff) | (data << 8);
			break;
		case 13:
			state->m_start_address = (state->m_start_address & 0xff00) | data;
	}
}


VIDEO_START( trs80 )
{
	trs80_state *state = machine.driver_data<trs80_state>();
	state->m_size_store = 0xff;
	state->m_mode &= 2;
}


/* 7 or 8-bit video, 32/64 characters per line = trs80, trs80l2, sys80 */
SCREEN_UPDATE( trs80 )
{
	trs80_state *state = screen->machine().driver_data<trs80_state>();
	UINT8 *videoram = state->m_videoram;
	UINT8 y,ra,chr,gfx,gfxbit;
	UINT16 sy=0,ma=0,x;
	UINT8 *FNT = screen->machine().region("gfx1")->base();
	UINT8 cols = (state->m_mode & 1) ? 32 : 64;
	UINT8 skip = (state->m_mode & 1) ? 2 : 1;

	if (state->m_mode != state->m_size_store)
	{
		state->m_size_store = state->m_mode & 1;
		screen->set_visible_area(0, cols*FW-1, 0, 16*FH-1);
	}

	for (y = 0; y < 16; y++)
	{
		for (ra = 0; ra < FH; ra++)
		{
			UINT16 *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = ma; x < ma + 64; x+=skip)
			{
				chr = videoram[x];

				if (chr & 0x80)
				{
					gfxbit = 1<<((ra & 0x0c)>>1);
					/* Display one line of a lores character (6 pixels) */
					*p++ = ( chr & gfxbit ) ? 1 : 0;
					*p++ = ( chr & gfxbit ) ? 1 : 0;
					*p++ = ( chr & gfxbit ) ? 1 : 0;
					gfxbit <<= 1;
					*p++ = ( chr & gfxbit ) ? 1 : 0;
					*p++ = ( chr & gfxbit ) ? 1 : 0;
					*p++ = ( chr & gfxbit ) ? 1 : 0;
				}
				else
				{
					if ((state->m_mode & 2) && (chr < 32)) chr+=64;

					/* get pattern of pixels for that character scanline */
					if (ra < 8)
						gfx = FNT[(chr<<3) | ra ];
					else
						gfx = 0;

					/* Display a scanline of a character (6 pixels) */
					*p++ = ( gfx & 0x20 ) ? 1 : 0;
					*p++ = ( gfx & 0x10 ) ? 1 : 0;
					*p++ = ( gfx & 0x08 ) ? 1 : 0;
					*p++ = ( gfx & 0x04 ) ? 1 : 0;
					*p++ = ( gfx & 0x02 ) ? 1 : 0;
					*p++ = ( gfx & 0x01 ) ? 1 : 0;
				}
			}
		}
		ma+=64;
	}
	return 0;
}

/* 8-bit video, 32/64/40/80 characters per line = trs80m3, trs80m4. */
SCREEN_UPDATE( trs80m4 )
{
	trs80_state *state = screen->machine().driver_data<trs80_state>();
	UINT8 *videoram = state->m_videoram;
	UINT8 y,ra,chr,gfx,gfxbit;
	UINT16 sy=0,ma=0,x;
	UINT8 *FNT = screen->machine().region("gfx1")->base();
	UINT8 skip=1;
	UINT8 cols = (state->m_mode & 4) ? 80 : 64;
	UINT8 rows = (state->m_mode & 4) ? 24 : 16;
	UINT8 lines = (state->m_mode & 4) ? 10 : 12;
	UINT8 s_cols = cols;
	UINT8 mask = (state->m_mode & 0x20) ? 0xff : 0xbf;	/* Select Japanese or extended chars */

	if (state->m_mode & 1)
	{
		s_cols >>= 1;
		skip = 2;
	}

	if ((state->m_mode & 0x7f) != state->m_size_store)
	{
		state->m_size_store = state->m_mode & 5;
		screen->set_visible_area(0, s_cols*8-1, 0, rows*lines-1);
	}

	for (y = 0; y < rows; y++)
	{
		for (ra = 0; ra < lines; ra++)
		{
			UINT16 *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = ma; x < ma + cols; x+=skip)
			{
				chr = videoram[x+state->m_start_address];

				if (((chr & 0xc0) == 0xc0) && (~state->m_mode & 8))
				{
					if (ra < 8)
						gfx = FNT[((chr&mask)<<3) | ra ];
					else
						gfx = 0;

					*p++ = ( gfx & 0x80 ) ? 1 : 0;
					*p++ = ( gfx & 0x40 ) ? 1 : 0;
					*p++ = ( gfx & 0x20 ) ? 1 : 0;
					*p++ = ( gfx & 0x10 ) ? 1 : 0;
					*p++ = ( gfx & 0x08 ) ? 1 : 0;
					*p++ = ( gfx & 0x04 ) ? 1 : 0;
					*p++ = ( gfx & 0x02 ) ? 1 : 0;
					*p++ = ( gfx & 0x01 ) ? 1 : 0;
				}
				else
				if ((chr & 0x80) && (~state->m_mode & 8))
				{
					gfxbit = 1<<((ra & 0x0c)>>1);
					/* Display one line of a lores character */
					*p++ = ( chr & gfxbit ) ? 1 : 0;
					*p++ = ( chr & gfxbit ) ? 1 : 0;
					*p++ = ( chr & gfxbit ) ? 1 : 0;
					*p++ = ( chr & gfxbit ) ? 1 : 0;
					gfxbit <<= 1;
					*p++ = ( chr & gfxbit ) ? 1 : 0;
					*p++ = ( chr & gfxbit ) ? 1 : 0;
					*p++ = ( chr & gfxbit ) ? 1 : 0;
					*p++ = ( chr & gfxbit ) ? 1 : 0;
				}
				else
				{
					/* get pattern of pixels for that character scanline */
					if (ra < 8)
						gfx = FNT[((chr&0x7f)<<3) | ra ];
					else
						gfx = 0;

					/* if inverse mode, and bit 7 set, invert gfx */
					if ((state->m_mode & 8) && (chr & 0x80))
						gfx ^= 0xff;

					/* Display a scanline of a character */
					*p++ = ( gfx & 0x80 ) ? 1 : 0;
					*p++ = ( gfx & 0x40 ) ? 1 : 0;
					*p++ = ( gfx & 0x20 ) ? 1 : 0;
					*p++ = ( gfx & 0x10 ) ? 1 : 0;
					*p++ = ( gfx & 0x08 ) ? 1 : 0;
					*p++ = ( gfx & 0x04 ) ? 1 : 0;
					*p++ = ( gfx & 0x02 ) ? 1 : 0;
					*p++ = ( gfx & 0x01 ) ? 1 : 0;
				}
			}
		}
		ma+=cols;
	}
	return 0;
}

/* 7 or 8-bit video, 64/32 characters per line = ht1080z, ht1080z2, ht108064 */
SCREEN_UPDATE( ht1080z )
{
	trs80_state *state = screen->machine().driver_data<trs80_state>();
	UINT8 *videoram = state->m_videoram;
	UINT8 y,ra,chr,gfx,gfxbit;
	UINT16 sy=0,ma=0,x;
	UINT8 *FNT = screen->machine().region("gfx1")->base();
	UINT8 cols = (state->m_mode & 1) ? 32 : 64;
	UINT8 skip = (state->m_mode & 1) ? 2 : 1;

	if (state->m_mode != state->m_size_store)
	{
		state->m_size_store = state->m_mode & 1;
		screen->set_visible_area(0, cols*FW-1, 0, 16*FH-1);
	}

	for (y = 0; y < 16; y++)
	{
		for (ra = 0; ra < FH; ra++)
		{
			UINT16 *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = ma; x < ma + 64; x+=skip)
			{
				chr = videoram[x];

				if (chr & 0x80)
				{
					gfxbit = 1<<((ra & 0x0c)>>1);
					/* Display one line of a lores character (6 pixels) */
					*p++ = ( chr & gfxbit ) ? 1 : 0;
					*p++ = ( chr & gfxbit ) ? 1 : 0;
					*p++ = ( chr & gfxbit ) ? 1 : 0;
					gfxbit <<= 1;
					*p++ = ( chr & gfxbit ) ? 1 : 0;
					*p++ = ( chr & gfxbit ) ? 1 : 0;
					*p++ = ( chr & gfxbit ) ? 1 : 0;
				}
				else
				{
					if ((state->m_mode & 2) && (chr < 32)) chr+=64;

					/* get pattern of pixels for that character scanline */
					gfx = FNT[(chr<<4) | ra ];

					/* Display a scanline of a character (6 pixels) */
					*p++ = ( gfx & 0x80 ) ? 1 : 0;
					*p++ = ( gfx & 0x40 ) ? 1 : 0;
					*p++ = ( gfx & 0x20 ) ? 1 : 0;
					*p++ = ( gfx & 0x10 ) ? 1 : 0;
					*p++ = ( gfx & 0x08 ) ? 1 : 0;
					*p++ = 0;	/* fix for ht108064 */
				}
			}
		}
		ma+=64;
	}
	return 0;
}

/* 8-bit video, 64/80 characters per line = lnw80 */
SCREEN_UPDATE( lnw80 )
{
	trs80_state *state = screen->machine().driver_data<trs80_state>();
	UINT8 *videoram = state->m_videoram;
	static const UINT16 rows[] = { 0, 0x200, 0x100, 0x300, 1, 0x201, 0x101, 0x301 };
	UINT8 chr,gfx,gfxbit,bg=7,fg=0;
	UINT16 sy=0,ma=0,x,y,ra;
	UINT8 *FNT = screen->machine().region("gfx1")->base();
	UINT8 cols = (state->m_mode & 0x10) ? 80 : 64;

	/* Although the OS can select 32-character mode, it is not supported by hardware */
	if (state->m_mode != state->m_size_store)
	{
		state->m_size_store = state->m_mode & 0x10;
		screen->set_visible_area(0, cols*FW-1, 0, 16*FH-1);
	}

	if (state->m_mode & 8)
	{
		bg = 0;
		fg = 7;
	}

	switch (state->m_mode & 0x30)
	{
		case 0:					// MODE 0
			for (y = 0; y < 16; y++)
			{
				for (ra = 0; ra < FH; ra++)
				{
					UINT16 *p = BITMAP_ADDR16(bitmap, sy++, 0);

					for (x = ma; x < ma + 64; x++)
					{
						chr = videoram[x];

						if (chr & 0x80)
						{
							gfxbit = 1<<((ra & 0x0c)>>1);
							/* Display one line of a lores character (6 pixels) */
							*p++ = ( chr & gfxbit ) ? fg : bg;
							*p++ = ( chr & gfxbit ) ? fg : bg;
							*p++ = ( chr & gfxbit ) ? fg : bg;
							gfxbit <<= 1;
							*p++ = ( chr & gfxbit ) ? fg : bg;
							*p++ = ( chr & gfxbit ) ? fg : bg;
							*p++ = ( chr & gfxbit ) ? fg : bg;
						}
						else
						{
							/* get pattern of pixels for that character scanline */
							if (ra < 8)
								gfx = FNT[(chr<<1) | rows[ra] ];
							else
								gfx = 0;

							/* Display a scanline of a character (6 pixels) */
							*p++ = ( gfx & 0x04 ) ? fg : bg;
							*p++ = ( gfx & 0x02 ) ? fg : bg;
							*p++ = ( gfx & 0x40 ) ? fg : bg;
							*p++ = ( gfx & 0x80 ) ? fg : bg;
							*p++ = ( gfx & 0x20 ) ? fg : bg;
							*p++ = ( gfx & 0x08 ) ? fg : bg;
						}
					}
				}
				ma+=64;
			}
			break;

		case 0x10:					// MODE 1
			for (y = 0; y < 0x400; y+=0x40)
			{
				for (ra = 0; ra < 0x3000; ra+=0x400)
				{
					UINT16 *p = BITMAP_ADDR16(bitmap, sy++, 0);

					for (x = 0; x < 0x40; x++)
					{
						gfx = state->m_gfxram[ y | x | ra];
						/* Display 6 pixels in normal region */
						*p++ = ( gfx & 0x01 ) ? fg : bg;
						*p++ = ( gfx & 0x02 ) ? fg : bg;
						*p++ = ( gfx & 0x04 ) ? fg : bg;
						*p++ = ( gfx & 0x08 ) ? fg : bg;
						*p++ = ( gfx & 0x10 ) ? fg : bg;
						*p++ = ( gfx & 0x20 ) ? fg : bg;
					}

					for (x = 0; x < 0x10; x++)
					{
						gfx = state->m_gfxram[ 0x3000 | x | (ra & 0xc00) | ((ra & 0x3000) >> 8)];
						/* Display 6 pixels in extended region */
						*p++ = ( gfx & 0x01 ) ? fg : bg;
						*p++ = ( gfx & 0x02 ) ? fg : bg;
						*p++ = ( gfx & 0x04 ) ? fg : bg;
						*p++ = ( gfx & 0x08 ) ? fg : bg;
						*p++ = ( gfx & 0x10 ) ? fg : bg;
						*p++ = ( gfx & 0x20 ) ? fg : bg;
					}
				}
			}
			break;

		case 0x20:					// MODE 2
			/* it seems the text video ram can have an effect in this mode,
                not explained clearly, so not emulated */
			for (y = 0; y < 0x400; y+=0x40)
			{
				for (ra = 0; ra < 0x3000; ra+=0x400)
				{
					UINT16 *p = BITMAP_ADDR16(bitmap, sy++, 0);

					for (x = 0; x < 0x40; x++)
					{
						gfx = state->m_gfxram[ y | x | ra];
						/* Display 6 pixels in normal region */
						fg = (gfx & 0x38) >> 3;
						*p++ = fg;
						*p++ = fg;
						*p++ = fg;
						fg = gfx & 0x07;
						*p++ = fg;
						*p++ = fg;
						*p++ = fg;
					}
				}
			}
			break;

		case 0x30:					// MODE 3
			/* the manual does not explain at all how colour is determined
                for the extended area. Further, the background colour
                is not mentioned anywhere. Black is assumed. */
			for (y = 0; y < 0x400; y+=0x40)
			{
				for (ra = 0; ra < 0x3000; ra+=0x400)
				{
					UINT16 *p = BITMAP_ADDR16(bitmap, sy++, 0);

					for (x = 0; x < 0x40; x++)
					{
						gfx = state->m_gfxram[ y | x | ra];
						fg = (videoram[ 0x3c00 | x | y ] & 0x38) >> 3;
						/* Display 6 pixels in normal region */
						*p++ = ( gfx & 0x01 ) ? fg : bg;
						*p++ = ( gfx & 0x02 ) ? fg : bg;
						*p++ = ( gfx & 0x04 ) ? fg : bg;
						fg = videoram[ 0x3c00 | x | y ] & 0x07;
						*p++ = ( gfx & 0x08 ) ? fg : bg;
						*p++ = ( gfx & 0x10 ) ? fg : bg;
						*p++ = ( gfx & 0x20 ) ? fg : bg;
					}

					for (x = 0; x < 0x10; x++)
					{
						gfx = state->m_gfxram[ 0x3000 | x | (ra & 0xc00) | ((ra & 0x3000) >> 8)];
						fg = (state->m_gfxram[ 0x3c00 | x | y ] & 0x38) >> 3;
						/* Display 6 pixels in extended region */
						*p++ = ( gfx & 0x01 ) ? fg : bg;
						*p++ = ( gfx & 0x02 ) ? fg : bg;
						*p++ = ( gfx & 0x04 ) ? fg : bg;
						fg = state->m_gfxram[ 0x3c00 | x | y ] & 0x07;
						*p++ = ( gfx & 0x08 ) ? fg : bg;
						*p++ = ( gfx & 0x10 ) ? fg : bg;
						*p++ = ( gfx & 0x20 ) ? fg : bg;
					}
				}
			}
			break;
	}
	return 0;
}

/* lores characters are in the character generator. Each character is 8x16. */
SCREEN_UPDATE( radionic )
{
	trs80_state *state = screen->machine().driver_data<trs80_state>();
	UINT8 *videoram = state->m_videoram;
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=0,x;
	UINT8 *FNT = screen->machine().region("gfx1")->base();
	UINT8 cols = (state->m_mode & 1) ? 32 : 64;
	UINT8 skip = (state->m_mode & 1) ? 2 : 1;

	if (state->m_mode != state->m_size_store)
	{
		state->m_size_store = state->m_mode & 1;
		screen->set_visible_area(0, cols*8-1, 0, 16*16-1);
	}

	for (y = 0; y < 16; y++)
	{
		for (ra = 0; ra < 16; ra++)
		{
			UINT16 *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = ma; x < ma + 64; x+=skip)
			{
				chr = videoram[x];

				/* get pattern of pixels for that character scanline */
				gfx = FNT[(chr<<3) | (ra & 7) | (ra & 8) << 8];

				/* Display a scanline of a character (8 pixels) */
				*p++ = ( gfx & 0x01 ) ? 1 : 0;
				*p++ = ( gfx & 0x02 ) ? 1 : 0;
				*p++ = ( gfx & 0x04 ) ? 1 : 0;
				*p++ = ( gfx & 0x08 ) ? 1 : 0;
				*p++ = ( gfx & 0x10 ) ? 1 : 0;
				*p++ = ( gfx & 0x20 ) ? 1 : 0;
				*p++ = ( gfx & 0x40 ) ? 1 : 0;
				*p++ = ( gfx & 0x80 ) ? 1 : 0;
			}
		}
		ma+=64;
	}
	return 0;
}


/***************************************************************************
  Write to video ram
***************************************************************************/

READ8_HANDLER( trs80_videoram_r )
{
	trs80_state *state = space->machine().driver_data<trs80_state>();
	UINT8 *videoram = state->m_videoram;
	if ((state->m_mode & 0x80) && (~state->m_model4 & 1)) offset |= 0x400;
	return videoram[offset];
}

WRITE8_HANDLER( trs80_videoram_w )
{
	trs80_state *state = space->machine().driver_data<trs80_state>();
	UINT8 *videoram = state->m_videoram;
	if ((state->m_mode & 0x80) && (~state->m_model4 & 1)) offset |= 0x400;
	videoram[offset] = data;
}


/***************************************************************************
  Write to graphics ram
***************************************************************************/

READ8_HANDLER( trs80_gfxram_r )
{
	trs80_state *state = space->machine().driver_data<trs80_state>();
	return state->m_gfxram[offset];
}

WRITE8_HANDLER( trs80_gfxram_w )
{
	trs80_state *state = space->machine().driver_data<trs80_state>();
	state->m_gfxram[offset] = data;
}


/***************************************************************************
  Palettes
***************************************************************************/

/* Levels are unknown - guessing */
static const rgb_t lnw80_palette[] =
{
	MAKE_RGB(220, 220, 220), // white
	MAKE_RGB(0, 175, 0), // green
	MAKE_RGB(200, 200, 0), // yellow
	MAKE_RGB(255, 0, 0), // red
	MAKE_RGB(255, 0, 255), // magenta
	MAKE_RGB(0, 0, 175), // blue
	MAKE_RGB(0, 255, 255), // cyan
	MAKE_RGB(0, 0, 0), // black
};

PALETTE_INIT( lnw80 )
{
	palette_set_colors(machine, 0, lnw80_palette, ARRAY_LENGTH(lnw80_palette));
}
