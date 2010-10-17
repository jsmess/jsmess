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

static UINT8 framecnt = 0;
static UINT8 *gfxram;
static UINT8 *colorram;
static UINT8 *videoram;
static UINT8 *attribram;
static UINT8 mbee_08;
static UINT8 mbee_0b;
static UINT8 mbee_1c;
static UINT8 is_premium;
static UINT8 sy6545_cursor[16];
static UINT8 sy6545_status = 0;
static void sy6545_cursor_configure(void);
static running_device *mc6845;
static UINT8 speed,flash;
static UINT16 cursor;
static UINT8 sy6545_reg[20];				/* registers */
static UINT8 sy6545_ind;				/* register index */
static const UINT8 sy6545_mask[32]={0xff,0xff,0xff,0x0f,0x7f,0x1f,0x7f,0x7f,3,0x1f,0x7f,0x1f,0x3f,0xff,0x3f,0xff,0,0,0x3f,0xff};



/***********************************************************

    Handlers of video, colour, and attribute RAM

************************************************************/

READ8_HANDLER( mbee_low_r )
{
	if (mbee_0b & 1)
		return gfxram[offset];
	else
		return videoram[offset];
}

WRITE8_HANDLER( mbee_low_w )
{
	videoram[offset] = data;
}

READ8_HANDLER( mbee_high_r )
{
	return gfxram[0x800 | offset];
}

WRITE8_HANDLER( mbee_high_w )
{
	gfxram[0x800 | offset] = data;
}

READ8_HANDLER ( mbee_0b_r )
{
	return mbee_0b;
}

WRITE8_HANDLER ( mbee_0b_w )
{
	mbee_0b = data;
}

READ8_HANDLER ( mbeeic_08_r )
{
	return mbee_08;
}

WRITE8_HANDLER ( mbeeic_08_w )
{
	mbee_08 = data;
}

READ8_HANDLER( mbeeic_high_r )
{
	if (mbee_08 & 0x40)
		return colorram[offset];
	else
		return gfxram[0x800 | offset];
}

WRITE8_HANDLER ( mbeeic_high_w )
{
	if ((mbee_08 & 0x40) && (~mbee_0b & 1))
		colorram[offset] = data;
	else
		gfxram[0x0800 | offset] = data;
}

READ8_HANDLER ( mbeeppc_1c_r )
{
	return mbee_1c;
}

WRITE8_HANDLER( mbeeppc_1c_w )
{
/*  d7 extended graphics (1=allow attributes and pcg banks)
    d5 bankswitch basic rom
    d4 select attribute ram
    d3..d0 select videoram bank */

	mbee_1c = data;
	memory_set_bank(space->machine, "basic", (data & 0x20) ? 1 : 0);
}

WRITE8_HANDLER( mbee256_1c_w )
{
/*  d7 extended graphics (1=allow attributes and pcg banks)
    d5 bankswitch basic rom
    d4 select attribute ram
    d3..d0 select videoram bank */

	mbee_1c = data;
}

READ8_HANDLER( mbeeppc_low_r )
{
	if (mbee_1c & 16)
		return attribram[offset];
	else
	if (mbee_0b & 1)
		return gfxram[offset];
	else
		return videoram[offset];
}

WRITE8_HANDLER( mbeeppc_low_w )
{
	if (mbee_1c & 16)
		attribram[offset] = data;
	else
		videoram[offset] = data;
}

READ8_HANDLER( mbeeppc_high_r )
{
	if (mbee_08 & 0x40)
		return colorram[offset];
	else
		return gfxram[(((mbee_1c & 15) + 1) << 11) | offset];
}

WRITE8_HANDLER ( mbeeppc_high_w )
{
	if ((mbee_08 & 0x40) && (~mbee_0b & 1))
		colorram[offset] = data;
	else
		gfxram[(((mbee_1c & 15) + 1) << 11) | offset] = data;
}


/***********************************************************

    CRTC-driven keyboard

************************************************************/

static const char *const keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7" };

/* The direction keys are used by the pc85 menu. Do not know what uses the "insert" key. */
static void keyboard_matrix_r(running_machine *machine, int offs)
{
	UINT8 port = (offs >> 7) & 7;
	UINT8 bit = (offs >> 4) & 7;
	UINT8 data = (input_port_read(machine, keynames[port]) >> bit) & 1;

	if ((data | is_premium) == 0)
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
		sy6545_reg[17] = offs;
		sy6545_reg[16] = (offs >> 8) & 0x3f;
		sy6545_status |= 0x40; //lpen_strobe
	}
}


static void mbee_video_kbd_scan( running_machine *machine, int param )
{
	if (mbee_0b) return;

	keyboard_matrix_r(machine, param);
}


/***********************************************************

    CRTC registers

************************************************************/

READ8_HANDLER ( m6545_status_r )
{
	screen_device *screen = screen_first(*space->machine);
	const rectangle &visarea = screen->visible_area();

	UINT8 data = sy6545_status; // bit 6 = lpen strobe, bit 7 = update strobe
	int y = space->machine->primary_screen->vpos();

	if( y < visarea.min_y || y > visarea.max_y )
		data |= 0x20;	/* vertical blanking */

	return data;
}

READ8_HANDLER ( m6545_data_r )
{
	UINT16 addr;
	UINT8 data = mc6845_register_r(mc6845, 0);

	switch( sy6545_ind )
	{
	case 16:
	case 17:
		sy6545_status &= 0x80; // turn off lpen_strobe
		break;
	case 31:
		/* This firstly pushes the contents of the transparent registers onto the MA lines,
		then increments the address, then sets update strobe on. */
		addr = (sy6545_reg[18] << 8) | sy6545_reg[19];
		keyboard_matrix_r(space->machine, addr);
		sy6545_reg[19]++;
		if (!sy6545_reg[19]) sy6545_reg[18]++;
		sy6545_status |= 0x80; // update_strobe
		break;
	}
	return data;
}

WRITE8_HANDLER ( m6545_index_w )
{
	data &= 0x1f;
	sy6545_ind = data;
	mc6845_address_w( mc6845, 0, data );
}

WRITE8_HANDLER ( m6545_data_w )
{
	int addr = 0;

	switch( sy6545_ind )
	{
	case 12:
		data &= 0x3f; // select alternate character set
		if( sy6545_reg[12] != data )
			memcpy(gfxram, memory_region(space->machine, "gfx") + (((data & 0x30) == 0x20) << 11), 0x800);
		break;
	case 31:
		/* This firstly pushes the contents of the transparent registers onto the MA lines,
		then increments the address, then sets update strobe on. */
		addr = (sy6545_reg[18] << 8) | sy6545_reg[19];
		keyboard_matrix_r(space->machine, addr);
		sy6545_reg[19]++;
		if (!sy6545_reg[19]) sy6545_reg[18]++;
		sy6545_status |= 0x80; // update_strobe
		break;
	}
	sy6545_reg[sy6545_ind] = data & sy6545_mask[sy6545_ind];	/* save data in register */
	mc6845_register_w( mc6845, 0, data );
	if ((sy6545_ind > 8) && (sy6545_ind < 12)) sy6545_cursor_configure();		/* adjust cursor shape - remove when mame fixed */
}



/***********************************************************

    The 6845 can produce a variety of cursor shapes - all
    are emulated here.

    Need to find out if the 6545 works the same way.

************************************************************/

static void sy6545_cursor_configure(void)
{
	UINT8 i,curs_type=0,r9,r10,r11;

	/* curs_type holds the general cursor shape to be created
        0 = no cursor
        1 = partial cursor (only shows on a block of scan lines)
        2 = full cursor
        3 = two-part cursor (has a part at the top and bottom with the middle blank) */

	for ( i = 0; i < ARRAY_LENGTH(sy6545_cursor); i++) sy6545_cursor[i] = 0;		// prepare cursor by erasing old one

	r9  = sy6545_reg[9];					// number of scan lines - 1
	r10 = sy6545_reg[10] & 0x1f;				// cursor start line = last 5 bits
	r11 = sy6545_reg[11]+1;					// cursor end line incremented to suit for-loops below

	/* decide the curs_type by examining the registers */
	if (r10 < r11) curs_type=1;				// start less than end, show start to end
	else
	if (r10 == r11) curs_type=2;				// if equal, show full cursor
	else curs_type=3;					// if start greater than end, it's a two-part cursor

	if ((r11 - 1) > r9) curs_type=2;			// if end greater than scan-lines, show full cursor
	if (r10 > r9) curs_type=0;				// if start greater than scan-lines, then no cursor
	if (r11 > 16) r11=16;					// truncate 5-bit register to fit our 4-bit hardware

	/* create the new cursor */
	if (curs_type > 1) for (i = 0;i < ARRAY_LENGTH(sy6545_cursor);i++) sy6545_cursor[i]=0xff; // turn on full cursor

	if (curs_type == 1) for (i = r10;i < r11;i++) sy6545_cursor[i]=0xff; // for each line that should show, turn on that scan line

	if (curs_type == 3) for (i = r11; i < r10;i++) sy6545_cursor[i]=0; // now take a bite out of the middle
}




/***********************************************************

    Video

************************************************************/

VIDEO_START( mbee )
{
	mc6845 = machine->device("crtc");
	videoram = memory_region(machine, "videoram");
	gfxram = memory_region(machine, "gfx")+0x1000;
	is_premium = 0;
}

VIDEO_START( mbeeic )
{
	mc6845 = machine->device("crtc");
	videoram = memory_region(machine, "videoram");
	colorram = memory_region(machine, "colorram");
	gfxram = memory_region(machine, "gfx")+0x1000;
	is_premium = 0;
}

VIDEO_START( mbeeppc )
{
	mc6845 = machine->device("crtc");
	videoram = memory_region(machine, "videoram");
	colorram = memory_region(machine, "colorram");
	gfxram = memory_region(machine, "gfx")+0x1000;
	attribram = memory_region(machine, "attrib");
	is_premium = 1;
}

VIDEO_UPDATE( mbee )
{
	framecnt++;
	speed = sy6545_reg[10]&0x20, flash = sy6545_reg[10]&0x40;			// cursor modes
	cursor = (sy6545_reg[14]<<8) | sy6545_reg[15];					// get cursor position
	mc6845_update(mc6845, bitmap, cliprect);
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
	UINT8 chr,gfx;
	UINT16 mem,x;
	UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);

	for (x = 0; x < x_count; x++)			// for each character
	{
		UINT8 inv=0;
		mem = (ma + x) & 0x7ff;
		chr = videoram[mem];

		mbee_video_kbd_scan(device->machine, x+ma);

		/* process cursor */
		if ((((!flash) && (!speed)) ||					// (5,6)=(0,0) = cursor on always
			((flash) && (speed) && (framecnt & 0x10)) ||		// (5,6)=(1,1) = cycle per 32 frames
			((flash) && (!speed) && (framecnt & 8))) &&		// (5,6)=(0,1) = cycle per 16 frames
			(mem == cursor))					// displaying at cursor position?
				inv ^= sy6545_cursor[ra];			// cursor scan row

		/* get pattern of pixels for that character scanline */
		gfx = gfxram[(chr<<4) | ra] ^ inv;

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
	UINT8 chr,gfx,fg,bg;
	UINT16 mem,x,col;
	UINT16 colourm = (mbee_08 & 0x0e) << 7;
	UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);

	for (x = 0; x < x_count; x++)			// for each character
	{
		UINT8 inv=0;
		mem = (ma + x) & 0x7ff;
		chr = videoram[mem];
		col = colorram[mem] | colourm;					// read a byte of colour

		mbee_video_kbd_scan(device->machine, x+ma);

		/* process cursor */
		if ((((!flash) && (!speed)) ||					// (5,6)=(0,0) = cursor on always
			((flash) && (speed) && (framecnt & 0x10)) ||		// (5,6)=(1,1) = cycle per 32 frames
			((flash) && (!speed) && (framecnt & 8))) &&		// (5,6)=(0,1) = cycle per 16 frames
			(mem == cursor))					// displaying at cursor position?
				inv ^= sy6545_cursor[ra];			// cursor scan row

		/* get pattern of pixels for that character scanline */
		gfx = gfxram[(chr<<4) | ra] ^ inv;
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
	UINT8 gfx,fg,bg;
	UINT16 mem,x,col,chr;
	UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);

	for (x = 0; x < x_count; x++)			// for each character
	{
		UINT8 inv=0;
		mem = (ma + x) & 0x7ff;
		chr = videoram[mem];
		col = colorram[mem];						// read a byte of colour

		if (mbee_1c & 0x80)						// are extended features enabled?
		{
			UINT8 attr = attribram[mem];

			if (chr & 0x80)
				chr += ((attr & 15) << 7);			// bump chr to its particular pcg definition

			if (attr & 0x40)
				inv ^= 0xff;					// inverse attribute

			if ((attr & 0x80) && (framecnt & 0x10))			// flashing attribute
				chr = 0x20;
		}

		mbee_video_kbd_scan(device->machine, x+ma);

		/* process cursor */
		if ((((!flash) && (!speed)) ||					// (5,6)=(0,0) = cursor on always
			((flash) && (speed) && (framecnt & 0x10)) ||		// (5,6)=(1,1) = cycle per 32 frames
			((flash) && (!speed) && (framecnt & 8))) &&		// (5,6)=(0,1) = cycle per 16 frames
			(mem == cursor))					// displaying at cursor position?
				inv ^= sy6545_cursor[ra];			// cursor scan row

		/* get pattern of pixels for that character scanline */
		gfx = gfxram[(chr<<4) | ra] ^ inv;
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

