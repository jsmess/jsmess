/***************************************************************************

        FM7

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"

static ADDRESS_MAP_START(fm7_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START( fm7_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( fm7 )
INPUT_PORTS_END


static MACHINE_RESET(fm7)
{
}

static VIDEO_START( fm7 )
{
}

static VIDEO_UPDATE( fm7 )
{
    return 0;
}

static MACHINE_DRIVER_START( fm7 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",M6809E, XTAL_2MHz)
    MDRV_CPU_PROGRAM_MAP(fm7_mem, 0)
    MDRV_CPU_IO_MAP(fm7_io, 0)

    MDRV_MACHINE_RESET(fm7)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(fm7)
    MDRV_VIDEO_UPDATE(fm7)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(fm7)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( fm7 )
  ROM_REGION( 0x20000, "maincpu", ROMREGION_ERASEFF )
  ROM_LOAD( "boot_bas.rom", 0x0000,  0x0200, CRC(c70f0c74) SHA1(53b63a301cba7e3030e79c59a4d4291eab6e64b0))
  ROM_LOAD( "boot_dos.rom", 0x0000,  0x0200, CRC(198614ff) SHA1(037e5881bd3fed472a210ee894a6446965a8d2ef))
  ROM_LOAD( "fb30rom1.dat", 0x0000,  0x7c00, CRC(a96d19b6) SHA1(8d5f0cfe7e0d39bf2ab7e4c798a13004769c28b2))
  ROM_LOAD( "fb30rom2.dat", 0x0000,  0x0200, CRC(c70f0c74) SHA1(53b63a301cba7e3030e79c59a4d4291eab6e64b0))
  ROM_LOAD( "fb30sub.dat",  0x0000,  0x2800, CRC(24cec93f) SHA1(50b7283db6fe1342c6063fc94046283f4feddc1c))
  ROM_LOAD( "fbasic30.rom", 0x0000,  0x7c00, CRC(a96d19b6) SHA1(8d5f0cfe7e0d39bf2ab7e4c798a13004769c28b2))
  ROM_LOAD( "initiate.rom", 0x0000,  0x2000, CRC(785cb06c) SHA1(b65987e98a9564a82c85eadb86f0204eee5a5c93))
  ROM_LOAD( "kanji.rom",    0x0000, 0x20000, CRC(62402ac9) SHA1(bf52d22b119d54410dad4949b0687bb0edf3e143))
  ROM_LOAD( "subsys_a.rom", 0x0000,  0x2000, CRC(e8014fbb) SHA1(038cb0b42aee9e933b20fccd6f19942e2f476c83))
  ROM_LOAD( "subsys_b.rom", 0x0000,  0x2000, CRC(9be69fac) SHA1(0305bdd44e7d9b7b6a17675aff0a3330a08d21a8))
  ROM_LOAD( "subsys_c.rom", 0x0000,  0x2800, CRC(24cec93f) SHA1(50b7283db6fe1342c6063fc94046283f4feddc1c))
  ROM_LOAD( "subsyscg.rom", 0x0000,  0x2000, CRC(e9f16c42) SHA1(8ab466b1546d023ba54987790a79e9815d2b7bb2))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, fm7,  0,       0, 	fm7, 	fm7, 	 0,  	  fm7,  	 "Fujitsu",   "FM7",		GAME_NOT_WORKING)

