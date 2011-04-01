
#include "emu.h"
#include "includes/kaypro.h"

static const UINT8 mc6845_mask[32]={0xff,0xff,0xff,0x0f,0x7f,0x1f,0x7f,0x7f,3,0x1f,0x7f,0x1f,0x3f,0xff,0x3f,0xff,0,0};


/***********************************************************

    Video

************************************************************/

PALETTE_INIT( kaypro )
{
	palette_set_color(machine, 0, RGB_BLACK); /* black */
	palette_set_color(machine, 1, MAKE_RGB(0, 220, 0)); /* green */
	palette_set_color(machine, 2, MAKE_RGB(0, 110, 0)); /* low intensity green */
}

SCREEN_UPDATE( kayproii )
{
	kaypro_state *state = screen->machine().driver_data<kaypro_state>();
	UINT8 *videoram = state->m_videoram;
/* The display consists of 80 columns and 24 rows. Each row is allocated 128 bytes of ram,
    but only the first 80 are used. The total video ram therefore is 0x0c00 bytes.
    There is one video attribute: bit 7 causes blinking. The first half of the
    character generator is blank, with the visible characters in the 2nd half.
    During the "off" period of blanking, the first half is used. Only 5 pixels are
    connected from the rom to the shift register, the remaining pixels are held high.
    A high pixel is black and a low pixel is green. */

	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=0,x;

	state->m_framecnt++;

	for (y = 0; y < 24; y++)
	{
		for (ra = 0; ra < 10; ra++)
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = ma; x < ma + 80; x++)
			{
				if (ra < 8)
				{
					chr = videoram[x]^0x80;

					/* Take care of flashing characters */
					if ((chr < 0x80) && (state->m_framecnt & 0x08))
						chr |= 0x80;

					/* get pattern of pixels for that character scanline */
					gfx = state->m_FNT[(chr<<3) | ra ];
				}
				else
					gfx = 0xff;

				/* Display a scanline of a character (7 pixels) */
				*p++ = 0;
				*p++ = ( gfx & 0x10 ) ? 0 : 1;
				*p++ = ( gfx & 0x08 ) ? 0 : 1;
				*p++ = ( gfx & 0x04 ) ? 0 : 1;
				*p++ = ( gfx & 0x02 ) ? 0 : 1;
				*p++ = ( gfx & 0x01 ) ? 0 : 1;
				*p++ = 0;
			}
		}
		ma+=128;
	}
	return 0;
}

SCREEN_UPDATE( omni2 )
{
	kaypro_state *state = screen->machine().driver_data<kaypro_state>();
	UINT8 *videoram = state->m_videoram;
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=0,x;

	state->m_framecnt++;

	for (y = 0; y < 24; y++)
	{
		for (ra = 0; ra < 10; ra++)
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = ma; x < ma + 80; x++)
			{
				if (ra < 8)
				{
					chr = videoram[x];

					/* Take care of flashing characters */
					if ((chr > 0x7f) && (state->m_framecnt & 0x08))
						chr |= 0x80;

					/* get pattern of pixels for that character scanline */
					gfx = state->m_FNT[(chr<<3) | ra ];
				}
				else
					gfx = 0xff;

				/* Display a scanline of a character (7 pixels) */
				*p++ = ( gfx & 0x40 ) ? 0 : 1;
				*p++ = ( gfx & 0x20 ) ? 0 : 1;
				*p++ = ( gfx & 0x10 ) ? 0 : 1;
				*p++ = ( gfx & 0x08 ) ? 0 : 1;
				*p++ = ( gfx & 0x04 ) ? 0 : 1;
				*p++ = ( gfx & 0x02 ) ? 0 : 1;
				*p++ = ( gfx & 0x01 ) ? 0 : 1;
			}
		}
		ma+=128;
	}
	return 0;
}

SCREEN_UPDATE( kaypro2x )
{
	kaypro_state *state = screen->machine().driver_data<kaypro_state>();
	state->m_framecnt++;
	state->m_speed = state->m_mc6845_reg[10]&0x20;
	state->m_flash = state->m_mc6845_reg[10]&0x40;				// cursor modes
	state->m_cursor = (state->m_mc6845_reg[14]<<8) | state->m_mc6845_reg[15];					// get cursor position
	mc6845_update(state->m_mc6845, bitmap, cliprect);
	return 0;
}

/* bit 6 of kaypro2x_system_port selects alternate characters (A12 on character generator rom).
    The diagram specifies a 2732 with 28 pins, and more address pins. Possibly a 2764 or 27128.
    Since our dump only goes up to A11, the alternate character set doesn't exist.

    0000-07FF of videoram is memory-mapped characters; 0800-0FFF is equivalent attribute bytes.
    d3 Underline
    d2 blinking (at unknown rate)
    d1 low intensity
    d0 reverse video

    Not sure how the attributes interact, for example does an underline blink? */


MC6845_UPDATE_ROW( kaypro2x_update_row )
{
	kaypro_state *state = device->machine().driver_data<kaypro_state>();
	UINT8 *videoram = state->m_videoram;
	UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);
	UINT16 x;
	UINT8 gfx,fg,bg;

	for (x = 0; x < x_count; x++)				// for each character
	{
		UINT8 inv=0;
		//      if (x == cursor_x) inv=0xff;    /* uncomment when mame fixed */
		UINT16 mem = (ma + x) & 0x7ff;
		UINT8 chr = videoram[mem];
		UINT8 attr = videoram[mem | 0x800];

		if ((attr & 3) == 3)
		{
			fg = 0;
			bg = 2;
		}
		else
		if ((attr & 3) == 2)
		{
			fg = 2;
			bg = 0;
		}
		else
		if ((attr & 3) == 1)
		{
			fg = 0;
			bg = 1;
		}
		else
		{
			fg = 1;
			bg = 0;
		}

		/* Take care of flashing characters */
		if ((attr & 4) && (state->m_framecnt & 8))
			fg = bg;

		/* process cursor - remove when mame fixed */
		if ((((!state->m_flash) && (!state->m_speed)) ||
			((state->m_flash) && (state->m_speed) && (state->m_framecnt & 0x10)) ||
			((state->m_flash) && (!state->m_speed) && (state->m_framecnt & 8))) &&
			(mem == state->m_cursor))
				inv ^= state->m_mc6845_cursor[ra];

		/* get pattern of pixels for that character scanline */
		if ((ra == 15) && (attr & 8))	/* underline */
			gfx = 0xff;
		else
			gfx = state->m_FNT[(chr<<4) | ra ] ^ inv;

		/* Display a scanline of a character (8 pixels) */
		*p++ = ( gfx & 0x80 ) ? fg : bg;
		*p++ = ( gfx & 0x40 ) ? fg : bg;
		*p++ = ( gfx & 0x20 ) ? fg : bg;
		*p++ = ( gfx & 0x10 ) ? fg : bg;
		*p++ = ( gfx & 0x08 ) ? fg : bg;
		*p++ = ( gfx & 0x04 ) ? fg : bg;
		*p++ = ( gfx & 0x02 ) ? fg : bg;
		*p++ = ( gfx & 0x01 ) ? fg : bg;
	}
}

/************************************* MC6845 SUPPORT ROUTINES ***************************************/

/* The 6845 can produce a variety of cursor shapes - all are emulated here - remove when mame fixed */
static void mc6845_cursor_configure(kaypro_state *state)
{
	UINT8 i,curs_type=0,r9,r10,r11;

	/* curs_type holds the general cursor shape to be created
        0 = no cursor
        1 = partial cursor (only shows on a block of scan lines)
        2 = full cursor
        3 = two-part cursor (has a part at the top and bottom with the middle blank) */

	for ( i = 0; i < ARRAY_LENGTH(state->m_mc6845_cursor); i++) state->m_mc6845_cursor[i] = 0;		// prepare cursor by erasing old one

	r9  = state->m_mc6845_reg[9];					// number of scan lines - 1
	r10 = state->m_mc6845_reg[10] & 0x1f;				// cursor start line = last 5 bits
	r11 = state->m_mc6845_reg[11]+1;					// cursor end line incremented to suit for-loops below

	/* decide the curs_type by examining the registers */
	if (r10 < r11) curs_type=1;				// start less than end, show start to end
	else
	if (r10 == r11) curs_type=2;				// if equal, show full cursor
	else curs_type=3;					// if start greater than end, it's a two-part cursor

	if ((r11 - 1) > r9) curs_type=2;			// if end greater than scan-lines, show full cursor
	if (r10 > r9) curs_type=0;				// if start greater than scan-lines, then no cursor
	if (r11 > 16) r11=16;					// truncate 5-bit register to fit our 4-bit hardware

	/* create the new cursor */
	if (curs_type > 1) for (i = 0;i < ARRAY_LENGTH(state->m_mc6845_cursor);i++) state->m_mc6845_cursor[i]=0xff; // turn on full cursor

	if (curs_type == 1) for (i = r10;i < r11;i++) state->m_mc6845_cursor[i]=0xff; // for each line that should show, turn on that scan line

	if (curs_type == 3) for (i = r11; i < r10;i++) state->m_mc6845_cursor[i]=0; // now take a bite out of the middle
}

/* Resize the screen within the limits of the hardware. Expand the image to fill the screen area.
    Standard screen is 640 x 400 = 0x7d0 bytes. */

static void mc6845_screen_configure(running_machine &machine)
{
	kaypro_state *state = machine.driver_data<kaypro_state>();
	rectangle visarea;

	UINT16 width = state->m_mc6845_reg[1]*8-1;							// width in pixels
	UINT16 height = state->m_mc6845_reg[6]*(state->m_mc6845_reg[9]+1)-1;					// height in pixels
	UINT16 bytes = state->m_mc6845_reg[1]*state->m_mc6845_reg[6]-1;						// video ram needed -1

	/* Resize the screen */
	visarea.min_x = 0;
	visarea.max_x = width-1;
	visarea.min_y = 0;
	visarea.max_y = height-1;
	if ((width < 640) && (height < 400) && (bytes < 0x800))	/* bounds checking to prevent an assert or violation */
		machine.primary_screen->set_visible_area(0, width, 0, height);
}


/**************************** I/O PORTS *****************************************************************/

READ8_HANDLER( kaypro2x_status_r )
{
	kaypro_state *state = space->machine().driver_data<kaypro_state>();
/* Need bit 7 high or computer hangs */

	return 0x80 | mc6845_register_r( state->m_mc6845, 0);
}

WRITE8_HANDLER( kaypro2x_index_w )
{
	kaypro_state *state = space->machine().driver_data<kaypro_state>();
	state->m_mc6845_ind = data & 0x1f;
	mc6845_address_w( state->m_mc6845, 0, data );
}

WRITE8_HANDLER( kaypro2x_register_w )
{
	kaypro_state *state = space->machine().driver_data<kaypro_state>();
	if (state->m_mc6845_ind < 16)
		state->m_mc6845_reg[state->m_mc6845_ind] = data & mc6845_mask[state->m_mc6845_ind];	/* save data in register */
	else
		state->m_mc6845_reg[state->m_mc6845_ind] = data;

	mc6845_register_w( state->m_mc6845, 0, data );

	if ((state->m_mc6845_ind == 1) || (state->m_mc6845_ind == 6) || (state->m_mc6845_ind == 9))
		mc6845_screen_configure(space->machine());			/* adjust screen size */

	if ((state->m_mc6845_ind > 8) && (state->m_mc6845_ind < 12))
		mc6845_cursor_configure(state);		/* adjust cursor shape - remove when mame fixed */

	if ((state->m_mc6845_ind > 17) && (state->m_mc6845_ind < 20))
		state->m_mc6845_video_address = state->m_mc6845_reg[19] | ((state->m_mc6845_reg[18] & 0x3f) << 8);	/* internal ULA address */
}

READ8_HANDLER( kaypro_videoram_r )
{
	kaypro_state *state = space->machine().driver_data<kaypro_state>();
	UINT8 *videoram = state->m_videoram;
	return videoram[offset];
}

WRITE8_HANDLER( kaypro_videoram_w )
{
	kaypro_state *state = space->machine().driver_data<kaypro_state>();
	UINT8 *videoram = state->m_videoram;
	videoram[offset] = data;
}

READ8_HANDLER( kaypro2x_videoram_r )
{
	kaypro_state *state = space->machine().driver_data<kaypro_state>();
	UINT8 *videoram = state->m_videoram;
	return videoram[state->m_mc6845_video_address];
}

WRITE8_HANDLER( kaypro2x_videoram_w )
{
	kaypro_state *state = space->machine().driver_data<kaypro_state>();
	UINT8 *videoram = state->m_videoram;
	videoram[state->m_mc6845_video_address] = data;
}

VIDEO_START( kaypro )
{
	kaypro_state *state = machine.driver_data<kaypro_state>();
	state->m_mc6845 = machine.device("crtc");
	state->m_FNT = machine.region("gfx1")->base();
}
