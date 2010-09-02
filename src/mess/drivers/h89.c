/***************************************************************************

        Heathkit H89

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(h89_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x1000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( h89_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( h89 )
INPUT_PORTS_END


static MACHINE_RESET(h89)
{
}

static VIDEO_START( h89 )
{
}

static VIDEO_UPDATE( h89 )
{
    return 0;
}

static MACHINE_CONFIG_START( h89, driver_device )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_12_288MHz / 6)
    MDRV_CPU_PROGRAM_MAP(h89_mem)
    MDRV_CPU_IO_MAP(h89_io)

    MDRV_MACHINE_RESET(h89)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(h89)
    MDRV_VIDEO_UPDATE(h89)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( h89 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "2732_444-142_mtr90.rom", 0x0000, 0x1000, CRC(c4ff47c5) SHA1(d6f3d71ff270a663003ec18a3ed1fa49f627123a))
	ROM_REGION( 0x10000, "otherroms", ROMREGION_ERASEFF )
	ROM_LOAD( "2716_444-19_h17.rom", 0x0000, 0x0800, CRC(26e80ae3) SHA1(0c0ee95d7cb1a760f924769e10c0db1678f2435c))
	ROM_LOAD( "2732_444-84_mtr84.rom", 0x0000, 0x1000, CRC(c98e5f4c) SHA1(03347206dca145ff69ca08435db822b70ce106af))
	ROM_LOAD( "2732_mms84a_magnoliamms.bin", 0x0000, 0x1000, CRC(5563f42a) SHA1(1b74cafca8213d5c083f16d8a848933ab56eb43b))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1979, h89,	0,       0, 		h89,	h89,	 0, 	  "Heath Inc",   "Heathkit H89",		GAME_NOT_WORKING | GAME_NO_SOUND)
