/***************************************************************************

        XOR-100-12

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(xor100_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START( xor100_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( xor100 )
INPUT_PORTS_END


static MACHINE_RESET(xor100)
{
}

static VIDEO_START( xor100 )
{
}

static VIDEO_UPDATE( xor100 )
{
    return 0;
}

static MACHINE_DRIVER_START( xor100 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(xor100_mem, 0)
    MDRV_CPU_IO_MAP(xor100_io, 0)

    MDRV_MACHINE_RESET(xor100)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(xor100)
    MDRV_VIDEO_UPDATE(xor100)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(xor100)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( xor100 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
  ROM_LOAD( "mon185.bin", 0x0000, 0x0800, CRC(0d0bda8d) SHA1(11c83f7cd7e6a570641b44a2f2cc5737a7dd8ae3))

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, xor100,  0,       0, 	xor100, 	xor100, 	 0,  	  xor100,  	 "Unknown",   "XOR-100-12",		GAME_NOT_WORKING)

