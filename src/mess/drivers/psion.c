/***************************************************************************
   
        Psion Organiser II series

        16/07/2010 Skeleton driver.

		Memory map info :
			http://archive.psion2.org/org2/techref/memory.htm
			
****************************************************************************/

#include "emu.h"
#include "cpu/m6800/m6800.h"

static ADDRESS_MAP_START(psion_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_RAM
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( psion )
INPUT_PORTS_END


static MACHINE_RESET(psion) 
{	
}

static VIDEO_START( psion )
{
}

static VIDEO_UPDATE( psion )
{
    return 0;
}

static MACHINE_DRIVER_START( psion )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",HD63701, 980000) // should be HD6303 at 0.98MHz
    MDRV_CPU_PROGRAM_MAP(psion_mem)

    MDRV_MACHINE_RESET(psion)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(psion)
    MDRV_VIDEO_UPDATE(psion)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( psioncm )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "24-cm.dat",    0x8000, 0x8000,  CRC(f6798394) SHA1(736997f0db9a9ee50d6785636bdc3f8ff1c33c66))
ROM_END

ROM_START( psionla )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "33-la.dat",    0x8000, 0x8000,  CRC(02668ed4) SHA1(e5d4ee6b1cde310a2970ffcc6f29a0ce09b08c46))
ROM_END

ROM_START( psionp350 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "36-p350.dat",  0x8000, 0x8000,  CRC(3a371a74) SHA1(9167210b2c0c3bd196afc08ca44ab23e4e62635e))
	ROM_LOAD( "38-p350.dat",  0x8000, 0x8000,  CRC(1b8b082f) SHA1(a3e875a59860e344f304a831148a7980f28eaa4a))
ROM_END

ROM_START( psionlam )
    ROM_REGION( 0x18000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "37-lam.dat",   0x8000, 0x10000, CRC(7ee3a1bc) SHA1(c7fbd6c8e47c9b7d5f636e9f56e911b363d6796b))	
ROM_END

ROM_START( psionlz64 )
    ROM_REGION( 0x18000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "44-lz64.dat",  0x8000, 0x10000, CRC(aa487913) SHA1(5a44390f63fc8c1bc94299ab2eb291bc3a5b989a))
ROM_END

ROM_START( psionlz64s )
    ROM_REGION( 0x18000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "46-lz64s.dat", 0x8000, 0x10000, CRC(328d9772) SHA1(7f9e2d591d59ecfb0822d7067c2fe59542ea16dd))
ROM_END

ROM_START( psionlz )
    ROM_REGION( 0x18000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "46-lz.dat",    0x8000, 0x10000, CRC(22715f48) SHA1(cf460c81cadb53eddb7afd8dadecbe8c38ea3fc2))
ROM_END

ROM_START( psionp464 )
    ROM_REGION( 0x18000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "46-p464.dat",  0x8000, 0x10000, CRC(672a0945) SHA1(d2a6e3fe1019d1bd7ae4725e33a0b9973f8cd7d8))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT COMPANY   FULLNAME       FLAGS */
COMP( ????, psioncm,  0,       0, 	psion, 		psion, 	 0,   "Psion",   "Organiser II CM",		GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( ????, psionla,  psioncm, 0, 	psion, 		psion, 	 0,   "Psion",   "Organiser II LA",		GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( ????, psionp350, psioncm, 0, 	psion, 		psion, 	 0,   "Psion",   "Organiser II P350",		GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( ????, psionlam,  psioncm, 0, 	psion, 		psion, 	 0,   "Psion",   "Organiser II LAM",		GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( ????, psionlz64,  psioncm, 0, 	psion, 		psion, 	 0,   "Psion",   "Organiser II LZ64",		GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( ????, psionlz64s,  psioncm, 0, 	psion, 		psion, 	 0,   "Psion",   "Organiser II LZ64S",		GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( ????, psionlz,  psioncm, 0, 	psion, 		psion, 	 0,   "Psion",   "Organiser II LZ",		GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( ????, psionp464,  psioncm, 0, 	psion, 		psion, 	 0,   "Psion",   "Organiser II P464",		GAME_NOT_WORKING | GAME_NO_SOUND)
