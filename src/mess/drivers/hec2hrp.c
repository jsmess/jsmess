/***************************************************************************

        Hector 2HR+

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(hec2hrp_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START( hec2hrp_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( hec2hrp )
INPUT_PORTS_END


static MACHINE_RESET(hec2hrp)
{
}

static VIDEO_START( hec2hrp )
{
}

static VIDEO_UPDATE( hec2hrp )
{
    return 0;
}

static MACHINE_DRIVER_START( hec2hrp )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(hec2hrp_mem)
    MDRV_CPU_IO_MAP(hec2hrp_io)

    MDRV_MACHINE_RESET(hec2hrp)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(hec2hrp)
    MDRV_VIDEO_UPDATE(hec2hrp)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(hec2hrp)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( hec2hrp )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
  ROM_LOAD( "hector2hrp.rom", 0x0000, 0x4000, CRC(983f52e4) SHA1(71695941d689827356042ee52ffe55ce7e6b8ecd))

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, hec2hrp,  0,       0, 	hec2hrp, 	hec2hrp, 	 0,  	  hec2hrp,  	 "Micronique",   "Hector 2HR+",		GAME_NOT_WORKING)

