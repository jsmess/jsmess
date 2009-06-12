/***************************************************************************
   
        MMD-1

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/i8085/i8085.h"

static ADDRESS_MAP_START(mmd1_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x00ff ) AM_ROM // Main ROM
	AM_RANGE( 0x0100, 0x01ff ) AM_ROM // Expansion slot
	AM_RANGE( 0x0200, 0x02ff ) AM_RAM
	AM_RANGE( 0x0300, 0x03ff ) AM_RAM	
ADDRESS_MAP_END

static ADDRESS_MAP_START( mmd1_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0x07)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( mmd1 )
INPUT_PORTS_END


static MACHINE_RESET(mmd1) 
{	
}

static VIDEO_START( mmd1 )
{
}

static VIDEO_UPDATE( mmd1 )
{
    return 0;
}

static MACHINE_DRIVER_START( mmd1 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",8080, 6750000 / 9)
    MDRV_CPU_PROGRAM_MAP(mmd1_mem)
    MDRV_CPU_IO_MAP(mmd1_io)	

    MDRV_MACHINE_RESET(mmd1)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(mmd1)
    MDRV_VIDEO_UPDATE(mmd1)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(mmd1)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( mmd1 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
    ROM_LOAD( "kex.ic15", 0x0000, 0x0100, CRC(434f6923) SHA1(a2af60deda54c8d3f175b894b47ff554eb37e9cb))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( 1976, mmd1,  0,       0, 	mmd1, 	mmd1, 	 0,  	  mmd1,  	 "E&L Instruments, Inc.",   "MMD-1",		GAME_NOT_WORKING)

