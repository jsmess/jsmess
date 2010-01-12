/***************************************************************************
   
        Digital Microsystems ZSBC-3

        11/01/2010 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(zsbc3_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x07ff ) AM_ROM
	AM_RANGE( 0x0800, 0xffff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( zsbc3_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( zsbc3 )
INPUT_PORTS_END


static MACHINE_RESET(zsbc3) 
{	
}

static VIDEO_START( zsbc3 )
{
}

static VIDEO_UPDATE( zsbc3 )
{
    return 0;
}

static MACHINE_DRIVER_START( zsbc3 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_16MHz /4)
    MDRV_CPU_PROGRAM_MAP(zsbc3_mem)
    MDRV_CPU_IO_MAP(zsbc3_io)	

    MDRV_MACHINE_RESET(zsbc3)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(zsbc3)
    MDRV_VIDEO_UPDATE(zsbc3)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( zsbc3 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
    ROM_LOAD( "54-3002_zsbc_monitor_1.09.bin", 0x0000, 0x0800, CRC(628588e9) SHA1(8f0d489147ec8382ca007236e0a95a83b6ebcd86))
    ROM_REGION( 0x10000, "hdc", ROMREGION_ERASEFF )
    ROM_LOAD( "54-8622_hdc13.bin", 0x0000, 0x0400, CRC(02c7cd6d) SHA1(494281ba081a0f7fbadfc30a7d2ea18c59e55101))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   			FULLNAME       FLAGS */
COMP( 1980, zsbc3,  0,       0, 		zsbc3, 	zsbc3, 	 0,  	  "Digital Microsystems",   "ZSBC-3",		GAME_NOT_WORKING)

