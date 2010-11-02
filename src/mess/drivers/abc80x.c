/*

Luxor ABC 800C/800M

Main PCB Layout
---------------

|-----------------------|-----------------------------------|
|   4116    4116        |                                   |
|   4116    4116        |                                   |
|   4116    4116        |                                   |
|   4116    4116        |           Z80         Z80CTC      |
|   4116    4116        |CN1                                |
|   4116    4116        |                                   |
|   4116    4116        |                       Z80DART     |
|                       |                       CN6         |
|   ROM3    ROM7        |                                   |
|                       |                       Z80SIO2     |
|   ROM2    ROM6        |-----------------------------------|
|                                                           |
|   ROM1    ROM5                                            |
|                                                       CN2 |
|   ROM0    ROM4                                            |
|                       CN7                                 |
|-------------------------------------------|           CN3 |
|                                           |               |
|                                           |               |
|                                           |           CN4 |
|                                           |               |
|               VIDEO BOARD                 |               |
|                                           |           CN5 |
|                                           |               |
|                                           |               |
|                                           |               |
|-------------------------------------------|---------------|

Notes:
    Relevant IC's shown.

    Z80     - ? Z80A CPU (labeled "Z80A (800) KASS.")
    Z80CTC  - Sharp LH0082A Z80A-CTC
    Z80DART - SGS Z8470AB1 Z80A-DART
    Z8OSIO2 - SGS Z8442AB1 Z80A-SIO/2
    4116    - Toshiba TMS4116-20NL 1Kx8 RAM
    ROM0    - NEC D2732D 4Kx8 EPROM "ABC M-12"
    ROM1    - NEC D2732D 4Kx8 EPROM "ABC 1-12"
    ROM2    - NEC D2732D 4Kx8 EPROM "ABC 2-12"
    ROM3    - NEC D2732D 4Kx8 EPROM "ABC 3-12"
    ROM4    - NEC D2732D 4Kx8 EPROM "ABC 4-12"
    ROM5    - NEC D2732D 4Kx8 EPROM "ABC 5-12"
    ROM6    - Hitachi HN462732G 4Kx8 EPROM "ABC 6-52"
    ROM7    - NEC D2732D 4Kx8 EPROM "ABC 7-22"
    CN1     - ABC bus connector
    CN2     - tape connector
    CN3     - RS-232 channel B connector
    CN4     - RS-232 channel A connector
    CN5     - video connector
    CN6     - keyboard connector
    CN7     - video board connector


Video PCB Layout (Color)
------------------------

55 10789-01

|-------------------------------------------|
|                                           |
|                                           |
|				2114						|
|                                           |
|				2114		TROM			|
|                                           |
|		12MHz				TIC				|
|                                           |
|                                           |
|-------------------------------------------|

Notes:
    Relevant IC's shown.

    2114    - GTE Microcircuits 2114-2CB 1Kx4 RAM
    TIC     - Philips SAA5020 Teletext Timing Chain
    TROM    - Philips SAA5052 Teletext Character Generator w/Swedish Character Set


Video PCB Layout (Monochrome)
-----------------------------

55 10790-02

|-------------------------------------------|
|                                           |
|               2114                        |
|               2114                        |
|                                      12MHz|
|               2114                        |
|               2114                        |
|                                           |
|               CRTC            ROM0        |
|                                           |
|-------------------------------------------|

Notes:
    Relevant IC's shown.

    2114    - Toshiba TMM314AP-1 1Kx4 RAM
    CRTC    - Hitachi HD46505SP CRTC
    ROM0    - NEC D2716D 2Kx8 EPROM "VUM SE"


Video PCB Layout (High Resolution)
----------------------------------

55 10791-01

|-------------------------------------------|
|											|
|	4116		PROM1						|
|	4116									|
|	4116									|
|	4116									|
|	4116									|
|	4116									|
|	4116									|
|	4116				PROM0				|
|-------------------------------------------|

Notes:
    Relevant IC's shown.

    4116    - Motorola MCM4116AC25 1Kx8 RAM
	PROM0	- some kind of PROM "HRU I"
	PROM1	- some kind of PROM "HRU II"


UNI-800 PCB Layout
------------------

8120 821025 REV.3

|-------------------------------------------|
|											|
|					4164		PROM0	CN3	|
|					4164					|
|					4164					|
|					4164					|
|					4164					|
|					4164					|
|CN1				4164				CN2	|
|					4164					|
|-------------------------------------------|

Notes:
    Relevant IC's shown.

    4164    - Hitachi HM4864P-2 64Kx1 RAM
	PROM0	- some kind of PROM ".800 1.2"
	CN1		- 2x6 pin PCB header
	CN2		- 2x10 pin PCB header
	CN3		- 2x10 pin PCB header


Keyboard PCB Layout
-------------------

KTC A65-02201-201K

    |---------------|
|---|      CN1      |---------------------------------------------------|
|                                                                       |
|       LS193       LS374                       8048        22-008-03   |
|   LS13    LS193               22-050-3B   5.9904MHz                   |
|                                                                       |
|                                                                       |
|                                                                       |
|                                                                       |
|                                                                       |
|                                                                       |
|                                                                       |
|                                                                       |
|-----------------------------------------------------------------------|

Notes:
    All IC's shown.

    8048        - National Semiconductor INS8048 MCU "8048-132"
    22-008-03   - Exar Semiconductor XR22-008-03 keyboard matrix capacitive readout latch
    22-050-3B   - Exar Semiconductor XR22-050-3B keyboard matrix row driver with 4 to 12 decoder/demultiplexer
    CN1         - keyboard data connector


XR22-008-03 Pinout
------------------
            _____   _____
    D0   1 |*    \_/     | 20  Vcc
    D1   2 |             | 19  _CLR
    D2   3 |             | 18  Q0
    D3   4 |             | 17  Q1
   HYS   5 |  22-008-03  | 16  Q2
    D4   6 |             | 15  Q3
    D5   7 |             | 14  Q4
    D6   8 |             | 13  Q5
    D7   9 |             | 12  Q6
   GND  10 |_____________| 11  Q7


XR22-050-3B Pinout
------------------
            _____   _____
    Y8   1 |*    \_/     | 20  Vcc
    Y9   2 |             | 19  Y7
   Y10   3 |             | 18  Y6
   Y11   4 |             | 17  Y5
  _STB   5 |  22-050-3B  | 16  Y4
    A0   6 |             | 15  Y3
    A1   7 |             | 14  Y2
    A2   8 |             | 13  Y1
    A3   9 |             | 12  Y0
   GND  10 |_____________| 11  OE?

*/

/*

    TODO:

	- update Z80 DART/CTC/SIO references to use new device system
	- rewrite abc806 bankswitch to use install_ram/rom
	- ABC800 keyboard: T1 clock frequency, caps lock led, keydown
    - ABC802/806 ABC77 keyboard
    - hard disks (ABC-850 10MB, ABC-852 20MB, ABC-856 60MB)

*/

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "includes/abc80x.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "cpu/mcs48/mcs48.h"
#include "devices/cassette.h"
#include "devices/printer.h"
#include "devices/messram.h"
#include "machine/serial.h"
#include "machine/z80ctc.h"
#include "machine/z80dart.h"
#include "machine/abcbus.h"
#include "machine/e0516.h"
#include "machine/abc77.h"
#include "machine/abc830.h"
#include "video/mc6845.h"
#include "video/saa5050.h"
#include "sound/discrete.h"



//**************************************************************************
//	KEYBOARD HACK
//**************************************************************************

static INPUT_PORTS_START( fake_keyboard )
	PORT_START("ROW0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("4 \xC2\xA4") PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR(0x00A4)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('/')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')

	PORT_START("ROW1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('=')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('+') PORT_CHAR('?')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR(0x00E9) PORT_CHAR(0x00C9)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('<') PORT_CHAR('>')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')

	PORT_START("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR(0x00E5) PORT_CHAR(0x00C5)

	PORT_START("ROW3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(0x00FC) PORT_CHAR(0x00DC)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR('\r')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')

	PORT_START("ROW4")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(0x00F6) PORT_CHAR(0x00D6)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(0x00E4) PORT_CHAR(0x00C4)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\'') PORT_CHAR('*')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')

	PORT_START("ROW5")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR(';')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR(':')

	PORT_START("ROW6")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("ROW7")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("LEFT SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("RIGHT SHIFT") PORT_CODE(KEYCODE_RSHIFT)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("UPPER CASE") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK)) PORT_TOGGLE
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

static int keylatch;

static const UINT8 abc800_keycodes[7*4][8] =
{
	// unshift
	{ 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38 },
	{ 0x39, 0x30, 0x2b, 0x60, 0x3c, 0x71, 0x77, 0x65 },
	{ 0x72, 0x74, 0x79, 0x75, 0x69, 0x6f, 0x70, 0x7d },
	{ 0x7e, 0x0d, 0x61, 0x73, 0x64, 0x66, 0x67, 0x68 },
	{ 0x6a, 0x6b, 0x6c, 0x7c, 0x7b, 0x27, 0x08, 0x7a },
	{ 0x78, 0x63, 0x76, 0x62, 0x6e, 0x6d, 0x2c, 0x2e },
	{ 0x2d, 0x09, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00 },

	// shift
	{ 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x2f, 0x28 },
	{ 0x29, 0x3d, 0x3f, 0x40, 0x3e, 0x51, 0x57, 0x45 },
	{ 0x52, 0x54, 0x59, 0x55, 0x49, 0x4f, 0x50, 0x5d },
	{ 0x5e, 0x0d, 0x41, 0x53, 0x44, 0x46, 0x47, 0x48 },
	{ 0x4a, 0x4b, 0x4c, 0x5c, 0x5b, 0x2a, 0x08, 0x5a },
	{ 0x58, 0x43, 0x56, 0x42, 0x4e, 0x4d, 0x3b, 0x3a },
	{ 0x5f, 0x09, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00 },

	// control
	{ 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38 },
	{ 0x39, 0x30, 0x2b, 0x00, 0x7f, 0x11, 0x17, 0x05 },
	{ 0x12, 0x14, 0x19, 0x15, 0x09, 0x0f, 0x10, 0x1d },
	{ 0x1e, 0x0d, 0x01, 0x13, 0x04, 0x06, 0x07, 0x08 },
	{ 0x0a, 0x0b, 0x0c, 0x1c, 0x1b, 0x27, 0x08, 0x1a },
	{ 0x18, 0x03, 0x16, 0x02, 0x0e, 0x0d, 0x2c, 0x2e },
	{ 0x2d, 0x09, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00 },

	// control-shift
	{ 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x2f, 0x28 },
	{ 0x29, 0x3d, 0x3f, 0x00, 0x7f, 0x11, 0x17, 0x05 },
	{ 0x12, 0x14, 0x19, 0x15, 0x09, 0x1f, 0x00, 0x1d },
	{ 0x1e, 0x0d, 0x01, 0x13, 0x04, 0x06, 0x07, 0x08 },
	{ 0x0a, 0x1b, 0x1c, 0x1c, 0x1b, 0x2a, 0x08, 0x1a },
	{ 0x18, 0x03, 0x16, 0x02, 0x1e, 0x1d, 0x3b, 0x3a },
	{ 0x5f, 0x09, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

static void scan_keyboard(running_machine *machine)
{
	UINT8 keycode = 0;
	UINT8 data;
	int table = 0, row, col;
	static const char *const keynames[] = { "ROW0", "ROW1", "ROW2", "ROW3", "ROW4", "ROW5", "ROW6" };

	// shift, upper case
	if (input_port_read(machine, "ROW7") & 0x07)
	{
		table |= 0x01;
	}

	// ctrl
	if (input_port_read(machine, "ROW7") & 0x08)
	{
		table |= 0x02;
	}

	for (row = 0; row < 7; row++)
	{
		data = input_port_read(machine, keynames[row]);

		if (data != 0)
		{
			UINT8 ibit = 1;

			for (col = 0; col < 8; col++)
			{
				if (data & ibit) keycode = abc800_keycodes[row + (table * 7)][col];
				ibit <<= 1;
			}
		}
	}

	if (keycode)
	{
		if (keycode != keylatch)
		{
			z80dart_device *m_dart = machine->device<z80dart_device>(Z80DART_TAG);

			z80dart_dcdb_w(m_dart, 0);
			m_dart->receive_data(1, keycode);

			keylatch = keycode;
		}
	}
	else
	{
		if (keylatch)
		{
			z80dart_device *m_dart = machine->device<z80dart_device>(Z80DART_TAG);

			z80dart_dcdb_w(m_dart, 1);
			m_dart->receive_data(1, 0);

			keylatch = 0;
		}
	}
}

static TIMER_DEVICE_CALLBACK( keyboard_tick )
{
	scan_keyboard(timer.machine);
}



//**************************************************************************
//	SOUND
//**************************************************************************

//-------------------------------------------------
//  DISCRETE_SOUND( abc800 )
//-------------------------------------------------

static DISCRETE_SOUND_START( abc800 )
	DISCRETE_INPUT_LOGIC(NODE_01)
	DISCRETE_OUTPUT(NODE_01, 5000)
DISCRETE_SOUND_END


//-------------------------------------------------
//  pling_r - speaker read
//-------------------------------------------------

READ8_MEMBER( abc800_state::pling_r )
{
	m_pling = !m_pling;

	discrete_sound_w(m_discrete, NODE_01, m_pling);

	return 0xff;
}


//-------------------------------------------------
//  pling_r - speaker read
//-------------------------------------------------

READ8_MEMBER( abc802_state::pling_r )
{
	m_pling = !m_pling;

	discrete_sound_w(m_discrete, NODE_01, m_pling);

	return 0xff;
}



//**************************************************************************
//	MEMORY BANKING
//**************************************************************************

//-------------------------------------------------
//  bankswitch
//-------------------------------------------------

void abc800_state::bankswitch()
{
	address_space *program = cpu_get_address_space(m_maincpu, ADDRESS_SPACE_PROGRAM);

	if (m_fetch_charram)
	{
		// HR video RAM selected
		memory_install_ram(program, 0x0000, 0x3fff, 0, 0, m_video_ram);
	}
	else
	{
		// BASIC ROM selected
		memory_install_rom(program, 0x0000, 0x3fff, 0, 0, memory_region(machine, Z80_TAG));
	}
}


//-------------------------------------------------
//  bankswitch
//-------------------------------------------------

void abc802_state::bankswitch()
{
	address_space *program = cpu_get_address_space(m_maincpu, ADDRESS_SPACE_PROGRAM);

	if (m_lrs)
	{
		// ROM and video RAM selected
		memory_install_rom(program, 0x0000, 0x77ff, 0, 0, memory_region(machine, Z80_TAG));
		memory_install_ram(program, 0x7800, 0x7fff, 0, 0, m_char_ram);
	}
	else
	{
		// low RAM selected
		memory_install_ram(program, 0x0000, 0x7fff, 0, 0, messram_get_ptr(m_ram));
	}
}


//-------------------------------------------------
//  bankswitch
//-------------------------------------------------

void abc806_state::bankswitch()
{
	address_space *program = cpu_get_address_space(m_maincpu, ADDRESS_SPACE_PROGRAM);
	UINT32 videoram_mask = messram_get_size(m_ram) - (32 * 1024) - 1;
	int bank;
	char bank_name[10];

	if (!m_keydtr)
	{
		// 32K block mapping

		UINT32 videoram_start = (m_hrs & 0xf0) << 11;

		for (bank = 1; bank <= 8; bank++)
		{
			// 0x0000-0x7FFF is video RAM

			UINT16 start_addr = 0x1000 * (bank - 1);
			UINT16 end_addr = start_addr + 0xfff;
			UINT32 videoram_offset = (videoram_start + start_addr) & videoram_mask;
			sprintf(bank_name,"bank%d",bank);
			//logerror("%04x-%04x: Video RAM %04x (32K)\n", start_addr, end_addr, videoram_offset);

			memory_install_readwrite_bank(program, start_addr, end_addr, 0, 0, bank_name);
			memory_configure_bank(machine, bank_name, 1, 1, m_video_ram + videoram_offset, 0);
			memory_set_bank(machine, bank_name, 1);
		}

		for (bank = 9; bank <= 16; bank++)
		{
			// 0x8000-0xFFFF is main RAM

			UINT16 start_addr = 0x1000 * (bank - 1);
			UINT16 end_addr = start_addr + 0xfff;
			sprintf(bank_name,"bank%d",bank);
			//logerror("%04x-%04x: Work RAM (32K)\n", start_addr, end_addr);

			memory_install_readwrite_bank(program, start_addr, end_addr, 0, 0, bank_name);
			memory_set_bank(machine, bank_name, 0);
		}
	}
	else
	{
		// 4K block mapping

		for (bank = 1; bank <= 16; bank++)
		{
			UINT16 start_addr = 0x1000 * (bank - 1);
			UINT16 end_addr = start_addr + 0xfff;
			UINT8 map = m_map[bank - 1];
			UINT32 videoram_offset = ((map & 0x7f) << 12) & videoram_mask;
			sprintf(bank_name,"bank%d",bank);
			if (BIT(map, 7) && m_eme)
			{
				// map to video RAM
				//logerror("%04x-%04x: Video RAM %04x (4K)\n", start_addr, end_addr, videoram_offset);

				memory_install_readwrite_bank(program, start_addr, end_addr, 0, 0, bank_name);
				memory_configure_bank(machine, bank_name, 1, 1, m_video_ram + videoram_offset, 0);
				memory_set_bank(machine, bank_name, 1);
			}
			else
			{
				// map to ROM/RAM

				switch (bank)
				{
				case 1: case 2: case 3: case 4: case 5: case 6: case 7:
					// ROM
					//logerror("%04x-%04x: ROM (4K)\n", start_addr, end_addr);

					memory_install_read_bank(program, start_addr, end_addr, 0, 0, bank_name);
					memory_unmap_write(program, start_addr, end_addr, 0, 0);
					memory_set_bank(machine, bank_name, 0);
					break;

				case 8:
					// ROM/char RAM
					//logerror("%04x-%04x: ROM (4K)\n", start_addr, end_addr);

					memory_install_read_bank(program, 0x7000, 0x77ff, 0, 0, bank_name);
					memory_unmap_write(program, 0x7000, 0x77ff, 0, 0);
					program->install_handler(0x7800, 0x7fff, 0, 0, read8_delegate_create(abc806_state, charram_r, *this), write8_delegate_create(abc806_state, charram_w, *this));
					memory_set_bank(machine, bank_name, 0);
					break;

				default:
					// work RAM
					//logerror("%04x-%04x: Work RAM (4K)\n", start_addr, end_addr);

					memory_install_readwrite_bank(program, start_addr, end_addr, 0, 0, bank_name);
					memory_set_bank(machine, bank_name, 0);
					break;
				}
			}
		}
	}

	if (m_fetch_charram)
	{
		// 30K block mapping

		UINT32 videoram_start = (m_hrs & 0xf0) << 11;

		for (bank = 1; bank <= 8; bank++)
		{
			// 0x0000-0x77FF is video RAM

			UINT16 start_addr = 0x1000 * (bank - 1);
			UINT16 end_addr = start_addr + 0xfff;
			UINT32 videoram_offset = (videoram_start + start_addr) & videoram_mask;
			sprintf(bank_name,"bank%d",bank);
			//logerror("%04x-%04x: Video RAM %04x (30K)\n", start_addr, end_addr, videoram_offset);

			if (start_addr == 0x7000)
			{
				memory_install_readwrite_bank(program, 0x7000, 0x77ff, 0, 0, bank_name);
				program->install_handler(0x7800, 0x7fff, 0, 0, read8_delegate_create(abc806_state, charram_r, *this), write8_delegate_create(abc806_state, charram_w, *this));
			}
			else
			{
				memory_install_readwrite_bank(program, start_addr, end_addr, 0, 0, bank_name);
			}

			memory_configure_bank(machine, bank_name, 1, 1, m_video_ram + videoram_offset, 0);
			memory_set_bank(machine, bank_name, 1);
		}
	}
}


//-------------------------------------------------
//  mai_r - memory bank map read
//-------------------------------------------------

READ8_MEMBER( abc806_state::mai_r )
{
	int bank = offset >> 12;

	return m_map[bank];
}


//-------------------------------------------------
//  mao_w - memory bank map write
//-------------------------------------------------

WRITE8_MEMBER( abc806_state::mao_w )
{
	/*

        bit     description

        0       physical block address bit 0
        1       physical block address bit 1
        2       physical block address bit 2
        3       physical block address bit 3
        4       physical block address bit 4
        5
        6
        7       allocate block

    */

	int bank = offset >> 12;

	m_map[bank] = data;

	bankswitch();
}



//**************************************************************************
//	INTEGRATED KEYBOARD
//**************************************************************************

//-------------------------------------------------
//  TIMER_DEVICE_CALLBACK( keyboard_t1_tick )
//-------------------------------------------------

static TIMER_DEVICE_CALLBACK( keyboard_t1_tick )
{
	abc800_state *state = timer.machine->driver_data<abc800_state>();

	state->m_kb_clk = !state->m_kb_clk;

	z80dart_rxtxcb_w(state->m_dart, state->m_kb_clk);
}


//-------------------------------------------------
//  keyboard_txd_r - keyboard transmit data read
//-------------------------------------------------

READ_LINE_MEMBER( abc800_state::keyboard_txd_r )
{
	return m_kb_txd;
}


//-------------------------------------------------
//  keyboard_rxd_w - keyboard receive data write
//-------------------------------------------------

static WRITE_LINE_DEVICE_HANDLER( keyboard_rxd_w )
{
	cpu_set_input_line(device, MCS48_INPUT_IRQ, state ? CLEAR_LINE : ASSERT_LINE);
}


//-------------------------------------------------
//  keyboard_col_r - keyboard column data read
//-------------------------------------------------

READ8_MEMBER( abc800_state::keyboard_col_r )
{
	static const char *const ABC800_KEY_ROW[] = { "Y0", "Y1", "Y2", "Y3", "Y4", "Y5", "Y6", "Y7", "Y8", "Y9", "Y10", "Y11" };
	UINT8 data = 0xff;

	if (m_kb_stb && m_kb_row < 12)
	{
		data = input_port_read(&m_machine, ABC800_KEY_ROW[m_kb_row]);
	}

	return data;
}


//-------------------------------------------------
//  keyboard_row_w - keyboard row write
//-------------------------------------------------

WRITE8_MEMBER( abc800_state::keyboard_row_w )
{
	/*

		bit		description

		0		A0
		1		A1
		2		A2
		3		A3
		4		?
		5		?
		6		
		7		

	*/

	// keyboard row
	if (!m_kb_stb)
	{
		m_kb_row = data & 0x0f;
	}
}


//-------------------------------------------------
//  keyboard_ctrl_w - keyboard control write
//-------------------------------------------------

WRITE8_MEMBER( abc800_state::keyboard_ctrl_w )
{
	/*

		bit		description

		0		
		1		
		2		
		3		
		4		TxD
		5		?
		6		22-008-03 CLR, 22-050-3B STB
		7		22-008-03 HYS

	*/

	// TxD
	m_kb_txd = !BIT(data, 4);

	// keydown
	z80dart_dcdb_w(m_dart, !BIT(data, 5));

	// strobe
	m_kb_stb = BIT(data, 6);
}


//-------------------------------------------------
//  keyboard_t1_r - keyboard T1 timer read
//-------------------------------------------------

READ8_MEMBER( abc800_state::keyboard_t1_r )
{
	return m_kb_clk;
}



//**************************************************************************
//	ADDRESS MAPS
//**************************************************************************

//-------------------------------------------------
//  ADDRESS_MAP( abc800c_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( abc800c_mem, ADDRESS_SPACE_PROGRAM, 8, abc800_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x3fff) AM_RAM AM_BASE(m_video_ram)
	AM_RANGE(0x4000, 0x7bff) AM_ROM
	AM_RANGE(0x7c00, 0x7fff) AM_DEVREADWRITE_LEGACY(SAA5052_TAG, saa5050_videoram_r, saa5050_videoram_w)
	AM_RANGE(0x8000, 0xffff) AM_RAM
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( abc800c_io )
//-------------------------------------------------

static ADDRESS_MAP_START( abc800c_io, ADDRESS_SPACE_IO, 8, abc800_state )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_MIRROR(0x18) AM_DEVREADWRITE_LEGACY(ABCBUS_TAG, abcbus_inp_r, abcbus_utp_w)
	AM_RANGE(0x01, 0x01) AM_MIRROR(0x18) AM_DEVREADWRITE_LEGACY(ABCBUS_TAG, abcbus_stat_r, abcbus_cs_w)
	AM_RANGE(0x02, 0x02) AM_MIRROR(0x18) AM_DEVWRITE_LEGACY(ABCBUS_TAG, abcbus_c1_w)
	AM_RANGE(0x03, 0x03) AM_MIRROR(0x18) AM_DEVWRITE_LEGACY(ABCBUS_TAG, abcbus_c2_w)
	AM_RANGE(0x04, 0x04) AM_MIRROR(0x18) AM_DEVWRITE_LEGACY(ABCBUS_TAG, abcbus_c3_w)
	AM_RANGE(0x05, 0x05) AM_MIRROR(0x18) AM_DEVWRITE_LEGACY(ABCBUS_TAG, abcbus_c4_w)
	AM_RANGE(0x05, 0x05) AM_MIRROR(0x18) AM_READ(pling_r)
	AM_RANGE(0x06, 0x06) AM_MIRROR(0x18) AM_WRITE(hrs_w)
	AM_RANGE(0x07, 0x07) AM_MIRROR(0x18) AM_DEVREAD_LEGACY(ABCBUS_TAG, abcbus_rst_r) AM_WRITE(hrc_w)
	AM_RANGE(0x20, 0x23) AM_MIRROR(0x0c) AM_DEVREADWRITE_LEGACY(Z80DART_TAG, z80dart_ba_cd_r, z80dart_ba_cd_w)
	AM_RANGE(0x40, 0x43) AM_MIRROR(0x1c) AM_DEVREADWRITE_LEGACY(Z80SIO_TAG, z80dart_ba_cd_r, z80dart_ba_cd_w)
	AM_RANGE(0x60, 0x63) AM_MIRROR(0x1c) AM_DEVREADWRITE_LEGACY(Z80CTC_TAG, z80ctc_r, z80ctc_w)
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( abc800_keyboard_io )
//-------------------------------------------------

static ADDRESS_MAP_START( abc800_keyboard_io, ADDRESS_SPACE_IO, 8, abc800_state )
	AM_RANGE(MCS48_PORT_P1, MCS48_PORT_P1) AM_READWRITE(keyboard_col_r, keyboard_row_w)
	AM_RANGE(MCS48_PORT_P2, MCS48_PORT_P2) AM_WRITE(keyboard_ctrl_w)
	AM_RANGE(MCS48_PORT_T1, MCS48_PORT_T1) AM_READ(keyboard_t1_r)
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( abc800m_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( abc800m_mem, ADDRESS_SPACE_PROGRAM, 8, abc800_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x3fff) AM_RAM AM_BASE(m_video_ram)
	AM_RANGE(0x4000, 0x77ff) AM_ROM
	AM_RANGE(0x7800, 0x7fff) AM_RAM AM_BASE(m_char_ram)
	AM_RANGE(0x8000, 0xffff) AM_RAM
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( abc800m_io )
//-------------------------------------------------

static ADDRESS_MAP_START( abc800m_io, ADDRESS_SPACE_IO, 8, abc800_state )
	AM_IMPORT_FROM( abc800c_io )
	AM_RANGE(0x31, 0x31) AM_MIRROR(0x06) AM_DEVREAD_LEGACY(MC6845_TAG, mc6845_register_r)
	AM_RANGE(0x38, 0x38) AM_MIRROR(0x06) AM_DEVWRITE_LEGACY(MC6845_TAG, mc6845_address_w)
	AM_RANGE(0x39, 0x39) AM_MIRROR(0x06) AM_DEVWRITE_LEGACY(MC6845_TAG, mc6845_register_w)
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( abc802_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( abc802_mem, ADDRESS_SPACE_PROGRAM, 8, abc802_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x77ff) AM_ROM
	AM_RANGE(0x7800, 0x7fff) AM_RAM AM_BASE(m_char_ram)
	AM_RANGE(0x8000, 0xffff) AM_RAM
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( abc802_io )
//-------------------------------------------------

static ADDRESS_MAP_START( abc802_io, ADDRESS_SPACE_IO, 8, abc802_state )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_MIRROR(0x18) AM_DEVREADWRITE_LEGACY(ABCBUS_TAG, abcbus_inp_r, abcbus_utp_w)
	AM_RANGE(0x01, 0x01) AM_MIRROR(0x18) AM_DEVREADWRITE_LEGACY(ABCBUS_TAG, abcbus_stat_r, abcbus_cs_w)
	AM_RANGE(0x02, 0x02) AM_MIRROR(0x18) AM_DEVWRITE_LEGACY(ABCBUS_TAG, abcbus_c1_w)
	AM_RANGE(0x03, 0x03) AM_MIRROR(0x18) AM_DEVWRITE_LEGACY(ABCBUS_TAG, abcbus_c2_w)
	AM_RANGE(0x04, 0x04) AM_MIRROR(0x18) AM_DEVWRITE_LEGACY(ABCBUS_TAG, abcbus_c3_w)
	AM_RANGE(0x05, 0x05) AM_MIRROR(0x18) AM_DEVWRITE_LEGACY(ABCBUS_TAG, abcbus_c4_w)
	AM_RANGE(0x05, 0x05) AM_MIRROR(0x08) AM_READ(pling_r)
	AM_RANGE(0x07, 0x07) AM_MIRROR(0x18) AM_DEVREAD_LEGACY(ABCBUS_TAG, abcbus_rst_r)
	AM_RANGE(0x20, 0x23) AM_MIRROR(0x0c) AM_DEVREADWRITE_LEGACY(Z80DART_TAG, z80dart_ba_cd_r, z80dart_ba_cd_w)
	AM_RANGE(0x31, 0x31) AM_MIRROR(0x06) AM_DEVREAD_LEGACY(MC6845_TAG, mc6845_register_r)
	AM_RANGE(0x38, 0x38) AM_MIRROR(0x06) AM_DEVWRITE_LEGACY(MC6845_TAG, mc6845_address_w)
	AM_RANGE(0x39, 0x39) AM_MIRROR(0x06) AM_DEVWRITE_LEGACY(MC6845_TAG, mc6845_register_w)
	AM_RANGE(0x40, 0x43) AM_MIRROR(0x1c) AM_DEVREADWRITE_LEGACY(Z80SIO_TAG, z80dart_ba_cd_r, z80dart_ba_cd_w)
	AM_RANGE(0x60, 0x63) AM_MIRROR(0x1c) AM_DEVREADWRITE_LEGACY(Z80CTC_TAG, z80ctc_r, z80ctc_w)
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( abc806_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( abc806_mem, ADDRESS_SPACE_PROGRAM, 8, abc806_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0fff) AM_RAMBANK("bank1")
	AM_RANGE(0x1000, 0x1fff) AM_RAMBANK("bank2")
	AM_RANGE(0x2000, 0x2fff) AM_RAMBANK("bank3")
	AM_RANGE(0x3000, 0x3fff) AM_RAMBANK("bank4")
	AM_RANGE(0x4000, 0x4fff) AM_RAMBANK("bank5")
	AM_RANGE(0x5000, 0x5fff) AM_RAMBANK("bank6")
	AM_RANGE(0x6000, 0x6fff) AM_RAMBANK("bank7")
	AM_RANGE(0x7000, 0x7fff) AM_RAMBANK("bank8")
	AM_RANGE(0x8000, 0x8fff) AM_RAMBANK("bank9")
	AM_RANGE(0x9000, 0x9fff) AM_RAMBANK("bank10")
	AM_RANGE(0xa000, 0xafff) AM_RAMBANK("bank11")
	AM_RANGE(0xb000, 0xbfff) AM_RAMBANK("bank12")
	AM_RANGE(0xc000, 0xcfff) AM_RAMBANK("bank13")
	AM_RANGE(0xd000, 0xdfff) AM_RAMBANK("bank14")
	AM_RANGE(0xe000, 0xefff) AM_RAMBANK("bank15")
	AM_RANGE(0xf000, 0xffff) AM_RAMBANK("bank16")
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( abc806_io )
//-------------------------------------------------

static ADDRESS_MAP_START( abc806_io, ADDRESS_SPACE_IO, 8, abc806_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00, 0x00) AM_MIRROR(0xff18) AM_DEVREADWRITE_LEGACY(ABCBUS_TAG, abcbus_inp_r, abcbus_utp_w)
	AM_RANGE(0x01, 0x01) AM_MIRROR(0xff18) AM_DEVREADWRITE_LEGACY(ABCBUS_TAG, abcbus_stat_r, abcbus_cs_w)
	AM_RANGE(0x02, 0x02) AM_MIRROR(0xff18) AM_DEVWRITE_LEGACY(ABCBUS_TAG, abcbus_c1_w)
	AM_RANGE(0x03, 0x03) AM_MIRROR(0xff18) AM_DEVWRITE_LEGACY(ABCBUS_TAG, abcbus_c2_w)
	AM_RANGE(0x04, 0x04) AM_MIRROR(0xff18) AM_DEVWRITE_LEGACY(ABCBUS_TAG, abcbus_c3_w)
	AM_RANGE(0x05, 0x05) AM_MIRROR(0xff18) AM_DEVWRITE_LEGACY(ABCBUS_TAG, abcbus_c4_w)
	AM_RANGE(0x06, 0x06) AM_MIRROR(0xff18) AM_WRITE(hrs_w)
	AM_RANGE(0x07, 0x07) AM_MIRROR(0xff18) AM_DEVREAD_LEGACY(ABCBUS_TAG, abcbus_rst_r) AM_WRITE(hrc_w)
	AM_RANGE(0x20, 0x23) AM_MIRROR(0xff0c) AM_DEVREADWRITE_LEGACY(Z80DART_TAG, z80dart_ba_cd_r, z80dart_ba_cd_w)
	AM_RANGE(0x31, 0x31) AM_MIRROR(0xff06) AM_DEVREAD_LEGACY(MC6845_TAG, mc6845_register_r)
	AM_RANGE(0x34, 0x34) AM_MIRROR(0xff00) AM_MASK(0xff00) AM_READWRITE(mai_r, mao_w)
	AM_RANGE(0x35, 0x35) AM_MIRROR(0xff00) AM_READWRITE(ami_r, amo_w)
	AM_RANGE(0x36, 0x36) AM_MIRROR(0xff00) AM_READWRITE(sti_r, sto_w)
	AM_RANGE(0x37, 0x37) AM_MIRROR(0xff00) AM_MASK(0xff00) AM_READWRITE(cli_r, sso_w)
	AM_RANGE(0x38, 0x38) AM_MIRROR(0xff06) AM_DEVWRITE_LEGACY(MC6845_TAG, mc6845_address_w)
	AM_RANGE(0x39, 0x39) AM_MIRROR(0xff06) AM_DEVWRITE_LEGACY(MC6845_TAG, mc6845_register_w)
	AM_RANGE(0x40, 0x43) AM_MIRROR(0xff1c) AM_DEVREADWRITE_LEGACY(Z80SIO_TAG, z80dart_ba_cd_r, z80dart_ba_cd_w)
	AM_RANGE(0x60, 0x63) AM_MIRROR(0xff1c) AM_DEVREADWRITE_LEGACY(Z80CTC_TAG, z80ctc_r, z80ctc_w)
ADDRESS_MAP_END



//**************************************************************************
//	INPUT PORTS
//**************************************************************************

//-------------------------------------------------
//  INPUT_PORTS( abc800 )
//-------------------------------------------------

INPUT_PORTS_START( abc800 )
	PORT_START("Y0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CAPS LOCK") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Left SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED ) // 80
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED ) // 81
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) // 82
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED ) // 83

	PORT_START("Y1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad -") PORT_CODE(KEYCODE_MINUS_PAD) PORT_CHAR(UCHAR_MAMEKEY(MINUS_PAD))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad RETURN") PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHAR(UCHAR_MAMEKEY(ENTER_PAD))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) // 84
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED ) // 85

	PORT_START("Y2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED ) // 18
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 6") PORT_CODE(KEYCODE_6_PAD) PORT_CHAR(UCHAR_MAMEKEY(6_PAD))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 3") PORT_CODE(KEYCODE_3_PAD) PORT_CHAR(UCHAR_MAMEKEY(3_PAD))
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED ) // 86

	PORT_START("Y3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("4 \xC2\xA4") PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR(0x00A4)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 9") PORT_CODE(KEYCODE_9_PAD) PORT_CHAR(UCHAR_MAMEKEY(9_PAD))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 5") PORT_CODE(KEYCODE_5_PAD) PORT_CHAR(UCHAR_MAMEKEY(5_PAD))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 2") PORT_CODE(KEYCODE_2_PAD) PORT_CHAR(UCHAR_MAMEKEY(2_PAD))
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')

	PORT_START("Y4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 8") PORT_CODE(KEYCODE_8_PAD) PORT_CHAR(UCHAR_MAMEKEY(8_PAD))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 4") PORT_CODE(KEYCODE_4_PAD) PORT_CHAR(UCHAR_MAMEKEY(4_PAD))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 1") PORT_CODE(KEYCODE_1_PAD) PORT_CHAR(UCHAR_MAMEKEY(1_PAD))
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad .") PORT_CODE(KEYCODE_DEL_PAD) PORT_CHAR(UCHAR_MAMEKEY(DEL_PAD))

	PORT_START("Y5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 7") PORT_CODE(KEYCODE_7_PAD) PORT_CHAR(UCHAR_MAMEKEY(7_PAD))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PF4") PORT_CODE(KEYCODE_F4) PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PF6") PORT_CODE(KEYCODE_F6) PORT_CHAR(UCHAR_MAMEKEY(F6))
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 0") PORT_CODE(KEYCODE_0_PAD) PORT_CHAR(UCHAR_MAMEKEY(0_PAD))

	PORT_START("Y6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('/')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PF2") PORT_CODE(KEYCODE_F2) PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PF3") PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F3))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PF5") PORT_CODE(KEYCODE_F5) PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PF8") PORT_CODE(KEYCODE_F8) PORT_CHAR(UCHAR_MAMEKEY(F8))

	PORT_START("Y7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PF1") PORT_CODE(KEYCODE_F1) PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR('\r')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PF7") PORT_CODE(KEYCODE_F7) PORT_CHAR(UCHAR_MAMEKEY(F7))

	PORT_START("Y8")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR(';')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('<') PORT_CHAR('>')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xC3\xBC \xC3\x9C") PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(0x00FC) PORT_CHAR(0x00DC)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH2) PORT_CHAR('\'') PORT_CHAR('*')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_TAB) PORT_CHAR(UCHAR_MAMEKEY(TAB))

	PORT_START("Y9")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('=')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR(':')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xC3\xA9 \xC3\x89") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR(0x00E9) PORT_CHAR(0x00C9)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xC3\xA5 \xC3\x85") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR(0x00E5) PORT_CHAR(0x00C5)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xC3\xA4 \xC3\x84") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(0x00E4) PORT_CHAR(0x00C4)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Right SHIFT") PORT_CODE(KEYCODE_RSHIFT)

	PORT_START("Y10")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('+') PORT_CHAR('?')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xC3\xB6 \xC3\x96") PORT_CODE(KEYCODE_COLON) PORT_CHAR(0x00F6) PORT_CHAR(0x00D6)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED ) // 87
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED ) // 88
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) // 89
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED ) // 8a

	PORT_START("Y11")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED ) // 03
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED ) // 23
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED ) // 96
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED ) // 8b
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED ) // 8c
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED ) // 8d
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) // 8e
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED ) // 8f

	PORT_START("SB")
	PORT_DIPNAME( 0xff, 0xaa, "Serial Communications" ) PORT_DIPLOCATION("SB:1,2,3,4,5,6,7,8")
	PORT_DIPSETTING(    0xaa, "Asynchronous, Single Speed" )
	PORT_DIPSETTING(    0x2e, "Asynchronous, Split Speed" )
	PORT_DIPSETTING(    0x50, "Synchronous" )
	PORT_DIPSETTING(    0x8b, "ABC NET" )

	//PORT_INCLUDE(luxor_55_21046)
INPUT_PORTS_END


//-------------------------------------------------
//  INPUT_PORTS( abc802 )
//-------------------------------------------------

static INPUT_PORTS_START( abc802 )
	PORT_INCLUDE(fake_keyboard)

	PORT_START("SB")
	PORT_DIPNAME( 0xff, 0xaa, "Serial Communications" ) PORT_DIPLOCATION("SB:1,2,3,4,5,6,7,8")
	PORT_DIPSETTING(    0xaa, "Asynchronous, Single Speed" )
	PORT_DIPSETTING(    0x2e, "Asynchronous, Split Speed" )
	PORT_DIPSETTING(    0x50, "Synchronous" )
	PORT_DIPSETTING(    0x8b, "ABC NET" )

	PORT_START("CONFIG")
	PORT_DIPNAME( 0x01, 0x00, "Clear Screen Time Out" ) PORT_DIPLOCATION("S1:1")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) ) PORT_DIPLOCATION("S2:1")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Characters Per Line" ) PORT_DIPLOCATION("S3:1")
	PORT_DIPSETTING(    0x00, "40" )
	PORT_DIPSETTING(    0x04, "80" )
	PORT_DIPNAME( 0x08, 0x08, "Frame Frequency" )
	PORT_DIPSETTING(    0x00, "60 Hz" )
	PORT_DIPSETTING(    0x08, "50 Hz" )

	//PORT_INCLUDE(luxor_55_21046)
INPUT_PORTS_END


//-------------------------------------------------
//  INPUT_PORTS( abc806 )
//-------------------------------------------------

static INPUT_PORTS_START( abc806 )
	PORT_INCLUDE(fake_keyboard)

	PORT_START("SB")
	PORT_DIPNAME( 0xff, 0xaa, "Serial Communications" ) PORT_DIPLOCATION("SB:1,2,3,4,5,6,7,8")
	PORT_DIPSETTING(    0xaa, "Asynchronous, Single Speed" )
	PORT_DIPSETTING(    0x2e, "Asynchronous, Split Speed" )
	PORT_DIPSETTING(    0x50, "Synchronous" )
	PORT_DIPSETTING(    0x8b, "ABC NET" )

	//PORT_INCLUDE(luxor_55_21046)
INPUT_PORTS_END



//**************************************************************************
//	DEVICE CONFIGURATION
//**************************************************************************


//-------------------------------------------------
//  ABC77_INTERFACE( abc800_abc77_intf )
//-------------------------------------------------

static ABC77_INTERFACE( abc800_abc77_intf )
{
	DEVCB_NULL,
	DEVCB_DEVICE_LINE(Z80DART_TAG, z80dart_rxtxcb_w),
	DEVCB_DEVICE_LINE(Z80DART_TAG, z80dart_dcdb_w)
};


//-------------------------------------------------
//  Z80CTC_INTERFACE( ctc_intf )
//-------------------------------------------------

static TIMER_DEVICE_CALLBACK( ctc_tick )
{
	running_device *z80ctc = timer.machine->device(Z80CTC_TAG);

	z80ctc_trg0_w(z80ctc, 1);
	z80ctc_trg0_w(z80ctc, 0);

	z80ctc_trg1_w(z80ctc, 1);
	z80ctc_trg1_w(z80ctc, 0);

	z80ctc_trg2_w(z80ctc, 1);
	z80ctc_trg2_w(z80ctc, 0);
}

static WRITE_LINE_DEVICE_HANDLER( ctc_z0_w )
{
	running_device *sio = device->machine->device(Z80SIO_TAG);

	static int m_sio_rxcb, m_sio_txcb; // FIXME

	UINT8 sb = input_port_read(device->machine, "SB");

	if (BIT(sb, 2))
	{
		// connected to SIO/2 TxCA, CTC CLK/TRG3
		z80dart_txca_w(sio, state);
		z80ctc_trg3_w(device, state);
	}

	// connected to SIO/2 RxCB through a thingy
	//m_sio_rxcb = ?
	z80dart_rxcb_w(sio, m_sio_rxcb);

	// connected to SIO/2 TxCB through a JK divide by 2
	m_sio_txcb = !m_sio_txcb;
	z80dart_txcb_w(sio, m_sio_txcb);
}

static WRITE_LINE_DEVICE_HANDLER( ctc_z1_w )
{
	running_device *sio = device->machine->device(Z80SIO_TAG);

	UINT8 sb = input_port_read(device->machine, "SB");

	if (BIT(sb, 3))
	{
		// connected to SIO/2 RxCA
		z80dart_rxca_w(sio, state);
	}

	if (BIT(sb, 4))
	{
		// connected to SIO/2 TxCA, CTC CLK/TRG3
		z80dart_txca_w(sio, state);
		z80ctc_trg3_w(device, state);
	}
}

static WRITE_LINE_DEVICE_HANDLER( ctc_z2_w )
{
	running_device *m_dart = device->machine->device(Z80DART_TAG);

	// connected to DART channel A clock inputs
	z80dart_rxca_w(m_dart, state);
	z80dart_txca_w(m_dart, state);
}

static Z80CTC_INTERFACE( ctc_intf )
{
	0,              								// timer disables
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),	// interrupt handler
	DEVCB_LINE(ctc_z0_w),							// ZC/TO0 callback
	DEVCB_LINE(ctc_z1_w),							// ZC/TO1 callback
	DEVCB_LINE(ctc_z2_w)    						// ZC/TO2 callback
};


//-------------------------------------------------
//  Z80DART_INTERFACE( sio_intf )
//-------------------------------------------------

static READ_LINE_DEVICE_HANDLER( dfd_in_r )
{
	return cassette_input(device) > 0.0;
}

static WRITE_LINE_DEVICE_HANDLER( dfd_out_w )
{
	cassette_output(device, state ? +1.0 : -1.0);
}

static WRITE_LINE_DEVICE_HANDLER( motor_control_w )
{
	cassette_change_state(device, state ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);
}

static Z80DART_INTERFACE( sio_intf )
{
	0, 0, 0, 0,

	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_DEVICE_LINE(CASSETTE_TAG, dfd_in_r),
	DEVCB_DEVICE_LINE(CASSETTE_TAG, dfd_out_w),
	DEVCB_DEVICE_LINE(CASSETTE_TAG, motor_control_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0)
};


//-------------------------------------------------
//  Z80DART_INTERFACE( abc800_dart_intf )
//-------------------------------------------------

static Z80DART_INTERFACE( abc800_dart_intf )
{
	0, 0, 0, 0,

	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_DRIVER_LINE_MEMBER(abc800_state, keyboard_txd_r),
	DEVCB_DEVICE_LINE(I8048_TAG, keyboard_rxd_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0)
};


//-------------------------------------------------
//  Z80DART_INTERFACE( abc802_dart_intf )
//-------------------------------------------------

WRITE_LINE_MEMBER( abc802_state::lrs_w )
{
	m_lrs = state;

	bankswitch();
}

WRITE_LINE_MEMBER( abc802_state::mux80_40_w )
{
	m_80_40_mux = state;
}

static Z80DART_INTERFACE( abc802_dart_intf )
{
	0, 0, 0, 0,

	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_NULL, // DEVCB_DEVICE_LINE(ABC77_TAG, abc77_txd_r),
	DEVCB_NULL, // DEVCB_DEVICE_LINE(ABC77_TAG, abc77_rxd_w),
	DEVCB_DRIVER_LINE_MEMBER(abc802_state, lrs_w),
	DEVCB_DRIVER_LINE_MEMBER(abc802_state, mux80_40_w),
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0)
};


//-------------------------------------------------
//  Z80DART_INTERFACE( abc806_dart_intf )
//-------------------------------------------------

WRITE_LINE_MEMBER( abc806_state::keydtr_w )
{
	m_keydtr = state;

	bankswitch();
}

static Z80DART_INTERFACE( abc806_dart_intf )
{
	0, 0, 0, 0,

	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_NULL, // DEVCB_DEVICE_LINE(ABC77_TAG, abc77_txd_r),
	DEVCB_NULL, // DEVCB_DEVICE_LINE(ABC77_TAG, abc77_rxd_w),
	DEVCB_DRIVER_LINE_MEMBER(abc806_state, keydtr_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0)
};


//-------------------------------------------------
//  z80_daisy_config abc800_daisy_chain
//-------------------------------------------------

static const z80_daisy_config abc800_daisy_chain[] =
{
	{ Z80CTC_TAG },
	{ Z80SIO_TAG },
	{ Z80DART_TAG },
	{ NULL }
};


//-------------------------------------------------
//  cassette_config abc800_cassette_config
//-------------------------------------------------

static const cassette_config abc800_cassette_config =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED | CASSETTE_MOTOR_DISABLED | CASSETTE_SPEAKER_MUTED),
	NULL
};


//-------------------------------------------------
//  ABCBUS_DAISY( abcbus_daisy )
//-------------------------------------------------

static ABCBUS_DAISY( abcbus_daisy )
{
	{ LUXOR_55_21046_ABCBUS("luxor_55_21046") },
	{ NULL }
};



//**************************************************************************
//	MACHINE INITIALIZATION
//**************************************************************************

//-------------------------------------------------
//  machine_start
//-------------------------------------------------

void abc800_state::machine_start()
{
	// initialize values
	m_kb_txd = 1;

	// register for state saving
	state_save_register_global(machine, m_fetch_charram);
	state_save_register_global(machine, m_kb_row);
	state_save_register_global(machine, m_kb_txd);
	state_save_register_global(machine, m_kb_clk);
	state_save_register_global(machine, m_kb_stb);
	state_save_register_global(machine, m_pling);
	state_save_register_global(machine, m_sio_rxcb);
	state_save_register_global(machine, m_sio_txcb);
}


//-------------------------------------------------
//  machine_reset
//-------------------------------------------------

void abc800_state::machine_reset()
{
	m_fetch_charram = 0;
	bankswitch();

	z80dart_ria_w(m_dart, 1);

	// 50/60 Hz
	z80dart_ctsb_w(m_dart, 0); // 0 = 50Hz, 1 = 60Hz
}


//-------------------------------------------------
//  machine_start
//-------------------------------------------------

void abc802_state::machine_start()
{
	// register for state saving
	state_save_register_global(machine, m_lrs);
	state_save_register_global(machine, m_pling);
}


//-------------------------------------------------
//  machine_reset
//-------------------------------------------------

void abc802_state::machine_reset()
{
	UINT8 config = input_port_read(machine, "CONFIG");

	// memory banking
	m_lrs = 1;
	bankswitch();

	// clear screen time out (S1)
	z80dart_dcdb_w(m_sio, BIT(config, 0));

	// unknown (S2)
	z80dart_ctsb_w(m_sio, BIT(config, 1));

	// 40/80 char (S3)
	z80dart_ria_w(m_dart, BIT(config, 2)); // 0 = 40, 1 = 80

	// 50/60 Hz
	z80dart_ctsb_w(m_dart, BIT(config, 3)); // 0 = 50Hz, 1 = 60Hz
}


//-------------------------------------------------
//  machine_start
//-------------------------------------------------

void abc806_state::machine_start()
{
	UINT8 *mem = memory_region(machine, Z80_TAG);
	UINT32 videoram_size = messram_get_size(m_ram) - (32 * 1024);
	int bank;
	char bank_name[10];

	// setup memory banking
	m_video_ram = auto_alloc_array(machine, UINT8, videoram_size);

	for (bank = 1; bank <= 16; bank++)
	{
		sprintf(bank_name,"bank%d",bank);
		memory_configure_bank(machine, bank_name, 0, 1, mem + (0x1000 * (bank - 1)), 0);
		memory_configure_bank(machine, bank_name, 1, 1, m_video_ram, 0);
		memory_set_bank(machine, bank_name, 0);
	}

	// register for state saving
	state_save_register_global(machine, m_keydtr);
	state_save_register_global(machine, m_eme);
	state_save_register_global(machine, m_fetch_charram);
	state_save_register_global_array(machine, m_map);
}


//-------------------------------------------------
//  machine_reset
//-------------------------------------------------

void abc806_state::machine_reset()
{
	// setup memory banking
	int bank;
	char bank_name[10];

	for (bank = 1; bank <= 16; bank++)
	{
		sprintf(bank_name,"bank%d",bank);
		memory_set_bank(machine, bank_name, 0);
	}

	bankswitch();

	// clear STO lines
	m_eme = 0;
	m_40 = 0;
	m_hru2_a8 = 0;
	m_txoff = 0;
	e0516_cs_w(m_rtc, 1);
	e0516_clk_w(m_rtc, 1);
	e0516_dio_w(m_rtc, 1);

	z80dart_ria_w(m_dart, 1);

	// 50/60 Hz
	z80dart_ctsb_w(m_dart, 0); // 0 = 50Hz, 1 = 60Hz
}



//**************************************************************************
//	MACHINE DRIVERS
//**************************************************************************

//-------------------------------------------------
//  MACHINE_CONFIG( abc800 )
//-------------------------------------------------

static MACHINE_CONFIG_START( abc800, abc800_state )
	// basic machine hardware
	MDRV_CPU_ADD(Z80_TAG, Z80, ABC800_X01/2/2)
	MDRV_CPU_CONFIG(abc800_daisy_chain)
	MDRV_CPU_PROGRAM_MAP(abc800m_mem)
	MDRV_CPU_IO_MAP(abc800m_io)

	MDRV_CPU_ADD(I8048_TAG, I8048, XTAL_5_9904MHz)
	MDRV_CPU_IO_MAP(abc800_keyboard_io)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("discrete", DISCRETE, 0)
	MDRV_SOUND_CONFIG_DISCRETE(abc800)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)

	// peripheral hardware
	MDRV_Z80CTC_ADD(Z80CTC_TAG, ABC800_X01/2/2, ctc_intf)
	MDRV_TIMER_ADD_PERIODIC("ctc", ctc_tick, HZ(ABC800_X01/2/2/2))
	MDRV_Z80SIO2_ADD(Z80SIO_TAG, ABC800_X01/2/2, sio_intf)
	MDRV_Z80DART_ADD(Z80DART_TAG, ABC800_X01/2/2, abc800_dart_intf)
	MDRV_PRINTER_ADD("printer")
	MDRV_CASSETTE_ADD(CASSETTE_TAG, abc800_cassette_config)
	MDRV_TIMER_ADD_PERIODIC("keyboard_t1", keyboard_t1_tick, HZ(XTAL_5_9904MHz/(3*5)/20)) // TODO correct frequency?

	// ABC bus
	MDRV_ABCBUS_ADD(ABCBUS_TAG, abcbus_daisy, Z80_TAG)
	MDRV_ABC830_ADD("luxor_55_21046", ABCBUS_TAG, DRIVE_BASF_6106_08)

	// internal ram
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("16K")
	MDRV_RAM_EXTRA_OPTIONS("32K")
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( abc800m )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( abc800m, abc800 )
	// video hardware
	MDRV_FRAGMENT_ADD(abc800m_video)
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( abc800c )
//-------------------------------------------------

static MACHINE_CONFIG_DERIVED( abc800c, abc800 )
	// basic machine hardware
	MDRV_CPU_MODIFY(Z80_TAG)
	MDRV_CPU_PROGRAM_MAP(abc800c_mem)
	MDRV_CPU_IO_MAP(abc800c_io)

	// video hardware
	MDRV_FRAGMENT_ADD(abc800c_video)
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( abc802 )
//-------------------------------------------------

static MACHINE_CONFIG_START( abc802, abc802_state )
	// basic machine hardware
	MDRV_CPU_ADD(Z80_TAG, Z80, ABC800_X01/2/2)
	MDRV_CPU_CONFIG(abc800_daisy_chain)
	MDRV_CPU_PROGRAM_MAP(abc802_mem)
	MDRV_CPU_IO_MAP(abc802_io)

	// video hardware
	MDRV_FRAGMENT_ADD(abc802_video)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("discrete", DISCRETE, 0)
	MDRV_SOUND_CONFIG_DISCRETE(abc800)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)

	// peripheral hardware
	MDRV_Z80CTC_ADD(Z80CTC_TAG, ABC800_X01/2/2, ctc_intf)
	MDRV_TIMER_ADD_PERIODIC("ctc", ctc_tick, HZ(ABC800_X01/2/2/2))
	MDRV_Z80SIO2_ADD(Z80SIO_TAG, ABC800_X01/2/2, sio_intf)
	MDRV_Z80DART_ADD(Z80DART_TAG, ABC800_X01/2/2, abc802_dart_intf)
	//MDRV_ABC77_ADD(abc800_abc77_intf)
	MDRV_PRINTER_ADD("printer")
	MDRV_CASSETTE_ADD(CASSETTE_TAG, abc800_cassette_config)

	// ABC bus
	MDRV_ABCBUS_ADD(ABCBUS_TAG, abcbus_daisy, Z80_TAG)
	MDRV_ABC834_ADD("luxor_55_21046", ABCBUS_TAG)

	// fake keyboard
	MDRV_TIMER_ADD_PERIODIC("keyboard", keyboard_tick, USEC(2500))

	// internal ram
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("64K")
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( abc806 )
//-------------------------------------------------

static MACHINE_CONFIG_START( abc806, abc806_state )
	// basic machine hardware
	MDRV_CPU_ADD(Z80_TAG, Z80, ABC800_X01/2/2)
	MDRV_CPU_CONFIG(abc800_daisy_chain)
	MDRV_CPU_PROGRAM_MAP(abc806_mem)
	MDRV_CPU_IO_MAP(abc806_io)

	// video hardware
	MDRV_FRAGMENT_ADD(abc806_video)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")

	// peripheral hardware
	MDRV_E0516_ADD(E0516_TAG, ABC806_X02)
	MDRV_Z80CTC_ADD(Z80CTC_TAG, ABC800_X01/2/2, ctc_intf)
	MDRV_TIMER_ADD_PERIODIC("ctc", ctc_tick, HZ(ABC800_X01/2/2/2))
	MDRV_Z80SIO2_ADD(Z80SIO_TAG, ABC800_X01/2/2, sio_intf)
	MDRV_Z80DART_ADD(Z80DART_TAG, ABC800_X01/2/2, abc806_dart_intf)
	//MDRV_ABC77_ADD(abc800_abc77_intf)
	MDRV_PRINTER_ADD("printer")
	MDRV_CASSETTE_ADD(CASSETTE_TAG, abc800_cassette_config)

	// ABC bus
	MDRV_ABCBUS_ADD(ABCBUS_TAG, abcbus_daisy, Z80_TAG)
	MDRV_ABC832_ADD("luxor_55_21046", ABCBUS_TAG, DRIVE_TEAC_FD55F)

	// fake keyboard
	MDRV_TIMER_ADD_PERIODIC("keyboard", keyboard_tick, USEC(2500))

	// internal ram
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("160K") // 32KB + 128KB
	MDRV_RAM_EXTRA_OPTIONS("544K") // 32KB + 512KB
MACHINE_CONFIG_END



//**************************************************************************
//	ROMS
//**************************************************************************

/*

    ABC800 DOS ROMs

    Label       Drive Type
    ----------------------------
    ABC 6-1X    ABC830,DD82,DD84
    800 8"      DD88
    ABC 6-2X    ABC832
    ABC 6-3X    ABC838
    ABC 6-5X    UFD dos
    ABC 6-52    UFD dos with HD
    UFD 6.XX    Winchester


    Floppy Controllers

    Art N/O
    --------
    55 10761-01     "old" controller
    55 10828-01     "old" controller
    55 20900-0x
    55 21046-11     Luxor Conkort   25 pin D-sub connector
    55 21046-21     Luxor Conkort   34 pin FDD connector
    55 21046-41     Luxor Conkort   both of the above

*/

//-------------------------------------------------
//  ROM( abc800c )
//-------------------------------------------------

ROM_START( abc800c )
	ROM_REGION( 0x8000, Z80_TAG, 0 )
	ROM_LOAD( "abc c-1.1m", 0x0000, 0x1000, NO_DUMP )
	ROM_LOAD( "abc 1-1.1l", 0x1000, 0x1000, NO_DUMP )
	ROM_LOAD( "abc 2-1.1k", 0x2000, 0x1000, NO_DUMP )
	ROM_LOAD( "abc 3-1.1j", 0x3000, 0x1000, NO_DUMP )
	ROM_LOAD( "abc 4-1.2m", 0x4000, 0x1000, NO_DUMP )
	ROM_LOAD( "abc 5-1.2l", 0x5000, 0x1000, NO_DUMP )
	ROM_LOAD( "abc 6-1.2k", 0x6000, 0x1000, NO_DUMP )
	ROM_LOAD( "abc 7-22.2j", 0x7000, 0x1000, CRC(774511ab) SHA1(5171e43213a402c2d96dee33453c8306ac1aafc8) )

	ROM_REGION( 0x400, I8048_TAG, 0 )
	ROM_LOAD( "8048-132.z9", 0x0000, 0x0400, CRC(05c4dce5) SHA1(1824c5d304bbd09f97056cfa408e1b18b5219ba2) )

	ROM_REGION( 0x1000, "gfx1", 0 )
	ROM_LOAD( "saa5052.5c", 0x0140, 0x08c0, BAD_DUMP CRC(78c17e3e) SHA1(4e1c59dc484505de1dc0b1ba7e5f70a54b0d4ccc) ) // copied from p2000t driver

	ROM_REGION( 0x20, "hru", 0 )
	ROM_LOAD( "hru i.4g", 0x0000, 0x0020, NO_DUMP )

	ROM_REGION( 0x200, "hru2", 0 )
	ROM_LOAD( "hru ii.3a", 0x0000, 0x0200, BAD_DUMP CRC(7a19de8d) SHA1(e7cc49e749b37f7d7dd14f3feda53eae843a8fe0) )
ROM_END


//-------------------------------------------------
//  ROM( abc800m )
//-------------------------------------------------

ROM_START( abc800m )
	ROM_REGION( 0x8000, Z80_TAG, 0 )
	ROM_LOAD( "abc m-12.1m", 0x0000, 0x1000, CRC(f85b274c) SHA1(7d0f5639a528d8d8130a22fe688d3218c77839dc) )
	ROM_LOAD( "abc 1-12.1l", 0x1000, 0x1000, CRC(1e99fbdc) SHA1(ec6210686dd9d03a5ed8c4a4e30e25834aeef71d) )
	ROM_LOAD( "abc 2-12.1k", 0x2000, 0x1000, CRC(ac196ba2) SHA1(64fcc0f03fbc78e4c8056e1fa22aee12b3084ef5) )
	ROM_LOAD( "abc 3-12.1j", 0x3000, 0x1000, CRC(3ea2b5ee) SHA1(5a51ac4a34443e14112a6bae16c92b5eb636603f) )
	ROM_LOAD( "abc 4-12.2m", 0x4000, 0x1000, CRC(695cb626) SHA1(9603ce2a7b2d7b1cbeb525f5493de7e5c1e5a803) )
	ROM_LOAD( "abc 5-12.2l", 0x5000, 0x1000, CRC(b4b02358) SHA1(95338efa3b64b2a602a03bffc79f9df297e9534a) )
	ROM_LOAD_OPTIONAL( "abc 6-13.2k", 0x6000, 0x1000, CRC(6fa71fb6) SHA1(b037dfb3de7b65d244c6357cd146376d4237dab6) ) // 1982-07-19
	ROM_LOAD_OPTIONAL( "abc 6-11 ufd.2k", 0x6000, 0x1000, CRC(2a45be80) SHA1(bf08a18a74e8bdaee2656a3c8246c0122398b58f) ) // 1983-05-31, should this be "ABC 6-5" or "ABC 6-51" instead of handwritten label "ABC 6-11 UFD"?
	ROM_LOAD( "abc 6-52.2k", 0x6000, 0x1000, CRC(c311b57a) SHA1(4bd2a541314e53955a0d53ef2f9822a202daa485) ) // 1984-03-02
	ROM_LOAD_OPTIONAL( "abc 7-21.2j", 0x7000, 0x1000, CRC(fd137866) SHA1(3ac914d90db1503f61397c0ea26914eb38725044) )
	ROM_LOAD( "abc 7-22.2j", 0x7000, 0x1000, CRC(774511ab) SHA1(5171e43213a402c2d96dee33453c8306ac1aafc8) )

	ROM_REGION( 0x400, I8048_TAG, 0 )
	ROM_LOAD( "8048-132.z9", 0x0000, 0x0400, CRC(05c4dce5) SHA1(1824c5d304bbd09f97056cfa408e1b18b5219ba2) )

	ROM_REGION( 0x800, MC6845_TAG, 0 )
	ROM_LOAD( "vum se.7c", 0x0000, 0x0800, CRC(f9152163) SHA1(997313781ddcbbb7121dbf9eb5f2c6b4551fc799) )

	ROM_REGION( 0x20, "hru", 0 )
	ROM_LOAD( "hru i.4g", 0x0000, 0x0020, NO_DUMP )

	ROM_REGION( 0x200, "hru2", 0 )
	ROM_LOAD( "hru ii.3a", 0x0000, 0x0200, BAD_DUMP CRC(7a19de8d) SHA1(e7cc49e749b37f7d7dd14f3feda53eae843a8fe0) )
ROM_END


//-------------------------------------------------
//  ROM( abc802 )
//-------------------------------------------------

ROM_START( abc802 )
	ROM_REGION( 0x8000, Z80_TAG, 0 )
	ROM_LOAD(  "abc 02-11.9f",  0x0000, 0x2000, CRC(b86537b2) SHA1(4b7731ef801f9a03de0b5acd955f1e4a1828355d) )
	ROM_LOAD(  "abc 12-11.11f", 0x2000, 0x2000, CRC(3561c671) SHA1(f12a7c0fe5670ffed53c794d96eb8959c4d9f828) )
	ROM_LOAD(  "abc 22-11.12f", 0x4000, 0x2000, CRC(8dcb1cc7) SHA1(535cfd66c84c0370fd022d6edf702d3d1ad1b113) )
	ROM_SYSTEM_BIOS( 0, "v19", "UDF-DOS v6.19" )
	ROMX_LOAD( "abc 32-21.14f", 0x6000, 0x2000, CRC(57050b98) SHA1(b977e54d1426346a97c98febd8a193c3e8259574), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "v20", "UDF-DOS v6.20" )
	ROMX_LOAD( "abc 32-31.14f", 0x6000, 0x2000, CRC(fc8be7a8) SHA1(a1d4cb45cf5ae21e636dddfa70c99bfd2050ad60), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "mica", "MICA DOS v6.20" )
	ROMX_LOAD( "mica820.14f",   0x6000, 0x2000, CRC(edf998af) SHA1(daae7e1ff6ef3e0ddb83e932f324c56f4a98f79b), ROM_BIOS(3) )

	ROM_REGION( 0x2000, MC6845_TAG, 0 )
	ROM_LOAD( "abc t02-1.3g", 0x0000, 0x2000, CRC(e21601ee) SHA1(2e838ebd7692e5cb9ba4e80fe2aa47ea2584133a) ) // 64 90191-01

	ROM_REGION( 0x400, "plds", 0 )
	ROM_LOAD( "abc p2-1.2g", 0x0000, 0x0400, NO_DUMP ) // PAL16R4
ROM_END


//-------------------------------------------------
//  ROM( abc806 )
//-------------------------------------------------

ROM_START( abc806 )
	ROM_REGION( 0x10000, Z80_TAG, 0 )
	ROM_LOAD( "abc 06-11.1m",  0x0000, 0x1000, CRC(27083191) SHA1(9b45592273a5673e4952c6fe7965fc9398c49827) ) // BASIC PROM ABC 06-11 "64 90231-02"
	ROM_LOAD( "abc 16-11.1l",  0x1000, 0x1000, CRC(eb0a08fd) SHA1(f0b82089c5c8191fbc6a3ee2c78ce730c7dd5145) ) // BASIC PROM ABC 16-11 "64 90232-02"
	ROM_LOAD( "abc 26-11.1k",  0x2000, 0x1000, CRC(97a95c59) SHA1(013bc0a2661f4630c39b340965872bf607c7bd45) ) // BASIC PROM ABC 26-11 "64 90233-02"
	ROM_LOAD( "abc 36-11.1j",  0x3000, 0x1000, CRC(b50e418e) SHA1(991a59ed7796bdcfed310012b2bec50f0b8df01c) ) // BASIC PROM ABC 36-11 "64 90234-02"
	ROM_LOAD( "abc 46-11.2m",  0x4000, 0x1000, CRC(17a87c7d) SHA1(49a7c33623642b49dea3d7397af5a8b9dde8185b) ) // BASIC PROM ABC 46-11 "64 90235-02"
	ROM_LOAD( "abc 56-11.2l",  0x5000, 0x1000, CRC(b4b02358) SHA1(95338efa3b64b2a602a03bffc79f9df297e9534a) ) // BASIC PROM ABC 56-11 "64 90236-02"
	ROM_SYSTEM_BIOS( 0, "v19",		"UDF-DOS v.19" )
	ROMX_LOAD( "abc 66-21.2k", 0x6000, 0x1000, CRC(c311b57a) SHA1(4bd2a541314e53955a0d53ef2f9822a202daa485), ROM_BIOS(1) ) // DOS PROM ABC 66-21 "64 90314-01"
	ROM_SYSTEM_BIOS( 1, "v20",		"UDF-DOS v.20" )
	ROMX_LOAD( "abc 66-31.2k", 0x6000, 0x1000, CRC(a2e38260) SHA1(0dad83088222cb076648e23f50fec2fddc968883), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "mica",		"MICA DOS v.20" )
	ROMX_LOAD( "mica2006.2k",  0x6000, 0x1000, CRC(58bc2aa8) SHA1(0604bd2396f7d15fcf3d65888b4b673f554037c0), ROM_BIOS(3) )
	ROM_SYSTEM_BIOS( 3, "catnet",	"CAT-NET" )
	ROMX_LOAD( "cmd8_5.2k",	   0x6000, 0x1000, CRC(25430ef7) SHA1(03a36874c23c215a19b0be14ad2f6b3b5fb2c839), ROM_BIOS(4) )
	ROM_LOAD_OPTIONAL( "abc 76-11.2j",  0x7000, 0x1000, CRC(3eb5f6a1) SHA1(02d4e38009c71b84952eb3b8432ad32a98a7fe16) ) // Options-PROM ABC 76-11 "64 90238-02"
	ROM_LOAD( "abc 76-xx.2j",  0x7000, 0x1000, CRC(b364cc49) SHA1(9a2c373778856a31902cdbd2ae3362c200a38e24) ) // Enhanced Options-PROM

	ROM_REGION( 0x1000, MC6845_TAG, 0 )
	ROM_LOAD( "abc t6-11.7c", 0x0000, 0x1000, CRC(b17c51c5) SHA1(e466e80ec989fbd522c89a67d274b8f0bed1ff72) ) // 64 90243-01

	ROM_REGION( 0x200, "rad", 0 )
	ROM_LOAD( "64 90241-01.9b", 0x0000, 0x0200, NO_DUMP ) // "RAD" 7621/7643 (82S131/82S137), character line address

	ROM_REGION( 0x20, "hru", 0 )
	ROM_LOAD( "64 90128-01.6e", 0x0000, 0x0020, NO_DUMP ) // "HRU I" 7603 (82S123), HR horizontal timing and video memory access

	ROM_REGION( 0x200, "hru2", 0 )
	ROM_LOAD( "64 90127-01.12g", 0x0000, 0x0200, BAD_DUMP CRC(7a19de8d) SHA1(e7cc49e749b37f7d7dd14f3feda53eae843a8fe0) ) // "HRU II" 7621 (82S131), ABC800C HR compatibility mode palette

	ROM_REGION( 0x400, "v50", 0 )
	ROM_LOAD( "64 90242-01.7e", 0x0000, 0x0200, NO_DUMP ) // "V50" 7621 (82S131), HR vertical timing 50Hz
	ROM_LOAD( "64 90319-01.7e", 0x0200, 0x0200, NO_DUMP ) // "V60" 7621 (82S131), HR vertical timing 60Hz

	ROM_REGION( 0x400, "plds", 0 )
	ROM_LOAD( "64 90225-01.11c", 0x0000, 0x0400, NO_DUMP ) // "VIDEO ATTRIBUTE" 40033A (?)
	ROM_LOAD( "64 90239-01.1b",  0x0000, 0x0400, NO_DUMP ) // "ABC P3-11" PAL16R4, color encoder
	ROM_LOAD( "64 90240-01.2d",  0x0000, 0x0400, NO_DUMP ) // "ABC P4-11" PAL16L8, memory mapper
ROM_END



//**************************************************************************
//	DRIVER INITIALIZATION
//**************************************************************************

//-------------------------------------------------
//  DRIVER_INIT( abc800c )
//-------------------------------------------------

DIRECT_UPDATE_HANDLER( abc800c_direct_update_handler )
{
	abc800_state *state = machine->driver_data<abc800_state>();

	if (address >= 0x7c00 && address < 0x8000)
	{
		direct.explicit_configure(0x7c00, 0x7fff, 0x3ff, memory_region(machine, Z80_TAG) + 0x7c00);

		if (!state->m_fetch_charram)
		{
			state->m_fetch_charram = 1;
			state->bankswitch();
		}

		return ~0;
	}

	if (state->m_fetch_charram)
	{
		state->m_fetch_charram = 0;
		state->bankswitch();
	}

	return address;
}

static DRIVER_INIT( abc800c )
{
	address_space *program = machine->device<cpu_device>(Z80_TAG)->space(AS_PROGRAM);
	program->set_direct_update_handler(direct_update_delegate_create_static(abc800c_direct_update_handler, *machine));
}


//-------------------------------------------------
//  DRIVER_INIT( abc800m )
//-------------------------------------------------

DIRECT_UPDATE_HANDLER( abc800m_direct_update_handler )
{
	abc800_state *state = machine->driver_data<abc800_state>();

	if (address >= 0x7800 && address < 0x8000)
	{
		direct.explicit_configure(0x7800, 0x7fff, 0x7ff, memory_region(machine, Z80_TAG) + 0x7800);

		if (!state->m_fetch_charram)
		{
			state->m_fetch_charram = 1;
			state->bankswitch();
		}

		return ~0;
	}

	if (state->m_fetch_charram)
	{
		state->m_fetch_charram = 0;
		state->bankswitch();
	}

	return address;
}

static DRIVER_INIT( abc800m )
{
	address_space *program = machine->device<cpu_device>(Z80_TAG)->space(AS_PROGRAM);
	program->set_direct_update_handler(direct_update_delegate_create_static(abc800m_direct_update_handler, *machine));
}


//-------------------------------------------------
//  DRIVER_INIT( abc802 )
//-------------------------------------------------

DIRECT_UPDATE_HANDLER( abc802_direct_update_handler )
{
	abc802_state *state = machine->driver_data<abc802_state>();

	if (state->m_lrs)
	{
		if (address >= 0x7800 && address < 0x8000)
		{
			direct.explicit_configure(0x7800, 0x7fff, 0x7ff, memory_region(machine, Z80_TAG) + 0x7800);
			return ~0;
		}
	}

	return address;
}

static DRIVER_INIT( abc802 )
{
	address_space *program = machine->device<cpu_device>(Z80_TAG)->space(AS_PROGRAM);
	program->set_direct_update_handler(direct_update_delegate_create_static(abc802_direct_update_handler, *machine));
}


//-------------------------------------------------
//  DRIVER_INIT( abc806 )
//-------------------------------------------------

DIRECT_UPDATE_HANDLER( abc806_direct_update_handler )
{
	abc806_state *state = machine->driver_data<abc806_state>();

	if (address >= 0x7800 && address < 0x8000)
	{
		direct.explicit_configure(0x7800, 0x7fff, 0x7ff, memory_region(machine, Z80_TAG) + 0x7800);

		if (!state->m_fetch_charram)
		{
			state->m_fetch_charram = 1;
			state->bankswitch();
		}

		return ~0;
	}

	if (state->m_fetch_charram)
	{
		state->m_fetch_charram = 0;
		state->bankswitch();
	}

	return address;
}

static DRIVER_INIT( abc806 )
{
	address_space *program = machine->device<cpu_device>(Z80_TAG)->space(AS_PROGRAM);
	program->set_direct_update_handler(direct_update_delegate_create_static(abc806_direct_update_handler, *machine));
}



//**************************************************************************
//	SYSTEM DRIVERS
//**************************************************************************

//    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT   INIT     COMPANY             FULLNAME        FLAGS
COMP( 1981, abc800m,    0,			0,      abc800m,    abc800, abc800m, "Luxor Datorer AB", "ABC 800 M/HR", GAME_SUPPORTS_SAVE )
COMP( 1981, abc800c,    abc800m,    0,      abc800c,    abc800, abc800c, "Luxor Datorer AB", "ABC 800 C/HR", GAME_SUPPORTS_SAVE | GAME_NOT_WORKING )
COMP( 1983, abc802,     0,          0,      abc802,     abc802, abc802,  "Luxor Datorer AB", "ABC 802",		 GAME_SUPPORTS_SAVE )
COMP( 1983, abc806,     0,          0,      abc806,     abc806, abc806,  "Luxor Datorer AB", "ABC 806",		 GAME_SUPPORTS_SAVE | GAME_NO_SOUND )
