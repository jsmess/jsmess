/***************************************************************************
   
        Hitachi Basic Master Level 3 (MB-6890)

        13/07/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/m6809/m6809.h"

static ADDRESS_MAP_START(bml3_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START( bml3_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( bml3 )
INPUT_PORTS_END


static MACHINE_RESET(bml3) 
{	
}

static VIDEO_START( bml3 )
{
}

static VIDEO_UPDATE( bml3 )
{
    return 0;
}

static MACHINE_DRIVER_START( bml3 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",M6809, XTAL_1MHz)
    MDRV_CPU_PROGRAM_MAP(bml3_mem)
    MDRV_CPU_IO_MAP(bml3_io)	

    MDRV_MACHINE_RESET(bml3)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(bml3)
    MDRV_VIDEO_UPDATE(bml3)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( bml3 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "l3bas.rom", 0x0000, 0x6000, CRC(d81baa07) SHA1(a8fd6b29d8c505b756dbf5354341c48f9ac1d24d))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1980, bml3,  	0,       0, 		bml3, 	bml3, 	 0,  	   "Hitachi",   "Basic Master Level 3",		GAME_NOT_WORKING | GAME_NO_SOUND)

