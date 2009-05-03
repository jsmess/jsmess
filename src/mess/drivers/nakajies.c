/******************************************************************************

  Driver for DreamWriter electronic typewriter made by NTS Computer Systems
  (and clones).

Information from http://web.archive.org/web/19980205154137/nts.dreamwriter.com/dreamwt4.html:

File Management & Memory:

· Uniquely name up to 128 files
· Recall, rename or delete files
· Copy files to and from PCMCIA Memory card
· PCMCIA Memory expansion cards available for 60 or 250 pages of text
· Working memory allows up to 20 pages of text (50KB) to be displayed
· Storage memory allows up to 80 pages of text (128KB) in total
· DreamLink software exports and imports documents in RTF retaining all
  formatting to Macintosh or Windows PC to all commonly used Word Processing programs
· Transfer cable provided compatible to both Macintosh and Windows PC's. 
· T400 is field upgradeable to IR with the optional Infrared module.

Hardware:

· LCD Screen displays 8 lines by 80 characters raised and tilted 30 degrees
· Contrast Dial and feet adjust to user preference
· Parallel and Serial ports( IR Upgrade Optional) for connectivity to printers, Macintosh and Windows PC's
· Full size 64 key keyboard with color coded keys and quick reference menu bar
· NiCad rechargeable batteries for up to 8 hours of continuous use prior to recharging
· AC adapter for recharging batteries is lightweight and compact design
· NEC V20HL 9.83 MHz processor for fast response time
· Durable solid state construction weighing 2.2 lbs including battery pack
· Dimensions approximately 11" wide by 8" long by 1" deep
· FCC and CSA approved



******************************************************************************/

#include "driver.h"
#include "cpu/nec/nec.h"
#include "sound/speaker.h"


static ADDRESS_MAP_START( drwr_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x00000, 0x1ffff ) AM_RAM
	AM_RANGE( 0x80000, 0xfffff ) AM_ROM AM_REGION( "bios", 0 )
ADDRESS_MAP_END


static INPUT_PORTS_START( drwrt400 )
INPUT_PORTS_END


static MACHINE_DRIVER_START( drwrt400 )
	MDRV_CPU_ADD( "v20hl", V20, 9830000 )	/* Weird clock */
	MDRV_CPU_PROGRAM_MAP( drwr_map, 0 )

	MDRV_SCREEN_ADD( "screen", LCD )
	MDRV_SCREEN_REFRESH_RATE( 50 )	/* Wild guess */
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16 )
	MDRV_SCREEN_SIZE( 80 * 8, 8 * 8 )
	MDRV_SCREEN_VISIBLE_AREA( 0, 8 * 80 - 1, 0, 8 * 8 - 1 )
	MDRV_PALETTE_LENGTH( 2 )

	/* sound */
	MDRV_SPEAKER_STANDARD_MONO( "mono" )
	MDRV_SOUND_ADD( "speaker", SPEAKER, 0 )
	MDRV_SOUND_ROUTE( ALL_OUTPUTS, "mono", 1.00 )
MACHINE_DRIVER_END


ROM_START(drwrt400)
	ROM_REGION( 0x80000, "bios", 0 )
	ROM_LOAD("drwrt400.rom", 0x00000, 0x80000, CRC(f0f45fd2) SHA1(3b4d5722b3e32e202551a1be8ae36f34ad705ddd))  /* 27040 eeprom */
ROM_END


ROM_START(wales210)
	ROM_REGION( 0x80000, "bios", 0 )
	ROM_LOAD("wales210.rom", 0x00000, 0x80000, CRC(a8e8d991) SHA1(9a133b37b2fbf689ae1c7ab5c7f4e97cd33fd596))  /* 27040 eeprom */
ROM_END


ROM_START(dator3k)
	ROM_REGION( 0x80000, "bios", 0 )
	ROM_LOAD("dator3000.rom", 0x00000, 0x80000, CRC(b67fffeb) SHA1(e48270d15bef9453bcb6ba8aa949fd2ab3feceed))	/* 27040 eeprom */
ROM_END


/*    YEAR  NAME      PARENT    COMPAT  MACHINE   INPUT     INIT    CONFIG  COMPANY    FULLNAME            FLAGS */
COMP( 19??, drwrt400,        0, 0,      drwrt400, drwrt400, 0,      0,      "NTS",     "DreamWriter T400", GAME_NOT_WORKING )	/* English */
COMP( 19??, wales210, drwrt400, 0,      drwrt400, drwrt400, 0,      0,      "Walther", "ES-210",           GAME_NOT_WORKING )	/* German */
COMP( 19??, dator3k,  drwrt400, 0,      drwrt400, drwrt400, 0,      0,      "Dator",   "Dator 3000",       GAME_NOT_WORKING )	/* Spanish */

