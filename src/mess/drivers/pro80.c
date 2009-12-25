/***************************************************************************

        Protec Pro-80

        06/12/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(pro80_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x03ff) AM_ROM
	AM_RANGE(0x1000, 0x13ff) AM_RAM
	AM_RANGE(0x1400, 0x17ff) AM_RAM // 2nd RAM is optional
ADDRESS_MAP_END

static ADDRESS_MAP_START( pro80_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( pro80 )
INPUT_PORTS_END


static MACHINE_RESET(pro80)
{
}

static VIDEO_START( pro80 )
{
}

static VIDEO_UPDATE( pro80 )
{
    return 0;
}

static MACHINE_DRIVER_START( pro80 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(pro80_mem)
    MDRV_CPU_IO_MAP(pro80_io)

    MDRV_MACHINE_RESET(pro80)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(pro80)
    MDRV_VIDEO_UPDATE(pro80)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( pro80 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	// This rom dump is taken out of manual for this machine
	ROM_LOAD( "pro80.bin", 0x0000, 0x0400, CRC(1bf6e0a5) SHA1(eb45816337e08ed8c30b589fc24960dc98b94db2))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY     FULLNAME       FLAGS */
COMP( 1981, pro80,  0,       0, 	pro80, 		pro80, 	 0,  	 "Protec",   "Pro-80",		GAME_NOT_WORKING)

