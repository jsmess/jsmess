/***************************************************************************

        Schachcomputer SC2

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(sc2_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x0fff ) AM_ROM
	AM_RANGE( 0x1000, 0x1fff ) AM_RAM
	AM_RANGE( 0x2000, 0x33ff ) AM_ROM
	AM_RANGE( 0x3400, 0x3fff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sc2_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( sc2 )
INPUT_PORTS_END


static MACHINE_RESET(sc2)
{
}

static VIDEO_START( sc2 )
{
}

static VIDEO_UPDATE( sc2 )
{
    return 0;
}

static MACHINE_DRIVER_START( sc2 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(sc2_mem)
    MDRV_CPU_IO_MAP(sc2_io)

    MDRV_MACHINE_RESET(sc2)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(sc2)
    MDRV_VIDEO_UPDATE(sc2)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(sc2)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( sc2 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "bm008.bin", 0x0000, 0x0400, CRC(3023ea82) SHA1(07020d153d802c672c39e1af3c716dbe35e23f08))
	ROM_LOAD( "bm009.bin", 0x0400, 0x0400, CRC(6a34814e) SHA1(e58ae6615297b028db135a48a8f9e186a4220f4f))
	ROM_LOAD( "bm010.bin", 0x0800, 0x0400, CRC(deab0373) SHA1(81c9a7197eef8d9131e47ecd2ec35b943caee54e))
	ROM_LOAD( "bm011.bin", 0x0c00, 0x0400, CRC(c8282339) SHA1(8d6b8861281e967a77609b6d77e80afd47d28ed2))
	ROM_LOAD( "bm012.bin", 0x2000, 0x0400, CRC(2e6a4294) SHA1(7b9bd191c9ec73139a65c3a339ab88e1f3eb5ed2))
	ROM_LOAD( "bm013.bin", 0x2400, 0x0400, CRC(3e02eb42) SHA1(2e4a9a8fd04c202c9518550d7e8cf9bfea394153))
	ROM_LOAD( "bm014.bin", 0x2800, 0x0400, CRC(538d449e) SHA1(c4186995b69e97740e01eaff84a20d49d03d180f))
	ROM_LOAD( "bm015.bin", 0x2c00, 0x0400, CRC(b4991dca) SHA1(6a6cdddf5c4afa24773acf693f58c34b99c8d328))
	ROM_LOAD( "bm016.bin", 0x3000, 0x0400, CRC(4fe0853a) SHA1(c2253e320778b0ea468fb54f26ae83d07f9700e6))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, sc2,  0,       0, 	sc2, 	sc2, 	 0,  	  sc2,  	 "VEB Mikroelektronik Erfurt",   "Schachcomputer SC2",		GAME_NOT_WORKING)

