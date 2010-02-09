/***************************************************************************

        UKNC

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/t11/t11.h"

static ADDRESS_MAP_START(uknc_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x7fff ) AM_RAM  // RAM
    AM_RANGE( 0x8000, 0xffff ) AM_ROM  // ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(uknc_sub_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x7fff ) AM_RAM  // RAM
    AM_RANGE( 0x8000, 0xffff ) AM_ROM  // ROM
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( uknc )
INPUT_PORTS_END


static MACHINE_RESET(uknc)
{
}

static VIDEO_START( uknc )
{
}

static VIDEO_UPDATE( uknc )
{
    return 0;
}

static const struct t11_setup t11_data =
{
	0x36ff			/* initial mode word has DAL15,14,11,8 pulled low */
};


static MACHINE_DRIVER_START( uknc )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", T11, 8000000)
	MDRV_CPU_CONFIG(t11_data)
    MDRV_CPU_PROGRAM_MAP(uknc_mem)

    MDRV_CPU_ADD("subcpu",  T11, 6000000)
	MDRV_CPU_CONFIG(t11_data)
    MDRV_CPU_PROGRAM_MAP(uknc_sub_mem)

    MDRV_MACHINE_RESET(uknc)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(uknc)
    MDRV_VIDEO_UPDATE(uknc)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( uknc )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "uknc.rom", 0x8000, 0x8000, CRC(a1536994) SHA1(b3c7c678c41ffa9b37f654fbf20fef7d19e6407b))
	ROM_REGION( 0x10000, "subcpu", ROMREGION_ERASEFF )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1987, uknc,  0,       0, 			uknc, 	uknc, 	 0,  	 "????",   "UKNC",		GAME_NOT_WORKING | GAME_NO_SOUND)

