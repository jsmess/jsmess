/***************************************************************************

        Acorn 6809

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"

static ADDRESS_MAP_START(a6809_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000,0xf7ff) AM_RAM
	AM_RANGE(0x0400,0x07ff) AM_BASE(&videoram)
	AM_RANGE(0xf800,0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( a6809_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( a6809 )
INPUT_PORTS_END


static MACHINE_RESET(a6809)
{
}

static VIDEO_START( a6809 )
{
}

static VIDEO_UPDATE( a6809 )
{
    return 0;
}

static MACHINE_DRIVER_START( a6809 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",M6809E, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(a6809_mem)
    MDRV_CPU_IO_MAP(a6809_io)

    MDRV_MACHINE_RESET(a6809)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(a6809)
    MDRV_VIDEO_UPDATE(a6809)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(a6809)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( a6809 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "acorn6809.bin", 0xf800, 0x0800, CRC(5fa5b632) SHA1(b14a884bf82a7a8c23bc03c2e112728dd1a74896) )
	/* This looks like some sort of prom */
	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "acorn6809.ic11", 0x0000, 0x0100, CRC(7908317d) SHA1(e0f1e5bd3a8598d3b62bc432dd1f3892ed7e66d8) )
	/* Character generator is missing */
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, a6809,  0,       0, 	a6809, 	a6809, 	 0,  	  a6809,  	 "Acorn",   "6809",		GAME_NOT_WORKING)

