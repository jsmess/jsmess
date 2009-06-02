/***************************************************************************

        C-80

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(c80_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x03ff ) AM_ROM
	AM_RANGE( 0x0400, 0xffff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( c80_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( c80 )
INPUT_PORTS_END


static MACHINE_RESET(c80)
{
}

static VIDEO_START( c80 )
{
}

static VIDEO_UPDATE( c80 )
{
    return 0;
}

static MACHINE_DRIVER_START( c80 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(c80_mem)
    MDRV_CPU_IO_MAP(c80_io)

    MDRV_MACHINE_RESET(c80)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(c80)
    MDRV_VIDEO_UPDATE(c80)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(c80)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( c80 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "c80.rom", 0x0000, 0x0400, CRC(ad2b3296) SHA1(14f72cb73a4068b7a5d763cc0e254639c251ce2e))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, c80,  0,       0, 	c80, 	c80, 	 0,  	  c80,  	 "Joachim Czepa",   "C-80",		GAME_NOT_WORKING)

