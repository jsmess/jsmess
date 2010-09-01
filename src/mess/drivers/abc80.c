/*

Luxor ABC 80

PCB Layout
----------

          CN1               CN2                                                CN5
  SW1   |-----|  |------------------------|                                  |-----|
|-------|-----|--|------------------------|--------------------------------------------|
|                                                             CN3       CN4            |
|                                                    7912                              |
|            MC1488                                                                    |
|   MC1489                                           7812                              |
|            LS245                     8205                                            |
|                                                   |-------|                          |
|   |-----CN6-----|   LS241   LS241    8205   LS32  |SN76477| LS04    LM339            |
|                                                   |-------|                          |
|   |--------------|  |------------|   PROM0  LS132   LS273   7406             LS08    |
|   |   Z80A PIO   |  |    Z80A    |                                                   |
|   |--------------|  |------------|   LS04   LS74A   LS86    LS161   LS166    74393   |
|                                                                                      |
|     ROM3   LS107    4116    4116     LS10   LS257   LS74A   LS08    LS107    PROM2   |
|                                                                                      |
|     ROM2   LS257    4116    4116     LS139  74393   LS107   LS32    LS175    74393   |
|                                                                                      |
|     ROM1   LS257    4116    4116     LS08   LS283   LS10    LS32    PROM1    74393   |
|                                                                                      |
|     ROM0   LS257    4116    4116     LS257  74393   LS375   74S263  LS145    PROM4   |
|                                                                                      |
|     DIPSW1 DIPSW2   4045    4045     LS257  LS245   LS375   LS273   LS166    PROM3   |
|--------------------------------------------------------------------------------------|

Notes:
    All IC's shown.

    ROM0-3  - Texas Instruments TMS4732 4Kx8 General Purpose Mask Programmable ROM
    PROM0-2 - Philips Semiconductors N82S129 256x4 TTL Bipolar PROM
    PROM3-4 - Philips Semiconductors N82S131 512x4 TTL Bipolar PROM
    4116    - Texas Instruments TMS4116-25 16Kx1 Dynamic RAM
    4045    - Texas Instruments TMS4045-15 1Kx4 General Purpose Static RAM with Multiplexed I/O
    Z80A    -
    Z80APIO -
    SN76477 - Texas Instruments SN76477N Complex Sound Generator
    74S263  - Texas Instruments SN74S263N Row Output Character Generator
    MC1488  - Texas Instruments MC1488 Quadruple Line Driver
    MC1489  - Texas Instruments MC1489 Quadruple Line Receiver
    8205    - ?
    CN1     - RS-232 connector
    CN2     - ABC bus connector (DIN 41612)
    CN3     - video connector
    CN4     - cassette motor connector
    CN5     - cassette connector
    CN6     - keyboard connector
    SW1     - reset switch
    DIPSW1  -
    DIPSW2  -

*/

/*

    TODO:

    - memory bank switching using ABC80/13 PROM
    - proper keyboard controller emulation
    - get BASIC v1 dump
    - MyAB 80-column card
    - GeJo 80-column card
    - Mikrodatorn 64K expansion
    - floppy
    - printer
    - IEC

*/

/* Core includes */
#include "emu.h"
#include "cpu/z80/z80.h"
#include "includes/abc80.h"

/* Components */
#include "cpu/z80/z80daisy.h"
#include "machine/z80pio.h"
#include "sound/sn76477.h"
#include "machine/abcbus.h"
#include "machine/conkort.h"

/* Devices */
#include "devices/flopdrv.h"
#include "devices/cassette.h"
#include "devices/printer.h"
#include "devices/messram.h"

/* Read/Write Handlers */

static WRITE8_DEVICE_HANDLER( abc80_sound_w )
{
	/*

        Bit Name     Description

        0  SYSENA   1 On, 0 Off (inverted)
        1  EXTVCO   00 High freq, 01 Low freq
        2  VCOSEL   10 SLF cntrl, 11 SLF ctrl
        3  MIXSELB  000 VCO, 001 Noise, 010 SLF
        4  MIXSELA  011 VCO+Noise, 100 SLF+Noise, 101 SLF+VCO
        5  MIXSELC  110 SLF+VCO+Noise, 111 Quiet
        6  ENVSEL2  00 VCO, 01 Rakt igenom
        7  ENVSEL1  10 Monovippa, 11 VCO alt.pol.

    */

	sn76477_enable_w(device, !BIT(data, 0));

	sn76477_vco_voltage_w(device, BIT(data, 1) ? 2.5 : 0);
	sn76477_vco_w(device, BIT(data, 2));

	sn76477_mixer_b_w(device, BIT(data, 3));
	sn76477_mixer_a_w(device, BIT(data, 4));
	sn76477_mixer_c_w(device, BIT(data, 5));

	sn76477_envelope_2_w(device, BIT(data, 6));
	sn76477_envelope_1_w(device, BIT(data, 7));
}

/* Keyboard HACK */

static const UINT8 abc80_keycodes[7*4][8] =
{
	/* unshifted */
	{ 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38 },
	{ 0x39, 0x30, 0x2B, 0x60, 0x3C, 0x71, 0x77, 0x65 },
	{ 0x72, 0x74, 0x79, 0x75, 0x69, 0x6F, 0x70, 0x7D },
	{ 0x7E, 0x0D, 0x61, 0x73, 0x64, 0x66, 0x67, 0x68 },
	{ 0x6A, 0x6B, 0x6C, 0x7C, 0x7B, 0x27, 0x08, 0x7A },
	{ 0x78, 0x63, 0x76, 0x62, 0x6E, 0x6D, 0x2C, 0x2E },
	{ 0x2D, 0x09, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00 },

	/* shift */
	{ 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x2f, 0x28 },
	{ 0x29, 0x3d, 0x3f, 0x40, 0x3e, 0x51, 0x57, 0x45 },
	{ 0x52, 0x54, 0x59, 0x55, 0x49, 0x4f, 0x50, 0x5d },
	{ 0x5e, 0x0d, 0x41, 0x53, 0x44, 0x46, 0x47, 0x48 },
	{ 0x4a, 0x4b, 0x4c, 0x5c, 0x5b, 0x2a, 0x08, 0x5a },
	{ 0x58, 0x43, 0x56, 0x42, 0x4e, 0x4d, 0x3b, 0x3a },
	{ 0x5f, 0x09, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00 },

	/* control */
	{ 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38 },
	{ 0x39, 0x30, 0x2b, 0x00, 0x7f, 0x11, 0x17, 0x05 },
	{ 0x12, 0x14, 0x19, 0x15, 0x09, 0x0f, 0x10, 0x1d },
	{ 0x1e, 0x0d, 0x01, 0x13, 0x04, 0x06, 0x07, 0x08 },
	{ 0x0a, 0x0b, 0x0c, 0x1c, 0x1b, 0x27, 0x08, 0x1a },
	{ 0x18, 0x03, 0x16, 0x02, 0x0e, 0x0d, 0x2c, 0x2e },
	{ 0x2d, 0x09, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00 },

	/* control-shift */
	{ 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x2f, 0x28 },
	{ 0x29, 0x3d, 0x3f, 0x00, 0x7f, 0x11, 0x17, 0x05 },
	{ 0x12, 0x14, 0x19, 0x15, 0x09, 0x1f, 0x00, 0x1d },
	{ 0x1e, 0x0d, 0x01, 0x13, 0x04, 0x06, 0x07, 0x08 },
	{ 0x0a, 0x1b, 0x1c, 0x1c, 0x1b, 0x2a, 0x08, 0x1a },
	{ 0x18, 0x03, 0x16, 0x02, 0x1e, 0x1d, 0x3b, 0x3a },
	{ 0x5f, 0x09, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

static TIMER_CALLBACK( keyboard_data_clear )
{
	abc80_state *state = machine->driver_data<abc80_state>();

	state->key_data = 0;
}

static void abc80_keyboard_scan(running_machine *machine)
{
	abc80_state *state = machine->driver_data<abc80_state>();

	static const char *const keynames[] = { "ROW0", "ROW1", "ROW2", "ROW3", "ROW4", "ROW5", "ROW6" };
	int table = 0, row, col;

	if (input_port_read(machine, "ROW7") & 0x07)
	{
		/* shift, upper case */
		table |= 0x01;
	}

	if (input_port_read(machine, "ROW7") & 0x08)
	{
		/* ctrl */
		table |= 0x02;
	}

	/* clear key strobe */
	state->key_strobe = 0;

	/* scan keyboard */
	for (row = 0; row < 7; row++)
	{
		UINT8 data = input_port_read(machine, keynames[row]);

		for (col = 0; col < 8; col++)
		{
			if (BIT(data, col))
			{
				UINT8 keydata = abc80_keycodes[row + (table * 7)][col];

				/* set key strobe */
				state->key_strobe = 1;

				if (state->key_data != keydata)
				{
					UINT8 pio_data = 0x80 | keydata;

					/* latch key data */
					state->key_data = keydata;

					z80pio_pa_w(state->z80pio, 0, pio_data);
					return;
				}
			}
		}
	}

	if (!state->key_strobe && state->key_data)
	{
		z80pio_pa_w(state->z80pio, 0, state->key_data);
		timer_set(machine, ATTOTIME_IN_MSEC(50), NULL, 0, keyboard_data_clear);
	}
}

static TIMER_DEVICE_CALLBACK( abc80_keyboard_tick )
{
	abc80_keyboard_scan(timer.machine);
}

/* Memory Maps */

static ADDRESS_MAP_START( abc80_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x6000, 0x6fff) AM_ROM
	AM_RANGE(0x7000, 0x73ff) AM_ROM
	AM_RANGE(0x7400, 0x77ff) AM_RAM AM_BASE_MEMBER(abc80_state, video_80_ram)
	AM_RANGE(0x7800, 0x7bff) AM_ROM
	AM_RANGE(0x7c00, 0x7fff) AM_RAM AM_BASE_MEMBER(abc80_state, video_ram)
	AM_RANGE(0x8000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( abc80_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0x17)
	AM_RANGE(0x00, 0x00) AM_DEVREADWRITE(ABCBUS_TAG, abcbus_inp_r, abcbus_utp_w)
	AM_RANGE(0x01, 0x01) AM_DEVREADWRITE(ABCBUS_TAG, abcbus_stat_r, abcbus_cs_w)
	AM_RANGE(0x02, 0x02) AM_DEVWRITE(ABCBUS_TAG, abcbus_c1_w)
	AM_RANGE(0x03, 0x03) AM_DEVWRITE(ABCBUS_TAG, abcbus_c2_w)
	AM_RANGE(0x04, 0x04) AM_DEVWRITE(ABCBUS_TAG, abcbus_c3_w)
	AM_RANGE(0x05, 0x05) AM_DEVWRITE(ABCBUS_TAG, abcbus_c4_w)
	AM_RANGE(0x06, 0x06) AM_DEVWRITE(SN76477_TAG, abc80_sound_w)
	AM_RANGE(0x07, 0x07) AM_DEVREAD(ABCBUS_TAG, abcbus_rst_r)
	AM_RANGE(0x10, 0x13) AM_MIRROR(0x04) AM_DEVREADWRITE(Z80PIO_TAG, z80pio_ba_cd_r, z80pio_ba_cd_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( abc80 )
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
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("RIGHT SHIFT") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("UPPER CASE") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK)) PORT_TOGGLE
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

/* Sound Interface */

static const sn76477_interface abc80_sn76477_interface =
{
	RES_K(47),		//  4  noise_res        R26 47k
	RES_K(330),		//  5  filter_res       R24 330k
	CAP_P(390),		//  6  filter_cap       C52 390p
	RES_K(47),		//  7  decay_res        R23 47k
	CAP_U(10),		//  8  attack_decay_cap C50 10u/35V
	RES_K(2.2),		// 10  attack_res       R21 2.2k
	RES_K(33),		// 11  amplitude_res    R19 33k
	RES_K(10),		// 12  feedback_res     R18 10k
	0,				// 16  vco_voltage      0V or 2.5V
	CAP_N(10) ,		// 17  vco_cap          C48 10n
	RES_K(100),		// 18  vco_res          R20 100k
	0,				// 19  pitch_voltage    N/C
	RES_K(220),		// 20  slf_res          R22 220k
	CAP_U(1),		// 21  slf_cap          C51 1u/35V
	CAP_U(0.1),		// 23  oneshot_cap      C53 0.1u
	RES_K(330)		// 24  oneshot_res      R25 330k
};

/* Interrupt Generators */

static INTERRUPT_GEN( abc80_nmi_interrupt )
{
	cputag_set_input_line(device->machine, Z80_TAG, INPUT_LINE_NMI, PULSE_LINE);
}

/* Z80 PIO */

static TIMER_DEVICE_CALLBACK( z80pio_astb_tick )
{
	abc80_state *state = timer.machine->driver_data<abc80_state>();

	/* toggle ASTB every other video line */
	state->z80pio_astb = !state->z80pio_astb;

	z80pio_astb_w(state->z80pio, state->z80pio_astb);
}

static READ8_DEVICE_HANDLER( pio_pa_r )
{
	/*

        PIO Port A

        bit     description

        0       keyboard data
        1       keyboard data
        2       keyboard data
        3       keyboard data
        4       keyboard data
        5       keyboard data
        6       keyboard data
        7       keyboard strobe

    */

	abc80_state *state = device->machine->driver_data<abc80_state>();

	return (state->key_strobe << 7) | state->key_data;
};

static READ8_DEVICE_HANDLER( pio_pb_r )
{
	/*

        PIO Channel B

        0  R    RS-232C RxD
        1  R    RS-232C _CTS
        2  R    RS-232C _DCD
        3  W    RS-232C TxD
        4  W    RS-232C _RTS
        5  W    Cassette Motor
        6  W    Cassette Data
        7  R    Cassette Data

    */

	/* cassette data */
	UINT8 data = (cassette_input(device) > +1.0) << 7;

	return data;
};

static WRITE8_DEVICE_HANDLER( pio_pb_w )
{
	/*

        PIO Channel B

        0  R    RS-232C RxD
        1  R    RS-232C _CTS
        2  R    RS-232C _DCD
        3  W    RS-232C TxD
        4  W    RS-232C _RTS
        5  W    Cassette Motor
        6  W    Cassette Data
        7  R    Cassette Data

    */

	/* cassette motor */
	cassette_change_state(device, BIT(data, 5) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);

	/* cassette data */
	cassette_output(device, BIT(data, 6) ? -1.0 : +1.0);
};

static Z80PIO_INTERFACE( pio_intf )
{
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),	/* callback when change interrupt status */
	DEVCB_HANDLER(pio_pa_r),						/* port A read callback */
	DEVCB_NULL,										/* port A write callback */
	DEVCB_NULL,										/* portA ready active callback */
	DEVCB_DEVICE_HANDLER(CASSETTE_TAG, pio_pb_r),	/* port B read callback */
	DEVCB_DEVICE_HANDLER(CASSETTE_TAG, pio_pb_w),	/* port B write callback */
	DEVCB_NULL										/* portB ready active callback */
};

static const z80_daisy_config abc80_daisy_chain[] =
{
	{ Z80PIO_TAG },
	{ NULL }
};

/* Cassette */

static const cassette_config abc80_cassette_config =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED | CASSETTE_MOTOR_DISABLED | CASSETTE_SPEAKER_MUTED),
	NULL
};

/* ABC BUS */

static ABCBUS_DAISY( abcbus_daisy )
{
	{ LUXOR_55_10828_ABCBUS("abc830") },
	{ NULL }
};

/* Machine Initialization */

static MACHINE_START( abc80 )
{
	abc80_state *state = machine->driver_data<abc80_state>();

	/* configure RAM expansion */
	if (messram_get_size(machine->device("messram")) == 16*1024)
	{
		memory_unmap_readwrite(cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM), 0x8000, 0xbfff, 0, 0);
	}

	/* find devices */
	state->z80pio = machine->device(Z80PIO_TAG);

	/* register for state saving */
	state_save_register_global(machine, state->key_data);
	state_save_register_global(machine, state->key_strobe);
	state_save_register_global(machine, state->z80pio_astb);
}

/* Machine Drivers */

static MACHINE_CONFIG_START( abc80, abc80_state )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80_TAG, Z80, ABC80_XTAL/2/2)	// 2.9952 MHz
	MDRV_CPU_PROGRAM_MAP(abc80_map)
	MDRV_CPU_IO_MAP(abc80_io_map)
	MDRV_CPU_CONFIG(abc80_daisy_chain)
	MDRV_CPU_VBLANK_INT(SCREEN_TAG, abc80_nmi_interrupt)

	MDRV_MACHINE_START(abc80)

	/* keyboard */
	MDRV_TIMER_ADD_PERIODIC("keyboard", abc80_keyboard_tick, USEC(2500))

	/* Z80PIO */
	MDRV_TIMER_ADD_SCANLINE("pio_astb", z80pio_astb_tick, SCREEN_TAG, 0, 1)
	MDRV_Z80PIO_ADD(Z80PIO_TAG, ABC80_XTAL/2/2, pio_intf)

	/* Luxor Conkort 55-10828 */
	MDRV_ABCBUS_ADD(ABCBUS_TAG, abcbus_daisy)
	MDRV_LUXOR_55_10828_ADD("abc830")

	/* video hardware */
	MDRV_FRAGMENT_ADD(abc80_video)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SN76477_TAG, SN76477, 0)
	MDRV_SOUND_CONFIG(abc80_sn76477_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* printer */
	MDRV_PRINTER_ADD("printer")

	/* cassette */
	MDRV_CASSETTE_ADD(CASSETTE_TAG, abc80_cassette_config)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("16K")
	MDRV_RAM_EXTRA_OPTIONS("32K")
MACHINE_CONFIG_END

/* ROMs */

ROM_START( abc80 )
	ROM_REGION( 0x10000, Z80_TAG, 0 )
	ROM_LOAD( "za3508.a2", 0x0000, 0x1000, CRC(e2afbf48) SHA1(9883396edd334835a844dcaa792d29599a8c67b9) )
	ROM_LOAD( "za3509.a3", 0x1000, 0x1000, CRC(d224412a) SHA1(30968054bba7c2aecb4d54864b75a446c1b8fdb1) )
	ROM_LOAD( "za3506.a4", 0x2000, 0x1000, CRC(1502ba5b) SHA1(5df45909c2c4296e5701c6c99dfaa9b10b3a729b) )
	ROM_LOAD( "za3507.a5", 0x3000, 0x1000, CRC(bc8860b7) SHA1(28b6cf7f5a4f81e017c2af091c3719657f981710) )
	ROM_SYSTEM_BIOS( 0, "default", "No DOS" )
	ROM_SYSTEM_BIOS( 1, "abcdos", "ABC-DOS" ) // Scandia Metric FD2
	ROMX_LOAD("abcdos",    0x6000, 0x1000, CRC(2cb2192f) SHA1(a6b3a9587714f8db807c05bee6c71c0684363744), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "abcdosdd", "ABC-DOS DD" ) // ABC 830
	ROMX_LOAD("abcdosdd",  0x6000, 0x1000, CRC(36db4c15) SHA1(ae462633f3a9c142bb029beb14749a84681377fa), ROM_BIOS(3) )
	ROM_SYSTEM_BIOS( 3, "ufd20", "UFD-DOS v.20" ) // ABC 830
	ROMX_LOAD("ufddos20",  0x6000, 0x1000, CRC(69b09c0b) SHA1(403997a06cf6495b8fa13dc74eff6a64ef7aa53e), ROM_BIOS(4) )
	ROM_LOAD( "iec",	   0x7000, 0x0400, NO_DUMP )
	ROM_LOAD( "printer",   0x7800, 0x0400, NO_DUMP )

	ROM_REGION( 0x10000, "keyboard", 0 )
	ROM_LOAD( "keyboard.rom", 0x0000, 0x1000, NO_DUMP )

	ROM_REGION( 0xa00, "chargen", 0 )
	ROM_LOAD( "sn74s263.h2", 0x0000, 0x0a00, BAD_DUMP CRC(9e064e91) SHA1(354783c8f2865f73dc55918c9810c66f3aca751f) ) // created by hand

	ROM_REGION( 0x80, "hsync", 0 )
	ROM_LOAD( "abc80_11.k5", 0x0000, 0x0080, NO_DUMP ) // "64 40029-01" 82S129 256x4 horizontal sync

	ROM_REGION( 0x100, "vsync", 0 )
	ROM_LOAD( "abc80_21.k2", 0x0000, 0x0100, NO_DUMP ) // "64 40030-01" 82S131 512x4 vertical sync

	ROM_REGION( 0x80, "attr", 0 )
	ROM_LOAD( "abc80_12.j3", 0x0000, 0x0080, NO_DUMP ) // "64 40056-01" 82S129 256x4 attribute

	ROM_REGION( 0x100, "line", 0 )
	ROM_LOAD( "abc80_22.k1", 0x0000, 0x0100, NO_DUMP ) // "64 40058-01" 82S131 512x4 chargen 74S263 row address

	ROM_REGION( 0x80, "mmu", 0 )
	ROM_LOAD( "abc80_13.e7", 0x0000, 0x0080, NO_DUMP ) // "64 40057-01" 82S129 256x4 address decoder
ROM_END

ROM_START( abc80h )
	ROM_REGION( 0x10000, Z80_TAG, 0 )
	ROM_LOAD( "za3508.a2", 0x0000, 0x1000, CRC(e2afbf48) SHA1(9883396edd334835a844dcaa792d29599a8c67b9) )
	ROM_LOAD( "za3509.a3", 0x1000, 0x1000, CRC(d224412a) SHA1(30968054bba7c2aecb4d54864b75a446c1b8fdb1) )
	ROM_LOAD( "za3506.a4", 0x2000, 0x1000, CRC(1502ba5b) SHA1(5df45909c2c4296e5701c6c99dfaa9b10b3a729b) )
	ROM_LOAD( "za3507.a5", 0x3000, 0x1000, CRC(bc8860b7) SHA1(28b6cf7f5a4f81e017c2af091c3719657f981710) )
	ROM_SYSTEM_BIOS( 0, "default", "No DOS" )
	ROM_SYSTEM_BIOS( 1, "abcdos", "ABC-DOS" ) // Scandia Metric FD2
	ROMX_LOAD("abcdos",    0x6000, 0x1000, CRC(2cb2192f) SHA1(a6b3a9587714f8db807c05bee6c71c0684363744), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "abcdosdd", "ABC-DOS DD" ) // ABC 830
	ROMX_LOAD("abcdosdd",  0x6000, 0x1000, CRC(36db4c15) SHA1(ae462633f3a9c142bb029beb14749a84681377fa), ROM_BIOS(3) )
	ROM_SYSTEM_BIOS( 3, "ufd20", "UFD-DOS v.20" ) // ABC 830
	ROMX_LOAD("ufddos20",  0x6000, 0x1000, CRC(69b09c0b) SHA1(403997a06cf6495b8fa13dc74eff6a64ef7aa53e), ROM_BIOS(4) )
	ROM_LOAD( "iec",	   0x7000, 0x0400, NO_DUMP )
	ROM_LOAD( "printer",   0x7800, 0x0400, NO_DUMP )

	ROM_REGION( 0x10000, "keyboard", 0 )
	ROM_LOAD( "keyboard.rom", 0x0000, 0x1000, NO_DUMP )

	ROM_REGION( 0xa00, "chargen", 0 )
	ROM_LOAD( "sn74s262.h2", 0x0000, 0x0a00, NO_DUMP ) // UK charset

	ROM_REGION( 0x80, "hsync", 0 )
	ROM_LOAD( "abc80_11.k5", 0x0000, 0x0080, NO_DUMP ) // "64 40029-01" 82S129 256x4 horizontal sync

	ROM_REGION( 0x100, "vsync", 0 )
	ROM_LOAD( "abc80_21.k2", 0x0000, 0x0100, NO_DUMP ) // "64 40030-01" 82S131 512x4 vertical sync

	ROM_REGION( 0x80, "attr", 0 )
	ROM_LOAD( "abc80_12.j3", 0x0000, 0x0080, NO_DUMP ) // "64 40056-01" 82S129 256x4 attribute

	ROM_REGION( 0x100, "line", 0 )
	ROM_LOAD( "abc80_22.k1", 0x0000, 0x0100, NO_DUMP ) // "64 40058-01" 82S131 512x4 chargen 74S263 row address

	ROM_REGION( 0x80, "mmu", 0 )
	ROM_LOAD( "abc80_13.e7", 0x0000, 0x0080, NO_DUMP ) // "64 40057-01" 82S129 256x4 address decoder
ROM_END

/* Drivers */

/*    YEAR  NAME    PARENT  COMPAT  MACHINE INPUT   INIT    COMPANY                             FULLNAME                    FLAGS */
COMP( 1978, abc80,  0,      0,      abc80,  abc80,  0,      "Luxor Datorer AB",					"ABC 80 (Sweden, Finland)",	GAME_SUPPORTS_SAVE )
COMP( 1978, abc80h, abc80,  0,      abc80,  abc80,  0,      "Budapesti Radiotechnikai Gyar",	"ABC 80 (Hungary)",			GAME_SUPPORTS_SAVE )
