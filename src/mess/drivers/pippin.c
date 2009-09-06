/***************************************************************************
   
        Bandai Pippin

        17/07/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/powerpc/ppc.h"

static ADDRESS_MAP_START( pippin_mem, ADDRESS_SPACE_PROGRAM, 64 )
	ADDRESS_MAP_UNMAP_HIGH	
	AM_RANGE(0x00000000, 0x005FFFFF) AM_RAM
	AM_RANGE(0xFFC00000, 0xFFFFFFFF) AM_ROM AM_REGION("user1",0)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( pippin )
INPUT_PORTS_END


static MACHINE_RESET(pippin) 
{	
}

static VIDEO_START( pippin )
{
}

static VIDEO_UPDATE( pippin )
{
    return 0;
}

static MACHINE_DRIVER_START( pippin )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",PPC603, 66000000)
    MDRV_CPU_PROGRAM_MAP(pippin_mem)

    MDRV_MACHINE_RESET(pippin)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(pippin)
    MDRV_VIDEO_UPDATE(pippin)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(pippin)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( pippin )
    ROM_REGION( 0x400000, "user1",  ROMREGION_64BIT | ROMREGION_BE )
	ROM_LOAD64_WORD_SWAP( "u1", 0x000006, 0x100000, CRC(aaea2449) SHA1(2f63e215260a42fb7c5f2364682d5e8c0604646f) )
	ROM_LOAD64_WORD_SWAP( "u2", 0x000004, 0x100000, CRC(3d584419) SHA1(e29c764816755662693b25f1fb3c24faef4e9470) )
	ROM_LOAD64_WORD_SWAP( "u3", 0x000002, 0x100000, CRC(d8ae5037) SHA1(d46ce4d87ca1120dfe2cf2ba01451f035992b6f6) )
	ROM_LOAD64_WORD_SWAP( "u4", 0x000000, 0x100000, CRC(3e2851ba) SHA1(7cbf5d6999e890f5e9ab2bc4b10ca897c4dc2016) )	

	ROM_REGION( 0x10000, "cdrom", 0 )
	ROM_LOAD( "504par4.0i.ic7", 0x0000, 0x10000, CRC(25f7dd46) SHA1(ec3b3031742807924c6259af865e701827208fec) )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( 1996, pippin,  0,       0, 	pippin, 	pippin, 	 0,  	  pippin,  	 "Apple/Bandai",   "Pippin @mark",		GAME_NOT_WORKING)

