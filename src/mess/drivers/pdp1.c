/*

Driver for a PDP1 emulator.

    Digital Equipment Corporation
    Brian Silverman (original Java Source)
    Vadim Gerasimov (original Java Source)
    Chris Salomon (MESS driver)
    Raphael Nabet (MESS driver)

Initially, this was a conversion of a JAVA emulator
(although code has been edited extensively ever since).
I have tried contacting the author, but heard as yet nothing of him,
so I don't know if it all right with him, but after all -> he did
release the source, so hopefully everything will be fine (no his
name is not Marat).

Note: naturally I have no PDP1, I have never seen one, nor have I any
programs for it.

The first supported program was:

SPACEWAR!

The first Videogame EVER!

When I saw the java emulator, running that game I was quite intrigued to
include a driver for MESS.
I think the historical value of SPACEWAR! is enormous.

Two other programs are supported: Munching squares and LISP.

Added Debugging and Disassembler...


Also:
ftp://minnie.cs.adfa.oz.au/pub/PDP-11/Sims/Supnik_2.3/software/lispswre.tar.gz
Is a packet which includes the original LISP as source and
binary form plus a makro assembler for PDP1 programs.

For more documentation look at the source for the driver,
and the cpu/pdp1/pdp1.c file (information about the whereabouts of information
and the java source).

*/

#include <math.h>

#include "emu.h"
#include "cpu/pdp1/pdp1.h"
#include "includes/pdp1.h"
#include "video/crt.h"

/*
 *
 * The loading storing OS... is not emulated (I haven't a clue where to
 * get programs for the machine)
 *
 */


static ADDRESS_MAP_START(pdp1_map, ADDRESS_SPACE_PROGRAM, 32)
	AM_RANGE(0x00000, 0x3ffff) AM_RAM
ADDRESS_MAP_END

static INPUT_PORTS_START( pdp1 )
	PORT_START("SPACEWAR")		/* 0: spacewar controllers */
	PORT_BIT( ROTATE_LEFT_PLAYER1, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_NAME("Spin Left Player 1") PORT_CODE(KEYCODE_A) PORT_CODE(JOYCODE_X_LEFT_SWITCH)
	PORT_BIT( ROTATE_RIGHT_PLAYER1, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_NAME("Spin Right Player 1") PORT_CODE(KEYCODE_S) PORT_CODE(JOYCODE_X_RIGHT_SWITCH)
	PORT_BIT( THRUST_PLAYER1, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Thrust Player 1") PORT_CODE(KEYCODE_D) PORT_CODE(JOYCODE_BUTTON1)
	PORT_BIT( FIRE_PLAYER1, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Fire Player 1") PORT_CODE(KEYCODE_F) PORT_CODE(JOYCODE_BUTTON2)
	PORT_BIT( ROTATE_LEFT_PLAYER2, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_NAME("Spin Left Player 2") PORT_CODE(KEYCODE_LEFT) PORT_CODE(JOYCODE_X_LEFT_SWITCH ) PORT_PLAYER(2)
	PORT_BIT( ROTATE_RIGHT_PLAYER2, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_NAME("Spin Right Player 2") PORT_CODE(KEYCODE_RIGHT) PORT_CODE(JOYCODE_X_RIGHT_SWITCH ) PORT_PLAYER(2)
	PORT_BIT( THRUST_PLAYER2, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Thrust Player 2") PORT_CODE(KEYCODE_UP) PORT_CODE(JOYCODE_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( FIRE_PLAYER2, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Fire Player 2") PORT_CODE(KEYCODE_DOWN) PORT_CODE(JOYCODE_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( HSPACE_PLAYER1, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("Hyperspace Player 1") PORT_CODE(KEYCODE_Z) PORT_CODE(JOYCODE_BUTTON3)
	PORT_BIT( HSPACE_PLAYER2, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("Hyperspace Player 2") PORT_CODE(KEYCODE_SLASH) PORT_CODE(JOYCODE_BUTTON3 ) PORT_PLAYER(2)

	PORT_START("CSW")		/* 1: various pdp1 operator control panel switches */
	PORT_BIT(pdp1_control, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("control panel key") PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT(pdp1_extend, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("extend") PORT_CODE(KEYCODE_TILDE)
	PORT_BIT(pdp1_start_nobrk, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("start (sequence break disabled)") PORT_CODE(KEYCODE_U)
	PORT_BIT(pdp1_start_brk, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("start (sequence break enabled)") PORT_CODE(KEYCODE_I)
	PORT_BIT(pdp1_stop, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("stop") PORT_CODE(KEYCODE_O)
	PORT_BIT(pdp1_continue, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("continue") PORT_CODE(KEYCODE_P)
	PORT_BIT(pdp1_examine, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("examine") PORT_CODE(KEYCODE_OPENBRACE)
	PORT_BIT(pdp1_deposit, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("deposit") PORT_CODE(KEYCODE_CLOSEBRACE)
	PORT_BIT(pdp1_read_in, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("read in") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT(pdp1_reader, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("reader")
	PORT_BIT(pdp1_tape_feed, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("tape feed")
	PORT_BIT(pdp1_single_step, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("single step") PORT_CODE(KEYCODE_STOP)
	PORT_BIT(pdp1_single_inst, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("single inst") PORT_CODE(KEYCODE_SLASH)

	PORT_START("SENSE")		/* 2: operator control panel sense switches */
	PORT_DIPNAME(	  040, 000, "Sense Switch 1") PORT_CODE(KEYCODE_1_PAD)
	PORT_DIPSETTING(    000, DEF_STR( Off ) )
	PORT_DIPSETTING(    040, DEF_STR( On ) )
	PORT_DIPNAME(	  020, 000, "Sense Switch 2") PORT_CODE(KEYCODE_2_PAD)
	PORT_DIPSETTING(    000, DEF_STR( Off ) )
	PORT_DIPSETTING(    020, DEF_STR( On ) )
	PORT_DIPNAME(	  010, 000, "Sense Switch 3") PORT_CODE(KEYCODE_3_PAD)
	PORT_DIPSETTING(    000, DEF_STR( Off ) )
	PORT_DIPSETTING(    010, DEF_STR( On ) )
	PORT_DIPNAME(	  004, 000, "Sense Switch 4") PORT_CODE(KEYCODE_4_PAD)
	PORT_DIPSETTING(    000, DEF_STR( Off ) )
	PORT_DIPSETTING(    004, DEF_STR( On ) )
	PORT_DIPNAME(	  002, 002, "Sense Switch 5") PORT_CODE(KEYCODE_5_PAD)
	PORT_DIPSETTING(    000, DEF_STR( Off ) )
	PORT_DIPSETTING(    002, DEF_STR( On ) )
	PORT_DIPNAME(	  001, 000, "Sense Switch 6") PORT_CODE(KEYCODE_6_PAD)
	PORT_DIPSETTING(    000, DEF_STR( Off ) )
	PORT_DIPSETTING(    001, DEF_STR( On ) )

	PORT_START("TSTADD")		/* 3: operator control panel test address switches */
	PORT_BIT( 0100000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Extension Test Address Switch 3") PORT_CODE(KEYCODE_1)
	PORT_BIT( 0040000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Extension Test Address Switch 4") PORT_CODE(KEYCODE_2)
	PORT_BIT( 0020000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Extension Test Address Switch 5") PORT_CODE(KEYCODE_3)
	PORT_BIT( 0010000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Extension Test Address Switch 6") PORT_CODE(KEYCODE_4)
	PORT_BIT( 0004000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test Address Switch 7") PORT_CODE(KEYCODE_5)
	PORT_BIT( 0002000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test Address Switch 8") PORT_CODE(KEYCODE_6)
	PORT_BIT( 0001000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test Address Switch 9") PORT_CODE(KEYCODE_7)
	PORT_BIT( 0000400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test Address Switch 10") PORT_CODE(KEYCODE_8)
	PORT_BIT( 0000200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test Address Switch 11") PORT_CODE(KEYCODE_9)
	PORT_BIT( 0000100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test Address Switch 12") PORT_CODE(KEYCODE_0)
	PORT_BIT( 0000040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test Address Switch 13") PORT_CODE(KEYCODE_MINUS)
	PORT_BIT( 0000020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test Address Switch 14") PORT_CODE(KEYCODE_EQUALS)
	PORT_BIT( 0000010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test Address Switch 15") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0000004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test Address Switch 16") PORT_CODE(KEYCODE_W)
	PORT_BIT( 0000002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test Address Switch 17") PORT_CODE(KEYCODE_E)
	PORT_BIT( 0000001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test Address Switch 18") PORT_CODE(KEYCODE_R)

	PORT_START("TWDMSB")		/* 4: operator control panel test word switches MSB */
	PORT_BIT(    0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test Word Switch 1") PORT_CODE(KEYCODE_A)
	PORT_BIT(    0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test Word Switch 2") PORT_CODE(KEYCODE_S)

	PORT_START("TWDLSB")		/* 5: operator control panel test word switches LSB */
	PORT_BIT( 0100000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test Word Switch 3") PORT_CODE(KEYCODE_D)
	PORT_BIT( 0040000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test Word Switch 4") PORT_CODE(KEYCODE_F)
	PORT_BIT( 0020000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test Word Switch 5") PORT_CODE(KEYCODE_G)
	PORT_BIT( 0010000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test Word Switch 6") PORT_CODE(KEYCODE_H)
	PORT_BIT( 0004000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test Word Switch 7") PORT_CODE(KEYCODE_J)
	PORT_BIT( 0002000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test Word Switch 8") PORT_CODE(KEYCODE_K)
	PORT_BIT( 0001000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test Word Switch 9") PORT_CODE(KEYCODE_L)
	PORT_BIT( 0000400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test Word Switch 10") PORT_CODE(KEYCODE_COLON)
	PORT_BIT( 0000200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test Word Switch 11") PORT_CODE(KEYCODE_QUOTE)
	PORT_BIT( 0000100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test Word Switch 12") PORT_CODE(KEYCODE_BACKSLASH)
	PORT_BIT( 0000040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test Word Switch 13") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0000020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test Word Switch 14") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0000010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test Word Switch 15") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0000004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test Word Switch 16") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0000002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test Word Switch 17") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0000001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Test Word Switch 18") PORT_CODE(KEYCODE_N)

	/*
        Note that I can see 2 additional keys whose purpose is unknown to me.
        The caps look like "MAR REL" for the leftmost one and "MAR SET" for
        rightmost one: maybe they were used to set the margin (I don't have the
        manual for the typewriter). */

	PORT_START("TWR0")		/* 6: typewriter codes 00-17 */
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(Space)") PORT_CODE(KEYCODE_SPACE)
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1 \"") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2 '") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3 ~") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4 (implies)") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5 (or)") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6 (and)") PORT_CODE(KEYCODE_6)
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7 <") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8 >") PORT_CODE(KEYCODE_8)
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9 (up arrow)") PORT_CODE(KEYCODE_9)

	PORT_START("TWR1")		/* 7: typewriter codes 20-37 */
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0 (right arrow)") PORT_CODE(KEYCODE_0)
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH)
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(", =") PORT_CODE(KEYCODE_COMMA)
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Tab Key") PORT_CODE(KEYCODE_TAB)

	PORT_START("TWR2")		/* 8: typewriter codes 40-57 */
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(non-spacing middle dot) _") PORT_CODE(KEYCODE_QUOTE)
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("- +") PORT_CODE(KEYCODE_COLON)
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(") ]") PORT_CODE(KEYCODE_EQUALS)
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(non-spacing overstrike) |") PORT_CODE(KEYCODE_OPENBRACE)
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("( [") PORT_CODE(KEYCODE_MINUS)

	PORT_START("TWR3")		/* 9: typewriter codes 60-77 */
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Lower Case") PORT_CODE(KEYCODE_LSHIFT)
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(". (multiply)") PORT_CODE(KEYCODE_STOP)
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Upper case") PORT_CODE(KEYCODE_RSHIFT)
	/* hack to support my macintosh which does not differentiate the  Right Shift key */
	/* PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Upper case") PORT_CODE(KEYCODE_CAPSLOCK) */
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE)
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER)

	PORT_START("CFG")		/* 10: pseudo-input port with config */
	PORT_DIPNAME( 0x0003, 0x0002, "RAM size")
	PORT_DIPSETTING(   0x0000, "4kw" )
	PORT_DIPSETTING(   0x0001, "32kw")
	PORT_DIPSETTING(   0x0002, "64kw")
	PORT_DIPNAME( 0x0004, 0x0000, "Hardware multiply")
	PORT_DIPSETTING(   0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0004, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0000, "Hardware divide")
	PORT_DIPSETTING(   0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0000, "Type 20 sequence break system")
	PORT_DIPSETTING(   0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0010, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0000, "Type 32 light pen") PORT_CODE(KEYCODE_ENTER_PAD)
	PORT_DIPSETTING(   0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x0020, DEF_STR( On ) )

	PORT_START("LIGHTPEN")	/* 11: pseudo-input port with lightpen status */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("select larger light pen tip") PORT_CODE(KEYCODE_PLUS_PAD)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("select smaller light pen tip") PORT_CODE(KEYCODE_MINUS_PAD)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("light pen down")

	PORT_START("LIGHTX") /* 12: lightpen - X AXIS */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1) PORT_RESET

	PORT_START("LIGHTY") /* 13: lightpen - Y AXIS */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1) PORT_RESET
INPUT_PORTS_END


static const gfx_layout fontlayout =
{
	6, 8,			/* 6*8 characters */
	pdp1_charnum,	/* 96+4 characters */
	1,				/* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 }, /* straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 /* every char takes 8 consecutive bytes */
};


/*
    The static palette only includes the pens for the control panel and
    the typewriter, as the CRT palette is generated dynamically.

    The CRT palette defines various levels of intensity between white and
    black.  Grey levels follow an exponential law, so that decrementing the
    color index periodically will simulate the remanence of a cathode ray tube.
*/
static const UINT8 pdp1_colors[] =
{
	0x00,0x00,0x00,	/* black */
	0xFF,0xFF,0xFF,	/* white */
	0x00,0xFF,0x00,	/* green */
	0x00,0x40,0x00,	/* dark green */
	0xFF,0x00,0x00,	/* red */
	0x80,0x80,0x80	/* light gray */
};

static const UINT8 pdp1_palette[] =
{
	pen_panel_bg, pen_panel_caption,
	pen_typewriter_bg, pen_black,
	pen_typewriter_bg, pen_red
};

static const UINT8 total_colors_needed = pen_crt_num_levels + sizeof(pdp1_colors) / 3;

static GFXDECODE_START( pdp1 )
	GFXDECODE_ENTRY( "gfx1", 0, fontlayout, pen_crt_num_levels + sizeof(pdp1_colors) / 3, 3 )
GFXDECODE_END

/* Initialise the palette */
static PALETTE_INIT( pdp1 )
{
	/* rgb components for the two color emissions */
	const double r1 = .1, g1 = .1, b1 = .924, r2 = .7, g2 = .7, b2 = .076;
	/* half period in seconds for the two color emissions */
	const double half_period_1 = .05, half_period_2 = .20;
	/* refresh period in seconds */
	const double update_period = 1./refresh_rate;
	double decay_1, decay_2;
	double cur_level_1, cur_level_2;
	UINT8 i, r, g, b;

	machine->colortable = colortable_alloc(machine, total_colors_needed);

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
		colortable_palette_set_color(machine->colortable, i, MAKE_RGB(r, g, b));
		/* apply decay for next iteration */
		cur_level_1 *= decay_1;
		cur_level_2 *= decay_2;
	}

	colortable_palette_set_color(machine->colortable, 0, MAKE_RGB(0, 0, 0));

	/* load static palette */
	for ( i = 0; i < 6; i++ )
	{
		r = pdp1_colors[i*3]; g = pdp1_colors[i*3+1]; b = pdp1_colors[i*3+2];
		colortable_palette_set_color(machine->colortable, pen_crt_num_levels + i, MAKE_RGB(r, g, b));
	}

	/* copy colortable to palette */
	for( i = 0; i < total_colors_needed; i++ )
		colortable_entry_set_value(machine->colortable, i, i);

	/* set up palette for text */
	for( i = 0; i < 6; i++ )
		colortable_entry_set_value(machine->colortable, total_colors_needed + i, pdp1_palette[i]);
}


static MACHINE_DRIVER_START(pdp1)

	/* basic machine hardware */
	/* PDP1 CPU @ 200 kHz (no master clock, but the instruction and memory rate is 200 kHz) */
	MDRV_CPU_ADD("maincpu", PDP1, 1000000/*the CPU core uses microsecond counts*/)
	MDRV_CPU_CONFIG(pdp1_reset_param)
	MDRV_CPU_PROGRAM_MAP(pdp1_map)
	MDRV_CPU_VBLANK_INT("screen", pdp1_interrupt)	/* dummy interrupt: handles input */

	MDRV_MACHINE_START( pdp1 )
	MDRV_MACHINE_RESET( pdp1 )

	/* video hardware (includes the control panel and typewriter output) */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(refresh_rate)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(virtual_width, virtual_height)
	MDRV_SCREEN_VISIBLE_AREA(0, virtual_width-1, 0, virtual_height-1)

	MDRV_GFXDECODE(pdp1)
	MDRV_PALETTE_LENGTH(pen_crt_num_levels + sizeof(pdp1_colors) / 3 + sizeof(pdp1_palette))

	MDRV_PALETTE_INIT(pdp1)
	MDRV_VIDEO_START(pdp1)
	MDRV_VIDEO_EOF(crt)
	MDRV_VIDEO_UPDATE(pdp1)
MACHINE_DRIVER_END

/*
    pdp1 can address up to 65336 18 bit words when extended (4096 otherwise).
*/
ROM_START(pdp1)
	ROM_REGION(pdp1_fontdata_size, "gfx1", ROMREGION_ERASEFF)
		/* space filled with our font */
ROM_END
/*
static void pdp1_punchtape_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
    switch(state)
    {
        case MESS_DEVINFO_INT_TYPE:                         info->i = IO_PUNCHTAPE; break;
        case MESS_DEVINFO_INT_COUNT:                            info->i = 2; break;

        case MESS_DEVINFO_PTR_START:                            info->start = DEVICE_START_NAME(pdp1_tape); break;
        case MESS_DEVINFO_PTR_LOAD:                         info->load = DEVICE_IMAGE_LOAD_NAME(pdp1_tape); break;
        case MESS_DEVINFO_PTR_UNLOAD:                       info->unload = DEVICE_IMAGE_UNLOAD_NAME(pdp1_tape); break;
        case MESS_DEVINFO_PTR_GET_DISPOSITIONS:             info->getdispositions = pdp1_get_open_mode; break;

        case MESS_DEVINFO_STR_FILE_EXTENSIONS:              strcpy(info->s = device_temp_str(), "tap,rim"); break;
    }
}

static void pdp1_printer_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
    switch(state)
    {
        case MESS_DEVINFO_INT_TYPE:                         info->i = IO_PRINTER; break;
        case MESS_DEVINFO_INT_READABLE:                     info->i = 0; break;
        case MESS_DEVINFO_INT_WRITEABLE:                        info->i = 1; break;
        case MESS_DEVINFO_INT_CREATABLE:                        info->i = 1; break;
        case MESS_DEVINFO_INT_COUNT:                            info->i = 1; break;

        case MESS_DEVINFO_PTR_LOAD:                         info->load = DEVICE_IMAGE_LOAD_NAME(pdp1_typewriter); break;
        case MESS_DEVINFO_PTR_UNLOAD:                       info->unload = DEVICE_IMAGE_UNLOAD_NAME(pdp1_typewriter); break;

        case MESS_DEVINFO_STR_FILE_EXTENSIONS:              strcpy(info->s = device_temp_str(), "typ"); break;
    }
}

static void pdp1_cylinder_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
    switch(state)
    {
        case MESS_DEVINFO_INT_TYPE:                         info->i = IO_CYLINDER; break;
        case MESS_DEVINFO_INT_READABLE:                     info->i = 1; break;
        case MESS_DEVINFO_INT_WRITEABLE:                        info->i = 1; break;
        case MESS_DEVINFO_INT_CREATABLE:                        info->i = 0; break;
        case MESS_DEVINFO_INT_COUNT:                            info->i = 1; break;

        case MESS_DEVINFO_PTR_LOAD:                         info->load = DEVICE_IMAGE_LOAD_NAME(pdp1_drum); break;
        case MESS_DEVINFO_PTR_UNLOAD:                       info->unload = DEVICE_IMAGE_UNLOAD_NAME(pdp1_drum); break;

        case MESS_DEVINFO_STR_FILE_EXTENSIONS:              strcpy(info->s = device_temp_str(), "drm"); break;
    }
}

static SYSTEM_CONFIG_START(pdp1)
    CONFIG_DEVICE(pdp1_punchtape_getinfo)
    CONFIG_DEVICE(pdp1_printer_getinfo)
    CONFIG_DEVICE(pdp1_cylinder_getinfo)
SYSTEM_CONFIG_END
*/

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME      PARENT    COMPAT  MACHINE   INPUT     INIT    COMPANY FULLNAME */
COMP( 1961, pdp1,	  0,		0,		pdp1,	  pdp1, 	0,		"Digital Equipment Corporation",  "PDP-1" , GAME_NO_SOUND | GAME_NOT_WORKING)
