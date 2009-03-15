/* Super80.c written by Robbbert, 2005-2009. See the MESS wiki for documentation. */

/* Notes on using MAME MC6845 device (MMD).
	1. Speed of MMD is about 20% slower than pre-MMD coding
	2. Undocumented cursor start and end-lines is not supported by MMD, so we do it here
	3. MMD doesn't support auto-screen-resize, so we do it here. */


#include "driver.h"
#include "video/mc6845.h"
#include "super80.h"


static UINT16 vidpg=0xfe00;	/* Home position of video page being displayed */

static UINT8 current_palette;	/* for super80m and super80v */
static UINT8 current_charset;	/* for super80m */

static UINT8 mc6845_cursor[16];				// cursor shape
static UINT8 mc6845_reg[20];				/* registers */
static UINT8 mc6845_ind;				/* register index */
static const UINT8 mc6845_mask[]={0xff,0xff,0xff,0x0f,0x7f,0x1f,0x7f,0x7f,3,0x1f,0x7f,0x1f,0x3f,0xff,0x3f,0xff,0,0};

static const device_config *mc6845;
static UINT8 chr,col,gfx,fg,bg;
static UINT16 mem,x;
static UINT8 framecnt=0;
static UINT8 speed,flash;
static UINT16 cursor;
static UINT8 options;


/**************************** PALETTES for super80m and super80v ******************************************/

static const UINT8 super80_rgb_palette[16*3] =
{
	0x00, 0x00, 0x00,	/*  0 Black		*/
	0x00, 0x00, 0x00,	/*  1 Black		*/
	0x00, 0x00, 0x7f,	/*  2 Blue		*/
	0x00, 0x00, 0xff,	/*  3 Light Blue	*/
	0x00, 0x7f, 0x00,	/*  4 Green		*/
	0x00, 0xff, 0x00,	/*  5 Bright Green	*/
	0x00, 0x7f, 0x7f,	/*  6 Cyan		*/
	0x00, 0xff, 0xff,	/*  7 Turquoise		*/
	0x7f, 0x00, 0x00,	/*  8 Dark Red		*/
	0xff, 0x00, 0x00,	/*  9 Red		*/
	0x7f, 0x00, 0x7f,	/* 10 Purple		*/
	0xff, 0x00, 0xff,	/* 11 Magenta		*/
	0x7f, 0x7f, 0x00,	/* 12 Lime		*/
	0xff, 0xff, 0x00,	/* 13 Yellow		*/
	0xbf, 0xbf, 0xbf,	/* 14 Off White		*/
	0xff, 0xff, 0xff,	/* 15 White		*/
};

static const UINT8 super80_comp_palette[16*3] =
{
	0x00, 0x00, 0x00,	/*  0 Black		*/
	0x80, 0x80, 0x80,	/*  1 Grey		*/
	0x00, 0x00, 0xff,	/*  2 Blue		*/
	0xff, 0xff, 0x80,	/*  3 Light Yellow	*/
	0x00, 0xff, 0x00,	/*  4 Green		*/
	0xff, 0x80, 0xff,	/*  5 Light Magenta	*/
	0x00, 0xff, 0xff,	/*  6 Cyan		*/
	0xff, 0x40, 0x40,	/*  7 Light Red		*/
	0xff, 0x00, 0x00,	/*  8 Red		*/
	0x00, 0x80, 0x80,	/*  9 Dark Cyan		*/
	0xff, 0x00, 0xff,	/* 10 Magenta		*/
	0x80, 0xff, 0x80,	/* 11 Light Green	*/
	0xff, 0xff, 0x00,	/* 12 Yellow		*/
	0x00, 0x00, 0x80,	/* 13 Dark Blue		*/
	0xff, 0xff, 0xff,	/* 14 White		*/
	0x00, 0x00, 0x00,	/* 15 Black		*/
};

static void palette_set_colors_rgb(running_machine *machine, const UINT8 *colors)
{
	UINT8 r, b, g, color_count = 16;

	while (color_count--)
	{
		r = *colors++; g = *colors++; b = *colors++;
		colortable_palette_set_color(machine->colortable, 15 - color_count, MAKE_RGB(r, g, b));
	}
}

PALETTE_INIT( super80m )
{
	int i;
	machine->colortable = colortable_alloc(machine, 16);
	palette_set_colors_rgb(machine, super80_rgb_palette);

	for( i = 0; i < 256; i++ )
	{
		colortable_entry_set_value(machine->colortable, i*2, i>>4);
		colortable_entry_set_value(machine->colortable, i*2+1, i&15);
	}
}


/**************************** VIDEO *****************************************************************/
/* the following are for super80v */
READ8_HANDLER( super80v_low_r )
{
	if (!super80v_vid_col)
		return colorram[offset];
	else
		return videoram[offset];
}

WRITE8_HANDLER( super80v_low_w )
{
	if (!super80v_vid_col)
		colorram[offset] = data;
	else
		videoram[offset] = data;
}

READ8_HANDLER( super80v_high_r )
{
	if (!super80v_vid_col)
		return colorram[0x800+offset];

	if (!super80v_rom_pcg)
		return pcgram[offset];
	else
		return pcgram[0x800+offset];
}

WRITE8_HANDLER( super80v_high_w )
{
	if (!super80v_vid_col)
		colorram[offset+0x800] = data;
	else
	{
		videoram[offset+0x800] = data;

		if (super80v_rom_pcg)
			pcgram[0x800+offset] = data;
	}
}

/* The 6845 can produce a variety of cursor shapes - all are emulated here - remove when mame fixed */
static void mc6845_cursor_configure(void)
{
	UINT8 i,curs_type=0,r9,r10,r11;

	/* curs_type holds the general cursor shape to be created
		0 = no cursor
		1 = partial cursor (only shows on a block of scan lines)
		2 = full cursor
		3 = two-part cursor (has a part at the top and bottom with the middle blank) */

	for ( i = 0; i < ARRAY_LENGTH(mc6845_cursor); i++) mc6845_cursor[i] = 0;		// prepare cursor by erasing old one

	r9  = mc6845_reg[9];					// number of scan lines - 1
	r10 = mc6845_reg[10] & 0x1f;				// cursor start line = last 5 bits
	r11 = mc6845_reg[11]+1;					// cursor end line incremented to suit for-loops below

	/* decide the curs_type by examining the registers */
	if (r10 < r11) curs_type=1;				// start less than end, show start to end
	else
	if (r10 == r11) curs_type=2;				// if equal, show full cursor
	else curs_type=3;					// if start greater than end, it's a two-part cursor

	if ((r11 - 1) > r9) curs_type=2;			// if end greater than scan-lines, show full cursor
	if (r10 > r9) curs_type=0;				// if start greater than scan-lines, then no cursor
	if (r11 > 16) r11=16;					// truncate 5-bit register to fit our 4-bit hardware

	/* create the new cursor */
	if (curs_type > 1) for (i = 0;i < ARRAY_LENGTH(mc6845_cursor);i++) mc6845_cursor[i]=0xff; // turn on full cursor

	if (curs_type == 1) for (i = r10;i < r11;i++) mc6845_cursor[i]=0xff; // for each line that should show, turn on that scan line

	if (curs_type == 3) for (i = r11; i < r10;i++) mc6845_cursor[i]=0; // now take a bite out of the middle
}

/* Resize the screen within the limits of the hardware. Expand the image to fill the screen area */
static void mc6845_screen_configure(running_machine *machine)
{
	rectangle visarea;

	UINT16 width = mc6845_reg[1]*7-1;							// width in pixels
	UINT16 height = mc6845_reg[6]*(mc6845_reg[9]+1)-1;					// height in pixels
	UINT16 bytes = mc6845_reg[1]*mc6845_reg[6]-1;						// video ram needed -1

	/* Resize the screen */
	visarea.min_x = 0;
	visarea.max_x = width-1;
	visarea.min_y = 0;
	visarea.max_y = height-1;
	if ((width < 610) && (height < 460) && (bytes < 0x1000))	/* bounds checking to prevent an assert or violation */
		video_screen_set_visarea(machine->primary_screen, 0, width, 0, height);
}

VIDEO_EOF( super80m )
{
	/* if we chose another palette or colour mode, enable it */
	UINT8 chosen_palette = (input_port_read(machine, "CONFIG") & 0x60)>>5;				// read colour dipswitches

	if (chosen_palette != current_palette)						// any changes?
	{
		current_palette = chosen_palette;					// save new palette
		if (!current_palette)
			palette_set_colors_rgb(machine, super80_comp_palette);		// composite colour
		else
			palette_set_colors_rgb(machine, super80_rgb_palette);		// rgb and b&w
	}
}

VIDEO_UPDATE( super80 )
{
	UINT8 x, y, code=32, screen_on=0;
	UINT8 mask = screen->machine->gfx[0]->total_elements - 1;	/* 0x3F for super80; 0xFF for super80d & super80e */
	UINT8 *RAM = memory_region(screen->machine, "maincpu");

	if ((super80_mhz == 1) || (!(input_port_read(screen->machine, "CONFIG") & 4)))	/* bit 2 of port F0 is high, OR user turned on config switch */
		screen_on++;

	/* display the picture */
	for (y=0; y<16; y++)
	{
		for (x=0; x<32; x++)
		{
			if (screen_on)
				code = RAM[vidpg | x | (y<<5)];		/* get character to display */

			drawgfx(bitmap, screen->machine->gfx[0], code & mask, 0, 0, 0, x*8, y*10, cliprect, TRANSPARENCY_NONE, 0);
		}
	}

	return 0;
}

VIDEO_UPDATE( super80m )
{
	UINT8 x, y, code=32, col=0, screen_on=0, options=input_port_read(screen->machine, "CONFIG");
	UINT8 *RAM = memory_region(screen->machine, "maincpu");

	/* get selected character generator */
	UINT8 cgen = current_charset ^ ((options & 0x10)>>4);	/* bit 0 of port F1 and cgen config switch */

	if ((super80_mhz == 1) || (!(options & 4)))	/* bit 2 of port F0 is high, OR user turned on config switch */
		screen_on++;

	if (screen_on)
	{
		if ((options & 0x60) == 0x60)
			col = 15;		/* b&w */
		else
			col = 5;		/* green */
	}

	/* display the picture */
	for (y=0; y<16; y++)
	{
		for (x=0; x<32; x++)
		{
			if (screen_on)
			{
				code = RAM[vidpg | x | (y<<5)];		/* get character to display */

				if (!(options & 0x40)) col = RAM[0xfe00 | x | (y<<5)];	/* byte of colour to display */
			}

			drawgfx(bitmap, screen->machine->gfx[cgen], code, col, 0, 0, x*8, y*10,	cliprect, TRANSPARENCY_NONE, 0);
		}
	}

	return 0;
}


VIDEO_START( super80v )
{
	mc6845 = devtag_get_device(machine, "crtc");
}

VIDEO_UPDATE( super80v )
{
	framecnt++;
	speed = mc6845_reg[10]&0x20, flash = mc6845_reg[10]&0x40;				// cursor modes
	cursor = (mc6845_reg[14]<<8) | mc6845_reg[15];					// get cursor position
	options=input_port_read(screen->machine, "CONFIG");
	mc6845_update(mc6845, bitmap, cliprect);
	return 0;
}

MC6845_UPDATE_ROW( super80v_update_row )
{
	UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);

	for (x = 0; x < x_count; x++)				// for each character
	{
		UINT8 inv=0;
		//		if (x == cursor_x) inv=0xff;	/* uncomment when mame fixed */
		mem = (ma + x) & 0xfff;
		chr = videoram[mem];

		/* get colour or b&w */
		col = 5;					/* green */
		if ((options & 0x60) == 0x60) col = 15;		/* b&w */
		if (!(options & 0x40)) col = colorram[mem];			// read a byte of colour

		/* if inverse mode, replace any pcgram chrs with inverse chrs */
		if ((!super80v_rom_pcg) && (chr & 0x80))			// is it a high chr in inverse mode
		{
			inv ^= 0xff;						// invert the chr
			chr &= 0x7f;						// and drop bit 7
		}

		/* process cursor - remove when mame fixed */
		if ((((!flash) && (!speed)) ||
			((flash) && (speed) && (framecnt & 0x10)) ||
			((flash) && (!speed) && (framecnt & 8))) &&
			(mem == cursor))
				inv ^= mc6845_cursor[ra];

		/* get pattern of pixels for that character scanline */
		gfx = pcgram[(chr<<4) | ra] ^ inv;
		fg = ((col & 0x0f) << 1) + 1;
		bg = ((col & 0xf0) >> 3) + 1;

		/* Display a scanline of a character (7 pixels) */
		*p = ( gfx & 0x80 ) ? fg : bg; p++;
		*p = ( gfx & 0x40 ) ? fg : bg; p++;
		*p = ( gfx & 0x20 ) ? fg : bg; p++;
		*p = ( gfx & 0x10 ) ? fg : bg; p++;
		*p = ( gfx & 0x08 ) ? fg : bg; p++;
		*p = ( gfx & 0x04 ) ? fg : bg; p++;
		*p = ( gfx & 0x02 ) ? fg : bg; p++;
	}
}

/**************************** I/O PORTS *****************************************************************/

WRITE8_HANDLER( super80v_10_w )
{
	if (data < 18) mc6845_ind = data; else mc6845_ind = 19;		/* make sure if you try using an invalid register your write will go nowhere */
	mc6845_address_w( mc6845, 0, data );
}

WRITE8_HANDLER( super80v_11_w )
{
	if (mc6845_ind < 16) mc6845_reg[mc6845_ind] = data & mc6845_mask[mc6845_ind];	/* save data in register */
	if ((mc6845_ind == 1) || (mc6845_ind == 6) || (mc6845_ind == 9)) mc6845_screen_configure(space->machine); /* adjust screen size */
	if ((mc6845_ind > 8) && (mc6845_ind < 12)) mc6845_cursor_configure();		/* adjust cursor shape - remove when mame fixed */
	mc6845_register_w( mc6845, 0, data );
}

WRITE8_HANDLER( super80_f1_w )
{
	vidpg = (data & 0xfe) << 8;
	current_charset = data & 1;
}

