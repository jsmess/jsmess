/***************************************************************************

        QX-10

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(qx10_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x07ff ) AM_ROM
	AM_RANGE( 0x0800, 0x7fff ) AM_RAM
	AM_RANGE( 0x8000, 0x8fff ) AM_ROM
	AM_RANGE( 0x9000, 0xffff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( qx10_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( qx10 )
INPUT_PORTS_END


static MACHINE_RESET(qx10)
{
}

static VIDEO_START( qx10 )
{
}

static VIDEO_UPDATE( qx10 )
{
    return 0;
}

static MACHINE_DRIVER_START( qx10 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(qx10_mem)
    MDRV_CPU_IO_MAP(qx10_io)

    MDRV_MACHINE_RESET(qx10)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(qx10)
    MDRV_VIDEO_UPDATE(qx10)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(qx10)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( qx10 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "qx10boot.bin", 0x0000, 0x0800, CRC(f8dfcba5) SHA1(a7608f8aa7da355dcaf257ee28b66ded8974ce3a))
	/* The first part of this rom looks like code for an embedded controller?
		From 8300 on, looks like a characters generator */
	ROM_LOAD( "mfboard.bin", 0x8000, 0x0800, CRC(fa27f333) SHA1(73d27084ca7b002d5f370220d8da6623a6e82132))
	/* This rom looks like a character generator */
	ROM_LOAD( "qge.bin", 0x8800, 0x0800, CRC(ed93cb81) SHA1(579e68bde3f4184ded7d89b72c6936824f48d10b))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, qx10,  0,       0, 	qx10, 	qx10, 	 0,  	  qx10,  	 "Epson",   "QX-10",		GAME_NOT_WORKING)

