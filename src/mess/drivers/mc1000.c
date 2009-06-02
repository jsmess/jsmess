/***************************************************************************

        MC 1000

        12/05/2009 Skeleton driver.

This is running - look at memory at 8000. There's even a flashing cursor.

The display looks like 32x16 although memory from 8000-97FE is filled with
spaces. The character generator rom is missing - unless it uses a 6847.
This is a color computer.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(mc1000_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0xbfff ) AM_RAM
	AM_RANGE( 0xc000, 0xffff ) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( mc1000_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( mc1000 )
INPUT_PORTS_END


static MACHINE_RESET(mc1000)
{
}

static VIDEO_START( mc1000 )
{
}

static VIDEO_UPDATE( mc1000 )
{
    return 0;
}

static MACHINE_DRIVER_START( mc1000 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(mc1000_mem)
    MDRV_CPU_IO_MAP(mc1000_io)

    MDRV_MACHINE_RESET(mc1000)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(mc1000)
    MDRV_VIDEO_UPDATE(mc1000)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(mc1000)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( mc1000 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "mc1000.rom", 0xc000, 0x4000, CRC(d46f67d5) SHA1(48a2539506a24ea7478cb20ae1ea1226e2f670c4))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, mc1000,  0,       0, 	mc1000, 	mc1000, 	 0,  	  mc1000,  	 "CCE",   "MC-1000",		GAME_NOT_WORKING)

