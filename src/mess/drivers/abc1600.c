#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "machine/abc99.h"
#include "devices/messram.h"

#define M68008_TAG "m68008"

static ADDRESS_MAP_START( abc1600_mem, ADDRESS_SPACE_PROGRAM, 8 )
ADDRESS_MAP_END

static INPUT_PORTS_START( abc1600 )
INPUT_PORTS_END

static VIDEO_UPDATE( abc1600 )
{
	return 0;
}

static MACHINE_CONFIG_START( abc1600, driver_device )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68008_TAG, M68008, 8000000)
	MDRV_CPU_PROGRAM_MAP(abc1600_mem)

	/* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(1024, 768)
    MDRV_SCREEN_VISIBLE_AREA(0, 1024-1, 0, 768-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_UPDATE(abc1600)

	/* sound hardware */

	/* devices */
	MDRV_ABC99_ADD

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("1M")
MACHINE_CONFIG_END

ROM_START( abc1600 )
	ROM_REGION( 0x4000, M68008_TAG, 0 )
	ROM_LOAD( "bios", 0x0000, 0x4000, NO_DUMP )
ROM_END

/*    YEAR  NAME      PARENT  COMPAT  MACHINE   INPUT     INIT  COMPANY     FULLNAME     FLAGS */
COMP( 1985, abc1600, 0,      0,      abc1600, abc1600, 0,    "Luxor", "ABC 1600", GAME_NOT_WORKING | GAME_NO_SOUND )
