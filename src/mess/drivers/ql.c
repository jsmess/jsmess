#include "driver.h"
#include "deprecat.h"
#include "cpu/i8039/i8039.h"
#include "cpu/m68000/m68000.h"
#include "devices/cartslot.h"
#include "includes/serial.h"
#include "sound/speaker.h"
#include "video/zx8301.h"
#include "machine/zx8302.h"

/*

    Sinclair QL

    MESS Driver by Curt Coder


    TODO:

    - microdrive simulation
    - RTC register write
    - serial data latching?
    - transmit interrupt timing
    - interface interrupt
    - accurate screen timings
    - ZX8301 cycle stealing
    - emulate COMCTL properly
    - emulate microdrive properly
    - get modified Danish version to boot (e.g. MD_DESEL was patched to jump to 0x1cd5e)

*/

#define X1 XTAL_15MHz
#define X2 XTAL_32_768kHz
#define X3 XTAL_4_436MHz
#define X4 XTAL_11MHz

/* Intelligent Peripheral Controller (IPC) */

static int zx8302_comdata, zx8302_comctl, zx8302_baudx4;

static struct IPC
{
	UINT8 keylatch;
	int ser1_txd, ser1_dtr;
	int ser2_rxd, ser2_cts;
	int ipl;
	int comdata;
} ipc;

static WRITE8_HANDLER( ipc_link_hack_w )
{
	/*

        IPC <-> ZX8302 serial link protocol
        ***********************************

        Send bit to IPC
        ---------------

        1. ZX initiates transmission (COMDATA = 0)
        2. IPC acknowledges (COMCTL = 0, COMTL = 1)
        3. ZX transmits data bit (COMDATA = 0 or 1)
        4. IPC acknowledges (COMCTL = 0, COMTL = 1)
        5. ZX ends bit transfer (COMDATA = 1)

        Receive bit from IPC
        --------------------

        1. ZX initiates transmission (COMDATA = 0)
        2. IPC acknowledges (COMCTL = 0, COMTL = 1)
        3. IPC transmits data bit (COMDATA = 0 or 1)
        4. IPC acknowledges (COMCTL = 0, COMTL = 1)
        5. IPC ends bit transfer (COMDATA = 1)

    */

	switch (activecpu_get_pc())
	{
	case 0x759:
		// transmit data bit to IPC
		ipc.comdata = zx8302_comdata;
		break;

	case 0x75b:
		// end transmit
		zx8302_comdata = 1;
		zx8302_comctl = 0;
		break;

	case 0x775:
		// latch data bit from IPC
		zx8302_comdata = ipc.comdata;
		zx8302_comctl = 0;
		break;
	}
}

static WRITE8_HANDLER( ipc_port1_w )
{
	/*

        bit     description

        0       Keyboard column output (KBO0)
        1       Keyboard column output (KBO1)
        2       Keyboard column output (KBO2)
        3       Keyboard column output (KBO3)
        4       Keyboard column output (KBO4)
        5       Keyboard column output (KBO5)
        6       Keyboard column output (KBO6)
        7       Keyboard column output (KBO7)

    */

	ipc.keylatch = data;
}

static WRITE8_HANDLER( ipc_port2_w )
{
	/*

        bit     description

        0       Serial data input (SER2 RxD, SER1 TxD)
        1       Speaker output
        2       Interrupt output (IPL0-2)
        3       Interrupt output (IPL1)
        4       Serial Clear-to-Send output (SER2 CTS)
        5       Serial Data Terminal Ready output (SER1 DTR)
        6       not connected
        7       ZX8302 serial link input/output (COMDATA)

    */

	int ipl = (BIT(data, 2) << 1) | BIT(data, 3);

	if (ipl != ipc.ipl)
	{
		switch (ipl)
		{
		case 0:
			cpunum_set_input_line(Machine, 0, MC68000_IRQ_2, CLEAR_LINE);
			cpunum_set_input_line(Machine, 0, MC68000_IRQ_5, CLEAR_LINE);
			cpunum_set_input_line(Machine, 0, MC68000_IRQ_7, HOLD_LINE);
			break;
		case 1: // CTRL-ALT-7
			cpunum_set_input_line(Machine, 0, MC68000_IRQ_2, CLEAR_LINE);
			cpunum_set_input_line(Machine, 0, MC68000_IRQ_5, HOLD_LINE);
			cpunum_set_input_line(Machine, 0, MC68000_IRQ_7, CLEAR_LINE);
			break;
		case 2:
			cpunum_set_input_line(Machine, 0, MC68000_IRQ_2, HOLD_LINE);
			cpunum_set_input_line(Machine, 0, MC68000_IRQ_5, CLEAR_LINE);
			cpunum_set_input_line(Machine, 0, MC68000_IRQ_7, CLEAR_LINE);
			break;
		case 3:
			cpunum_set_input_line(Machine, 0, MC68000_IRQ_2, CLEAR_LINE);
			cpunum_set_input_line(Machine, 0, MC68000_IRQ_5, CLEAR_LINE);
			cpunum_set_input_line(Machine, 0, MC68000_IRQ_7, CLEAR_LINE);
			break;
		}

		ipc.ipl = ipl;
	}

	speaker_level_w(0, BIT(data, 1));

	ipc.ser2_cts = BIT(data, 4);
	ipc.ser1_dtr = BIT(data, 5);

	ipc.comdata = BIT(data, 7);
}

static READ8_HANDLER( ipc_port2_r )
{
	/*

        bit     description

        0       Serial data input (SER2 RxD, SER1 TxD)
        1       Speaker output
        2       Interrupt output (IPL0-2)
        3       Interrupt output (IPL1)
        4       Serial Clear-to-Send output (SER2 CTS)
        5       Serial Data Terminal Ready output (SER1 DTR)
        6       not connected
        7       ZX8302 serial link input/output (COMDATA)

    */

	int irq = (ipc.ser2_rxd | ipc.ser1_txd);

	cpunum_set_input_line(Machine, 1, INPUT_LINE_IRQ0, irq);

	return (ipc.comdata << 7) | irq;
}

static READ8_HANDLER( ipc_t1_r )
{
	return zx8302_baudx4;
}

static READ8_HANDLER( ipc_bus_r )
{
	/*

        bit     description

        0       Keyboard row input (KBI0)
        1       Keyboard row input (KBI1)
        2       Keyboard row input (KBI2)
        3       Keyboard row input (KBI3)
        4       Keyboard row input (KBI4)
        5       Keyboard row input (KBI5)
        6       Keyboard row input (KBI6)
        7       Keyboard row input (KBI7)

    */

	if (BIT(ipc.keylatch, 0)) return readinputportbytag("ROW0") | readinputportbytag("JOY0");
	if (BIT(ipc.keylatch, 1)) return readinputportbytag("ROW1") | readinputportbytag("JOY1");
	if (BIT(ipc.keylatch, 2)) return readinputportbytag("ROW2");
	if (BIT(ipc.keylatch, 3)) return readinputportbytag("ROW3");
	if (BIT(ipc.keylatch, 4)) return readinputportbytag("ROW4");
	if (BIT(ipc.keylatch, 5)) return readinputportbytag("ROW5");
	if (BIT(ipc.keylatch, 6)) return readinputportbytag("ROW6");
	if (BIT(ipc.keylatch, 7)) return readinputportbytag("ROW7");

	return 0;
}

static READ8_HANDLER( ipc_ea_r )
{
	// connected to ground via a 1K resistor, but needs to be 1 because the logic is reversed in i8039.c

	return 1;
}

/* Memory Maps */

static ADDRESS_MAP_START( ql_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x000000, 0x00bfff) AM_ROM	// System ROM
	AM_RANGE(0x00c000, 0x00ffff) AM_ROM // Cartridge ROM
	AM_RANGE(0x010000, 0x017fff) AM_NOP // expansion I/O
	AM_RANGE(0x018000, 0x018003) AM_READ(zx8302_rtc_r)
	AM_RANGE(0x018000, 0x018001) AM_WRITE(zx8302_rtc_w)
	AM_RANGE(0x018002, 0x018002) AM_WRITE(zx8302_control_w)
	AM_RANGE(0x018003, 0x018003) AM_WRITE(zx8302_ipc_command_w)
	AM_RANGE(0x018020, 0x018020) AM_READWRITE(zx8302_status_r, zx8302_mdv_control_w)
	AM_RANGE(0x018021, 0x018021) AM_READWRITE(zx8302_irq_status_r, zx8302_irq_acknowledge_w)
	AM_RANGE(0x018022, 0x018022) AM_READWRITE(zx8302_mdv_track_r, zx8302_data_w)
	AM_RANGE(0x018023, 0x018023) AM_READ(zx8302_mdv_track_r) AM_WRITENOP
	AM_RANGE(0x018063, 0x018063) AM_WRITE(zx8301_control_w)
	AM_RANGE(0x01c000, 0x01ffff) AM_ROM // expansion I/O
	AM_RANGE(0x020000, 0x02ffff) AM_RAM AM_BASE(&videoram)
	AM_RANGE(0x030000, 0x03ffff) AM_RAM // onboard RAM
	AM_RANGE(0x040000, 0x0bffff) AM_RAM // 512KB expansion RAM
	AM_RANGE(0x0c0000, 0x0dffff) AM_NOP // 8x16KB device slots
	AM_RANGE(0x0e0000, 0x0fffff) AM_NOP // expansion I/O
ADDRESS_MAP_END

static ADDRESS_MAP_START( ipc_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x000, 0x7ff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( ipc_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x7f) AM_WRITE(ipc_link_hack_w)
	AM_RANGE(0x27, 0x28) AM_READNOP // IPC reads these to set P0 (bus) to Hi-Z mode
	AM_RANGE(I8039_p1, I8039_p1) AM_WRITE(ipc_port1_w)
	AM_RANGE(I8039_p2, I8039_p2) AM_READWRITE(ipc_port2_r, ipc_port2_w)
	AM_RANGE(I8039_t1, I8039_t1) AM_READ(ipc_t1_r)
	AM_RANGE(I8039_bus, I8039_bus) AM_READ(ipc_bus_r)
	AM_RANGE(I8039_ea, I8039_ea) AM_READ(ipc_ea_r)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( ql )
	PORT_START_TAG("ROW0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F4) PORT_NAME("F4")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F1) PORT_NAME("F1")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F2) PORT_NAME("F2")
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F3) PORT_NAME("F3")
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F5) PORT_NAME("F5")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('&')

	PORT_START_TAG("ROW1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13) PORT_NAME("ENTER")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT)) PORT_NAME("\xe2\x86\x90")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP)) PORT_NAME("\xe2\x86\x91")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC)) PORT_NAME("ESC @") PORT_CHAR('\x1b') PORT_CHAR('@')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT)) PORT_NAME("\xe2\x86\x92")
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\') PORT_CHAR('|')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ') PORT_NAME("SPACE")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN)) PORT_NAME("\xe2\x86\x93")

	PORT_START_TAG("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_TILDE) PORT_NAME("\xc2\xa3 ~") PORT_CHAR(0xa3) PORT_CHAR('~') // ? ~
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('\'') PORT_CHAR('\"')

	PORT_START_TAG("ROW3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CAPSLOCK) PORT_NAME("CAPS LOCK")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('+')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR(':')

	PORT_START_TAG("ROW4")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')

	PORT_START_TAG("ROW5")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR('(')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_TAB) PORT_CHAR('\t') PORT_NAME("TABULATE")
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')

	PORT_START_TAG("ROW6")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('*')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('^')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR(')')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')

	PORT_START_TAG("ROW7")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1) PORT_NAME("SHIFT")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL)) PORT_NAME("CTRL")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_LALT) PORT_CODE(KEYCODE_RALT) PORT_CHAR(UCHAR_MAMEKEY(LALT)) PORT_CHAR(UCHAR_MAMEKEY(RALT)) PORT_NAME("ALT")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')

	PORT_START_TAG("JOY0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )	PORT_PLAYER(1) PORT_8WAY PORT_CODE(KEYCODE_F4)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )	PORT_PLAYER(1) PORT_8WAY PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )	PORT_PLAYER(1) PORT_8WAY PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )PORT_PLAYER(1) PORT_8WAY PORT_CODE(KEYCODE_F3)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 )		PORT_PLAYER(1) PORT_CODE(KEYCODE_F5)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("JOY1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )	PORT_PLAYER(2) PORT_8WAY PORT_CODE(KEYCODE_LEFT)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )	PORT_PLAYER(2) PORT_8WAY PORT_CODE(KEYCODE_UP)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )PORT_PLAYER(2) PORT_8WAY PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )		PORT_PLAYER(2) PORT_CODE(KEYCODE_SPACE)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )	PORT_PLAYER(2) PORT_8WAY PORT_CODE(KEYCODE_DOWN)
INPUT_PORTS_END

static INPUT_PORTS_START( ql_es )
	PORT_INCLUDE(ql)

	PORT_MODIFY("ROW1")
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_NAME("] \xc3\x9c") PORT_CHAR(']') PORT_CHAR(0xfc) PORT_CHAR(0xdc) // ] ?

	PORT_MODIFY("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('`') PORT_CHAR('^')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('!')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_TILDE) PORT_NAME("[ \xc3\x87") PORT_CHAR('[') PORT_CHAR(0xe7) PORT_CHAR(0xc7) // [ ?
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(';') PORT_CHAR(':')

	PORT_MODIFY("ROW3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('\'') PORT_CHAR('"')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_NAME("\xc3\x91") PORT_CHAR(0xf1) PORT_CHAR(0xd1) // ?

	PORT_MODIFY("ROW4")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_NAME("1 \xc2\xa1") PORT_CHAR('1') PORT_CHAR(0xa1) // 1 ?

	PORT_MODIFY("ROW6")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_NAME("2 \xc2\xbf") PORT_CHAR('2') PORT_CHAR(0xbf) // 2 ?
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('/')

	PORT_MODIFY("ROW7")
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('<') PORT_CHAR('>')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('?')
INPUT_PORTS_END

static INPUT_PORTS_START( ql_de )
	PORT_INCLUDE(ql)

	PORT_MODIFY("ROW0")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('/')

	PORT_MODIFY("ROW1")
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('<') PORT_CHAR('>')

	PORT_MODIFY("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('+') PORT_CHAR('*')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR(':')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_TILDE) PORT_CHAR('\\') PORT_CHAR('^')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_NAME("\xc3\x84") PORT_CHAR(0xe4) PORT_CHAR(0xc4) // ?

	PORT_MODIFY("ROW3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_NAME("\xc3\x9c") PORT_CHAR(0xfc) PORT_CHAR(0xdc) // ?
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('#') PORT_CHAR('\'')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_NAME("\xc3\x96") PORT_CHAR(0xf6) PORT_CHAR(0xd6) // ?

	PORT_MODIFY("ROW4")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_NAME("3 \xc2\xa3") PORT_CHAR('3') PORT_CHAR(0xa7) // 3 ?

	PORT_MODIFY("ROW5")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR('(')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_NAME("\xc3\x9f ?") PORT_CHAR(0xdf) PORT_CHAR('?') // ? ?
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')

	PORT_MODIFY("ROW6")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('=')

	PORT_MODIFY("ROW7")
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR(';')
INPUT_PORTS_END

static INPUT_PORTS_START( ql_it )
	PORT_INCLUDE(ql)

	PORT_MODIFY("ROW0")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('(') PORT_CHAR('5')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('\'') PORT_CHAR('4')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("7 \xc3\xa8") PORT_CODE(KEYCODE_7) PORT_CHAR('?') PORT_CHAR('7') // ? 7

	PORT_MODIFY("ROW1")
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('<') PORT_CHAR('>')

	PORT_MODIFY("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('$') PORT_CHAR('&')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR(':') PORT_CHAR('/')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("* \xc2\xa7") PORT_CODE(KEYCODE_TILDE) PORT_CHAR('*') PORT_CHAR(0xa7) // * ?
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('?')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\xb9 %") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(0xf9) PORT_CHAR('%') // ? %

	PORT_MODIFY("ROW3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\xac =") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR(0xec) PORT_CHAR('=') // ? =
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('-') PORT_CHAR('+')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')

	PORT_MODIFY("ROW4")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('"') PORT_CHAR('3')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('#') PORT_CHAR('1')

	PORT_MODIFY("ROW5")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\xa7 9") PORT_CODE(KEYCODE_9) PORT_CHAR(0xe7) PORT_CHAR('9') // ? 9
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR(')') PORT_CHAR('\\')

	PORT_MODIFY("ROW6")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('^') PORT_CHAR('8')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\xa9 2") PORT_CODE(KEYCODE_2) PORT_CHAR(0xe9) PORT_CHAR('2') // ? 2
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('_') PORT_CHAR('6')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\xa0 0") PORT_CODE(KEYCODE_0) PORT_CHAR(0xe0) PORT_CHAR('0') // ? 0

	PORT_MODIFY("ROW7")
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\xb2 !") PORT_CODE(KEYCODE_SLASH) PORT_CHAR(0xf2) PORT_CHAR('!') // ? !
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('.')
INPUT_PORTS_END

static INPUT_PORTS_START( ql_fr )
	PORT_INCLUDE(ql)
INPUT_PORTS_END

static INPUT_PORTS_START( ql_se )
	PORT_INCLUDE(ql)
INPUT_PORTS_END

static INPUT_PORTS_START( ql_dk )
	PORT_INCLUDE(ql)
INPUT_PORTS_END

/* Machine Start */
/*
static void zx8302_interrupt(int state)
{
	cpunum_set_input_line(Machine, 0, MC68000_IRQ_2, state);
}

static const struct zx8302_interface zx8302_intf =
{
	X1,
	X2,
	&zx8302_comdata,
	&zx8302_comctl,
	&zx8302_baudx4,
	zx8302_interrupt
};
*/
static MACHINE_START( ql )
{
	// ZX8302

//	zx8302_config(&zx8302_intf);

	// IPC

	state_save_register_global(ipc.keylatch);
	state_save_register_global(ipc.ser1_txd);
	state_save_register_global(ipc.ser1_dtr);
	state_save_register_global(ipc.ser2_rxd);
	state_save_register_global(ipc.ser2_cts);
	state_save_register_global(ipc.ipl);
	state_save_register_global(ipc.comdata);
	state_save_register_global(zx8302_comdata);
	state_save_register_global(zx8302_comctl);
	state_save_register_global(zx8302_baudx4);

	memset(&ipc, 0, sizeof(ipc));
}

/* Machine Drivers */

static MACHINE_DRIVER_START( ql )
	// basic machine hardware
	MDRV_CPU_ADD(M68008, X1/2)
	MDRV_CPU_PROGRAM_MAP(ql_map, 0)
	MDRV_CPU_VBLANK_INT(zx8302_int, 1)

	MDRV_CPU_ADD(I8749, X4)
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

	MDRV_PALETTE_INIT(zx8301)
	MDRV_VIDEO_START(zx8301)
	MDRV_VIDEO_UPDATE(zx8301)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ql_ntsc )
	MDRV_IMPORT_FROM(ql)
	MDRV_SCREEN_REFRESH_RATE(60)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( ql )
    ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_SYSTEM_BIOS( 0, "js", "v1.10 (JS)" )
	ROMX_LOAD( "ql.js 0000.ic33", 0x000000, 0x008000, CRC(1bbad3b8) SHA1(59fd4372771a630967ee102760f4652904d7d5fa), ROM_BIOS(1) )
    ROMX_LOAD( "ql.js 8000.ic34", 0x008000, 0x004000, CRC(c970800e) SHA1(b8c9203026a7de6a44bd0942ec9343e8b222cb41), ROM_BIOS(1) )

	ROM_SYSTEM_BIOS( 1, "tb", "v1.0? (TB)" )
	ROMX_LOAD( "tb.ic33", 0x000000, 0x008000, BAD_DUMP CRC(1c86d688) SHA1(7df8028e6671afc4ebd5f65bf6c2d6019181f239), ROM_BIOS(2) )
    ROMX_LOAD( "tb.ic34", 0x008000, 0x004000, BAD_DUMP CRC(de7f9669) SHA1(9d6bc0b794541a4cec2203256ae92c7e68d1011d), ROM_BIOS(2) )

	ROM_SYSTEM_BIOS( 2, "jm", "v1.03 (JM)" )
	ROMX_LOAD( "ql.jm 0000.ic33", 0x000000, 0x008000, CRC(1f8e840a) SHA1(7929e716dfe88318bbe99e34f47d039957fe3cc0), ROM_BIOS(3) )
    ROMX_LOAD( "ql.jm 8000.ic34", 0x008000, 0x004000, CRC(9168a2e9) SHA1(1e7c47a59fc40bd96dfefc2f4d86827c15f0199e), ROM_BIOS(3) )

	ROM_SYSTEM_BIOS( 3, "ah", "v1.02 (AH)" )
	ROMX_LOAD( "ah.ic33.1", 0x000000, 0x004000, BAD_DUMP CRC(a9b4d2df) SHA1(142d6f01a9621aff5e0ad678bd3cbf5cde0db801), ROM_BIOS(4) )
    ROMX_LOAD( "ah.ic33.2", 0x004000, 0x004000, BAD_DUMP CRC(36488e4e) SHA1(ff6f597b30ea03ce480a3d6728fd1d858da34d6a), ROM_BIOS(4) )
	ROMX_LOAD( "ah.ic34",   0x008000, 0x004000, BAD_DUMP CRC(61259d4c) SHA1(bdd10d111e7ba488551a27c8d3b2743917ff1307), ROM_BIOS(4) )

	ROM_SYSTEM_BIOS( 4, "pm", "v1.01 (PM)" )
	ROMX_LOAD( "pm.ic33", 0x000000, 0x008000, NO_DUMP, ROM_BIOS(5) )
    ROMX_LOAD( "pm.ic34", 0x008000, 0x004000, NO_DUMP, ROM_BIOS(5) )

	ROM_SYSTEM_BIOS( 5, "fb", "v1.00 (FB)" )
    ROMX_LOAD( "fb.ic33", 0x000000, 0x008000, NO_DUMP, ROM_BIOS(6) )
    ROMX_LOAD( "fb.ic34", 0x008000, 0x004000, NO_DUMP, ROM_BIOS(6) )

	ROM_SYSTEM_BIOS( 6, "tyche", "v2.05 (Tyche)" )
    ROMX_LOAD( "tyche.rom", 0x000000, 0x010000, BAD_DUMP CRC(8724b495) SHA1(5f33a1bc3f23fd09c31844b65bc3aca7616f180a), ROM_BIOS(7) )

	ROM_SYSTEM_BIOS( 7, "min189", "Minerva v1.89" )
    ROMX_LOAD( "minerva.rom", 0x000000, 0x00c000, CRC(930befe3) SHA1(84a99c4df13b97f90baf1ec8cb6c2e52e3e1bb4d), ROM_BIOS(8) )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "ipc8049.ic24", 0x0000, 0x0800, CRC(6a0d1f20) SHA1(fcb1c97ee7c66e5b6d8fbb57c06fd2f6509f2e1b) )
ROM_END

ROM_START( ql_us )
    ROM_REGION( 0x400000, REGION_CPU1, 0 )
    ROM_LOAD( "jsu.ic33", 0x000000, 0x008000, BAD_DUMP CRC(e397f49f) SHA1(c06f92eabaf3e6dd298c51cb7f7535d8ef0ef9c5) )
    ROM_LOAD( "jsu.ic34", 0x008000, 0x004000, BAD_DUMP CRC(3debbacc) SHA1(9fbc3e42ec463fa42f9c535d63780ff53a9313ec) )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "ipc8049.ic24", 0x0000, 0x0800, CRC(6a0d1f20) SHA1(fcb1c97ee7c66e5b6d8fbb57c06fd2f6509f2e1b) )
ROM_END

ROM_START( ql_es )
    ROM_REGION( 0x400000, REGION_CPU1, 0 )
    ROM_LOAD( "mge.ic33", 0x000000, 0x008000, BAD_DUMP CRC(d5293bde) SHA1(bf5af7e53a472d4e9871f182210787d601db0634) )
    ROM_LOAD( "mge.ic34", 0x008000, 0x004000, BAD_DUMP CRC(a694f8d7) SHA1(bd2868656008de85d7c191598588017ae8aa3339) )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "ipc8049.ic24", 0x0000, 0x0800, CRC(6a0d1f20) SHA1(fcb1c97ee7c66e5b6d8fbb57c06fd2f6509f2e1b) )
ROM_END

ROM_START( ql_fr )
    ROM_REGION( 0x400000, REGION_CPU1, 0 )
    ROM_LOAD( "mgf.ic33", 0x000000, 0x008000, NO_DUMP )
    ROM_LOAD( "mgf.ic34", 0x008000, 0x004000, NO_DUMP )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "ipc8049.ic24", 0x0000, 0x0800, CRC(6a0d1f20) SHA1(fcb1c97ee7c66e5b6d8fbb57c06fd2f6509f2e1b) )
ROM_END

ROM_START( ql_de )
    ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_SYSTEM_BIOS( 0, "mg", "v1.10 (MG)" )
    ROMX_LOAD( "mgg.ic33", 0x000000, 0x008000, BAD_DUMP CRC(b4e468fd) SHA1(cd02a3cd79af90d48b65077d0571efc2f12f146e), ROM_BIOS(1) )
    ROMX_LOAD( "mgg.ic34", 0x008000, 0x004000, BAD_DUMP CRC(54959d40) SHA1(ffc0be9649f26019d7be82925c18dc699259877f), ROM_BIOS(1) )

	ROM_SYSTEM_BIOS( 1, "mf", "v1.14 (MF)" )
    ROMX_LOAD( "mf.ic33", 0x000000, 0x008000, BAD_DUMP CRC(49c40563) SHA1(d3bcd0614cf9b52e9d7fc2832e11463e5030476b), ROM_BIOS(2) )
    ROMX_LOAD( "mf.ic34", 0x008000, 0x004000, BAD_DUMP CRC(5974616b) SHA1(c3603768c08535c25f077eed02fb80128aff13d9), ROM_BIOS(2) )

	ROM_SYSTEM_BIOS( 2, "ultramg", "Ultrasoft" )
	ROMX_LOAD( "ultramg.rom", 0x000000, 0x00c000, BAD_DUMP CRC(ad12463b) SHA1(0561b3bc7ce090f3101b2142ee957c18c250eefa), ROM_BIOS(3) )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "ipc8049.ic24", 0x0000, 0x0800, CRC(6a0d1f20) SHA1(fcb1c97ee7c66e5b6d8fbb57c06fd2f6509f2e1b) )
ROM_END

ROM_START( ql_it )
    ROM_REGION( 0x400000, REGION_CPU1, 0 )
    ROM_LOAD( "mgi.ic33", 0x000000, 0x008000, BAD_DUMP CRC(d5293bde) SHA1(bf5af7e53a472d4e9871f182210787d601db0634) )
    ROM_LOAD( "mgi.ic34", 0x008000, 0x004000, BAD_DUMP CRC(a2fdfb83) SHA1(162b1052737500f3c13497cdf0f813ba006bdae9) )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "ipc8049.ic24", 0x0000, 0x0800, CRC(6a0d1f20) SHA1(fcb1c97ee7c66e5b6d8fbb57c06fd2f6509f2e1b) )
ROM_END

ROM_START( ql_se )
    ROM_REGION( 0x400000, REGION_CPU1, 0 )
    ROM_LOAD( "mgs.ic33", 0x000000, 0x008000, NO_DUMP )
    ROM_LOAD( "mgs.ic34", 0x008000, 0x004000, NO_DUMP )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "ipc8049.ic24", 0x0000, 0x0800, CRC(6a0d1f20) SHA1(fcb1c97ee7c66e5b6d8fbb57c06fd2f6509f2e1b) )
ROM_END

ROM_START( ql_gr )
    ROM_REGION( 0x400000, REGION_CPU1, 0 )
    ROM_LOAD( "efp.ic33", 0x000000, 0x008000, BAD_DUMP CRC(eb181641) SHA1(43c1e0215cf540cbbda240b1048910ff55681059) )
    ROM_LOAD( "efp.ic34", 0x008000, 0x004000, BAD_DUMP CRC(4c3b34b7) SHA1(f9dc571d2d4f68520b306ecc7516acaeea69ec0d) )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "ipc8049.ic24", 0x0000, 0x0800, CRC(6a0d1f20) SHA1(fcb1c97ee7c66e5b6d8fbb57c06fd2f6509f2e1b) )
ROM_END

ROM_START( ql_dk )
    ROM_REGION( 0x400000, REGION_CPU1, 0 )
    ROM_LOAD( "mgd.ic33", 0x000000, 0x008000, BAD_DUMP CRC(f57755eb) SHA1(dc57939ffb8741e17967a1d2479c339750ec7ff6) )
    ROM_LOAD( "mgd.ic34", 0x008000, 0x004000, BAD_DUMP CRC(1892465a) SHA1(0ff3046b5276da6639d3fe79b22ae25cc265d540) )
	ROM_LOAD( "extra.rom", 0x01c000, 0x004000, NO_DUMP )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "ipc8049.ic24", 0x0000, 0x0800, CRC(6a0d1f20) SHA1(fcb1c97ee7c66e5b6d8fbb57c06fd2f6509f2e1b) )
ROM_END

/* System Configuration */

static DEVICE_LOAD( ql_serial )
{
	/* filename specified */
	if (serial_device_load(image)==INIT_PASS)
	{
		/* setup transmit parameters */
		serial_device_setup(image, 9600, 8, 1, SERIAL_PARITY_NONE);

		serial_device_set_protocol(image, SERIAL_PROTOCOL_NONE);

		/* and start transmit */
		serial_device_set_transmit_state(image, 1);

		return INIT_PASS;
	}

	return INIT_FAIL;
}

static void ql_serial_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* serial */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:							info->i = IO_SERIAL; break;
		case DEVINFO_INT_COUNT:							info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_INIT:							info->init = serial_device_init; break;
		case DEVINFO_PTR_LOAD:							info->load = device_load_ql_serial; break;
		case DEVINFO_PTR_UNLOAD:						info->unload = serial_device_unload; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "txt"); break;
	}
}

static DEVICE_LOAD( ql_cart )
{
	UINT8 *ptr = memory_region(REGION_CPU1) + 0x00c000;
	int	filesize = image_length(image);

	if (filesize <= 16 * 1024)
	{
		if (image_fread(image, ptr, filesize) == filesize)
		{
			return INIT_PASS;
		}
	}

	return INIT_FAIL;
}

static void ql_cartslot_getinfo( const device_class *devclass, UINT32 state, union devinfo *info )
{
	switch( state )
	{
	case DEVINFO_INT_COUNT:
		info->i = 1;
		break;
	case DEVINFO_PTR_LOAD:
		info->load = device_load_ql_cart;
		break;
	case DEVINFO_STR_FILE_EXTENSIONS:
		strcpy(info->s = device_temp_str(), "bin");
		break;
	default:
		cartslot_device_getinfo( devclass, state, info );
		break;
	}
}

SYSTEM_CONFIG_START( ql )
	CONFIG_RAM_DEFAULT	(128 * 1024)
	CONFIG_RAM			(640 * 1024)
	CONFIG_DEVICE(zx8302_microdrive_getinfo)
	CONFIG_DEVICE(ql_serial_getinfo)
	CONFIG_DEVICE(ql_cartslot_getinfo)
SYSTEM_CONFIG_END

/* Computer Drivers */

/*    YEAR  NAME    PARENT  COMPAT  MACHINE     INPUT   INIT    CONFIG  COMPANY                     FULLNAME        FLAGS */
COMP( 1984, ql,     0,      0,      ql,         ql,     0,      ql,     "Sinclair Research Ltd",    "QL (UK)",      GAME_SUPPORTS_SAVE )
COMP( 1985, ql_us,  ql,     0,      ql_ntsc,    ql,     0,      ql,     "Sinclair Research Ltd",    "QL (USA)",     GAME_SUPPORTS_SAVE )
COMP( 1985, ql_es,  ql,     0,      ql,         ql_es,  0,      ql,     "Sinclair Research Ltd",    "QL (Spain)",   GAME_SUPPORTS_SAVE )
COMP( 1985, ql_fr,  ql,     0,      ql,         ql_fr,  0,      ql,     "Sinclair Research Ltd",    "QL (France)",  GAME_SUPPORTS_SAVE )
COMP( 1985, ql_de,  ql,     0,      ql,         ql_de,  0,      ql,     "Sinclair Research Ltd",    "QL (Germany)", GAME_SUPPORTS_SAVE )
COMP( 1985, ql_it,  ql,     0,      ql,         ql_it,  0,      ql,     "Sinclair Research Ltd",    "QL (Italy)",   GAME_SUPPORTS_SAVE )
COMP( 1985, ql_se,  ql,     0,      ql,         ql_se,  0,      ql,     "Sinclair Research Ltd",    "QL (Sweden)",  GAME_SUPPORTS_SAVE )
COMP( 1985, ql_dk,  ql,     0,      ql,         ql_dk,  0,      ql,     "Sinclair Research Ltd",    "QL (Denmark)", GAME_NOT_WORKING )
COMP( 1985, ql_gr,  ql,     0,      ql,         ql,     0,      ql,     "Sinclair Research Ltd",    "QL (Greece)",  GAME_SUPPORTS_SAVE )
