/***************************************************************************

        Onmibyte MSBC-1

        11/12/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"

static ADDRESS_MAP_START(msbc1_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000000, 0x00007fff) AM_ROM AM_REGION("user1",0)
	AM_RANGE(0x00f80000, 0x00f87fff) AM_ROM AM_REGION("user1",0)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( msbc1 )
INPUT_PORTS_END


static MACHINE_RESET(msbc1)
{
}

static VIDEO_START( msbc1 )
{
}

static VIDEO_UPDATE( msbc1 )
{
    return 0;
}

static MACHINE_DRIVER_START( msbc1 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",M68000, XTAL_8MHz)
    MDRV_CPU_PROGRAM_MAP(msbc1_mem)

    MDRV_MACHINE_RESET(msbc1)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(msbc1)
    MDRV_VIDEO_UPDATE(msbc1)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( msbc1 )
    ROM_REGION( 0x8000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "msbc1_u19.bin", 0x0000, 0x4000, CRC(4814b9e1) SHA1(d96cf75084c6588cb33513830c6beeeffc2de853))
	ROM_LOAD16_BYTE( "msbc1_u18.bin", 0x0001, 0x4000, CRC(14f25d47) SHA1(964bc49c1dd9e9680c0d3d89ff3794c5044bea62))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( ????, msbc1,  0,       0, 	msbc1,	msbc1,	 0,			 "Omnibyte",   "MSBC-1",		GAME_NOT_WORKING | GAME_NO_SOUND)

