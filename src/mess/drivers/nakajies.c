/******************************************************************************

  Driver for the ES-2xx series electronic typewriters made by Nakajima.

Nakajima was the OEM manufacturer for a series of typewriters which were
sold by different brands around the world. The PCB layouts for these
machines are the same. The models differed in the amount of (static) RAM:
128KB or 256KB; and in the system rom (mainly only different language
support).


Model   |  SRAM | Language | Branded model
--------+-------+----------+----------------------
ES-210N | 128KB | German   | Walther ES-210
ES-220  | 256KB | Englsih  | NTS DreamWriter T400
ES-2??  | ???KB | Spanish  | Dator 3000



NTS information from http://web.archive.org/web/19980205154137/nts.dreamwriter.com/dreamwt4.html:

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


static ADDRESS_MAP_START( nakajies128_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x00000, 0x1ffff ) AM_RAM
	AM_RANGE( 0x80000, 0xfffff ) AM_ROM AM_REGION( "bios", 0 )
ADDRESS_MAP_END


static ADDRESS_MAP_START( nakajies256_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x00000, 0x3ffff ) AM_RAM
	AM_RANGE( 0x80000, 0xfffff ) AM_ROM AM_REGION( "bios", 0 )
ADDRESS_MAP_END


static INPUT_PORTS_START( nakajies )
INPUT_PORTS_END


static MACHINE_DRIVER_START( nakajies128 )
	MDRV_CPU_ADD( "v20hl", V20, 9830000 )	/* Weird clock */
	MDRV_CPU_PROGRAM_MAP( nakajies128_map, 0 )

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


static MACHINE_DRIVER_START( nakajies256 )
	MDRV_CPU_ADD( "v20hl", V20, 9830000 )	/* Weird clock */
	MDRV_CPU_PROGRAM_MAP( nakajies256_map, 0 )

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
	ROM_LOAD("t4_ir_2.1.ic303", 0x00000, 0x80000, CRC(f0f45fd2) SHA1(3b4d5722b3e32e202551a1be8ae36f34ad705ddd))
ROM_END


ROM_START(wales210)
	ROM_REGION( 0x80000, "bios", 0 )
	ROM_LOAD("wales210.ic303", 0x00000, 0x80000, CRC(a8e8d991) SHA1(9a133b37b2fbf689ae1c7ab5c7f4e97cd33fd596))		/* 27c4001 */
ROM_END


/* TODO: Verify rom name and RAM size */
ROM_START(dator3k)
	ROM_REGION( 0x80000, "bios", 0 )
	ROM_LOAD("dator3000.ic303", 0x00000, 0x80000, CRC(b67fffeb) SHA1(e48270d15bef9453bcb6ba8aa949fd2ab3feceed))
ROM_END


/*    YEAR  NAME      PARENT    COMPAT  MACHINE      INPUT     INIT    CONFIG  COMPANY    FULLNAME            FLAGS */
COMP( 199?, wales210,        0, 0,      nakajies128, nakajies, 0,      0,      "Walther", "ES-210",           GAME_NOT_WORKING )	/* German */
COMP( 199?, dator3k,  wales210, 0,      nakajies128, nakajies, 0,      0,      "Dator",   "Dator 3000",       GAME_NOT_WORKING )	/* Spanish */
COMP( 199?, drwrt400, wales210, 0,      nakajies256, nakajies, 0,      0,      "NTS",     "DreamWriter T400", GAME_NOT_WORKING )	/* English */


