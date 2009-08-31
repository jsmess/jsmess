/***************************************************************************
   
    Jugend+Technik CompJU+TEr

    15/07/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z8/z8.h"

static ADDRESS_MAP_START( jtc_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START( jtc_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( jtc )
INPUT_PORTS_END


static MACHINE_RESET(jtc) 
{	
}

static VIDEO_START( jtc )
{
}

static VIDEO_UPDATE( jtc )
{
    return 0;
}


static MACHINE_DRIVER_START( jtc )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", UB8830D, XTAL_8MHz)
    MDRV_CPU_PROGRAM_MAP(jtc_mem)
    MDRV_CPU_IO_MAP(jtc_io)	

    MDRV_MACHINE_RESET(jtc)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(jtc)
    MDRV_VIDEO_UPDATE(jtc)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(jtc)
SYSTEM_CONFIG_END

/* ROM definition */

ROM_START( jtc )
	ROM_REGION( 0x10000, "maincpu", 0 )
  	ROM_LOAD( "u883rom.bin", 0x0000, 0x0800, CRC(2453c8c1) SHA1(816f5d08f8064b69b1779eb6661fde091aa58ba8) )
ROM_END

ROM_START( jtces88 )
	ROM_REGION( 0x10000, "maincpu", 0 )
  	ROM_LOAD( "u883rom.bin", 0x0000, 0x0800, CRC(2453c8c1) SHA1(816f5d08f8064b69b1779eb6661fde091aa58ba8) )
  	ROM_LOAD( "es1988_0800.bin", 0x0800, 0x0800, CRC(af3e882f) SHA1(65af0d0f5f882230221e9552707d93ed32ba794d) )
  	ROM_LOAD( "es1988_2000.bin", 0x2000, 0x0800, CRC(5ff87c1e) SHA1(fbd2793127048bd9706970b7bce84af2cb258dc5) )
ROM_END

ROM_START( jtces23 )
	ROM_REGION( 0x10000, "maincpu", 0 )
  	ROM_LOAD( "u883rom.bin", 0x0000, 0x0800, CRC(2453c8c1) SHA1(816f5d08f8064b69b1779eb6661fde091aa58ba8) )
  	ROM_LOAD( "es23_0800.bin", 0x0800, 0x1000, CRC(16128b64) SHA1(90fb0deeb5660f4a2bb38d51981cc6223d5ddf6b) )
ROM_END

ROM_START( jtces40 )
	ROM_REGION( 0x10000, "maincpu", 0 )
  	ROM_LOAD( "u883rom.bin", 0x0000, 0x0800, CRC(2453c8c1) SHA1(816f5d08f8064b69b1779eb6661fde091aa58ba8) )
  	ROM_LOAD( "es40_0800.bin", 0x0800, 0x1800, CRC(770c87ce) SHA1(1a5227ba15917f2a572cb6c27642c456f5b32b90) )
ROM_END

/* Driver */

/*    YEAR  NAME		PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( 1987, jtc,		0,       0, 	jtc, 	jtc, 	 0,  	  jtc,  	 "Jugend+Technik",   "CompJU+TEr",		GAME_NOT_WORKING)
COMP( 1988, jtces88,	jtc,     0, 	jtc, 	jtc, 	 0,  	  jtc,  	 "Jugend+Technik",   "CompJU+TEr (EMR-ES 1988)",	GAME_NOT_WORKING)
COMP( 1989, jtces23,	jtc,     0, 	jtc, 	jtc, 	 0,  	  jtc,  	 "Jugend+Technik",   "CompJU+TEr (ES 2.3)",	GAME_NOT_WORKING)
COMP( 1990, jtces40,	jtc,     0, 	jtc, 	jtc, 	 0,  	  jtc,  	 "Jugend+Technik",   "CompJU+TEr (ES 4.0)",	GAME_NOT_WORKING)
