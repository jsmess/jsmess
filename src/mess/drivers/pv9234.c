/***************************************************************************

        Scientific Atlanta PowerVu pv9234 STB

        20/03/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/arm7/arm7.h"

static ADDRESS_MAP_START(pv9234_map, ADDRESS_SPACE_PROGRAM, 32)
	AM_RANGE(0x00000,0x7ffff) AM_ROM
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( pv9234 )
INPUT_PORTS_END


static MACHINE_RESET(pv9234)
{
}

static VIDEO_START( pv9234 )
{
}

static VIDEO_UPDATE( pv9234 )
{
    return 0;
}

static MACHINE_DRIVER_START( pv9234 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", ARM7, 4915000) //probably a more powerful clone.
    MDRV_CPU_PROGRAM_MAP(pv9234_map)

    MDRV_MACHINE_RESET(pv9234)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(pv9234)
    MDRV_VIDEO_UPDATE(pv9234)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( pv9234 )
    ROM_REGION( 0x80000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "u19.bin", 0x00000, 0x20000, CRC(1e06b0c8) SHA1(f8047f7127919e73675375578bb9fcc0eed2178e))
	ROM_LOAD16_BYTE( "u18.bin", 0x00001, 0x20000, CRC(924487dd) SHA1(fb1d7c9a813ded8c820589fa85ae72265a0427c7))
	ROM_LOAD16_BYTE( "u17.bin", 0x40000, 0x20000, CRC(cac03650) SHA1(edd8aec6fed886d47de39ed4e127de0a93250a45))
	ROM_LOAD16_BYTE( "u16.bin", 0x40001, 0x20000, CRC(bd07d545) SHA1(90a63af4ee82b0f7d0ed5f0e09569377f22dd98c))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1997, pv9234,  0,       0, 	pv9234,	pv9234,	 0, 		 "Scientific Atlanta",   "PowerVu D9234",		GAME_NOT_WORKING | GAME_NO_SOUND)

