/***************************************************************************

        SAPI-1 driver by Miodrag Milanovic

        09/09/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "includes/sapi1.h"


/* Address maps */
static ADDRESS_MAP_START(sapi1_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x1000, 0x3fff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sapi1_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( sapi1 )
INPUT_PORTS_END

/* Machine driver */
static MACHINE_DRIVER_START( sapi1 )
	  /* basic machine hardware */
	  MDRV_CPU_ADD("main", 8080, 2000000)
	  MDRV_CPU_PROGRAM_MAP(sapi1_mem, 0)
	  MDRV_CPU_IO_MAP(sapi1_io, 0)

	  MDRV_MACHINE_START( sapi1 )
	  MDRV_MACHINE_RESET( sapi1 )

    /* video hardware */
		MDRV_SCREEN_ADD("main", RASTER)
		MDRV_SCREEN_REFRESH_RATE(50)
		MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
		MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
		MDRV_SCREEN_SIZE(64*6, 32*12)
		MDRV_SCREEN_VISIBLE_AREA(0, 64*6-1, 0, 32*12-1)

		MDRV_PALETTE_LENGTH(2)
		MDRV_PALETTE_INIT(black_and_white)

		MDRV_VIDEO_START(sapi1)
    MDRV_VIDEO_UPDATE(sapi1)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(sapi1)
	CONFIG_RAM_DEFAULT(64 * 1024)
SYSTEM_CONFIG_END

/* ROM definition */

ROM_START( sapi1 )
    ROM_REGION( 0x10000, "main", ROMREGION_ERASEFF )
    ROM_LOAD( "sapi1.rom", 0x0000, 0x1000, CRC(c6e85b01) SHA1(2a26668249c6161aef7215a1e2b92bfdf6fe3671) )
    ROM_REGION(0x1000, "gfx1",0)
		ROM_LOAD ("sapi1.chr", 0x0000, 0x1000, CRC(9edafa2c) SHA1(a903db0e8923cca91646274d010dc19b6b377e3e))
ROM_END


/* Driver */

/*    YEAR  NAME    PARENT  COMPAT  MACHINE     INPUT       INIT     CONFIG COMPANY                  FULLNAME   FLAGS */
COMP( 1985, sapi1, 	0, 	 	0,		sapi1, 		sapi1, 		sapi1, 	 sapi1,  	"Tesla",					 "SAPI-1",	 GAME_NOT_WORKING)

