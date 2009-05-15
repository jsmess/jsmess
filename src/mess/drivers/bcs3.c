/***************************************************************************

        BCS 3

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(bcs3_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START( bcs3_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( bcs3 )
INPUT_PORTS_END


static MACHINE_RESET(bcs3)
{
}

static VIDEO_START( bcs3 )
{
}

static VIDEO_UPDATE( bcs3 )
{
    return 0;
}

static MACHINE_DRIVER_START( bcs3 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(bcs3_mem)
    MDRV_CPU_IO_MAP(bcs3_io)

    MDRV_MACHINE_RESET(bcs3)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(bcs3)
    MDRV_VIDEO_UPDATE(bcs3)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(bcs3)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( bcs3 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
  ROM_LOAD( "se24.bin", 0x0000, 0x0800, CRC(268de5ee) SHA1(78784945956c1b0282a4e82ad55e7c3a77389e50))
  ROM_LOAD( "se24font.bin", 0x0000, 0x0400, CRC(eaed9d84) SHA1(7023a6187cd6bd0c6489d76ff662453f14e5b636))
  ROM_LOAD( "se31_29.bin", 0x0000, 0x1000, CRC(e9b55544) SHA1(82bae68c4bcaecf66632f5b43913b50a1acba316))
  ROM_LOAD( "se31_40.bin", 0x0000, 0x1000, CRC(4e993152) SHA1(6bb01ff5779627fa2eb2df432fffcfccc1e33231))
  ROM_LOAD( "se31font.bin", 0x0000, 0x0400, CRC(a20c93c9) SHA1(b2be1c0d98b7ac05713349b099b392975968be1d))
  ROM_LOAD( "se31mceditor.bin", 0x0000, 0x0400, CRC(8eac92ec) SHA1(8950a3ef05d02abf34269bfce002c46d273ce113))
  ROM_LOAD( "sp33_29.bin", 0x0000, 0x1000, CRC(1c851eb2) SHA1(4f8bb5274ea1861a35a840e8f3482bdc693047c4))
  ROM_LOAD( "sp33font.bin", 0x0000, 0x0400, CRC(b27f1c07) SHA1(61c80c585f198370ba5e856839c12b15acdc58ee))

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, bcs3,  0,       0, 	bcs3, 	bcs3, 	 0,  	  bcs3,  	 "Eckhard Schiller",   "BCS 3",		GAME_NOT_WORKING)

