/***************************************************************************

    Tiki 100

    12/05/2009 Skeleton driver.

    http://www.djupdal.org/tiki/

****************************************************************************/

/*

    TODO:

    - palette RAM should be written during HBLANK
    - double sided disks have t0s0,t0s1,t1s0,t1s1... format
    - DART clocks
    - daisy chain order?
    - winchester hard disk
    - analog/digital I/O
    - light pen
    - 8088 CPU card

*/

#include "emu.h"
#include "includes/tiki100.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "devices/flopdrv.h"
#include "formats/basicdsk.h"
#include "machine/z80ctc.h"
#include "machine/z80dart.h"
#include "machine/z80pio.h"
#include "machine/wd17xx.h"
#include "sound/ay8910.h"
#include "devices/messram.h"

INLINE running_device *get_floppy_image(running_machine *machine, int drive)
{
	return floppy_get_device(machine, drive);
}

/* Memory Banking */

static READ8_HANDLER( gfxram_r )
{
	tiki100_state *state = space->machine->driver_data<tiki100_state>();

	UINT16 addr = (offset + (state->scroll << 7)) & TIKI100_VIDEORAM_MASK;

	return state->video_ram[addr];
}

static WRITE8_HANDLER( gfxram_w )
{
	tiki100_state *state = space->machine->driver_data<tiki100_state>();

	UINT16 addr = (offset + (state->scroll << 7)) & TIKI100_VIDEORAM_MASK;

	state->video_ram[addr] = data;
}

static void tiki100_bankswitch(running_machine *machine)
{
	tiki100_state *state = machine->driver_data<tiki100_state>();
	const address_space *program = cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM);

	if (state->vire)
	{
		if (!state->rome)
		{
			/* reserved */
			memory_unmap_readwrite(program, 0x0000, 0xffff, 0, 0);
		}
		else
		{
			/* GFXRAM, GFXRAM, RAM */
			memory_install_readwrite8_handler(program, 0x0000, 0x7fff, 0, 0, gfxram_r, gfxram_w);
			memory_install_readwrite_bank(program, 0x8000, 0xffff, 0, 0, "bank3");

			memory_set_bank(machine, "bank1", BANK_VIDEO_RAM);
			memory_set_bank(machine, "bank2", BANK_VIDEO_RAM);
			memory_set_bank(machine, "bank3", BANK_RAM);
		}
	}
	else
	{
		if (!state->rome)
		{
			/* ROM, RAM, RAM */
			memory_install_read_bank(program, 0x0000, 0x3fff, 0, 0, "bank1");
			memory_unmap_write( program, 0x0000, 0x3fff, 0, 0 );
			memory_install_readwrite_bank(program, 0x4000, 0x7fff, 0, 0, "bank2");
			memory_install_readwrite_bank(program, 0x8000, 0xffff, 0, 0, "bank3");

			memory_set_bank(machine, "bank1", BANK_ROM);
			memory_set_bank(machine, "bank2", BANK_RAM);
			memory_set_bank(machine, "bank3", BANK_RAM);
		}
		else
		{
			/* RAM, RAM, RAM */
			memory_install_readwrite_bank(program, 0x0000, 0x3fff, 0, 0, "bank1");
			memory_install_readwrite_bank(program, 0x4000, 0x7fff, 0, 0, "bank2");
			memory_install_readwrite_bank(program, 0x8000, 0xffff, 0, 0, "bank3");

			memory_set_bank(machine, "bank1", BANK_RAM);
			memory_set_bank(machine, "bank2", BANK_RAM);
			memory_set_bank(machine, "bank3", BANK_RAM);
		}
	}
}

/* Read/Write Handlers */

static READ8_HANDLER( keyboard_r )
{
	tiki100_state *state = space->machine->driver_data<tiki100_state>();

	static const char *const keynames[] = { "ROW1", "ROW2", "ROW3", "ROW4", "ROW5", "ROW6", "ROW7", "ROW8", "ROW9", "ROW10", "ROW11", "ROW12" };
	UINT8 data = input_port_read(space->machine, keynames[state->keylatch]);

	state->keylatch++;

	if (state->keylatch == 12) state->keylatch = 0;

	return data;
}

static WRITE8_HANDLER( keyboard_w )
{
	tiki100_state *state = space->machine->driver_data<tiki100_state>();

	state->keylatch = 0;
}

static WRITE8_HANDLER( video_mode_w )
{
	/*

        bit     description

        0       palette entry bit 0
        1       palette entry bit 1
        2       palette entry bit 2
        3       palette entry bit 3
        4       mode select bit 0
        5       mode select bit 1
        6       unused
        7       write color during HBLANK

    */

	tiki100_state *state = space->machine->driver_data<tiki100_state>();

	state->mode = data;

	if (BIT(data, 7))
	{
		int color = data & 0x0f;
		UINT8 colordata = ~state->palette;

		palette_set_color_rgb(space->machine, color, pal3bit(colordata >> 5), pal3bit(colordata >> 2), pal2bit(colordata >> 0));
	}
}

static WRITE8_HANDLER( palette_w )
{
	/*

        bit     description

        0       blue intensity bit 0
        1       blue intensity bit 1
        2       green intensity bit 0
        3       green intensity bit 1
        4       green intensity bit 2
        5       red intensity bit 0
        6       red intensity bit 1
        7       red intensity bit 2

    */

	tiki100_state *state = space->machine->driver_data<tiki100_state>();

	state->palette = data;
}

static WRITE8_HANDLER( system_w )
{
	/*

        bit     signal  description

        0       DRIS0   drive select 0
        1       DRIS1   drive select 1
        2       _ROME   enable ROM at 0000-3fff
        3       VIRE    enable video RAM at 0000-7fff
        4       SDEN    single density select (0=DD, 1=SD)
        5       _LMP0   GRAFIKK key led
        6       MOTON   floppy motor
        7       _LMP1   LOCK key led

    */

	tiki100_state *state = space->machine->driver_data<tiki100_state>();

	/* drive select */
	if (BIT(data, 0)) wd17xx_set_drive(state->fd1797, 0);
	if (BIT(data, 1)) wd17xx_set_drive(state->fd1797, 1);

	/* density select */
	wd17xx_dden_w(state->fd1797, BIT(data, 4));

	/* floppy motor */
	floppy_mon_w(get_floppy_image(space->machine, 0), !BIT(data, 6));
	floppy_mon_w(get_floppy_image(space->machine, 1), !BIT(data, 6));
	floppy_drive_set_ready_state(get_floppy_image(space->machine, 0), BIT(data, 6), 1);
	floppy_drive_set_ready_state(get_floppy_image(space->machine, 1), BIT(data, 6), 1);

	/* GRAFIKK key led */
	set_led_status(space->machine, 1, BIT(data, 5));

	/* LOCK key led */
	set_led_status(space->machine, 2, BIT(data, 7));

	/* bankswitch */
	state->rome = BIT(data, 2);
	state->vire = BIT(data, 3);

	tiki100_bankswitch(space->machine);
}

/* Memory Maps */

static ADDRESS_MAP_START( tiki100_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x3fff) AM_RAMBANK("bank1")
	AM_RANGE(0x4000, 0x7fff) AM_RAMBANK("bank2")
	AM_RANGE(0x8000, 0xffff) AM_RAMBANK("bank3")
ADDRESS_MAP_END

static ADDRESS_MAP_START( tiki100_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_MIRROR(0x03) AM_READWRITE(keyboard_r, keyboard_w)
	AM_RANGE(0x04, 0x07) AM_DEVREADWRITE(Z80DART_TAG, z80dart_cd_ba_r, z80dart_cd_ba_w)
	AM_RANGE(0x08, 0x0b) AM_DEVREADWRITE(Z80PIO_TAG, z80pio_cd_ba_r, z80pio_cd_ba_w)
	AM_RANGE(0x0c, 0x0c) AM_MIRROR(0x03) AM_WRITE(video_mode_w)
	AM_RANGE(0x10, 0x13) AM_DEVREADWRITE(FD1797_TAG, wd17xx_r, wd17xx_w)
	AM_RANGE(0x14, 0x14) AM_MIRROR(0x01) AM_WRITE(palette_w)
	AM_RANGE(0x16, 0x16) AM_DEVWRITE(AY8912_TAG, ay8910_address_w)
	AM_RANGE(0x17, 0x17) AM_DEVREADWRITE(AY8912_TAG, ay8910_r, ay8910_data_w)
	AM_RANGE(0x18, 0x1b) AM_DEVREADWRITE(Z80CTC_TAG, z80ctc_r, z80ctc_w)
	AM_RANGE(0x1c, 0x1c) AM_MIRROR(0x03) AM_WRITE(system_w)
//  AM_RANGE(0x20, 0x27) winchester controller
//  AM_RANGE(0x60, 0x6f) analog I/O (SINTEF)
//  AM_RANGE(0x60, 0x67) digital I/O (RVO)
//  AM_RANGE(0x70, 0x77) analog/digital I/O
//  AM_RANGE(0x78, 0x7b) light pen
//  AM_RANGE(0x7e, 0x7f) 8088/87 processor w/128KB RAM
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( tiki100 )
/*
        |    0    |    1    |    2    |    3    |    4    |    5    |    6    |    7    |
    ----+---------+---------+---------+---------+---------+---------+---------+---------+
      1 | CTRL    | SHIFT   | BRYT    | RETUR   | MLMROM  | / (num) | SLETT   |         |
      2 | GRAFIKK | 1       | ANGRE   | a       | <       | z       | q       | LOCK    |
      3 | 2       | w       | s       | x       | 3       | e       | d       | c       |
      4 | 4       | r       | f       | v       | 5       | t       | g       | b       |
      5 | 6       | y       | h       | n       | 7       | u       | j       | m       |
      6 | 8       | i       | k       | ,       | 9       | o       | l       | .       |
      7 | 0       | p       | ?       | -       | +       | ?       | ?       | HJELP   |
      8 | @       | ^       | '       | VENSTRE | UTVID   | F1      | F4      | SIDEOPP |
      9 | F2      | F3      | F5      | F6      | OPP     | SIDENED | VTAB    | NED     |
     10 | + (num) | - (num) | * (num) | 7 (num) | 8 (num) | 9 (num) | % (num) | = (num) |
     11 | 4 (num) | 5 (num) | 6 (num) | HTAB    | 1 (num) | 0 (num) | . (num) |         |
     12 | HJEM    | H?YRE   | 2 (num) | 3 (num) | ENTER   |         |         |         |
    ----+---------+---------+---------+---------+---------+---------+---------+---------+
*/
	PORT_START("ROW1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("BRYTT") PORT_CODE(KEYCODE_ESC)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("MLMROM") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad /") PORT_CODE(KEYCODE_SLASH_PAD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SLETT") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("GRAFIKK") PORT_CODE(KEYCODE_LALT) PORT_CHAR(UCHAR_MAMEKEY(LALT))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("ANGRE") PORT_CODE(KEYCODE_DEL) PORT_CHAR(UCHAR_MAMEKEY(DEL))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH2) PORT_CHAR('<') PORT_CHAR('>')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("LOCK") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))

	PORT_START("ROW3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')

	PORT_START("ROW4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')

	PORT_START("ROW5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('/')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')

	PORT_START("ROW6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR(';')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR(':')

	PORT_START("ROW7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('=')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xC3\xB8 \xC3\x98") PORT_CODE(KEYCODE_COLON) PORT_CHAR(0x00f8) PORT_CHAR(0x00d8)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('+') PORT_CHAR('?')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xC3\xA5 \xC3\x85") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR(0x00e5) PORT_CHAR(0x00c5)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xC3\xA6 \xC3\x86") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(0x00e6) PORT_CHAR(0x00c6)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("HJELP")

	PORT_START("ROW8")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('@') PORT_CHAR('`')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('^') PORT_CHAR('|')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\'') PORT_CHAR('*')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("UTVID") PORT_CODE(KEYCODE_INSERT) PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F1") PORT_CODE(KEYCODE_F1) PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F4") PORT_CODE(KEYCODE_F4) PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SIDEOPP") PORT_CODE(KEYCODE_PGUP) PORT_CHAR(UCHAR_MAMEKEY(PGUP))

	PORT_START("ROW9")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F2") PORT_CODE(KEYCODE_F2) PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F3") PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F3))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F5") PORT_CODE(KEYCODE_F5) PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F6") PORT_CODE(KEYCODE_F6) PORT_CHAR(UCHAR_MAMEKEY(F6))
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SIDENED") PORT_CODE(KEYCODE_PGDN) PORT_CHAR(UCHAR_MAMEKEY(PGDN))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("VTAB")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x93") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))

	PORT_START("ROW10")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad +") PORT_CODE(KEYCODE_PLUS_PAD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad -") PORT_CODE(KEYCODE_MINUS_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad *") PORT_CODE(KEYCODE_ASTERISK)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 7") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 8") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 9") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad %")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad =")

	PORT_START("ROW11")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 4") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 5") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 6") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("HTAB") PORT_CODE(KEYCODE_TAB) PORT_CHAR(UCHAR_MAMEKEY(TAB)) PORT_CHAR('\t')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 1") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 0") PORT_CODE(KEYCODE_0_PAD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad .") PORT_CODE(KEYCODE_DEL_PAD)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW12")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("HJEM") PORT_CODE(KEYCODE_HOME) PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 2") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 3") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad ENTER") PORT_CODE(KEYCODE_ENTER_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

/* Video */

static VIDEO_UPDATE( tiki100 )
{
	tiki100_state *state = screen->machine->driver_data<tiki100_state>();

	UINT16 addr = (state->scroll << 7);
	int sx, y, pixel, mode = (state->mode >> 4) & 0x03;

	for (y = 0; y < 256; y++)
	{
		for (sx = 0; sx < 128; sx++)
		{
			UINT8 data = state->video_ram[addr & TIKI100_VIDEORAM_MASK];

			switch (mode)
			{
			case 0:
				for (pixel = 0; pixel < 8; pixel++)
				{
					int x = (sx * 8) + pixel;

					*BITMAP_ADDR16(bitmap, y, x) = 0;
				}
				break;

			case 1: /* 1024x256x2 */
				for (pixel = 0; pixel < 8; pixel++)
				{
					int x = (sx * 8) + pixel;
					int color = BIT(data, 0);

					*BITMAP_ADDR16(bitmap, y, x) = color;

					data >>= 1;
				}
				break;

			case 2: /* 512x256x4 */
				for (pixel = 0; pixel < 4; pixel++)
				{
					int x = (sx * 8) + (pixel * 2);
					int color = data & 0x03;

					*BITMAP_ADDR16(bitmap, y, x) = color;
					*BITMAP_ADDR16(bitmap, y, x + 1) = color;

					data >>= 2;
				}
				break;

			case 3: /* 256x256x16 */
				for (pixel = 0; pixel < 2; pixel++)
				{
					int x = (sx * 8) + (pixel * 4);
					int color = data & 0x0f;

					*BITMAP_ADDR16(bitmap, y, x) = color;
					*BITMAP_ADDR16(bitmap, y, x + 1) = color;
					*BITMAP_ADDR16(bitmap, y, x + 2) = color;
					*BITMAP_ADDR16(bitmap, y, x + 3) = color;

					data >>= 4;
				}
				break;
			}

			addr++;
		}
	}

	return 0;
}

/* Z80-DART Interface */

static Z80DART_INTERFACE( dart_intf )
{
	0,
	0,
	0,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0)
};

/* Z80-PIO Interface */

static Z80PIO_INTERFACE( pio_intf )
{
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),	/* callback when change interrupt status */
	DEVCB_NULL,						/* port A read callback */
	DEVCB_NULL,						/* port A write callback */
	DEVCB_NULL,						/* portA ready active callback */
	DEVCB_NULL,						/* port B read callback */
	DEVCB_NULL,						/* port B write callback */
	DEVCB_NULL						/* portB ready active callback */
};

/* Z80-CTC Interface */

static TIMER_DEVICE_CALLBACK( ctc_tick )
{
	tiki100_state *state = timer.machine->driver_data<tiki100_state>();

	z80ctc_trg0_w(state->z80ctc, 1);
	z80ctc_trg0_w(state->z80ctc, 0);

	z80ctc_trg1_w(state->z80ctc, 1);
	z80ctc_trg1_w(state->z80ctc, 0);
}

static WRITE_LINE_DEVICE_HANDLER( ctc_z1_w )
{
}

static Z80CTC_INTERFACE( ctc_intf )
{
	0,              			/* timer disables */
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),	/* interrupt handler */
	DEVCB_LINE(z80ctc_trg2_w),	/* ZC/TO0 callback */
	DEVCB_LINE(ctc_z1_w),		/* ZC/TO1 callback */
	DEVCB_LINE(z80ctc_trg3_w)	/* ZC/TO2 callback */
};

/* FD1797 Interface */

static const wd17xx_interface tiki100_wd17xx_interface =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};

/* AY-3-8912 Interface */

static WRITE8_DEVICE_HANDLER( video_scroll_w )
{
	tiki100_state *state = device->machine->driver_data<tiki100_state>();

	state->scroll = data;
}

static const ay8910_interface ay8910_intf =
{
	AY8910_LEGACY_OUTPUT,
	AY8910_DEFAULT_LOADS,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(video_scroll_w),
	DEVCB_NULL
};

/* Z80 Daisy Chain */

static const z80_daisy_config tiki100_daisy_chain[] =
{
	{ Z80CTC_TAG },
	{ Z80PIO_TAG },
	{ Z80DART_TAG },
	{ NULL }
};

/* Machine Start */

static MACHINE_START( tiki100 )
{
	tiki100_state *state = machine->driver_data<tiki100_state>();

	/* find devices */
	state->fd1797 = machine->device(FD1797_TAG);
	state->z80ctc = machine->device(Z80CTC_TAG);

	/* allocate video RAM */
	state->video_ram = auto_alloc_array(machine, UINT8, TIKI100_VIDEORAM_SIZE);

	/* setup memory banking */
	memory_configure_bank(machine, "bank1", BANK_ROM, 1, memory_region(machine, Z80_TAG), 0);
	memory_configure_bank(machine, "bank1", BANK_RAM, 1, messram_get_ptr(machine->device("messram")), 0);
	memory_configure_bank(machine, "bank1", BANK_VIDEO_RAM, 1, state->video_ram, 0);

	memory_configure_bank(machine, "bank2", BANK_RAM, 1, messram_get_ptr(machine->device("messram")) + 0x4000, 0);
	memory_configure_bank(machine, "bank2", BANK_VIDEO_RAM, 1, state->video_ram + 0x4000, 0);

	memory_configure_bank(machine, "bank3", BANK_RAM, 1, messram_get_ptr(machine->device("messram")) + 0x8000, 0);

	tiki100_bankswitch(machine);

	/* register for state saving */
	state_save_register_global(machine, state->rome);
	state_save_register_global(machine, state->vire);
	state_save_register_global_pointer(machine, state->video_ram, TIKI100_VIDEORAM_SIZE);
	state_save_register_global(machine, state->scroll);
	state_save_register_global(machine, state->mode);
	state_save_register_global(machine, state->palette);
	state_save_register_global(machine, state->keylatch);
}

static FLOPPY_OPTIONS_START(tiki100)
	FLOPPY_OPTION(tiki100, "dsk", "SSSD disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([40])
		SECTORS([18])
		SECTOR_LENGTH([128])
		FIRST_SECTOR_ID([1]))
	FLOPPY_OPTION(tiki100, "dsk", "SSDD disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([40])
		SECTORS([10])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))
	FLOPPY_OPTION(tiki100, "dsk", "DSDD disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([40])
		SECTORS([10])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))
	FLOPPY_OPTION(tiki100, "dsk", "DSHD disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([80])
		SECTORS([10])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

static const floppy_config tiki100_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	FLOPPY_OPTIONS_NAME(tiki100),
	NULL
};

/* Machine Driver */

static MACHINE_DRIVER_START( tiki100 )
	MDRV_DRIVER_DATA(tiki100_state)

	/* basic machine hardware */
    MDRV_CPU_ADD(Z80_TAG, Z80, 2000000)
    MDRV_CPU_PROGRAM_MAP(tiki100_mem)
    MDRV_CPU_IO_MAP(tiki100_io)
	MDRV_CPU_CONFIG(tiki100_daisy_chain)

    MDRV_MACHINE_START(tiki100)

    /* video hardware */
    MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(1024, 256)
    MDRV_SCREEN_VISIBLE_AREA(0, 1024-1, 0, 256-1)

	MDRV_PALETTE_LENGTH(16)

    MDRV_VIDEO_UPDATE(tiki100)

	/* devices */
	MDRV_Z80DART_ADD(Z80DART_TAG, 2000000, dart_intf)
	MDRV_Z80PIO_ADD(Z80PIO_TAG, 2000000, pio_intf)
	MDRV_Z80CTC_ADD(Z80CTC_TAG, 2000000, ctc_intf)
	MDRV_TIMER_ADD_PERIODIC("ctc", ctc_tick, HZ(2000000))
	MDRV_WD179X_ADD(FD1797_TAG, tiki100_wd17xx_interface) // FD1767PL-02 or FD1797-PL
	MDRV_FLOPPY_2_DRIVES_ADD(tiki100_floppy_config)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8912_TAG, AY8912, 2000000)
	MDRV_SOUND_CONFIG(ay8910_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("64K")
MACHINE_DRIVER_END

/* ROMs */

ROM_START( kontiki )
	ROM_REGION( 0x10000, Z80_TAG, ROMREGION_ERASEFF )
	ROM_LOAD( "tikirom-1.30.u10",  0x0000, 0x2000, CRC(c482dcaf) SHA1(d140706bb7fc8b1fbb37180d98921f5bdda73cf9) )
ROM_END

ROM_START( tiki100 )
	ROM_REGION( 0x10000, Z80_TAG, ROMREGION_ERASEFF )
	ROM_DEFAULT_BIOS( "v203w" )

	ROM_SYSTEM_BIOS( 0, "v135", "TIKI ROM v1.35" )
	ROMX_LOAD( "tikirom-1.35.u10",  0x0000, 0x2000, CRC(7dac5ee7) SHA1(14d622fd843833faec346bf5357d7576061f5a3d), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "v203w", "TIKI ROM v2.03 W" )
	ROMX_LOAD( "tikirom-2.03w.u10", 0x0000, 0x2000, CRC(79662476) SHA1(96336633ecaf1b2190c36c43295ac9f785d1f83a), ROM_BIOS(2) )
ROM_END

/* System Drivers */

/*    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT       INIT    COMPANY             FULLNAME        FLAGS */
COMP( 1984, kontiki,	0,			0,		tiki100,	tiki100,	0,		"Kontiki Data A/S",	"KONTIKI 100",	GAME_SUPPORTS_SAVE )
COMP( 1984, tiki100,	kontiki,	0,		tiki100,	tiki100,	0,		"Tiki Data A/S",	"TIKI 100",		GAME_SUPPORTS_SAVE )
