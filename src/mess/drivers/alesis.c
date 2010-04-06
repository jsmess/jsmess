/***************************************************************************
   
        Alesis HR-16 and SR-16 drum machines

        06/04/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/mcs51/mcs51.h"

static ADDRESS_MAP_START(hr16_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( hr16_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START(sr16_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sr16_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( alesis )
INPUT_PORTS_END


static MACHINE_RESET(alesis) 
{	
}

static VIDEO_START( alesis )
{
}

static VIDEO_UPDATE( alesis )
{
    return 0;
}

static DRIVER_INIT( hr16 )
{
	int i;
	UINT8 *ROM = memory_region(machine, "maincpu");
	UINT8 *orig = memory_region(machine, "user1");
	for (i = 0; i < 0x8000; i++)
	{
		ROM[BITSWAP16(i,15,14,13,12,11,10,9,8,0,1,2,3,4,5,6,7)] = orig[i];
	}
}
static MACHINE_DRIVER_START( hr16 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",I8031, XTAL_12MHz)
    MDRV_CPU_PROGRAM_MAP(hr16_mem)
    MDRV_CPU_IO_MAP(hr16_io)	

    MDRV_MACHINE_RESET(alesis)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(alesis)
    MDRV_VIDEO_UPDATE(alesis)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( sr16 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",I8031, XTAL_12MHz)
    MDRV_CPU_PROGRAM_MAP(sr16_mem)
    MDRV_CPU_IO_MAP(sr16_io)	

    MDRV_MACHINE_RESET(alesis)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(alesis)
    MDRV_VIDEO_UPDATE(alesis)
MACHINE_DRIVER_END
/* ROM definition */
ROM_START( hr16 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_REGION( 0x10000, "user1", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "v107", "ver 1.07")
	ROMX_LOAD( "2-19-0256-v107.u11",  0x0000, 0x8000, CRC(2582b6a2) SHA1(f1f135335578c938be63b37ed207e82b7a0e13be), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "v109", "ver 1.09")
	ROMX_LOAD( "2-19-0256-v109.u11",  0x0000, 0x8000, CRC(a9bdbf20) SHA1(229b4230c7b5380efbfd42fa95645723d3fd6d55), ROM_BIOS(2))
	ROM_REGION( 0x100000, "sounddata", ROMREGION_ERASEFF )
	ROM_LOAD( "2-27-0003.u15", 0x00000, 0x80000, CRC(82e9b78c) SHA1(89728cb38ae172b5e347a03018617c94a087dce0))
	ROM_LOAD( "2-27-0004.u16", 0x80000, 0x80000, CRC(8e103536) SHA1(092e1cf649fbef171cfaf91e20707d89998b7a1e))
ROM_END

ROM_START( hr16b )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )	
	ROM_REGION( 0x10000, "user1", ROMREGION_ERASEFF )
	// version 2.00
	ROM_LOAD( "2-19-0256-v200.u11",0x0000,  0x8000, CRC(19cf0fce) SHA1(f8b3786b32d68e3627a654b8b3916befbe9bc540))
	ROM_REGION( 0x100000, "sounddata", ROMREGION_ERASEFF )
	ROM_LOAD( "2-27-0007.u15", 0x00000, 0x80000, CRC(319746db) SHA1(46b32a3ab2fbad67fb4566f607f578a2e9defd63))
	ROM_LOAD( "2-27-0008.u16", 0x80000, 0x80000, CRC(11ca930e) SHA1(2f57fdd02f9b2146a551370a74cab1fa800145ab))
ROM_END

ROM_START( sr16 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	// version 1.04
	ROM_LOAD( "sr16_v1_04.bin", 0x0000, 0x10000, CRC(d049af6e) SHA1(0bbeb4bd25e33a9eca64d5a31480f96a0040617e))
	ROM_REGION( 0x100000, "sounddata", ROMREGION_ERASEFF )
	ROM_LOAD( "sr16.u5", 0x00000, 0x80000, CRC(8bb25cfa) SHA1(273ad59d017b54a7e8d5e1ec61c8cd807a0e4af3))
	ROM_LOAD( "sr16.u6", 0x80000, 0x80000, CRC(6da96987) SHA1(3ec8627d440bc73841e1408a19def09a8b0b77f7))	
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( ????, hr16,  0,       0, 		hr16, 	alesis, 	 hr16, 	 "Alesis",   "HR-16",		GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( ????, hr16b, hr16,    0, 		hr16, 	alesis, 	 hr16, 	 "Alesis",   "HR-16B",		GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( ????, sr16,  0,       0, 		sr16, 	alesis, 	 0,  	 "Alesis",   "SR-16",		GAME_NOT_WORKING | GAME_NO_SOUND)
