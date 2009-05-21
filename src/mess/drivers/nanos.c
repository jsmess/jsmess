/***************************************************************************
   
        Nanos

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(nanos_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START( nanos_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( nanos )
INPUT_PORTS_END


static MACHINE_RESET(nanos) 
{	
}

static VIDEO_START( nanos )
{
}

static VIDEO_UPDATE( nanos )
{
    return 0;
}

static MACHINE_DRIVER_START( nanos )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(nanos_mem)
    MDRV_CPU_IO_MAP(nanos_io)	

    MDRV_MACHINE_RESET(nanos)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(nanos)
    MDRV_VIDEO_UPDATE(nanos)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(nanos)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( nanos )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
  ROM_LOAD( "k7634_1.rom", 0x0000, 0x0800, CRC(8e34e6ac) SHA1(fd342f6effe991823c2a310737fbfcba213c4fe3))
  ROM_LOAD( "k7634_2.rom", 0x0000, 0x0180, CRC(4e01b02b) SHA1(8a279da886555c7470a1afcbb3a99693ea13c237))
  ROM_LOAD( "zg_nanos.rom", 0x0000, 0x0800, CRC(5682d3f9) SHA1(5b738972c815757821c050ee38b002654f8da163))

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, nanos,  0,       0, 	nanos, 	nanos, 	 0,  	  nanos,  	 "Ingenieurhochschule fur Seefahrt Warnemunde/Wustrow",   "Nanos",		GAME_NOT_WORKING)

