/*
	TX-0

	Raphael Nabet, 2004
*/

#include <math.h>
#include "driver.h"

#include "cpu/pdp1/tx0.h"
#include "includes/tx0.h"
#include "video/crt.h"


/* pointer to TX-0 RAM */
static UINT32 *tx0_memory;


/*
	driver init function

	Set up the tx0_memory pointer
*/
static DRIVER_INIT( tx0 )
{
	UINT8 *dst;

	static const unsigned char fontdata6x8[tx0_fontdata_size] =
	{	/* ASCII characters */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x20,0x20,0x20,0x20,0x00,0x20,0x00,
		0x50,0x50,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x50,0xf8,0x50,0xf8,0x50,0x00,0x00,
		0x20,0x70,0xc0,0x70,0x18,0xf0,0x20,0x00,0x40,0xa4,0x48,0x10,0x20,0x48,0x94,0x08,
		0x60,0x90,0xa0,0x40,0xa8,0x90,0x68,0x00,0x10,0x20,0x40,0x00,0x00,0x00,0x00,0x00,
		0x20,0x40,0x40,0x40,0x40,0x40,0x20,0x00,0x10,0x08,0x08,0x08,0x08,0x08,0x10,0x00,
		0x20,0xa8,0x70,0xf8,0x70,0xa8,0x20,0x00,0x00,0x20,0x20,0xf8,0x20,0x20,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x60,0x00,0x00,0x00,0xf8,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x00,0x00,0x08,0x10,0x20,0x40,0x80,0x00,0x00,
		0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00,0x10,0x30,0x10,0x10,0x10,0x10,0x10,0x00,
		0x70,0x88,0x08,0x10,0x20,0x40,0xf8,0x00,0x70,0x88,0x08,0x30,0x08,0x88,0x70,0x00,
		0x10,0x30,0x50,0x90,0xf8,0x10,0x10,0x00,0xf8,0x80,0xf0,0x08,0x08,0x88,0x70,0x00,
		0x70,0x80,0xf0,0x88,0x88,0x88,0x70,0x00,0xf8,0x08,0x08,0x10,0x20,0x20,0x20,0x00,
		0x70,0x88,0x88,0x70,0x88,0x88,0x70,0x00,0x70,0x88,0x88,0x88,0x78,0x08,0x70,0x00,
		0x00,0x00,0x30,0x30,0x00,0x30,0x30,0x00,0x00,0x00,0x30,0x30,0x00,0x30,0x30,0x60,
		0x10,0x20,0x40,0x80,0x40,0x20,0x10,0x00,0x00,0x00,0xf8,0x00,0xf8,0x00,0x00,0x00,
		0x40,0x20,0x10,0x08,0x10,0x20,0x40,0x00,0x70,0x88,0x08,0x10,0x20,0x00,0x20,0x00,
		0x70,0x88,0xb8,0xa8,0xb8,0x80,0x70,0x00,0x70,0x88,0x88,0xf8,0x88,0x88,0x88,0x00,
		0xf0,0x88,0x88,0xf0,0x88,0x88,0xf0,0x00,0x70,0x88,0x80,0x80,0x80,0x88,0x70,0x00,
		0xf0,0x88,0x88,0x88,0x88,0x88,0xf0,0x00,0xf8,0x80,0x80,0xf0,0x80,0x80,0xf8,0x00,
		0xf8,0x80,0x80,0xf0,0x80,0x80,0x80,0x00,0x70,0x88,0x80,0x98,0x88,0x88,0x70,0x00,
		0x88,0x88,0x88,0xf8,0x88,0x88,0x88,0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,
		0x08,0x08,0x08,0x08,0x88,0x88,0x70,0x00,0x88,0x90,0xa0,0xc0,0xa0,0x90,0x88,0x00,
		0x80,0x80,0x80,0x80,0x80,0x80,0xf8,0x00,0x88,0xd8,0xa8,0x88,0x88,0x88,0x88,0x00,
		0x88,0xc8,0xa8,0x98,0x88,0x88,0x88,0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00,
		0xf0,0x88,0x88,0xf0,0x80,0x80,0x80,0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x08,
		0xf0,0x88,0x88,0xf0,0x88,0x88,0x88,0x00,0x70,0x88,0x80,0x70,0x08,0x88,0x70,0x00,
		0xf8,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x88,0x88,0x88,0x88,0x88,0x88,0x70,0x00,
		0x88,0x88,0x88,0x88,0x88,0x50,0x20,0x00,0x88,0x88,0x88,0x88,0xa8,0xd8,0x88,0x00,
		0x88,0x50,0x20,0x20,0x20,0x50,0x88,0x00,0x88,0x88,0x88,0x50,0x20,0x20,0x20,0x00,
		0xf8,0x08,0x10,0x20,0x40,0x80,0xf8,0x00,0x30,0x20,0x20,0x20,0x20,0x20,0x30,0x00,
		0x40,0x40,0x20,0x20,0x10,0x10,0x08,0x08,0x30,0x10,0x10,0x10,0x10,0x10,0x30,0x00,
		0x20,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfc,
		0x40,0x20,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x70,0x08,0x78,0x88,0x78,0x00,
		0x80,0x80,0xf0,0x88,0x88,0x88,0xf0,0x00,0x00,0x00,0x70,0x88,0x80,0x80,0x78,0x00,
		0x08,0x08,0x78,0x88,0x88,0x88,0x78,0x00,0x00,0x00,0x70,0x88,0xf8,0x80,0x78,0x00,
		0x18,0x20,0x70,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0x78,0x88,0x88,0x78,0x08,0x70,
		0x80,0x80,0xf0,0x88,0x88,0x88,0x88,0x00,0x20,0x00,0x20,0x20,0x20,0x20,0x20,0x00,
		0x20,0x00,0x20,0x20,0x20,0x20,0x20,0xc0,0x80,0x80,0x90,0xa0,0xe0,0x90,0x88,0x00,
		0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0xf0,0xa8,0xa8,0xa8,0xa8,0x00,
		0x00,0x00,0xb0,0xc8,0x88,0x88,0x88,0x00,0x00,0x00,0x70,0x88,0x88,0x88,0x70,0x00,
		0x00,0x00,0xf0,0x88,0x88,0xf0,0x80,0x80,0x00,0x00,0x78,0x88,0x88,0x78,0x08,0x08,
		0x00,0x00,0xb0,0xc8,0x80,0x80,0x80,0x00,0x00,0x00,0x78,0x80,0x70,0x08,0xf0,0x00,
		0x20,0x20,0x70,0x20,0x20,0x20,0x18,0x00,0x00,0x00,0x88,0x88,0x88,0x98,0x68,0x00,
		0x00,0x00,0x88,0x88,0x88,0x50,0x20,0x00,0x00,0x00,0xa8,0xa8,0xa8,0xa8,0x50,0x00,
		0x00,0x00,0x88,0x50,0x20,0x50,0x88,0x00,0x00,0x00,0x88,0x88,0x88,0x78,0x08,0x70,
		0x00,0x00,0xf8,0x10,0x20,0x40,0xf8,0x00,0x08,0x10,0x10,0x20,0x10,0x10,0x08,0x00,
		0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x40,0x20,0x20,0x10,0x20,0x20,0x40,0x00,
		0x00,0x68,0xb0,0x00,0x00,0x00,0x00,0x00,0x20,0x50,0x20,0x50,0xa8,0x50,0x00,0x00,
	};

	/* set up memory regions */
	tx0_memory = (UINT32 *) memory_region(REGION_CPU1);

	/* set up our font */
	dst = memory_region(REGION_GFX1);

	memcpy(dst, fontdata6x8, tx0_fontdata_size);
}


static READ18_HANDLER(tx0_read_mem)
{
	return tx0_memory ? tx0_memory[offset] : 0;
}

static WRITE18_HANDLER(tx0_write_mem)
{
	if (tx0_memory)
		tx0_memory[offset] = data;
}


static ADDRESS_MAP_START(tx0_64kw_map, ADDRESS_SPACE_PROGRAM, 32)
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(tx0_read_mem, tx0_write_mem)
ADDRESS_MAP_END

static ADDRESS_MAP_START(tx0_8kw_map, ADDRESS_SPACE_PROGRAM, 32)
	AM_RANGE(0x0000, 0x1fff) AM_READWRITE(tx0_read_mem, tx0_write_mem)
ADDRESS_MAP_END


static INPUT_PORTS_START( tx0 )

	PORT_START		/* 0: various tx0 operator control panel switches */
	PORT_BIT(tx0_control, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("control panel key") PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT(tx0_stop_cyc0, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("stop at cycle 0") PORT_CODE(KEYCODE_Q)
	PORT_BIT(tx0_stop_cyc1, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("stop at cycle 1") PORT_CODE(KEYCODE_W)
	PORT_BIT(tx0_gbl_cm_sel, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CM select") PORT_CODE(KEYCODE_E)
	PORT_BIT(tx0_stop, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("stop") PORT_CODE(KEYCODE_P)
	PORT_BIT(tx0_restart, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("restart") PORT_CODE(KEYCODE_O)
	PORT_BIT(tx0_read_in, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("read in") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT(tx0_toggle_dn, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("edit next toggle switch register") PORT_CODE(KEYCODE_DOWN)
	PORT_BIT(tx0_toggle_up, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("edit previous toggle switch register") PORT_CODE(KEYCODE_UP)
	PORT_BIT(tx0_cm_sel, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("TSS CM switch") PORT_CODE(KEYCODE_A)
	PORT_BIT(tx0_lr_sel, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("TSS LR switch") PORT_CODE(KEYCODE_SLASH)

    PORT_START		/* 1: operator control panel toggle switch register switches MS */
	PORT_BIT(    0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 0") PORT_CODE(KEYCODE_S)
	PORT_BIT(    0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 1") PORT_CODE(KEYCODE_D)

    PORT_START		/* 2: operator control panel toggle switch register switches LS */
	PORT_BIT( 0100000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 2") PORT_CODE(KEYCODE_F)
	PORT_BIT( 0040000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 3") PORT_CODE(KEYCODE_G)
	PORT_BIT( 0020000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 4") PORT_CODE(KEYCODE_H)
	PORT_BIT( 0010000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 5") PORT_CODE(KEYCODE_J)
	PORT_BIT( 0004000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 6") PORT_CODE(KEYCODE_K)
	PORT_BIT( 0002000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 7") PORT_CODE(KEYCODE_L)
	PORT_BIT( 0001000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 8") PORT_CODE(KEYCODE_COLON)
	PORT_BIT( 0000400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 9") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0000200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 10") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0000100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 11") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0000040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 12") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0000020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 13") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0000010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 14") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0000004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 15") PORT_CODE(KEYCODE_M)
   	PORT_BIT( 0000002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 16") PORT_CODE(KEYCODE_COMMA)
   	PORT_BIT( 0000001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Toggle Switch Register Switch 17") PORT_CODE(KEYCODE_STOP)

    PORT_START		/* 3: typewriter codes 00-17 */
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("| _") PORT_CODE(KEYCODE_SLASH)
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(Space)") PORT_CODE(KEYCODE_SPACE)
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("= :") PORT_CODE(KEYCODE_EQUALS)
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("+ /") PORT_CODE(KEYCODE_QUOTE)
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)

    PORT_START		/* 4: typewriter codes 20-37 */
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(". )") PORT_CODE(KEYCODE_STOP)
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(". (") PORT_CODE(KEYCODE_COMMA)
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("- +") PORT_CODE(KEYCODE_MINUS)
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)

    PORT_START		/* 5: typewriter codes 40-57 */
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE)
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Tab Key") PORT_CODE(KEYCODE_TAB)
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)

    PORT_START		/* 6: typewriter codes 60-77 */
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Upper case") PORT_CODE(KEYCODE_CAPSLOCK)
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Lower Case") PORT_CODE(KEYCODE_LSHIFT)
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)

INPUT_PORTS_END


static gfx_layout fontlayout =
{
	6, 8,			/* 6*8 characters */
	tx0_charnum,	/* 96+xx characters */
	1,				/* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 }, /* straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 /* every char takes 8 consecutive bytes */
};

static gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &fontlayout, 0, 3 },
	{ -1 }	/* end of array */
};


/*
	The static palette only includes the pens for the control panel and
	the typewriter, as the CRT palette is generated dynamically.

	The CRT palette defines various levels of intensity between white and
	black.  Grey levels follow an exponential law, so that decrementing the
	color index periodically will simulate the remanence of a cathode ray tube.
*/
static const unsigned char palette[] =
{
	0xFF,0xFF,0xFF,	/* white */
	0x00,0xFF,0x00,	/* green */
	0x00,0x40,0x00,	/* dark green */
	0xFF,0x00,0x00,	/* red */
	0x80,0x80,0x80	/* light gray */
};

static const unsigned short tx0_colortable[] =
{
	pen_panel_bg, pen_panel_caption,
	pen_typewriter_bg, pen_black,
	pen_typewriter_bg, pen_red
};

/* Initialise the palette */
static PALETTE_INIT( tx0 )
{
	/* rgb components for the two color emissions */
	const double r1 = .1, g1 = .1, b1 = .924, r2 = .7, g2 = .7, b2 = .076;
	/* half period in seconds for the two color emissions */
	const double half_period_1 = .05, half_period_2 = .20;
	/* refresh period in seconds */
	const double update_period = 1./refresh_rate;
	double decay_1, decay_2;
	double cur_level_1, cur_level_2;
#ifdef MAME_DEBUG
	/* level at which we stop emulating the decay and say the pixel is black */
	double cut_level = .02;
#endif
	int i;
	int r, g, b;


	/* initialize CRT palette */

	/* compute the decay factor per refresh frame */
	decay_1 = pow(.5, update_period / half_period_1);
	decay_2 = pow(.5, update_period / half_period_2);

	cur_level_1 = cur_level_2 = 255.;	/* start with maximum level */

	for (i=pen_crt_max_intensity; i>0; i--)
	{
		/* compute the current color */
		r = (int) ((r1*cur_level_1 + r2*cur_level_2) + .5);
		g = (int) ((g1*cur_level_1 + g2*cur_level_2) + .5);
		b = (int) ((b1*cur_level_1 + b2*cur_level_2) + .5);
		/* write color in palette */
		palette_set_color(machine, i, r, g, b);
		/* apply decay for next iteration */
		cur_level_1 *= decay_1;
		cur_level_2 *= decay_2;
	}

#ifdef MAME_DEBUG
	{
		int recommended_pen_crt_num_levels;
		if (decay_1 > decay_2)
			recommended_pen_crt_num_levels = ceil(log(cut_level)/log(decay_1))+1;
		else
			recommended_pen_crt_num_levels = ceil(log(cut_level)/log(decay_2))+1;
		if (recommended_pen_crt_num_levels != pen_crt_num_levels)
			mame_printf_debug("File %s line %d: recommended value for pen_crt_num_levels is %d\n", __FILE__, __LINE__, recommended_pen_crt_num_levels);
	}
	/*if ((cur_level_1 > 255.*cut_level) || (cur_level_2 > 255.*cut_level))
		mame_printf_debug("File %s line %d: Please take higher value for pen_crt_num_levels or smaller value for decay\n", __FILE__, __LINE__);*/
#endif

	palette_set_color(machine, 0, 0, 0, 0);

	/* load static palette */
	palette_set_colors(machine, pen_crt_num_levels, palette, sizeof(palette) / sizeof(palette[0]) / 3);

	memcpy(colortable, tx0_colortable, sizeof(tx0_colortable));
}


tx0_reset_param_t tx0_reset_param =
{
	{
		tx0_io_cpy,
		tx0_io_r1l,
		tx0_io_dis,
		tx0_io_r3l,
		tx0_io_prt,
		/*tx0_io_typ*/NULL,
		tx0_io_p6h,
		tx0_io_p7h
	},
	tx0_sel,
	tx0_io_reset_callback
};


static MACHINE_DRIVER_START(tx0_64kw)

	/* basic machine hardware */
	/* TX0 CPU @ approx. 167 kHz (no master clock, but the memory cycle time is
	approximately 6usec) */
	MDRV_CPU_ADD(TX0_64KW, 166667)
	MDRV_CPU_CONFIG(tx0_reset_param)
	MDRV_CPU_PROGRAM_MAP(tx0_64kw_map, 0)
	/*MDRV_CPU_PORTS(readport, writeport)*/
	/* dummy interrupt: handles input */
	MDRV_CPU_VBLANK_INT(tx0_interrupt, 1)
	/*MDRV_CPU_PERIODIC_INT(func, rate)*/

	MDRV_SCREEN_REFRESH_RATE(refresh_rate)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	/*MDRV_INTERLEAVE(interleave)*/

	MDRV_MACHINE_START( tx0 )

	/* video hardware (includes the control panel and typewriter output) */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(virtual_width, virtual_height)
	MDRV_SCREEN_VISIBLE_AREA(0, virtual_width-1, 0, virtual_height-1)

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(pen_crt_num_levels + (sizeof(palette) / sizeof(palette[0]) / 3))
	MDRV_COLORTABLE_LENGTH(sizeof(tx0_colortable) / sizeof(tx0_colortable[0]))

	MDRV_PALETTE_INIT(tx0)
	MDRV_VIDEO_START(tx0)
	MDRV_VIDEO_EOF(crt)
	MDRV_VIDEO_UPDATE(tx0)

	/* no sound */

MACHINE_DRIVER_END

static MACHINE_DRIVER_START(tx0_8kw)

	/* basic machine hardware */
	/* TX0 CPU @ approx. 167 kHz (no master clock, but the memory cycle time is
	approximately 6usec) */
	MDRV_CPU_ADD(TX0_8KW, 166667)
	MDRV_CPU_CONFIG(tx0_reset_param)
	MDRV_CPU_PROGRAM_MAP(tx0_8kw_map, 0)
	/*MDRV_CPU_PORTS(readport, writeport)*/
	/* dummy interrupt: handles input */
	MDRV_CPU_VBLANK_INT(tx0_interrupt, 1)
	/*MDRV_CPU_PERIODIC_INT(func, rate)*/

	MDRV_SCREEN_REFRESH_RATE(refresh_rate)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	/*MDRV_INTERLEAVE(interleave)*/

	MDRV_MACHINE_START( tx0 )

	/* video hardware (includes the control panel and typewriter output) */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(virtual_width, virtual_height)
	MDRV_SCREEN_VISIBLE_AREA(0, virtual_width-1, 0, virtual_height-1)

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(pen_crt_num_levels + (sizeof(palette) / sizeof(palette[0]) / 3))
	MDRV_COLORTABLE_LENGTH(sizeof(tx0_colortable) / sizeof(tx0_colortable[0]))

	MDRV_PALETTE_INIT(tx0)
	MDRV_VIDEO_START(tx0)
	MDRV_VIDEO_EOF(crt)
	MDRV_VIDEO_UPDATE(tx0)

	/* no sound */

MACHINE_DRIVER_END

ROM_START(tx0_64kw)
	/*CPU memory space*/
	ROM_REGION(0x10000 * sizeof(UINT32),REGION_CPU1,0)
		/* Note this computer has no ROM... */

	ROM_REGION(tx0_fontdata_size, REGION_GFX1, 0)
		/* space filled with our font */
ROM_END

ROM_START(tx0_8kw)
	/*CPU memory space*/
	ROM_REGION(0x2000 * sizeof(UINT32),REGION_CPU1,0)
		/* Note this computer has no ROM... */

	ROM_REGION(tx0_fontdata_size, REGION_GFX1, 0)
		/* space filled with our font */
ROM_END

static void tx0_punchtape_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* punchtape */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_PUNCHTAPE; break;
		case DEVINFO_INT_COUNT:							info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_INIT:							info->init = device_init_tx0_tape; break;
		case DEVINFO_PTR_LOAD:							info->load = device_load_tx0_tape; break;
		case DEVINFO_PTR_UNLOAD:						info->unload = device_unload_tx0_tape; break;
		case DEVINFO_PTR_GET_DISPOSITIONS:				info->getdispositions = tx0_tape_get_open_mode; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "tap,rim"); break;
	}
}

static void tx0_printer_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* printer */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_PRINTER; break;
		case DEVINFO_INT_READABLE:						info->i = 0; break;
		case DEVINFO_INT_WRITEABLE:						info->i = 1; break;
		case DEVINFO_INT_CREATABLE:						info->i = 1; break;
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_tx0_typewriter; break;
		case DEVINFO_PTR_UNLOAD:						info->unload = device_unload_tx0_typewriter; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "typ"); break;
	}
}

static void tx0_magtape_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* magtape */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_CASSETTE; break;
		case DEVINFO_INT_READABLE:						info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:						info->i = 1; break;
		case DEVINFO_INT_CREATABLE:						info->i = 0; break;
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_INIT:							info->init = device_init_tx0_magtape; break;
		case DEVINFO_PTR_LOAD:							info->load = device_load_tx0_magtape; break;
		case DEVINFO_PTR_UNLOAD:						info->unload = device_unload_tx0_magtape; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "tap"); break;
	}
}

SYSTEM_CONFIG_START(tx0)
	CONFIG_DEVICE(tx0_punchtape_getinfo)
	CONFIG_DEVICE(tx0_magtape_getinfo)
	CONFIG_DEVICE(tx0_printer_getinfo)
SYSTEM_CONFIG_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

/*	  YEAR	NAME	  PARENT	COMPAT	MACHINE   INPUT 	INIT	CONFIG	COMPANY					  FULLNAME */
COMP( 1956, tx0_64kw, 0, 		0,		tx0_64kw, tx0,		tx0,	tx0,	"MIT", "TX-0 original demonstrator (64 kWords of RAM)" , 0)
COMP( 1962, tx0_8kw,  0, 		0,		tx0_8kw,  tx0,		tx0,	tx0,	"MIT", "TX-0 upgraded system (8 kWords of RAM)" , 0)
