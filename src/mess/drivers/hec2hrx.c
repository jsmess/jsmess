/***************************************************************************

        Hector 2HRX

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(hec2hrx_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START( hec2hrx_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( hec2hrx )
INPUT_PORTS_END


static MACHINE_RESET(hec2hrx)
{
}

static VIDEO_START( hec2hrx )
{
}

static VIDEO_UPDATE( hec2hrx )
{
    return 0;
}

static MACHINE_DRIVER_START( hec2hrx )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(hec2hrx_mem)
    MDRV_CPU_IO_MAP(hec2hrx_io)

    MDRV_MACHINE_RESET(hec2hrx)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(hec2hrx)
    MDRV_VIDEO_UPDATE(hec2hrx)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(hec2hrx)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( hec2hrx )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
  ROM_LOAD( "hector2hrx.rom", 0x0000, 0x4000, CRC(f047c521) SHA1(744336b2acc76acd7c245b562bdc96dca155b066))

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, hec2hrx,  0,       0, 	hec2hrx, 	hec2hrx, 	 0,  	  hec2hrx,  	 "Micronique",   "Hector 2HRX",		GAME_NOT_WORKING)

