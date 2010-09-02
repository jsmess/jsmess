/***************************************************************************

        Digital Microsystems DMS-5000

        11/01/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/i86/i86.h"

static ADDRESS_MAP_START(dms5000_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000, 0x1ffff) AM_RAM
	AM_RANGE(0xfc000, 0xfffff) AM_ROM AM_REGION("user1",0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( dms5000_io , ADDRESS_SPACE_IO, 16)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( dms5000 )
INPUT_PORTS_END


static MACHINE_RESET(dms5000)
{
}

static VIDEO_START( dms5000 )
{
}

static VIDEO_UPDATE( dms5000 )
{
    return 0;
}

static MACHINE_CONFIG_START( dms5000, driver_device )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",I8086, XTAL_9_8304MHz)
    MDRV_CPU_PROGRAM_MAP(dms5000_mem)
    MDRV_CPU_IO_MAP(dms5000_io)

    MDRV_MACHINE_RESET(dms5000)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(dms5000)
    MDRV_VIDEO_UPDATE(dms5000)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( dms5000 )
    ROM_REGION( 0x4000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "dms-5000_54-8673o.bin", 0x0001, 0x2000, CRC(dce9823e) SHA1(d36ab87d2e6f5e9f02d59a6a7724ad3ce2428a2f))
	ROM_LOAD16_BYTE( "dms-5000_54-8672e.bin", 0x0000, 0x2000, CRC(94d64c06) SHA1(be5a53da7bb29a5fa9ac31efe550d5d6ff8b77cd))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1982, dms5000,  0,       0,	dms5000,	dms5000,	 0, 	 "Digital Microsystems",   "DMS-5000",		GAME_NOT_WORKING | GAME_NO_SOUND)

