/******************************************************************************

    kc.c
    system driver

    A big thankyou to Torsten Paul for his great help with this
    driver!


    Kevin Thacker [MESS driver]

 ******************************************************************************/

/* Core includes */
#include "driver.h"
#include "includes/kc.h"

/* Components */
#include "machine/z80ctc.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "machine/nec765.h"

/* Devices */
#include "devices/cassette.h"
#include "devices/basicdsk.h"


/* pio is last in chain and therefore has highest priority */

static const struct z80_irq_daisy_chain kc85_daisy_chain[] =
{
	{z80pio_reset, z80ctc_irq_state, z80ctc_irq_ack, z80ctc_irq_reti, 0},
	{z80ctc_reset, z80ctc_irq_state, z80ctc_irq_ack, z80ctc_irq_reti, 0},
	{0,0,0,0,-1}
};

static READ8_HANDLER(kc85_4_port_r)
{
	int port;

	port = offset & 0x0ff;

	switch (port)
	{
//      case 0x080:
//          return kc85_module_r(machine, offset);

		case 0x085:
		case 0x084:
			return kc85_4_84_r(machine, offset);


		case 0x086:
		case 0x087:
			return kc85_4_86_r(machine, offset);

		case 0x088:
		case 0x089:
			return kc85_pio_data_r(machine, port-0x088);
		case 0x08a:
		case 0x08b:
			return kc85_pio_control_r(machine, port-0x08a);
		case 0x08c:
		case 0x08d:
		case 0x08e:
		case 0x08f:
			return kc85_ctc_r(machine, port-0x08c);

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
//          kc85_module_w(machine, offset,data);
//          return;

		case 0x085:
		case 0x084:
			kc85_4_84_w(machine, offset,data);
			return;

		case 0x086:
		case 0x087:
			kc85_4_86_w(machine, offset,data);
			return;

		case 0x088:
		case 0x089:
			kc85_4_pio_data_w(machine, port-0x088, data);
			return;

		case 0x08a:
		case 0x08b:
			kc85_pio_control_w(machine, port-0x08a, data);
			return;

		case 0x08c:
		case 0x08d:
		case 0x08e:
		case 0x08f:
			kc85_ctc_w(machine, port-0x08c, data);
			return;
	}

	logerror("unhandled port w: %04x\n",offset);
}


static ADDRESS_MAP_START(kc85_4_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x0000, 0x0ffff) AM_READWRITE( kc85_4_port_r, kc85_4_port_w )
ADDRESS_MAP_END

static ADDRESS_MAP_START(kc85_4_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_READWRITE(SMH_BANK1, SMH_BANK7)
	AM_RANGE(0x4000, 0x7fff) AM_READWRITE(SMH_BANK2, SMH_BANK8)
	AM_RANGE(0x8000, 0xa7ff) AM_READWRITE(SMH_BANK3, SMH_BANK9)
//  AM_RANGE(0xa800, 0xbfff) AM_RAM
	AM_RANGE(0xa800, 0xbfff) AM_READWRITE(SMH_BANK4, SMH_BANK10)
	AM_RANGE(0xc000, 0xdfff) AM_READ(SMH_BANK5)
	AM_RANGE(0xe000, 0xffff) AM_READ(SMH_BANK6)
ADDRESS_MAP_END

static ADDRESS_MAP_START(kc85_3_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_READWRITE(SMH_BANK1, SMH_BANK6)
	AM_RANGE(0x4000, 0x7fff) AM_READWRITE(SMH_BANK2, SMH_BANK7)
	AM_RANGE(0x8000, 0xbfff) AM_READWRITE(SMH_BANK3, SMH_BANK8)
	AM_RANGE(0xc000, 0xdfff) AM_READ(SMH_BANK4)
	AM_RANGE(0xe000, 0xffff) AM_READ(SMH_BANK5)
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
			return kc85_pio_data_r(machine, port-0x088);
		case 0x08a:
		case 0x08b:
			return kc85_pio_control_r(machine, port-0x08a);
		case 0x08c:
		case 0x08d:
		case 0x08e:
		case 0x08f:
			return kc85_ctc_r(machine, port-0x08c);
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
//          kc85_module_w(offset,data);
//          return;

		case 0x088:
		case 0x089:
			kc85_3_pio_data_w(machine, port-0x088, data);
			return;

		case 0x08a:
		case 0x08b:
			kc85_pio_control_w(machine, port-0x08a, data);
			return;

		case 0x08c:
		case 0x08d:
		case 0x08e:
		case 0x08f:
			kc85_ctc_w(machine, port-0x08c, data);
			return;
	}

	logerror("unhandled port w: %04x\n",offset);
}


static ADDRESS_MAP_START(kc85_3_io, ADDRESS_SPACE_IO, 8)
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


/********************/
/** DISC INTERFACE **/

static ADDRESS_MAP_START(kc85_disc_hw_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x0ffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START(kc85_disc_hw_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0f0, 0x0f0) AM_READ(nec765_status_r)
	AM_RANGE(0x0f1, 0x0f1) AM_READWRITE(nec765_data_r, nec765_data_w)
	AM_RANGE(0x0f2, 0x0f3) AM_READWRITE(nec765_dack_r, nec765_dack_w)
	AM_RANGE(0x0f4, 0x0f5) AM_READ(kc85_disc_hw_input_gate_r)
	/*{0x0f6, 0x0f7, SMH_NOP},*/		/* for controller */
	AM_RANGE(0x0f8, 0x0f9) AM_WRITE( kc85_disc_hw_terminal_count_w) /* terminal count */
	AM_RANGE(0x0fc, 0x0ff) AM_READWRITE(kc85_disk_hw_ctc_r, kc85_disk_hw_ctc_w)
ADDRESS_MAP_END

static MACHINE_DRIVER_START( cpu_kc_disc )
	MDRV_CPU_ADD("disc", Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(kc85_disc_hw_mem, 0)
	MDRV_CPU_IO_MAP(kc85_disc_hw_io, 0)
MACHINE_DRIVER_END



static MACHINE_DRIVER_START( kc85_3 )
	/* basic machine hardware */
	MDRV_CPU_ADD("main", Z80, KC85_3_CLOCK)
	MDRV_CPU_PROGRAM_MAP(kc85_3_mem, 0)
	MDRV_CPU_IO_MAP(kc85_3_io, 0)
	MDRV_CPU_CONFIG(kc85_daisy_chain)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET( kc85_3 )

	/* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(KC85_SCREEN_WIDTH, KC85_SCREEN_HEIGHT)
	MDRV_SCREEN_VISIBLE_AREA(0, (KC85_SCREEN_WIDTH - 1), 0, (KC85_SCREEN_HEIGHT - 1))
	MDRV_PALETTE_LENGTH(KC85_PALETTE_SIZE)
	MDRV_PALETTE_INIT( kc85 )

	MDRV_VIDEO_START( kc85_3 )
	MDRV_VIDEO_UPDATE( kc85_3 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("wave", WAVE, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	/* devices */
	MDRV_QUICKLOAD_ADD(kc, "kcc", 0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( kc85_4 )
	MDRV_IMPORT_FROM( kc85_3 )

	MDRV_CPU_REPLACE("main", Z80, KC85_4_CLOCK)
	MDRV_CPU_PROGRAM_MAP(kc85_4_mem, 0)
	MDRV_CPU_IO_MAP(kc85_4_io, 0)

	MDRV_MACHINE_RESET( kc85_4 )
	MDRV_VIDEO_START( kc85_4 )
	MDRV_VIDEO_UPDATE( kc85_4 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( kc85_4d )
	MDRV_IMPORT_FROM( kc85_4 )
	MDRV_IMPORT_FROM( cpu_kc_disc )
	MDRV_INTERLEAVE( 2 )
	MDRV_MACHINE_RESET( kc85_4d )
MACHINE_DRIVER_END


ROM_START(kc85_4)
	ROM_REGION(0x015000, "main",0)

    ROM_LOAD("basic_c0.854", 0x10000, 0x2000, CRC(dfe34b08) SHA1(c2e3af55c79e049e811607364f88c703b0285e2e))
    ROM_LOAD("caos__c0.854", 0x12000, 0x1000, CRC(57d9ab02) SHA1(774fc2496a59b77c7c392eb5aa46420e7722797e))
    ROM_LOAD("caos__e0.854", 0x13000, 0x2000, CRC(d64cd50b) SHA1(809cde2d189c1976d838e3e614468705183de800))
ROM_END


ROM_START(kc85_4d)
	ROM_REGION(0x015000, "main",0)

    ROM_LOAD("basic_c0.854", 0x10000, 0x2000, CRC(dfe34b08) SHA1(c2e3af55c79e049e811607364f88c703b0285e2e))
    ROM_LOAD("caos__c0.854", 0x12000, 0x1000, CRC(57d9ab02) SHA1(774fc2496a59b77c7c392eb5aa46420e7722797e))
    ROM_LOAD("caos__e0.854", 0x13000, 0x2000, CRC(d64cd50b) SHA1(809cde2d189c1976d838e3e614468705183de800))

	ROM_REGION(0x010000, "disc", ROMREGION_ERASEFF)
ROM_END

ROM_START(kc85_3)
	ROM_REGION(0x014000, "main",0)

    ROM_LOAD("basic_c0.854", 0x10000, 0x2000, CRC(dfe34b08) SHA1(c2e3af55c79e049e811607364f88c703b0285e2e))
	ROM_LOAD("caos__e0.853", 0x12000, 0x2000, CRC(52bc2199) SHA1(207d3e1c4ebf82ac7553ed0a0850b627b9796d4b))
ROM_END

static void kc85_cassette_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

static SYSTEM_CONFIG_START(kc85)
	CONFIG_RAM_DEFAULT		(64 * 1024)
	CONFIG_DEVICE(kc85_cassette_getinfo)
SYSTEM_CONFIG_END

static void kc85d_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(kc85_floppy); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "dsk"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

static SYSTEM_CONFIG_START(kc85d)
	CONFIG_IMPORT_FROM(kc85)
	CONFIG_DEVICE(kc85d_floppy_getinfo)
SYSTEM_CONFIG_END

/*     YEAR  NAME      PARENT   COMPAT  MACHINE  INPUT     INIT  CONFIG  COMPANY   FULLNAME */
COMP( 1987, kc85_3,   0,		0,		kc85_3,  kc85,     0,    kc85,   "VEB Mikroelektronik", "KC 85/3", GAME_NOT_WORKING)
COMP( 1989, kc85_4,   kc85_3,  0,		kc85_4,  kc85,     0,    kc85,   "VEB Mikroelektronik", "KC 85/4", GAME_NOT_WORKING)
COMP( 1989, kc85_4d,  kc85_3,  0,		kc85_4d, kc85,     0,    kc85d,  "VEB Mikroelektronik", "KC 85/4 + Disk Interface Module (D004)", GAME_NOT_WORKING)
