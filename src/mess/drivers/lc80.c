/***************************************************************************

        LC-80

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(lc80_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START( lc80_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( lc80 )
INPUT_PORTS_END


static MACHINE_RESET(lc80)
{
}

static VIDEO_START( lc80 )
{
}

static VIDEO_UPDATE( lc80 )
{
    return 0;
}

static MACHINE_DRIVER_START( lc80 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(lc80_mem)
    MDRV_CPU_IO_MAP(lc80_io)

    MDRV_MACHINE_RESET(lc80)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(lc80)
    MDRV_VIDEO_UPDATE(lc80)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(lc80)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( lc80 )
	/* There seem to be a number of bios versions here...
		Rom 1 and 2 make a set
		Rom 3 no idea where it fits in
		Rom 4 works by itself
		Rom 5 works by itself
		Roms 6 and 7 make a set */
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "lc80__00.rom", 0x0000, 0x0400, CRC(e754ef53) SHA1(044440b13e62addbc3f6a77369cfd16f99b39752))
	ROM_LOAD( "lc80__08.rom", 0x0800, 0x0400, CRC(2b544da1) SHA1(3a6cbd0c57c38eadb7055dca4b396c348567d1d5))
	ROM_LOAD( "lc80_2.bin", 0x0c00, 0x1000, CRC(2e06d768) SHA1(d9cddaf847831e4ab21854c0f895348b7fda20b8))
	ROM_LOAD( "lc80_2716.bin", 0x0000, 0x0800, CRC(b3025934) SHA1(6fff953f0f1eee829fd774366313ab7e8053468c))
	ROM_LOAD( "lc80_u505.bin", 0x0000, 0x1000, CRC(d2d40b03) SHA1(1a18e98b35214c46fe5147ff4a90bdcc18cbb7ab))
	ROM_LOAD( "lc80e_0000.bin", 0x0000, 0x2000, CRC(c98a8f08) SHA1(64c728459181ce163341eb94420e031e30d762b6))
	ROM_LOAD( "lc80e_c000.bin", 0xc000, 0x1000, CRC(9c858d9c) SHA1(2f7b3fd046c965185606253f6cd9372da289ca6f))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, lc80,  0,       0, 	lc80, 	lc80, 	 0,  	  lc80,  	 "Unknown",   "LC-80",		GAME_NOT_WORKING)

