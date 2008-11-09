/***************************************************************************

        Pecom driver by Miodrag Milanovic

        08/11/2008 Preliminary driver.

****************************************************************************/

#include "driver.h"
#include "cpu/cdp1802/cdp1802.h"
#include "video/cdp1869.h"
#include "includes/pecom.h"


/* Address maps */
static ADDRESS_MAP_START(pecom64_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x3fff ) AM_RAMBANK(1)
	AM_RANGE( 0x4000, 0x7fff ) AM_RAMBANK(2)
    AM_RANGE( 0x8000, 0xbfff ) AM_ROM  // ROM 1
    AM_RANGE( 0xc000, 0xf3ff ) AM_ROM  // ROM 2
    AM_RANGE( 0xf400, 0xf7ff ) AM_DEVREADWRITE(CDP1869_VIDEO, CDP1869_TAG, cdp1869_charram_r, cdp1869_charram_w)
	AM_RANGE( 0xf800, 0xffff ) AM_DEVREADWRITE(CDP1869_VIDEO, CDP1869_TAG, cdp1869_pageram_r, cdp1869_pageram_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pecom64_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_WRITE(pecom_bank_w)
	AM_RANGE(0x03, 0x03) AM_READ(pecom_keyboard_r) AM_DEVWRITE(CDP1869_VIDEO, CDP1869_TAG, cdp1869_out3_w)
	AM_RANGE(0x04, 0x04) AM_DEVWRITE(CDP1869_VIDEO, CDP1869_TAG, cdp1869_out4_w)
	AM_RANGE(0x05, 0x05) AM_DEVWRITE(CDP1869_VIDEO, CDP1869_TAG, cdp1869_out5_w)
	AM_RANGE(0x06, 0x06) AM_DEVWRITE(CDP1869_VIDEO, CDP1869_TAG, cdp1869_out6_w)
	AM_RANGE(0x07, 0x07) AM_DEVWRITE(CDP1869_VIDEO, CDP1869_TAG, cdp1869_out7_w)
ADDRESS_MAP_END 

/* Input ports */
static INPUT_PORTS_START( pecom )
	PORT_START("LINE0")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0)
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1)
		PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2)
		PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3)
		PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4)
		PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5)
		PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6)
		PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7)
INPUT_PORTS_END


static SYSTEM_CONFIG_START(pecom64)
	CONFIG_RAM_DEFAULT(32 * 1024)
SYSTEM_CONFIG_END


/* Machine driver */
static MACHINE_DRIVER_START( pecom64 )
	MDRV_DRIVER_DATA(pecom_state)
	
    /* basic machine hardware */
	MDRV_CPU_ADD("main", CDP1802, 5626000)
	MDRV_CPU_PROGRAM_MAP(pecom64_mem, 0)
  	MDRV_CPU_IO_MAP(pecom64_io, 0)
  	MDRV_CPU_CONFIG(pecom64_cdp1802_config)
  	
  	MDRV_MACHINE_START( pecom )
  	MDRV_MACHINE_RESET( pecom )
		
	// video hardware

	MDRV_IMPORT_FROM(pecom_video)

	// sound hardware	
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD("cdp1869", CDP1869, CDP1869_DOT_CLK_PAL)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( pecom64 )
	  ROM_REGION( 0x10000, "main", ROMREGION_ERASEFF )
	  ROM_LOAD( "pecom64-1.bin", 0x8000, 0x4000, CRC(9a433b47) SHA1(dadb8c399e0a25a2693e10e42a2d7fc2ea9ad427) )
	  ROM_LOAD( "pecom64-2.bin", 0xc000, 0x4000, CRC(2116cadc) SHA1(03f11055cd221d438a40a41874af8fba0fa116d9) )
ROM_END

/* Driver */

/*    YEAR  NAME   PARENT  COMPAT  		MACHINE  	INPUT   INIT  	CONFIG	 COMPANY  FULLNAME   	FLAGS */
COMP( 1985, pecom64,     0,      0, 	pecom64, 	pecom, 	pecom, pecom64,  "Ei Nis", "Pecom 64",	GAME_NOT_WORKING)
