#include "driver.h"
#include "cpu/i8039/i8039.h"
#include "cpu/m68000/m68000.h"
#include "video/generic.h"
#include "video/ql.h"
#include "inputx.h"

/*

	Sinclair QL

	MESS Driver by Curt Coder


	TODO:

	- everything

*/

/* Peripheral Chip (ZX8302) */

static int ZXmode, ZXbps;
static int zx8302_comdata, zx8302_baudx4;

static mame_timer *zx8302_baud_timer;

static TIMER_CALLBACK( zx8302_baud_tick )
{
	zx8302_baudx4 = zx8302_baudx4 ? 0 : 1;
}

static WRITE8_HANDLER( zx8302_w )
{
	ZXmode = data & 0x18;

	switch(ZXmode)
	{
	case 0x00: logerror("ZXSER1\n"); break;
	case 0x08: logerror("ZXSER2\n"); break;
	case 0x10: logerror("ZXMDV\n"); break;
	case 0x18: logerror("ZXNET\n"); break;
	}

	switch(data & 0x07)
	{
	case 0: ZXbps = 19200; break;
	case 1: ZXbps = 9600; break;
	case 2: ZXbps = 4800; break;
	case 3: ZXbps = 2400; break;
	case 4: ZXbps = 1200; break;
	case 5: ZXbps = 600; break;
	case 6: ZXbps = 300; break;
	case 7: ZXbps = 75; break;
	}
}

static WRITE8_HANDLER( i8049_w )
{
	/*
	process 8049 commands (p92)
	WR 18003: 8049  commands
	0:
	1: get interrupt status
	2: open ser1
	3: open ser2
	4: close ser1/ser2
	5:
	6: serial1 receive
	7: serial2 receive
	8: read keyboard
	9: keyrow
	a: set sound
	b: kill sound
	c:
	d: set serial baudrate
	e: get response
	f: test

	write nibble to 8049:
	'dbca' in 4 writes to 18003: 000011d0, 000011c0, 000011b0, 000011a0.
	each write is acknowledged by 8049 with a '0' in bit 6 of 18020.

	response from 8049:
	repeat for any number of bits to be received (MSB first)
	970718
	{ write 00001101 to 18003; wait for '0' in bit6 18020; read bit7 of 18020 }

	*/
}

static WRITE8_HANDLER( mdv_ctrl_w )
{
	/*
	MDV control
	at reset; and after data transfer of any mdv
		(02 00) 8 times			stop all motors
	start an mdvN:
		(03 01) (02 00) N-1 times	start motorN
	format (erase):
		0a
		(0e 0e) 0a per header and per sector
	format (verify):
		02
		(02 02) after finding pulses per header/sector
	format (write directory):
		0a (0e 0e) 02 write sector
	format (find sector 0):
		(02 02) after pulses
	format (write directory):
		0a (0e 0e) 02 write sector
		(02 00) 8 times 		stop motor

	read sector:
		(02 02) after pulses
		get 4 bytes
		(02 02) indication to skip PLL sequence: 6*00,2*ff
	*/
}

static WRITE8_HANDLER( mdv_data_w )
{
}

static READ8_HANDLER( mdv_status_r )
{
	return 0x0;
}

static WRITE8_HANDLER( clock_w )
{
}

/* Intelligent Peripheral Controller (IPC) */

static UINT8 ipc_keylatch;

static WRITE8_HANDLER( ipc_port1_w )
{
	/*
		
		bit		description
		
		0		Keyboard column output (KBO0)
		1		Keyboard column output (KBO1)
		2		Keyboard column output (KBO2)
		3		Keyboard column output (KBO3)
		4		Keyboard column output (KBO4)
		5		Keyboard column output (KBO5)
		6		Keyboard column output (KBO6)
		7		Keyboard column output (KBO7)

	*/

	ipc_keylatch = data;
}

static WRITE8_HANDLER( ipc_port2_w )
{
	/*
		
		bit		description
		
		0		Serial data input
		1		Speaker output
		2		Interrupt IPL2 output
		3		Interrupt IPL1 output
		4		SER2 CTS
		5		SER1 DTR
		6		N/C
		7		ZX8302 link output

	*/

	int ipl = (BIT(data, 2) << 1) | BIT(data, 3);

	switch (ipl)
	{
	case 0:
		cpunum_set_input_line(0, MC68000_IRQ_6, HOLD_LINE);
		break;
	case 1:
		cpunum_set_input_line(0, MC68000_IRQ_4, HOLD_LINE);
		break;
	case 2:
		cpunum_set_input_line(0, MC68000_IRQ_2, HOLD_LINE);
		break;
	case 3:
		// do nothing
		break;
	}

	zx8302_comdata = BIT(data, 7);
}

static READ8_HANDLER( ipc_port2_r )
{
	/*
		
		bit		description
		
		0		Serial data input
		1		Speaker output
		2		Interrupt IPL2 output
		3		Interrupt IPL1 output
		4		SER2 CTS
		5		SER1 DTR
		6		N/C
		7		ZX8302 link output

	*/

	return zx8302_comdata << 7;
}

static READ8_HANDLER( ipc_t1_r )
{
	return zx8302_baudx4;
}

static READ8_HANDLER( ipc_bus_r )
{
	/*
		
		bit		description
		
		0		Keyboard row input (KBI0)
		1		Keyboard row input (KBI1)
		2		Keyboard row input (KBI2)
		3		Keyboard row input (KBI3)
		4		Keyboard row input (KBI4)
		5		Keyboard row input (KBI5)
		6		Keyboard row input (KBI6)
		7		Keyboard row input (KBI7)

	*/

	if (ipc_keylatch & 0x01) return readinputportbytag("ROW0");
	if (ipc_keylatch & 0x02) return readinputportbytag("ROW1");
	if (ipc_keylatch & 0x04) return readinputportbytag("ROW2");
	if (ipc_keylatch & 0x08) return readinputportbytag("ROW3");
	if (ipc_keylatch & 0x10) return readinputportbytag("ROW4");
	if (ipc_keylatch & 0x20) return readinputportbytag("ROW5");
	if (ipc_keylatch & 0x40) return readinputportbytag("ROW6");
	if (ipc_keylatch & 0x80) return readinputportbytag("ROW7");

	return 0;
}

/* Memory Maps */

static ADDRESS_MAP_START( ql_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x000000, 0x00bfff) AM_ROM	// System ROM
	AM_RANGE(0x00c000, 0x00ffff) AM_ROM	// Cartridge ROM
	AM_RANGE(0x018000, 0x018001) AM_WRITE(clock_w)
	AM_RANGE(0x018002, 0x018002) AM_WRITE(zx8302_w)
	AM_RANGE(0x018003, 0x018003) AM_WRITE(i8049_w)
	AM_RANGE(0x018020, 0x018020) AM_READWRITE(mdv_status_r, mdv_ctrl_w)
	AM_RANGE(0x018021, 0x018021) AM_WRITENOP // ???
	AM_RANGE(0x018022, 0x018022) AM_WRITE(mdv_data_w)
	AM_RANGE(0x018023, 0x018023) AM_WRITENOP // ???
	AM_RANGE(0x018063, 0x018063) AM_WRITE(ql_video_ctrl_w)
	AM_RANGE(0x020000, 0x027fff) AM_RAM AM_WRITE(ql_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x028000, 0x02ffff) AM_RAM // videoram 2
	AM_RANGE(0x030000, 0x03ffff) AM_RAM // onboard RAM
	AM_RANGE(0x040000, 0x0bffff) AM_RAM // 512KB add-on RAM
	AM_RANGE(0x0c0000, 0x0dffff) AM_NOP // 8x16KB device slots
	AM_RANGE(0x0e0000, 0x0fffff) AM_ROM	// add-on ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( ipc_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x000, 0x7ff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( ipc_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(I8039_p1, I8039_p1) AM_WRITE(ipc_port1_w)
	AM_RANGE(I8039_p2, I8039_p2) AM_READWRITE(ipc_port2_r, ipc_port2_w)
	AM_RANGE(I8039_t1, I8039_t1) AM_READ(ipc_t1_r)
	AM_RANGE(I8039_bus, I8039_bus) AM_READ(ipc_bus_r)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( ql )
	PORT_START_TAG("ROW0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F1) PORT_NAME("F1")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F2) PORT_NAME("F2")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F3) PORT_NAME("F3")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('&')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F4) PORT_NAME("F4")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F5) PORT_NAME("F5")

	PORT_START_TAG("ROW1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('^')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('*')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR(')')

	PORT_START_TAG("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_TAB) PORT_CHAR('\t') PORT_NAME("TABULATE")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('_')

	PORT_START_TAG("ROW3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')

	PORT_START_TAG("ROW4")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CAPSLOCK) PORT_NAME("CAPS LOCK")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR(':')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('+')

	PORT_START_TAG("ROW5")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('\'') PORT_CHAR('\"')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_TILDE) PORT_CHAR(0x00A3) PORT_CHAR('~')

	PORT_START_TAG("ROW6")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')

	PORT_START_TAG("ROW7") // where is UP?
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT)) PORT_NAME("\xE2\x86\x90") 
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC)) PORT_NAME("ESC \xC2\xA9") 
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT)) PORT_NAME("\xE2\x86\x92") 
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ') PORT_NAME("SPACE") 
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN)) PORT_NAME("\xE2\x86\x93") 
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13) PORT_NAME("ENTER")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\') PORT_CHAR('|')

	PORT_START_TAG("SPECIAL")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1) PORT_NAME("SHIFT")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(ESC)) PORT_NAME("CTRL")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_LALT) PORT_CODE(KEYCODE_RALT) PORT_CHAR(UCHAR_MAMEKEY(LALT)) PORT_CHAR(UCHAR_MAMEKEY(RALT)) PORT_NAME("ALT")

	PORT_START_TAG("JOY0")

	PORT_START_TAG("JOY1")
INPUT_PORTS_END

/* Machine Drivers */

static MACHINE_START( ql )
{
	zx8302_baud_timer = mame_timer_alloc(zx8302_baud_tick);
}

static MACHINE_DRIVER_START( ql )
	// basic machine hardware
	MDRV_CPU_ADD(M68008, 15000000/2)	// 7.5 MHz
	MDRV_CPU_PROGRAM_MAP(ql_map, 0)

	MDRV_CPU_ADD(I8048, 11000000)		// 11 MHz (i8049 IPC)
	MDRV_CPU_PROGRAM_MAP(ipc_map, 0)
	MDRV_CPU_IO_MAP(ipc_io_map, 0)

	MDRV_MACHINE_START(ql)

	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)

    // video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_SCREEN_VISIBLE_AREA(0, 512-1, 0, 256-1)

	MDRV_PALETTE_LENGTH(8)

	MDRV_PALETTE_INIT(ql)
	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(generic_bitmapped)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ql_ntsc )
	MDRV_IMPORT_FROM(ql)
	MDRV_SCREEN_REFRESH_RATE(60)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( ql )
    ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_SYSTEM_BIOS( 0, "js", "v1.10 (JS)" ) // confirmed dump
	ROMX_LOAD( "ql.js 0000.ic33", 0x000000, 0x008000, CRC(1bbad3b8) SHA1(59fd4372771a630967ee102760f4652904d7d5fa), ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(1) )
    ROMX_LOAD( "ql.js 8000.ic34", 0x008000, 0x004000, CRC(c970800e) SHA1(b8c9203026a7de6a44bd0942ec9343e8b222cb41), ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(1) )

	ROM_SYSTEM_BIOS( 1, "tb", "v1.0? (TB)" )
	ROMX_LOAD( "tb.ic33", 0x000000, 0x008000, CRC(1c86d688) SHA1(7df8028e6671afc4ebd5f65bf6c2d6019181f239), ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(2) )
    ROMX_LOAD( "tb.ic34", 0x008000, 0x004000, CRC(de7f9669) SHA1(9d6bc0b794541a4cec2203256ae92c7e68d1011d), ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(2) )

	ROM_SYSTEM_BIOS( 2, "jm", "v1.03 (JM)" ) // confirmed dump
	ROMX_LOAD( "ql.jm 0000.ic33", 0x000000, 0x008000, CRC(1f8e840a) SHA1(7929e716dfe88318bbe99e34f47d039957fe3cc0), ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(3) )
    ROMX_LOAD( "ql.jm 8000.ic34", 0x008000, 0x004000, CRC(9168a2e9) SHA1(1e7c47a59fc40bd96dfefc2f4d86827c15f0199e), ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(3) )

	ROM_SYSTEM_BIOS( 3, "ah", "v1.02 (AH)" )
	ROMX_LOAD( "ah.ic33.1", 0x000000, 0x004000, CRC(a9b4d2df) SHA1(142d6f01a9621aff5e0ad678bd3cbf5cde0db801), ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(4) )
    ROMX_LOAD( "ah.ic33.2", 0x004000, 0x004000, CRC(36488e4e) SHA1(ff6f597b30ea03ce480a3d6728fd1d858da34d6a), ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(4) )
	ROMX_LOAD( "ah.ic34",   0x008000, 0x004000, CRC(61259d4c) SHA1(bdd10d111e7ba488551a27c8d3b2743917ff1307), ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(4) )

	ROM_SYSTEM_BIOS( 4, "pm", "v1.01 (PM)" )
	ROMX_LOAD( "pm.ic33", 0x000000, 0x008000, NO_DUMP, ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(5) )
    ROMX_LOAD( "pm.ic34", 0x008000, 0x004000, NO_DUMP, ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(5) )

	ROM_SYSTEM_BIOS( 5, "fb", "v1.00 (FB)" )
    ROMX_LOAD( "fb.ic33", 0x000000, 0x008000, NO_DUMP, ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(6) )
    ROMX_LOAD( "fb.ic34", 0x008000, 0x004000, NO_DUMP, ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(6) )

	ROM_SYSTEM_BIOS( 6, "tyche", "v2.05 (Tyche)" )
    ROMX_LOAD( "tyche.rom", 0x000000, 0x010000, CRC(8724b495) SHA1(5f33a1bc3f23fd09c31844b65bc3aca7616f180a), ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(7) )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "v07.ic24",	  0x0000, 0x0800, CRC(051111f9) SHA1(83ed562464df89b9fdd9740db51d45884a512696) ) // V0.7
	ROM_LOAD( "ipc8049.ic24", 0x0000, 0x0800, CRC(6a0d1f20) SHA1(fcb1c97ee7c66e5b6d8fbb57c06fd2f6509f2e1b) )
ROM_END

ROM_START( ql_jsu )
    ROM_REGION( 0x400000, REGION_CPU1, 0 )
    ROM_LOAD16_WORD_SWAP( "jsu.ic33", 0x000000, 0x008000, CRC(e397f49f) SHA1(c06f92eabaf3e6dd298c51cb7f7535d8ef0ef9c5) )
    ROM_LOAD16_WORD_SWAP( "jsu.ic34", 0x008000, 0x004000, CRC(3debbacc) SHA1(9fbc3e42ec463fa42f9c535d63780ff53a9313ec) )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "ipc8049.ic24", 0x0000, 0x0800, CRC(6a0d1f20) SHA1(fcb1c97ee7c66e5b6d8fbb57c06fd2f6509f2e1b) )
ROM_END

ROM_START( ql_mge )
    ROM_REGION( 0x400000, REGION_CPU1, 0 )
    ROM_LOAD16_WORD_SWAP( "mge.ic33", 0x000000, 0x008000, CRC(d5293bde) SHA1(bf5af7e53a472d4e9871f182210787d601db0634) )
    ROM_LOAD16_WORD_SWAP( "mge.ic34", 0x008000, 0x004000, CRC(a694f8d7) SHA1(bd2868656008de85d7c191598588017ae8aa3339) )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "ipc8049.ic24", 0x0000, 0x0800, CRC(6a0d1f20) SHA1(fcb1c97ee7c66e5b6d8fbb57c06fd2f6509f2e1b) )
ROM_END

ROM_START( ql_mgf )
    ROM_REGION( 0x400000, REGION_CPU1, 0 )
    ROM_LOAD16_WORD_SWAP( "mgf.ic33", 0x000000, 0x008000, NO_DUMP )
    ROM_LOAD16_WORD_SWAP( "mgf.ic34", 0x008000, 0x004000, NO_DUMP )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "ipc8049.ic24", 0x0000, 0x0800, CRC(6a0d1f20) SHA1(fcb1c97ee7c66e5b6d8fbb57c06fd2f6509f2e1b) )
ROM_END

ROM_START( ql_mgg )
    ROM_REGION( 0x400000, REGION_CPU1, 0 )
    ROM_LOAD16_WORD_SWAP( "mgg.ic33", 0x000000, 0x008000, CRC(b4e468fd) SHA1(cd02a3cd79af90d48b65077d0571efc2f12f146e) )
    ROM_LOAD16_WORD_SWAP( "mgg.ic34", 0x008000, 0x004000, CRC(54959d40) SHA1(ffc0be9649f26019d7be82925c18dc699259877f) )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "ipc8049.ic24", 0x0000, 0x0800, CRC(6a0d1f20) SHA1(fcb1c97ee7c66e5b6d8fbb57c06fd2f6509f2e1b) )
ROM_END

ROM_START( ql_mgi )
    ROM_REGION( 0x400000, REGION_CPU1, 0 )
    ROM_LOAD16_WORD_SWAP( "mgi.ic33", 0x000000, 0x008000, CRC(d5293bde) SHA1(bf5af7e53a472d4e9871f182210787d601db0634) )
    ROM_LOAD16_WORD_SWAP( "mgi.ic34", 0x008000, 0x004000, CRC(a2fdfb83) SHA1(162b1052737500f3c13497cdf0f813ba006bdae9) )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "ipc8049.ic24", 0x0000, 0x0800, CRC(6a0d1f20) SHA1(fcb1c97ee7c66e5b6d8fbb57c06fd2f6509f2e1b) )
ROM_END

ROM_START( ql_mgs )
    ROM_REGION( 0x400000, REGION_CPU1, 0 )
    ROM_LOAD16_WORD_SWAP( "mgs.ic33", 0x000000, 0x008000, NO_DUMP )
    ROM_LOAD16_WORD_SWAP( "mgs.ic34", 0x008000, 0x004000, NO_DUMP )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "ipc8049.ic24", 0x0000, 0x0800, CRC(6a0d1f20) SHA1(fcb1c97ee7c66e5b6d8fbb57c06fd2f6509f2e1b) )
ROM_END

ROM_START( ql_efp )
    ROM_REGION( 0x400000, REGION_CPU1, 0 )
    ROM_LOAD16_WORD_SWAP( "efp.ic33", 0x000000, 0x008000, CRC(eb181641) SHA1(43c1e0215cf540cbbda240b1048910ff55681059) )
    ROM_LOAD16_WORD_SWAP( "efp.ic34", 0x008000, 0x004000, CRC(4c3b34b7) SHA1(f9dc571d2d4f68520b306ecc7516acaeea69ec0d) )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "ipc8049.ic24", 0x0000, 0x0800, CRC(6a0d1f20) SHA1(fcb1c97ee7c66e5b6d8fbb57c06fd2f6509f2e1b) )
ROM_END

/* System Configuration */

SYSTEM_CONFIG_START( ql )
	CONFIG_RAM_DEFAULT	(128 * 1024)
	CONFIG_RAM			(640 * 1024)
SYSTEM_CONFIG_END

/* Computer Drivers */

/*    YEAR  NAME    PARENT  COMPAT  MACHINE     INPUT   INIT    CONFIG  COMPANY                     FULLNAME        FLAGS */
COMP( 1984, ql,     0,      0,      ql,         ql,     0,      ql,     "Sinclair Research Ltd",    "QL (UK)",      GAME_NOT_WORKING )
COMP( 1985, ql_jsu, ql,     0,      ql_ntsc,    ql,     0,      ql,     "Sinclair Research Ltd",    "QL (USA)",     GAME_NOT_WORKING )
COMP( 1985, ql_mge, ql,     0,      ql,         ql,     0,      ql,     "Sinclair Research Ltd",    "QL (Spain)",   GAME_NOT_WORKING )
COMP( 1985, ql_mgf, ql,     0,      ql,         ql,     0,      ql,     "Sinclair Research Ltd",    "QL (France)",  GAME_NOT_WORKING )
COMP( 1985, ql_mgg, ql,     0,      ql,         ql,     0,      ql,     "Sinclair Research Ltd",    "QL (Germany)", GAME_NOT_WORKING )
COMP( 1985, ql_mgi, ql,     0,      ql,         ql,     0,      ql,     "Sinclair Research Ltd",    "QL (Italy)",   GAME_NOT_WORKING )
COMP( 1985, ql_mgs, ql,     0,      ql,         ql,     0,      ql,     "Sinclair Research Ltd",    "QL (Sweden)",  GAME_NOT_WORKING )
COMP( 1985, ql_efp, ql,     0,      ql,         ql,     0,      ql,     "Sinclair Research Ltd",    "QL (Mexico)",  GAME_NOT_WORKING )
