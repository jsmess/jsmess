/***************************************************************************

        SC-80

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(sc80_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x1fff ) AM_ROM
	AM_RANGE( 0x2000, 0xffff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sc80_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( sc80 )
INPUT_PORTS_END


static MACHINE_RESET(sc80)
{
}

static VIDEO_START( sc80 )
{
}

static VIDEO_UPDATE( sc80 )
{
    return 0;
}

static MACHINE_DRIVER_START( sc80 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(sc80_mem)
    MDRV_CPU_IO_MAP(sc80_io)

    MDRV_MACHINE_RESET(sc80)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(sc80)
    MDRV_VIDEO_UPDATE(sc80)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(sc80)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( sc80 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "lc80e-0000-schach.rom", 0x0000, 0x1000, CRC(e3cca61d) SHA1(f2be3f2a9d3780d59657e49b3abeffb0fc13db89))
	ROM_LOAD( "lc80e-1000-schach.rom", 0x1000, 0x1000, CRC(b0323160) SHA1(0ea019b0944736ae5b842bf9aa3537300f259b98))

	ROM_REGION( 0x1000, "gfx1", ROMREGION_ERASEFF )
	ROM_LOAD( "lc80e-c000-schach.rom", 0x0000, 0x1000, CRC(9c858d9c) SHA1(2f7b3fd046c965185606253f6cd9372da289ca6f))

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, sc80,  0,       0, 	sc80, 	sc80, 	 0,  	  sc80,  	 "Unknown",   "SC-80",		GAME_NOT_WORKING)

