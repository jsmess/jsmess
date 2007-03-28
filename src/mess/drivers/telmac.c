/*

	telmac.c


	MESS Driver by Curt Coder


	Telmac 1800
	-----------
	(c) 10/1977 Telercas Oy, Finland

	CPU:		CDP1802		1.76 MHz
	RAM:		2 KB (4 KB)
	ROM:		512 B

	Video:		CDP1861		1.76 MHz			(OSM-200)
	Resolution:	32x64

	Designer:	Osmo Kainulainen
	Keyboard:	OS-70 A (QWERTY, hexadecimal keypad)


	Oscom 1000B
	-----------
	(c) 197? Oscom Oy, Finland

	CPU:		CDP1802A	? MHz

	Enhanced Telmac 1800 with built-in CRT board.


	Telmac 2000
	-----------
	(c) 1980 Telercas Oy, Finland

	CPU:		CDP1802A	1.75 MHz
	RAM:		16 KB
	ROM:		512 B / 2 KB (TOOL-2000)

	Video:		CDP1864		1.75 MHz
	Color RAM:	512 B

	Colors:		8 fg, 4 bg
	Resolution:	64x192
	Sound:		frequency control, volume on/off
	Keyboard:	ASCII, KB-16, KB-64


	Telmac 2000E
	------------
	(c) 1980 Telercas Oy, Finland

	CPU:		CDP1802A	1.75 MHz
	RAM:		8 KB
	ROM:		8 KB

	Video:		CDP1864		1.75 MHz
	Color RAM:	1 KB

	Colors:		8 fg, 4 bg
	Resolution:	64x192
	Sound:		frequency control, volume on/off
	Keyboard:	ASCII (RCA VP-601/VP-611), KB-16/KB-64

	SBASIC:		24.0


	Oscom Nano
	----------
	(c) 1981 Oscom Oy, Finland

	CPU:		CDP1802A	? MHz
	ROM:		512 B / 1 KB

	Small form factor version of Telmac 1800. Combined text and graphics output.


	Telmac TMC-121/111/112
	----------------------
	(c) 198? Telercas Oy, Finland

	CPU:		CDP1802A	? MHz

	Built from Telmac 2000 series cards. Huge metal box.

*/

/*

	2003-05-20	Project started
	2003-05-21  Telmac TMC-600 (Sarja II) acquired, unit broken
	2003-06-22	Preliminary driver, missing ROMs among other things
	2003-07-28	Added some TMC-1800 info
	2003-07-31	Added some computer info
	2004-01-04	Reformatted inputs and added memory map for tmc600
	2004-01-06	Fixed some code
	2004-01-19	Dumped ROMs, hooked up display and keyboard
	2004-01-27	Scanned in Telmac 2000 manual
	2004-01-28	Scanned in Telmac 2000E manual
	2004-01-29	Added Telmac 2000 code
	2004-02-03	Modified cdp1802.c to use regular ports instead of out_n/in_n callback functions
	2004-02-04	Figured out the keyboard except for one key, the port update fixed reading, still a bit too quick to repeat
	2004-02-19	Examined the connection between 1802 and 1869 on the pcb with a multimeter, added vismac_w
	2004-02-22	Separated video code to vidhrdw/cdp186x.c
	2004-03-21	Cleaned up
	2004-04-11	More cleanup
	2004-04-29	Fixed CDP1802 core -> gfx anomalies now gone, fixed scrolling, bkg color & dblsize
	2004-08-12	Added colorram and cursor blink approximation, cdp1869 tone output
	2004-08-13	Added printer output, quickload, more devices

*/

#include "driver.h"
#include "inputx.h"
#include "cpu/cdp1802/cdp1802.h"
#include "video/generic.h"
#include "video/cdp1864.h"
#include "devices/printer.h"
#include "devices/basicdsk.h"
#include "devices/cassette.h"
#include "devices/snapquik.h"
#include "sound/beep.h"

/* Read/Write Handlers */

static int keylatch;

static WRITE8_HANDLER( keyboard_latch_w )
{
	keylatch = data;
}

static WRITE8_HANDLER( tmc2000_bankswitch_w )
{
	memory_set_bank(1, data & 0x01);

	cdp1864_tone_divisor_latch_w(0, data);
}

/* Memory Maps */

// Telmac 1800

static ADDRESS_MAP_START( tmc1800_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM
	AM_RANGE(0x8000, 0x81ff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( tmc1800_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x02, 0x02) AM_WRITE(keyboard_latch_w)
ADDRESS_MAP_END

// Telmac 2000

static ADDRESS_MAP_START( tmc2000_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_RAM
	AM_RANGE(0x8000, 0x87ff) AM_ROMBANK(1)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tmc2000_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_READWRITE(cdp1864_audio_enable_r, cdp1864_step_background_color_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(keyboard_latch_w)
	AM_RANGE(0x04, 0x04) AM_READWRITE(cdp1864_audio_disable_r, tmc2000_bankswitch_w)
ADDRESS_MAP_END

/* Input Ports */

INPUT_PORTS_START( tmc1800 )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CODE(KEYCODE_0_PAD) PORT_CHAR('0')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CODE(KEYCODE_1_PAD) PORT_CHAR('1')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CODE(KEYCODE_2_PAD) PORT_CHAR('2')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CODE(KEYCODE_3_PAD) PORT_CHAR('3')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CODE(KEYCODE_4_PAD) PORT_CHAR('4')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CODE(KEYCODE_5_PAD) PORT_CHAR('5')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CODE(KEYCODE_6_PAD) PORT_CHAR('6')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CODE(KEYCODE_7_PAD) PORT_CHAR('7')

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CODE(KEYCODE_8_PAD) PORT_CHAR('8')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CODE(KEYCODE_9_PAD) PORT_CHAR('9')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
INPUT_PORTS_END

/* CDP1802 Interfaces */

// Telmac 1800

static void tmc1800_video_dma(int cycles)
{
}

static void tmc1800_out_q(int level)
{
}

static int tmc1800_in_ef(void)
{
	int flags = 0;

	/*
		EF1		?
		EF2		?
		EF3		keyboard
		EF4		?
	*/

	// keyboard
	if (~readinputport(keylatch / 8) & (1 << (keylatch % 8))) flags |= EF3;

	return flags;
}

static CDP1802_CONFIG tmc1800_config =
{
	tmc1800_video_dma,
	tmc1800_out_q,
	tmc1800_in_ef
};

// Telmac 2000

static void tmc2000_video_dma(int cycles)
{
}

static void tmc2000_out_q(int level)
{
	// CDP1864 sound generator on/off
	cdp1864_audio_output_w(level);

	// set Q led status
	set_led_status(1, level);
}

static int tmc2000_in_ef(void)
{
	int flags = 0;

	/*
		EF1		?
		EF2		keyboard
		EF3		?
		EF4		?
	*/

	// keyboard
	if (~readinputport(keylatch / 8) & (1 << (keylatch % 8))) flags |= EF2;

	return flags;
}

static CDP1802_CONFIG tmc2000_config =
{
	tmc2000_video_dma,
	tmc2000_out_q,
	tmc2000_in_ef
};

/* Machine Initialization */

static MACHINE_START( tmc2000 )
{
	memory_configure_bank(1, 0, 1, memory_region(REGION_CPU1) + 0x8000, 0);
	memory_configure_bank(1, 1, 1, &colorram, 0);

	return 0;
}

static MACHINE_RESET( tmc2000 )
{
	// set program counter to 0x8000
}

/* Machine Drivers */

static MACHINE_DRIVER_START( tmc1800 )
	
	// basic system hardware

	MDRV_CPU_ADD(CDP1802, 1750000)	// 1.75 MHz
	MDRV_CPU_PROGRAM_MAP(tmc1800_map, 0)
	MDRV_CPU_IO_MAP(tmc1800_io_map, 0)
	MDRV_CPU_CONFIG(tmc1800_config)

	MDRV_SCREEN_REFRESH_RATE(CDP1864_FPS)	//
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_SCREEN_SIZE(360, 262)
	MDRV_SCREEN_VISIBLE_AREA(0, 360-1, 80, 208-1)
	MDRV_PALETTE_LENGTH(2)

	// video hardware

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( tmc2000 )
	
	// basic system hardware

	MDRV_CPU_ADD(CDP1802, CDP1864_CLK_FREQ)	// 1.75 MHz
	MDRV_CPU_PROGRAM_MAP(tmc2000_map, 0)
	MDRV_CPU_IO_MAP(tmc2000_io_map, 0)
	MDRV_CPU_CONFIG(tmc2000_config)

	MDRV_MACHINE_START(tmc2000)
	MDRV_MACHINE_RESET(tmc2000)

	MDRV_SCREEN_REFRESH_RATE(CDP1864_FPS)	// 50.08 Hz
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	// video hardware

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(360, 312)
	MDRV_SCREEN_VISIBLE_AREA(0, 360-1, 20, 308-1)
	MDRV_PALETTE_LENGTH(8)

	MDRV_PALETTE_INIT(tmc2000)
	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(cdp1864)

	// sound hardware

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( tmc1800 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "monitor",	0x8000, 0x0200, NO_DUMP )
ROM_END

SYSTEM_BIOS_START( tmc2000 )
	SYSTEM_BIOS_ADD( 0, "monitor",  "Monitor" )
	SYSTEM_BIOS_ADD( 1, "tool2000", "TOOL-2000" )
SYSTEM_BIOS_END

ROM_START( tmc2000 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROMX_LOAD( "200.m5",	0x8000, 0x0200, CRC(7f8e7ce4) SHA1(3302628f9a57ce456347f37c9a970be6390465e7), ROM_BIOS(1) )
	ROMX_LOAD( "tool2000",	0x8000, 0x0800, NO_DUMP, ROM_BIOS(2) )
ROM_END

/* System Configuration */

static void tmc1800_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START( tmc1800 )
	CONFIG_RAM_DEFAULT	( 2 * 1024)
	CONFIG_RAM			( 4 * 1024)
	CONFIG_DEVICE(tmc1800_cassette_getinfo)
SYSTEM_CONFIG_END

static void tmc2000_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START( tmc2000 )
	CONFIG_RAM_DEFAULT	(16 * 1024)
	CONFIG_RAM			(32 * 1024)
	CONFIG_DEVICE(tmc2000_cassette_getinfo)
SYSTEM_CONFIG_END

static void setup_beep(int dummy)
{
	beep_set_volume(0, 0);
	beep_set_state(0, 1);
}

static DRIVER_INIT( telmac )
{
	timer_set(0.0, 0, setup_beep);
}

/* System Drivers */

//    YEAR  NAME 	  PARENT   COMPAT   MACHINE   INPUT     INIT	CONFIG    COMPANY 	     FULLNAME
COMP( 1977, tmc1800,  0,       0,	    tmc1800,  tmc1800,  telmac, tmc1800,  "Telercas Oy", "Telmac 1800", GAME_NOT_WORKING )
COMPB( 1980, tmc2000,  0,       tmc2000, 0, 		tmc2000,  tmc1800,  telmac, tmc2000,  "Telercas Oy", "Telmac 2000",	GAME_NOT_WORKING )
