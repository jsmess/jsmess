/*
    SNUG SGCPU (a.k.a. 99/4p) system (preliminary)

    This system is a reimplementation of the old ti99/4a console.  It is known
    both as the 99/4p ("peripheral box", since the system is a card to be
    inserted in the peripheral box, instead of a self contained console), and
    as the SGCPU ("Second Generation CPU", which was originally the name used
    in TI documentation to refer to either (or both) TI99/5 and TI99/8
    projects).

    The SGCPU was designed and built by the SNUG (System 99 Users Group),
    namely by Michael Becker for the hardware part and Harald Glaab for the
    software part.  It has no relationship with TI.

    The card is architectured around a 16-bit bus (vs. an 8-bit bus in every
    other TI99 system).  It includes 64kb of ROM, including a GPL interpreter,
    an internal DSR ROM which contains system-specific code, part of the TI
    extended Basic interpreter, and up to 1Mbyte of RAM.  It still includes a
    16-bit to 8-bit multiplexer in order to support extension cards designed
    for TI99/4a, but it can support 16-bit cards, too.  It does not include
    GROMs, video or sound: instead, it relies on the HSGPL and EVPC cards to
    do the job.

    TODO:
    * Test the system? Call Debug, Call XB16.
    * Implement MEM8 timings.

    2009-11-15
    This driver found to be hopelessly broken.
    1. Fixed crash in DRIVER_INIT - was trying to set up GROM when this model doesn't have any.
    2. Fixed crash in MACHINE_RESET - new cart system depends on GROM.
    3. Fixed crash when drawing the lower border - screen size changed to the same as ti99_4ev.
    Now, it produces a black screen.
    If you use memory view in the debugger, it crashes at 6000 and some higher addresses. This is
    because the memory map has references to GROM handlers.

    2010-06-19
    MZ: Driver fixed
    Important: The SGCPU card relies on a properly set up HSGPL flash memory
    card; without, it will immediately lock up. It is impossible to set it up
    from here (a bootstrap problem; you cannot start without the HSGPL).
    The best chance is to start a ti99_4ev, activate the
    HSGPL, and go through the setup process there. Copy the hsgpl.nv into this
    driver's nvram subdirectory. The contents will be directly usable for the SGCPU.
*/

#include "emu.h"
#include "deprecat.h"
#include "cpu/tms9900/tms9900.h"
#include "sound/wave.h"
#include "sound/dac.h"
#include "sound/sn76496.h"
#include "sound/tms5220.h"
#include "video/v9938.h"

#include "machine/tms9901.h"
#include "machine/tms9902.h"
#include "machine/ti99/peribox.h"

#include "imagedev/cassette.h"
#include "machine/ti99/videowrp.h"
#include "machine/ti99/sgcpu.h"
#include "machine/ti99/peribox.h"


class ti99_4p_state : public driver_device
{
public:
	ti99_4p_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};


static ADDRESS_MAP_START(memmap, AS_PROGRAM, 16)
	AM_RANGE(0x0000, 0xffff) AM_DEVREADWRITE("sgcpu_board", sgcpu_r, sgcpu_w )
ADDRESS_MAP_END

static ADDRESS_MAP_START(cru_map, AS_IO, 8)
	AM_RANGE(0x0000, 0x007f) AM_DEVREAD("tms9901", tms9901_cru_r)
	AM_RANGE(0x0080, 0x01ff) AM_DEVREAD("sgcpu_board", sgcpu_cru_r )

	AM_RANGE(0x0000, 0x03ff) AM_DEVWRITE("tms9901", tms9901_cru_w)
	AM_RANGE(0x0400, 0x0fff) AM_DEVWRITE("sgcpu_board", sgcpu_cru_w )
ADDRESS_MAP_END

/*
    Input ports, used by machine code for TI keyboard and joystick emulation.

    Since the keyboard microcontroller is not emulated, we use the TI99/4a 48-key keyboard,
    plus two optional joysticks.
*/

static INPUT_PORTS_START(ti99_4p)

	PORT_START( "SPEECH" )
	PORT_CONFNAME( 0x01, 0x01, "Speech synthesizer" )
		PORT_CONFSETTING( 0x00, DEF_STR( Off ) )
		PORT_CONFSETTING( 0x01, DEF_STR( On ) )

	PORT_START( "DISKCTRL" )
	PORT_CONFNAME( 0x07, 0x03, "Disk controller" )
		PORT_CONFSETTING(    0x00, DEF_STR( None ) )
		PORT_CONFSETTING(    0x01, "TI SD Floppy Controller" )
		PORT_CONFSETTING(    0x02, "SNUG BwG Controller" )
		PORT_CONFSETTING(    0x03, "Myarc HFDC" )
//      PORT_CONFSETTING(    0x04, "Corcomp" )

	PORT_START( "HDCTRL" )
	PORT_CONFNAME( 0x03, 0x00, "HD controller" )
		PORT_CONFSETTING(    0x00, DEF_STR( None ) )
//      PORT_CONFSETTING(    0x01, "Nouspikel IDE Controller" )
//      PORT_CONFSETTING(    0x02, "WHTech SCSI Controller" )
	PORT_CONFNAME( 0x08, 0x00, "USB-SM card" )
		PORT_CONFSETTING(    0x00, DEF_STR( Off ) )
		PORT_CONFSETTING(    0x08, DEF_STR( On ) )

	PORT_START( "SERIAL" )
	PORT_CONFNAME( 0x03, 0x00, "Serial/Parallel interface" )
		PORT_CONFSETTING(    0x00, DEF_STR( None ) )
		PORT_CONFSETTING(    0x01, "TI RS-232 card" )

	PORT_START( "EXTCARD" )
	PORT_CONFNAME( 0x03, 0x02, "HSGPL extension" )
		PORT_CONFSETTING(    0x01, "Flash" )
		PORT_CONFSETTING(    0x02, DEF_STR( On ) )

	// We do not want to show this setting; makes only sense for Geneve
	PORT_START( "MODE" )
	PORT_CONFNAME( 0x01, 0x00, "Ext. cards modification" ) PORT_CONDITION( "HFDCDIP", 0xff, PORTCOND_EQUALS, GM_NEVER )
		PORT_CONFSETTING(    0x00, "Standard" )
		PORT_CONFSETTING(    GENMOD, "GenMod" )

	PORT_START( "HFDCDIP" )
	PORT_DIPNAME( 0xff, 0x55, "HFDC drive config" ) PORT_CONDITION( "DISKCTRL", 0x07, PORTCOND_EQUALS, 0x03 )
		PORT_DIPSETTING( 0x00, "40 track, 16 ms")
		PORT_DIPSETTING( 0xaa, "40 track, 8 ms")
		PORT_DIPSETTING( 0x55, "80 track, 2 ms")
		PORT_DIPSETTING( 0xff, "80 track HD, 2 ms")

	PORT_START( "V9938-MEM" )
	PORT_CONFNAME( 0x01, 0x00, "V9938 RAM size" )
		PORT_CONFSETTING(	0x00, "128 KiB" )
		PORT_CONFSETTING(	0x01, "192 KiB" )

	PORT_START( "DRVSPD" )
	PORT_CONFNAME( 0x01, 0x01, "Floppy and HD speed" ) PORT_CONDITION( "DISKCTRL", 0x07, PORTCOND_EQUALS, 0x03 )
		PORT_CONFSETTING( 0x00, "No delay")
		PORT_CONFSETTING( 0x01, "Realistic")

	PORT_START( "EVPC-SW1" )
	PORT_DIPNAME( 0x01, 0x00, "EVPC video mode" ) PORT_CONDITION( "EVPC-SW8", 0x01, PORTCOND_EQUALS, 0x00 )
		PORT_DIPSETTING(    0x00, "PAL" )
		PORT_DIPSETTING(    0x01, "NTSC" )

	PORT_START( "EVPC-SW3" )
	PORT_DIPNAME( 0x01, 0x00, "EVPC charset" ) PORT_CONDITION( "EVPC-SW8", 0x01, PORTCOND_EQUALS, 0x00 )
		PORT_DIPSETTING(    0x00, DEF_STR( International ))
		PORT_DIPSETTING(    0x01, DEF_STR( German ))

	PORT_START( "EVPC-SW4" )
	PORT_DIPNAME( 0x01, 0x00, "EVPC VDP RAM" ) PORT_CONDITION( "EVPC-SW8", 0x01, PORTCOND_EQUALS, 0x00 )
		PORT_DIPSETTING(    0x00, "shifted" )
		PORT_DIPSETTING(    0x01, "not shifted" )

	PORT_START( "EVPC-SW8" )
	PORT_DIPNAME( 0x01, 0x00, "EVPC Configuration" )
		PORT_DIPSETTING(    0x00, "DIP" )
		PORT_DIPSETTING(    0x01, "NOVRAM" )

	/* 3 ports for mouse */
	PORT_START("MOUSEX") /* Mouse - X AXIS */
		PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)

	PORT_START("MOUSEY") /* Mouse - Y AXIS */
		PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)

	PORT_START("MOUSE0") /* Mouse - buttons */
		PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Mouse Button 1") PORT_PLAYER(1)
		PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Mouse Button 2") PORT_PLAYER(1)

	/* 4 ports for keyboard and joystick */
	PORT_START("KEY0")	/* col 0 */
		PORT_BIT(0x0088, IP_ACTIVE_LOW, IPT_UNUSED)
		/* The original control key is located on the left, but we accept the right control key as well */
		PORT_BIT(0x0040, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL)
		/* TI99/4a has a second shift key which maps the same */
		PORT_BIT(0x0020, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
		/* The original control key is located on the right, but we accept the left function key as well */
		PORT_BIT(0x0010, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("FCTN") PORT_CODE(KEYCODE_RALT) PORT_CODE(KEYCODE_LALT) PORT_CHAR(UCHAR_SHIFT_2)
		PORT_BIT(0x0004, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
		PORT_BIT(0x0002, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(SPACE)") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
		PORT_BIT(0x0001, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("= + QUIT") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('+')
				/* col 1 */
		PORT_BIT(0x8000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("x X (DOWN)") PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')
		PORT_BIT(0x4000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("w W ~") PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W') PORT_CHAR('~')
		PORT_BIT(0x2000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("s S (LEFT)") PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
		PORT_BIT(0x1000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 @ INS") PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('@')
		PORT_BIT(0x0800, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 ( BACK") PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR('(')
		PORT_BIT(0x0400, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("o O '") PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O') PORT_CHAR('\'')
		PORT_BIT(0x0200, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("l L") PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
		PORT_BIT(0x0100, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')

	PORT_START("KEY1")	/* col 2 */
		PORT_BIT(0x0080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("c C `") PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C') PORT_CHAR('`')
		PORT_BIT(0x0040, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("e E (UP)") PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
		PORT_BIT(0x0020, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("d D (RIGHT)") PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
		PORT_BIT(0x0010, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 # ERASE") PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
		PORT_BIT(0x0008, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 * REDO") PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('*')
		PORT_BIT(0x0004, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("i I ?") PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I') PORT_CHAR('?')
		PORT_BIT(0x0002, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("k K") PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
		PORT_BIT(0x0001, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
				/* col 3 */
		PORT_BIT(0x8000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("v V") PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
		PORT_BIT(0x4000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("r R [") PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R') PORT_CHAR('[')
		PORT_BIT(0x2000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("f F {") PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F') PORT_CHAR('{')
		PORT_BIT(0x1000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 $ CLEAR") PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
		PORT_BIT(0x0800, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 & AID") PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('&')
		PORT_BIT(0x0400, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("u U _") PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U') PORT_CHAR('_')
		PORT_BIT(0x0200, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("j J") PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
		PORT_BIT(0x0100, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("m M") PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')

	PORT_START("KEY2")	/* col 4 */
		PORT_BIT(0x0080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("b B") PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
		PORT_BIT(0x0040, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("t T ]") PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T') PORT_CHAR(']')
		PORT_BIT(0x0020, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("g G }") PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G') PORT_CHAR('}')
		PORT_BIT(0x0010, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 % BEGIN") PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
		PORT_BIT(0x0008, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 ^ PROC'D") PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('^')
		PORT_BIT(0x0004, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("y Y") PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
		PORT_BIT(0x0002, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("h H") PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
		PORT_BIT(0x0001, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("n N") PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
				/* col 5 */
		PORT_BIT(0x8000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("z Z \\") PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z') PORT_CHAR('\\')
		PORT_BIT(0x4000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("q Q") PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
		PORT_BIT(0x2000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("a A |") PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A') PORT_CHAR('|')
		PORT_BIT(0x1000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 ! DEL") PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
		PORT_BIT(0x0800, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0 )") PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR(')')
		PORT_BIT(0x0400, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("p P \"") PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P') PORT_CHAR('\"')
		PORT_BIT(0x0200, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("; :") PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR(':')
		PORT_BIT(0x0100, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/ -") PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('-')

	PORT_START("KEY3")	/* col 6: "wired handset 1" (= joystick 1) */
		PORT_BIT(0x00E0, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_UP/*, "(1UP)", CODE_NONE, OSD_JOY_UP*/) PORT_PLAYER(1)
		PORT_BIT(0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN/*, "(1DOWN)", CODE_NONE, OSD_JOY_DOWN, 0*/) PORT_PLAYER(1)
		PORT_BIT(0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT/*, "(1RIGHT)", CODE_NONE, OSD_JOY_RIGHT, 0*/) PORT_PLAYER(1)
		PORT_BIT(0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT/*, "(1LEFT)", CODE_NONE, OSD_JOY_LEFT, 0*/) PORT_PLAYER(1)
		PORT_BIT(0x0001, IP_ACTIVE_LOW, IPT_BUTTON1/*, "(1FIRE)", CODE_NONE, OSD_JOY_FIRE, 0*/) PORT_PLAYER(1)
			/* col 7: "wired handset 2" (= joystick 2) */
		PORT_BIT(0xE000, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP/*, "(2UP)", CODE_NONE, OSD_JOY2_UP, 0*/) PORT_PLAYER(2)
		PORT_BIT(0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN/*, "(2DOWN)", CODE_NONE, OSD_JOY2_DOWN, 0*/) PORT_PLAYER(2)
		PORT_BIT(0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT/*, "(2RIGHT)", CODE_NONE, OSD_JOY2_RIGHT, 0*/) PORT_PLAYER(2)
		PORT_BIT(0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT/*, "(2LEFT)", CODE_NONE, OSD_JOY2_LEFT, 0*/) PORT_PLAYER(2)
		PORT_BIT(0x0100, IP_ACTIVE_LOW, IPT_BUTTON1/*, "(2FIRE)", CODE_NONE, OSD_JOY2_FIRE, 0*/) PORT_PLAYER(2)

	PORT_START("ALPHA")	/* one more port for Alpha line */
		PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Alpha Lock") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE

INPUT_PORTS_END

DRIVER_INIT( ti99_4p )
{
}

MACHINE_START( ti99_4p )
{
}

/*
    Reset the machine.
*/
MACHINE_RESET( ti99_4p )
{
	tms9901_set_single_int(machine.device("tms9901"), 12, 0);
}

INTERRUPT_GEN( ti99_4p_hblank_interrupt )
{
	v9938_interrupt(device->machine(), 0);
}

/*
    Machine description.
*/
static MACHINE_CONFIG_START( ti99_4p_60hz, ti99_4p_state )
	/* basic machine hardware */
	/* TMS9900 CPU @ 3.0 MHz */
	MCFG_CPU_ADD("maincpu", TMS9900, 3000000)
	MCFG_CPU_PROGRAM_MAP(memmap)
	MCFG_CPU_IO_MAP(cru_map)
	MCFG_CPU_VBLANK_INT_HACK(ti99_4p_hblank_interrupt, 262)	/* 262.5 in 60Hz, 312.5 in 50Hz */

	/* video hardware */
	MCFG_TI_V9938_ADD("video", 60, "screen", 2500, 512+32, (212+28)*2, tms9901_sg_set_int2)

	MCFG_MACHINE_START( ti99_4p )
	MCFG_MACHINE_RESET( ti99_4p )

// Didn't work, probably just done wrong by me:
//  MCFG_TIMER_ADD_SCANLINE("v9938_scanline", ti99_4ev_scanline_interrupt , "screen", 0, 1)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("dac", DAC, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)
	MCFG_SOUND_WAVE_ADD("wave", "cassette")
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)
	MCFG_SOUND_ADD("soundgen", SN76496, 3579545)	/* 3.579545 MHz */
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)

	/* tms9901 */
	MCFG_TMS9901_ADD("tms9901", tms9901_wiring_ti99_4p)

	/* devices */
	MCFG_PBOXSG_ADD( "peribox", card_extint, card_notconnected, card_ready )
	MCFG_SGCPUB_ADD( "sgcpu_board" )
	MCFG_CASSETTE_ADD( "cassette", default_cassette_config )
MACHINE_CONFIG_END


ROM_START(ti99_4p)
	/*CPU memory space*/
	ROM_REGION16_BE(0x10000, "maincpu", 0)
	ROM_LOAD16_BYTE("sgcpu_hb.bin", 0x0000, 0x8000, CRC(aa100730) SHA1(35e585b2dcd3f2a0005bebb15ede6c5b8c787366) ) /* system ROMs */
	ROM_LOAD16_BYTE("sgcpu_lb.bin", 0x0001, 0x8000, CRC(2a5dc818) SHA1(dec141fe2eea0b930859cbe1ebd715ac29fa8ecb) ) /* system ROMs */
ROM_END

/*    YEAR  NAME      PARENT   COMPAT   MACHINE      INPUT    INIT      COMPANY     FULLNAME */
COMP( 1996, ti99_4p,  0,	   0,		ti99_4p_60hz, ti99_4p, ti99_4p, "System 99 Users Group",		"SGCPU (a.k.a. 99/4P)" , 0 )
