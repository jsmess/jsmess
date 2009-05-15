/***************************************************************************

        PX-8

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(px8_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START( px8_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( px8 )
INPUT_PORTS_END


static MACHINE_RESET(px8)
{
}

static VIDEO_START( px8 )
{
}

static VIDEO_UPDATE( px8 )
{
    return 0;
}

static MACHINE_DRIVER_START( px8 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(px8_mem)
    MDRV_CPU_IO_MAP(px8_io)

    MDRV_MACHINE_RESET(px8)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(px8)
    MDRV_VIDEO_UPDATE(px8)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(px8)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( px8 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
  ROM_LOAD( "font.rom", 0x0000, 0x0800, CRC(5b52edbd) SHA1(38197edf301bb2843bea040536af545f76b3d44f))
  ROM_LOAD( "sys.rom", 0x0000, 0x8000, CRC(bd3e4938) SHA1(5bd48abd2a563a1ae31ff137280f40c8f756e969))
  ROM_LOAD( "px060688.epr", 0x0000, 0x8000, CRC(44308bdf) SHA1(5c4545fcf1af9931b4699436294d9b6298052a7b))

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, px8,  0,       0, 	px8, 	px8, 	 0,  	  px8,  	 "Epson",   "PX-8",		GAME_NOT_WORKING)

