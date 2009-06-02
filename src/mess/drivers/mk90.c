/***************************************************************************

        Elektronika MK-90

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/t11/t11.h"

static ADDRESS_MAP_START(mk90_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x7fff ) AM_RAM
	AM_RANGE( 0x8000, 0xffff ) AM_ROM
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( mk90 )
INPUT_PORTS_END


static MACHINE_RESET(mk90)
{
}

static VIDEO_START( mk90 )
{
}

static VIDEO_UPDATE( mk90 )
{
    return 0;
}

static const struct t11_setup t11_data =
{
	0x36ff			/* initial mode word has DAL15,14,11,8 pulled low */
};

static MACHINE_DRIVER_START( mk90 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",T11, XTAL_4MHz)
    MDRV_CPU_CONFIG(t11_data)
    MDRV_CPU_PROGRAM_MAP(mk90_mem)

    MDRV_MACHINE_RESET(mk90)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(mk90)
    MDRV_VIDEO_UPDATE(mk90)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(mk90)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( mk90 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "rom.bin", 0x8000, 0x8000, CRC(d8b3a5f5) SHA1(8f7ab2d97c7466392b6354c0ea7017531c2133ae))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, mk90,  0,       0, 	mk90, 	mk90, 	 0,  	  mk90,  	 "Elektronika",   "MK-90",		GAME_NOT_WORKING)

