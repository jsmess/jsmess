/***************************************************************************
   
        pipbug

        08/04/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/s2650/s2650.h"

static ADDRESS_MAP_START(pipbug_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x03ff) AM_ROM
	AM_RANGE( 0x0400, 0x7fff) AM_RAM	
ADDRESS_MAP_END

static ADDRESS_MAP_START( pipbug_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH	
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( pipbug )
INPUT_PORTS_END


static MACHINE_RESET(pipbug) 
{	
}

static VIDEO_START( pipbug )
{
}

static VIDEO_UPDATE( pipbug )
{
    return 0;
}

static MACHINE_DRIVER_START( pipbug )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",S2650, XTAL_1MHz)
    MDRV_CPU_PROGRAM_MAP(pipbug_mem)
    MDRV_CPU_IO_MAP(pipbug_io)	

    MDRV_MACHINE_RESET(pipbug)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(pipbug)
    MDRV_VIDEO_UPDATE(pipbug)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( pipbug )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "pipbug.rom", 0x0000, 0x0400, CRC(f242b93e) SHA1(f82857cc882e6b5fc9f00b20b375988024f413ff))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1979, pipbug,  0,       0, 	pipbug, 	pipbug, 	 0,  "<unknown>",   "PIPBUG",		GAME_NOT_WORKING | GAME_NO_SOUND )

