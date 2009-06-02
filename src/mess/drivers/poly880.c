/***************************************************************************

        Poly-Computer 880

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(poly880_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x0fff ) AM_ROM
	AM_RANGE( 0x1000, 0xffff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( poly880_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( poly880 )
INPUT_PORTS_END


static MACHINE_RESET(poly880)
{
}

static VIDEO_START( poly880 )
{
}

static VIDEO_UPDATE( poly880 )
{
    return 0;
}

static MACHINE_DRIVER_START( poly880 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(poly880_mem)
    MDRV_CPU_IO_MAP(poly880_io)

    MDRV_MACHINE_RESET(poly880)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(poly880)
    MDRV_VIDEO_UPDATE(poly880)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(poly880)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( poly880 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "poly880_0000.bin", 0x0000, 0x0400, CRC(b1c571e8) SHA1(85bfe53d39d6690e79999a1e1240789497e72db0))
	ROM_LOAD( "poly880_1000.bin", 0x0000, 0x0400, CRC(9efddf5b) SHA1(6ffa2f80b2c6f8ec9e22834f739c82f9754272b8))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, poly880,  0,       0, 	poly880, 	poly880, 	 0,  	  poly880,  	 "VEB Polytechnik",   "Poly-Computer 880",		GAME_NOT_WORKING)

