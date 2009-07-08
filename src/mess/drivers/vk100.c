/***************************************************************************

        DEC VK 100

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/i8085/i8085.h"

static ADDRESS_MAP_START(vk100_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START( vk100_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( vk100 )
INPUT_PORTS_END


static MACHINE_RESET(vk100)
{
}

static VIDEO_START( vk100 )
{
}

static VIDEO_UPDATE( vk100 )
{
    return 0;
}

static MACHINE_DRIVER_START( vk100 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",8085A, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(vk100_mem)
    MDRV_CPU_IO_MAP(vk100_io)

    MDRV_MACHINE_RESET(vk100)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(vk100)
    MDRV_VIDEO_UPDATE(vk100)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(vk100)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( vk100 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
  ROM_LOAD( "23-031e4-00.rom1", 0x0000, 0x2000, CRC(c8596398) SHA1(a8dc833dcdfb7550c030ac3d4143e266b1eab03a))
  ROM_LOAD( "23-017e4-00.rom2", 0x0000, 0x2000, CRC(e857a01e) SHA1(914b2c51c43d0d181ffb74e3ea59d74e70ab0813))
  ROM_LOAD( "23-018e4-00.rom3", 0x0000, 0x2000, CRC(b3e7903b) SHA1(8ad6ed25cd9b04a9968aa09ab69ba526d35ca550))
  ROM_LOAD( "23-190e2-00.rom4", 0x0000, 0x1000, CRC(ad596fa5) SHA1(b30a24155640d32c1b47a3a16ea33cd8df2624f6))
  ROM_LOAD( "6301.pr3", 0x0000, 0x0100, CRC(75885a9f) SHA1(c721dad6a69c291dd86dad102ed3a8ddd620ecc4))
  ROM_LOAD( "6309.pr1", 0x0000, 0x0100, CRC(71b01864) SHA1(e552f5b0bc3f443299282b1da7e9dbfec60e12bf))
  ROM_LOAD( "6309.pr2", 0x0000, 0x0100, CRC(198317fc) SHA1(00e97104952b3fbe03a4f18d800d608b837d10ae))
  ROM_LOAD( "7643.pr4", 0x0000, 0x0400, CRC(e8ecf59f) SHA1(49e9d109dad3d203d45471a3f4ca4985d556161f))

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( 198?, vk100,  0,       0, 	vk100, 	vk100, 	 0,  	  vk100,  	 "DEC",   "VK 100",		GAME_NOT_WORKING)

