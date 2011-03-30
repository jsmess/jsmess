/******************************************************************************

    kc.c
    system driver

    A big thankyou to Torsten Paul for his great help with this
    driver!


    Kevin Thacker [MESS driver]

 ******************************************************************************/

/* Core includes */
#include "emu.h"
#include "includes/kc.h"

/* Components */
#include "machine/z80ctc.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "sound/speaker.h"
#include "sound/wave.h"
#include "machine/upd765.h"

/* Devices */
#include "imagedev/cassette.h"
#include "imagedev/flopdrv.h"
#include "formats/basicdsk.h"
#include "machine/ram.h"

static READ8_HANDLER(kc85_4_port_r)
{
	int port;

	port = offset & 0x0ff;

	switch (port)
	{
//      case 0x080:
//          return kc85_module_r(space, offset);

		case 0x085:
		case 0x084:
			return kc85_4_84_r(space, offset);


		case 0x086:
		case 0x087:
			return kc85_4_86_r(space, offset);

		case 0x088:
		case 0x089:
			return kc85_pio_data_r(space, port-0x088);
		case 0x08a:
		case 0x08b:
			return kc85_pio_control_r(space, port-0x08a);
		case 0x08c:
		case 0x08d:
		case 0x08e:
		case 0x08f:
			return kc85_ctc_r(space->machine().device("z80ctc"), port&3);

	}

	logerror("unhandled port r: %04x\n",offset);
	return 0x0ff;
}

static WRITE8_HANDLER(kc85_4_port_w)
{
	int port;

	port = offset & 0x0ff;

	switch (port)
	{
//      case 0x080:
//          kc85_module_w(space, offset,data);
//          return;

		case 0x085:
		case 0x084:
			kc85_4_84_w(space, offset,data);
			return;

		case 0x086:
		case 0x087:
			kc85_4_86_w(space, offset,data);
			return;

		case 0x088:
		case 0x089:
			kc85_4_pio_data_w(space, port-0x088, data);
			return;

		case 0x08a:
		case 0x08b:
			kc85_pio_control_w(space, port-0x08a, data);
			return;

		case 0x08c:
		case 0x08d:
		case 0x08e:
		case 0x08f:
			kc85_ctc_w(space->machine().device("z80ctc"), port&3, data);
			return;
	}

	logerror("unhandled port w: %04x\n",offset);
}


static ADDRESS_MAP_START(kc85_4_io, AS_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x0000, 0x0ffff) AM_READWRITE( kc85_4_port_r, kc85_4_port_w )
ADDRESS_MAP_END

static ADDRESS_MAP_START(kc85_4_mem, AS_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_READ_BANK("bank1") AM_WRITE_BANK("bank7")
	AM_RANGE(0x4000, 0x7fff) AM_READ_BANK("bank2") AM_WRITE_BANK("bank8")
	AM_RANGE(0x8000, 0xa7ff) AM_READ_BANK("bank3") AM_WRITE_BANK("bank9")
//  AM_RANGE(0xa800, 0xbfff) AM_RAM
	AM_RANGE(0xa800, 0xbfff) AM_READ_BANK("bank4") AM_WRITE_BANK("bank10")
	AM_RANGE(0xc000, 0xdfff) AM_READ_BANK("bank5")
	AM_RANGE(0xe000, 0xffff) AM_READ_BANK("bank6")
ADDRESS_MAP_END

static ADDRESS_MAP_START(kc85_3_mem, AS_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_READ_BANK("bank1") AM_WRITE_BANK("bank6")
	AM_RANGE(0x4000, 0x7fff) AM_READ_BANK("bank2") AM_WRITE_BANK("bank7")
	AM_RANGE(0x8000, 0xbfff) AM_READ_BANK("bank3") AM_WRITE_BANK("bank8")
	AM_RANGE(0xc000, 0xdfff) AM_READ_BANK("bank4")
	AM_RANGE(0xe000, 0xffff) AM_READ_BANK("bank5")
ADDRESS_MAP_END

static READ8_HANDLER(kc85_3_port_r)
{
	int port;

	port = offset & 0x0ff;

	switch (port)
	{
//      case 0x080:
//          return kc85_module_r(offset);

		case 0x088:
		case 0x089:
			return kc85_pio_data_r(space, port-0x088);
		case 0x08a:
		case 0x08b:
			return kc85_pio_control_r(space, port-0x08a);
		case 0x08c:
		case 0x08d:
		case 0x08e:
		case 0x08f:
			return kc85_ctc_r(space->machine().device("z80ctc"), port&3);
	}

	logerror("unhandled port r: %04x\n",offset);
	return 0x0ff;
}

static WRITE8_HANDLER(kc85_3_port_w)
{
	int port;

	port = offset & 0x0ff;

	switch (port)
	{
//      case 0x080:
//          kc85_module_w(space, offset,data);
//          return;

		case 0x088:
		case 0x089:
			kc85_3_pio_data_w(space, port-0x088, data);
			return;

		case 0x08a:
		case 0x08b:
			kc85_pio_control_w(space, port-0x08a, data);
			return;

		case 0x08c:
		case 0x08d:
		case 0x08e:
		case 0x08f:
			kc85_ctc_w(space->machine().device("z80ctc"), port&3, data);
			return;
	}

	logerror("unhandled port w: %04x\n",offset);
}


static ADDRESS_MAP_START(kc85_3_io, AS_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x0000, 0x0ffff) AM_READWRITE(kc85_3_port_r, kc85_3_port_w)
ADDRESS_MAP_END



/* this is a fake keyboard layout. The keys are converted into codes
which are transmitted by the keyboard to the base-unit. key code can
be calculated as (line*8)+bit_index */

/* 2008-05 FP:
Small note about natural keyboard: currently,
- "Brk" is mapped to 'Esc'
- "Stop" is mapped to 'Pause'
- "Clr" is mapped to 'Backspace'             */

static INPUT_PORTS_START( kc85 )
	/* start of keyboard scan-codes */
	/* codes 0-7 */
	PORT_START("KEY0")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_W)			PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_A)			PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_2)			PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_LEFT)		PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_F12)			PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS)		PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_F2)			PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z)			PORT_CHAR('y') PORT_CHAR('Y')
	/* codes 8-15 */
	PORT_START("KEY1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_E)			PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_S)			PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_3)			PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE)	PORT_CHAR('^') PORT_CHAR(0x00AC)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Clr") PORT_CODE(KEYCODE_F11) PORT_CHAR(8)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS)		PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_F3)			PORT_CHAR(UCHAR_MAMEKEY(F3))
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_X)			PORT_CHAR('x') PORT_CHAR('X')
	/* codes 16-23 */
	PORT_START("KEY2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_T)			PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_F)			PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_5)			PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_P)			PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Del") PORT_CODE(KEYCODE_F10) PORT_CHAR(UCHAR_MAMEKEY(DEL))
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_0)			PORT_CHAR('0') PORT_CHAR('@')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_F5)			PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_V)			PORT_CHAR('v') PORT_CHAR('V')
	/* codes 24-31 */
	PORT_START("KEY3")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_U)			PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_H)			PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_7)			PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_O)			PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Ins") PORT_CODE(KEYCODE_F9) PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_9)			PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Brk") PORT_CODE(KEYCODE_F7) PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_N)			PORT_CHAR('n') PORT_CHAR('N')
	/* codes 32-39 */
	PORT_START("KEY4")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_I)			PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_J)			PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_8)			PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE)		PORT_CHAR(' ')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_K)			PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA)		PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Stop") PORT_CODE(KEYCODE_F8) PORT_CHAR(UCHAR_MAMEKEY(PAUSE))
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_M)			PORT_CHAR('m') PORT_CHAR('M')
	/* codes 40-47 */
	PORT_START("KEY5")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y)			PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_G)			PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_6)			PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT(0x08, 0x00, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_L)			PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP)		PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_F6)			PORT_CHAR(UCHAR_MAMEKEY(F6))
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_B)			PORT_CHAR('b') PORT_CHAR('B')
	/* codes 48-56 */
	PORT_START("KEY6")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_R)			PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_D)			PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_4)			PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE)		PORT_CHAR('-') PORT_CHAR('|')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON)		PORT_CHAR('+') PORT_CHAR(';')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH)		PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_F4)			PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_C)			PORT_CHAR('c') PORT_CHAR('C')
	/* codes 56-63 */
	PORT_START("KEY7")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q)			PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Shift Lock") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_1)			PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_DOWN)		PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_UP)			PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_RIGHT)		PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_F1)			PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER)		PORT_CHAR(13)
	/* end of keyboard scan-codes */
	PORT_START("SHIFT")
	/* has a single shift key. Mapped here to left and right shift. */
	/* shift is connected to the transmit chip inside the keyboard and affects bit 0 */
	/* of the scan-code sent directly */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
INPUT_PORTS_END


/* pio is last in chain and therefore has highest priority */

static const z80_daisy_config kc85_daisy_chain[] =
{
	{ "z80pio" },
	{ "z80ctc" },
	{ NULL }
};


/********************/
/** DISC INTERFACE **/

static Z80CTC_INTERFACE( kc85_disc_ctc_intf )
{
	0,				/* timer disablers */
	DEVCB_NULL,			/* interrupt callback */
	DEVCB_NULL,			/* ZC/TO0 callback */
	DEVCB_NULL,			/* ZC/TO1 callback */
	DEVCB_NULL			/* ZC/TO2 callback */
};


static ADDRESS_MAP_START(kc85_disc_hw_mem, AS_PROGRAM, 8)
	AM_RANGE(0x0000, 0x0ffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START(kc85_disc_hw_io, AS_IO, 8)
	AM_RANGE(0x0f0, 0x0f0) AM_DEVREAD("upd765", upd765_status_r)
	AM_RANGE(0x0f1, 0x0f1) AM_DEVREADWRITE("upd765", upd765_data_r, upd765_data_w)
	AM_RANGE(0x0f2, 0x0f3) AM_DEVREADWRITE("upd765", upd765_dack_r, upd765_dack_w)
	AM_RANGE(0x0f4, 0x0f5) AM_READ(kc85_disc_hw_input_gate_r)
	/*{0x0f6, 0x0f7, SMH_NOP},*/		/* for controller */
	AM_RANGE(0x0f8, 0x0f9) AM_WRITE( kc85_disc_hw_terminal_count_w) /* terminal count */
	AM_RANGE(0x0fc, 0x0ff) AM_DEVREADWRITE("z80ctc_1", kc85_disk_hw_ctc_r, kc85_disk_hw_ctc_w)
ADDRESS_MAP_END


static FLOPPY_OPTIONS_START(kc85)
	FLOPPY_OPTION(kc85, "img", "KC85 disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([80])
		SECTORS([9])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

static const floppy_config kc85_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	FLOPPY_OPTIONS_NAME(kc85),
	NULL
};


static MACHINE_CONFIG_FRAGMENT( cpu_kc_disc )
	MCFG_CPU_ADD("disc", Z80, 4000000)
	MCFG_CPU_PROGRAM_MAP(kc85_disc_hw_mem)
	MCFG_CPU_IO_MAP(kc85_disc_hw_io)

	//FIX: put right clock value for CTC
	MCFG_Z80CTC_ADD( "z80ctc_1", KC85_4_CLOCK, kc85_disc_ctc_intf )

	MCFG_UPD765A_ADD("upd765", kc_fdc_interface)
MACHINE_CONFIG_END



static MACHINE_CONFIG_START( kc85_3, kc_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z80, KC85_3_CLOCK)
	MCFG_CPU_PROGRAM_MAP(kc85_3_mem)
	MCFG_CPU_IO_MAP(kc85_3_io)
	MCFG_CPU_CONFIG(kc85_daisy_chain)
	MCFG_QUANTUM_TIME(attotime::from_hz(60))

	MCFG_MACHINE_START( kc85 )
	MCFG_MACHINE_RESET( kc85_3 )

	MCFG_Z80PIO_ADD( "z80pio", 1379310.344828, kc85_pio_intf )
	MCFG_Z80CTC_ADD( "z80ctc", 1379310.344828, kc85_ctc_intf )

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(KC85_SCREEN_WIDTH, KC85_SCREEN_HEIGHT)
	MCFG_SCREEN_VISIBLE_AREA(0, (KC85_SCREEN_WIDTH - 1), 0, (KC85_SCREEN_HEIGHT - 1))
	MCFG_SCREEN_UPDATE( kc85_3 )

	MCFG_PALETTE_LENGTH(KC85_PALETTE_SIZE)
	MCFG_PALETTE_INIT( kc85 )

	MCFG_VIDEO_START( kc85_3 )

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_WAVE_ADD("wave", "cassette")
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	/* devices */
	MCFG_QUICKLOAD_ADD("quickload", kc, "kcc", 0)

	MCFG_CASSETTE_ADD( "cassette", default_cassette_config )

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("64K")
MACHINE_CONFIG_END


static MACHINE_CONFIG_DERIVED( kc85_4, kc85_3 )

	MCFG_CPU_REPLACE("maincpu", Z80, KC85_4_CLOCK)
	MCFG_CPU_PROGRAM_MAP(kc85_4_mem)
	MCFG_CPU_IO_MAP(kc85_4_io)

	MCFG_MACHINE_RESET( kc85_4 )

	MCFG_VIDEO_START( kc85_4 )
	
	MCFG_SCREEN_MODIFY("screen")
	MCFG_SCREEN_UPDATE( kc85_4 )
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( kc85_4d, kc85_4 )
	MCFG_FRAGMENT_ADD( cpu_kc_disc )
	MCFG_QUANTUM_TIME(attotime::from_hz(120))

	MCFG_MACHINE_RESET( kc85_4d )

	MCFG_FLOPPY_4_DRIVES_ADD(kc85_floppy_config)
MACHINE_CONFIG_END


ROM_START(kc85_4)
	ROM_REGION(0x015000, "maincpu",0)

    ROM_LOAD("basic_c0.854", 0x10000, 0x2000, CRC(dfe34b08) SHA1(c2e3af55c79e049e811607364f88c703b0285e2e))

    ROM_SYSTEM_BIOS(0, "caos42", "CAOS 4.2" )
    ROMX_LOAD("caos__c0.854", 0x12000, 0x1000, CRC(57d9ab02) SHA1(774fc2496a59b77c7c392eb5aa46420e7722797e), ROM_BIOS(1))
    ROMX_LOAD("caos__e0.854", 0x13000, 0x2000, CRC(ee273933) SHA1(4300f7ff813c1fb2d5c928dbbf1c9e1fe52a9577), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "caos41", "CAOS 4.1" )
	ROMX_LOAD( "caos41c.854", 0x12000, 0x1000, CRC(c7e1c011) SHA1(acd998e3d9e8f592cd884aafc8ac4d291e40e097), ROM_BIOS(2))
	ROMX_LOAD( "caos41e.854", 0x13000, 0x2000, CRC(60e045e5) SHA1(e19819fb477dcb742a13729a9bf5943d63abe863), ROM_BIOS(2))
ROM_END

ROM_START(kc85_4d)
	ROM_REGION(0x015000, "maincpu",0)

    ROM_SYSTEM_BIOS(0, "caos42", "CAOS 4.2" )
    ROMX_LOAD("caos__c0.854", 0x12000, 0x1000, CRC(57d9ab02) SHA1(774fc2496a59b77c7c392eb5aa46420e7722797e), ROM_BIOS(1))
    ROMX_LOAD("caos__e0.854", 0x13000, 0x2000, CRC(ee273933) SHA1(4300f7ff813c1fb2d5c928dbbf1c9e1fe52a9577), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "caos41", "CAOS 4.1" )
	ROMX_LOAD( "caos41c.854", 0x12000, 0x1000, CRC(c7e1c011) SHA1(acd998e3d9e8f592cd884aafc8ac4d291e40e097), ROM_BIOS(2))
	ROMX_LOAD( "caos41e.854", 0x13000, 0x2000, CRC(60e045e5) SHA1(e19819fb477dcb742a13729a9bf5943d63abe863), ROM_BIOS(2))

	ROM_REGION(0x010000, "disc", ROMREGION_ERASEFF)
ROM_END

ROM_START(kc85_3)
	ROM_REGION(0x014000, "maincpu",0)
    ROM_LOAD( "basic_c0.853", 0x10000, 0x2000, CRC(dfe34b08) SHA1(c2e3af55c79e049e811607364f88c703b0285e2e))
    ROM_SYSTEM_BIOS(0, "caos31", "CAOS 3.1" )
	ROMX_LOAD( "caos__e0.853", 0x12000, 0x2000, CRC(639e4864) SHA1(efd002fc9146116936e6e6be0366d2afca33c1ab), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "caos33", "CAOS 3.3" )
	ROMX_LOAD( "caos33.853",   0x12000, 0x2000, CRC(ca0fecad) SHA1(20447d27c9aa41a1c7a3d6ad0699edb06a207aa6), ROM_BIOS(2))
	ROM_SYSTEM_BIOS(2, "caos34", "CAOS 3.4" )
	ROMX_LOAD( "caos34.853",   0x12000, 0x2000, CRC(d0245a3e) SHA1(ee9f8e7427b9225ae2cecbcfb625d629ab6a601d), ROM_BIOS(3))
	ROM_SYSTEM_BIOS(3, "pi88ge", "OS PI/88 (yellow/blue)" )
	ROMX_LOAD( "pi88_ge.853",  0x12000, 0x2000, CRC(4bf0cfde) SHA1(b8373a44e4553197e3dd23008168d5214b878837), ROM_BIOS(4))
	ROM_SYSTEM_BIOS(4, "pi88sw", "OS PI/88 (black/white)" )
	ROMX_LOAD( "pi88_sw.853",  0x12000, 0x2000, CRC(f7d2e8fc) SHA1(9b5c068f10ff34bc3253f5b51abad51c8da9dd5d), ROM_BIOS(5))
	ROM_SYSTEM_BIOS(5, "pi88ws", "OS PI/88 (white/blue)" )
	ROMX_LOAD( "pi88_ws.853",  0x12000, 0x2000, CRC(9ef4efbf) SHA1(b8b6f606b76bce9fb7fcd61a14120e5e026b6b6e), ROM_BIOS(6))
ROM_END

ROM_START(kc85_2)
	ROM_REGION(0x014000, "maincpu",0)
	ROM_SYSTEM_BIOS(0, "hc900", "HC900 CAOS" )
	ROMX_LOAD( "hc900.852",    0x12000, 0x2000, CRC(e6f4c0ab) SHA1(242a777788c774c5f764313361b1e0a65139ab32), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "caos22", "CAOS 2.2" )
	ROMX_LOAD( "caos__e0.852", 0x12000, 0x2000, CRC(48d5624c) SHA1(568dd59bfad4c604ba36bc05b094fc598a642f85), ROM_BIOS(2))
ROM_END

ROM_START(kc85_5)
	ROM_REGION(0x01C000, "maincpu",0)

	ROM_LOAD("basic_c0.855", 0x10000, 0x8000, CRC(0ed9f8b0) SHA1(be2c68a5b461014c57e33a127c3ffb32b0ff2346))
    ROM_SYSTEM_BIOS(0, "caos44", "CAOS 4.4" )
    ROMX_LOAD( "caos__c0.855",0x18000, 0x2000, CRC(f56d5c18) SHA1(2cf8023ee71ca50b92f9f151b7519f59727d1c79), ROM_BIOS(1))
	ROMX_LOAD( "caos__e0.855",0x1A000, 0x2000, CRC(1dbc2e6d) SHA1(53ba4394d96e287ff8af01322af1e9879d4e77c4), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "caos43", "CAOS 4.3" )
	ROMX_LOAD( "caos43c.855", 0x18000, 0x2000, CRC(2f0f9eaa) SHA1(5342be5104206d15e7471b094c7749a8a3d708ad), ROM_BIOS(2))
	ROMX_LOAD( "caos43e.855", 0x1A000, 0x2000, CRC(b66fc6c3) SHA1(521ac2fbded4148220f8af2d5a5ab99634364079), ROM_BIOS(2))
ROM_END

/*     YEAR  NAME      PARENT   COMPAT  MACHINE  INPUT     INIT  COMPANY   FULLNAME */
COMP( 1987, kc85_2,   0,	   0,		kc85_3,  kc85,     0,    "VEB Mikroelektronik", "HC900 / KC 85/2", GAME_NOT_WORKING)
COMP( 1987, kc85_3,   kc85_2,  0,		kc85_3,  kc85,     0,    "VEB Mikroelektronik", "KC 85/3", GAME_NOT_WORKING)
COMP( 1989, kc85_4,   kc85_2,  0,		kc85_4,  kc85,     0,    "VEB Mikroelektronik", "KC 85/4", GAME_NOT_WORKING)
COMP( 1989, kc85_4d,  kc85_2,  0,		kc85_4d, kc85,     0,    "VEB Mikroelektronik", "KC 85/4 + Disk Interface Module (D004)", GAME_NOT_WORKING)
COMP( 1989, kc85_5,   kc85_2,  0,		kc85_4,  kc85,     0,    "VEB Mikroelektronik", "KC 85/5", GAME_NOT_WORKING)
