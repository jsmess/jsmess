/***************************************************************************
   
        Aamber Pegasus computer (New Zealand)

	http://web.mac.com/lord_philip/aamber_pegasus/Aamber_Pegasus.html

        23/04/2010 Skeleton driver.

	Each copy of the monitor rom was made for an individual machine.
	Therefore the only way to emulate is to hack out the protection check.

****************************************************************************/

#include "emu.h"
#include "cpu/m6809/m6809.h"
#include "machine/6821pia.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "machine/ctronics.h"
#include "includes/pegasus.h"

static UINT8 pegasus_kbd_row = 0;
static UINT8 pegasus_kbd_irq = 1;
static UINT8 pegasus_control_bits = 0;
static running_device *pegasus_cass;


static TIMER_DEVICE_CALLBACK( pegasus_firq )
{
	running_device *cpu = devtag_get_device( timer->machine, "maincpu" );
	cpu_set_input_line(cpu, M6809_FIRQ_LINE, HOLD_LINE);
}

static WRITE_LINE_DEVICE_HANDLER( pegasus_firq_clr )
{
	cputag_set_input_line(device->machine, "maincpu", M6809_FIRQ_LINE, CLEAR_LINE);
}

static READ8_DEVICE_HANDLER( pegasus_keyboard_r )
{
	static const char *const keynames[] = { "X0", "X1", "X2", "X3", "X4", "X5", "X6", "X7" };
	UINT8 bit,data = 0xff;
	for (bit = 0; bit < 8; bit++)
		if (!BIT(pegasus_kbd_row, bit)) data &= input_port_read(device->machine, keynames[bit]);

	pegasus_kbd_irq = (data == 0xff) ? 1 : 0;
	if (pegasus_control_bits & 8) data<<=4;
	return data;
}

static WRITE8_DEVICE_HANDLER( pegasus_keyboard_w )
{
	pegasus_kbd_row = data;
}

static WRITE8_DEVICE_HANDLER( pegasus_controls )
{
/*	Bit 0 - Blank - Video blanking
	Bit 1 - Char - select character rom or ram
	Bit 2 - Page - ?
	Bit 3 - Asc - Select which half of the keyboard to read
*/

	pegasus_control_bits = data;
}

static READ_LINE_DEVICE_HANDLER( pegasus_keyboard_irq )
{
	return pegasus_kbd_irq;
}

static READ_LINE_DEVICE_HANDLER( pegasus_cassette_r )
{
	return cassette_input(pegasus_cass);
}

static WRITE_LINE_DEVICE_HANDLER( pegasus_cassette_w )
{
	cassette_output(pegasus_cass, state ? 1 : -1);
}

static ADDRESS_MAP_START(pegasus_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xbdff) AM_RAM
	AM_RANGE(0xbe00, 0xbfff) AM_RAM AM_BASE(&pegasus_video_ram)
	AM_RANGE(0xc000, 0xdfff) AM_ROM
	AM_RANGE(0xe200, 0xe3ff) AM_RAM		// PCG
	AM_RANGE(0xe400, 0xe403) AM_DEVREADWRITE("pegasus_pia_u", pia6821_r, pia6821_w)
	AM_RANGE(0xe600, 0xe603) AM_DEVREADWRITE("pegasus_pia_s", pia6821_r, pia6821_w)
	AM_RANGE(0xf000, 0xffff) AM_ROM

/*	AM_RANGE(0xe000, 0xe000) AM_READ(pegasus_protection)
	AM_RANGE(0xe800, 0xe800) Printer Port
	AM_RANGE(0xe808, 0xe808) Cassette Interface
	AM_RANGE(0xe810, 0xe810) General Purpose User Interface */
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( pegasus )
	PORT_START("X0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('+')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR(')')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8) PORT_CHAR(8)
//	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_*) PORT_CHAR(' ') PORT_CHAR(' ')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\') PORT_CHAR('|')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR(':')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("X1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR('(')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR(']')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_DEL) PORT_CHAR(127) PORT_CHAR(' ')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('\'') PORT_CHAR('"')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')

	PORT_START("X2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('*')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('^')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_TILDE) PORT_CHAR('`') PORT_CHAR('~')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')

	PORT_START("X3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('&')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
//	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_*) PORT_CHAR(' ') PORT_CHAR(' ')
//	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_*) PORT_CHAR(' ') PORT_CHAR(' ')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT( 0x18, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("X4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
//	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_*) PORT_CHAR(' ') PORT_CHAR(' ')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("X5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
//	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_*) PORT_CHAR(' ') PORT_CHAR(' ')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHAR(10)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("X6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_TAB) PORT_CHAR(9) PORT_CHAR(9)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_ESC) PORT_CHAR(27) PORT_CHAR(27)
//	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_*) PORT_CHAR(' ') PORT_CHAR(' ')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13) PORT_CHAR(13)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')
//	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_*) PORT_CHAR(' ') PORT_CHAR(' ')
	PORT_BIT( 0x90, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("X7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
//	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_*) PORT_CHAR(' ') PORT_CHAR(' ')
//	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_*) PORT_CHAR(' ') PORT_CHAR(' ')
//	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_*) PORT_CHAR(' ') PORT_CHAR(' ')
//	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_*) PORT_CHAR(' ') PORT_CHAR(' ')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('{') PORT_CHAR('}')
	PORT_BIT( 0x3c, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

/* System - for keyboard, video, general housekeeping */
static const pia6821_interface pegasus_pia_s_intf=
{
	DEVCB_NULL,						/* port A input */
	DEVCB_HANDLER(pegasus_keyboard_r),			/* port B input */
	DEVCB_LINE(pegasus_cassette_r),				/* CA1 input */
	DEVCB_NULL,						/* CB1 input */
	DEVCB_LINE(pegasus_keyboard_irq),			/* CA2 input */
	DEVCB_NULL,						/* CB2 input */
	DEVCB_HANDLER(pegasus_keyboard_w),			/* port A output */
	DEVCB_HANDLER(pegasus_controls),			/* port B output */
	DEVCB_LINE(pegasus_cassette_w),				/* CA2 output */
	DEVCB_DEVICE_LINE("maincpu", pegasus_firq_clr),		/* CB2 output */
	DEVCB_CPU_INPUT_LINE("maincpu", M6809_IRQ_LINE),	/* IRQA output */
	DEVCB_CPU_INPUT_LINE("maincpu", M6809_IRQ_LINE)		/* IRQB output */
};

/* User interface - for connection of external equipment */
static const pia6821_interface pegasus_pia_u_intf=
{
	DEVCB_NULL,		/* port A input */
	DEVCB_NULL,		/* port B input */
	DEVCB_NULL,		/* CA1 input */
	DEVCB_NULL,		/* CB1 input */
	DEVCB_NULL,		/* CA2 input */
	DEVCB_NULL,		/* CB2 input */
	DEVCB_NULL,		/* port A output */
	DEVCB_NULL,		/* port B output */
	DEVCB_NULL,		/* CA2 output */
	DEVCB_NULL,		/* CB2 output */
	DEVCB_CPU_INPUT_LINE("maincpu", M6809_IRQ_LINE),
	DEVCB_CPU_INPUT_LINE("maincpu", M6809_IRQ_LINE)
};

static const cassette_config pegasus_cassette_config =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED|CASSETTE_MOTOR_ENABLED)
};

static DEVICE_IMAGE_LOAD( pegasus_cart_1 )
{
	image_fread(image, memory_region(image->machine, "maincpu") + 0xc000, 0x1000);

	return INIT_PASS;
}

static DEVICE_IMAGE_LOAD( pegasus_cart_2 )
{
	image_fread(image, memory_region(image->machine, "maincpu") + 0xd000, 0x1000);

	return INIT_PASS;
}

static MACHINE_START( pegasus )
{
	pegasus_cass = devtag_get_device(machine, "cassette");
}

static MACHINE_DRIVER_START( pegasus )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M6809E, XTAL_4MHz)	// actually a 6809C
	MDRV_CPU_PROGRAM_MAP(pegasus_mem)

	MDRV_TIMER_ADD_PERIODIC("pegasus_firq", pegasus_firq, HZ(50))
	MDRV_MACHINE_START(pegasus)
	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*10, 16*16)
	MDRV_SCREEN_VISIBLE_AREA(0, 32*10-1, 0, 16*15-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_UPDATE(pegasus)

	/* devices */
	MDRV_PIA6821_ADD( "pegasus_pia_s", pegasus_pia_s_intf )
	MDRV_PIA6821_ADD( "pegasus_pia_u", pegasus_pia_u_intf )
	MDRV_CARTSLOT_ADD("cart1")
	MDRV_CARTSLOT_EXTENSION_LIST("bin")
	MDRV_CARTSLOT_LOAD(pegasus_cart_1)
	MDRV_CARTSLOT_ADD("cart2")
	MDRV_CARTSLOT_EXTENSION_LIST("bin")
	MDRV_CARTSLOT_LOAD(pegasus_cart_2)
	MDRV_CASSETTE_ADD( "cassette", pegasus_cassette_config )
//	MDRV_CENTRONICS_ADD("centronics", standard_centronics)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( pegasus )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "mon11_2674.bin", 0xf000, 0x1000, CRC(1640ff7e) SHA1(8199643749fb40fb8be05e9f311c75620ca939b1) )
	ROM_FILL(0xf09e, 1, 0x20)		// hack out the protection
//	ROM_LOAD( "mon23_2601.bin", 0xf000, 0x1000, CRC(7dd451bb) SHA1(7843e9166151570f9c915589dc252feb0cb1cec4) )
//	ROM_LOAD( "mon23a_2569.bin", 0xf000, 0x1000, CRC(b4c6b352) SHA1(10bb08305a574dab48d0384e35d0781b6bde880f) )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1981, pegasus,  0,     0, 	pegasus, 	pegasus, 	 0,  "Technosys",   "Aamber Pegasus", GAME_NOT_WORKING | GAME_NO_SOUND)

