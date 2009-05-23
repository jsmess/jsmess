/***************************************************************************

        ET-3400

        12/05/2009 Skeleton driver.

The incorrect processor is being used here.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(et3400_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x03ff ) AM_ROM
	AM_RANGE( 0x0400, 0x3fff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( et3400_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( et3400 )
INPUT_PORTS_END


static MACHINE_RESET(et3400)
{
}

static VIDEO_START( et3400 )
{
}

static VIDEO_UPDATE( et3400 )
{
    return 0;
}

static MACHINE_DRIVER_START( et3400 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(et3400_mem)
    MDRV_CPU_IO_MAP(et3400_io)

    MDRV_MACHINE_RESET(et3400)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(et3400)
    MDRV_VIDEO_UPDATE(et3400)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(et3400)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( et3400 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
  ROM_LOAD( "et3400.bin", 0x0000, 0x0400, CRC(2eff1f58) SHA1(38b655de7393d7a92b08276f7c14a99eaa2a4a9f))

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, et3400,  0,       0, 	et3400, 	et3400, 	 0,  	  et3400,  	 "Heathkit",   "ET-3400",		GAME_NOT_WORKING)

