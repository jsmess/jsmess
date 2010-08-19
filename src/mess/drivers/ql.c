/*

    Sinclair QL

    MESS Driver by Curt Coder

*/

/*

    TODO:

    - Tyche bios is broken
    - several disk interfaces (720K/1.44MB/3.2MB)
    - discrete sound
    - Gold Card (68000 @ 16MHz, 2MB RAM)
    - Super Gold Card (68020 @ 24MHz, 4MB RAM)
    - QLToolkit II ROM
    - GST 68kOS ROM
    - natural keyboard SHIFT does not work, it causes characters to be skipped altogether
    - get modified Danish version to boot (e.g. MD_DESEL was patched to jump to 0x1cd5e)
    - ICL One Per Desk / British Telecom Merlin Tonto / Telecom Australia Computerphone / Mega OPD (US)
    - OPD: 68008P8 @ 7.5MHz, ZX8301 @ 12MHz, CDP65C51E1 @ 1.8432MHz, 128K RAM, 2K NVRAM, 128K ROM (4x32), 16K speech ROM, ADM7910 modem, TMS5220 speech, 2x100K microdrives, RTC 1/1/1970, 1xRS-423
    - Sandy Q-XT 640 (original motherboard, Super QBoard 512KB, Centronics, Super Toolkit II, FDC, 1 or 2 3.5" 1MB drives, 3 expansion slots)
    - CST Thor PC 1F (720K 3.5")
    - CST Thor PC 2F (2x 720k 3.5")
    - CST Thor PC 2FW (20MB Winchester + 720K 3.5")
    - CST Thor 20 (68020)
    - CST Thor 21 (68020+68881)
    - CST Thor XVI CF    (Workstation, 68000 @ 8 MHz, M6802, 68682 DUART)
    - CST Thor XVI IF    (Single Floppy)
    - CST Thor XVI FF    (Dual Floppy)
    - CST Thor XVI W20F  (20MB Winchester, 1 Floppy)
    - CST Thor XVI W20FF (20MB Winchester, 2 Floppies)
    - CST Thor XVI W40F  (40MB Winchester, 1 Floppy)
    - CST Thor XVI W40FF (40MB Winchester, 2 Floppies)

*/

#include "emu.h"
#include "cpu/mcs51/mcs51.h"
#include "includes/ql.h"
#include "cpu/mcs48/mcs48.h"
#include "cpu/m68000/m68000.h"
#include "devices/flopdrv.h"
#include "formats/basicdsk.h"
#include "devices/cartslot.h"
#include "devices/microdrv.h"
#include "devices/snapquik.h"
#include "sound/speaker.h"
#include "video/zx8301.h"
#include "machine/zx8302.h"
#include "devices/messram.h"

static QUICKLOAD_LOAD( ql );

/* Intelligent Peripheral Controller (IPC) */

static WRITE8_HANDLER( ipc_w )
{
	ql_state *state = space->machine->driver_data<ql_state>();

	// pulse COMCTL line

	zx8302_comctl_w(state->zx8302, 0);
	zx8302_comctl_w(state->zx8302, 1);
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

	ql_state *state = space->machine->driver_data<ql_state>();

	state->keylatch = data;
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

	running_device *speaker = space->machine->device("speaker");
	ql_state *state = space->machine->driver_data<ql_state>();

	int ipl = (BIT(data, 2) << 1) | BIT(data, 3);

	if (ipl != state->ipl)
	{
		switch (ipl)
		{
		case 0:
			cputag_set_input_line(space->machine, M68008_TAG, M68K_IRQ_7, ASSERT_LINE);
			break;

		case 1:
			// CTRL-ALT-7 pressed
			cputag_set_input_line(space->machine, M68008_TAG, M68K_IRQ_5, ASSERT_LINE);
			break;

		case 2:
			cputag_set_input_line(space->machine, M68008_TAG, M68K_IRQ_2, ASSERT_LINE);
			break;

		case 3:
			cputag_set_input_line(space->machine, M68008_TAG, M68K_IRQ_7, CLEAR_LINE);
			break;
		}

		state->ipl = ipl;
	}

	speaker_level_w(speaker, BIT(data, 1));

	state->ser2_cts = BIT(data, 4);
	state->ser1_dtr = BIT(data, 5);

	state->comdata = BIT(data, 7);

	zx8302_comdata_w(state->zx8302, BIT(data, 7));
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

	ql_state *state = space->machine->driver_data<ql_state>();

	int irq = (state->ser2_rxd | state->ser1_txd);

	cputag_set_input_line(space->machine, I8749_TAG, INPUT_LINE_IRQ0, irq);

	return (state->comdata << 7) | irq;
}

static READ8_HANDLER( ipc_t1_r )
{
	ql_state *state = space->machine->driver_data<ql_state>();

	return state->baudx4;
}

static READ8_HANDLER( ipc_bus_r )
{
	ql_state *state = space->machine->driver_data<ql_state>();

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

	UINT8 data = 0;

	if (BIT(state->keylatch, 0)) data |= input_port_read(space->machine, "ROW0") | input_port_read(space->machine, "JOY0");
	if (BIT(state->keylatch, 1)) data |= input_port_read(space->machine, "ROW1") | input_port_read(space->machine, "JOY1");
	if (BIT(state->keylatch, 2)) data |= input_port_read(space->machine, "ROW2");
	if (BIT(state->keylatch, 3)) data |= input_port_read(space->machine, "ROW3");
	if (BIT(state->keylatch, 4)) data |= input_port_read(space->machine, "ROW4");
	if (BIT(state->keylatch, 5)) data |= input_port_read(space->machine, "ROW5");
	if (BIT(state->keylatch, 6)) data |= input_port_read(space->machine, "ROW6");
	if (BIT(state->keylatch, 7)) data |= input_port_read(space->machine, "ROW7");

	return data;
}

/* Memory Maps */

static ADDRESS_MAP_START( ql_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x000000, 0x00bfff) AM_ROM	// 48K System ROM
	AM_RANGE(0x00c000, 0x00ffff) AM_ROMBANK("bank1") // 16K Cartridge ROM
	AM_RANGE(0x010000, 0x017fff) AM_UNMAP // 32K Expansion I/O
	AM_RANGE(0x018000, 0x018003) AM_DEVREAD(ZX8302_TAG, zx8302_rtc_r)
	AM_RANGE(0x018000, 0x018001) AM_DEVWRITE(ZX8302_TAG, zx8302_rtc_w)
	AM_RANGE(0x018002, 0x018002) AM_DEVWRITE(ZX8302_TAG, zx8302_control_w)
	AM_RANGE(0x018003, 0x018003) AM_DEVWRITE(ZX8302_TAG, zx8302_ipc_command_w)
	AM_RANGE(0x018020, 0x018020) AM_DEVREADWRITE(ZX8302_TAG, zx8302_status_r, zx8302_mdv_control_w)
	AM_RANGE(0x018021, 0x018021) AM_DEVREADWRITE(ZX8302_TAG, zx8302_irq_status_r, zx8302_irq_acknowledge_w)
	AM_RANGE(0x018022, 0x018022) AM_DEVREADWRITE(ZX8302_TAG, zx8302_mdv_track_r, zx8302_data_w)
	AM_RANGE(0x018023, 0x018023) AM_DEVREAD(ZX8302_TAG, zx8302_mdv_track_r) AM_WRITENOP
	AM_RANGE(0x018063, 0x018063) AM_DEVWRITE(ZX8301_TAG, zx8301_control_w)
	AM_RANGE(0x01c000, 0x01ffff) AM_UNMAP // 16K Expansion I/O
	AM_RANGE(0x020000, 0x03ffff) AM_DEVREADWRITE(ZX8301_TAG, zx8301_ram_r, zx8301_ram_w)
	AM_RANGE(0x040000, 0x0fffff) AM_RAMBANK("bank2")
ADDRESS_MAP_END

static ADDRESS_MAP_START( ipc_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x7f) AM_WRITE(ipc_w)
	AM_RANGE(0x27, 0x28) AM_READNOP // IPC reads these to set P0 (bus) to Hi-Z mode
	AM_RANGE(MCS48_PORT_P1, MCS48_PORT_P1) AM_WRITE(ipc_port1_w)
	AM_RANGE(MCS48_PORT_P2, MCS48_PORT_P2) AM_READWRITE(ipc_port2_r, ipc_port2_w)
	AM_RANGE(MCS48_PORT_T1, MCS48_PORT_T1) AM_READ(ipc_t1_r)
	AM_RANGE(MCS48_PORT_BUS, MCS48_PORT_BUS) AM_READ(ipc_bus_r) AM_WRITENOP
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( ql )
	PORT_START("ROW0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F4) PORT_CHAR(UCHAR_MAMEKEY(F4)) PORT_NAME("F4")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F1) PORT_CHAR(UCHAR_MAMEKEY(F1)) PORT_NAME("F1")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F2) PORT_CHAR(UCHAR_MAMEKEY(F2)) PORT_NAME("F2")
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F3)) PORT_NAME("F3")
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F5) PORT_CHAR(UCHAR_MAMEKEY(F5)) PORT_NAME("F5")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('&')

	PORT_START("ROW1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xe2\x86\x90") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xe2\x86\x91") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("ESC @") PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC)) PORT_CHAR('\x1b') PORT_CHAR('@')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xe2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\') PORT_CHAR('|')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xe2\x86\x93") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))

	PORT_START("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_TILDE) PORT_NAME("\xc2\xa3 ~") PORT_CHAR(0xa3) PORT_CHAR('~') // ? ~
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('\'') PORT_CHAR('\"')

	PORT_START("ROW3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("CAPS LOCK") PORT_CODE(KEYCODE_CAPSLOCK)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('+')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR(':')

	PORT_START("ROW4")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')

	PORT_START("ROW5")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR('(')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("TABULATE") PORT_CODE(KEYCODE_TAB) PORT_CHAR('\t')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')

	PORT_START("ROW6")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('*')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('^')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR(')')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')

	PORT_START("ROW7")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("ALT") PORT_CODE(KEYCODE_LALT) PORT_CHAR(UCHAR_MAMEKEY(LALT)) PORT_CODE(KEYCODE_RALT)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')

	PORT_START("JOY0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )	PORT_PLAYER(1) PORT_8WAY PORT_CODE(KEYCODE_F4)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )	PORT_PLAYER(1) PORT_8WAY PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )	PORT_PLAYER(1) PORT_8WAY PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )PORT_PLAYER(1) PORT_8WAY PORT_CODE(KEYCODE_F3)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 )		PORT_PLAYER(1) PORT_CODE(KEYCODE_F5)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("JOY1")
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
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_NAME("] \xc3\x9c") PORT_CHAR(']') PORT_CHAR(0xfc) PORT_CHAR(0xdc)

	PORT_MODIFY("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('`') PORT_CHAR('^')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('!')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("[ \xc3\x87") PORT_CODE(KEYCODE_TILDE) PORT_CHAR('[') PORT_CHAR(0xe7) PORT_CHAR(0xc7)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(';') PORT_CHAR(':')

	PORT_MODIFY("ROW3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('\'') PORT_CHAR('"')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\x91") PORT_CODE(KEYCODE_COLON) PORT_CHAR(0xf1) PORT_CHAR(0xd1)

	PORT_MODIFY("ROW4")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("1 \xc2\xa1") PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR(0xa1)

	PORT_MODIFY("ROW6")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("2 \xc2\xbf") PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR(0xbf)
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
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR(':')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_TILDE) PORT_CHAR('\\') PORT_CHAR('^')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\x84") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(0xe4) PORT_CHAR(0xc4)

	PORT_MODIFY("ROW3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\x9c") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR(0xfc) PORT_CHAR(0xdc)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('#') PORT_CHAR('\'')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\x96") PORT_CODE(KEYCODE_COLON) PORT_CHAR(0xf6) PORT_CHAR(0xd6)

	PORT_MODIFY("ROW4")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("3 \xc2\xa3") PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR(0xa7)

	PORT_MODIFY("ROW5")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR('(')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\x9f ?") PORT_CODE(KEYCODE_MINUS) PORT_CHAR(0xdf) PORT_CHAR('?')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')

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
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("7 \xc3\xa8") PORT_CODE(KEYCODE_7) PORT_CHAR('?') PORT_CHAR('7')

	PORT_MODIFY("ROW1")
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('<') PORT_CHAR('>')

	PORT_MODIFY("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('$') PORT_CHAR('&')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR(':') PORT_CHAR('/')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("* \xc2\xa7") PORT_CODE(KEYCODE_TILDE) PORT_CHAR('*') PORT_CHAR(0xa7)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('?')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\xb9 %") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(0xf9) PORT_CHAR('%')

	PORT_MODIFY("ROW3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\xac =") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR(0xec) PORT_CHAR('=')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('-') PORT_CHAR('+')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')

	PORT_MODIFY("ROW4")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('"') PORT_CHAR('3')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('#') PORT_CHAR('1')

	PORT_MODIFY("ROW5")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\xa7 9") PORT_CODE(KEYCODE_9) PORT_CHAR(0xe7) PORT_CHAR('9')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR(')') PORT_CHAR('\\')

	PORT_MODIFY("ROW6")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('^') PORT_CHAR('8')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\xa9 2") PORT_CODE(KEYCODE_2) PORT_CHAR(0xe9) PORT_CHAR('2')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('_') PORT_CHAR('6')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\xa0 0") PORT_CODE(KEYCODE_0) PORT_CHAR(0xe0) PORT_CHAR('0')

	PORT_MODIFY("ROW7")
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc3\xb2 !") PORT_CODE(KEYCODE_SLASH) PORT_CHAR(0xf2) PORT_CHAR('!')
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

/* ZX8301 Interface */

static READ8_HANDLER( ql_ram_r )
{
	return messram_get_ptr(space->machine->device("messram"))[offset];
}

static WRITE8_HANDLER( ql_ram_w )
{
	messram_get_ptr(space->machine->device("messram"))[offset] = data;
}

static ZX8301_INTERFACE( ql_zx8301_intf )
{
	M68008_TAG,
	SCREEN_TAG,
	DEVCB_MEMORY_HANDLER(M68008_TAG, PROGRAM, ql_ram_r),
	DEVCB_MEMORY_HANDLER(M68008_TAG, PROGRAM, ql_ram_w),
	DEVCB_DEVICE_LINE(ZX8302_TAG, zx8302_vsync_w)
};

/* ZX8302 Interface */

static WRITE_LINE_DEVICE_HANDLER( ql_baudx4_w )
{
	ql_state *driver_state = device->machine->driver_data<ql_state>();

	driver_state->baudx4 = state;
}

static WRITE_LINE_DEVICE_HANDLER( ql_comdata_w )
{
	ql_state *driver_state = device->machine->driver_data<ql_state>();

	driver_state->comdata = state;
}

static ZX8302_INTERFACE( ql_zx8302_intf )
{
	X2,
	MDV1_TAG,
	MDV2_TAG,
	DEVCB_CPU_INPUT_LINE(M68008_TAG, M68K_IRQ_2),
	DEVCB_LINE(ql_baudx4_w),
	DEVCB_LINE(ql_comdata_w)
};

static VIDEO_UPDATE( ql )
{
	ql_state *state = screen->machine->driver_data<ql_state>();

	zx8301_update(state->zx8301, bitmap, cliprect);

	return 0;
}

/* Machine Drivers */

static MACHINE_START( ql )
{
	ql_state *state = machine->driver_data<ql_state>();

	address_space *program = cputag_get_address_space(machine, M68008_TAG, ADDRESS_SPACE_PROGRAM);

	/* configure ROM cartridge */

	memory_unmap_readwrite(program, 0x00c000, 0x00ffff, 0, 0);
	memory_configure_bank(machine, "bank1", 0, 1, memory_region(machine, M68008_TAG) + 0x00c000, 0);
	memory_set_bank(machine, "bank1", 0);

	/* configure RAM */

	switch (messram_get_size(machine->device("messram")))
	{
	case 128*1024:
		memory_unmap_readwrite(program, 0x040000, 0x0fffff, 0, 0);
		break;

	case 192*1024:
		memory_install_readwrite_bank(program, 0x040000, 0x04ffff, 0, 0, "bank2");
		memory_unmap_readwrite(program, 0x050000, 0x0fffff, 0, 0);
		break;

	case 256*1024:
		memory_install_readwrite_bank(program, 0x040000, 0x05ffff, 0, 0, "bank2");
		memory_unmap_readwrite(program, 0x060000, 0x0fffff, 0, 0);
		break;

	case 384*1024:
		memory_install_readwrite_bank(program, 0x040000, 0x07ffff, 0, 0, "bank2");
		memory_unmap_readwrite(program, 0x080000, 0x0fffff, 0, 0);
		break;

	case 640*1024:
		memory_install_readwrite_bank(program, 0x040000, 0x0bffff, 0, 0, "bank2");
		memory_unmap_readwrite(program, 0x0c0000, 0x0fffff, 0, 0);
		break;

	case 896*1024:
		memory_install_readwrite_bank(program, 0x040000, 0x0fffff, 0, 0, "bank2");
		break;
	}

	memory_configure_bank(machine, "bank2", 0, 1, memory_region(machine, M68008_TAG) + 0x050000, 0);
	memory_set_bank(machine, "bank2", 0);

	// find devices

	state->zx8301 = machine->device(ZX8301_TAG);
	state->zx8302 = machine->device(ZX8302_TAG);

	// register for state saving

	state_save_register_global(machine, state->keylatch);
	state_save_register_global(machine, state->ser1_txd);
	state_save_register_global(machine, state->ser1_dtr);
	state_save_register_global(machine, state->ser2_rxd);
	state_save_register_global(machine, state->ser2_cts);
	state_save_register_global(machine, state->ipl);
	state_save_register_global(machine, state->comdata);
	state_save_register_global(machine, state->comctl);
	state_save_register_global(machine, state->baudx4);
}

static DEVICE_IMAGE_LOAD( ql_cart )
{
//  UINT8 *ptr = memory_region(image.device().machine, M68008_TAG) + 0x00c000;
//  int filesize = image.length();
//
//  if (filesize <= 16 * 1024)
//  {
//      if (image.fread( ptr, filesize) == filesize)
//      {
//          memory_install_read_bank(cputag_get_address_space(image.device().machine, M68008_TAG, ADDRESS_SPACE_PROGRAM), 0x00c000, 0x00ffff, 0, 0, "bank1");
//          memory_unmap_write(cputag_get_address_space(image.device().machine, M68008_TAG, ADDRESS_SPACE_PROGRAM), 0x00c000, 0x00ffff, 0, 0);
//
//          return IMAGE_INIT_PASS;
//      }
//  }
//
	return IMAGE_INIT_FAIL;
}


static FLOPPY_OPTIONS_START(ql)
	FLOPPY_OPTION(ql, "dsk", "SSSD disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([40])
		SECTORS([9])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))
	FLOPPY_OPTION(ql, "dsk", "DSSD disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([40])
		SECTORS([9])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))
	FLOPPY_OPTION(ql, "dsk", "DSDD disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([80])
		SECTORS([9])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))
	FLOPPY_OPTION(ql, "dsk", "DSHD disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([80])
		SECTORS([18])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))
	FLOPPY_OPTION(ql, "dsk", "DSED disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([80])
		SECTORS([40])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

static const floppy_config ql_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	FLOPPY_OPTIONS_NAME(ql),
	NULL
};

static MACHINE_DRIVER_START( ql )
	MDRV_DRIVER_DATA(ql_state)

	// basic machine hardware
	MDRV_CPU_ADD(M68008_TAG, M68008, X1/2)
	MDRV_CPU_PROGRAM_MAP(ql_map)

	MDRV_CPU_ADD(I8749_TAG, I8749, X4)
	MDRV_CPU_IO_MAP(ipc_io_map)

	MDRV_MACHINE_START(ql)

	MDRV_ZX8302_ADD(ZX8302_TAG, X1, ql_zx8302_intf)

	MDRV_DEVICE_ADD(MDV1_TAG, MICRODRIVE, 0)
	MDRV_DEVICE_ADD(MDV2_TAG, MICRODRIVE, 0)

	// video hardware

	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_REFRESH_RATE(50.08)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(960, 312)
	MDRV_SCREEN_VISIBLE_AREA(0, 512-1, 0, 256-1)

	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(zx8301)
	MDRV_VIDEO_UPDATE(ql)

	MDRV_ZX8301_ADD(ZX8301_TAG, X1, ql_zx8301_intf)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* quickload */
	MDRV_QUICKLOAD_ADD("quickload", ql, "bas", 0)

	/* cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("bin")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_LOAD(ql_cart)

	MDRV_FLOPPY_2_DRIVES_ADD(ql_floppy_config)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("128K")
	MDRV_RAM_EXTRA_OPTIONS("192K,256K,384K,640K,896K")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ql_ntsc )
	MDRV_IMPORT_FROM(ql)

	MDRV_SCREEN_MODIFY(SCREEN_TAG)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_SIZE(960, 262)
	MDRV_SCREEN_VISIBLE_AREA(0, 512-1, 0, 256-1)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( opd )
	MDRV_IMPORT_FROM(ql)

	/* internal ram */
	MDRV_RAM_MODIFY("messram")
	MDRV_RAM_DEFAULT_SIZE("128K")
	MDRV_RAM_EXTRA_OPTIONS("256K")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( megaopd )
	MDRV_IMPORT_FROM(ql)

	/* internal ram */
	MDRV_RAM_MODIFY("messram")
	MDRV_RAM_DEFAULT_SIZE("256K")
MACHINE_DRIVER_END

/* ROMs */

#define ROM_IPC	\
	ROM_REGION( 0x800, I8749_TAG, 0 )\
	ROM_LOAD( "ipc8049.ic24", 0x0000, 0x0800, CRC(6a0d1f20) SHA1(fcb1c97ee7c66e5b6d8fbb57c06fd2f6509f2e1b) )

ROM_START( ql )
    ROM_REGION( 0x400000, M68008_TAG, 0 )
	ROM_SYSTEM_BIOS( 0, "default", "v1.10 (JS)" )
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
    ROMX_LOAD( "tyche.rom", 0x000000, 0x010000, CRC(8724b495) SHA1(5f33a1bc3f23fd09c31844b65bc3aca7616f180a), ROM_BIOS(7) )

	ROM_SYSTEM_BIOS( 7, "min189", "Minerva v1.89" )
    ROMX_LOAD( "minerva.rom", 0x000000, 0x00c000, BAD_DUMP CRC(930befe3) SHA1(84a99c4df13b97f90baf1ec8cb6c2e52e3e1bb4d), ROM_BIOS(8) )

	ROM_IPC
	/*
    ROM_REGION( 0x400, "plds", 0 )
    ROM_LOAD( "hal16l8.ic38", 0x0000, 0x0400, NO_DUMP )
    */
ROM_END

ROM_START( ql_us )
    ROM_REGION( 0x400000, M68008_TAG, 0 )
    ROM_LOAD( "jsu.ic33", 0x000000, 0x008000, BAD_DUMP CRC(e397f49f) SHA1(c06f92eabaf3e6dd298c51cb7f7535d8ef0ef9c5) )
    ROM_LOAD( "jsu.ic34", 0x008000, 0x004000, BAD_DUMP CRC(3debbacc) SHA1(9fbc3e42ec463fa42f9c535d63780ff53a9313ec) )

	ROM_IPC
ROM_END

ROM_START( ql_es )
    ROM_REGION( 0x400000, M68008_TAG, 0 )
    ROM_LOAD( "mge.ic33", 0x000000, 0x008000, BAD_DUMP CRC(d5293bde) SHA1(bf5af7e53a472d4e9871f182210787d601db0634) )
    ROM_LOAD( "mge.ic34", 0x008000, 0x004000, BAD_DUMP CRC(a694f8d7) SHA1(bd2868656008de85d7c191598588017ae8aa3339) )

	ROM_IPC
ROM_END

ROM_START( ql_fr )
    ROM_REGION( 0x400000, M68008_TAG, 0 )
    ROM_LOAD( "mgf.ic33", 0x000000, 0x008000, NO_DUMP )
    ROM_LOAD( "mgf.ic34", 0x008000, 0x004000, NO_DUMP )

	ROM_IPC
ROM_END

ROM_START( ql_de )
    ROM_REGION( 0x400000, M68008_TAG, 0 )
	ROM_SYSTEM_BIOS( 0, "default", "v1.10 (MG)" )
    ROMX_LOAD( "mgg.ic33", 0x000000, 0x008000, BAD_DUMP CRC(b4e468fd) SHA1(cd02a3cd79af90d48b65077d0571efc2f12f146e), ROM_BIOS(1) )
    ROMX_LOAD( "mgg.ic34", 0x008000, 0x004000, BAD_DUMP CRC(54959d40) SHA1(ffc0be9649f26019d7be82925c18dc699259877f), ROM_BIOS(1) )

	ROM_SYSTEM_BIOS( 1, "mf", "v1.14 (MF)" )
    ROMX_LOAD( "mf.ic33", 0x000000, 0x008000, BAD_DUMP CRC(49c40563) SHA1(d3bcd0614cf9b52e9d7fc2832e11463e5030476b), ROM_BIOS(2) )
    ROMX_LOAD( "mf.ic34", 0x008000, 0x004000, BAD_DUMP CRC(5974616b) SHA1(c3603768c08535c25f077eed02fb80128aff13d9), ROM_BIOS(2) )

	ROM_SYSTEM_BIOS( 2, "ultramg", "Ultrasoft" )
	ROMX_LOAD( "ultramg.rom", 0x000000, 0x00c000, BAD_DUMP CRC(ad12463b) SHA1(0561b3bc7ce090f3101b2142ee957c18c250eefa), ROM_BIOS(3) )

	ROM_IPC
ROM_END

ROM_START( ql_it )
    ROM_REGION( 0x400000, M68008_TAG, 0 )
    ROM_LOAD( "mgi.ic33", 0x000000, 0x008000, BAD_DUMP CRC(d5293bde) SHA1(bf5af7e53a472d4e9871f182210787d601db0634) )
    ROM_LOAD( "mgi.ic34", 0x008000, 0x004000, BAD_DUMP CRC(a2fdfb83) SHA1(162b1052737500f3c13497cdf0f813ba006bdae9) )

	ROM_IPC
ROM_END

ROM_START( ql_se )
    ROM_REGION( 0x400000, M68008_TAG, 0 )
    ROM_LOAD( "mgs.ic33", 0x000000, 0x008000, NO_DUMP )
    ROM_LOAD( "mgs.ic34", 0x008000, 0x004000, NO_DUMP )

	ROM_IPC
ROM_END

ROM_START( ql_gr )
    ROM_REGION( 0x400000, M68008_TAG, 0 )
    ROM_LOAD( "efp.ic33", 0x000000, 0x008000, BAD_DUMP CRC(eb181641) SHA1(43c1e0215cf540cbbda240b1048910ff55681059) )
    ROM_LOAD( "efp.ic34", 0x008000, 0x004000, BAD_DUMP CRC(4c3b34b7) SHA1(f9dc571d2d4f68520b306ecc7516acaeea69ec0d) )

	ROM_IPC
ROM_END

ROM_START( ql_dk )
    ROM_REGION( 0x400000, M68008_TAG, 0 )
    ROM_LOAD( "mgd.ic33",  0x000000, 0x008000, BAD_DUMP CRC(f57755eb) SHA1(dc57939ffb8741e17967a1d2479c339750ec7ff6) )
    ROM_LOAD( "mgd.ic34",  0x008000, 0x004000, BAD_DUMP CRC(1892465a) SHA1(0ff3046b5276da6639d3fe79b22ae25cc265d540) )
	ROM_LOAD( "extra.rom", 0x01c000, 0x004000, NO_DUMP )

	ROM_IPC
ROM_END

ROM_START( tonto )
    ROM_REGION( 0x400000, M68008_TAG, 0 )
	ROM_LOAD( "bios-1.rom", 0x000000, 0x008000, NO_DUMP )
	ROM_LOAD( "bios-2.rom", 0x008000, 0x008000, NO_DUMP )
	ROM_LOAD( "bios-3.rom", 0x010000, 0x008000, NO_DUMP )
	ROM_LOAD( "bios-4.rom", 0x018000, 0x008000, NO_DUMP )

    ROM_REGION( 0x10000, I8051_TAG, 0 )
	ROM_LOAD( "8051-1.rom", 0x000000, 0x010000, NO_DUMP )

    ROM_REGION( 0x10000, "tms5220", 0 )
	ROM_LOAD( "tms5220.rom", 0x000000, 0x004000, NO_DUMP )

    ROM_REGION( 0x400000, "rompack", 0 )
	ROM_LOAD( "rompack-1.rom", 0x000000, 0x008000, NO_DUMP )
	ROM_LOAD( "rompack-2.rom", 0x008000, 0x008000, NO_DUMP )
	ROM_LOAD( "rompack-3.rom", 0x010000, 0x008000, NO_DUMP )
	ROM_LOAD( "rompack-4.rom", 0x018000, 0x008000, NO_DUMP )
	ROM_LOAD( "rompack-5.rom", 0x020000, 0x008000, NO_DUMP )
ROM_END

ROM_START( megaopd )
    ROM_REGION( 0x400000, M68008_TAG, 0 )
	ROM_LOAD( "bios-1.rom", 0x000000, 0x008000, NO_DUMP )
	ROM_LOAD( "bios-2.rom", 0x008000, 0x008000, NO_DUMP )
	ROM_LOAD( "bios-3.rom", 0x010000, 0x008000, NO_DUMP )
	ROM_LOAD( "bios-4.rom", 0x018000, 0x008000, NO_DUMP )

    ROM_REGION( 0x10000, I8051_TAG, 0 )
	ROM_LOAD( "8051-1.rom", 0x000000, 0x010000, NO_DUMP )

    ROM_REGION( 0x10000, "tms5220", 0 )
	ROM_LOAD( "tms5220.rom", 0x000000, 0x004000, NO_DUMP )

    ROM_REGION( 0x400000, "rompack", 0 )
	ROM_LOAD( "rompack-1.rom", 0x000000, 0x008000, NO_DUMP )
	ROM_LOAD( "rompack-2.rom", 0x008000, 0x008000, NO_DUMP )
	ROM_LOAD( "rompack-3.rom", 0x010000, 0x008000, NO_DUMP )
	ROM_LOAD( "rompack-4.rom", 0x018000, 0x008000, NO_DUMP )
	ROM_LOAD( "rompack-5.rom", 0x020000, 0x008000, NO_DUMP )
ROM_END

/* System Configuration */

static QUICKLOAD_LOAD( ql )
{
	image.fread(messram_get_ptr(image.device().machine->device("messram")), 128*1024);

	return IMAGE_INIT_PASS;
}

/* Computer Drivers */

/*    YEAR  NAME    PARENT  COMPAT  MACHINE     INPUT   INIT    COMPANY                     FULLNAME        FLAGS */
COMP( 1984, ql,     0,      0,      ql,         ql,     0,      "Sinclair Research Ltd",    "QL (UK)",      GAME_SUPPORTS_SAVE )
COMP( 1985, ql_us,  ql,     0,      ql_ntsc,    ql,     0,      "Sinclair Research Ltd",    "QL (USA)",     GAME_SUPPORTS_SAVE )
COMP( 1985, ql_es,  ql,     0,      ql,         ql_es,  0,      "Sinclair Research Ltd",    "QL (Spain)",   GAME_SUPPORTS_SAVE )
COMP( 1985, ql_fr,  ql,     0,      ql,         ql_fr,  0,      "Sinclair Research Ltd",    "QL (France)",  GAME_NOT_WORKING )
COMP( 1985, ql_de,  ql,     0,      ql,         ql_de,  0,      "Sinclair Research Ltd",    "QL (Germany)", GAME_SUPPORTS_SAVE )
COMP( 1985, ql_it,  ql,     0,      ql,         ql_it,  0,      "Sinclair Research Ltd",    "QL (Italy)",   GAME_SUPPORTS_SAVE )
COMP( 1985, ql_se,  ql,     0,      ql,         ql_se,  0,      "Sinclair Research Ltd",    "QL (Sweden)",  GAME_NOT_WORKING )
COMP( 1985, ql_dk,  ql,     0,      ql,         ql_dk,  0,      "Sinclair Research Ltd",    "QL (Denmark)", GAME_NOT_WORKING )
COMP( 1985, ql_gr,  ql,     0,      ql,         ql,     0,      "Sinclair Research Ltd",    "QL (Greece)",  GAME_SUPPORTS_SAVE )
// last two did not used system config
COMP( 1984, tonto,  0,		0,		opd,		ql,		0,		"British Telecom Business Systems", "Merlin M1800 Tonto", GAME_NOT_WORKING )
COMP( 1986, megaopd,tonto,	0,		megaopd,	ql,		0,		"International Computer Limited", "MegaOPD (USA)", GAME_NOT_WORKING )
