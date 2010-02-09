/***************************************************************************

    PROF-180X

    07/07/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(prof180x_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x1fff ) AM_ROM
	AM_RANGE( 0x2000, 0xffff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( prof180x_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( prof180x )
INPUT_PORTS_END


static MACHINE_RESET(prof180x)
{
}

static VIDEO_START( prof180x )
{
}

static VIDEO_UPDATE( prof180x )
{
    return 0;
}

static MACHINE_DRIVER_START( prof180x )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz) // HD64180
    MDRV_CPU_PROGRAM_MAP(prof180x_mem)
    MDRV_CPU_IO_MAP(prof180x_io)

    MDRV_MACHINE_RESET(prof180x)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(prof180x)
    MDRV_VIDEO_UPDATE(prof180x)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( prof180x )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pmon1_3.bin", 0x0000, 0x4000, CRC(32986688) SHA1(a6229d7e66ef699722ca3d41179fe3f1b75185d4))
	ROM_LOAD( "pmon1_4.bin", 0x0000, 0x4000, CRC(ed03f49f) SHA1(e016f9e0b89ab64c6203e2e46501d0b09f74ee9b))
	ROM_LOAD( "pmon1_5.bin", 0x0000, 0x4000, CRC(f43d185c) SHA1(a7a219b3d48c74602b3116cfcd34e44d6e7bc423))
	ROM_LOAD( "pmon.bin", 0x0000, 0x4000, CRC(4f3732d7) SHA1(7dc27262db4e0c8f109470253b9364a216909f2c))
	ROM_LOAD( "eboot1.bin", 0x0000, 0x8000, CRC(7a164b3c) SHA1(69367804b5cbc0633e3d7bbbcc256c2c8c9e7aca))
	ROM_LOAD( "eboot2.bin", 0x0000, 0x8000, CRC(0c2d4301) SHA1(f1a4f457e287b19e14d8ccdbc0383f183d8a3efe))
	ROM_LOAD( "epmon1.bin", 0x0000, 0x10000, CRC(27aabfb4) SHA1(41adf038c474596dbf7d387a1a7f33ed86aa7869))
	ROM_LOAD( "epmon2.bin", 0x0000, 0x10000, CRC(3b8a7b59) SHA1(33741f0725e3eaa21c6881c712579b3c1fd30607))
	ROM_LOAD( "epmon3.bin", 0x0000, 0x10000, CRC(51313af1) SHA1(60c293171a1c7cb9a5ff6d681e61894f44fddbd1))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1986, prof180x,  0,       0, 	prof180x, 	prof180x, 	 0,  "Conitec Datensysteme",   "PROF-180X",		GAME_NOT_WORKING  | GAME_NO_SOUND)
