/***************************************************************************

        PK-8020 driver by Miodrag Milanovic

        18/07/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "includes/pk8020.h"

/* Address maps */
static ADDRESS_MAP_START(pk8020_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x1fff ) AM_RAMBANK( 1)
	AM_RANGE( 0x2000, 0x37ff ) AM_RAMBANK( 2)
	AM_RANGE( 0x3800, 0x39ff ) AM_RAMBANK( 3)
	AM_RANGE( 0x3a00, 0x3aff ) AM_RAMBANK( 4)
	AM_RANGE( 0x3b00, 0x3bff ) AM_RAMBANK( 5)
	AM_RANGE( 0x3c00, 0x3fff ) AM_RAMBANK( 6)
	AM_RANGE( 0x4000, 0x5fff ) AM_RAMBANK( 7)
	AM_RANGE( 0x6000, 0x7fff ) AM_RAMBANK( 8)
	AM_RANGE( 0x8000, 0xbeff ) AM_RAMBANK( 9)
	AM_RANGE( 0xbf00, 0xbfff ) AM_RAMBANK(10)
	AM_RANGE( 0xc000, 0xf7ff ) AM_RAMBANK(11)
	AM_RANGE( 0xf800, 0xf9ff ) AM_RAMBANK(12)
	AM_RANGE( 0xfa00, 0xfaff ) AM_RAMBANK(13)
	AM_RANGE( 0xfb00, 0xfbff ) AM_RAMBANK(14)
	AM_RANGE( 0xfc00, 0xfdff ) AM_RAMBANK(15)
	AM_RANGE( 0xfe00, 0xfeff ) AM_RAMBANK(16)
	AM_RANGE( 0xff00, 0xffff ) AM_RAMBANK(17)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pk8020_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( pk8020 )
INPUT_PORTS_END


/* Machine driver */
static MACHINE_DRIVER_START( pk8020 )
  /* basic machine hardware */
  MDRV_CPU_ADD("main", 8080, 2500000)
  MDRV_CPU_PROGRAM_MAP(pk8020_mem, 0)
  MDRV_CPU_IO_MAP(pk8020_io, 0)

  MDRV_MACHINE_RESET( pk8020 )

    /* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_SCREEN_VISIBLE_AREA(0, 256-1, 0, 256-1)
	MDRV_PALETTE_LENGTH(16)
	MDRV_PALETTE_INIT(pk8020)

	MDRV_VIDEO_START(pk8020)
  MDRV_VIDEO_UPDATE(pk8020)
MACHINE_DRIVER_END

/* ROM definition */

ROM_START( korvet )
    ROM_REGION( 0x16000, "main", ROMREGION_ERASEFF )
    ROM_LOAD( "korvet11.rom", 0x10000, 0x6000, CRC(81BDC2AF) )
ROM_END

SYSTEM_CONFIG_START(pk8020)
 	CONFIG_RAM_DEFAULT(128 * 1024)
SYSTEM_CONFIG_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT  MACHINE     INPUT       INIT     CONFIG COMPANY                  FULLNAME   FLAGS */
COMP( 1987, korvet, 	 0,  	 0,	pk8020, 	pk8020, 	pk8020, pk8020,  "", 					 "PK-8020 Korvet",	 GAME_NOT_WORKING)
