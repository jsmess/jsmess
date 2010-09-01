/***************************************************************************

        Xircom / Intel REX 6000

        18/06/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(rex6000_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0xffff ) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( rex6000_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( rex6000 )
INPUT_PORTS_END


static MACHINE_RESET(rex6000)
{
}

static VIDEO_START( rex6000 )
{
}

static VIDEO_UPDATE( rex6000 )
{
    return 0;
}

static MACHINE_CONFIG_START( rex6000, driver_data_t )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(rex6000_mem)
    MDRV_CPU_IO_MAP(rex6000_io)

    MDRV_MACHINE_RESET(rex6000)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(240, 120)
    MDRV_SCREEN_VISIBLE_AREA(0, 240-1, 0, 120-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(rex6000)
    MDRV_VIDEO_UPDATE(rex6000)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( rex6000 )
    ROM_REGION( 0x200000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "rex6000.dat", 0x0000, 0x200000, CRC(b476f7e0) SHA1(46a56634576408a5b0dca80aea08513e856c3bb1))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 2000, rex6000,  0,       0,	rex6000,	rex6000,	 0,   "Xircom / Intel",   "REX 6000",		GAME_NOT_WORKING | GAME_NO_SOUND)

