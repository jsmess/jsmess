/***************************************************************************

        6809 Protable

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/m6809/m6809.h"

static ADDRESS_MAP_START(d6809_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000,0xdfff) AM_RAM
	AM_RANGE(0xe000,0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( d6809_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( d6809 )
INPUT_PORTS_END


static MACHINE_RESET(d6809)
{
}

static VIDEO_START( d6809 )
{
}

static VIDEO_UPDATE( d6809 )
{
    return 0;
}

static MACHINE_DRIVER_START( d6809 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",M6809E, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(d6809_mem)
    MDRV_CPU_IO_MAP(d6809_io)

    MDRV_MACHINE_RESET(d6809)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(d6809)
    MDRV_VIDEO_UPDATE(d6809)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( d6809 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "d6809.rom", 0xe000, 0x2000, CRC(2ceb40b8) SHA1(780111541234b4f0f781a118d955df61daa56e7e))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 198?, d6809,  0,       0, 	d6809,	d6809,	 0, 		 "Dunfield",   "6809 Portable",		GAME_NOT_WORKING | GAME_NO_SOUND)

