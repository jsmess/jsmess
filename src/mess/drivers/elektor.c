/***************************************************************************

        elektor

        08/04/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/s2650/s2650.h"

static ADDRESS_MAP_START(elektor_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x07ff) AM_ROM
	AM_RANGE( 0x0800, 0x7fff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( elektor_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( elektor )
INPUT_PORTS_END


static MACHINE_RESET(elektor)
{
}

static VIDEO_START( elektor )
{
}

static VIDEO_UPDATE( elektor )
{
    return 0;
}

static MACHINE_DRIVER_START( elektor )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",S2650, XTAL_1MHz)
    MDRV_CPU_PROGRAM_MAP(elektor_mem)
    MDRV_CPU_IO_MAP(elektor_io)

    MDRV_MACHINE_RESET(elektor)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(elektor)
    MDRV_VIDEO_UPDATE(elektor)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( elektor )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "elektor.rom", 0x0000, 0x0800, CRC(e6ef1ee1) SHA1(6823b5a22582344016415f2a37f9f3a2dc75d2a7))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1979, elektor,  0,       0,	elektor,	elektor,	 0,   "Elektor",   "Elektor TV Games Computer",		GAME_NOT_WORKING | GAME_NO_SOUND )

