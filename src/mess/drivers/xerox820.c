/***************************************************************************

        Xerox 820

        12/05/2009 Skeleton driver.


****************************************************************************/

/*

    TODO:

    - Big Board (+ Italian version MK-82)
    - Big Board II (+ Italian version MK-83)
    - Xerox 820-II
    - Xerox 16/8
    - Emerald Microware X120 board
    - type in Monitor v1.0 from manual
    - proper keyboard emulation (MCU?)

    http://users.telenet.be/lust/Xerox820/index.htm
    http://www.classiccmp.org/dunfield/img41867/system.htm
    http://www.microcodeconsulting.com/z80/plus2.htm

*/

#include "driver.h"
#include "includes/xerox820.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "machine/z80pio.h"
#include "machine/z80ctc.h"
#include "machine/z80sio.h"
#include "machine/wd17xx.h"
#include "machine/com8116.h"
#include "devices/flopdrv.h"
#include "formats/basicdsk.h"
#include "devices/messram.h"

INLINE const device_config *get_floppy_image(running_machine *machine, int drive)
{
	return floppy_get_device(machine, drive);
}

/* Keyboard HACK */

static const UINT8 xerox820_keycodes[3][9][8] =
{
	/* unshifted */
	{
	{ 0x1e, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37 },
	{ 0x38, 0x39, 0x30, 0x2d, 0x3d, 0x08, 0x7f, 0x2d },
	{ 0x37, 0x38, 0x39, 0x09, 0x71, 0x77, 0x65, 0x72 },
	{ 0x74, 0x79, 0x75, 0x69, 0x6f, 0x70, 0x5b, 0x5d },
	{ 0x1b, 0x2b, 0x34, 0x35, 0x36, 0x61, 0x73, 0x64 },
	{ 0x66, 0x67, 0x68, 0x6a, 0x6b, 0x6c, 0x3b, 0x27 },
	{ 0x0d, 0x0a, 0x01, 0x31, 0x32, 0x33, 0x7a, 0x78 },
	{ 0x63, 0x76, 0x62, 0x6e, 0x6d, 0x2c, 0x2e, 0x2f },
	{ 0x04, 0x02, 0x03, 0x30, 0x2e, 0x20, 0x00, 0x00 }
	},

	/* shifted */
	{
	{ 0x1e, 0x21, 0x40, 0x23, 0x24, 0x25, 0x5e, 0x26 },
	{ 0x2a, 0x28, 0x29, 0x5f, 0x2b, 0x08, 0x7f, 0x2d },
	{ 0x37, 0x38, 0x39, 0x09, 0x51, 0x57, 0x45, 0x52 },
	{ 0x54, 0x59, 0x55, 0x49, 0x4f, 0x50, 0x7b, 0x7d },
	{ 0x1b, 0x2b, 0x34, 0x35, 0x36, 0x41, 0x53, 0x44 },
	{ 0x46, 0x47, 0x48, 0x4a, 0x4b, 0x4c, 0x3a, 0x22 },
	{ 0x0d, 0x0a, 0x01, 0x31, 0x32, 0x33, 0x5a, 0x58 },
	{ 0x43, 0x56, 0x42, 0x4e, 0x4d, 0x3c, 0x3e, 0x3f },
	{ 0x04, 0x02, 0x03, 0x30, 0x2e, 0x20, 0x00, 0x00 }
	},

	/* control */
	{
	{ 0x9e, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97 },
	{ 0x98, 0x99, 0x90, 0x1f, 0x9a, 0x88, 0xff, 0xad },
	{ 0xb7, 0xb8, 0xb9, 0x89, 0x11, 0x17, 0x05, 0x12 },
	{ 0x14, 0x19, 0x15, 0x09, 0x0f, 0x10, 0x1b, 0x1d },
	{ 0x9b, 0xab, 0xb4, 0xb5, 0xb6, 0x01, 0x13, 0x04 },
	{ 0x06, 0x07, 0x08, 0x0a, 0x0b, 0x0c, 0x7e, 0x60 },
	{ 0x8d, 0x8a, 0x81, 0xb1, 0xb2, 0xb3, 0x1a, 0x18 },
	{ 0x03, 0x16, 0x02, 0x0e, 0x0d, 0x1c, 0x7c, 0x5c },
	{ 0x84, 0x82, 0x83, 0xb0, 0xae, 0x00, 0x00, 0x00 }
	}
};

static void xerox820_keyboard_scan(running_machine *machine)
{
	xerox820_state *state = machine->driver_data;

	static const char *const keynames[] = { "ROW0", "ROW1", "ROW2", "ROW3", "ROW4", "ROW5", "ROW6", "ROW7", "ROW8" };
	int table = 0, row, col;
	int keydata = -1;

	if (input_port_read(machine, "ROW9") & 0x07)
	{
		/* shift, upper case */
		table = 1;
	}

	if (input_port_read(machine, "ROW9") & 0x18)
	{
		/* ctrl */
		table = 2;
	}

	/* scan keyboard */
	for (row = 0; row < 9; row++)
	{
		UINT8 data = input_port_read(machine, keynames[row]);

		for (col = 0; col < 8; col++)
		{
			if (!BIT(data, col))
			{
				/* latch key data */
				keydata = ~xerox820_keycodes[table][row][col];

				if (state->keydata != keydata)
				{
					state->keydata = keydata;
					//z80pio_bstb_w(state->kbpio, 0);
					//z80pio_bstb_w(state->kbpio, 1);
					z80pio_p_w(state->kbpio, 1, state->keydata);
				}
			}
		}
	}

	state->keydata = keydata;
}

static TIMER_DEVICE_CALLBACK( xerox820_keyboard_tick )
{
	xerox820_keyboard_scan(timer->machine);
}

/* Read/Write Handlers */

static void xerox820_bankswitch(running_machine *machine, int bank)
{
	const address_space *program = cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM);

	if (bank)
	{
		/* ROM */
		memory_install_readwrite8_handler(program, 0x0000, 0x0fff, 0, 0, SMH_BANK(1), SMH_UNMAP);
		memory_install_readwrite8_handler(program, 0x1000, 0x2fff, 0, 0, SMH_UNMAP, SMH_UNMAP);
	}
	else
	{
		/* RAM */
		memory_install_readwrite8_handler(program, 0x0000, 0x0fff, 0, 0, SMH_BANK(1), SMH_BANK(1));
		memory_install_readwrite8_handler(program, 0x1000, 0x2fff, 0, 0, SMH_BANK(2), SMH_BANK(2));
	}

	memory_install_readwrite8_handler(program, 0x3000, 0x3fff, 0, 0, SMH_BANK(3), SMH_BANK(3));

	memory_set_bank(machine, 1, bank);
	memory_set_bank(machine, 3, bank);
}

static WRITE8_HANDLER( scroll_w )
{
	xerox820_state *state = space->machine->driver_data;

	state->scroll = (offset >> 8) & 0x1f;
}

#ifdef UNUSED_CODE
static WRITE8_HANDLER( x120_system_w )
{
	/*

        bit     signal      description

        0       DSEL0       drive select bit 0 (01=A, 10=B, 00=C, 11=D)
        1       DSEL1       drive select bit 1
        2       SIDE        side select
        3       VATT        video attribute (0=inverse, 1=blinking)
        4       BELL        bell trigger
        5       DENSITY     density (0=double, 1=single)
        6       _MOTOR      disk motor (0=on, 1=off)
        7       BANK        memory bank switch (0=RAM, 1=ROM/video)

    */
}
#endif

/* Memory Maps */

static ADDRESS_MAP_START( xerox820_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0fff) AM_RAMBANK(1)
	AM_RANGE(0x1000, 0x2fff) AM_RAMBANK(2)
	AM_RANGE(0x3000, 0x3fff) AM_RAMBANK(3)
	AM_RANGE(0x4000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( xerox820_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_MIRROR(0xff03) AM_DEVWRITE(COM8116_TAG, com8116_str_w)
	AM_RANGE(0x04, 0x04) AM_MIRROR(0xff02) AM_DEVREADWRITE(Z80SIO_TAG, z80sio_d_r, z80sio_d_w)
	AM_RANGE(0x05, 0x05) AM_MIRROR(0xff02) AM_DEVREADWRITE(Z80SIO_TAG, z80sio_c_r, z80sio_c_w)
	AM_RANGE(0x08, 0x0b) AM_MIRROR(0xff00) AM_DEVREADWRITE(Z80GPPIO_TAG, z80pio_alt_r, z80pio_alt_w)
	AM_RANGE(0x0c, 0x0c) AM_MIRROR(0xff03) AM_DEVWRITE(COM8116_TAG, com8116_stt_w)
	AM_RANGE(0x10, 0x13) AM_MIRROR(0xff00) AM_DEVREADWRITE(WD1771_TAG, wd17xx_r, wd17xx_w)
	AM_RANGE(0x14, 0x14) AM_MIRROR(0xff03) AM_MASK(0xff00) AM_WRITE(scroll_w)
	AM_RANGE(0x18, 0x1b) AM_MIRROR(0xff00) AM_DEVREADWRITE(Z80CTC_TAG, z80ctc_r, z80ctc_w)
	AM_RANGE(0x1c, 0x1f) AM_MIRROR(0xff00) AM_DEVREADWRITE(Z80KBPIO_TAG, z80pio_alt_r, z80pio_alt_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( xerox820 )
	PORT_START("ROW0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("HELP") PORT_CODE(KEYCODE_TILDE)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('^')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('&')

	PORT_START("ROW1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('*')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR('(')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR(')')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('+')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("BACKSPACE") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad -") PORT_CODE(KEYCODE_MINUS_PAD)

	PORT_START("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 7") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 8") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 9") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB) PORT_CHAR('\t')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')

	PORT_START("ROW3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('}')

	PORT_START("ROW4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad +") PORT_CODE(KEYCODE_PLUS_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 4") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 5") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 6") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')

	PORT_START("ROW5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR(':')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('\'') PORT_CHAR('"')

	PORT_START("ROW6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR('\r')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("LINE FEED") PORT_CODE(KEYCODE_ENTER_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 1") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 2") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 3") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')

	PORT_START("ROW7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')

	PORT_START("ROW8")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x93") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 0") PORT_CODE(KEYCODE_0_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad .") PORT_CODE(KEYCODE_ASTERISK)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START("ROW9")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("LOCK") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("LEFT SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("RIGHT SHIFT") PORT_CODE(KEYCODE_RSHIFT)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("LEFT CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("RIGHT CTRL") PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_MAMEKEY(RCONTROL))
INPUT_PORTS_END

/* Z80 PIO */

static WRITE_LINE_DEVICE_HANDLER( xerox820_pio_pbrdy_w )
{
	xerox820_state *driver_state = device->machine->driver_data;

	driver_state->pbrdy = state;
}

static READ8_DEVICE_HANDLER( xerox820_pio_port_a_r )
{
	/*

        bit     signal          description

        0
        1
        2
        3       PBRDY           keyboard data available
        4       8/N5            8"/5.25" disk select (0=5.25", 1=8")
        5       400/460         double sided disk detect (only on Etch 2 PCB) (0=SS, 1=DS)
        6
        7

    */

	xerox820_state *state = device->machine->driver_data;

	return (state->dsdd << 5) | (state->_8n5 << 4) | (state->pbrdy << 3);
};

static WRITE8_DEVICE_HANDLER( xerox820_pio_port_a_w )
{
	/*

        bit     signal          description

        0       _DVSEL1         drive select 1
        1       _DVSEL2         drive select 2
        2       _DVSEL3         side select
        3
        4
        5
        6       NCSET2          display character set (inverted and connected to chargen A10)
        7       BANK            bank switching (0=RAM, 1=ROM/videoram)

    */

	xerox820_state *state = device->machine->driver_data;

	/* drive select */
	int dvsel1 = BIT(data, 0);
	int dvsel2 = BIT(data, 1);

	if (dvsel1) wd17xx_set_drive(state->wd1771, 0);
	if (dvsel2) wd17xx_set_drive(state->wd1771, 1);

	floppy_drive_set_motor_state(get_floppy_image(device->machine, 0), dvsel1);
	floppy_drive_set_motor_state(get_floppy_image(device->machine, 1), dvsel2);
	floppy_drive_set_ready_state(get_floppy_image(device->machine, 0), dvsel1, 1);
	floppy_drive_set_ready_state(get_floppy_image(device->machine, 1), dvsel2, 1);

	/* side select */
	wd17xx_set_side(state->wd1771, BIT(data, 2));

	/* display character set */
	state->ncset2 = !BIT(data, 6);

	/* bank switching */
	xerox820_bankswitch(device->machine, BIT(data, 7));
};

static READ8_DEVICE_HANDLER( xerox820_pio_port_b_r )
{
	/*

        bit     description

        0       KB0
        1       KB1
        2       KB2
        3       KB3
        4       KB4
        5       KB5
        6       KB6
        7       KB7

    */

	xerox820_state *state = device->machine->driver_data;

	return state->keydata;
};

static const z80pio_interface kbpio_intf =
{
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),	/* callback when change interrupt status */
	DEVCB_HANDLER(xerox820_pio_port_a_r),		/* port A read callback */
	DEVCB_HANDLER(xerox820_pio_port_b_r),		/* port B read callback */
	DEVCB_HANDLER(xerox820_pio_port_a_w),		/* port A write callback */
	DEVCB_NULL,									/* port B write callback */
	DEVCB_NULL,									/* portA ready active callback */
	DEVCB_LINE(xerox820_pio_pbrdy_w)			/* portB ready active callback */
};

static const z80pio_interface gppio_intf =
{
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),	/* callback when change interrupt status */
	DEVCB_NULL,		/* port A read callback */
	DEVCB_NULL,		/* port B read callback */
	DEVCB_NULL,		/* port A write callback */
	DEVCB_NULL,		/* port B write callback */
	DEVCB_NULL,		/* portA ready active callback */
	DEVCB_NULL		/* portB ready active callback */
};

/* Z80 SIO */

static WRITE_LINE_DEVICE_HANDLER( z80daisy_interrupt )
{
	cputag_set_input_line(device->machine, Z80_TAG, INPUT_LINE_IRQ0, state);
}

static const z80sio_interface sio_intf =
{
	z80daisy_interrupt,		/* interrupt handler */
	NULL,				/* DTR changed handler */
	NULL,				/* RTS changed handler */
	NULL,				/* BREAK changed handler */
	NULL,				/* transmit handler */
	NULL				/* receive handler */
};

/* Z80 CTC */

static TIMER_DEVICE_CALLBACK( ctc_tick )
{
	xerox820_state *state = timer->machine->driver_data;

	z80ctc_trg0_w(state->z80ctc, 1);
	z80ctc_trg0_w(state->z80ctc, 0);
}

static WRITE_LINE_DEVICE_HANDLER( ctc_z0_w )
{
//  z80ctc_trg1_w(device, state);
}

static WRITE_LINE_DEVICE_HANDLER( ctc_z2_w )
{
//  z80ctc_trg3_w(device, state);
}

static Z80CTC_INTERFACE( ctc_intf )
{
	0,              			/* timer disables */
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),	/* interrupt handler */
	DEVCB_LINE(ctc_z0_w),		/* ZC/TO0 callback */
	DEVCB_LINE(z80ctc_trg2_w),	/* ZC/TO1 callback */
	DEVCB_LINE(ctc_z2_w)  		/* ZC/TO2 callback */
};

/* Z80 Daisy Chain */

static const z80_daisy_chain xerox820_daisy_chain[] =
{
	{ Z80SIO_TAG },
	{ Z80KBPIO_TAG },
	{ Z80GPPIO_TAG },
	{ Z80CTC_TAG },
	{ NULL }
};

/* WD1771 Interface */

static WRITE_LINE_DEVICE_HANDLER( xerox820_wd1771_intrq_w )
{
	xerox820_state *driver_state = device->machine->driver_data;
	int halt = cpu_get_reg(cputag_get_cpu(device->machine, Z80_TAG), Z80_HALT);

	driver_state->fdc_irq = state;

	if (halt && state)
		cputag_set_input_line(device->machine, Z80_TAG, INPUT_LINE_NMI, ASSERT_LINE);
	else
		cputag_set_input_line(device->machine, Z80_TAG, INPUT_LINE_NMI, CLEAR_LINE);
}

static WRITE_LINE_DEVICE_HANDLER( xerox820_wd1771_drq_w )
{
	xerox820_state *driver_state = device->machine->driver_data;
	int halt = cpu_get_reg(cputag_get_cpu(device->machine, Z80_TAG), Z80_HALT);

	driver_state->fdc_drq = state;

	if (halt && state)
		cputag_set_input_line(device->machine, Z80_TAG, INPUT_LINE_NMI, ASSERT_LINE);
	else
		cputag_set_input_line(device->machine, Z80_TAG, INPUT_LINE_NMI, CLEAR_LINE);
}

static const wd17xx_interface wd1771_intf =
{
	DEVCB_LINE(xerox820_wd1771_intrq_w),
	DEVCB_LINE(xerox820_wd1771_drq_w),
	{FLOPPY_0, FLOPPY_1, NULL, NULL}
};

/* COM8116 Interface */

static COM8116_INTERFACE( com8116_intf )
{
	DEVCB_NULL,		/* fX/4 output */
	DEVCB_NULL,		/* fR output */
	DEVCB_NULL,		/* fT output */
	{ 101376, 67584, 46080, 37686, 33792, 16896, 8448, 4224, 2816, 2534, 2112, 1408, 1056, 704, 528, 264 },			/* receiver divisor ROM */
	{ 101376, 67584, 46080, 37686, 33792, 16896, 8448, 4224, 2816, 2534, 2112, 1408, 1056, 704, 528, 264 },			/* transmitter divisor ROM */
};

/* Video */

static VIDEO_START( xerox820 )
{
	xerox820_state *state = machine->driver_data;

	/* find memory regions */
	state->char_rom = memory_region(machine, "chargen");
}

static VIDEO_UPDATE( xerox820 )
{
	xerox820_state *state = screen->machine->driver_data;

	static UINT8 framecnt=0;
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=(state->scroll + 1) * 0x80,x;

	framecnt++;

	for (y = 0; y < 24; y++)
	{
		if (ma > 0xb80) ma = 0;

		for (ra = 0; ra < 10; ra++)
		{
			UINT16 *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = ma; x < ma + 80; x++)
			{
				if (ra < 8)
				{
					chr = state->video_ram[x & XEROX820_LCD_VIDEORAM_MASK] ^ 0x80;

					/* Take care of flashing characters */
					if ((chr < 0x80) && (framecnt & 0x08))
						chr |= 0x80;

					/* get pattern of pixels for that character scanline */
					gfx = state->char_rom[(state->ncset2 << 10) | (chr<<3) | ra ];
				}
				else
					gfx = 0xff;

				/* Display a scanline of a character (7 pixels) */
				*p = 0; p++;
				*p = ( gfx & 0x10 ) ? 0 : 1; p++;
				*p = ( gfx & 0x08 ) ? 0 : 1; p++;
				*p = ( gfx & 0x04 ) ? 0 : 1; p++;
				*p = ( gfx & 0x02 ) ? 0 : 1; p++;
				*p = ( gfx & 0x01 ) ? 0 : 1; p++;
				*p = 0; p++;
			}
		}
		ma+=128;
	}
	return 0;
}
static void xerox820_load_proc(const device_config *image)
{
	xerox820_state *state = image->machine->driver_data;

	switch (image_length(image))
	{
	case 77*1*26*128: // 250K 8" SSSD
		state->_8n5 = 1;
		state->dsdd = 0;
		return;

	case 77*1*26*256: // 500K 8" SSDD
		state->_8n5 = 1;
		state->dsdd = 0;
		return;

	case 40*1*18*128: // 90K 5.25" SSSD
		state->_8n5 = 0;
		state->dsdd = 0;
		return;

	case 40*2*18*128: // 180K 5.25" DSSD
		state->_8n5 = 0;
		state->dsdd = 1;
		return;
	}

}

/* Machine Initialization */

static MACHINE_START( xerox820 )
{
	int drive;
	xerox820_state *state = machine->driver_data;

	/* find devices */
	state->kbpio = devtag_get_device(machine, Z80KBPIO_TAG);
	state->z80ctc = devtag_get_device(machine, Z80CTC_TAG);
	state->wd1771 = devtag_get_device(machine, WD1771_TAG);

	/* allocate video memory */
	state->video_ram = auto_alloc_array(machine, UINT8, XEROX820_LCD_VIDEORAM_SIZE);

	/* setup memory banking */
	memory_configure_bank(machine, 1, 0, 1, memory_region(machine, Z80_TAG), 0);
	memory_configure_bank(machine, 1, 1, 1, memory_region(machine, "monitor"), 0);

	memory_configure_bank(machine, 2, 0, 1, memory_region(machine, Z80_TAG) + 0x1000, 0);
	memory_set_bank(machine, 2, 0);

	memory_configure_bank(machine, 3, 0, 1, memory_region(machine, Z80_TAG) + 0x3000, 0);
	memory_configure_bank(machine, 3, 1, 1, state->video_ram, 0);

	/* bank switch */
	xerox820_bankswitch(machine, 1);

	for(drive=0;drive<2;drive++)
	{
		floppy_install_load_proc(floppy_get_device(machine, drive), xerox820_load_proc);
	}


	/* register for state saving */
	state_save_register_global_pointer(machine, state->video_ram, XEROX820_LCD_VIDEORAM_SIZE);
	state_save_register_global(machine, state->pbrdy);
	state_save_register_global(machine, state->keydata);
	state_save_register_global(machine, state->scroll);
	state_save_register_global(machine, state->ncset2);
	state_save_register_global(machine, state->vatt);
	state_save_register_global(machine, state->fdc_irq);
	state_save_register_global(machine, state->fdc_drq);
	state_save_register_global(machine, state->_8n5);
	state_save_register_global(machine, state->dsdd);
}

static FLOPPY_OPTIONS_START(xerox820 )
	FLOPPY_OPTION( sssd8, "dsk", "8\" SSSD", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([77])
		SECTORS([26])
		SECTOR_LENGTH([128])
		FIRST_SECTOR_ID([1]))
	FLOPPY_OPTION( ssdd8, "dsk", "8\" SSDD", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([77])
		SECTORS([26])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
	FLOPPY_OPTION( sssd5, "dsk", "5.25\" SSSD", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([40])
		SECTORS([18])
		SECTOR_LENGTH([128])
		FIRST_SECTOR_ID([1]))
	FLOPPY_OPTION( ssdd5, "dsk", "5.25\" SSDD", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([40])
		SECTORS([18])
		SECTOR_LENGTH([128])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

static const floppy_config xerox820_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(xerox820),
	DO_NOT_KEEP_GEOMETRY
};

/* Machine Drivers */

static MACHINE_DRIVER_START( xerox820 )
	MDRV_DRIVER_DATA(xerox820_state)

    /* basic machine hardware */
    MDRV_CPU_ADD(Z80_TAG, Z80, XTAL_20MHz/8)
    MDRV_CPU_PROGRAM_MAP(xerox820_mem)
    MDRV_CPU_IO_MAP(xerox820_io)
	MDRV_CPU_CONFIG(xerox820_daisy_chain)

    MDRV_MACHINE_START(xerox820)

    /* video hardware */
    MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(XTAL_10_69425MHz, 700, 0, 560, 260, 0, 240)

	MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(xerox820)
    MDRV_VIDEO_UPDATE(xerox820)

	/* keyboard */
	MDRV_TIMER_ADD_PERIODIC("keyboard", xerox820_keyboard_tick, HZ(60))
	MDRV_TIMER_ADD_PERIODIC("ctc", ctc_tick, HZ(XTAL_20MHz/8))

	/* devices */
	MDRV_Z80SIO_ADD(Z80SIO_TAG, XTAL_20MHz/8, sio_intf)
	MDRV_Z80PIO_ADD(Z80KBPIO_TAG, kbpio_intf)
	MDRV_Z80PIO_ADD(Z80GPPIO_TAG, gppio_intf)
	MDRV_Z80CTC_ADD(Z80CTC_TAG, XTAL_20MHz/8, ctc_intf)
	MDRV_WD1771_ADD(WD1771_TAG, wd1771_intf)
	MDRV_FLOPPY_2_DRIVES_ADD(xerox820_floppy_config)
	MDRV_COM8116_ADD(COM8116_TAG, XTAL_5_0688MHz, com8116_intf)
	
	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("64K")	
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( xerox820ii )
//  MDRV_CPU_ADD(Z80_TAG, Z80, XTAL_16MHz/4)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( xerox820 )
	ROM_REGION( 0x10000, Z80_TAG, ROMREGION_ERASE00 )

	ROM_REGION( 0x1000, "monitor", 0 )
	ROM_DEFAULT_BIOS( "v20" )
	ROM_SYSTEM_BIOS( 0, "v10", "Xerox Monitor v1.0" )
	ROMX_LOAD( "x820v10.u64", 0x0000, 0x0800, NO_DUMP, ROM_BIOS(1) )
	ROMX_LOAD( "x820v10.u63", 0x0800, 0x0800, NO_DUMP, ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "v20", "Xerox Monitor v2.0" )
	ROMX_LOAD( "x820v20.u64", 0x0000, 0x0800, CRC(2fc227e2) SHA1(b4ea0ae23d281a687956e8a514cb364a1372678e), ROM_BIOS(2) )
	ROMX_LOAD( "x820v20.u63", 0x0800, 0x0800, CRC(bc11f834) SHA1(4fd2b209a6e6ff9b0c41800eb5228c34a0d7f7ef), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "smart23", "MICROCode SmartROM v2.3" )
	ROMX_LOAD( "mxkx25a.u64", 0x0000, 0x0800, CRC(7ec5f100) SHA1(5d0ff35a51aa18afc0d9c20ef99ff5d9d3f2075b), ROM_BIOS(3) )
	ROMX_LOAD( "mxkx25b.u63", 0x0800, 0x0800, CRC(a7543798) SHA1(886e617e1003d13f86f33085cbd49391b77291a3), ROM_BIOS(3) )
	ROM_SYSTEM_BIOS( 3, "plus2", "MICROCode Plus2 v0.2a" )
	ROMX_LOAD( "p2x25a.u64",  0x0000, 0x0800, CRC(3ccd7a8f) SHA1(6e46c88f03fc7289595dd6bec95e23bb13969525), ROM_BIOS(4) )
	ROMX_LOAD( "p2x25b.u63",  0x0800, 0x0800, CRC(1e580391) SHA1(e91f8ce82586df33c0d6d02eb005e8079f4de67d), ROM_BIOS(4) )

	ROM_REGION( 0x800, "chargen", 0 )
	ROM_LOAD( "x820.u92", 0x0000, 0x0800, CRC(b823fa98) SHA1(ad0ea346aa257a53ad5701f4201896a2b3a0f928) )
ROM_END

ROM_START( xerox820ii )
	ROM_REGION( 0x10000, Z80_TAG, ROMREGION_ERASE00 )

	ROM_REGION( 0x2000, "monitor", 0 )
	ROM_LOAD( "x820ii.u33", 0x0000, 0x0800, NO_DUMP )
	ROM_LOAD( "x820ii.u34", 0x0800, 0x0800, NO_DUMP )
	ROM_LOAD( "x820ii.u35", 0x1000, 0x0800, NO_DUMP )
	ROM_LOAD( "x820ii.u36", 0x1800, 0x0800, NO_DUMP )

	ROM_REGION( 0x800, "chargen", 0 )
	ROM_LOAD( "x820ii.u58", 0x0000, 0x0800, NO_DUMP )
ROM_END

/* System Drivers */

/*    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT       INIT    CONFIG      COMPANY                         FULLNAME        FLAGS */
COMP( 1981, xerox820,	0,			0,		xerox820,	xerox820,	0,		0,	"Xerox",						"Xerox 820",	0)
/*
COMP( 1980, bigboard,   0,          0,      bigboard,   bigboard,   0,      bigboard,   "Digital Research Computers",   "Big Board",    GAME_NOT_WORKING)
COMP( 1983, bigbord2,   0,          0,      bigbord2,   bigboard,   0,      bigbord2,   "Digital Research Computers",   "Big Board II", GAME_NOT_WORKING)
COMP( 1983, xerox820ii, 0,          0,      xerox820ii, xerox820,   0,      xerox820ii, "Xerox",                        "Xerox 820-II", GAME_NOT_WORKING)
COMP( 1983, xerox168,   0,          0,      xerox168,   xerox168,   0,      xerox168,   "Xerox",                        "Xerox 16/8",   GAME_NOT_WORKING)
*/
