/***************************************************************************
    microbee.c

    video hardware
    Juergen Buchmueller <pullmoll@t-online.de>, Dec 1999

    Tests of keyboard. Start mbeeic.

    1. Load ASTEROIDS PLUS, stay in attract mode, hold down spacebar,
        it should only fire bullets. If it sometimes starts turning,
        thrusting or using the shield, then there is a problem.

    2. Load SCAVENGER and make sure it doesn't go to the next level
        until you find the Exit.

    3. At the Basic prompt, type in EDASM press enter. At the memory size
        prompt press enter. Now, make sure the keyboard works properly.



    TODO:

    1. mbeeppc keyboard response is quite slow. You need to hold each
    key until it responds. It works much better if you overclock the cpu.

****************************************************************************/

#include "emu.h"
#include "includes/mbee.h"

static void sy6545_cursor_configure(mbee_state *state);
static const UINT8 sy6545_mask[32]={0xff,0xff,0xff,0x0f,0x7f,0x1f,0x7f,0x7f,3,0x1f,0x7f,0x1f,0x3f,0xff,0x3f,0xff,0,0,0x3f,0xff};



/***********************************************************

    Handlers of video, colour, and attribute RAM

************************************************************/

READ8_HANDLER( mbee_low_r )
{
	mbee_state *state = space->machine().driver_data<mbee_state>();
	if (state->m_0b & 1)
		return state->m_gfxram[offset];
	else
		return state->m_videoram[offset];
}

WRITE8_HANDLER( mbee_low_w )
{
	mbee_state *state = space->machine().driver_data<mbee_state>();
	state->m_videoram[offset] = data;
}

READ8_HANDLER( mbee_high_r )
{
	mbee_state *state = space->machine().driver_data<mbee_state>();
	return state->m_gfxram[0x800 | offset];
}

WRITE8_HANDLER( mbee_high_w )
{
	mbee_state *state = space->machine().driver_data<mbee_state>();
	state->m_gfxram[0x800 | offset] = data;
}

READ8_HANDLER ( mbee_0b_r )
{
	mbee_state *state = space->machine().driver_data<mbee_state>();
	return state->m_0b;
}

WRITE8_HANDLER ( mbee_0b_w )
{
	mbee_state *state = space->machine().driver_data<mbee_state>();
	state->m_0b = data;
}

READ8_HANDLER ( mbeeic_08_r )
{
	mbee_state *state = space->machine().driver_data<mbee_state>();
	return state->m_08;
}

WRITE8_HANDLER ( mbeeic_08_w )
{
	mbee_state *state = space->machine().driver_data<mbee_state>();
	state->m_08 = data;
}

READ8_HANDLER( mbeeic_high_r )
{
	mbee_state *state = space->machine().driver_data<mbee_state>();
	if (state->m_08 & 0x40)
		return state->m_colorram[offset];
	else
		return state->m_gfxram[0x800 | offset];
}

WRITE8_HANDLER ( mbeeic_high_w )
{
	mbee_state *state = space->machine().driver_data<mbee_state>();
	if ((state->m_08 & 0x40) && (~state->m_0b & 1))
		state->m_colorram[offset] = data;
	else
		state->m_gfxram[0x0800 | offset] = data;
}

READ8_HANDLER ( mbeeppc_1c_r )
{
	mbee_state *state = space->machine().driver_data<mbee_state>();
	return state->m_1c;
}

WRITE8_HANDLER( mbeeppc_1c_w )
{
	mbee_state *state = space->machine().driver_data<mbee_state>();
/*  d7 extended graphics (1=allow attributes and pcg banks)
    d5 bankswitch basic rom
    d4 select attribute ram
    d3..d0 select state->m_videoram bank */

	state->m_1c = data;
	memory_set_bank(space->machine(), "basic", (data & 0x20) ? 1 : 0);
}

WRITE8_HANDLER( mbee256_1c_w )
{
	mbee_state *state = space->machine().driver_data<mbee_state>();
/*  d7 extended graphics (1=allow attributes and pcg banks)
    d5 bankswitch basic rom
    d4 select attribute ram
    d3..d0 select state->m_videoram bank */

	state->m_1c = data;
}

READ8_HANDLER( mbeeppc_low_r )
{
	mbee_state *state = space->machine().driver_data<mbee_state>();
	if ((state->m_1c & 0x1f) == 0x10)
		return state->m_attribram[offset];
	else
	if (state->m_0b & 1)
		return state->m_gfxram[offset];
	else
		return state->m_videoram[offset];
}

WRITE8_HANDLER( mbeeppc_low_w )
{
	mbee_state *state = space->machine().driver_data<mbee_state>();
	if (state->m_1c & 16)
		state->m_attribram[offset] = data;
	else
		state->m_videoram[offset] = data;
}

READ8_HANDLER( mbeeppc_high_r )
{
	mbee_state *state = space->machine().driver_data<mbee_state>();
	if (state->m_08 & 0x40)
		return state->m_colorram[offset];
	else
		return state->m_gfxram[(((state->m_1c & 15) + 1) << 11) | offset];
}

WRITE8_HANDLER ( mbeeppc_high_w )
{
	mbee_state *state = space->machine().driver_data<mbee_state>();
	if ((state->m_08 & 0x40) && (~state->m_0b & 1))
		state->m_colorram[offset] = data;
	else
		state->m_gfxram[(((state->m_1c & 15) + 1) << 11) | offset] = data;
}


/***********************************************************

    CRTC-driven keyboard

************************************************************/

static const char *const keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7" };

/* The direction keys are used by the pc85 menu. Do not know what uses the "insert" key. */
static void keyboard_matrix_r(running_machine &machine, int offs)
{
	mbee_state *state = machine.driver_data<mbee_state>();
	UINT8 port = (offs >> 7) & 7;
	UINT8 bit = (offs >> 4) & 7;
	UINT8 data = (input_port_read(machine, keynames[port]) >> bit) & 1;

	if ((data | state->m_is_premium) == 0)
	{
		UINT8 extra = input_port_read(machine, "EXTRA");

		if( extra & 0x01 )	/* extra: cursor up */
		{
			if( port == 7 && bit == 1 ) data = 1;	/* Control */
			if( port == 0 && bit == 5 ) data = 1;	/* E */
		}
		else
		if( extra & 0x02 )	/* extra: cursor down */
		{
			if( port == 7 && bit == 1 ) data = 1;	/* Control */
			if( port == 3 && bit == 0 ) data = 1;	/* X */
		}
		else
		if( extra & 0x04 )	/* extra: cursor left */
		{
			if( port == 7 && bit == 1 ) data = 1;	/* Control */
			if( port == 2 && bit == 3 ) data = 1;	/* S */
		}
		else
		if( extra & 0x08 )	/* extra: cursor right */
		{
			if( port == 7 && bit == 1 ) data = 1;	/* Control */
			if( port == 0 && bit == 4 ) data = 1;	/* D */
		}
#if 0
		// this key doesn't appear on any keyboard afaik
		else
		if( extra & 0x10 )	/* extra: insert */
		{
			if( port == 7 && bit == 1 ) data = 1;	/* Control */
			if( port == 2 && bit == 6 ) data = 1;	/* V */
		}
#endif
	}

	if( data )
	{
		state->m_sy6545_reg[17] = offs;
		state->m_sy6545_reg[16] = (offs >> 8) & 0x3f;
		state->m_sy6545_status |= 0x40; //lpen_strobe
	}
}


static void mbee_video_kbd_scan( running_machine &machine, int param )
{
	mbee_state *state = machine.driver_data<mbee_state>();
	if (state->m_0b) return;

	keyboard_matrix_r(machine, param);
}


/***********************************************************

    CRTC registers

************************************************************/

READ8_HANDLER ( m6545_status_r )
{
	mbee_state *state = space->machine().driver_data<mbee_state>();
	screen_device *screen = space->machine().first_screen();
	const rectangle &visarea = screen->visible_area();

	UINT8 data = state->m_sy6545_status; // bit 6 = lpen strobe, bit 7 = update strobe
	int y = space->machine().primary_screen->vpos();

	if( y < visarea.min_y || y > visarea.max_y )
		data |= 0x20;	/* vertical blanking */

	return data;
}

READ8_HANDLER ( m6545_data_r )
{
	mbee_state *state = space->machine().driver_data<mbee_state>();
	UINT16 addr;
	UINT8 data = mc6845_register_r(state->m_mc6845, 0);

	switch( state->m_sy6545_ind )
	{
	case 16:
	case 17:
		state->m_sy6545_status &= 0x80; // turn off lpen_strobe
		break;
	case 31:
		/* This firstly pushes the contents of the transparent registers onto the MA lines,
        then increments the address, then sets update strobe on. */
		addr = (state->m_sy6545_reg[18] << 8) | state->m_sy6545_reg[19];
		keyboard_matrix_r(space->machine(), addr);
		state->m_sy6545_reg[19]++;
		if (!state->m_sy6545_reg[19]) state->m_sy6545_reg[18]++;
		state->m_sy6545_status |= 0x80; // update_strobe
		break;
	}
	return data;
}

WRITE8_HANDLER ( m6545_index_w )
{
	mbee_state *state = space->machine().driver_data<mbee_state>();
	data &= 0x1f;
	state->m_sy6545_ind = data;
	mc6845_address_w( state->m_mc6845, 0, data );
}

WRITE8_HANDLER ( m6545_data_w )
{
	mbee_state *state = space->machine().driver_data<mbee_state>();
	int addr = 0;

	switch( state->m_sy6545_ind )
	{
	case 12:
		data &= 0x3f; // select alternate character set
		if( state->m_sy6545_reg[12] != data )
			memcpy(state->m_gfxram, space->machine().region("gfx")->base() + (((data & 0x30) == 0x20) << 11), 0x800);
		break;
	case 31:
		/* This firstly pushes the contents of the transparent registers onto the MA lines,
        then increments the address, then sets update strobe on. */
		addr = (state->m_sy6545_reg[18] << 8) | state->m_sy6545_reg[19];
		keyboard_matrix_r(space->machine(), addr);
		state->m_sy6545_reg[19]++;
		if (!state->m_sy6545_reg[19]) state->m_sy6545_reg[18]++;
		state->m_sy6545_status |= 0x80; // update_strobe
		break;
	}
	state->m_sy6545_reg[state->m_sy6545_ind] = data & sy6545_mask[state->m_sy6545_ind];	/* save data in register */
	mc6845_register_w( state->m_mc6845, 0, data );
	if ((state->m_sy6545_ind > 8) && (state->m_sy6545_ind < 12)) sy6545_cursor_configure(state);		/* adjust cursor shape - remove when mame fixed */
}



/***********************************************************

    The 6845 can produce a variety of cursor shapes - all
    are emulated here.

    Need to find out if the 6545 works the same way.

************************************************************/

static void sy6545_cursor_configure(mbee_state *state)
{
	UINT8 i,curs_type=0,r9,r10,r11;

	/* curs_type holds the general cursor shape to be created
        0 = no cursor
        1 = partial cursor (only shows on a block of scan lines)
        2 = full cursor
        3 = two-part cursor (has a part at the top and bottom with the middle blank) */

	for ( i = 0; i < ARRAY_LENGTH(state->m_sy6545_cursor); i++) state->m_sy6545_cursor[i] = 0;		// prepare cursor by erasing old one

	r9  = state->m_sy6545_reg[9];					// number of scan lines - 1
	r10 = state->m_sy6545_reg[10] & 0x1f;				// cursor start line = last 5 bits
	r11 = state->m_sy6545_reg[11]+1;					// cursor end line incremented to suit for-loops below

	/* decide the curs_type by examining the registers */
	if (r10 < r11) curs_type=1;				// start less than end, show start to end
	else
	if (r10 == r11) curs_type=2;				// if equal, show full cursor
	else curs_type=3;					// if start greater than end, it's a two-part cursor

	if ((r11 - 1) > r9) curs_type=2;			// if end greater than scan-lines, show full cursor
	if (r10 > r9) curs_type=0;				// if start greater than scan-lines, then no cursor
	if (r11 > 16) r11=16;					// truncate 5-bit register to fit our 4-bit hardware

	/* create the new cursor */
	if (curs_type > 1) for (i = 0;i < ARRAY_LENGTH(state->m_sy6545_cursor);i++) state->m_sy6545_cursor[i]=0xff; // turn on full cursor

	if (curs_type == 1) for (i = r10;i < r11;i++) state->m_sy6545_cursor[i]=0xff; // for each line that should show, turn on that scan line

	if (curs_type == 3) for (i = r11; i < r10;i++) state->m_sy6545_cursor[i]=0; // now take a bite out of the middle
}




/***********************************************************

    Video

************************************************************/

VIDEO_START( mbee )
{
	mbee_state *state = machine.driver_data<mbee_state>();
	state->m_mc6845 = machine.device("crtc");
	state->m_videoram = machine.region("videoram")->base();
	state->m_gfxram = machine.region("gfx")->base()+0x1000;
	state->m_is_premium = 0;
}

VIDEO_START( mbeeic )
{
	mbee_state *state = machine.driver_data<mbee_state>();
	state->m_mc6845 = machine.device("crtc");
	state->m_videoram = machine.region("videoram")->base();
	state->m_colorram = machine.region("colorram")->base();
	state->m_gfxram = machine.region("gfx")->base()+0x1000;
	state->m_is_premium = 0;
}

VIDEO_START( mbeeppc )
{
	mbee_state *state = machine.driver_data<mbee_state>();
	state->m_mc6845 = machine.device("crtc");
	state->m_videoram = machine.region("videoram")->base();
	state->m_colorram = machine.region("colorram")->base();
	state->m_gfxram = machine.region("gfx")->base()+0x1000;
	state->m_attribram = machine.region("attrib")->base();
	state->m_is_premium = 1;
}

SCREEN_UPDATE( mbee )
{
	mbee_state *state = screen->machine().driver_data<mbee_state>();
	state->m_framecnt++;
	state->m_speed = state->m_sy6545_reg[10]&0x20, state->m_flash = state->m_sy6545_reg[10]&0x40;			// cursor modes
	state->m_cursor = (state->m_sy6545_reg[14]<<8) | state->m_sy6545_reg[15];					// get cursor position
	mc6845_update(state->m_mc6845, bitmap, cliprect);
	return 0;
}


MC6845_ON_UPDATE_ADDR_CHANGED( mbee_update_addr )
{
/* not sure what goes in here - parameters passed are device, address, strobe */
}

MC6845_ON_UPDATE_ADDR_CHANGED( mbee256_update_addr )
{
/* not used on 256TC */
}


/* monochrome bee */
MC6845_UPDATE_ROW( mbee_update_row )
{
	mbee_state *state = device->machine().driver_data<mbee_state>();
	UINT8 chr,gfx;
	UINT16 mem,x;
	UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);

	for (x = 0; x < x_count; x++)			// for each character
	{
		UINT8 inv=0;
		mem = (ma + x) & 0x7ff;
		chr = state->m_videoram[mem];

		mbee_video_kbd_scan(device->machine(), x+ma);

		/* process cursor */
		if ((((!state->m_flash) && (!state->m_speed)) ||					// (5,6)=(0,0) = cursor on always
			((state->m_flash) && (state->m_speed) && (state->m_framecnt & 0x10)) ||		// (5,6)=(1,1) = cycle per 32 frames
			((state->m_flash) && (!state->m_speed) && (state->m_framecnt & 8))) &&		// (5,6)=(0,1) = cycle per 16 frames
			(mem == state->m_cursor))					// displaying at cursor position?
				inv ^= state->m_sy6545_cursor[ra];			// cursor scan row

		/* get pattern of pixels for that character scanline */
		gfx = state->m_gfxram[(chr<<4) | ra] ^ inv;

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

/* prom-based colours */
MC6845_UPDATE_ROW( mbeeic_update_row )
{
	mbee_state *state = device->machine().driver_data<mbee_state>();
	UINT8 chr,gfx,fg,bg;
	UINT16 mem,x,col;
	UINT16 colourm = (state->m_08 & 0x0e) << 7;
	UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);

	for (x = 0; x < x_count; x++)			// for each character
	{
		UINT8 inv=0;
		mem = (ma + x) & 0x7ff;
		chr = state->m_videoram[mem];
		col = state->m_colorram[mem] | colourm;					// read a byte of colour

		mbee_video_kbd_scan(device->machine(), x+ma);

		/* process cursor */
		if ((((!state->m_flash) && (!state->m_speed)) ||					// (5,6)=(0,0) = cursor on always
			((state->m_flash) && (state->m_speed) && (state->m_framecnt & 0x10)) ||		// (5,6)=(1,1) = cycle per 32 frames
			((state->m_flash) && (!state->m_speed) && (state->m_framecnt & 8))) &&		// (5,6)=(0,1) = cycle per 16 frames
			(mem == state->m_cursor))					// displaying at cursor position?
				inv ^= state->m_sy6545_cursor[ra];			// cursor scan row

		/* get pattern of pixels for that character scanline */
		gfx = state->m_gfxram[(chr<<4) | ra] ^ inv;
		fg = (col & 0x001f) | 64;					// map to foreground palette
		bg = (col & 0x07e0) >> 5;					// and background palette

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


/* new colours & hires2 */
MC6845_UPDATE_ROW( mbeeppc_update_row )
{
	mbee_state *state = device->machine().driver_data<mbee_state>();
	UINT8 gfx,fg,bg;
	UINT16 mem,x,col,chr;
	UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);

	for (x = 0; x < x_count; x++)			// for each character
	{
		UINT8 inv=0;
		mem = (ma + x) & 0x7ff;
		chr = state->m_videoram[mem];
		col = state->m_colorram[mem];						// read a byte of colour

		if (state->m_1c & 0x80)						// are extended features enabled?
		{
			UINT8 attr = state->m_attribram[mem];

			if (chr & 0x80)
				chr += ((attr & 15) << 7);			// bump chr to its particular pcg definition

			if (attr & 0x40)
				inv ^= 0xff;					// inverse attribute

			if ((attr & 0x80) && (state->m_framecnt & 0x10))			// flashing attribute
				chr = 0x20;
		}

		mbee_video_kbd_scan(device->machine(), x+ma);

		/* process cursor */
		if ((((!state->m_flash) && (!state->m_speed)) ||					// (5,6)=(0,0) = cursor on always
			((state->m_flash) && (state->m_speed) && (state->m_framecnt & 0x10)) ||		// (5,6)=(1,1) = cycle per 32 frames
			((state->m_flash) && (!state->m_speed) && (state->m_framecnt & 8))) &&		// (5,6)=(0,1) = cycle per 16 frames
			(mem == state->m_cursor))					// displaying at cursor position?
				inv ^= state->m_sy6545_cursor[ra];			// cursor scan row

		/* get pattern of pixels for that character scanline */
		gfx = state->m_gfxram[(chr<<4) | ra] ^ inv;
		fg = col & 15;							// map to foreground palette
		bg = (col & 0xf0) >> 4;						// and background palette

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


/***********************************************************

    Palette

************************************************************/

PALETTE_INIT( mbeeic )
{
	UINT16 i;
	UINT8 r, b, g, k;
	UINT8 level[] = { 0, 0x80, 0xff, 0xff };	/* off, half, full intensity */

	/* set up background palette (00-63) */
	for (i = 0; i < 64; i++)
	{
		r = level[((i>>0)&1)|((i>>2)&2)];
		g = level[((i>>1)&1)|((i>>3)&2)];
		b = level[((i>>2)&1)|((i>>4)&2)];
		palette_set_color(machine, i, MAKE_RGB(r, g, b));
	}

	/* set up foreground palette (64-95) by reading the prom */
	for (i = 0; i < 32; i++)
	{
		k = color_prom[i];
		r = level[((k>>2)&1)|((k>>4)&2)];
		g = level[((k>>1)&1)|((k>>3)&2)];
		b = level[((k>>0)&1)|((k>>2)&2)];
		palette_set_color(machine, i|64, MAKE_RGB(r, g, b));
	}
}


PALETTE_INIT( mbeepc85b )
{
	UINT16 i;
	UINT8 r, b, g, k;
	UINT8 level[] = { 0, 0x80, 0x80, 0xff };	/* off, half, full intensity */

	/* set up background palette (00-63) */
	for (i = 0; i < 64; i++)
	{
		r = level[((i>>0)&1)|((i>>2)&2)];
		g = level[((i>>1)&1)|((i>>3)&2)];
		b = level[((i>>2)&1)|((i>>4)&2)];
		palette_set_color(machine, i, MAKE_RGB(r, g, b));
	}

	level[2] = 0xff;

	/* set up foreground palette (64-95) by reading the prom */
	for (i = 0; i < 32; i++)
	{
		k = color_prom[i];
		r = level[((k>>2)&1)|((k>>4)&2)];
		g = level[((k>>1)&1)|((k>>3)&2)];
		b = level[((k>>0)&1)|((k>>2)&2)];
		palette_set_color(machine, i|64, MAKE_RGB(r, g, b));
	}
}


PALETTE_INIT( mbeeppc )
{
	UINT16 i;
	UINT8 r, b, g;

	/* set up 8 low intensity colours */
	for (i = 0; i < 8; i++)
	{
		r = (i & 1) ? 0xc0 : 0;
		g = (i & 2) ? 0xc0 : 0;
		b = (i & 4) ? 0xc0 : 0;
		palette_set_color(machine, i, MAKE_RGB(r, g, b));
	}

	/* set up 8 high intensity colours */
	for (i = 9; i < 16; i++)
	{
		r = (i & 1) ? 0xff : 0;
		g = (i & 2) ? 0xff : 0;
		b = (i & 4) ? 0xff : 0;
		palette_set_color(machine, i, MAKE_RGB(r, g, b));
	}
}

