#include "emu.h"
#include "includes/pc8401a.h"
#include "cpu/z80/z80.h"
#include "devices/cartslot.h"
#include "machine/i8255a.h"
#include "machine/msm8251.h"
#include "machine/upd1990a.h"
#include "video/sed1330.h"
#include "video/mc6845.h"
#include "devices/messram.h"

/*

    NEC PC-8401A-LS "Starlet"
    NEC PC-8500 "Studley"

    TODO:

    - keyboard interrupt
    - RTC TP pulse
    - disassembler for NEC uPD70008C (RST mnemonics are different from Z80)
    - clock does not advance in menu
    - mirror e800-ffff to 6800-7fff
    - soft power on/off
    - NVRAM
    - 8251 USART
    - 8255 ports
    - MC6845 palette
    - MC6845 chargen ROM

    - peripherals
        * PC-8431A Dual Floppy Drive
        * PC-8441A CRT / Disk Interface
        * PC-8461A 1200 Baud Modem
        * PC-8407A 128KB RAM Expansion
        * PC-8508A ROM/RAM Cartridge

    - Use the 600 baud save rate (PIP CAS2:=A:<filename.ext> this is more reliable than the 1200 baud (PIP CAS:=A:<filename.ext> rate.

*/

/* Fake Keyboard */

static UINT8 key_latch;

static void pc8401a_keyboard_scan(running_machine *machine)
{
	pc8401a_state *state = (pc8401a_state *)machine->driver_data;
	int row, strobe = 0;

	static const char *const keynames[] = { "KEY0", "KEY1", "KEY2", "KEY3", "KEY4", "KEY5", "KEY6", "KEY7", "KEY8", "KEY9" };

	/* scan keyboard */
	for (row = 0; row < 10; row++)
	{
		UINT8 data = input_port_read(machine, keynames[row]);

		if (data != 0xff)
		{
			strobe = 1;
			key_latch = data;
		}
	}

	if (!state->key_strobe && strobe)
	{
		/* trigger interrupt */
		cputag_set_input_line_and_vector(machine, Z80_TAG, INPUT_LINE_IRQ0, ASSERT_LINE, 0x28);
		logerror("INTERRUPT\n");
	}

	if (strobe)	state->key_strobe = strobe;
}

static TIMER_DEVICE_CALLBACK( pc8401a_keyboard_tick )
{
	pc8401a_keyboard_scan(timer->machine);
}

/* Read/Write Handlers */

static void pc8401a_bankswitch(running_machine *machine, UINT8 data)
{
	const address_space *program = cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM);

	int rombank = data & 0x03;
	int ram0000 = (data >> 2) & 0x03;
	int ram8000 = (data >> 4) & 0x03;

	switch (ram0000)
	{
	case 0: /* ROM 0000H to 7FFFH */
		if (rombank < 3)
		{
			/* internal ROM */
			memory_install_read_bank(program, 0x0000, 0x7fff, 0, 0, "bank1");
			memory_unmap_write(program, 0x0000, 0x7fff, 0, 0);
			memory_set_bank(machine, "bank1", rombank);
		}
		else
		{
			/* ROM cartridge */
			memory_unmap_readwrite(program, 0x0000, 0x7fff, 0, 0);
		}
		//logerror("0x0000-0x7fff = ROM %u\n", rombank);
		break;

	case 1: /* RAM 0000H to 7FFFH */
		memory_install_readwrite_bank(program, 0x0000, 0x7fff, 0, 0, "bank1");
		memory_set_bank(machine, "bank1", 4);
		//logerror("0x0000-0x7fff = RAM 0-7fff\n");
		break;

	case 2:	/* RAM 8000H to FFFFH */
		memory_install_readwrite_bank(program, 0x0000, 0x7fff, 0, 0, "bank1");
		memory_set_bank(machine, "bank1", 5);
		//logerror("0x0000-0x7fff = RAM 8000-ffff\n");
		break;

	case 3: /* invalid */
		logerror("0x0000-0x7fff = invalid\n");
		break;
	}

	switch (ram8000)
	{
	case 0: /* cell addresses 0000H to 3FFFH */
		memory_install_readwrite_bank(program, 0x8000, 0xbfff, 0, 0, "bank3");
		memory_set_bank(machine, "bank3", 0);
		//logerror("0x8000-0xbfff = RAM 0-3fff\n");
		break;

	case 1: /* cell addresses 4000H to 7FFFH */
		memory_install_readwrite_bank(program, 0x8000, 0xbfff, 0, 0, "bank3");
		memory_set_bank(machine, "bank3", 1);
		//logerror("0x8000-0xbfff = RAM 4000-7fff\n");
		break;

	case 2: /* cell addresses 8000H to BFFFH */
		memory_install_readwrite_bank(program, 0x8000, 0xbfff, 0, 0, "bank3");
		memory_set_bank(machine, "bank3", 2);
		//logerror("0x8000-0xbfff = RAM 8000-bfff\n");
		break;

	case 3: /* RAM cartridge */
		if (messram_get_size(devtag_get_device(machine, "messram")) > 64)
		{
			memory_install_readwrite_bank(program, 0x8000, 0xbfff, 0, 0, "bank3");
			memory_set_bank(machine, "bank3", 3); // TODO or 4
		}
		else
		{
			memory_unmap_readwrite(program, 0x8000, 0xbfff, 0, 0);
		}
		//logerror("0x8000-0xbfff = RAM cartridge\n");
		break;
	}

	if (BIT(data, 6))
	{
		/* CRT video RAM */
		memory_install_readwrite_bank(program, 0xc000, 0xdfff, 0, 0, "bank4");
		memory_unmap_readwrite(program, 0xe000, 0xe7ff, 0, 0);
		memory_set_bank(machine, "bank4", 1);
		//logerror("0xc000-0xdfff = video RAM\n");
	}
	else
	{
		/* RAM */
		memory_install_readwrite_bank(program, 0xc000, 0xe7ff, 0, 0, "bank4");
		memory_set_bank(machine, "bank4", 0);
		//logerror("0xc000-0e7fff = RAM c000-e7fff\n");
	}
}

static WRITE8_HANDLER( mmr_w )
{
	/*

        bit     description

        0       ROM section bit 0
        1       ROM section bit 1
        2       mapping for CPU addresses 0000H to 7FFFH bit 0
        3       mapping for CPU addresses 0000H to 7FFFH bit 1
        4       mapping for CPU addresses 8000H to BFFFH bit 0
        5       mapping for CPU addresses 8000H to BFFFH bit 1
        6       mapping for CPU addresses C000H to E7FFH
        7

    */

	pc8401a_state *state = (pc8401a_state *)space->machine->driver_data;

	if (data != state->mmr)
	{
		pc8401a_bankswitch(space->machine, data);
	}

	state->mmr = data;
}

static READ8_HANDLER( mmr_r )
{
	pc8401a_state *state = (pc8401a_state *)space->machine->driver_data;

	return state->mmr;
}

static READ8_HANDLER( rtc_r )
{
	/*

        bit     description

        0       RTC TP?
        1       RTC DATA OUT
        2       ?
        3
        4
        5
        6
        7

    */

	pc8401a_state *state = (pc8401a_state *)space->machine->driver_data;

	return (state->rtc_data << 1) | (state->rtc_tp << 2);
}

static WRITE8_HANDLER( rtc_cmd_w )
{
	/*

        bit     description

        0       RTC C0
        1       RTC C1
        2       RTC C2
        3       RTC DATA IN?
        4
        5
        6
        7

    */

	pc8401a_state *state = (pc8401a_state *)space->machine->driver_data;

	upd1990a_c0_w(state->upd1990a, BIT(data, 0));
	upd1990a_c1_w(state->upd1990a, BIT(data, 1));
	upd1990a_c2_w(state->upd1990a, BIT(data, 2));
	upd1990a_data_in_w(state->upd1990a, BIT(data, 3));
}

static WRITE8_HANDLER( rtc_ctrl_w )
{
	/*

        bit     description

        0       RTC OE or CS?
        1       RTC STB
        2       RTC CLK
        3
        4
        5
        6
        7

    */

	pc8401a_state *state = (pc8401a_state *)space->machine->driver_data;

	upd1990a_oe_w(state->upd1990a, BIT(data, 0));
	upd1990a_stb_w(state->upd1990a, BIT(data, 1));
	upd1990a_clk_w(state->upd1990a, BIT(data, 2));
}

static READ8_HANDLER( io_rom_data_r )
{
	pc8401a_state *state = (pc8401a_state *)space->machine->driver_data;

	UINT8 *iorom = memory_region(space->machine, "iorom");

	//logerror("I/O ROM read from %05x\n", state->io_addr);

	return iorom[state->io_addr];
}

static WRITE8_HANDLER( io_rom_addr_w )
{
	pc8401a_state *state = (pc8401a_state *)space->machine->driver_data;

	switch (offset)
	{
	case 0: /* A17..A16 */
		state->io_addr = ((data & 0x03) << 16) | (state->io_addr & 0xffff);
		break;

	case 1: /* A15..A8 */
		state->io_addr = (state->io_addr & 0x300ff) | (data << 8);
		break;

	case 2: /* A7..A0 */
		state->io_addr = (state->io_addr & 0x3ff00) | data;
		break;

	case 3:
		/* the same data is written here as to 0xb2, maybe this latches the address value? */
		break;
	}
}

static READ8_HANDLER( port70_r )
{
	/*

        bit     description

        0       key pressed
        1
        2
        3
        4       must be 1 or CPU goes to HALT
        5
        6
        7

    */

	pc8401a_state *state = (pc8401a_state *)space->machine->driver_data;

	return 0x10 | state->key_strobe;
}

static READ8_HANDLER( port71_r )
{
	return key_latch;
}

static WRITE8_HANDLER( port70_w )
{
	pc8401a_state *state = (pc8401a_state *)space->machine->driver_data;

	state->key_strobe = 0;
}

static WRITE8_HANDLER( port71_w )
{
	cputag_set_input_line(space->machine, Z80_TAG, INPUT_LINE_IRQ0, CLEAR_LINE);
	key_latch = data;
}

/* Memory Maps */

static ADDRESS_MAP_START( pc8401a_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_RAMBANK("bank1")
	AM_RANGE(0x8000, 0xbfff) AM_RAMBANK("bank3")
	AM_RANGE(0xc000, 0xe7ff) AM_RAMBANK("bank4")
	AM_RANGE(0xe800, 0xffff) AM_RAMBANK("bank5")
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc8401a_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc8500_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_READ_PORT("KEY0")
	AM_RANGE(0x01, 0x01) AM_READ_PORT("KEY1")
	AM_RANGE(0x02, 0x02) AM_READ_PORT("KEY2")
	AM_RANGE(0x03, 0x03) AM_READ_PORT("KEY3")
	AM_RANGE(0x04, 0x04) AM_READ_PORT("KEY4")
	AM_RANGE(0x05, 0x05) AM_READ_PORT("KEY5")
	AM_RANGE(0x06, 0x06) AM_READ_PORT("KEY6")
	AM_RANGE(0x07, 0x07) AM_READ_PORT("KEY7")
	AM_RANGE(0x08, 0x08) AM_READ_PORT("KEY8")
	AM_RANGE(0x09, 0x09) AM_READ_PORT("KEY9")
	AM_RANGE(0x10, 0x10) AM_WRITE(rtc_cmd_w)
	AM_RANGE(0x20, 0x20) AM_DEVREADWRITE(MSM8251_TAG, msm8251_data_r, msm8251_data_w)
	AM_RANGE(0x21, 0x21) AM_DEVREADWRITE(MSM8251_TAG, msm8251_status_r, msm8251_control_w)
	AM_RANGE(0x30, 0x30) AM_READWRITE(mmr_r, mmr_w)
//  AM_RANGE(0x31, 0x31)
	AM_RANGE(0x40, 0x40) AM_READWRITE(rtc_r, rtc_ctrl_w)
//  AM_RANGE(0x41, 0x41)
//  AM_RANGE(0x50, 0x51)
	AM_RANGE(0x60, 0x60) AM_DEVREADWRITE(SED1330_TAG, sed1330_status_r, sed1330_data_w)
	AM_RANGE(0x61, 0x61) AM_DEVREADWRITE(SED1330_TAG, sed1330_data_r, sed1330_command_w)
	AM_RANGE(0x70, 0x70) AM_READWRITE(port70_r, port70_w)
	AM_RANGE(0x71, 0x71) AM_READWRITE(port71_r, port71_w)
//  AM_RANGE(0x80, 0x80) modem status, set to 0xff to boot
//  AM_RANGE(0x8b, 0x8b)
//  AM_RANGE(0x90, 0x93)
//  AM_RANGE(0xa0, 0xa1)
	AM_RANGE(0x98, 0x98) AM_DEVWRITE(MC6845_TAG, mc6845_address_w)
	AM_RANGE(0x99, 0x99) AM_DEVREADWRITE(MC6845_TAG, mc6845_register_r, mc6845_register_w)
	AM_RANGE(0xb0, 0xb3) AM_WRITE(io_rom_addr_w)
	AM_RANGE(0xb3, 0xb3) AM_READ(io_rom_data_r)
//  AM_RANGE(0xc8, 0xc8)
	AM_RANGE(0xfc, 0xff) AM_DEVREADWRITE(I8255A_TAG, i8255a_r, i8255a_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( pc8401a )
	PORT_START("KEY0")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("STOP")// PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START("KEY1")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')

	PORT_START("KEY2")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')

	PORT_START("KEY3")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')

	PORT_START("KEY4")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('*')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('\'') PORT_CHAR('*')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('*')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\') PORT_CHAR('*')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR('*')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')

	PORT_START("KEY5")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('*')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('*')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('*')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('*')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('*')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('*')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('*')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('*')

	PORT_START("KEY6")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('*')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('*')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('*')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) // ?
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('*')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR('*')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('*')

	PORT_START("KEY7")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) // ^I
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F5") PORT_CODE(KEYCODE_F5) PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F4") PORT_CODE(KEYCODE_F4) PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F3") PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F3))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F2") PORT_CODE(KEYCODE_F2) PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F1") PORT_CODE(KEYCODE_F1) PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) // ^C

	PORT_START("KEY8")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F6)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F7)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("KEY9")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F8)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F9)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F10)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F11)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F12)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("DEL BKSP") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
INPUT_PORTS_END

/* Machine Initialization */

static MACHINE_START( pc8401a )
{
	pc8401a_state *state = (pc8401a_state *)machine->driver_data;

	/* find devices */
	state->upd1990a = devtag_get_device(machine, UPD1990A_TAG);

	/* initialize RTC */
	upd1990a_cs_w(state->upd1990a, 1);

	/* allocate CRT video RAM */
	state->crt_ram = auto_alloc_array(machine, UINT8, PC8401A_CRT_VIDEORAM_SIZE);

	/* set up A0/A1 memory banking */
	memory_configure_bank(machine, "bank1", 0, 4, memory_region(machine, Z80_TAG), 0x8000);
	memory_configure_bank(machine, "bank1", 4, 2, messram_get_ptr(devtag_get_device(machine, "messram")), 0x8000);
	memory_set_bank(machine, "bank1", 0);

	/* set up A2 memory banking */
	memory_configure_bank(machine, "bank3", 0, 5, messram_get_ptr(devtag_get_device(machine, "messram")), 0x4000);
	memory_set_bank(machine, "bank3", 0);

	/* set up A3 memory banking */
	memory_configure_bank(machine, "bank4", 0, 1, messram_get_ptr(devtag_get_device(machine, "messram")) + 0xc000, 0);
	memory_configure_bank(machine, "bank4", 1, 1, state->crt_ram, 0);
	memory_set_bank(machine, "bank4", 0);

	/* set up A4 memory banking */
	memory_configure_bank(machine, "bank5", 0, 1, messram_get_ptr(devtag_get_device(machine, "messram")) + 0xe800, 0);
	memory_set_bank(machine, "bank5", 0);

	/* bank switch */
	pc8401a_bankswitch(machine, 0);

	/* register for state saving */
	state_save_register_global_pointer(machine, state->crt_ram, PC8401A_CRT_VIDEORAM_SIZE);
	state_save_register_global(machine, state->rtc_data);
	state_save_register_global(machine, state->rtc_tp);
	state_save_register_global(machine, state->mmr);
	state_save_register_global(machine, state->io_addr);
}

static READ8_DEVICE_HANDLER( pc8401a_8255_c_r )
{
	/*

        bit     signal          description

        PC0
        PC1
        PC2
        PC3
        PC4     PC-8431A DAV    data valid
        PC5     PC-8431A RFD    ready for data
        PC6     PC-8431A DAC    data accepted
        PC7     PC-8431A ATN    attention

    */

	return 0;
}

static WRITE8_DEVICE_HANDLER( pc8401a_8255_c_w )
{
	/*

        bit     signal          description

        PC0
        PC1
        PC2
        PC3
        PC4     PC-8431A DAV    data valid
        PC5     PC-8431A RFD    ready for data
        PC6     PC-8431A DAC    data accepted
        PC7     PC-8431A ATN    attention

    */
}

static I8255A_INTERFACE( pc8401a_8255_interface )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(pc8401a_8255_c_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(pc8401a_8255_c_w),
};

/* uPD1990A Interface */

static WRITE_LINE_DEVICE_HANDLER( pc8401a_upd1990a_data_w )
{
	pc8401a_state *driver_state = (pc8401a_state *)device->machine->driver_data;

	driver_state->rtc_data = state;
}

static WRITE_LINE_DEVICE_HANDLER( pc8401a_upd1990a_tp_w )
{
	pc8401a_state *driver_state = (pc8401a_state *)device->machine->driver_data;

	driver_state->rtc_tp = state;
}

static UPD1990A_INTERFACE( pc8401a_upd1990a_intf )
{
	DEVCB_LINE(pc8401a_upd1990a_data_w),
	DEVCB_LINE(pc8401a_upd1990a_tp_w)
};

/* MSM8251 Interface */

static msm8251_interface pc8401a_msm8251_interface = {
	NULL,
	NULL,
	NULL
};

/* Machine Drivers */

static MACHINE_DRIVER_START( common )
	MDRV_DRIVER_DATA(pc8401a_state)

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80_TAG, Z80, 4000000) // NEC uPD70008C
	MDRV_CPU_PROGRAM_MAP(pc8401a_mem)
	MDRV_CPU_IO_MAP(pc8401a_io)

	MDRV_MACHINE_START(pc8401a)

	/* fake keyboard */
	MDRV_TIMER_ADD_PERIODIC("keyboard", pc8401a_keyboard_tick, HZ(64))

	/* devices */
	MDRV_UPD1990A_ADD(UPD1990A_TAG, XTAL_32_768kHz, pc8401a_upd1990a_intf)
	MDRV_I8255A_ADD(I8255A_TAG, pc8401a_8255_interface)
	MDRV_MSM8251_ADD(MSM8251_TAG, pc8401a_msm8251_interface)

	/* option ROM cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("rom,bin")
	MDRV_CARTSLOT_NOT_MANDATORY

	/* I/O ROM cartridge */
	MDRV_CARTSLOT_ADD("iocart")
	MDRV_CARTSLOT_EXTENSION_LIST("rom,bin")
	MDRV_CARTSLOT_NOT_MANDATORY

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("64K")
	MDRV_RAM_EXTRA_OPTIONS("96K")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( pc8401a )
	MDRV_IMPORT_FROM(common)

	/* video hardware */
	MDRV_IMPORT_FROM(pc8401a_video)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( pc8500 )
	MDRV_IMPORT_FROM(common)

	/* basic machine hardware */
	MDRV_CPU_MODIFY(Z80_TAG)
	MDRV_CPU_IO_MAP(pc8500_io)

	/* video hardware */
	MDRV_IMPORT_FROM(pc8500_video)

	/* internal ram */
	MDRV_RAM_MODIFY("messram")
	MDRV_RAM_DEFAULT_SIZE("64K")
MACHINE_DRIVER_END

/* ROMs */

ROM_START( pc8401a )
	ROM_REGION( 0x20000, Z80_TAG, ROMREGION_ERASEFF )
	ROM_LOAD( "pc8401a.bin", 0x0000, 0x18000, NO_DUMP )
	ROM_CART_LOAD("cart", 0x18000, 0x8000, ROM_NOMIRROR | ROM_OPTIONAL)

	ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD( "pc8441a.bin", 0x0000, 0x1000, NO_DUMP )

	ROM_REGION( 0x40000, "iorom", ROMREGION_ERASEFF )
	ROM_CART_LOAD("iocart", 0x00000, 0x40000, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START( pc8500 )
	ROM_REGION( 0x20000, Z80_TAG, ROMREGION_ERASEFF )
	ROM_LOAD( "pc8500.bin", 0x0000, 0x10000, CRC(c2749ef0) SHA1(f766afce9fda9ec84ed5b39ebec334806798afb3) )
	ROM_CART_LOAD("cart", 0x18000, 0x8000, ROM_NOMIRROR | ROM_OPTIONAL)

	ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD( "pc8441a.bin", 0x0000, 0x1000, NO_DUMP )

	ROM_REGION( 0x40000, "iorom", ROMREGION_ERASEFF )
	ROM_CART_LOAD("iocart", 0x00000, 0x40000, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

/* System Drivers */

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT       INIT    COMPANY FULLNAME */
COMP( 1984,	pc8401a,	0,		0,		pc8401a,	pc8401a,	0,		"Nippon Electronic Company",	"PC-8401A-LS", GAME_NOT_WORKING )
/*
COMP( 1984, pc8401bd,   pc8401a,0,      pc8401a,    pc8401a,    0,      "Nippon Electronic Company",  "PC-8401BD", GAME_NOT_WORKING )
*/
COMP( 1985, pc8500,		0,		0,		pc8500,		pc8401a,	0,		"Nippon Electronic Company",	"PC-8500", GAME_NOT_WORKING | GAME_NO_SOUND)
