/* Super80.c written by Robbbert, 2005-2008. See super80.txt for documentation. */

#include "driver.h"
#include "machine/z80pio.h"
#include "cpu/z80/z80daisy.h"
#include "devices/snapquik.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "devices/z80bin.h"
#include "inputx.h"
#include "sound/speaker.h"
#include "deprecat.h"		/* "Machine" needed for z80pio interrupt */

static UINT8 super80_mhz=2;	/* state of bit 2 of port F0 */
static UINT16 vidpg=0xfe00;	/* Home position of video page being displayed */
static UINT8 int_sw;		/* internal 1 mhz flipflop */

static UINT8 current_palette;	/* for super80m and super80v */
static UINT8 current_charset;	/* for super80m */

/* the rest are for super80v */
static UINT8 *pcgram;
static int off_x = 0;
static int off_y = 0;
static UINT8 framecnt = 0;
static UINT8 super80v_vid_col=1;			// 0 = color ram ; 1 = video ram
static UINT8 super80v_rom_pcg=1;			// 0 = prom ; 1 = pcg
static UINT8 mc6845_cursor[15];				// cursor shape
static UINT8 mc6845[20];				/* registers */
static UINT8 mc6845_reg;				/* register index */
static UINT8 mc6845_mask[]={0xff,0xff,0xff,0x0f,0x7f,0x1f,0x7f,0x7f,3,0x1f,0x7f,0x1f,0x3f,0xff,0x3f,0xff,0,0};

#define MASTER_CLOCK			(12e6)		/* 12mhz */
#define HTOTAL				(384)
#define HBEND				(0)
#define HBSTART				(256)
#define VTOTAL				(240)
#define VBEND				(0)
#define VBSTART				(160)

#define SUPER80V_SCREEN_WIDTH		(560)
#define SUPER80V_SCREEN_HEIGHT		(300)

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

static PALETTE_INIT( super80m )
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
static READ8_HANDLER( super80v_low_r )
{
	if (!super80v_vid_col)
		return colorram[offset];
	else
		return videoram[offset];
}

static WRITE8_HANDLER( super80v_low_w )
{
	if (!super80v_vid_col)
		colorram[offset] = data;
	else
		videoram[offset] = data;
}

static READ8_HANDLER( super80v_high_r )
{
	if (!super80v_vid_col)
		return colorram[0x800+offset];

	if (!super80v_rom_pcg)
		return pcgram[offset];
	else
		return pcgram[0x800+offset];
}

static WRITE8_HANDLER( super80v_high_w )
{
	if (!super80v_vid_col)
		colorram[offset+0x800] = data;
	else
	{
		videoram[offset+0x800] = data;

		if (super80v_rom_pcg)
		{
			int chr = 0x80 + (offset>>4);
			pcgram[0x800+offset] = data;

			/* decode character graphics again */
			decodechar(Machine->gfx[0], chr, pcgram);
		}
	}
}

/* The 6845 can produce a variety of cursor shapes - all are emulated here */
static void mc6845_cursor_configure(void)
{
	UINT8 i,curs_type=0,r9,r10,r11;

	/* curs_type holds the general cursor shape to be created
		0 = no cursor
		1 = partial cursor (only shows on a block of scan lines)
		2 = full cursor
		3 = two-part cursor (has a part at the top and bottom with the middle blank) */

	for ( i = 0; i < 16; i++) mc6845_cursor[i] = 0;		// prepare cursor by erasing old one

	r9  = mc6845[9];					// number of scan lines - 1
	r10 = mc6845[10] & 0x1f;				// cursor start line = last 5 bits
	r11 = mc6845[11]+1;					// cursor end line incremented to suit for-loops below

	/* decide the curs_type by examining the registers */
	if (r10 < r11) curs_type=1;				// start less than end, show start to end
	else
	if (r10 == r11) curs_type=2;				// if equal, show full cursor
	else curs_type=3;					// if start greater than end, it's a two-part cursor

	if ((r11 - 1) > r9) curs_type=2;			// if end greater than scan-lines, show full cursor
	if (r10 > r9) curs_type=0;				// if start greater than scan-lines, then no cursor
	if (r11 > 16) r11=16;					// truncate 5-bit register to fit our 4-bit hardware

	/* create the new cursor */
	if (curs_type > 1) for (i = 0;i < 16;i++) mc6845_cursor[i]=0xff; // turn on full cursor

	if (curs_type == 1) for (i = r10;i < r11;i++) mc6845_cursor[i]=0xff; // for each line that should show, turn on that scan line
		
	if (curs_type == 3) for (i = r11; i < r10;i++) mc6845_cursor[i]=0; // now take a bite out of the middle
}

/* Resize the screen within the limits of the hardware.
	If we are using dynamic screen resizing, expand the image to fill the screen area */
static void mc6845_screen_configure(void)
{
	screen_state *state = &Machine->screen[0];
	rectangle visarea;
	UINT16 width, height, bytes;	
	UINT8 dyn = readinputportbytag("CONFIG") & 0x10;			// read dipswitch
	visarea.min_x = 0;
	visarea.max_x = SUPER80V_SCREEN_WIDTH-1;
	visarea.min_y = 0;
	visarea.max_y = SUPER80V_SCREEN_HEIGHT-1;

	/* calculate the effect of sync and vertical adjustments */
	if ( mc6845[2] ) off_x = mc6845[0] - mc6845[2] - 23; else off_x = -24;

	off_y = (mc6845[4] - mc6845[7]) * (mc6845[9] + 1) + mc6845[5];

	if( off_y < 0 ) off_y = 0;

	if( off_y > 128 ) off_y = 128;

	if (dyn)
	{
		off_x = 0;
		off_y = 0;
	}

	width = mc6845[1]*7-1;							// width in pixels
	height = mc6845[6]*(mc6845[9]+1)-1;					// height in pixels
	bytes = mc6845[1]*mc6845[6]-1;						// video ram needed -1

	/* Physically resize the screen now */
	if (dyn)
	{
		visarea.min_x = 0;
		visarea.max_x = width;
		visarea.min_y = 0;
		visarea.max_y = height;
		if ((width < 610)
		&& (height < 460)			/* bounds checking to prevent an assert or violation */
		&& (bytes < 0x1000))
			video_screen_configure(0, width+1, height+1, &visarea, state->refresh); 
	}
	else
			video_screen_configure(0, SUPER80V_SCREEN_WIDTH, SUPER80V_SCREEN_HEIGHT, &visarea, state->refresh);	// in case dipswitch was just turned to NO
}

static VIDEO_EOF( super80m )
{
	/* if we chose another palette or colour mode, enable it */
	UINT8 chosen_palette = (readinputportbytag("CONFIG") & 0x60)>>5;				// read colour dipswitches

	if (chosen_palette != current_palette)						// any changes?
	{
		current_palette = chosen_palette;					// save new palette
		if (!current_palette)
			palette_set_colors_rgb(machine, super80_comp_palette);		// composite colour
		else
			palette_set_colors_rgb(machine, super80_rgb_palette);		// rgb and b&w
	}
}

static VIDEO_UPDATE( super80 )
{
	UINT8 x, y, code=32, screen_on=0;
	UINT8 mask = machine->gfx[0]->total_elements - 1;	/* 0x3F for super80; 0xFF for super80d & super80e */

	if ((super80_mhz == 1) || (!(readinputportbytag("CONFIG") & 4)))	/* bit 2 of port F0 is high, OR user turned on config switch */
		screen_on++;

	/* display the picture */
	for (y=0; y<16; y++)
	{
		for (x=0; x<32; x++)
		{
			if (screen_on)
				code = program_read_byte(vidpg + x + (y<<5));

			drawgfx(bitmap, machine->gfx[0], code & mask, 0, 0, 0, x*8, y*10,
				&machine->screen[0].visarea, TRANSPARENCY_NONE, 0);
		}
	}

	return 0;
}

static VIDEO_UPDATE( super80m )
{
	UINT8 x, y, code=32, col=0, screen_on=0, options=readinputportbytag("CONFIG");

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
				code = program_read_byte(vidpg + x + (y<<5));		/* get character to display */

				if (!(options & 0x40)) col = program_read_byte(0xfe00 + x + (y<<5));	/* byte of colour to display */
			}

			drawgfx(bitmap, machine->gfx[cgen], code, col, 0, 0, x*8, y*10,
				&machine->screen[0].visarea, TRANSPARENCY_NONE, 0);
		}
	}

	return 0;
}

static VIDEO_UPDATE( super80v )
{
	UINT16 i, bytes = mc6845[1]*mc6845[6];
	UINT8 speed = mc6845[10]&0x20, flash = mc6845[10]&0x40;				// cursor modes
	UINT16 cursor = (mc6845[14]<<8) | mc6845[15];					// get cursor position
	UINT16 screen_home = (mc6845[12]<<8) | mc6845[13];				// screen home offset (usually zero)
	UINT8 options=readinputportbytag("CONFIG");
	framecnt++;

	/* Get the graphics of the character under the cursor, xor with the visible cursor scan lines,
	   and store as character number 256. If inverse mode, drop bit 7 of character before xoring */
	if (!super80v_rom_pcg)
		for ( i = 0; i < 16; i++)
			pcgram[0x1000+i] = pcgram[(videoram[cursor]&0x7f)*16+i] ^ mc6845_cursor[i];
	else
		for ( i = 0; i < 16; i++)
			pcgram[0x1000+i] = pcgram[(videoram[cursor])*16 + i] ^ mc6845_cursor[i];

	decodechar(machine->gfx[0],256, pcgram);			// and into machine graphics

	for( i = screen_home; (i < (bytes + screen_home)) & (i < 0x1000); i++ )
	{
		int sx, sy, chr, col;
		sy = off_y + ((i - screen_home) / mc6845[1]) * (mc6845[9] + 1);
		sx = (off_x + ((i - screen_home) % mc6845[1])) * 7;
		chr = videoram[i];

		/* get colour or b&w */
		if ((options & 0x60) == 0x60) col = 15;		/* b&w */
		else col = 5;					/* green */
		if (!(options & 0x40)) col = colorram[i];			// read a byte of colour

		if ((!super80v_rom_pcg) && (chr > 0x7f))			// is it a high chr in inverse mode
		{
			col = BITSWAP8(col,3,2,1,0,7,6,5,4);			// swap nibbles
			chr &= 0x7f;						// and drop bit 7
		}

		/* if cursor is on and we are at cursor position, show it */
		/* NOTE: flash rates obtained from real hardware */

		if ((((!flash) && (!speed)) ||					// (5,6)=(0,0) = cursor on always
			((flash) && (speed) && (framecnt & 0x10)) ||		// (5,6)=(1,1) = cycle per 32 frames
			((flash) && (!speed) && (framecnt & 8))) &&		// (5,6)=(0,1) = cycle per 16 frames
			(i == cursor))						// displaying at cursor position?
				chr = 256;					// 256 = cursor character

		drawgfx( bitmap,Machine->gfx[0],chr,col,0,0,sx,sy,
			&Machine->screen[0].visarea,TRANSPARENCY_NONE,0);	// put character on the screen

	}

	return 0;
}

/**************************** PIO ******************************************************************************/

static void pio_interrupt(int state)
{
	cpunum_set_input_line( Machine, 0, 0, state ? ASSERT_LINE : CLEAR_LINE );
}

static const z80pio_interface pio_intf =
{
	pio_interrupt,		/* callback when change interrupt status */
	NULL,			/* portA ready active callback (not used in super80) */
	NULL			/* portB ready active callback (not used in super80) */
};


static const struct z80_irq_daisy_chain super80_daisy_chain[] =
{
	{ z80pio_reset, z80pio_irq_state, z80pio_irq_ack, z80pio_irq_reti, 0 },
	{ 0, 0, 0, 0, -1}      /* end mark */
};

/**************************** CASSETTE ROUTINES *****************************************************************/

static mess_image *cassette_device_image(void)
{
	return image_from_devtype_and_index(IO_CASSETTE, 0);
}

static void cassette_motor( UINT8 data )
{
	if (data)
		cassette_change_state(image_from_devtype_and_index(IO_CASSETTE, 0), CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);
	else
		cassette_change_state(image_from_devtype_and_index(IO_CASSETTE, 0), CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);

	/* does user want to hear the sound? */
	if (readinputportbytag("CONFIG") & 8)
		cassette_change_state(image_from_devtype_and_index(IO_CASSETTE, 0), CASSETTE_SPEAKER_ENABLED, CASSETTE_MASK_SPEAKER);
	else
		cassette_change_state(image_from_devtype_and_index(IO_CASSETTE, 0), CASSETTE_SPEAKER_MUTED, CASSETTE_MASK_SPEAKER);
}

/********************************************* TIMER ************************************************/

static UINT8 wave_length, last_wave_state=0, cass_out;

/* this timer runs at 200khz and does 2 jobs:
	1. Scan the keyboard and present the results to the pio
	2. Emulate the 2 chips in the cassette input circuit

Reasons why it is necessary:
	1. The real z80pio is driven by the cpu clock and is capable of independent actions.
	MAME does not support this at all. If the interrupt key sequence is entered, the
	computer can be reset out of a hung state by the operator.
	2. This "emulates" U79 CD4046BCN PLL chip and U1 LM311P op-amp. U79 converts a frequency to a voltage,
	and U1 amplifies that voltage to digital levels. U1 has a trimpot connected, to set the midpoint. */

static TIMER_CALLBACK( super80_timer )
{
	UINT8 wave_state=0, i, mask=1, out_f8=z80pio_p_r(0,0), in_fa=255;

	for ( i=1; i < 9;i++)
	{
		if (!(out_f8 & mask)) in_fa &= readinputport(i);
		mask<<=1;
	}

	z80pio_p_w(0,1,in_fa);

	if (cassette_input(cassette_device_image()) > +0.03) wave_state+=2;

	if (wave_state == last_wave_state)
		wave_length++;
	else
	{
		last_wave_state = wave_state;
		cass_out = wave_state;
		if (wave_length < 0x40) cass_out++;
		wave_length = 0;
	}
}

/**************************** I/O PORTS *****************************************************************/

static READ8_HANDLER( super80v_11_r )
{
	if ((mc6845_reg > 13) && (mc6845_reg < 18))		/* determine readable registers */
		return mc6845[mc6845_reg];
	else
		return 0;
}

static READ8_HANDLER( super80_f2_r )
{
	UINT8 data = readinputportbytag("DSW") & 0xf0;	// dip switches on pcb
	data |= cass_out;			// bit 0 = output of U1, bit 1 = current wave_state
	data |= 0x0c;				// bits 2,3 - not used
	return data;
}

static WRITE8_HANDLER( super80v_10_w )
{
	if (data < 18) mc6845_reg = data; else mc6845_reg = 19;		/* make sure if you try using an invalid register your write will go nowhere */
}

static WRITE8_HANDLER( super80v_11_w )
{
	if (mc6845_reg < 16) mc6845[mc6845_reg] = data & mc6845_mask[mc6845_reg];	/* save data in register */
	if (mc6845_reg < 10) mc6845_screen_configure();					/* adjust screen size */
	if ((mc6845_reg > 8) && (mc6845_reg < 12)) mc6845_cursor_configure();		/* adjust cursor shape */
}

static UINT8 last_data;

static WRITE8_HANDLER( super80_f0_w )
{
	UINT8 bits = data ^ last_data;

	if (bits & 0x20) set_led_status(2,(data & 32) ? 0 : 1);		/* bit 5 - LED - scroll lock led is used */
	speaker_level_w(0, (data & 8) ? 0 : 1);				/* bit 3 - speaker */
	super80_mhz = (data & 4) ? 1 : 2;				/* bit 2 - video on/off */
	if (bits & 0x02) cassette_motor( data & 2 ? 1 : 0);		/* bit 1 - cassette motor */
	if (bits & 0x01) cassette_output(cassette_device_image(), (data & 1) ? -1.0 : +1.0);	/* bit 0 - cass out */

	last_data = data;
}

static WRITE8_HANDLER( super80v_f0_w )
{
	UINT8 bits = data ^ last_data;

	if (bits & 0x20) set_led_status(2,(data & 32) ? 0 : 1);		/* bit 5 - LED - scroll lock led is used */
	super80v_rom_pcg = data & 0x10;					/* bit 4 - bankswitch gfx rom or pcg */
	speaker_level_w(0, (data & 8) ? 0 : 1);				/* bit 3 - speaker */
	super80v_vid_col = data & 4;					/* bit 2 - bankswitch video or colour ram */
	if (bits & 0x02) cassette_motor( data & 2 ? 1 : 0);		/* bit 1 - cassette motor */
	if (bits & 0x01) cassette_output(cassette_device_image(), (data & 1) ? -1.0 : +1.0);	/* bit 0 - cass out */

	last_data = data;
}

static WRITE8_HANDLER( super80_f1_w )
{
	vidpg = (data & 0xfe) << 8;
	current_charset = data & 1;
}

static READ8_HANDLER( super80_f8_r ) { return z80pio_d_r (0,0); }
static READ8_HANDLER( super80_f9_r ) { return z80pio_c_r (0,0); }
static READ8_HANDLER( super80_fa_r ) { return z80pio_d_r (0,1); }
static READ8_HANDLER( super80_fb_r ) { return z80pio_c_r (0,1); }
static WRITE8_HANDLER( super80_f8_w ) { z80pio_d_w(0,0, data); }
static WRITE8_HANDLER( super80_f9_w ) { z80pio_c_w(0,0, data); }
static WRITE8_HANDLER( super80_fa_w ) { z80pio_d_w(0,1, data); }
static WRITE8_HANDLER( super80_fb_w ) { z80pio_c_w(0,1, data); }

/**************************** MEMORY AND I/O MAPPINGS *****************************************************************/

static READ8_HANDLER( super80_read_ff ) { return 0xff; }	/* returns the true state of unmapped memory */

static ADDRESS_MAP_START( super80_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_RAMBANK(1)
	AM_RANGE(0x4000, 0xbfff) AM_RAM
	AM_RANGE(0xc000, 0xefff) AM_ROM
	AM_RANGE(0xf000, 0xffff) AM_READWRITE(super80_read_ff, MWA8_NOP)
ADDRESS_MAP_END

static ADDRESS_MAP_START( super80m_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_RAMBANK(1)
	AM_RANGE(0x4000, 0xbfff) AM_RAM
	AM_RANGE(0xc000, 0xefff) AM_ROM
	AM_RANGE(0xf000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( super80v_map, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_RAMBANK(1)
	AM_RANGE(0x4000, 0xbfff) AM_RAM
	AM_RANGE(0xc000, 0xefff) AM_ROM
	AM_RANGE(0xf000, 0xf7ff) AM_READWRITE(super80v_low_r, super80v_low_w) AM_BASE(&pcgram)
	AM_RANGE(0xf800, 0xffff) AM_READWRITE(super80v_high_r, super80v_high_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( super80_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0xdf) AM_READWRITE(super80_read_ff, MWA8_NOP)
	AM_RANGE(0xe0, 0xe0) AM_MIRROR(0x14) AM_READWRITE(super80_read_ff, super80_f0_w)
	AM_RANGE(0xe1, 0xe1) AM_MIRROR(0x14) AM_READWRITE(super80_read_ff, super80_f1_w)
	AM_RANGE(0xe2, 0xe2) AM_MIRROR(0x14) AM_READWRITE(super80_f2_r, MWA8_NOP)
	AM_RANGE(0xe3, 0xe3) AM_MIRROR(0x14) AM_READWRITE(super80_read_ff, MWA8_NOP)	/* decoded but not connected to anything */
	AM_RANGE(0xe8, 0xef) AM_READWRITE(super80_read_ff, MWA8_NOP)
	AM_RANGE(0xf8, 0xf8) AM_MIRROR(0x04) AM_READWRITE(super80_f8_r,super80_f8_w)
	AM_RANGE(0xf9, 0xf9) AM_MIRROR(0x04) AM_READWRITE(super80_f9_r,super80_f9_w)
	AM_RANGE(0xfa, 0xfa) AM_MIRROR(0x04) AM_READWRITE(super80_fa_r,super80_fa_w)
	AM_RANGE(0xfb, 0xfb) AM_MIRROR(0x04) AM_READWRITE(super80_fb_r,super80_fb_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( super80v_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x0f) AM_READWRITE(super80_read_ff, MWA8_NOP)
	AM_RANGE(0x10, 0x10) AM_READWRITE(super80_read_ff, super80v_10_w)
	AM_RANGE(0x11, 0x11) AM_READWRITE(super80v_11_r, super80v_11_w)
	AM_RANGE(0x12, 0xdf) AM_READWRITE(super80_read_ff, MWA8_NOP)
	AM_RANGE(0xe0, 0xe0) AM_MIRROR(0x14) AM_READWRITE(super80_read_ff, super80v_f0_w)
	AM_RANGE(0xe1, 0xe1) AM_MIRROR(0x14) AM_READWRITE(super80_read_ff, MWA8_NOP)	/* decoded but not connected to anything */
	AM_RANGE(0xe2, 0xe2) AM_MIRROR(0x14) AM_READWRITE(super80_f2_r, MWA8_NOP)
	AM_RANGE(0xe3, 0xe3) AM_MIRROR(0x14) AM_READWRITE(super80_read_ff, MWA8_NOP)	/* decoded but not connected to anything */
	AM_RANGE(0xe8, 0xef) AM_READWRITE(super80_read_ff, MWA8_NOP)
	AM_RANGE(0xf8, 0xf8) AM_MIRROR(0x04) AM_READWRITE(super80_f8_r,super80_f8_w)
	AM_RANGE(0xf9, 0xf9) AM_MIRROR(0x04) AM_READWRITE(super80_f9_r,super80_f9_w)
	AM_RANGE(0xfa, 0xfa) AM_MIRROR(0x04) AM_READWRITE(super80_fa_r,super80_fa_w)
	AM_RANGE(0xfb, 0xfb) AM_MIRROR(0x04) AM_READWRITE(super80_fb_r,super80_fb_w)
ADDRESS_MAP_END

/**************************** DIPSWITCHES, KEYBOARD, HARDWARE CONFIGURATION ****************************************/

static INPUT_PORTS_START( super80 )
	PORT_START_TAG("DSW")
	PORT_BIT( 0xf, 0xf, IPT_UNUSED )
	PORT_DIPNAME( 0x10, 0x00, "Switch A") PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x10, DEF_STR(Off))
	PORT_DIPSETTING(    0x00, DEF_STR(On))
	PORT_DIPNAME( 0x20, 0x20, "Switch B") PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x20, DEF_STR(Off))
	PORT_DIPSETTING(    0x00, DEF_STR(On))
	PORT_DIPNAME( 0x40, 0x40, "Switch C") PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x40, DEF_STR(Off))
	PORT_DIPSETTING(    0x00, DEF_STR(On))
	PORT_DIPNAME( 0x80, 0x00, "Switch D") PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x80, DEF_STR(Off))
	PORT_DIPSETTING(    0x00, DEF_STR(On))

	PORT_START	/* line 0 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("@ `") PORT_CODE(KEYCODE_TILDE) PORT_CHAR('@') PORT_CHAR('`')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 !") PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 )") PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("REPT") PORT_CODE(KEYCODE_LALT) 
	PORT_START	/* line 1 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 \"") PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(": *") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_START	/* line 2 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 #") PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("; +") PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB) PORT_CHAR(9)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(Fire)") PORT_CODE(KEYCODE_INSERT) PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	PORT_START	/* line 3 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("[ {") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 $") PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LINEFEED") PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHAR(10)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_START	/* line 4 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\\ |")PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\') PORT_CHAR('|')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 %") PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("BRK") PORT_CODE(KEYCODE_NUMLOCK) PORT_CHAR(3)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(Right)") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_START	/* line 5 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("] }") PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 &") PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC) PORT_CHAR(27)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(Left)") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_START	/* line 6 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("^ ~") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('^') PORT_CHAR('~')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 \'") PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL) 
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(Down)") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_START	/* line 7 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("- =") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 (") PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR(' ')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LOCK") PORT_CODE(KEYCODE_CAPSLOCK) 
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(Up)") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))

	/* Enhanced options not available on real hardware */
	PORT_START_TAG("CONFIG")
	PORT_CONFNAME( 0x01, 0x01, "Autorun on Quickload")
	PORT_CONFSETTING(    0x00, DEF_STR(No))
	PORT_CONFSETTING(    0x01, DEF_STR(Yes))
	PORT_CONFNAME( 0x02, 0x02, "2 Mhz always")
	PORT_CONFSETTING(    0x02, DEF_STR(No))
	PORT_CONFSETTING(    0x00, DEF_STR(Yes))
	PORT_CONFNAME( 0x04, 0x04, "Screen on always")
	PORT_CONFSETTING(    0x04, DEF_STR(No))
	PORT_CONFSETTING(    0x00, DEF_STR(Yes))
	PORT_CONFNAME( 0x08, 0x08, "Cassette Speaker")
	PORT_CONFSETTING(    0x08, DEF_STR(On))
	PORT_CONFSETTING(    0x00, DEF_STR(Off))
INPUT_PORTS_END

static INPUT_PORTS_START( super80m )
	PORT_INCLUDE( super80 )

	PORT_MODIFY("CONFIG")

	/* Enhanced options not available on real hardware */
	PORT_CONFNAME( 0x10, 0x10, "Swap CharGens")
	PORT_CONFSETTING(    0x10, DEF_STR(No))
	PORT_CONFSETTING(    0x00, DEF_STR(Yes))
	PORT_CONFNAME( 0x60, 0x40, "Colour")
	PORT_CONFSETTING(    0x60, "MonoChrome")
	PORT_CONFSETTING(    0x40, "Green")
	PORT_CONFSETTING(    0x00, "Composite")
	PORT_CONFSETTING(    0x20, "RGB")
INPUT_PORTS_END

static INPUT_PORTS_START( super80v )
	PORT_INCLUDE( super80m )

	PORT_MODIFY("CONFIG")

	/* Enhanced options not available on real hardware */
	PORT_BIT( 0x6, 0x6, IPT_UNUSED )
	PORT_CONFNAME( 0x10, 0x10, "Auto-Resize?")
	PORT_CONFSETTING(    0x00, DEF_STR(No))
	PORT_CONFSETTING(    0x10, DEF_STR(Yes))
INPUT_PORTS_END

/**************************** GRAPHICS DECODE *****************************************************************/

static const gfx_layout super80_charlayout =
{
	8,10,					/* 8 x 10 characters */
	64,					/* 64 characters */
	1,						/* 1 bits per pixel */
	{ 0 },					/* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{  0*8,  2*8,  4*8,  6*8,  8*8, 10*8, 12*8, 14*8,
	   1*8,  3*8,  5*8,  7*8,  9*8, 11*8, 13*8, 15*8 },
	8*16					/* every char takes 16 bytes */
};

static const gfx_layout super80d_charlayout =
{
	8,10,					/* 8 x 10 characters */
	256,					/* 256 characters */
	1,						/* 1 bits per pixel */
	{ 0 },					/* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{  0*8,  2*8,  4*8,  6*8,  8*8, 10*8, 12*8, 14*8,
	   1*8,  3*8,  5*8,  7*8,  9*8, 11*8, 13*8, 15*8 },
	8*16					/* every char takes 16 bytes */
};

static const gfx_layout super80v_charlayout =
{
	8,16,					/* 8 x 16 characters */
	257,					/* 256 characters + cursor character */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets triple height: use each line three times */
	{  0*8,  1*8,  2*8,  3*8,  4*8,  5*8,  6*8,  7*8,
	   8*8,  9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	   8*16					/* every char takes 16 bytes */
};

static GFXDECODE_START( super80 )
	GFXDECODE_ENTRY( REGION_GFX1, 0x0000, super80_charlayout, 0, 1 )
GFXDECODE_END

static GFXDECODE_START( super80d )
	GFXDECODE_ENTRY( REGION_GFX1, 0x0000, super80d_charlayout, 0, 1 )
GFXDECODE_END

static GFXDECODE_START( super80m )
	GFXDECODE_ENTRY( REGION_GFX1, 0x0000, super80d_charlayout, 0, 256 )
	GFXDECODE_ENTRY( REGION_GFX2, 0x0000, super80d_charlayout, 0, 256 )
GFXDECODE_END

static GFXDECODE_START( super80v )
	GFXDECODE_ENTRY( REGION_CPU1, 0xf000, super80v_charlayout, 0, 256 )
GFXDECODE_END

/**************************** BASIC MACHINE CONSTRUCTION ***********************************************************/

static TIMER_CALLBACK( super80_halfspeed )
{
	UINT8 go_fast = 0;
	if ((super80_mhz == 2) || (!(readinputportbytag("CONFIG") & 2)))	/* bit 2 of port F0 is low, OR user turned on config switch */
		go_fast++;

	/* code to slow down computer to 1mhz by halting cpu on every second frame */
	if (!go_fast)
	{
		if (!int_sw)
			cpunum_set_input_line(machine, 0, INPUT_LINE_HALT, ASSERT_LINE);	// if going, stop it

		int_sw++;
		if (int_sw > 1)
		{
			cpunum_set_input_line(machine, 0, INPUT_LINE_HALT, CLEAR_LINE);		// if stopped, start it
			int_sw = 0;
		}
	}
	else
	{
		if (int_sw < 8)								// @2mhz, reset just once
		{
			cpunum_set_input_line(machine, 0, INPUT_LINE_HALT, CLEAR_LINE);
			int_sw = 8;							// ...not every time
		}
	}
}


/* after the first 4 bytes have been read from ROM, switch the ram back in */
static TIMER_CALLBACK( super80_reset )
{
	memory_set_bank(1, 0);
}

MACHINE_RESET( super80 )
{
	z80pio_init(0, &pio_intf);
	timer_set(ATTOTIME_IN_USEC(10), NULL, 0, super80_reset);
	memory_set_bank(1, 1);
}

static MACHINE_DRIVER_START( super80 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, MASTER_CLOCK/6)		/* 2 Mhz */
	MDRV_CPU_PROGRAM_MAP(super80_map, 0)
	MDRV_CPU_IO_MAP(super80_io, 0)
	MDRV_CPU_CONFIG(super80_daisy_chain)

	MDRV_MACHINE_RESET( super80 )

	MDRV_GFXDECODE(super80)
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(48.8)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(MASTER_CLOCK, HTOTAL, HBEND, HBSTART, VTOTAL, VBEND, VBSTART)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_UPDATE(super80)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(WAVE, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD(SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( super80d )
	MDRV_IMPORT_FROM(super80)
	MDRV_GFXDECODE(super80d)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( super80m )
	MDRV_IMPORT_FROM(super80)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(super80m_map, 0)

	MDRV_GFXDECODE(super80m)
	MDRV_PALETTE_LENGTH(256*2)
	MDRV_PALETTE_INIT(super80m)
	MDRV_VIDEO_EOF(super80m)
	MDRV_VIDEO_UPDATE(super80m)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( super80v )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, MASTER_CLOCK/6)		/* 2 Mhz */
	MDRV_CPU_PROGRAM_MAP(super80v_map, 0)
	MDRV_CPU_IO_MAP(super80v_io, 0)
	MDRV_CPU_CONFIG(super80_daisy_chain)

	MDRV_MACHINE_RESET( super80 )

	MDRV_GFXDECODE(super80v)
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(SUPER80V_SCREEN_WIDTH, SUPER80V_SCREEN_HEIGHT)
	MDRV_SCREEN_VISIBLE_AREA(0, SUPER80V_SCREEN_WIDTH-1, 0, SUPER80V_SCREEN_HEIGHT-1)
	MDRV_PALETTE_LENGTH(256*2)
	MDRV_PALETTE_INIT(super80m)

	MDRV_VIDEO_EOF(super80m)
	MDRV_VIDEO_UPDATE(super80v)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(WAVE, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD(SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

static void driver_init_common( void )
{
	UINT8 *RAM = memory_region(REGION_CPU1);
	memory_configure_bank(1, 0, 2, &RAM[0x0000], 0xc000);
	timer_pulse(ATTOTIME_IN_HZ(200000),NULL,0,super80_timer);	/* timer for keyboard and cassette */
}

static DRIVER_INIT( super80 )
{
	timer_pulse(ATTOTIME_IN_HZ(100),NULL,0,super80_halfspeed);	/* timer for 1mhz slowdown */
	driver_init_common();
}

static DRIVER_INIT( super80d )
{
	UINT8 *RAM = memory_region(REGION_GFX1);
	UINT16 i;	/* create inverse characters */
	for (i = 0x0800; i < 0x1000; i++) RAM[i] = ~RAM[i];
	DRIVER_INIT_CALL( super80 );
}

static DRIVER_INIT( super80v )
{
	pcgram = memory_region(REGION_CPU1)+0xf000;
	videoram = memory_region(REGION_CPU1)+0x18000;
	colorram = memory_region(REGION_CPU1)+0x1C000;
	driver_init_common();
}


/**************************** ROMS *****************************************************************/

ROM_START( super80 )
	ROM_REGION(0x10000, REGION_CPU1, 0)
	ROM_LOAD("super80.u26",  0xc000, 0x1000, CRC(6a6a9664) SHA1(2c4fcd943aa9bf7419d58fbc0e28ffb89ef22e0b) )
	ROM_LOAD("super80.u33",  0xd000, 0x1000, CRC(cf8020a8) SHA1(2179a61f80372cd49e122ad3364773451531ae85) )
	ROM_LOAD("super80.u42",  0xe000, 0x1000, CRC(a1c6cb75) SHA1(d644ca3b399c1a8902f365c6095e0bbdcea6733b) )

	ROM_REGION(0x0400, REGION_GFX1, ROMREGION_DISPOSE)
	ROM_LOAD("super80.u27",  0x0000, 0x0400, CRC(d1e4b3c6) SHA1(3667b97c6136da4761937958f281609690af4081) )
ROM_END

ROM_START( super80d )
	ROM_REGION(0x10000, REGION_CPU1, 0)
	ROM_LOAD("super80d.u26", 0xc000, 0x1000, CRC(cebd2613) SHA1(87b94cc101a5948ce590211c68272e27f4cbe95a) )
	ROM_LOAD("super80.u33",	 0xd000, 0x1000, CRC(cf8020a8) SHA1(2179a61f80372cd49e122ad3364773451531ae85) )
	ROM_LOAD("super80.u42",	 0xe000, 0x1000, CRC(a1c6cb75) SHA1(d644ca3b399c1a8902f365c6095e0bbdcea6733b) )

	ROM_REGION(0x1000, REGION_GFX1, ROMREGION_DISPOSE)
	ROM_LOAD("super80d.u27", 0x0000, 0x0800, CRC(cb4c81e2) SHA1(8096f21c914fa76df5d23f74b1f7f83bd8645783) )
	ROM_RELOAD(              0x0800, 0x0800 )
ROM_END

ROM_START( super80e )
	ROM_REGION(0x10000, REGION_CPU1, 0)
	ROM_LOAD("super80e.u26", 0xc000, 0x1000, CRC(bdc668f8) SHA1(3ae30b3cab599fca77d5e461f3ec1acf404caf07) )
	ROM_LOAD("super80.u33",	 0xd000, 0x1000, CRC(cf8020a8) SHA1(2179a61f80372cd49e122ad3364773451531ae85) )
	ROM_LOAD("super80.u42",	 0xe000, 0x1000, CRC(a1c6cb75) SHA1(d644ca3b399c1a8902f365c6095e0bbdcea6733b) )

	ROM_REGION(0x1000, REGION_GFX1, ROMREGION_DISPOSE)
	ROM_LOAD("super80e.u27", 0x0000, 0x1000, CRC(ebe763a7) SHA1(ffaa6d6a2c5dacc5a6651514e6707175a32e83e8) )
ROM_END

ROM_START( super80m )
	ROM_REGION(0x10000, REGION_CPU1, 0)
	ROM_LOAD("s80-8r0.u26",	 0xc000, 0x1000, CRC(48d410d8) SHA1(750d984abc013a3344628300288f6d1ba140a95f) )
	ROM_LOAD("s80-8r0.u33",  0xd000, 0x1000, CRC(9765793e) SHA1(4951b127888c1f3153004cc9fb386099b408f52c) )
	ROM_LOAD("s80-8r0.u42",  0xe000, 0x1000, CRC(5f65d94b) SHA1(fe26b54dec14e1c4911d996c9ebd084a38dcb691) )

	ROM_REGION(0x1000, REGION_GFX1, ROMREGION_DISPOSE)
	ROM_LOAD("super80d.u27", 0x0000, 0x0800, CRC(cb4c81e2) SHA1(8096f21c914fa76df5d23f74b1f7f83bd8645783) )
	ROM_RELOAD(              0x0800, 0x0800 )

	ROM_REGION(0x1000, REGION_GFX2, ROMREGION_DISPOSE)
	ROM_LOAD("super80e.u27", 0x0000, 0x1000, CRC(ebe763a7) SHA1(ffaa6d6a2c5dacc5a6651514e6707175a32e83e8) )
ROM_END

ROM_START( super80v )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD("s80-v37v.u26", 0xc000, 0x1000, CRC(01e0c0dd) SHA1(ef66af9c44c651c65a21d5bda939ffa100078c08) )
	ROM_LOAD("s80-v37.u33",  0xd000, 0x1000, CRC(812ad777) SHA1(04f355bea3470a7d9ea23bb2811f6af7d81dc400) )
	ROM_LOAD("s80-v37.u42",  0xe000, 0x1000, CRC(e02e736e) SHA1(57b0264c805da99234ab5e8e028fca456851a4f9) )
	ROM_LOAD("s80hmce.ic24", 0xf000, 0x0800, CRC(a6488a1e) SHA1(7ba613d70a37a6b738dcd80c2bb9988ff1f011ef) )
ROM_END

/**************************** DEVICES *****************************************************************/

static QUICKLOAD_LOAD( super80 )
{
	UINT8 sw = readinputportbytag("CONFIG") & 1;				/* reading the dipswitch: 1 = autorun */
	UINT16 exec_addr;
	UINT64 return_info = z80bin_load_file( image, file_type );	/* load file */

	if (return_info == INIT_FAIL) return INIT_FAIL;			/* failure */

	exec_addr = (return_info & 0xffff0000) >> 16;			/* get program run address */

	if ((exec_addr != 0xffff) && (sw))
		cpunum_set_reg(0, REG_PC, exec_addr);

	return INIT_PASS;
}

static void super80_quickload_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* quickload */
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_DEV_FILE:		strcpy(info->s = device_temp_str(), __FILE__); break;
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:	strcpy(info->s = device_temp_str(), "bin"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_QUICKLOAD_LOAD:	info->f = (genf *) quickload_load_super80; break;

		default:				quickload_device_getinfo(devclass, state, info); break;
	}
}

static DEVICE_LOAD( super80_cart )
{
/*	int size = mame_fsize(fp);
	UINT8 *mem = malloc(size);
	if( mem )
	{
		if( mame_fread(fp, mem, size) == size )
		{
			memcpy(memory_region(REGION_CPU1)+0xc000, mem, size);
		}
		free(mem);
	} */

	image_fread(image, memory_region(REGION_CPU1) + 0xc000, 0x3000);

	return INIT_PASS;
}

static void super80_cartslot_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:			info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:			info->load = device_load_super80_cart; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:	strcpy(info->s = device_temp_str(), "rom"); break;

		default:				cartslot_device_getinfo(devclass, state, info); break;
	}
}

static void super80_cassette_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:			info->i = 1; break;
		case MESS_DEVINFO_INT_CASSETTE_DEFAULT_STATE:info->i = CASSETTE_STOPPED | CASSETTE_MOTOR_DISABLED | CASSETTE_SPEAKER_ENABLED; break;

		default:				cassette_device_getinfo(devclass, state, info); break;
	}
}
INPUT_PORTS_START(0)
INPUT_PORTS_END

SYSTEM_CONFIG_START(super80)
	CONFIG_DEVICE(super80_cassette_getinfo)
	CONFIG_DEVICE(super80_cartslot_getinfo)
	CONFIG_DEVICE(super80_quickload_getinfo)
SYSTEM_CONFIG_END

/*    YEAR  NAME      PARENT COMPAT MACHINE INPUT     INIT       CONFIG   COMPANY       FULLNAME */
COMP( 1981, super80,  0,       0, super80,  super80,  super80,  super80, "Dick Smith","Super-80 (V1.2)" , 0)
COMP( 1981, super80d, super80, 0, super80d, super80,  super80d, super80, "Dick Smith","Super-80 (V2.2)" , 0)
COMP( 1981, super80e, super80, 0, super80d, super80,  super80,  super80, "Dick Smith","Super-80 (El Graphix 4)" , 0)
COMP( 1981, super80m, super80, 0, super80m, super80m, super80d, super80, "Dick Smith","Super-80 (8R0)" , 0)
COMP( 1981, super80v, super80, 0, super80v, super80v, super80v, super80, "Dick Smith","Super-80 (with VDUEB)" , 0)

