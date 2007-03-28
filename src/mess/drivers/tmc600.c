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

	ROM0-5	- Toshiba TMM2732DI 4Kx8 EPROM
	ROM6	- Hitachi HN462732G 4Kx8 EPROM
	5114	- RCA MWS5114E1 1024-Word x 4-Bit LSI Static RAM
	MC1374	- Motorola MC1374P TV Modulator
	CDP1802 - RCA CDP1802BE CMOS 8-Bit Microprocessor running at ? MHz
	CDP1852	- RCA CDP1852CE Byte-Wide Input/Output Port
	CDP1853	- RCA CDP1853CE N-Bit 1 of 8 Decoder
	CDP1856	- RCA CDP1856CE 4-Bit Memory Buffer
	CDP1869	- RCA CDP1869CE Video Interface System (VIS) Address and Sound Generator
	CDP1870	- RCA CDP1870CE Video Interface System (VIS) Color Video (DOT XTAL at 5.6260MHz, CHROM XTAL at 8.867238MHz)
	CN1		- RF connector [TMC-700]
	CN2		- printer connector [TMC-700]
	CN3		- EURO connector
	CN4		- tape connector
	CN5		- video connector
	CN6		- power connector
	CN7		- audio connector [TMCP-300]
	CN8		- keyboard connector
	SW1		- RUN/STOP switch
	SW2		- internal speaker/external audio switch [TMCP-300]
	P1		- color phase lock adjustment
	C1		- dot oscillator adjustment
	C2		- chrom oscillator adjustment
	T1		- RF signal strength adjustment [TMC-700]
	T2		- tape recording level adjustment (0.57 Vpp)
	T3		- video output level adjustment (1 Vpp)
	T4		- video synchronization pulse adjustment
	K1		- RF signal quality adjustment [TMC-700]
	K2		- RF channel adjustment (VHF I) [TMC-700]
	LS1		- loudspeaker

*/

#include "driver.h"
#include "inputx.h"
#include "devices/printer.h"
#include "devices/basicdsk.h"
#include "devices/cassette.h"
#include "devices/snapquik.h"
#include "cpu/cdp1802/cdp1802.h"
#include "video/generic.h"
#include "video/cdp1869.h"

/* Read/Write Handlers */

static int vismac_reg_latch;
static int vismac_color_latch;
static int vismac_bkg_latch;
static int vismac_blink;
static UINT8 vismac_colorram[0x400]; // 1024x4 bit color ram (0x08 = blink)

static WRITE8_HANDLER( vismac_register_w )
{
	vismac_reg_latch = data;
}

static WRITE8_HANDLER( vismac_data_w )
{
	switch (vismac_reg_latch)
	{
	case 0x20:
		// set character color
		vismac_color_latch = data;
		break;
	
	case 0x30:
		// write cdp1869 command on the data bus
		vismac_bkg_latch = data & 0x07;
		cdp1869_out3_w(0, data);
		break;
	
	case 0x40:
		cdp1869_out4_w(0, data);
		break;
	
	case 0x50:
		cdp1869_out5_w(0, data);
		break;
	
	case 0x60:
		cdp1869_out6_w(0, data);
		break;
	
	case 0x70:
		cdp1869_out7_w(0, data);
		break;
	}
}

static WRITE8_HANDLER( vismac_pageram_w )
{
	cdp1869_pageram_w( offset, data );
	vismac_colorram[offset] = vismac_color_latch;
}

static int keylatch;

static WRITE8_HANDLER( keyboard_latch_w )
{
	keylatch = data;
}

static WRITE8_HANDLER( printer_w )
{
	printer_output(image_from_devtype_and_index(IO_PRINTER, 0), data);
}

/* Memory Maps */

static ADDRESS_MAP_START( tmc600_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x4fff) AM_ROM
	AM_RANGE(0x6000, 0x7fff) AM_RAM
	AM_RANGE(0xf400, 0xf7ff) AM_READWRITE(cdp1869_charram_r, cdp1869_charram_w)
	AM_RANGE(0xf800, 0xffff) AM_READWRITE(cdp1869_pageram_r, vismac_pageram_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tmc600_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x03, 0x03) AM_WRITE(keyboard_latch_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(printer_w)
	AM_RANGE(0x05, 0x05) AM_WRITE(vismac_data_w)
//	AM_RANGE(0x06, 0x06) AM_WRITE(floppy_w)
	AM_RANGE(0x07, 0x07) AM_WRITE(vismac_register_w)
ADDRESS_MAP_END

/* Input Ports */

INPUT_PORTS_START( tmc600 )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CODE(KEYCODE_0_PAD) PORT_CHAR('0')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CODE(KEYCODE_1_PAD) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CODE(KEYCODE_2_PAD) PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CODE(KEYCODE_3_PAD) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CODE(KEYCODE_4_PAD) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CODE(KEYCODE_5_PAD) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CODE(KEYCODE_6_PAD) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CODE(KEYCODE_7_PAD) PORT_CHAR('7') PORT_CHAR('/')

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CODE(KEYCODE_8_PAD) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CODE(KEYCODE_9_PAD) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_TILDE) PORT_CHAR('@') PORT_CHAR('\\')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')

	PORT_START_TAG("IN3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')

	PORT_START_TAG("IN4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')

	PORT_START_TAG("IN5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('Å')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('Ä')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR('Ö')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('^') PORT_CHAR('~')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("BREAK") PORT_CODE(KEYCODE_END) PORT_CHAR(UCHAR_MAMEKEY(END))

	PORT_START_TAG("IN6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("DEL") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("E2") PORT_CODE(KEYCODE_RALT) PORT_CHAR(UCHAR_MAMEKEY(RALT))
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CTRL1") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CTRL2") PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_MAMEKEY(RCONTROL))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("E1") PORT_CODE(KEYCODE_LALT) PORT_CHAR(UCHAR_MAMEKEY(LALT))
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)

	PORT_START_TAG("IN7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SHIFT LOCK") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("(unknown)") PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("LINE FEED") PORT_CODE(KEYCODE_HOME) PORT_CHAR(UCHAR_MAMEKEY(HOME)) PORT_CHAR(10)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHAR(13)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x93") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))

	PORT_START_TAG("EF")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SPECIAL ) //
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL ) //
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SPECIAL ) // keyboard
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SPECIAL ) //
INPUT_PORTS_END

/* CDP1802 Interface */

static void tmc600_out_q(int level)
{
	//logerror("q level: %u\n", level);
}

static int tmc600_in_ef(void)
{
	int flags = 0xff;

	/*
		EF1		?
		EF2		?
		EF3		keyboard
		EF4		?
	*/

	// keyboard

	flags -= (~readinputport(keylatch / 8) & (1 << (keylatch % 8))) ? EF3 : 0;

	return flags;
}

static CDP1802_CONFIG tmc600_cdp1802_config =
{
	NULL,
	tmc600_out_q,
	tmc600_in_ef
};

static INTERRUPT_GEN( vismac_blink_int )
{
	vismac_blink = !vismac_blink;
}

/* Machine Drivers */

static MACHINE_DRIVER_START( tmc600 )
	
	// basic system hardware

	MDRV_CPU_ADD(CDP1802, 3579545)	// ???
	MDRV_CPU_PROGRAM_MAP(tmc600_map, 0)
	MDRV_CPU_IO_MAP(tmc600_io_map, 0)
	MDRV_CPU_CONFIG(tmc600_cdp1802_config)
	MDRV_CPU_PERIODIC_INT(vismac_blink_int, TIME_IN_HZ(2))

	// video hardware

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_PALETTE_LENGTH(8+64)
	MDRV_PALETTE_INIT(cdp1869)
	MDRV_VIDEO_START(cdp1869)
	MDRV_VIDEO_UPDATE(cdp1869)

	MDRV_SCREEN_ADD("main", 0)
	MDRV_SCREEN_RAW_PARAMS(CDP1869_DOT_CLK_PAL, CDP1869_SCREEN_WIDTH, CDP1869_HBLANK_END, CDP1869_HBLANK_START, CDP1869_TOTAL_SCANLINES_PAL, CDP1869_SCANLINE_VBLANK_END_PAL, CDP1869_SCANLINE_VBLANK_START_PAL)

	// sound hardware

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(CDP1869, CDP1869_DOT_CLK_PAL)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( tmc600s1 )
	ROM_REGION( 0x5000, REGION_CPU1, 0 )
	ROM_LOAD( "sb20",		0x0000, 0x1000, NO_DUMP )
	ROM_LOAD( "sb21",		0x1000, 0x1000, NO_DUMP )
	ROM_LOAD( "sb22",		0x2000, 0x1000, NO_DUMP )
	ROM_LOAD( "sb23",		0x3000, 0x1000, NO_DUMP )
	ROM_LOAD( "190482_2",	0x4000, 0x1000, NO_DUMP )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "chargen",	0x0000, 0x1000, NO_DUMP )
ROM_END

ROM_START( tmc600s2 )
	ROM_REGION( 0x5000, REGION_CPU1, 0 )
	ROM_LOAD( "sb30",		0x0000, 0x1000, CRC(95d1292a) SHA1(1fa52d59d3005f8ac74a32c2164fdb22947c2748) )
	ROM_LOAD( "sb31",		0x1000, 0x1000, CRC(2c8f3d17) SHA1(f14e8adbcddeaeaa29b1e7f3dfa741f4e230f599) )
	ROM_LOAD( "sb32",		0x2000, 0x1000, CRC(dd58a128) SHA1(be9bdb0fc5e0cc3dcc7f2fb7ccab69bf5b043803) )
	ROM_LOAD( "sb33",		0x3000, 0x1000, CRC(b7d241fa) SHA1(6f3eadf86c4e3aaf93d123e302a18dc4d9db964b) )
	ROM_LOAD( "151182",		0x4000, 0x1000, CRC(c1a8d9d8) SHA1(4552e1f06d0e338ba7b0f1c3a20b8a51c27dafde) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "chargen",	0x0000, 0x1000, CRC(93f92cbf) SHA1(371156fb38fa5319c6fde537ccf14eed94e7adfb) )
ROM_END

ROM_START( tmc600as )
	ROM_REGION( 0x5000, REGION_CPU1, 0 )
	ROM_LOAD( "1",			0x0000, 0x1000, NO_DUMP )
	ROM_LOAD( "2",			0x1000, 0x1000, NO_DUMP )
	ROM_LOAD( "3",			0x2000, 0x1000, NO_DUMP )
	ROM_LOAD( "4",			0x3000, 0x1000, NO_DUMP )
	ROM_LOAD( "5",			0x4000, 0x1000, NO_DUMP )
	ROM_LOAD( "6",			0x5000, 0x1000, NO_DUMP )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "chargen",	0x0000, 0x1000, NO_DUMP )
ROM_END

/* System Configuration */

QUICKLOAD_LOAD( tmc600 )
{
	image_fread(image, memory_region(REGION_CPU1) + 0x6300, 0x9500);

	return INIT_PASS;
}

static void tmc600_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

static void tmc600_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_basicdsk_floppy; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "dsk"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

static void tmc600_printer_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* printer */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										printer_device_getinfo(devclass, state, info); break;
	}
}

static void tmc600_quickload_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* quickload */
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "sbp"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_QUICKLOAD_LOAD:				info->f = (genf *) quickload_load_tmc600; break;

		default:										quickload_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START( tmc600 )
	CONFIG_RAM_DEFAULT	( 9 * 1024)
	CONFIG_RAM			(30 * 1024)
	CONFIG_DEVICE(tmc600_cassette_getinfo)
	CONFIG_DEVICE(tmc600_floppy_getinfo)
	CONFIG_DEVICE(tmc600_printer_getinfo)
	CONFIG_DEVICE(tmc600_quickload_getinfo)
SYSTEM_CONFIG_END

static UINT8 tmc600_get_color_bits(UINT8 cramdata, UINT16 cramaddr, UINT16 pramaddr)
{
	UINT8 color = vismac_colorram[pramaddr];

	if ((color & 0x08) && vismac_blink)
	{
		return vismac_bkg_latch;
	}
	else
	{
		return color;
	}
}

static const CDP1869_interface tmc600_CDP1869_interface =
{
	CDP1869_PAL,
	REGION_GFX1,
	0x1000,					// charrom size
	0x400,					// pageram size
	tmc600_get_color_bits	// color bit callback
};

static DRIVER_INIT( tmc600 )
{
	cdp1869_configure(&tmc600_CDP1869_interface);
}

/* System Drivers */

//    YEAR  NAME 	  PARENT    COMPAT   MACHINE   INPUT     INIT	 CONFIG    COMPANY 	      FULLNAME
COMP( 1982, tmc600s1, 0,		0,	     tmc600,   tmc600,   tmc600, tmc600,   "Telercas Oy", "Telmac TMC-600 (Sarja I)",  GAME_NOT_WORKING )
COMP( 1982, tmc600s2, 0,		0,	     tmc600,   tmc600,   tmc600, tmc600,   "Telercas Oy", "Telmac TMC-600 (Sarja II)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
