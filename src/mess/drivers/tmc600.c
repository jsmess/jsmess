/*

Telmac TMC-600

PCB Layout
----------

HP14782-1

            CN2                    CN3
        |---------|     |------------------------|
|-------|---------|-----|------------------------|------------------------------------------|
|  CN1                                                  CN4 SW1       CN5 CN6 P1 SW2 CN7    |
|         |-------|                                4049         T3                          |
| T1 K1   |CDP1852|         4050  4050  4050            40107 T2         C1  C2 8.867238MHz |
|         |-------|         |-------|                                     5.6260MHz T4      |
| MC1374                    |CDP1852|                               ROM5    |---------|     |
|                           |-------|                                       | CDP1870 |     |
|    K2                                                             ROM4    |---------|     |
|                       |-------------|       4050   5114   5114                            |
|           4013        |   CDP1802   |       4050   5114   5114    ROM3    4030  ROM6  4060|
|           4013        |-------------|       4050   5114   5114                            |
|           4081  40106                       4051   5114   5114    ROM2    4076  5114  4011|
|                                             4556   5114   5114          CDP1856 5114      |
|                   |-------|                        5114   5114    ROM1  CDP1856 5114  4076|
|           LS1     |CDP1852| CDP1853       CDP1856  5114   5114                            |
|                   |-------|               CDP1856  5114   5114    ROM0    |---------| 4011|
|                                                                           | CDP1869 |     |
|   7805            4051      4051                                          |---------|     |
|                                                                                           |
|                     |--CN8--|                                                             |
|-------------------------------------------------------------------------------------------|

Notes:
    All IC's shown. TMCP-300 and TMC-700 expansions have been installed.

    ROM0-5  - Toshiba TMM2732DI 4Kx8 EPROM
    ROM6    - Hitachi HN462732G 4Kx8 EPROM
    5114    - RCA MWS5114E1 1024-Word x 4-Bit LSI Static RAM
    MC1374  - Motorola MC1374P TV Modulator
    CDP1802 - RCA CDP1802BE CMOS 8-Bit Microprocessor running at ? MHz
    CDP1852 - RCA CDP1852CE Byte-Wide Input/Output Port
    CDP1853 - RCA CDP1853CE N-Bit 1 of 8 Decoder
    CDP1856 - RCA CDP1856CE 4-Bit Memory Buffer
    CDP1869 - RCA CDP1869CE Video Interface System (VIS) Address and Sound Generator
    CDP1870 - RCA CDP1870CE Video Interface System (VIS) Color Video (DOT XTAL at 5.6260MHz, CHROM XTAL at 8.867238MHz)
    CN1     - RF connector [TMC-700]
    CN2     - printer connector [TMC-700]
    CN3     - EURO connector
    CN4     - tape connector
    CN5     - video connector
    CN6     - power connector
    CN7     - audio connector [TMCP-300]
    CN8     - keyboard connector
    SW1     - RUN/STOP switch
    SW2     - internal speaker/external audio switch [TMCP-300]
    P1      - color phase lock adjustment
    C1      - dot oscillator adjustment
    C2      - chrom oscillator adjustment
    T1      - RF signal strength adjustment [TMC-700]
    T2      - tape recording level adjustment (0.57 Vpp)
    T3      - video output level adjustment (1 Vpp)
    T4      - video synchronization pulse adjustment
    K1      - RF signal quality adjustment [TMC-700]
    K2      - RF channel adjustment (VHF I) [TMC-700]
    LS1     - loudspeaker

*/

/*

    TODO:

    - proper emulation of the VISMAC interface (cursor blinking, color RAM), schematics are needed
    - disk interface
    - CPU frequency needs to be derived from the schematics
    - serial interface expansion card
    - centronics printer handshaking

*/

#include "emu.h"
#include "machine/ctronics.h"
#include "devices/flopdrv.h"
#include "formats/basicdsk.h"
#include "devices/cassette.h"
#include "devices/snapquik.h"
#include "cpu/cdp1802/cdp1802.h"
#include "sound/cdp1869.h"
#include "includes/tmc600.h"
#include "devices/messram.h"

/* Read/Write Handlers */

static WRITE8_HANDLER( keyboard_latch_w )
{
	tmc600_state *state = (tmc600_state *)space->machine->driver_data;

	state->keylatch = data;
}

/* Memory Maps */

static ADDRESS_MAP_START( tmc600_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x4fff) AM_ROM
	AM_RANGE(0x6000, 0xbfff) AM_RAM
	AM_RANGE(0xf400, 0xf7ff) AM_DEVREADWRITE(CDP1869_TAG, cdp1869_charram_r, cdp1869_charram_w)
	AM_RANGE(0xf800, 0xffff) AM_DEVREADWRITE(CDP1869_TAG, cdp1869_pageram_r, cdp1869_pageram_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tmc600_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x03, 0x03) AM_WRITE(keyboard_latch_w)
	AM_RANGE(0x04, 0x04) AM_DEVWRITE(CENTRONICS_TAG, centronics_data_w)
	AM_RANGE(0x05, 0x05) AM_DEVWRITE(CDP1869_TAG, tmc600_vismac_data_w)
//  AM_RANGE(0x06, 0x06) AM_WRITE(floppy_w)
	AM_RANGE(0x07, 0x07) AM_WRITE(tmc600_vismac_register_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( tmc600 )
	PORT_START("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CODE(KEYCODE_0_PAD) PORT_CHAR('0')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CODE(KEYCODE_1_PAD) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CODE(KEYCODE_2_PAD) PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CODE(KEYCODE_3_PAD) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CODE(KEYCODE_4_PAD) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CODE(KEYCODE_5_PAD) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CODE(KEYCODE_6_PAD) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CODE(KEYCODE_7_PAD) PORT_CHAR('7') PORT_CHAR('/')

	PORT_START("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CODE(KEYCODE_8_PAD) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CODE(KEYCODE_9_PAD) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')

	PORT_START("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_TILDE) PORT_CHAR('@') PORT_CHAR('\\')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')

	PORT_START("IN3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')

	PORT_START("IN4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')

	PORT_START("IN5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xC3\x85") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR(0x00C5)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xC3\x84") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(0x00C4)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xC3\x96") PORT_CODE(KEYCODE_COLON) PORT_CHAR(0x00D6)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('^') PORT_CHAR('~')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("BREAK") PORT_CODE(KEYCODE_END) PORT_CHAR(UCHAR_MAMEKEY(END))

	PORT_START("IN6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("DEL") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("E2") PORT_CODE(KEYCODE_RALT) PORT_CHAR(UCHAR_MAMEKEY(RALT))
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CTRL1") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CTRL2") PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_MAMEKEY(RCONTROL))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("E1") PORT_CODE(KEYCODE_LALT) PORT_CHAR(UCHAR_MAMEKEY(LALT))
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)

	PORT_START("IN7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SHIFT LOCK") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("(unknown)") PORT_CODE(KEYCODE_F1) PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("LINE FEED") PORT_CODE(KEYCODE_HOME) PORT_CHAR(UCHAR_MAMEKEY(HOME)) PORT_CHAR(10)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHAR(13)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x93") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))

	PORT_START("EF")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SPECIAL ) //
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL ) // tape in
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SPECIAL ) // keyboard
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SPECIAL ) //

	PORT_START("RUN")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Run/Stop") PORT_CODE(KEYCODE_F10) PORT_CHAR(UCHAR_MAMEKEY(F10)) PORT_TOGGLE
INPUT_PORTS_END

/* CDP1802 Interface */

static CDP1802_MODE_READ( tmc600_mode_r )
{
	if (input_port_read(device->machine, "RUN") & 0x01)
	{
		return CDP1802_MODE_RESET;
	}
	else
	{
		return CDP1802_MODE_RUN;
	}
}

static CDP1802_EF_READ( tmc600_ef_r )
{
	tmc600_state *state = (tmc600_state *)device->machine->driver_data;

	int flags = 0x0f;
	static const char *const keynames[] = { "IN0", "IN1", "IN2", "IN3", "IN4", "IN5", "IN6", "IN7" };

	/*
        EF1     ?
        EF2     tape in
        EF3     keyboard
        EF4     ?
    */

	// tape in

	if (cassette_input(state->cassette) < +0.0) flags -= EF2;

	// keyboard

	flags -= (~input_port_read(device->machine, keynames[state->keylatch / 8]) & (1 << (state->keylatch % 8))) ? EF3 : 0;

	return flags;
}

static WRITE_LINE_DEVICE_HANDLER( tmc600_q_w )
{
	tmc600_state *driver_state = (tmc600_state *)device->machine->driver_data;

	cassette_output(driver_state->cassette, state ? +1.0 : -1.0);
}

static CDP1802_INTERFACE( tmc600_cdp1802_config )
{
	tmc600_mode_r,
	tmc600_ef_r,
	NULL,
	DEVCB_LINE(tmc600_q_w),
	DEVCB_NULL,
	DEVCB_NULL
};

/* Machine Initialization */

static MACHINE_START( tmc600 )
{
	tmc600_state *state = (tmc600_state *)machine->driver_data;
	const address_space *program = cputag_get_address_space(machine, CDP1802_TAG, ADDRESS_SPACE_PROGRAM);

	/* find devices */
	state->cassette = machine->device(CASSETTE_TAG);

	/* configure RAM */
	switch (messram_get_size(machine->device("messram")))
	{
	case 8*1024:
		memory_unmap_readwrite(program, 0x8000, 0xbfff, 0, 0);
		break;

	case 16*1024:
		memory_unmap_readwrite(program, 0xa000, 0xbfff, 0, 0);
		break;
	}

	/* register for state saving */
	state_save_register_global(machine, state->keylatch);
}

static MACHINE_RESET( tmc600 )
{
	cputag_set_input_line(machine, CDP1802_TAG, INPUT_LINE_RESET, PULSE_LINE);
}

/* Machine Drivers */

static const cassette_config tmc600_cassette_config =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED),
	NULL
};
static FLOPPY_OPTIONS_START(tmc600)
	// dsk
FLOPPY_OPTIONS_END

static const floppy_config tmc600_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(tmc600),
	NULL
};

/* F4 Character Displayer */
static const gfx_layout tmc600_charlayout =
{
	6, 9,					/* 6 x 9 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 2048*8 },
	8*8					/* every char takes 2 x 8 bytes */
};

static GFXDECODE_START( tmc600 )
	GFXDECODE_ENTRY( "chargen", 0x0000, tmc600_charlayout, 0, 36 )
GFXDECODE_END

static MACHINE_DRIVER_START( tmc600 )
	MDRV_DRIVER_DATA(tmc600_state)

	// basic system hardware

	MDRV_CPU_ADD(CDP1802_TAG, CDP1802, 3579545)	// ???
	MDRV_CPU_PROGRAM_MAP(tmc600_map)
	MDRV_CPU_IO_MAP(tmc600_io_map)
	MDRV_CPU_CONFIG(tmc600_cdp1802_config)

	MDRV_MACHINE_START(tmc600)
	MDRV_MACHINE_RESET(tmc600)

	// sound and video hardware

	MDRV_IMPORT_FROM(tmc600_video)
	MDRV_GFXDECODE(tmc600)

	/* printer */
	MDRV_CENTRONICS_ADD(CENTRONICS_TAG, standard_centronics)

	/* cassette */
	MDRV_CASSETTE_ADD(CASSETTE_TAG, tmc600_cassette_config)

	MDRV_FLOPPY_4_DRIVES_ADD(tmc600_floppy_config)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("8K")
	MDRV_RAM_EXTRA_OPTIONS("16K,24K")
MACHINE_DRIVER_END

/* ROMs */

ROM_START( tmc600s1 )
	ROM_REGION( 0x5000, CDP1802_TAG, 0 )
	ROM_LOAD( "sb20",		0x0000, 0x1000, NO_DUMP )
	ROM_LOAD( "sb21",		0x1000, 0x1000, NO_DUMP )
	ROM_LOAD( "sb22",		0x2000, 0x1000, NO_DUMP )
	ROM_LOAD( "sb23",		0x3000, 0x1000, NO_DUMP )
	ROM_LOAD( "190482_2",	0x4000, 0x1000, NO_DUMP )

	ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD( "chargen",	0x0000, 0x1000, NO_DUMP )
ROM_END

ROM_START( tmc600s2 )
	ROM_REGION( 0x5000, CDP1802_TAG, 0 )
	ROM_LOAD( "sb30",		0x0000, 0x1000, CRC(95d1292a) SHA1(1fa52d59d3005f8ac74a32c2164fdb22947c2748) )
	ROM_LOAD( "sb31",		0x1000, 0x1000, CRC(2c8f3d17) SHA1(f14e8adbcddeaeaa29b1e7f3dfa741f4e230f599) )
	ROM_LOAD( "sb32",		0x2000, 0x1000, CRC(dd58a128) SHA1(be9bdb0fc5e0cc3dcc7f2fb7ccab69bf5b043803) )
	ROM_LOAD( "sb33",		0x3000, 0x1000, CRC(b7d241fa) SHA1(6f3eadf86c4e3aaf93d123e302a18dc4d9db964b) )
	ROM_LOAD( "151182",		0x4000, 0x1000, CRC(c1a8d9d8) SHA1(4552e1f06d0e338ba7b0f1c3a20b8a51c27dafde) )

	ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD( "chargen",	0x0000, 0x1000, CRC(93f92cbf) SHA1(371156fb38fa5319c6fde537ccf14eed94e7adfb) )
ROM_END

/* System Drivers */
//    YEAR  NAME      PARENT    COMPAT   MACHINE   INPUT     INIT    COMPANY        FULLNAME
COMP( 1982, tmc600s1, 0,	0,	     tmc600,   tmc600,   0, 	   "Telercas Oy", "Telmac TMC-600 (Sarja I)",  GAME_NOT_WORKING )
COMP( 1982, tmc600s2, 0,	0,	     tmc600,   tmc600,   0, 	   "Telercas Oy", "Telmac TMC-600 (Sarja II)", GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE )
