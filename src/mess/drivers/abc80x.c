/*
	abc800.c

	MESS Driver by Curt Coder

	Luxor ABC 800C
	--------------
	(c) 1981 Luxor Datorer AB, Sweden

	CPU:			Z80 @ 3 MHz
	ROM:			32 KB
	RAM:			16 KB, 1 KB frame buffer, 16 KB high-resolution videoram (800C/HR)
	CRTC:			6845
	Resolution:		240x240
	Colors:			8

	Luxor ABC 800M
	--------------
	(c) 1981 Luxor Datorer AB, Sweden

	CPU:			Z80 @ 3 MHz
	ROM:			32 KB
	RAM:			16 KB, 2 KB frame buffer, 16 KB high-resolution videoram (800M/HR)
	CRTC:			6845
	Resolution:		480x240, 240x240 (HR)
	Colors:			2

	Luxor ABC 802
	-------------
	(c) 1983 Luxor Datorer AB, Sweden

	CPU:			Z80 @ 3 MHz
	ROM:			32 KB
	RAM:			16 KB, 2 KB frame buffer, 16 KB ram-floppy
	CRTC:			6845
	Resolution:		480x240
	Colors:			2

	Luxor ABC 806
	-------------
	(c) 1983 Luxor Datorer AB, Sweden

	CPU:			Z80 @ 3 MHz
	ROM:			32 KB
	RAM:			32 KB, 4 KB frame buffer, 128 KB scratch pad ram
	CRTC:			6845
	Resolution:		240x240, 256x240, 512x240
	Colors:			8

	http://www.devili.iki.fi/Computers/Luxor/
	http://hem.passagen.se/mani/abc/
*/

/*

	TODO:

	- keyboard ROM dump is needed!
	- keyboard NE556 discrete beeper
	- ABC806 memory banking
	- proper port mirroring
	- COM port DIP switch
	- use MAME CRTC6845 implementation
	- HR graphics board
	- floppy controller board
	- Facit DTC (recased ABC-800?)
	- hard disks (ABC-850 10MB, ABC-852 20MB, ABC-856 60MB)

*/

#include "driver.h"
#include "inputx.h"
#include "video/generic.h"
#include "includes/centroni.h"
#include "includes/serial.h"
#include "devices/basicdsk.h"
#include "devices/cassette.h"
#include "devices/printer.h"
#include "cpu/z80/z80daisy.h"
#include "cpu/i8039/i8039.h"
#include "machine/z80ctc.h"
#include "machine/z80sio.h"
#include "machine/z80dart.h"
#include "machine/abcbus.h"
#include "video/abc80x.h"

static mame_timer *abc800_ctc_timer;

/* Read/Write Handlers */

// HR

static WRITE8_HANDLER( hrs_w )
{
}

static WRITE8_HANDLER( hrc_w )
{
}

// ABC 806

static READ8_HANDLER( bankswitch_r )
{
	return 0;
}

static WRITE8_HANDLER( bankswitch_w )
{
}

static READ8_HANDLER( attribute_r )
{
	return 0;
}

static WRITE8_HANDLER( attribute_w )
{
}

static READ8_HANDLER( sync_r )
{
	return 0;
}

static WRITE8_HANDLER( fgctlprom_w )
{
}

static WRITE8_HANDLER( ram_ctrl_w )
{
}

// SIO/2

static READ8_HANDLER( sio2_r )
{
	switch (offset)
	{
	case 0:
		return z80sio_d_r(0, 0);
	case 1:
		return z80sio_c_r(0, 0);
	case 2:
		return z80sio_d_r(0, 1);
	case 3:
		return z80sio_c_r(0, 1);
	}

	return 0;
}

static WRITE8_HANDLER( sio2_w )
{
	switch (offset)
	{
	case 0:
		z80sio_d_w(0, 0, data);
		break;
	case 1:
		z80sio_c_w(0, 0, data);
		break;
	case 2:
		z80sio_d_w(0, 1, data);
		break;
	case 3:
		z80sio_c_w(0, 1, data);
		break;
	}
}

// DART

static READ8_HANDLER( dart_r )
{
	switch (offset)
	{
	case 0:
		return z80dart_d_r(0, 0);
	case 1:
		return z80dart_c_r(0, 0);
	case 2:
		return z80dart_d_r(0, 1);
	case 3:
		return z80dart_c_r(0, 1);
	}

	return 0xff;
}

static WRITE8_HANDLER( dart_w )
{
	switch (offset)
	{
	case 0:
		z80dart_d_w(0, 0, data);
		break;
	case 1:
		z80dart_c_w(0, 0, data);
		break;
	case 2:
		z80dart_d_w(0, 1, data);
		break;
	case 3:
		z80dart_c_w(0, 1, data);
		break;
	}
}

// ABC-77 keyboard

/*

	Keyboard connector pinout

	pin		description		connected to
	-------------------------------------------
	1		TxD				i8035 pin 36 (P25)
	2		GND
	3		RxD				i8035 pin 6 (_INT)
	4		CLOCK			i8035 pin 39 (T1)
	5		_KEYDOWN		i8035 pin 37 (P26)
	6		+12V
	7		_RESET			NE556 pin 8 (2TRIG)

*/

static int abc77_keylatch, abc77_clock;

static READ8_HANDLER( abc77_clock_r )
{
	return abc77_clock;
}

static READ8_HANDLER( abc77_data_r )
{
	return readinputport(abc77_keylatch);
}

static WRITE8_HANDLER( abc77_data_w )
{
	abc77_keylatch = data & 0x0f;

	if (abc77_keylatch == 1)
	{
		watchdog_reset_w(0, 0);
	}

//	abc77_beep = data & 0x10;
	z80dart_set_dcd(0, 0, (data & 0x40) ? 0 : 1);
//	abc77_hys = data & 0x80;
}

/* Memory Maps */

// ABC 800

static ADDRESS_MAP_START( abc800_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x0000, 0x77ff) AM_ROM
	AM_RANGE(0x7800, 0x7fff) AM_RAM AM_WRITE(abc800_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x8000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( abc800_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) | AMEF_UNMAP(1) )
	AM_RANGE(0x00, 0x00) AM_READWRITE(abcbus_data_r, abcbus_data_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(abcbus_status_r, abcbus_channel_w)
	AM_RANGE(0x02, 0x05) AM_WRITE(abcbus_command_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(hrs_w)
	AM_RANGE(0x07, 0x07) AM_READWRITE(abcbus_reset_r, hrc_w)
	AM_RANGE(0x20, 0x23) AM_READWRITE(dart_r, dart_w)
	AM_RANGE(0x30, 0x32) AM_WRITE(ram_ctrl_w)
	AM_RANGE(0x31, 0x31) AM_READ(crtc6845_register_r)
	AM_RANGE(0x38, 0x38) AM_WRITE(crtc6845_address_w)
	AM_RANGE(0x39, 0x39) AM_WRITE(crtc6845_register_w)
	AM_RANGE(0x40, 0x43) AM_READWRITE(sio2_r, sio2_w)
	AM_RANGE(0x50, 0x53) AM_READWRITE(z80ctc_0_r, z80ctc_0_w)
	AM_RANGE(0x80, 0xff) AM_READWRITE(abcbus_strobe_r, abcbus_strobe_w)
ADDRESS_MAP_END

// ABC 77 keyboard

static ADDRESS_MAP_START( abc77_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x000, 0xfff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( abc77_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(I8039_p1, I8039_p1) AM_READ(abc77_data_r)
	AM_RANGE(I8039_p2, I8039_p2) AM_WRITE(abc77_data_w)
	AM_RANGE(I8039_t1, I8039_t1) AM_READ(abc77_clock_r)
	AM_RANGE(I8039_bus, I8039_bus) AM_READ(port_tag_to_handler8("DSW"))
ADDRESS_MAP_END

// ABC 802

static ADDRESS_MAP_START( abc802_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x0000, 0x77ff) AM_RAMBANK(1)
	AM_RANGE(0x7800, 0x7fff) AM_RAM AM_WRITE(abc800_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x8000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( abc802_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) | AMEF_UNMAP(1) )
	AM_RANGE(0x00, 0x00) AM_READWRITE(abcbus_data_r, abcbus_data_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(abcbus_status_r, abcbus_channel_w)
	AM_RANGE(0x02, 0x05) AM_WRITE(abcbus_command_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(hrs_w)
	AM_RANGE(0x07, 0x07) AM_READWRITE(abcbus_reset_r, hrc_w)
	AM_RANGE(0x20, 0x23) AM_READWRITE(dart_r, dart_w)
	AM_RANGE(0x31, 0x31) AM_READ(crtc6845_register_r)
	AM_RANGE(0x32, 0x35) AM_READWRITE(sio2_r, sio2_w)
	AM_RANGE(0x38, 0x38) AM_WRITE(crtc6845_address_w)
	AM_RANGE(0x39, 0x39) AM_WRITE(crtc6845_register_w)
	AM_RANGE(0x60, 0x63) AM_READWRITE(z80ctc_0_r, z80ctc_0_w)
	AM_RANGE(0x80, 0xff) AM_READWRITE(abcbus_strobe_r, abcbus_strobe_w)
ADDRESS_MAP_END

// ABC 806

static ADDRESS_MAP_START( abc806_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x0000, 0x77ff) AM_RAMBANK(1)
	AM_RANGE(0x7800, 0x7fff) AM_RAM AM_WRITE(abc800_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x8000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( abc806_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) | AMEF_UNMAP(1) )
	AM_RANGE(0x00, 0x00) AM_READWRITE(abcbus_data_r, abcbus_data_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(abcbus_status_r, abcbus_channel_w)
	AM_RANGE(0x02, 0x05) AM_WRITE(abcbus_command_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(hrs_w)
	AM_RANGE(0x07, 0x07) AM_READWRITE(abcbus_reset_r, hrc_w)
	AM_RANGE(0x20, 0x23) AM_READWRITE(dart_r, dart_w)
	AM_RANGE(0x31, 0x31) AM_READ(crtc6845_register_r)
	AM_RANGE(0x34, 0x34) AM_READWRITE(bankswitch_r, bankswitch_w)
	AM_RANGE(0x35, 0x35) AM_READWRITE(attribute_r, attribute_w)
	AM_RANGE(0x37, 0x37) AM_READWRITE(sync_r, fgctlprom_w)
	AM_RANGE(0x38, 0x38) AM_WRITE(crtc6845_address_w)
	AM_RANGE(0x39, 0x39) AM_WRITE(crtc6845_register_w)
	AM_RANGE(0x40, 0x41) AM_READWRITE(sio2_r, sio2_w)
	AM_RANGE(0x60, 0x63) AM_READWRITE(z80ctc_0_r, z80ctc_0_w)
	AM_RANGE(0x80, 0xff) AM_READWRITE(abcbus_strobe_r, abcbus_strobe_w)
ADDRESS_MAP_END

/* Input Ports */

INPUT_PORTS_START( abc77 )
	PORT_START_TAG("X0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CAPS LOCK") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RIGHT SHIFT") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("LEFT SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)

	PORT_START_TAG("X1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("X2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("< >") PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('<') PORT_CHAR('>')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR('\r')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("LEFT") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RIGHT") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))

	PORT_START_TAG("X3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR(0x00C9)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('+') PORT_CHAR('?')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR(0x00C5)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(0x00DC)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(0x00C4)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\'') PORT_CHAR('*')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')

	PORT_START_TAG("X4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('=')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(0x00D6)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR(':')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('-') PORT_CHAR('_')

	PORT_START_TAG("X5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('/')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR(';')

	PORT_START_TAG("X6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')

	PORT_START_TAG("X7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR(0x00A4)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')

	PORT_START_TAG("X8")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X')

	PORT_START_TAG("X9")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9_PAD) PORT_NAME("KEYPAD 9")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_PLUS_PAD) PORT_NAME("KEYPAD +")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6_PAD) PORT_NAME("KEYPAD 6")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS_PAD) PORT_NAME("KEYPAD -")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3_PAD) PORT_NAME("KEYPAD 3")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_ENTER_PAD) PORT_NAME("KEYPAD RETURN")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_DEL_PAD) PORT_NAME("KEYPAD .")

	PORT_START_TAG("X10")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7_PAD) PORT_NAME("KEYPAD 7")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8_PAD) PORT_NAME("KEYPAD 8")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4_PAD) PORT_NAME("KEYPAD 4")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5_PAD) PORT_NAME("KEYPAD 5")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1_PAD) PORT_NAME("KEYPAD 1")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2_PAD) PORT_NAME("KEYPAD 2")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0_PAD) PORT_NAME("KEYPAD 0")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("X11")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F1) PORT_NAME("PF1")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F2) PORT_NAME("PF2")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F3) PORT_NAME("PF3")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F4) PORT_NAME("PF4")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F5) PORT_NAME("PF5")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F6) PORT_NAME("PF6")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F7) PORT_NAME("PF7")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F8) PORT_NAME("PF8")

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x01, 0x01, "Keyboard Program" )
	PORT_DIPSETTING(    0x00, "Internal (8048)" )
	PORT_DIPSETTING(    0x01, "External PROM" )
	PORT_DIPNAME( 0x02, 0x00, "Character Set" )
	PORT_DIPSETTING(    0x00, "Swedish" )
	PORT_DIPSETTING(    0x02, "US ASCII" )
	PORT_DIPNAME( 0x04, 0x04, "External PROM" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, "Keyboard Type" )
	PORT_DIPSETTING(    0x00, "Danish" )
	PORT_DIPSETTING(    0x10, DEF_STR( French ) )
	PORT_DIPSETTING(    0x08, DEF_STR( German ) )
	PORT_DIPSETTING(    0x18, DEF_STR( Spanish ) )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END

INPUT_PORTS_START( abc800 )
	PORT_INCLUDE(abc77)
INPUT_PORTS_END

INPUT_PORTS_START( abc802 )
	PORT_INCLUDE(abc77)
INPUT_PORTS_END

INPUT_PORTS_START( abc806 )
	PORT_INCLUDE(abc77)
INPUT_PORTS_END

/* Graphics Layouts */

static const gfx_layout charlayout_abc800m =
{
	6, 10,
	128,
	1,
	{ 0 },
	{ 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8 },
	16*8
};

static const gfx_layout charlayout_abc802_40 =
{
	12, 10,
	256,
	1,
	{ 0 },
	{ 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8 },
	16*8
};

static const gfx_layout charlayout_abc802_80 =
{
	6, 10,
	256,
	1,
	{ 0 },
	{ 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8 },
	16*8
};

/* Graphics Decode Info */

static GFXDECODE_START( gfxdecodeinfo_abc800m )
	GFXDECODE_ENTRY( REGION_GFX1, 0, charlayout_abc800m, 0, 2 )
GFXDECODE_END

static GFXDECODE_START( gfxdecodeinfo_abc802 )
	GFXDECODE_ENTRY( REGION_GFX1, 0, charlayout_abc802_40, 0, 8 )
	GFXDECODE_ENTRY( REGION_GFX1, 0, charlayout_abc802_80, 0, 8 )
GFXDECODE_END

/* Machine Initialization */

static WRITE8_HANDLER( abc800_ctc_z2_w )
{
	abc77_clock = data;
}

static z80ctc_interface abc800_ctc_intf =
{
	ABC800_X01/2/2,			/* clock */
	0,              		/* timer disables */
	0,				  		/* interrupt handler */
	0,						/* ZC/TO0 callback */
	0,              		/* ZC/TO1 callback */
	abc800_ctc_z2_w    		/* ZC/TO2 callback */
};

static WRITE8_HANDLER( sio_serial_transmit )
{
}

static int sio_serial_receive(int ch)
{
	return -1;
}

static z80sio_interface abc800_sio_intf =
{
	ABC800_X01/2/2,			/* clock */
	0,						/* interrupt handler */
	0,						/* DTR changed handler */
	0,						/* RTS changed handler */
	0,						/* BREAK changed handler */
	sio_serial_transmit,	/* transmit handler */
	sio_serial_receive		/* receive handler */
};

static WRITE8_HANDLER( dart_serial_transmit )
{
}

static int dart_serial_receive(int ch)
{
	return -1;
}

static z80dart_interface abc800_dart_intf =
{
	ABC800_X01/2/2,			/* clock */
	0,						/* interrupt handler */
	0,						/* DTR changed handler */
	0,						/* RTS changed handler */
	0,						/* BREAK changed handler */
	dart_serial_transmit,	/* transmit handler */
	dart_serial_receive		/* receive handler */
};

static WRITE8_HANDLER( abc802_dart_dtr_w )
{
	if (offset == 1)
	{
		memory_set_bank(1, data);
	}
}

static WRITE8_HANDLER( abc802_dart_rts_w )
{
	if (offset == 1)
	{
		abc802_set_columns(data ? 40 : 80);
	}
}

static z80dart_interface abc802_dart_intf =
{
	ABC800_X01/2/2,			/* clock */
	0,						/* interrupt handler */
	abc802_dart_dtr_w,		/* DTR changed handler */
	abc802_dart_rts_w,		/* RTS changed handler */
	0,						/* BREAK changed handler */
	dart_serial_transmit,	/* transmit handler */
	dart_serial_receive		/* receive handler */
};

static struct z80_irq_daisy_chain abc800_daisy_chain[] =
{
	{ z80ctc_reset, z80ctc_irq_state, z80ctc_irq_ack, z80ctc_irq_reti, 0 },
	{ z80sio_reset, z80sio_irq_state, z80sio_irq_ack, z80sio_irq_reti, 0 },
	{ z80dart_reset, z80dart_irq_state, z80dart_irq_ack, z80dart_irq_reti, 0 },
	{ 0, 0, 0, 0, -1 }
};

static TIMER_CALLBACK(abc800_ctc_tick)
{
	z80ctc_trg_w(0, 0, 1);
	z80ctc_trg_w(0, 0, 0);
	z80ctc_trg_w(0, 1, 1);
	z80ctc_trg_w(0, 1, 0);
	z80ctc_trg_w(0, 2, 1);
	z80ctc_trg_w(0, 2, 0);
}

static MACHINE_START( abc800 )
{
	state_save_register_global(abc77_keylatch);
	state_save_register_global(abc77_clock);

	abc800_ctc_timer = mame_timer_alloc(abc800_ctc_tick);
	mame_timer_adjust(abc800_ctc_timer, time_zero, 0, MAME_TIME_IN_HZ(ABC800_X01/2/2/2));

	z80ctc_init(0, &abc800_ctc_intf);
	z80sio_init(0, &abc800_sio_intf);
	z80dart_init(0, &abc800_dart_intf);
}

INTERRUPT_GEN( abc802_vblank_interrupt )
{
	z80dart_set_ri(0, 0);
	z80dart_set_ri(0, 1);
}

static MACHINE_START( abc802 )
{
	machine_start_abc800(machine);
	
	z80dart_init(0, &abc802_dart_intf);

	memory_configure_bank(1, 0, 1, memory_region(REGION_CPU1), 0);
	memory_configure_bank(1, 1, 1, mess_ram, 0);
}

static MACHINE_RESET( abc802 )
{
	memory_set_bank(1, 0);
}

static MACHINE_START( abc806 )
{
	machine_start_abc800(machine);
	
	memory_configure_bank(1, 0, 1, memory_region(REGION_CPU1), 0);
	memory_configure_bank(1, 1, 1, mess_ram, 0);
}

static MACHINE_RESET( abc806 )
{
	memory_set_bank(1, 0);
}

/* Machine Drivers */

static MACHINE_DRIVER_START( abc800m )
	// basic machine hardware
	MDRV_CPU_ADD_TAG("main", Z80, ABC800_X01/2/2)	// 3 MHz
	MDRV_CPU_CONFIG(abc800_daisy_chain)
	MDRV_CPU_PROGRAM_MAP(abc800_map, 0)
	MDRV_CPU_IO_MAP(abc800_io_map, 0)

	MDRV_CPU_ADD(I8035, 4608000) // 4.608 MHz, keyboard cpu
	MDRV_CPU_PROGRAM_MAP(abc77_map, 0)
	MDRV_CPU_IO_MAP(abc77_io_map, 0)

	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_START(abc800)

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(80*6, 24*10)
	MDRV_SCREEN_VISIBLE_AREA(0, 80*6-1, 0, 24*10-1)
	MDRV_GFXDECODE(gfxdecodeinfo_abc800m)
	MDRV_PALETTE_LENGTH(2)

	MDRV_PALETTE_INIT(abc800m)
	MDRV_VIDEO_START(abc800m)
	MDRV_VIDEO_UPDATE(abc800)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( abc800c )
	MDRV_IMPORT_FROM(abc800m)
	
	MDRV_SCREEN_SIZE(40*6, 24*10)
	MDRV_SCREEN_VISIBLE_AREA(0, 40*6-1, 0, 24*10-1)
	MDRV_PALETTE_LENGTH(8)

	MDRV_PALETTE_INIT(abc800c)
	MDRV_VIDEO_START(abc800c)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( abc802 )
	MDRV_IMPORT_FROM(abc800m)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(abc802_map, 0)
	MDRV_CPU_IO_MAP(abc802_io_map, 0)
	MDRV_CPU_VBLANK_INT(abc802_vblank_interrupt, 1)
	
	MDRV_MACHINE_START(abc802)
	MDRV_MACHINE_RESET(abc802)

	MDRV_GFXDECODE(gfxdecodeinfo_abc802)
	MDRV_PALETTE_INIT(abc800m)
	MDRV_VIDEO_START(abc802)
	MDRV_VIDEO_UPDATE(abc802)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( abc806 )
	MDRV_IMPORT_FROM(abc800m)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(abc806_map, 0)
	MDRV_CPU_IO_MAP(abc806_io_map, 0)
	
	MDRV_MACHINE_START(abc806)
	MDRV_MACHINE_RESET(abc806)
MACHINE_DRIVER_END

/* ROMs */

/*

	ABC800 DOS ROMs

	Label		Drive Type
	----------------------------
	ABC 6-1X	ABC830,DD82,DD84
	800 8"		DD88
	ABC 6-2X	ABC832
	ABC 6-3X	ABC838
	UFD 6.XX	Winchester

*/

#define ROM_ABC77 \
	ROM_REGION( 0x1000, REGION_CPU2, 0 ) \
	ROM_LOAD( "65-02486.z10", 0x0000, 0x0800, NO_DUMP ) /* 2716 ABC55/77 keyboard controller Swedish EPROM */ \
	ROM_LOAD( "keyboard.z14", 0x0800, 0x0800, NO_DUMP ) /* 2716 ABC55/77 keyboard controller non-Swedish EPROM */

ROM_START( abc800c )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "abcc.1m",    0x0000, 0x1000, NO_DUMP )
	ROM_LOAD( "abc1-12.1l", 0x1000, 0x1000, CRC(1e99fbdc) SHA1(ec6210686dd9d03a5ed8c4a4e30e25834aeef71d) )
	ROM_LOAD( "abc2-12.1k", 0x2000, 0x1000, CRC(ac196ba2) SHA1(64fcc0f03fbc78e4c8056e1fa22aee12b3084ef5) )
	ROM_LOAD( "abc3-12.1j", 0x3000, 0x1000, CRC(3ea2b5ee) SHA1(5a51ac4a34443e14112a6bae16c92b5eb636603f) )
	ROM_LOAD( "abc4-12.2m", 0x4000, 0x1000, CRC(695cb626) SHA1(9603ce2a7b2d7b1cbeb525f5493de7e5c1e5a803) )
	ROM_LOAD( "abc5-12.2l", 0x5000, 0x1000, CRC(b4b02358) SHA1(95338efa3b64b2a602a03bffc79f9df297e9534a) )
	ROM_LOAD( "abc6-13.2k", 0x6000, 0x1000, CRC(6fa71fb6) SHA1(b037dfb3de7b65d244c6357cd146376d4237dab6) )
	ROM_LOAD( "abc7-21.2j", 0x7000, 0x1000, CRC(fd137866) SHA1(3ac914d90db1503f61397c0ea26914eb38725044) )

	ROM_ABC77

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "vuc-se.7c",  0x0000, 0x1000, NO_DUMP )

	ROM_REGION( 0x2000, REGION_USER1, 0 )
	// Fast Controller
	ROM_LOAD( "fast108.bin",  0x0000, 0x2000, CRC(229764cb) SHA1(a2e2f6f49c31b827efc62f894de9a770b65d109d) ) // Luxor v1.08
	ROM_LOAD( "fast207.bin",  0x0000, 0x2000, CRC(86622f52) SHA1(61ad271de53152c1640c0b364fce46d1b0b4c7e2) ) // DIAB v2.07

	// MyAB Turbo-Kontroller
	ROM_LOAD( "unidis5d.bin", 0x0000, 0x1000, CRC(569dd60c) SHA1(47b810bcb5a063ffb3034fd7138dc5e15d243676) ) // 5" 25-pin
	ROM_LOAD( "unidiskh.bin", 0x0000, 0x1000, CRC(5079ad85) SHA1(42bb91318f13929c3a440de3fa1f0491a0b90863) ) // 5" 34-pin
	ROM_LOAD( "unidisk8.bin", 0x0000, 0x1000, CRC(d04e6a43) SHA1(8db504d46ff0355c72bd58fd536abeb17425c532) ) // 8"

	// ABC-832
	ROM_LOAD( "micr1015.bin", 0x0000, 0x0800, CRC(a7bc05fa) SHA1(6ac3e202b7ce802c70d89728695f1cb52ac80307) ) // Micropolis 1015
	ROM_LOAD( "micr1115.bin", 0x0000, 0x0800, CRC(f2fc5ccc) SHA1(86d6baadf6bf1d07d0577dc1e092850b5ff6dd1b) ) // Micropolis 1115
	ROM_LOAD( "basf6118.bin", 0x0000, 0x0800, CRC(9ca1a1eb) SHA1(04973ad69de8da403739caaebe0b0f6757e4a6b1) ) // BASF 6118

	// ABC-850
	ROM_LOAD( "rodi202.bin",  0x0000, 0x0800, CRC(337b4dcf) SHA1(791ebeb4521ddc11fb9742114018e161e1849bdf) ) // Rodime 202
	ROM_LOAD( "basf6185.bin", 0x0000, 0x0800, CRC(06f8fe2e) SHA1(e81f2a47c854e0dbb096bee3428d79e63591059d) ) // BASF 6185

	// ABC-852
	ROM_LOAD( "nec5126.bin",  0x0000, 0x1000, CRC(17c247e7) SHA1(7339738b87751655cb4d6414422593272fe72f5d) ) // NEC 5126

	// ABC-856
	ROM_LOAD( "micr1325.bin", 0x0000, 0x0800, CRC(084af409) SHA1(342b8e214a8c4c2b014604e53c45ef1bd1c69ea3) ) // Micropolis 1325

	// unknown
	ROM_LOAD( "st4038.bin",   0x0000, 0x0800, CRC(4c803b87) SHA1(1141bb51ad9200fc32d92a749460843dc6af8953) ) // Seagate ST4038
	ROM_LOAD( "st225.bin",    0x0000, 0x0800, CRC(c9f68f81) SHA1(7ff8b2a19f71fe0279ab3e5a0a5fffcb6030360c) ) // Seagate ST225
ROM_END

ROM_START( abc800m )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "abcm.1m",    0x0000, 0x1000, CRC(f85b274c) SHA1(7d0f5639a528d8d8130a22fe688d3218c77839dc) )
	ROM_LOAD( "abc1-12.1l", 0x1000, 0x1000, CRC(1e99fbdc) SHA1(ec6210686dd9d03a5ed8c4a4e30e25834aeef71d) )
	ROM_LOAD( "abc2-12.1k", 0x2000, 0x1000, CRC(ac196ba2) SHA1(64fcc0f03fbc78e4c8056e1fa22aee12b3084ef5) )
	ROM_LOAD( "abc3-12.1j", 0x3000, 0x1000, CRC(3ea2b5ee) SHA1(5a51ac4a34443e14112a6bae16c92b5eb636603f) )
	ROM_LOAD( "abc4-12.2m", 0x4000, 0x1000, CRC(695cb626) SHA1(9603ce2a7b2d7b1cbeb525f5493de7e5c1e5a803) )
	ROM_LOAD( "abc5-12.2l", 0x5000, 0x1000, CRC(b4b02358) SHA1(95338efa3b64b2a602a03bffc79f9df297e9534a) )
	ROM_LOAD( "abc6-13.2k", 0x6000, 0x1000, CRC(6fa71fb6) SHA1(b037dfb3de7b65d244c6357cd146376d4237dab6) )
	ROM_LOAD( "abc7-21.2j", 0x7000, 0x1000, CRC(fd137866) SHA1(3ac914d90db1503f61397c0ea26914eb38725044) )

	ROM_ABC77

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "vum-se.7c",  0x0000, 0x0800, CRC(f9152163) SHA1(997313781ddcbbb7121dbf9eb5f2c6b4551fc799) )
ROM_END

ROM_START( abc802 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD(  "abc02-11.9f",  0x0000, 0x2000, CRC(b86537b2) SHA1(4b7731ef801f9a03de0b5acd955f1e4a1828355d) )
	ROM_LOAD(  "abc12-11.11f", 0x2000, 0x2000, CRC(3561c671) SHA1(f12a7c0fe5670ffed53c794d96eb8959c4d9f828) )
	ROM_LOAD(  "abc22-11.12f", 0x4000, 0x2000, CRC(8dcb1cc7) SHA1(535cfd66c84c0370fd022d6edf702d3d1ad1b113) )
	ROM_SYSTEM_BIOS( 0, "v19",		"UDF-DOS v.19" )
	ROMX_LOAD( "abc32-21.14f", 0x6000, 0x2000, CRC(57050b98) SHA1(b977e54d1426346a97c98febd8a193c3e8259574), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "v20",		"UDF-DOS v.20" )
	ROMX_LOAD( "abc32-31.14f", 0x6000, 0x2000, CRC(fc8be7a8) SHA1(a1d4cb45cf5ae21e636dddfa70c99bfd2050ad60), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "mica",		"MICA DOS v.20" )
	ROMX_LOAD( "mica820.14f",  0x6000, 0x2000, CRC(edf998af) SHA1(daae7e1ff6ef3e0ddb83e932f324c56f4a98f79b), ROM_BIOS(3) )

	ROM_ABC77

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "abct2-11.3g",  0x0000, 0x2000, CRC(e21601ee) SHA1(2e838ebd7692e5cb9ba4e80fe2aa47ea2584133a) )

	ROM_REGION( 0x400, REGION_PROMS, 0 )
	ROM_LOAD( "abcp2-11.2g", 0x0000, 0x0400, NO_DUMP ) // PAL16R4
ROM_END

ROM_START( abc806 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "abc06-11.1m",  0x0000, 0x1000, CRC(27083191) SHA1(9b45592273a5673e4952c6fe7965fc9398c49827) )
	ROM_LOAD( "abc16-11.1l",  0x1000, 0x1000, CRC(eb0a08fd) SHA1(f0b82089c5c8191fbc6a3ee2c78ce730c7dd5145) )
	ROM_LOAD( "abc26-11.1k",  0x2000, 0x1000, CRC(97a95c59) SHA1(013bc0a2661f4630c39b340965872bf607c7bd45) )
	ROM_LOAD( "abc36-11.1j",  0x3000, 0x1000, CRC(b50e418e) SHA1(991a59ed7796bdcfed310012b2bec50f0b8df01c) )
	ROM_LOAD( "abc46-11.2m",  0x4000, 0x1000, CRC(17a87c7d) SHA1(49a7c33623642b49dea3d7397af5a8b9dde8185b) )
	ROM_LOAD( "abc56-11.2l",  0x5000, 0x1000, CRC(b4b02358) SHA1(95338efa3b64b2a602a03bffc79f9df297e9534a) )
	ROM_SYSTEM_BIOS( 0, "v19",		"UDF-DOS v.19" )
	ROMX_LOAD( "abc66-21.2k", 0x6000, 0x1000, CRC(c311b57a) SHA1(4bd2a541314e53955a0d53ef2f9822a202daa485), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "v20",		"UDF-DOS v.20" )
	ROMX_LOAD( "abc66-31.2k", 0x6000, 0x1000, CRC(a2e38260) SHA1(0dad83088222cb076648e23f50fec2fddc968883), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "mica",		"MICA DOS v.20" )
	ROMX_LOAD( "mica2006.2k", 0x6000, 0x1000, CRC(58bc2aa8) SHA1(0604bd2396f7d15fcf3d65888b4b673f554037c0), ROM_BIOS(3) )
	ROM_SYSTEM_BIOS( 3, "catnet",	"CAT-NET" )
	ROMX_LOAD( "cmd8_5.2k",	  0x6000, 0x1000, CRC(25430ef7) SHA1(03a36874c23c215a19b0be14ad2f6b3b5fb2c839), ROM_BIOS(4) )
	ROM_LOAD( "abc76-11.2j",  0x7000, 0x1000, CRC(3eb5f6a1) SHA1(02d4e38009c71b84952eb3b8432ad32a98a7fe16) ) // 6490238-02

	ROM_ABC77

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "abct6-11.7c",   0x0000, 0x1000, CRC(b17c51c5) SHA1(e466e80ec989fbd522c89a67d274b8f0bed1ff72) ) // 6490243-01

	ROM_REGION( 0x400, REGION_PROMS, 0 )
	ROM_LOAD( "abcp3-11.1b", 0x0000, 0x0400, NO_DUMP ) // PAL16R4
	ROM_LOAD( "abcp4-11.2d", 0x0000, 0x0400, NO_DUMP ) // PAL16R4
	ROM_LOAD( "rad.9b",		 0x0000, 0x0400, NO_DUMP ) // 7621/7643 == 82S131/82S137
	ROM_LOAD( "atthand.11c", 0x0000, 0x0400, NO_DUMP ) // 40033A
	ROM_LOAD( "hrui.6e",	 0x0000, 0x0400, NO_DUMP ) // 7603 == 82S123
	ROM_LOAD( "v5o.7e",		 0x0000, 0x0400, NO_DUMP ) // 7621 == 82S131
ROM_END

/* System Configuration */

static void abc800_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

static void abc800_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_abc_floppy; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "dsk"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

static void abc800_printer_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* printer */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										printer_device_getinfo(devclass, state, info); break;
	}
}

DEVICE_LOAD( abc800_serial )
{
	/* filename specified */
	if (serial_device_load(image)==INIT_PASS)
	{
		/* setup transmit parameters */
		serial_device_setup(image, 9600 >> readinputportbytag("BAUD"), 8, 1, SERIAL_PARITY_NONE);

		serial_device_set_protocol(image, SERIAL_PROTOCOL_NONE);

		/* and start transmit */
		serial_device_set_transmit_state(image, 1);

		return INIT_PASS;
	}

	return INIT_FAIL;
}

static void abc800_serial_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* serial */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_SERIAL; break;
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_INIT:							info->init = serial_device_init; break;
		case DEVINFO_PTR_LOAD:							info->load = device_load_abc800_serial; break;
		case DEVINFO_PTR_UNLOAD:						info->unload = serial_device_unload; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "txt"); break;
	}
}

SYSTEM_CONFIG_START( abc800 )
	CONFIG_RAM_DEFAULT(16 * 1024)
	CONFIG_RAM		  (32 * 1024)
	CONFIG_DEVICE(abc800_cassette_getinfo)
	CONFIG_DEVICE(abc800_printer_getinfo)
	CONFIG_DEVICE(abc800_floppy_getinfo)
	CONFIG_DEVICE(abc800_serial_getinfo)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START( abc802 )
	CONFIG_RAM_DEFAULT(64 * 1024)
	CONFIG_DEVICE(abc800_cassette_getinfo)
	CONFIG_DEVICE(abc800_printer_getinfo)
	CONFIG_DEVICE(abc800_floppy_getinfo)
	CONFIG_DEVICE(abc800_serial_getinfo)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START( abc806 )
	CONFIG_RAM_DEFAULT(64 * 1024)
	CONFIG_DEVICE(abc800_cassette_getinfo)
	CONFIG_DEVICE(abc800_printer_getinfo)
	CONFIG_DEVICE(abc800_floppy_getinfo)
	CONFIG_DEVICE(abc800_serial_getinfo)
SYSTEM_CONFIG_END

/* Driver Initialization */

static offs_t abc800_opbase_handler(offs_t pc)
{
	if (pc >= 0x7800 && pc < 0x8000)
	{
		opcode_base = opcode_arg_base = memory_region(REGION_CPU1);
		return ~0;
	}

	return pc;
}

static DRIVER_INIT( abc800 )
{
	memory_set_opbase_handler(0, abc800_opbase_handler);
}

/* System Drivers */

//	   YEAR  NAME		PARENT		BIOS	COMPAT	MACHINE		INPUT		INIT	CONFIG		COMPANY			FULLNAME
COMP ( 1981, abc800c,	0,					0,		abc800c,	abc800,		abc800,	abc800,		"Luxor Datorer AB", "ABC 800C", GAME_NOT_WORKING )
COMP ( 1981, abc800m,	abc800c,			0,		abc800m,	abc800,		abc800,	abc800,		"Luxor Datorer AB", "ABC 800M", GAME_NOT_WORKING )
COMPB( 1983, abc802,	0,			abc802,	0,		abc802,		abc802,		abc800,	abc802,		"Luxor Datorer AB", "ABC 802", GAME_NOT_WORKING )
COMPB( 1983, abc806,	0,			abc806,	0,		abc806,		abc806,		abc800,	abc806,		"Luxor Datorer AB", "ABC 806", GAME_NOT_WORKING )
