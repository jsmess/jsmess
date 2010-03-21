/***************************************************************************
   
        Dream Multimedia Dreambox 7000/5620/500

        20/03/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/powerpc/ppc.h"

static ADDRESS_MAP_START( dm7000_mem, ADDRESS_SPACE_PROGRAM, 64 )
	ADDRESS_MAP_UNMAP_HIGH	
	AM_RANGE(0x00000000, 0x0000ffff) AM_RAM	 // placed for driver not to crash
	AM_RANGE(0xa0000000, 0xa000ffff) AM_RAM	 // placed for driver not to crash on real system 64MB of RAM
	AM_RANGE(0xfff00000, 0xfff1ffff) AM_ROM AM_REGION("user1",0)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( dm7000 )
INPUT_PORTS_END


static MACHINE_RESET(dm7000) 
{	
}

static VIDEO_START( dm7000 )
{
}

static VIDEO_UPDATE( dm7000 )
{
    return 0;
}

static MACHINE_DRIVER_START( dm7000 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",PPC603, 66000000)
    MDRV_CPU_PROGRAM_MAP(dm7000_mem)

    MDRV_MACHINE_RESET(dm7000)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(dm7000)
    MDRV_VIDEO_UPDATE(dm7000)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( dm7000 )
    ROM_REGION( 0x20000, "user1", ROMREGION_64BIT | ROMREGION_BE )
	ROM_LOAD( "dm7000.bin", 0x0000, 0x20000, CRC(8a410f67) SHA1(9d6c9e4f5b05b28453d3558e69a207f05c766f54))
ROM_END

ROM_START( dm5620 )
    ROM_REGION( 0x20000, "user1", ROMREGION_64BIT | ROMREGION_BE )
	ROM_LOAD( "dm5620.bin", 0x0000, 0x20000, CRC(ccddb822) SHA1(3ecf553ced0671599438368f59d8d30df4d13ade))
ROM_END

ROM_START( dm500 )
    ROM_REGION( 0x20000, "user1", ROMREGION_64BIT | ROMREGION_BE )
	ROM_SYSTEM_BIOS( 0, "alps", "Alps" )
    ROMX_LOAD( "dm500-alps-boot.bin",   0x0000, 0x20000, CRC(daf2da34) SHA1(68f3734b4589fcb3e73372e258040bc8b83fd739), ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "phil", "Philips" )
    ROMX_LOAD( "dm500-philps-boot.bin", 0x0000, 0x20000, CRC(af3477c7) SHA1(9ac918f6984e6927f55bea68d6daaf008787136e), ROM_GROUPWORD | ROM_REVERSE | ROM_BIOS(2))
ROM_END
/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( ????, dm7000,  0,       0, 	dm7000, 	dm7000, 	 0,   "Dream Multimedia",   "Dreambox 7000",		GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( ????, dm5620,  dm7000,  0, 	dm7000, 	dm7000, 	 0,   "Dream Multimedia",   "Dreambox 5620",		GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( ????, dm500,   dm7000,  0, 	dm7000, 	dm7000, 	 0,   "Dream Multimedia",   "Dreambox 500",			GAME_NOT_WORKING | GAME_NO_SOUND)

