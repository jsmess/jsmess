/***************************************************************************

        VCS-80

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(vcs80_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x01ff ) AM_ROM
	AM_RANGE( 0x0200, 0xffff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( vcs80_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( vcs80 )
INPUT_PORTS_END


static MACHINE_RESET(vcs80)
{
}

static VIDEO_START( vcs80 )
{
}

static VIDEO_UPDATE( vcs80 )
{
    return 0;
}

static MACHINE_DRIVER_START( vcs80 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(vcs80_mem)
    MDRV_CPU_IO_MAP(vcs80_io)

    MDRV_MACHINE_RESET(vcs80)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(vcs80)
    MDRV_VIDEO_UPDATE(vcs80)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(vcs80)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( vcs80 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
  ROM_LOAD( "monitor.rom", 0x0000, 0x0200, CRC(44aff4e9) SHA1(3472e5a9357eaba3ed6de65dee2b1c6b29349dd2))

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, vcs80,  0,       0, 	vcs80, 	vcs80, 	 0,  	  vcs80,  	 "Eckhard Schiller",   "VCS-80",		GAME_NOT_WORKING)

