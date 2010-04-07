/***************************************************************************

    VideoBrain

    07/04/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/f8/f8.h"

static ADDRESS_MAP_START(vidbrain_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x07ff) AM_MIRROR(0xc000) AM_ROM AM_REGION("maincpu", 0)
	AM_RANGE(0x0c00, 0x0fff) AM_RAM
//	AM_RANGE(0x1000, 0x1fff) AM_ROM cartridge
	AM_RANGE(0x2000, 0x27ff) AM_ROM AM_REGION("maincpu", 0x800)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( vidbrain )
INPUT_PORTS_END


static MACHINE_RESET(vidbrain)
{
}

static VIDEO_START( vidbrain )
{
}

static VIDEO_UPDATE( vidbrain )
{
    return 0;
}

static MACHINE_DRIVER_START( vidbrain )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", F8, 1790000)
    MDRV_CPU_PROGRAM_MAP(vidbrain_mem)

    MDRV_MACHINE_RESET(vidbrain)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(vidbrain)
    MDRV_VIDEO_UPDATE(vidbrain)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( vidbrain )
    ROM_REGION( 0x1000, "maincpu", 0 )
	ROM_LOAD( "res1.d67", 0x0000, 0x0800, CRC(065fe7c2) SHA1(9776f9b18cd4d7142e58eff45ac5ee4bc1fa5a2a) )
	ROM_LOAD( "res2.e5",  0x0800, 0x0800, CRC(1d85d7be) SHA1(26c5a25d1289dedf107fa43aa8dfc14692fd9ee6) )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1977, vidbrain,  0,       0,	vidbrain,	vidbrain,	 0,			 "VideoBrain Computer Company",   "VideoBrain Family Computer",		GAME_NOT_WORKING | GAME_NO_SOUND )
