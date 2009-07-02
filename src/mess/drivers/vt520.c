/***************************************************************************
   
        DEC VT520

        02/07/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/mcs51/mcs51.h"

static ADDRESS_MAP_START(vt520_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( vt520_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( vt520 )
INPUT_PORTS_END


static MACHINE_RESET(vt520) 
{	
}

static VIDEO_START( vt520 )
{
}

static VIDEO_UPDATE( vt520 )
{
    return 0;
}

static MACHINE_DRIVER_START( vt520 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",I8032, XTAL_20MHz)
    MDRV_CPU_PROGRAM_MAP(vt520_mem)
    MDRV_CPU_IO_MAP(vt520_io)	

    MDRV_MACHINE_RESET(vt520)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(vt520)
    MDRV_VIDEO_UPDATE(vt520)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(vt520)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( vt520 )
    ROM_REGION( 0x80000, "maincpu", ROMREGION_ERASEFF )
  ROM_LOAD( "23-010ed-00__9739_d.e20.bin", 0x0000, 0x80000, CRC(2502cc22) SHA1(0437c3107412f69e09d050fef003f2a81d8a3163))

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, vt520,  0,       0, 	vt520, 	vt520, 	 0,  	  vt520,  	 "DEC",   "VT520",		GAME_NOT_WORKING)

