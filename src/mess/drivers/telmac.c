/*

	telmac.c


	MESS Driver by Curt Coder


	Telmac 1800
	-----------
	(c) 10/1977 Telercas Oy, Finland

	CPU:		CDP1802		1.76 MHz
	RAM:		2 KB (4 KB)
	ROM:		512 B

	Video:		CDP1861		1.76 MHz			(OSM-200)
	Resolution:	32x64

	Designer:	Osmo Kainulainen
	Keyboard:	OS-70 A (QWERTY, hexadecimal keypad)

	
	Oscom 1000B
	-----------
	(c) 197? Oscom Oy, Finland

	CPU:		CDP1802A	? MHz

	Enhanced Telmac 1800 with built-in CRT board.

	
	Telmac 2000
	-----------
	(c) 1980 Telercas Oy, Finland

	CPU:		CDP1802A	1.75 MHz
	RAM:		16 KB
	ROM:		512 B / 2 KB (TOOL-2000)

	Video:		CDP1864		1.75 MHz
	Color RAM:	512 B

	Colors:		8 fg, 4 bg
	Resolution:	64x192
	Sound:		frequency control, volume on/off
	Keyboard:	ASCII, KB-16, KB-64


	Telmac 2000E
	------------
	(c) 1980 Telercas Oy, Finland

	CPU:		CDP1802A	1.75 MHz
	RAM:		8 KB
	ROM:		8 KB

	Video:		CDP1864		1.75 MHz
	Color RAM:	1 KB

	Colors:		8 fg, 4 bg
	Resolution:	64x192
	Sound:		frequency control, volume on/off
	Keyboard:	ASCII (RCA VP-601/VP-611), KB-16/KB-64

	SBASIC:		24.0

	
	Oscom Nano
	----------
	(c) 1981 Oscom Oy, Finland

	CPU:		CDP1802A	? MHz
	ROM:		512 B / 1 KB

	Small form factor version of Telmac 1800. Combined text and graphics output.

	
	Telmac TMC-121/111/112
	----------------------
	(c) 198? Telercas Oy, Finland

	CPU:		CDP1802A	? MHz

	Built from Telmac 2000 series cards. Huge metal box.

	
	Telmac TMC-600 (Series I/Series II)
	-----------------------------------
	(c) 1982 Telercas Oy, Finland

	CPU:		CDP1802A	3.58 MHz

	RAM:		8 KB, expansion to 28 KB
	ROM:		20 KB, on-board expansion 8 KB

	Video RAM:	1 KB
	Color RAM:	512 B 

	SBASIC:		24.2/24.3

	Video:		CDP1870		5.626 MHz		supports only tilemap gfx
				CDP1869		5.626 MHz		sound generator, address decoder
	Resolution:	240 x 216
	Colors:		8 fg, 8 bg, blink attribute
	Sound:		8 octaves, white noise generator, volume control
	Charset:	96 ASCII chars, 128 graphic symbols, character size 6x9 / 12x18 (double size mode)

	Keyboard:
	DEL, 1 !, 2 ", 3 #, 4 $, 5 %, 6 &, 7 /, 8 (, 9 ), 0, = -, ~ ^, @, BREAK
	CTRL, Q, W, E, R, T, Y, U, I, O, P, Å, + ;, RETURN
	SHIFT LOCK, A, S, D, F, G, H, J, K, L, Ö, Ä, * :, LINE FEED
	SHIFT, Z, X, C, V, B, N, M, < , > ., ? /, SHIFT
	SPACE

	Numeric keypad:
	7, 8, 9, UP, ESC
	4, 5, 6, RIGHT, []
	1, 2, 3, DOWN, CTRL
	,, 0, ., LEFT, ALT MODE

	Devices:	Cassette using a standard 5-pin DIN connector. (Any tape recorder will do)
				Centronics Printer/RF Modulator
				RTC using Telmac TMC-700 expansion (also includes new SBASIC commands)

*/

/*

	2003-05-20	Project started
	2003-05-21  Telmac TMC-600 (Sarja II) acquired, unit broken
	2003-06-22	Preliminary driver, missing ROMs among other things
	2003-07-28	Added some TMC-1800 info
	2003-07-31	Added some computer info
	2004-01-04	Reformatted inputs and added memory map for tmc600
	2004-01-06	Fixed some code
	2004-01-19	Dumped ROMs, hooked up display and keyboard
	2004-01-27	Scanned in Telmac 2000 manual
	2004-01-28	Scanned in Telmac 2000E manual
	2004-01-29	Added Telmac 2000 code
	2004-02-03	Modified cdp1802.c to use regular ports instead of out_n/in_n callback functions
	2004-02-04	Figured out the keyboard except for one key, the port update fixed reading, still a bit too quick to repeat
	2004-02-19	Examined the connection between 1802 and 1869 on the pcb with a multimeter, added vismac_w
	2004-02-22	Separated video code to vidhrdw/cdp186x.c
	2004-03-21	Cleaned up
	2004-04-11	More cleanup
	2004-04-29	Fixed CDP1802 core -> gfx anomalies now gone, fixed scrolling, bkg color & dblsize
	2004-08-12	Added colorram and cursor blink approximation, cdp1869 tone output
	2004-08-13	Added printer output, quickload, more devices

*/

/*

	TODO:

	- tmc1800: monitor rom
	- tmc2000t: tool2000 rom
	- tmc2000e: sbasic roms
	- tmc600: all roms
	- tmc600a: 151182+sbdos, sb33.4 fixrom
	- tmc600as: all roms, what is this white-case machine anyway?
	- tmc600: keyboard repeat delay is WAY too short
	- tmc600: natural keyboard needs fixing
	- tmc600: does SCREEN @4700 show only a partial picture? verify from real tmc600
	- tmc600: real cdp1869 video emulation instead of a simple tilemap hack
	- tmc600: cdp1869 white noise emulation
	- tmc600: real-time clock
	- schematics for all machines

*/

#include "driver.h"
#include "inputx.h"
#include "cpu/cdp1802/cdp1802.h"
#include "video/generic.h"
#include "video/cdp186x.h"
#include "devices/printer.h"
#include "devices/basicdsk.h"
#include "devices/cassette.h"
#include "devices/snapquik.h"
#include "sound/beep.h"

static int keylatch, vismac_latch;

/* Read/Write Handlers */

static  READ8_HANDLER( vismac_r )
{
	return 0;
}

static WRITE8_HANDLER( vismac_w )
{
}

static WRITE8_HANDLER( vismac_register_w )
{
	vismac_latch = data;
}

static WRITE8_HANDLER( vismac_data_w )
{
	UINT16 word = activecpu_get_reg(activecpu_get_reg(CDP1802_X) + CDP1802_R0) - 1; // TODO: why -1?

	switch (vismac_latch)
	{
	case 0x20:
		// set character color
	case 0x30:
		// write cdp1869 command on the data bus
		cdp1869_instruction_w(0, vismac_latch / 0x10, data);
		break;
	default:
		// write cdp1870 command using the address bus bits as data
		cdp1869_instruction_w(0, vismac_latch / 0x10, word);
		break;
	}
}

static  READ8_HANDLER( floppy_r )
{
	return 0;
}

static WRITE8_HANDLER( floppy_w )
{
}

static WRITE8_HANDLER( printer_w )
{
	printer_output(image_from_devtype_and_index(IO_PRINTER, 0), data);
}

static  READ8_HANDLER( ascii_keyboard_r )
{
	return 0;
}

static  READ8_HANDLER( io_r )
{
	return 0;
}

static WRITE8_HANDLER( io_w )
{
}

static WRITE8_HANDLER( io_select_w )
{
}

static WRITE8_HANDLER( tmc2000_bankswitch_w )
{
	if (data & 0x01)
	{
		// enable Color RAM
		memory_install_read8_handler(  0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x81ff, 0, 0, MRA8_RAM );
		memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x81ff, 0, 0, MWA8_RAM );
		memory_set_bankptr(1, &colorram);
	}
	else
	{
		// enable ROM
		memory_install_read8_handler(  0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x81ff, 0, 0, MRA8_ROM );
		memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x81ff, 0, 0, MWA8_ROM );
		memory_set_bankptr(1, memory_region(REGION_CPU1) + 0x8000);
	}

	cdp1864_tone_divisor_latch_w(0, data);
}

static WRITE8_HANDLER( keyboard_latch_w )
{
	keylatch = data;
}

/* Memory Maps */

// Telmac 1800

static ADDRESS_MAP_START( tmc1800_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM									// Work RAM
	AM_RANGE(0x0800, 0x0fff) AM_RAM									// Expanded RAM
	AM_RANGE(0x8000, 0x81ff) AM_ROM									// Monitor ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( tmc1800_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x02, 0x02) AM_WRITE(keyboard_latch_w)
ADDRESS_MAP_END

// Telmac 2000

static ADDRESS_MAP_START( tmc2000_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_RAM									// Work RAM
	AM_RANGE(0x4000, 0x7fff) AM_RAM									// Expanded RAM
	AM_RANGE(0x8000, 0x81ff) AM_READWRITE(MRA8_BANK1, MWA8_BANK1)	// Monitor ROM / Color RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( tmc2000_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_READWRITE(cdp1864_audio_enable_r, cdp1864_step_background_color_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(keyboard_latch_w)
	AM_RANGE(0x04, 0x04) AM_READWRITE(cdp1864_audio_disable_r, tmc2000_bankswitch_w)
ADDRESS_MAP_END

// Telmac 2000 (TOOL-2000)

static ADDRESS_MAP_START( tmc2000t_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_RAM									// Work RAM
	AM_RANGE(0x4000, 0x7fff) AM_RAM									// Expanded RAM
	AM_RANGE(0x8000, 0x87ff) AM_ROM									// TOOL-2000 ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( tmc2000t_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_READWRITE(cdp1864_audio_enable_r, cdp1864_step_background_color_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(keyboard_latch_w)
	AM_RANGE(0x04, 0x04) AM_READWRITE(cdp1864_audio_disable_r, cdp1864_tone_divisor_latch_w)
ADDRESS_MAP_END

// Telmac 2000E

static ADDRESS_MAP_START( tmc2000e_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_RAM									// Work RAM
	AM_RANGE(0xc000, 0xdfff) AM_ROM									// Monitor ROM
	AM_RANGE(0xfc00, 0xffff) AM_RAM AM_BASE(&colorram)				// Color RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( tmc2000e_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_WRITE(cdp1864_tone_divisor_latch_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(cdp1864_step_background_color_w)
	AM_RANGE(0x03, 0x03) AM_READWRITE(ascii_keyboard_r, keyboard_latch_w)
	AM_RANGE(0x04, 0x04) AM_READWRITE(io_r, io_w)
	AM_RANGE(0x05, 0x05) AM_READWRITE(vismac_r, vismac_w)
	AM_RANGE(0x06, 0x06) AM_READWRITE(floppy_r, floppy_w)
	AM_RANGE(0x07, 0x07) AM_READWRITE(input_port_8_r, io_select_w)
ADDRESS_MAP_END

// Telmac TMC-600

static ADDRESS_MAP_START( tmc600_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM									// SBASIC ROM
	AM_RANGE(0x4000, 0x4fff) AM_ROM									// SBASIC Expansion ROM 1 (SB040282)
	AM_RANGE(0x5000, 0x5fff) AM_ROM									// SBASIC Expansion ROM 2 (SBDOS)
	AM_RANGE(0x6000, 0x63ff) AM_RAM									// SBASIC Work RAM
	AM_RANGE(0x6400, 0x7fff) AM_RAM									// Work RAM
	AM_RANGE(0x8000, 0xf7ff) AM_RAM									// Expanded RAM
	AM_RANGE(0xf800, 0xfbff) AM_RAM AM_WRITE(cdp1869_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0xfc00, 0xfdff) AM_RAM									// Color RAM ???
	AM_RANGE(0xfe00, 0xffff) AM_RAM									// ???
ADDRESS_MAP_END

static ADDRESS_MAP_START( tmc600_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x03, 0x03) AM_WRITE(keyboard_latch_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(printer_w)
	AM_RANGE(0x05, 0x05) AM_WRITE(vismac_data_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(floppy_w)
	AM_RANGE(0x07, 0x07) AM_WRITE(vismac_register_w)
ADDRESS_MAP_END

/* Input Ports */

INPUT_PORTS_START( tmc1800 )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CODE(KEYCODE_0_PAD) PORT_CHAR('0')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CODE(KEYCODE_1_PAD) PORT_CHAR('1')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CODE(KEYCODE_2_PAD) PORT_CHAR('2')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CODE(KEYCODE_3_PAD) PORT_CHAR('3')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CODE(KEYCODE_4_PAD) PORT_CHAR('4')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CODE(KEYCODE_5_PAD) PORT_CHAR('5')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CODE(KEYCODE_6_PAD) PORT_CHAR('6')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CODE(KEYCODE_7_PAD) PORT_CHAR('7')

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CODE(KEYCODE_8_PAD) PORT_CHAR('8')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CODE(KEYCODE_9_PAD) PORT_CHAR('9')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
INPUT_PORTS_END

INPUT_PORTS_START( tmc2000e )
	PORT_START_TAG("DSW0")	// System Configuration DIPs
	PORT_DIPNAME( 0x80, 0x00, "Keyboard Type" )
	PORT_DIPSETTING(    0x00, "ASCII" )
	PORT_DIPSETTING(    0x80, "Matrix" )
	PORT_DIPNAME( 0x40, 0x00, "Operating System" )
	PORT_DIPSETTING(    0x00, "TOOL" )
	PORT_DIPSETTING(    0x40, "Floppy" )
	PORT_DIPNAME( 0x30, 0x00, "Display Interface" )
	PORT_DIPSETTING(    0x00, "PAL" )
	PORT_DIPSETTING(    0x10, "CDG-80" )
	PORT_DIPSETTING(    0x20, "VISMAC" )
	PORT_DIPSETTING(    0x30, "UART" )
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	// ram placement 0000-0fff ... f000-ffff

	PORT_START
	PORT_CONFNAME( 0xf000, 0xc000, "ROM Address" )
	PORT_CONFSETTING(      0x0000, "0000-1FFF" )
	PORT_CONFSETTING(      0x2000, "2000-3FFF" )
	PORT_CONFSETTING(      0x4000, "4000-5FFF" )
	PORT_CONFSETTING(      0x6000, "6000-7FFF" )
	PORT_CONFSETTING(      0x8000, "8000-9FFF" )
	PORT_CONFSETTING(      0xa000, "A000-BFFF" )
	PORT_CONFSETTING(      0xc000, "C000-DFFF" )
	PORT_CONFSETTING(      0xe000, "E000-FFFF" )

	PORT_START
	PORT_CONFNAME( 0xf000, 0xc000, "Startup Address" )
	PORT_CONFSETTING(      0x0000, "0000" )
	PORT_CONFSETTING(      0x1000, "1000" )
	PORT_CONFSETTING(      0x2000, "2000" )
	PORT_CONFSETTING(      0x3000, "3000" )
	PORT_CONFSETTING(      0x4000, "4000" )
	PORT_CONFSETTING(      0x5000, "5000" )
	PORT_CONFSETTING(      0x6000, "6000" )
	PORT_CONFSETTING(      0x7000, "7000" )
	PORT_CONFSETTING(      0x8000, "8000" )
	PORT_CONFSETTING(      0x9000, "9000" )
	PORT_CONFSETTING(      0xa000, "A000" )
	PORT_CONFSETTING(      0xb000, "B000" )
	PORT_CONFSETTING(      0xc000, "C000" )
	PORT_CONFSETTING(      0xd000, "D000" )
	PORT_CONFSETTING(      0xe000, "E000" )
	PORT_CONFSETTING(      0xf000, "F000" )
INPUT_PORTS_END

INPUT_PORTS_START( tmc600 )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CODE(KEYCODE_0_PAD) PORT_CHAR('0')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CODE(KEYCODE_1_PAD) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CODE(KEYCODE_2_PAD) PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CODE(KEYCODE_3_PAD) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CODE(KEYCODE_4_PAD) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CODE(KEYCODE_5_PAD) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CODE(KEYCODE_6_PAD) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CODE(KEYCODE_7_PAD) PORT_CHAR('7') PORT_CHAR('/')

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CODE(KEYCODE_8_PAD) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CODE(KEYCODE_9_PAD) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_TILDE) PORT_CHAR('@') PORT_CHAR('\\')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')

	PORT_START_TAG("IN3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')

	PORT_START_TAG("IN4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')

	PORT_START_TAG("IN5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('Å')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('Ä')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR('Ö')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('^') PORT_CHAR('~')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("BREAK") PORT_CODE(KEYCODE_END) PORT_CHAR(UCHAR_MAMEKEY(END))

	PORT_START_TAG("IN6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("(space)") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("DEL") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("E2") PORT_CODE(KEYCODE_RALT) PORT_CHAR(UCHAR_MAMEKEY(RALT))
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CTRL1") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CTRL2") PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_MAMEKEY(RCONTROL))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("E1") PORT_CODE(KEYCODE_LALT) PORT_CHAR(UCHAR_MAMEKEY(LALT))
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)

	PORT_START_TAG("IN7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SHIFT LOCK") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("(unknown)") PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("LINE FEED") PORT_CODE(KEYCODE_HOME) PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHAR(13)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x93") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
INPUT_PORTS_END

/* Graphics Layouts */

static gfx_layout charlayout_normal_size =
{
	CDP1869_CHAR_WIDTH, CDP1869_CHAR_HEIGHT_PAL,
	256,
	1,
	{ 0 },
	{ 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 2048*8 },
	8*8
};

static gfx_layout charlayout_double_size =
{
	CDP1869_CHAR_WIDTH*2, CDP1869_CHAR_HEIGHT_PAL*2,
	256,
	1,
	{ 0 },
	{ 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7 },
	{ 0*8, 0*8, 1*8, 1*8, 2*8, 2*8, 3*8, 3*8, 4*8, 4*8, 5*8, 5*8, 6*8, 6*8, 7*8, 7*8, 2048*8, 2048*8 },
	8*8
};

/* Graphics Decode Information */

static gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout_normal_size, 0, 128 },
	{ REGION_GFX1, 0, &charlayout_double_size, 0, 128 },
	{ -1 }
};

/* CDP1802 Interfaces */

// Telmac 1800

static void tmc1800_video_dma(int cycles)
{
}

static void tmc1800_out_q(int level)
{
}

static int tmc1800_in_ef(void)
{
	int flags = 0;

	/*
		EF1		?
		EF2		?
		EF3		keyboard
		EF4		?
	*/

	// keyboard
	if (~readinputport(keylatch / 8) & (1 << (keylatch % 8))) flags |= EF3;

	return flags;
}

static CDP1802_CONFIG tmc1800_config =
{
	tmc1800_video_dma,
	tmc1800_out_q,
	tmc1800_in_ef
};

// Telmac 2000

static void tmc2000_video_dma(int cycles)
{
}

static void tmc2000_out_q(int level)
{
	// CDP1864 sound generator on/off
	cdp1864_audio_output_w(level);

	// set Q led status
	set_led_status(1, level);
}

static int tmc2000_in_ef(void)
{
	int flags = 0;

	/*
		EF1		?
		EF2		?
		EF3		keyboard
		EF4		?
	*/

	// keyboard
	if (~readinputport(keylatch / 8) & (1 << (keylatch % 8))) flags |= EF3;

	return flags;
}

static CDP1802_CONFIG tmc2000_config =
{
	tmc2000_video_dma,
	tmc2000_out_q,
	tmc2000_in_ef
};

// Telmac 2000E

static void tmc2000e_video_dma(int cycles)
{
}

static void tmc2000e_out_q(int level)
{
	// CDP1864 sound generator on/off
	cdp1864_audio_output_w(level);

	// set Q led status
	// leds: Wait, Q, Power
	set_led_status(1, level);

	// tape control
	// floppy control (FDC-6)
}

static int tmc2000e_in_ef(void)
{
	int flags = 0;

	/*
		EF1		CDP1864
		EF2		tape, floppy
		EF3		keyboard
		EF4		I/O port
	*/

	// keyboard
	if (~readinputport(keylatch / 8) & (1 << (keylatch % 8))) flags |= EF3;

	return flags;
}

static CDP1802_CONFIG tmc2000e_config =
{
	tmc2000e_video_dma,
	tmc2000e_out_q,
	tmc2000e_in_ef
};

// Telmac TMC-600

static void tmc600_out_q(int level)
{
	//logerror("q level: %u\n", level);
}

static int tmc600_in_ef(void)
{
	int flags = 0;

	/*
		EF1		?
		EF2		?
		EF3		keyboard
		EF4		?
	*/

	// keyboard
	if (~readinputport(keylatch / 8) & (1 << (keylatch % 8))) flags |= EF3;

	return flags;
}

static CDP1802_CONFIG tmc600_config =
{
	NULL,
	tmc600_out_q,
	tmc600_in_ef
};

/* Interrupt Generators */

static INTERRUPT_GEN( telmac_frame_int )
{
}

/* Machine Initialization */

static MACHINE_RESET( tmc2000 )
{
	// set program counter to 0x8000
}

static MACHINE_RESET( tmc2000e )
{
	// set program counter to selected value
}

/* Machine Drivers */

static MACHINE_DRIVER_START( tmc1800 )
	// basic system hardware
	MDRV_CPU_ADD_TAG("main", CDP1802, CDP1864_CLK_FREQ)	// 1.75 MHz
	MDRV_CPU_PROGRAM_MAP(tmc1800_map, 0)
	MDRV_CPU_IO_MAP(tmc1800_io_map, 0)
	MDRV_CPU_VBLANK_INT(telmac_frame_int, 1)
	MDRV_CPU_CONFIG(tmc1800_config)

	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( tmc2000 )
	// basic system hardware
	MDRV_CPU_ADD_TAG("main", CDP1802, CDP1864_CLK_FREQ)	// 1.75 MHz
	MDRV_CPU_PROGRAM_MAP(tmc2000_map, 0)
	MDRV_CPU_IO_MAP(tmc2000_io_map, 0)
	MDRV_CPU_VBLANK_INT(telmac_frame_int, 1)
	MDRV_CPU_CONFIG(tmc2000_config)

	MDRV_MACHINE_RESET(tmc2000)

	MDRV_SCREEN_REFRESH_RATE(CDP1864_FPS)	// 50.08 Hz
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(CDP1864_SCREEN_WIDTH, CDP1864_SCANLINES)
	MDRV_SCREEN_VISIBLE_AREA(1*8, 13*8-1, 20, CDP1864_SCANLINES-4)
	MDRV_PALETTE_LENGTH(8)

	MDRV_PALETTE_INIT(tmc2000)
	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(cdp1864)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( tmc2000t )
	MDRV_IMPORT_FROM(tmc2000)

	// basic system hardware
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(tmc2000t_map, 0)
	MDRV_CPU_IO_MAP(tmc2000t_io_map, 0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( tmc2000e )
	MDRV_IMPORT_FROM(tmc2000)

	MDRV_MACHINE_RESET(tmc2000e)

	// basic system hardware
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(tmc2000e_map, 0)
	MDRV_CPU_IO_MAP(tmc2000e_io_map, 0)
	MDRV_CPU_CONFIG(tmc2000e_config)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( tmc600 )
	// basic system hardware
	MDRV_CPU_ADD(CDP1802, 3579545)	// 3.58 MHz		TODO: where is this derived from?
	MDRV_CPU_PROGRAM_MAP(tmc600_map, 0)
	MDRV_CPU_IO_MAP(tmc600_io_map, 0)
	MDRV_CPU_VBLANK_INT(telmac_frame_int, 1)
	MDRV_CPU_CONFIG(tmc600_config)

	MDRV_SCREEN_REFRESH_RATE(CDP1869_FPS_PAL)	// 50.09 Hz
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(CDP1869_SCREEN_WIDTH, CDP1869_SCANLINES_PAL)
	MDRV_SCREEN_VISIBLE_AREA(0, CDP1869_SCREEN_WIDTH-4*CDP1869_CHAR_WIDTH, 0, CDP1869_SCANLINES_PAL-4)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8)
	MDRV_COLORTABLE_LENGTH(2*8)

	MDRV_PALETTE_INIT(cdp1869)
	MDRV_VIDEO_START(cdp1869)
	MDRV_VIDEO_UPDATE(cdp1869)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
//	MDRV_SOUND_ADD(CUSTOM, cdp1869_sound)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( tmc1800 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "monitor",	0x8000, 0x0200, NO_DUMP )
ROM_END

ROM_START( tmc2000 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "200.m5",		0x0000, 0x0200, CRC(7f8e7ce4) SHA1(3302628f9a57ce456347f37c9a970be6390465e7) )
	ROM_RELOAD(				0x8000, 0x0200 )	// TODO: how to start program counter at 0x8000?
ROM_END

ROM_START( tmc2000t )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "tool2000",	0x8000, 0x0800, NO_DUMP )
ROM_END

ROM_START( tmc2000e )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "1",			0xc000, 0x0800, NO_DUMP )
	ROM_LOAD( "2",			0xc800, 0x0800, NO_DUMP )
	ROM_LOAD( "3",			0xd000, 0x0800, NO_DUMP )
	ROM_LOAD( "4",			0xd800, 0x0800, NO_DUMP )
ROM_END

ROM_START( tmc600 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "1",			0x0000, 0x1000, NO_DUMP )
	ROM_LOAD( "2",			0x1000, 0x1000, NO_DUMP )
	ROM_LOAD( "3",			0x2000, 0x1000, NO_DUMP )
	ROM_LOAD( "4",			0x3000, 0x1000, NO_DUMP )
	ROM_LOAD( "190482_2",	0x4000, 0x1000, NO_DUMP )
	ROM_LOAD( "sbdos",		0x5000, 0x1000, NO_DUMP )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "chargen",	0x0000, 0x1000, NO_DUMP )
ROM_END

ROM_START( tmc600a )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "1",			0x0000, 0x1000, CRC(95d1292a) SHA1(1fa52d59d3005f8ac74a32c2164fdb22947c2748) )
	ROM_LOAD( "2",			0x1000, 0x1000, CRC(2c8f3d17) SHA1(f14e8adbcddeaeaa29b1e7f3dfa741f4e230f599) )
	ROM_LOAD( "3",			0x2000, 0x1000, CRC(dd58a128) SHA1(be9bdb0fc5e0cc3dcc7f2fb7ccab69bf5b043803) )
	ROM_LOAD( "4",			0x3000, 0x1000, CRC(b7d241fa) SHA1(6f3eadf86c4e3aaf93d123e302a18dc4d9db964b) )
	ROM_LOAD( "151182",		0x4000, 0x1000, CRC(c1a8d9d8) SHA1(4552e1f06d0e338ba7b0f1c3a20b8a51c27dafde) )
	ROM_LOAD( "sbdos",		0x5000, 0x1000, NO_DUMP )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "chargen",	0x0000, 0x1000, CRC(93f92cbf) SHA1(371156fb38fa5319c6fde537ccf14eed94e7adfb) )
ROM_END

ROM_START( tmc600as )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "1",			0x0000, 0x1000, NO_DUMP )
	ROM_LOAD( "2",			0x1000, 0x1000, NO_DUMP )
	ROM_LOAD( "3",			0x2000, 0x1000, NO_DUMP )
	ROM_LOAD( "4",			0x3000, 0x1000, NO_DUMP )
	ROM_LOAD( "5",			0x4000, 0x1000, NO_DUMP )
	ROM_LOAD( "6",			0x5000, 0x1000, NO_DUMP )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "chargen",	0x0000, 0x1000, NO_DUMP )
ROM_END

/* System Configuration */

QUICKLOAD_LOAD( tmc600 )
{
/*	int i;
	int size = mame_fsize(fp);
	UINT8 *buffer = malloc(size);

	if (!buffer)
		return INIT_FAIL;
logerror("malloc ok\n");

	mame_fread(fp, buffer, size);
logerror("read ok\n");

	for (i = 0; i < size; i++)
		program_write_byte(0x6300 + i, buffer[i]);
logerror("memwrite ok\n");

	free(buffer);
*/
	image_fread(image, memory_region(REGION_CPU1) + 0x6300, 0x9500);

	return INIT_PASS;
}

static void tmc1800_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START( tmc1800 )
	CONFIG_RAM_DEFAULT	( 2 * 1024)
	CONFIG_RAM			( 4 * 1024)
	CONFIG_DEVICE(tmc1800_cassette_getinfo)
SYSTEM_CONFIG_END

static void tmc2000_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START( tmc2000 )
	CONFIG_RAM_DEFAULT	(16 * 1024)
	CONFIG_RAM			(32 * 1024)
	CONFIG_DEVICE(tmc2000_cassette_getinfo)
SYSTEM_CONFIG_END

static void tmc2000e_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START( tmc2000e )
	CONFIG_RAM_DEFAULT	( 8 * 1024)
	CONFIG_DEVICE(tmc2000e_cassette_getinfo)
SYSTEM_CONFIG_END

static void tmc600_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

static void tmc600_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_basicdsk_floppy; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "dsk"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

static void tmc600_printer_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* printer */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										printer_device_getinfo(devclass, state, info); break;
	}
}

static void tmc600_quickload_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* quickload */
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "sbp"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_QUICKLOAD_LOAD:				info->f = (genf *) quickload_load_tmc600; break;

		default:										quickload_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START( tmc600 )
	CONFIG_RAM_DEFAULT	( 9 * 1024)
	CONFIG_RAM			(30 * 1024)
	CONFIG_DEVICE(tmc600_cassette_getinfo)
	CONFIG_DEVICE(tmc600_floppy_getinfo)
	CONFIG_DEVICE(tmc600_printer_getinfo)
	CONFIG_DEVICE(tmc600_quickload_getinfo)
SYSTEM_CONFIG_END

static void setup_beep(int dummy)
{
	beep_set_volume(0, 0);
	beep_set_state(0, 1);
}

static DRIVER_INIT( telmac )
{
	timer_set(0.0, 0, setup_beep);
}



/* System Drivers */

//     YEAR  NAME 	   PARENT   COMPAT   MACHINE   INPUT     INIT	 CONFIG    COMPANY 	      FULLNAME
COMP( 1977, tmc1800,  0,       0,	     tmc1800,  tmc1800,  telmac, tmc1800,  "Telercas Oy", "Telmac 1800", GAME_NOT_WORKING )
COMP( 1980, tmc2000,  0,       tmc1800, tmc2000,  tmc1800,  telmac, tmc2000,  "Telercas Oy", "Telmac 2000", GAME_NOT_WORKING )
COMP( 1980, tmc2000t, tmc2000, tmc1800, tmc2000t, tmc1800,  telmac, tmc2000,  "Telercas Oy", "Telmac 2000 (TOOL-2000)", GAME_NOT_WORKING )
COMP( 1980, tmc2000e, 0,       0,	     tmc2000e, tmc2000e, telmac, tmc2000e, "Telercas Oy", "Telmac 2000E", GAME_NOT_WORKING )
COMP( 1982, tmc600,   0,       0,	     tmc600,   tmc600,   telmac, tmc600,   "Telercas Oy", "Telmac TMC-600 (Series I)", GAME_NOT_WORKING )
COMP( 1982, tmc600a,  tmc600,  0,	     tmc600,   tmc600,   telmac, tmc600,   "Telercas Oy", "Telmac TMC-600 (Series II)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
COMP( 1982, tmc600as, tmc600,  0,	     tmc600,   tmc600,   telmac, tmc600,   "Telercas Oy", "Telmac TMC-600 AS", GAME_NOT_WORKING )
