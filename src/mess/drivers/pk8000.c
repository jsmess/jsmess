/***************************************************************************
   
        PK-8000

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/i8085/i8085.h"

static ADDRESS_MAP_START(pk8000_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0xffff) AM_RAM	
ADDRESS_MAP_END

static ADDRESS_MAP_START( pk8000_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( pk8000 )
INPUT_PORTS_END


static MACHINE_RESET(pk8000) 
{	
}

static VIDEO_START( pk8000 )
{
}

static VIDEO_UPDATE( pk8000 )
{
    return 0;
}

static MACHINE_DRIVER_START( pk8000 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",8080, 1780000)
    MDRV_CPU_PROGRAM_MAP(pk8000_mem)
    MDRV_CPU_IO_MAP(pk8000_io)	

    MDRV_MACHINE_RESET(pk8000)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(pk8000)
    MDRV_VIDEO_UPDATE(pk8000)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(pk8000)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( vesta )
  ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
  ROM_LOAD( "vesta.rom", 0x0000, 0x4000, CRC(fbf7e2cc) SHA1(4bc5873066124bd926c3c6aa2fd1a062c87af339))
ROM_END

ROM_START( hobby )
  ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
  ROM_LOAD( "hobby.rom", 0x0000, 0x4000, CRC(a25b4b2c) SHA1(0d86e6e4be8d1aa389bfa9dd79e3604a356729f7))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( 1987, vesta,  0,       0, 	pk8000, 	pk8000, 	 0,  	  pk8000,  	 "BP EVM",   "PK8000 Vesta",		GAME_NOT_WORKING)
COMP( 1987, hobby,  vesta,   0, 	pk8000, 	pk8000, 	 0,  	  pk8000,  	 "BP EVM",   "PK8000 Sura/Hobby",		GAME_NOT_WORKING)

