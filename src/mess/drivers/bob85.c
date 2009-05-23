/***************************************************************************

        BOB85

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(bob85_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x02ff ) AM_ROM
	AM_RANGE( 0x0300, 0x1fff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( bob85_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( bob85 )
INPUT_PORTS_END


static MACHINE_RESET(bob85)
{
}

static VIDEO_START( bob85 )
{
}

static VIDEO_UPDATE( bob85 )
{
    return 0;
}

static MACHINE_DRIVER_START( bob85 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(bob85_mem)
    MDRV_CPU_IO_MAP(bob85_io)

    MDRV_MACHINE_RESET(bob85)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(bob85)
    MDRV_VIDEO_UPDATE(bob85)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(bob85)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( bob85 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "bob85.rom", 0x0000, 0x0300, CRC(adde33a8) SHA1(00f26dd0c52005e7705e6cc9cb11a20e572682c6))
	ROM_FILL(6,1,0)	// there are one or more bad bytes at the start, this fixes the hung state
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, bob85,  0,       0, 	bob85, 	bob85, 	 0,  	  bob85,  	 "Unknown",   "BOB85",		GAME_NOT_WORKING)

