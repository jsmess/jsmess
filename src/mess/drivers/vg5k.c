/***************************************************************************

    Philips VG-5000mu

	05/2009 Skeleton driver.

    Informations ( see the very informative http://vg5k.free.fr/ ):
     - Variants: Radiola VG5000 and Schneider VG5000
     - CPU: Zilog Z80 running at 4MHz
     - ROM: 18KB (16 KB BASIC + 2 KB charset )
     - RAM: 24 KB
     - Video: SGS Thomson EF9345 processor
            - Text mode: 25 rows x 40 columns
            - Character matrix: 8 x 10
            - ASCII characters set, 128 graphics mode characters, 192 user characters.
            - Graphics mode: not available within basic, only semi graphic is available.
            - Colors: 8
     - Sound: Synthesizer, 4 Octaves
     - Keyboard: 63 keys AZERTY, Caps Lock, CTRL key to access 33 BASIC instructions
     - I/O: Tape recorder connector (1200/2400 bauds), Scart connector to TV (RGB),
       External PSU (VU0022) connector, Bus connector (2x25 pins)
     - There are 2 versions of the VG5000 ROM, one with Basic v1.0, 
       contained in two 8 KB ROMs, and one with Basic 1.1, contained in
       a single 16 KB ROM.
     - RAM: 24 KB (3 x 8 KB) type SRAM D4168C, more precisely:
         2 x 8 KB, used by the system
         1 x 8 KB, used by the video processor
     - Memory Map:
		 $0000 - $3fff  BASIC + monitor
         $4000 - $47cf  Screen
         $47d0 - $7fff  reserved area for BASIC, variables, etc
         $8000 - $bfff  Memory Expansion 16K or ROM cart
         $c000 - $ffff  Memory Expansion 32K or ROM cart
     - This computer was NOT MSX-compatible!


****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START( vg5k_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x3fff ) AM_ROM
	AM_RANGE( 0x4000, 0xffff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( vg5k_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( vg5k )
INPUT_PORTS_END


static MACHINE_RESET( vg5k )
{
}

static VIDEO_START( vg5k )
{
}

static VIDEO_UPDATE( vg5k )
{
	return 0;
}

static MACHINE_DRIVER_START( vg5k )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
	MDRV_CPU_PROGRAM_MAP(vg5k_mem)
	MDRV_CPU_IO_MAP(vg5k_io)

	MDRV_MACHINE_RESET(vg5k)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MDRV_PALETTE_LENGTH(8)

	MDRV_VIDEO_START(vg5k)
	MDRV_VIDEO_UPDATE(vg5k)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( vg5k )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "default", "VG 5000")
	ROMX_LOAD( "vg5k.bin",  0x0000, 0x4000, CRC(a6998ff8) SHA1(881ba594be0a721a999378312aea0c3c1c7b2b58), ROM_BIOS(1) )			// dumped from a Radiola VG-5000
	ROM_SYSTEM_BIOS(1, "alt", "VG 5000 (alt)")
	ROMX_LOAD( "vg5k.rom", 0x0000, 0x4000, BAD_DUMP CRC(a6f4a0ea) SHA1(58eccce33cc21fc17bc83921018f531b8001eda3), ROM_BIOS(2) )	// from dcvg5k

	ROM_REGION( 0x2000, "gfx1", 0 )
	ROM_LOAD( "charset.rom", 0x0000, 0x2000, BAD_DUMP CRC(b2f49eb3) SHA1(d0ef530be33bfc296314e7152302d95fdf9520fc) )			// from dcvg5k
ROM_END

static SYSTEM_CONFIG_START( vg5k )
	CONFIG_RAM_DEFAULT(24 * 1024)
SYSTEM_CONFIG_END

/* Driver */
/*    YEAR  NAME    PARENT  COMPAT  MACHINE  INPUT  INIT   CONFIG COMPANY     FULLNAME   FLAGS */
COMP( 1984, vg5k,   0,      0,      vg5k,    vg5k,  0,     vg5k,  "Philips",  "VG-5000", GAME_NOT_WORKING)
