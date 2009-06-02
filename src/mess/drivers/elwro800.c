/***************************************************************************

        Elwro 800 Junior

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(elwro800_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START( elwro800_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( elwro800 )
INPUT_PORTS_END


static MACHINE_RESET(elwro800)
{
}

static VIDEO_START( elwro800 )
{
}

static VIDEO_UPDATE( elwro800 )
{
    return 0;
}

static MACHINE_DRIVER_START( elwro800 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(elwro800_mem)
    MDRV_CPU_IO_MAP(elwro800_io)

    MDRV_MACHINE_RESET(elwro800)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(elwro800)
    MDRV_VIDEO_UPDATE(elwro800)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(elwro800)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( elwro800 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
  ROM_LOAD( "bas01.bin", 0x0000, 0x2000, CRC(6ab16f36) SHA1(49a19b279f311279c7fed3d2b3f207732d674c26))
  ROM_LOAD( "bas02.bin", 0x0000, 0x2000, CRC(15c54f1b) SHA1(b9b8dd6e0e1efff9288d0bd2079c2043f86efa2b))
  ROM_LOAD( "bas03.bin", 0x0000, 0x2000, CRC(6ab16f36) SHA1(49a19b279f311279c7fed3d2b3f207732d674c26))
  ROM_LOAD( "bas11.bin", 0x0000, 0x2000, CRC(a743eb80) SHA1(3a300550838535b4adfe6d05c05fe0b39c47df16))
  ROM_LOAD( "bas12.bin", 0x0000, 0x2000, CRC(a247c19b) SHA1(98c0a73b32548cb33c87ceb5f89f62ff0a1cf8a0))
  ROM_LOAD( "bas13.bin", 0x0000, 0x2000, CRC(a743eb80) SHA1(3a300550838535b4adfe6d05c05fe0b39c47df16))
  ROM_LOAD( "boot01.bin", 0x0000, 0x2000, CRC(0efd0e7c) SHA1(b86a96519510750e7805029af463c0b9526e0c3a))
  ROM_LOAD( "boot02.bin", 0x0000, 0x2000, CRC(24d62a81) SHA1(f08c525bb602f6f53790d55af1f78ec222a90924))
  ROM_LOAD( "boot03.bin", 0x0000, 0x2000, CRC(0efd0e7c) SHA1(b86a96519510750e7805029af463c0b9526e0c3a))
  ROM_LOAD( "junior_io_prom.bin", 0x0000, 0x0200,  CRC(c6a777c4) SHA1(41debc1b4c3bd4eef7e0e572327c759e0399a49c))
  ROM_LOAD( "junior_mem_prom.bin", 0x0000, 0x0200, CRC(0f745f42) SHA1(360ec23887fb6d7e19ee85d2bb30d9fa57f4936e))

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, elwro800,  0,       0, 	elwro800, 	elwro800, 	 0,  	  elwro800,  	 "Elwro",   "800 Junior",		GAME_NOT_WORKING)

