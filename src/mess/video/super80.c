/* Super80.c written by Robbbert, 2005-2010. See the MESS wiki for documentation. */

/* Notes on using MAME MC6845 Device (MMD).
    1. Speed of MMD is about 20% slower than pre-MMD coding
    2. Undocumented cursor start and end-lines is not supported by MMD, so we do it here. */


#include "emu.h"
#include "includes/super80.h"






/**************************** PALETTES for super80m and super80v ******************************************/

static const UINT8 super80_rgb_palette[16*3] =
{
	0x00, 0x00, 0x00,	/*  0 Black     */
	0x00, 0x00, 0x00,	/*  1 Black     */
	0x00, 0x00, 0x7f,	/*  2 Blue      */
	0x00, 0x00, 0xff,	/*  3 Light Blue    */
	0x00, 0x7f, 0x00,	/*  4 Green     */
	0x00, 0xff, 0x00,	/*  5 Bright Green  */
	0x00, 0x7f, 0x7f,	/*  6 Cyan      */
	0x00, 0xff, 0xff,	/*  7 Turquoise     */
	0x7f, 0x00, 0x00,	/*  8 Dark Red      */
	0xff, 0x00, 0x00,	/*  9 Red       */
	0x7f, 0x00, 0x7f,	/* 10 Purple        */
	0xff, 0x00, 0xff,	/* 11 Magenta       */
	0x7f, 0x7f, 0x00,	/* 12 Lime      */
	0xff, 0xff, 0x00,	/* 13 Yellow        */
	0xbf, 0xbf, 0xbf,	/* 14 Off White     */
	0xff, 0xff, 0xff,	/* 15 White     */
};

static const UINT8 super80_comp_palette[16*3] =
{
	0x00, 0x00, 0x00,	/*  0 Black     */
	0x80, 0x80, 0x80,	/*  1 Grey      */
	0x00, 0x00, 0xff,	/*  2 Blue      */
	0xff, 0xff, 0x80,	/*  3 Light Yellow  */
	0x00, 0xff, 0x00,	/*  4 Green     */
	0xff, 0x80, 0xff,	/*  5 Light Magenta */
	0x00, 0xff, 0xff,	/*  6 Cyan      */
	0xff, 0x40, 0x40,	/*  7 Light Red     */
	0xff, 0x00, 0x00,	/*  8 Red       */
	0x00, 0x80, 0x80,	/*  9 Dark Cyan     */
	0xff, 0x00, 0xff,	/* 10 Magenta       */
	0x80, 0xff, 0x80,	/* 11 Light Green   */
	0xff, 0xff, 0x00,	/* 12 Yellow        */
	0x00, 0x00, 0x80,	/* 13 Dark Blue     */
	0xff, 0xff, 0xff,	/* 14 White     */
	0x00, 0x00, 0x00,	/* 15 Black     */
};

static void palette_set_colors_rgb(running_machine *machine, const UINT8 *colors)
{
	UINT8 r, b, g, color_count = 16;

	while (color_count--)
	{
		r = *colors++; g = *colors++; b = *colors++;
		palette_set_color(machine, 15-color_count, MAKE_RGB(r, g, b));
	}
}

PALETTE_INIT( super80m )
{
	palette_set_colors_rgb(machine, super80_rgb_palette);
}



VIDEO_EOF( super80m )
{
	super80_state *state = machine->driver_data<super80_state>();
	/* if we chose another palette or colour mode, enable it */
	UINT8 chosen_palette = (input_port_read(machine, "CONFIG") & 0x60)>>5;				// read colour dipswitches

	if (chosen_palette != state->current_palette)						// any changes?
	{
		state->current_palette = chosen_palette;					// save new palette
		if (!state->current_palette)
			palette_set_colors_rgb(machine, super80_comp_palette);		// composite colour
		else
			palette_set_colors_rgb(machine, super80_rgb_palette);		// rgb and b&w
	}
}

VIDEO_UPDATE( super80 )
{
	super80_state *state = screen->machine->driver_data<super80_state>();
	UINT8 y,ra,chr=32,gfx,screen_on=0;
	UINT16 sy=0,ma=state->vidpg,x;
	UINT8 *RAM = screen->machine->region("maincpu")->base();

	output_set_value("cass_led",(state->super80_shared & 0x20) ? 1 : 0);

	if ((state->super80_shared & 4) || (!(input_port_read(screen->machine, "CONFIG") & 4)))	/* bit 2 of port F0 is high, OR user turned on config switch */
		screen_on++;

	for (y = 0; y < 16; y++)
	{
		for (ra = 0; ra < 10; ra++)
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = 0; x < 32; x++)	// done this way to avoid x overflowing on page FF
			{
				if (screen_on)
					chr = RAM[ma | x] & 0x3f;

				/* get pattern of pixels for that character scanline */
				gfx = state->FNT[(chr<<4) | ((ra & 8) >> 3) | ((ra & 7) << 1)];

				/* Display a scanline of a character (8 pixels) */
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
		ma+=32;
	}
	return 0;
}

VIDEO_UPDATE( super80d )
{
	super80_state *state = screen->machine->driver_data<super80_state>();
	UINT8 y,ra,chr=32,gfx,screen_on=0;
	UINT16 sy=0,ma=state->vidpg,x;
	UINT8 *RAM = screen->machine->region("maincpu")->base();

	output_set_value("cass_led",(state->super80_shared & 0x20) ? 1 : 0);

	if ((state->super80_shared & 4) || (!(input_port_read(screen->machine, "CONFIG") & 4)))	/* bit 2 of port F0 is high, OR user turned on config switch */
		screen_on++;

	for (y = 0; y < 16; y++)
	{
		for (ra = 0; ra < 10; ra++)
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = 0; x < 32; x++)
			{
				if (screen_on)
					chr = RAM[ma | x];

				/* get pattern of pixels for that character scanline */
				gfx = state->FNT[((chr & 0x7f)<<4) | ((ra & 8) >> 3) | ((ra & 7) << 1)] ^ ((chr & 0x80) ? 0xff : 0);

				/* Display a scanline of a character (8 pixels) */
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
		ma+=32;
	}
	return 0;
}

VIDEO_UPDATE( super80e )
{
	super80_state *state = screen->machine->driver_data<super80_state>();
	UINT8 y,ra,chr=32,gfx,screen_on=0;
	UINT16 sy=0,ma=state->vidpg,x;
	UINT8 *RAM = screen->machine->region("maincpu")->base();

	output_set_value("cass_led",(state->super80_shared & 0x20) ? 1 : 0);

	if ((state->super80_shared & 4) || (!(input_port_read(screen->machine, "CONFIG") & 4)))	/* bit 2 of port F0 is high, OR user turned on config switch */
		screen_on++;

	for (y = 0; y < 16; y++)
	{
		for (ra = 0; ra < 10; ra++)
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = 0; x < 32; x++)
			{
				if (screen_on)
					chr = RAM[ma | x];

				/* get pattern of pixels for that character scanline */
				gfx = state->FNT[(chr<<4) | ((ra & 8) >> 3) | ((ra & 7) << 1)];

				/* Display a scanline of a character (8 pixels) */
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
		ma+=32;
	}
	return 0;
}

VIDEO_UPDATE( super80m )
{
	super80_state *state = screen->machine->driver_data<super80_state>();
	UINT8 y,ra,chr=32,gfx,screen_on=0;
	UINT16 sy=0,ma=state->vidpg,x;
	UINT8 col, bg=0, fg=0, options=input_port_read(screen->machine, "CONFIG");
	UINT8 *RAM = screen->machine->region("maincpu")->base();

	/* get selected character generator */
	UINT8 cgen = state->current_charset ^ ((options & 0x10)>>4);	/* bit 0 of port F1 and cgen config switch */

	output_set_value("cass_led",(state->super80_shared & 0x20) ? 1 : 0);

	if ((state->super80_shared & 4) || (!(options & 4)))	/* bit 2 of port F0 is high, OR user turned on config switch */
		screen_on++;

	if (screen_on)
	{
		if ((options & 0x60) == 0x60)
			fg = 15;	/* b&w */
		else
			fg = 5;		/* green */
	}

	for (y = 0; y < 16; y++)
	{
		for (ra = 0; ra < 10; ra++)
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = 0; x < 32; x++)
			{
				if (screen_on)
					chr = RAM[ma | x];

				if (!(options & 0x40))
				{
					col = RAM[0xfe00 | ma | x];	/* byte of colour to display */
					fg = col & 0x0f;
					bg = (col & 0xf0) >> 4;
				}

				/* get pattern of pixels for that character scanline */
				if (cgen)
					gfx = state->FNT[(chr<<4) | ((ra & 8) >> 3) | ((ra & 7) << 1)];
				else
					gfx = state->FNT[0x1000 | ((chr & 0x7f)<<4) | ((ra & 8) >> 3) | ((ra & 7) << 1)] ^ ((chr & 0x80) ? 0xff : 0);

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
		ma+=32;
	}
	return 0;
}

VIDEO_START( super80 )
{
	super80_state *state = machine->driver_data<super80_state>();
	state->vidpg = 0xfe00;
	state->FNT = machine->region("gfx1")->base();
}

/**************************** I/O PORTS *****************************************************************/

WRITE8_HANDLER( super80_f1_w )
{
	super80_state *state = space->machine->driver_data<super80_state>();
	state->vidpg = (data & 0xfe) << 8;
	state->current_charset = data & 1;
}

/*---------------------------------------------------------------

    Super-80R and Super-80V

---------------------------------------------------------------*/

static const UINT8 mc6845_mask[32]={0xff,0xff,0xff,0x0f,0x7f,0x1f,0x7f,0x7f,3,0x1f,0x7f,0x1f,0x3f,0xff,0x3f,0xff,0,0};

READ8_HANDLER( super80v_low_r )
{
	super80_state *state = space->machine->driver_data<super80_state>();

	if (state->super80_shared & 4)
		return state->videoram[offset];
	else
		return state->colorram[offset];
}

WRITE8_HANDLER( super80v_low_w )
{
	super80_state *state = space->machine->driver_data<super80_state>();
	
	if (state->super80_shared & 4)
		state->videoram[offset] = data;
	else
		state->colorram[offset] = data;
}

READ8_HANDLER( super80v_high_r )
{
	super80_state *state = space->machine->driver_data<super80_state>();

	if (~state->super80_shared & 4)
		return state->colorram[0x800 | offset];
	else
	if (state->super80_shared & 0x10)
		return state->pcgram[0x800 | offset];
	else
		return state->pcgram[offset];
}

WRITE8_HANDLER( super80v_high_w )
{
	super80_state *state = space->machine->driver_data<super80_state>();

	if (~state->super80_shared & 4)
		state->colorram[0x800 | offset] = data;
	else
	{
		state->videoram[0x800 | offset] = data;

		if (state->super80_shared & 0x10)
			state->pcgram[0x800 | offset] = data;
	}
}

/* The 6845 can produce a variety of cursor shapes - all are emulated here - remove when mame fixed */
static void mc6845_cursor_configure(super80_state *state)
{
	UINT8 i,curs_type=0,r9,r10,r11;

	/* curs_type holds the general cursor shape to be created
        0 = no cursor
        1 = partial cursor (only shows on a block of scan lines)
        2 = full cursor
        3 = two-part cursor (has a part at the top and bottom with the middle blank) */

	for ( i = 0; i < ARRAY_LENGTH(state->mc6845_cursor); i++) state->mc6845_cursor[i] = 0;		// prepare cursor by erasing old one

	r9  = state->mc6845_reg[9];					// number of scan lines - 1
	r10 = state->mc6845_reg[10] & 0x1f;				// cursor start line = last 5 bits
	r11 = state->mc6845_reg[11]+1;					// cursor end line incremented to suit for-loops below

	/* decide the curs_type by examining the registers */
	if (r10 < r11) curs_type=1;				// start less than end, show start to end
	else
	if (r10 == r11) curs_type=2;				// if equal, show full cursor
	else curs_type=3;					// if start greater than end, it's a two-part cursor

	if ((r11 - 1) > r9) curs_type=2;			// if end greater than scan-lines, show full cursor
	if (r10 > r9) curs_type=0;				// if start greater than scan-lines, then no cursor
	if (r11 > 16) r11=16;					// truncate 5-bit register to fit our 4-bit hardware

	/* create the new cursor */
	if (curs_type > 1) for (i = 0;i < ARRAY_LENGTH(state->mc6845_cursor);i++) state->mc6845_cursor[i]=0xff; // turn on full cursor

	if (curs_type == 1) for (i = r10;i < r11;i++) state->mc6845_cursor[i]=0xff; // for each line that should show, turn on that scan line

	if (curs_type == 3) for (i = r11; i < r10;i++) state->mc6845_cursor[i]=0; // now take a bite out of the middle
}

VIDEO_START( super80v )
{
	super80_state *state = machine->driver_data<super80_state>();
	state->mc6845 = machine->device("crtc");
	state->pcgram = machine->region("maincpu")->base()+0xf000;
	state->videoram = machine->region("videoram")->base();
	state->colorram = machine->region("colorram")->base();
}

VIDEO_UPDATE( super80v )
{
	super80_state *state = screen->machine->driver_data<super80_state>();
	state->framecnt++;
	state->speed = state->mc6845_reg[10]&0x20, state->flash = state->mc6845_reg[10]&0x40;			// cursor modes
	state->cursor = (state->mc6845_reg[14]<<8) | state->mc6845_reg[15];					// get cursor position
	state->s_options=input_port_read(screen->machine, "CONFIG");
	output_set_value("cass_led",(state->super80_shared & 0x20) ? 1 : 0);
	mc6845_update(state->mc6845, bitmap, cliprect);
	return 0;
}

MC6845_UPDATE_ROW( super80v_update_row )
{
	super80_state *state = device->machine->driver_data<super80_state>();
	UINT8 chr,col,gfx,fg,bg=0;
	UINT16 mem,x;
	UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);

	for (x = 0; x < x_count; x++)				// for each character
	{
		UINT8 inv=0;
		//      if (x == cursor_x) inv=0xff;    /* uncomment when mame fixed */
		mem = (ma + x) & 0xfff;
		chr = state->videoram[mem];

		/* get colour or b&w */
		fg = 5;						/* green */
		if ((state->s_options & 0x60) == 0x60) fg = 15;		/* b&w */

		if (~state->s_options & 0x40)
		{
			col = state->colorram[mem];					/* byte of colour to display */
			fg = col & 0x0f;
			bg = (col & 0xf0) >> 4;
		}

		/* if inverse mode, replace any pcgram chrs with inverse chrs */
		if ((~state->super80_shared & 0x10) && (chr & 0x80))			// is it a high chr in inverse mode
		{
			inv ^= 0xff;						// invert the chr
			chr &= 0x7f;						// and drop bit 7
		}

		/* process cursor - remove when mame fixed */
		if ((((!state->flash) && (!state->speed)) ||
			((state->flash) && (state->speed) && (state->framecnt & 0x10)) ||
			((state->flash) && (!state->speed) && (state->framecnt & 8))) &&
			(mem == state->cursor))
				inv ^= state->mc6845_cursor[ra];

		/* get pattern of pixels for that character scanline */
		gfx = state->pcgram[(chr<<4) | ra] ^ inv;

		/* Display a scanline of a character (7 pixels) */
		*p++ = ( gfx & 0x80 ) ? fg : bg;
		*p++ = ( gfx & 0x40 ) ? fg : bg;
		*p++ = ( gfx & 0x20 ) ? fg : bg;
		*p++ = ( gfx & 0x10 ) ? fg : bg;
		*p++ = ( gfx & 0x08 ) ? fg : bg;
		*p++ = ( gfx & 0x04 ) ? fg : bg;
		*p++ = ( gfx & 0x02 ) ? fg : bg;
	}
}

/**************************** I/O PORTS *****************************************************************/

WRITE8_HANDLER( super80v_10_w )
{
	super80_state *state = space->machine->driver_data<super80_state>();
	data &= 0x1f;
	state->mc6845_ind = data;
	mc6845_address_w( state->mc6845, 0, data );
}

WRITE8_HANDLER( super80v_11_w )
{
	super80_state *state = space->machine->driver_data<super80_state>();
	state->mc6845_reg[state->mc6845_ind] = data & mc6845_mask[state->mc6845_ind];	/* save data in register */
	mc6845_register_w( state->mc6845, 0, data );
	if ((state->mc6845_ind > 8) && (state->mc6845_ind < 12)) mc6845_cursor_configure(state);		/* adjust cursor shape - remove when mame fixed */
}
