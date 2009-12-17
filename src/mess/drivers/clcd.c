/***************************************************************************

        Commodore LCD prototype

        17/12/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/m6502/m6502.h"

static ADDRESS_MAP_START(clcd_mem, ADDRESS_SPACE_PROGRAM, 8)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( clcd )
INPUT_PORTS_END


static MACHINE_RESET(clcd)
{
}

static VIDEO_START( clcd )
{
}

static VIDEO_UPDATE( clcd )
{
    return 0;
}

static MACHINE_DRIVER_START( clcd )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",M6502, 1000000) // really M65C102
    MDRV_CPU_PROGRAM_MAP(clcd_mem)

    MDRV_MACHINE_RESET(clcd)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(clcd)
    MDRV_VIDEO_UPDATE(clcd)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( clcd )
    ROM_REGION( 0x8000, "maincpu", 0 )
	ROM_LOAD( "kizapr.u102",		0x0000, 0x8000, CRC(59103d52) SHA1(e49c20b237a78b54c2cb26b133d5903bb60bd8ef))
	ROM_LOAD( "sizapr.u103",		0x0000, 0x8000, CRC(0aa91d9f) SHA1(f0842f370607f95d0a0ec6afafb81bc063c32745))
	ROM_LOAD( "sept-m-13apr.u104",	0x0000, 0x8000, CRC(41028c3c) SHA1(fcab6f0bbeef178eb8e5ecf82d9c348d8f318a8f))
	ROM_LOAD( "ss-calc-13apr.u105", 0x0000, 0x8000, CRC(88a587a7) SHA1(b08f3169b7cd696bb6a9b6e6e87a077345377ac4))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( 1985, clcd,  0,       0, 	clcd, 	clcd, 	 0,		0,  	 "Commodore",   "LCD (Prototype)",		GAME_NOT_WORKING)
