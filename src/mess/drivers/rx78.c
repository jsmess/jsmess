/***************************************************************************
   
        Bandai Gundam RX-78

        13/07/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(rx78_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START( rx78_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( rx78 )
INPUT_PORTS_END


static MACHINE_RESET(rx78) 
{	
}

static VIDEO_START( rx78 )
{
}

static VIDEO_UPDATE( rx78 )
{
    return 0;
}

static MACHINE_DRIVER_START( rx78 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(rx78_mem)
    MDRV_CPU_IO_MAP(rx78_io)	

    MDRV_MACHINE_RESET(rx78)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(rx78)
    MDRV_VIDEO_UPDATE(rx78)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( rx78 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "ipl.rom", 0x0000, 0x2000, CRC(a194ea53) SHA1(ba39e73e6eb7cbb8906fff1f81a98964cd62af0d))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1983, rx78,  	0,       0, 		rx78, 	rx78, 	 0,  	  "Bandai",   "Gundam RX-78",		GAME_NOT_WORKING | GAME_NO_SOUND)

