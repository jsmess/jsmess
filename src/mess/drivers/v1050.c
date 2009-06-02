/***************************************************************************

        Visual 1050

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(v1050_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x3fff ) AM_ROM
	AM_RANGE( 0x4000, 0xffff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( v1050_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( v1050 )
INPUT_PORTS_END


static MACHINE_RESET(v1050)
{
}

static VIDEO_START( v1050 )
{
}

static VIDEO_UPDATE( v1050 )
{
    return 0;
}

static MACHINE_DRIVER_START( v1050 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(v1050_mem)
    MDRV_CPU_IO_MAP(v1050_io)

    MDRV_MACHINE_RESET(v1050)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(v1050)
    MDRV_VIDEO_UPDATE(v1050)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(v1050)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( v1050 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
  ROM_LOAD( "visual_u77.bin", 0x2000, 0x2000, CRC(c0502b66) SHA1(bc0015f5b14f98110e652eef9f7c57c614683be5))
  ROM_LOAD( "visual_u86.bin", 0x0000, 0x2000, CRC(46f847a7) SHA1(374db7a38a9e9230834ce015006e2f1996b9609a))

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, v1050,  0,       0, 	v1050, 	v1050, 	 0,  	  v1050,  	 "Visual Technology Incorporated",   "Visual 1050",		GAME_NOT_WORKING)

