/***************************************************************************

        JR-200

        12/05/2009 Skeleton driver.

	The incorrect processor is used here.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(jr200_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START( jr200_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( jr200 )
INPUT_PORTS_END


static MACHINE_RESET(jr200)
{
}

static VIDEO_START( jr200 )
{
}

static VIDEO_UPDATE( jr200 )
{
    return 0;
}

static MACHINE_DRIVER_START( jr200 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(jr200_mem)
    MDRV_CPU_IO_MAP(jr200_io)

    MDRV_MACHINE_RESET(jr200)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(jr200)
    MDRV_VIDEO_UPDATE(jr200)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(jr200)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( jr200 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "basic.rom", 0x0000, 0x2000, CRC(cc53eb52) SHA1(910927b98a8338ba072173d79613422a8cb796da))
	ROM_LOAD( "jr200u.bin", 0x2000, 0x2000, CRC(37ca3080) SHA1(17d3fdedb4de521da7b10417407fa2b61f01a77a))

	ROM_REGION( 0x0800, "char", ROMREGION_ERASEFF )
	ROM_LOAD( "char.rom", 0x0000, 0x0800, CRC(cb641624) SHA1(6fe890757ebc65bbde67227f9c7c490d8edd84f2))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, jr200,  0,       0, 	jr200, 	jr200, 	 0,  	  jr200,  	 "National",   "JR-200",		GAME_NOT_WORKING)
