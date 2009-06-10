/***************************************************************************
   
        Intel SDK-86

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/i86/i86.h"

static ADDRESS_MAP_START(sdk86_mem, ADDRESS_SPACE_PROGRAM, 16)
	AM_RANGE(0x00000, 0x03fff) AM_RAM
	AM_RANGE(0xfe000, 0xfffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sdk86_io , ADDRESS_SPACE_IO, 16)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( sdk86 )
INPUT_PORTS_END


static MACHINE_RESET(sdk86) 
{	
}

static VIDEO_START( sdk86 )
{
}

static VIDEO_UPDATE( sdk86 )
{
    return 0;
}

static MACHINE_DRIVER_START( sdk86 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",I8086, XTAL_5MHz) /* jumper selection slow it down on 2.5MHz*/
    MDRV_CPU_PROGRAM_MAP(sdk86_mem)
    MDRV_CPU_IO_MAP(sdk86_io)	

    MDRV_MACHINE_RESET(sdk86)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(sdk86)
    MDRV_VIDEO_UPDATE(sdk86)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(sdk86)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( sdk86 )
	ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASEFF )
	// Monitor Version 1.2
	ROM_LOAD16_BYTE( "0169_102042-001.a27", 0xfe000, 0x0800, CRC(3f46311a) SHA1(a97e6861b736f26230b9adbf5cd2576a9f60d626))
  	ROM_LOAD16_BYTE( "0170_102043-001.a30", 0xfe001, 0x0800, CRC(65924471) SHA1(5d258695bf585f89179dfa0a113a0eeeabd5ee2b))  
  	ROM_LOAD16_BYTE( "0456_104531-001.a36", 0xff000, 0x0800, CRC(f9c4a809) SHA1(aea324c3f52dd393f1eed2b856ba11f050a35b93))
  	ROM_LOAD16_BYTE( "0457_104532-001.a37", 0xff001, 0x0800, CRC(a245ba5c) SHA1(7f67277f866fca5377cb123e9cc405b5fdfe61d3))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( 1979, sdk86,  0,       0, 	sdk86, 	sdk86, 	 0,  	  sdk86,  	 "Intel",   "SDK-86",		GAME_NOT_WORKING)

