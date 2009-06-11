/***************************************************************************

    Casio X-07

    05/2009 Skeleton driver

    CPU was actually a NCS800 (Z80 compatible)
    More info: http://www.silicium.org/calc/x07/

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(x07_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0xb000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( x07_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( x07 )
INPUT_PORTS_END


static MACHINE_RESET( x07 )
{
}

static VIDEO_START( x07 )
{
}

static VIDEO_UPDATE( x07 )
{
	return 0;
}

static MACHINE_DRIVER_START( x07 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, XTAL_4MHz)
	MDRV_CPU_PROGRAM_MAP(x07_mem)
	MDRV_CPU_IO_MAP(x07_io)

	MDRV_MACHINE_RESET(x07)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(x07)
	MDRV_VIDEO_UPDATE(x07)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(x07)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( x07 )
	/* very strange size... */
	ROM_REGION( 0x11000, "maincpu", 0 )
	ROM_LOAD( "x07.bin", 0xb000, 0x5001, CRC(61a6e3cc) SHA1(c53c22d33085ac7d5e490c5d8f41207729e5f08a) )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME    FLAGS */
COMP( 1983, x07,    0,      0,       x07,       x07,     0,      x07,   "Canon",  "X-07",     GAME_NOT_WORKING)
