/***************************************************************************

        Hubler/Everts

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(huebler_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START( huebler_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( huebler )
INPUT_PORTS_END


static MACHINE_RESET(huebler)
{
}

static VIDEO_START( huebler )
{
}

static VIDEO_UPDATE( huebler )
{
    return 0;
}

static MACHINE_DRIVER_START( huebler )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(huebler_mem)
    MDRV_CPU_IO_MAP(huebler_io)

    MDRV_MACHINE_RESET(huebler)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(huebler)
    MDRV_VIDEO_UPDATE(huebler)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(huebler)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( huebler )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	/* Various bioses here, load address of each is shown */
	ROM_LOAD( "mon21.bin",    0xf000, 0x0bdf, CRC(ba905563) SHA1(1fa0aeab5428731756bdfa74efa3c664898bf083))
	ROM_LOAD( "mon30.bin",    0x0000, 0x1000, CRC(033f8112) SHA1(0c6ae7b9d310dec093652db6e8ae84f8ebfdcd29))
	ROM_LOAD( "mon30p_hbasic33p.bin", 0x0000, 0x4800, CRC(c927e7be) SHA1(2d1f3ff4d882c40438a1281872c6037b2f07fdf2))

	ROM_REGION( 0x0400, "gfx1", ROMREGION_ERASEFF )
	ROM_LOAD( "hemcfont.bin", 0x0000, 0x0400, CRC(1074d103) SHA1(e558279cff5744acef4eccf30759a9508b7f8750))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, huebler,  0,       0, 	huebler, 	huebler, 	 0,  	  huebler,  	 "Bernd Hubler, Klaus-Peter Evert",   "Hubler/Everts",		GAME_NOT_WORKING)

