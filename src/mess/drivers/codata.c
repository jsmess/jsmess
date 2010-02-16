/***************************************************************************

        Contel Codata Corporation Codata

        11/01/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"


static ADDRESS_MAP_START(codata_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( codata )
INPUT_PORTS_END


static MACHINE_RESET(codata)
{
}

static VIDEO_START( codata )
{
}

static VIDEO_UPDATE( codata )
{
    return 0;
}

static MACHINE_DRIVER_START( codata )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",M68000, XTAL_16MHz / 2)
    MDRV_CPU_PROGRAM_MAP(codata_mem)

    MDRV_MACHINE_RESET(codata)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(codata)
    MDRV_VIDEO_UPDATE(codata)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( codata )
    ROM_REGION( 0x8000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "27-0042-01a.u101", 0x0000, 0x2000, CRC(70014b16) SHA1(19a82000894d79817358d40ae520200e976be310))
	ROM_LOAD16_BYTE( "27-0043-01a.u102", 0x4000, 0x2000, CRC(fca9c314) SHA1(2f8970fad479000f28536003867066d6df9e33d9))
	ROM_LOAD16_BYTE( "27-0044-01a.u103", 0x0001, 0x2000, CRC(dc5d5cea) SHA1(b3e9248abf89d674c463d21d2f7be34508cf16c2))
	ROM_LOAD16_BYTE( "27-0045-01a.u104", 0x4001, 0x2000, CRC(a937e7b3) SHA1(d809bbd437fe7d925325958072b9e0dc33dd36a6))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( ????, codata,  0,       0,	codata, 	codata, 	 0,   "Contel Codata Corporation",   "Codata",		GAME_NOT_WORKING | GAME_NO_SOUND)

