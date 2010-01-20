/***************************************************************************
   
        Digital Microsystems DMS-86

        11/01/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/i86/i86.h"

static ADDRESS_MAP_START(dms86_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000, 0x1ffff) AM_RAM
	AM_RANGE(0xfe000, 0xfffff) AM_ROM AM_REGION("user1",0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( dms86_io , ADDRESS_SPACE_IO, 16)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( dms86 )
INPUT_PORTS_END


static MACHINE_RESET(dms86) 
{	
}

static VIDEO_START( dms86 )
{
}

static VIDEO_UPDATE( dms86 )
{
    return 0;
}

static MACHINE_DRIVER_START( dms86 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",I8086, XTAL_9_8304MHz)
    MDRV_CPU_PROGRAM_MAP(dms86_mem)
    MDRV_CPU_IO_MAP(dms86_io)	

    MDRV_MACHINE_RESET(dms86)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(dms86)
    MDRV_VIDEO_UPDATE(dms86)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( dms86 )
	ROM_REGION( 0x2000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "hns-86_54-8678.bin", 0x0000, 0x1000, CRC(95f58e1c) SHA1(6fc8f087f0c887d8b429612cd035c6c1faab570c))
	ROM_LOAD16_BYTE( "hns-86_54-8677.bin", 0x0001, 0x1000, CRC(78fad756) SHA1(ddcbff1569ec6975b8489935cdcfa80eba413502))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1982, dms86,  0,       0, 	dms86, 	dms86, 	 0,   	 "Digital Microsystems",   "DMS-86",		GAME_NOT_WORKING)

