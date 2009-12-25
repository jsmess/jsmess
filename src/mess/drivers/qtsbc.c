/***************************************************************************

        QT Computer Systems SBC +2/4

        11/12/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(qtsbc_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x07ff ) AM_ROM
	AM_RANGE( 0x0800, 0xffff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( qtsbc_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( qtsbc )
INPUT_PORTS_END


static MACHINE_RESET(qtsbc)
{
}

static VIDEO_START( qtsbc )
{
}

static VIDEO_UPDATE( qtsbc )
{
    return 0;
}

static MACHINE_DRIVER_START( qtsbc )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz) // Mostek MK3880
    MDRV_CPU_PROGRAM_MAP(qtsbc_mem)
    MDRV_CPU_IO_MAP(qtsbc_io)

    MDRV_MACHINE_RESET(qtsbc)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(qtsbc)
    MDRV_VIDEO_UPDATE(qtsbc)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( qtsbc )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "qtsbc.bin", 0x0000, 0x0800, CRC(823fd942) SHA1(64c4f74dd069ae4d43d301f5e279185f32a1efa0))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( ????, qtsbc,  0,       0, 	qtsbc, 	qtsbc, 	 0,  	   	 "Computer Systems Inc.",   "QT SBC +2/4",		GAME_NOT_WORKING)
