/***************************************************************************

        Tiki 100

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(tiki100_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x5fff ) AM_ROM
	AM_RANGE( 0x6000, 0xffff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( tiki100_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( tiki100 )
INPUT_PORTS_END


static MACHINE_RESET(tiki100)
{
}

static VIDEO_START( tiki100 )
{
}

static VIDEO_UPDATE( tiki100 )
{
    return 0;
}

static MACHINE_DRIVER_START( tiki100 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(tiki100_mem)
    MDRV_CPU_IO_MAP(tiki100_io)

    MDRV_MACHINE_RESET(tiki100)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(tiki100)
    MDRV_VIDEO_UPDATE(tiki100)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(tiki100)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( tiki100 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
  ROM_LOAD( "tikirom-1.30", 0x0000, 0x2000, CRC(c482dcaf) SHA1(d140706bb7fc8b1fbb37180d98921f5bdda73cf9))
  ROM_LOAD( "tikirom-1.35", 0x2000, 0x2000, CRC(7dac5ee7) SHA1(14d622fd843833faec346bf5357d7576061f5a3d))
  ROM_LOAD( "tikirom-2.03w", 0x4000, 0x2000, CRC(79662476) SHA1(96336633ecaf1b2190c36c43295ac9f785d1f83a))

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, tiki100,  0,       0, 	tiki100, 	tiki100, 	 0,  	  tiki100,  	 "Tiki Data",   "Tiki 100",		GAME_NOT_WORKING)

