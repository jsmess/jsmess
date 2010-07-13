/***************************************************************************
   
        Hitachi Basic Master Jr (MB-6885)

        13/07/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/m6800/m6800.h"

static ADDRESS_MAP_START(bmjr_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( bmjr )
INPUT_PORTS_END


static MACHINE_RESET(bmjr) 
{	
}

static VIDEO_START( bmjr )
{
}

static VIDEO_UPDATE( bmjr )
{
    return 0;
}

static MACHINE_DRIVER_START( bmjr )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",M6800, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(bmjr_mem)

    MDRV_MACHINE_RESET(bmjr)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(bmjr)
    MDRV_VIDEO_UPDATE(bmjr)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( bmjr )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "bas.rom", 0x0000, 0x3000, CRC(2318e04e) SHA1(cdb3535663090f5bcaba20b1dbf1f34724ef6a5f))
	ROM_LOAD( "mon.rom", 0x0000, 0x1000, CRC(776cfa3a) SHA1(be747bc40fdca66b040e0f792b05fcd43a1565ce))
	ROM_LOAD( "prt.rom", 0x0000, 0x0800, CRC(b9aea867) SHA1(b8dd5348790d76961b6bdef41cfea371fdbcd93d))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1982, bmjr,  	0,       0, 	bmjr, 		bmjr, 	 0,  	 "Hitachi",   "Basic Master Jr",	GAME_NOT_WORKING | GAME_NO_SOUND)

