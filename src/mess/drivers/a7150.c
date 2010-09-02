/***************************************************************************

    Robotron A7150

    04/10/2009 Skeleton driver.

    http://www.robotrontechnik.de/index.htm?/html/computer/a7150.htm

****************************************************************************/

#include "emu.h"
#include "cpu/i86/i86.h"

static ADDRESS_MAP_START(a7150_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000,0xeffff) AM_RAM
	AM_RANGE(0xf8000,0xfffff) AM_ROM AM_REGION("user1", 0)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( a7150 )
INPUT_PORTS_END


static MACHINE_RESET(a7150)
{
}

static VIDEO_START( a7150 )
{
}

static VIDEO_UPDATE( a7150 )
{
	return 0;
}

static MACHINE_CONFIG_START( a7150, driver_device )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", I8086, 4915000)
	MDRV_CPU_PROGRAM_MAP(a7150_mem)

	MDRV_MACHINE_RESET(a7150)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(a7150)
	MDRV_VIDEO_UPDATE(a7150)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( a7150 )
	ROM_REGION( 0x10000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD( "a7150.rom", 0x0000, 0x8000, CRC(57855abd) SHA1(b58f1363623d2c3ff1221e449529ecaa22573bff))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY           FULLNAME       FLAGS */
COMP( 1986, a7150,  0,      0,       a7150,     a7150,    0,     "VEB Robotron",   "A7150",       GAME_NOT_WORKING | GAME_NO_SOUND)
