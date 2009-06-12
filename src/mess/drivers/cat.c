/***************************************************************************
   
        Canon Cat

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"

static ADDRESS_MAP_START(cat_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000000, 0x0003ffff) AM_ROM // 256 KB ROM
	AM_RANGE(0x00040000, 0x00ffffff) AM_RAM // 256 KB RAM but not sure about mapping
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( cat )
INPUT_PORTS_END


static MACHINE_RESET(cat) 
{	
}

static VIDEO_START( cat )
{
}

static VIDEO_UPDATE( cat )
{
    return 0;
}

static MACHINE_DRIVER_START( cat )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",M68000, XTAL_5MHz)
    MDRV_CPU_PROGRAM_MAP(cat_mem)

    MDRV_MACHINE_RESET(cat)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(cat)
    MDRV_VIDEO_UPDATE(cat)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(cat)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( cat )
    ROM_REGION( 0x40000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "r240l0.bin", 0x00001, 0x10000, CRC(1b89bdc4) SHA1(39c639587dc30f9d6636b46d0465f06272838432))
	ROM_LOAD16_BYTE( "r240h0.bin", 0x00000, 0x10000, CRC(94f89b8c) SHA1(6c336bc30636a02c625d31f3057ec86bf4d155fc))
	ROM_LOAD16_BYTE( "r240l1.bin", 0x20001, 0x10000, CRC(1a73be4f) SHA1(e2de2cb485f78963368fb8ceba8fb66ca56dba34))
	ROM_LOAD16_BYTE( "r240h1.bin", 0x20000, 0x10000, CRC(898dd9f6) SHA1(93e791dd4ed7e4afa47a04df6fdde359e41c2075))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( 1987, cat,  0,       0, 	cat, 	cat, 	 0,  	  cat,  	 "Canon",   "Cat",		GAME_NOT_WORKING)

