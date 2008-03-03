/*

Luxor ABC 80

PCB Layout
----------

          CN1               CN2                                                CN5
  SW1   |-----|  |------------------------|                                  |-----|
|-------|-----|--|------------------------|--------------------------------------------|
|                                                             CN3       CN4            |
|                                                                                      |
|            MC1488                                                                    |
|   MC1489                                                                             |
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

    - keyboard scanning is awkwardly slow
    - cassette interface
    - floppy
    - printer
    - IEC

*/

/* Core includes */
#include "driver.h"
#include "includes/abc80.h"

/* Components */
#include "machine/centroni.h"
#include "cpu/z80/z80daisy.h"
#include "machine/z80pio.h"
#include "sound/sn76477.h"
#include "machine/abcbus.h"

/* Devices */
#include "devices/basicdsk.h"
#include "devices/cassette.h"
#include "devices/printer.h"


static emu_timer *abc80_keyboard_timer;

/* Read/Write Handlers */

// Sound

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

static WRITE8_HANDLER( abc80_sound_w )
{
	SN76477_enable_w(0, ~data & 0x01);

	SN76477_vco_voltage_w(0, (data & 0x02) ? 2.5 : 0);
	SN76477_vco_w(0, (data & 0x04) ? 1 : 0);

	SN76477_mixer_b_w(0, (data & 0x08) ? 1 : 0);
	SN76477_mixer_a_w(0, (data & 0x10) ? 1 : 0);
	SN76477_mixer_c_w(0, (data & 0x20) ? 1 : 0);

	SN76477_envelope_2_w(0, (data & 0x40) ? 1 : 0);
	SN76477_envelope_1_w(0, (data & 0x80) ? 1 : 0);
}

// Keyboard

static int keylatch;

static const UINT8 abc80_keycodes[7*4][8] =
{
	// unshift
	{ 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38 },
	{ 0x39, 0x30, 0x2B, 0x60, 0x3C, 0x71, 0x77, 0x65 },
	{ 0x72, 0x74, 0x79, 0x75, 0x69, 0x6F, 0x70, 0x7D },
	{ 0x7E, 0x0D, 0x61, 0x73, 0x64, 0x66, 0x67, 0x68 },
	{ 0x6A, 0x6B, 0x6C, 0x7C, 0x7B, 0x27, 0x08, 0x7A },
	{ 0x78, 0x63, 0x76, 0x62, 0x6E, 0x6D, 0x2C, 0x2E },
	{ 0x2D, 0x09, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00 },

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

static void abc80_keyboard_scan(void)
{
	UINT8 keycode = 0;
	UINT8 data;
	int table = 0, row, col;

	// shift, upper case
	if (readinputport(7) & 0x07)
	{
		table |= 0x01;
	}

	// ctrl
	if (readinputport(7) & 0x08)
	{
		table |= 0x02;
	}

	for (row = 0; row < 7; row++)
	{
		data = readinputport(row);

		if (data != 0)
		{
			UINT8 ibit = 1;

			for (col = 0; col < 8; col++)
			{
				if (data & ibit)
				{
					keycode = abc80_keycodes[row + (table * 7)][col];
				}
				ibit <<= 1;
			}
		}
	}

	if (keycode != 0)
	{
		keylatch = keycode;
		z80pio_p_w(0, 0, keylatch | 0x80);
	}
	else
	{
		z80pio_p_w(0, 0, keylatch);
	}
}

// PIO

static READ8_HANDLER( abc80_pio_r )
{
	switch (offset)
	{
	case 0:
		return z80pio_d_r(0, 0);
	case 1:
		return z80pio_c_r(0, 0);
	case 2:
		return z80pio_d_r(0, 1);
	case 3:
		return z80pio_c_r(0, 1);
	}

	return 0xff;
}

static WRITE8_HANDLER( abc80_pio_w )
{
	switch (offset)
	{
	case 0:
		z80pio_d_w(0, 0, data);
		break;
	case 1:
		z80pio_c_w(0, 0, data);
		break;
	case 2:
		z80pio_d_w(0, 1, data);
		break;
	case 3:
		z80pio_c_w(0, 1, data);
		break;
	}
}

/* Memory Maps */

static ADDRESS_MAP_START( abc80_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x6000, 0x6fff) AM_ROM
	AM_RANGE(0x7000, 0x73ff) AM_ROM
	AM_RANGE(0x7800, 0x7bff) AM_ROM
	AM_RANGE(0x7c00, 0x7fff) AM_RAM AM_BASE(&videoram)
	AM_RANGE(0x8000, 0xbfff) AM_RAM	// Expanded
	AM_RANGE(0xc000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( abc80_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) | AMEF_UNMAP(1) )
	AM_RANGE(0x00, 0x00) AM_READWRITE(abcbus_data_r, abcbus_data_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(abcbus_status_r, abcbus_channel_w)
	AM_RANGE(0x02, 0x05) AM_WRITE(abcbus_command_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(abc80_sound_w)
	AM_RANGE(0x07, 0x07) AM_READ(abcbus_reset_r)
	AM_RANGE(0x38, 0x3b) AM_READWRITE(abc80_pio_r, abc80_pio_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( abc80 )
	PORT_START_TAG("ROW0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR(0x00A4)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('/')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')

	PORT_START_TAG("ROW1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('=')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('+') PORT_CHAR('?')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR(0x00C9)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("< >") PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('<') PORT_CHAR('>')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')

	PORT_START_TAG("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR(0x00C5)

	PORT_START_TAG("ROW3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(0x00DC)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR('\r')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')

	PORT_START_TAG("ROW4")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(0x00D6)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(0x00C4)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\'') PORT_CHAR('*')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("LEFT") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')

	PORT_START_TAG("ROW5")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR(';')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR(':')

	PORT_START_TAG("ROW6")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("RIGHT") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("ROW7")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("LEFT SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("RIGHT SHIFT") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("UPPER CASE") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

/* Graphics Layout */

static const gfx_layout charlayout_abc80 =
{
	6, 10,
	128,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8 },
	10*8
};

/* Graphics Decode Information */

static GFXDECODE_START( abc80 )
	GFXDECODE_ENTRY( REGION_GFX1, 0, charlayout_abc80, 0, 2 )	// normal characters
	GFXDECODE_ENTRY( REGION_GFX1, 0x500, charlayout_abc80, 0, 2 )	// graphics characters
GFXDECODE_END

/* Sound Interface */

static const struct SN76477interface sn76477_interface =
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
	cpunum_set_input_line(machine, 0, INPUT_LINE_NMI, PULSE_LINE);
}

/* Machine Initialization */

/*
    PIO Channel A

    0  R    Keyboard Data
    1  R    Keyboard Data
    2  R    Keyboard Data
    3  R    Keyboard Data
    4  R    Keyboard Data
    5  R    Keyboard Data
    6  R    Keyboard Data
    7  R    Keyboard Strobe

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

static const struct z80_irq_daisy_chain abc80_daisy_chain[] =
{
	{ z80pio_reset, z80pio_irq_state, z80pio_irq_ack, z80pio_irq_reti, 0 },
	{ 0, 0, 0, 0, -1 }
};

static TIMER_CALLBACK(abc80_keyboard_tick)
{
	abc80_keyboard_scan();
}

static MACHINE_START( abc80 )
{
	state_save_register_global(keylatch);

	abc80_keyboard_timer = timer_alloc(abc80_keyboard_tick, NULL);
	timer_adjust_periodic(abc80_keyboard_timer, attotime_zero, 0, ATTOTIME_IN_USEC(2500));
}

/* Machine Drivers */

static MACHINE_DRIVER_START( abc80 )

	// basic machine hardware

	MDRV_CPU_ADD(Z80, ABC80_XTAL/2/2)	// 2.9952 MHz
	MDRV_CPU_CONFIG(abc80_daisy_chain)
	MDRV_CPU_PROGRAM_MAP(abc80_map, 0)
	MDRV_CPU_IO_MAP(abc80_io_map, 0)
	MDRV_CPU_VBLANK_INT(abc80_nmi_interrupt, 1)

	MDRV_MACHINE_START(abc80)

	// video hardware

	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_GFXDECODE(abc80)
	MDRV_PALETTE_LENGTH(2)

	MDRV_PALETTE_INIT(abc80)
	MDRV_VIDEO_START(abc80)
	MDRV_VIDEO_UPDATE(abc80)

	MDRV_SCREEN_RAW_PARAMS(ABC80_XTAL/2, ABC80_HTOTAL, ABC80_HBEND, ABC80_HBSTART, ABC80_VTOTAL, ABC80_VBEND, ABC80_VBSTART)

	// sound hardware

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SN76477, 0)
	MDRV_SOUND_CONFIG(sn76477_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( abc80 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "za3508.a2", 0x0000, 0x1000, CRC(e2afbf48) SHA1(9883396edd334835a844dcaa792d29599a8c67b9) )
	ROM_LOAD( "za3509.a3", 0x1000, 0x1000, CRC(d224412a) SHA1(30968054bba7c2aecb4d54864b75a446c1b8fdb1) )
	ROM_LOAD( "za3506.a4", 0x2000, 0x1000, CRC(1502ba5b) SHA1(5df45909c2c4296e5701c6c99dfaa9b10b3a729b) )
	ROM_LOAD( "za3507.a5", 0x3000, 0x1000, CRC(bc8860b7) SHA1(28b6cf7f5a4f81e017c2af091c3719657f981710) )
	ROM_SYSTEM_BIOS( 0, "abcdos",		"ABC-DOS" )
	ROMX_LOAD("abcdos",    0x6000, 0x1000, CRC(2cb2192f) SHA1(a6b3a9587714f8db807c05bee6c71c0684363744), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "abcdosdd",		"ABC-DOS DD" )
	ROMX_LOAD("abcdosdd",  0x6000, 0x1000, CRC(36db4c15) SHA1(ae462633f3a9c142bb029beb14749a84681377fa), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "udf20",		"UDF-DOS v.20" )
	ROMX_LOAD("udfdos20",  0x6000, 0x1000, CRC(69b09c0b) SHA1(403997a06cf6495b8fa13dc74eff6a64ef7aa53e), ROM_BIOS(3) )
	ROM_LOAD( "iec",	   0x7000, 0x0400, NO_DUMP )
	ROM_LOAD( "printer",   0x7800, 0x0400, NO_DUMP )

	ROM_REGION( 0x0a00, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sn74s262.h2", 0x0000, 0x0a00, NO_DUMP ) // UK charset
	ROM_LOAD( "sn74s263.h2", 0x0000, 0x0a00, CRC(9e064e91) SHA1(354783c8f2865f73dc55918c9810c66f3aca751f) )

	ROM_REGION( 0x400, REGION_PROMS, 0 )
	ROM_LOAD( "abc8011.k5", 0x0000, 0x0080, NO_DUMP ) // 82S129 256x4 horizontal sync
	ROM_LOAD( "abc8012.j3", 0x0080, 0x0080, NO_DUMP ) // 82S129 256x4 chargen 74S263 column address
	ROM_LOAD( "abc8013.e7", 0x0100, 0x0080, NO_DUMP ) // 82S129 256x4 address decoder
	ROM_LOAD( "abc8021.k2", 0x0180, 0x0100, NO_DUMP ) // 82S131 512x4 vertical sync, videoram
	ROM_LOAD( "abc8022.k1", 0x0280, 0x0100, NO_DUMP ) // 82S131 512x4 chargen 74S263 row address
ROM_END

/* System Configuration */

static void abc80_printer_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* printer */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										printer_device_getinfo(devclass, state, info); break;
	}
}

#if 0
static void abc80_cassette_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	// cassette
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_CASSETTE_FORMATS:				info->p = (void *) abc80_cassette_formats; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}
#endif

static void abc80_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:							info->load = device_load_abc_floppy; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "dsk"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START( abc80 )
	CONFIG_RAM_DEFAULT(16 * 1024)
	CONFIG_RAM		  (32 * 1024)
	CONFIG_DEVICE(abc80_printer_getinfo)
//  CONFIG_DEVICE(abc80_cassette_getinfo)
	CONFIG_DEVICE(abc80_floppy_getinfo)
SYSTEM_CONFIG_END

/* Drivers */

/*    YEAR  NAME    PARENT  COMPAT  MACHINE INPUT   INIT    CONFIG  COMPANY             FULLNAME    FLAGS */
COMP( 1978, abc80,  0,      0,      abc80,  abc80,  0,      abc80,  "Luxor Datorer AB", "ABC 80",   GAME_SUPPORTS_SAVE )
