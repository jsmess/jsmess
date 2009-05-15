/***************************************************************************

        Interact Family Computer

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(interact_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START( interact_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( interact )
INPUT_PORTS_END


static MACHINE_RESET(interact)
{
}

static VIDEO_START( interact )
{
}

static VIDEO_UPDATE( interact )
{
    return 0;
}

static MACHINE_DRIVER_START( interact )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(interact_mem)
    MDRV_CPU_IO_MAP(interact_io)

    MDRV_MACHINE_RESET(interact)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(interact)
    MDRV_VIDEO_UPDATE(interact)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(interact)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( interact )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
  ROM_LOAD( "interact.rom", 0x0000, 0x0800, CRC(1aa50444) SHA1(405806c97378abcf7c7b0d549430c78c7fc60ba2))

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, interact,  0,       0, 	interact, 	interact, 	 0,  	  interact,  	 "Interact",   "Interact Family Computer",		GAME_NOT_WORKING)

