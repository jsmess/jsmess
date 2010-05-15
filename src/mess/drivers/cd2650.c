/***************************************************************************

        cd2650

        08/04/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/s2650/s2650.h"

static ADDRESS_MAP_START(cd2650_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x03ff) AM_ROM
	AM_RANGE( 0x0400, 0x7fff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( cd2650_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( cd2650 )
INPUT_PORTS_END


static MACHINE_RESET(cd2650)
{
}

static VIDEO_START( cd2650 )
{
}

static VIDEO_UPDATE( cd2650 )
{
    return 0;
}

static MACHINE_DRIVER_START( cd2650 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",S2650, XTAL_1MHz)
    MDRV_CPU_PROGRAM_MAP(cd2650_mem)
    MDRV_CPU_IO_MAP(cd2650_io)

    MDRV_MACHINE_RESET(cd2650)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(cd2650)
    MDRV_VIDEO_UPDATE(cd2650)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( cd2650 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "cd2650.rom", 0x0000, 0x0400, CRC(5397328e) SHA1(7106fdb60e1ad2bc5e8e45527f348c23296e8d6a))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1977, cd2650,  0,       0,	cd2650, 	cd2650, 	 0,   "Central Data",   "CD 2650",		GAME_NOT_WORKING | GAME_NO_SOUND )

