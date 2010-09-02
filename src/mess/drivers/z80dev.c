/***************************************************************************

        Z80 dev board (uknown)

        http://retro.hansotten.nl/index.php?page=z80-dev-kit

        23/04/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(z80dev_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x07ff) AM_ROM
	AM_RANGE(0x1000, 0x10ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( z80dev_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( z80dev )
INPUT_PORTS_END


static MACHINE_RESET(z80dev)
{
}

static VIDEO_START( z80dev )
{
}

static VIDEO_UPDATE( z80dev )
{
    return 0;
}

static MACHINE_CONFIG_START( z80dev, driver_device )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(z80dev_mem)
    MDRV_CPU_IO_MAP(z80dev_io)

    MDRV_MACHINE_RESET(z80dev)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(z80dev)
    MDRV_VIDEO_UPDATE(z80dev)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( z80dev )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "z80dev.bin", 0x0000, 0x0800, CRC(dd5b9cd9) SHA1(97c176fcb63674f0592851b7858cb706886b5857))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 198?, z80dev,  0,       0,	z80dev, 	z80dev, 	 0,  "<unknown>",   "Z80 dev board",		GAME_NOT_WORKING | GAME_NO_SOUND)

