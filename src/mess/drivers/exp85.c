/***************************************************************************
   
        Explorer 85

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/i8085/i8085.h"

static ADDRESS_MAP_START(exp85_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0xbfff ) AM_RAM
	AM_RANGE( 0xc000, 0xf7ff ) AM_ROM
	AM_RANGE( 0xf800, 0xffff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( exp85_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( exp85 )
INPUT_PORTS_END


static MACHINE_RESET(exp85) 
{	
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), I8085_PC, 0xf000);
}

static VIDEO_START( exp85 )
{
}

static VIDEO_UPDATE( exp85 )
{
    return 0;
}

static MACHINE_DRIVER_START( exp85 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",8085A, 3072000 )
    MDRV_CPU_PROGRAM_MAP(exp85_mem)
    MDRV_CPU_IO_MAP(exp85_io)	

    MDRV_MACHINE_RESET(exp85)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(exp85)
    MDRV_VIDEO_UPDATE(exp85)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(exp85)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( exp85 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "c000.bin", 0xc000, 0x0800, CRC(73ce4aad) SHA1(2c69cd0b6c4bdc92f4640bce18467e4e99255bab))
	ROM_LOAD( "c800.bin", 0xc800, 0x0800, CRC(eb3fdedc) SHA1(af92d07f7cb7533841b16e1176401363176857e1))
	ROM_LOAD( "d000.bin", 0xd000, 0x0800, CRC(c10c4a22) SHA1(30588ba0b27a775d85f8c581ad54400c8521225d))
	ROM_LOAD( "d800.bin", 0xd800, 0x0800, CRC(dfa43ef4) SHA1(56a7e7a64928bdd1d5f0519023d1594cacef49b3))
	ROM_LOAD( "explorer.bin", 0xf000, 0x0800, CRC(1a99d0d9) SHA1(57b6d48e71257bc4ef2d3dddc9b30edf6c1db766))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( 1979, exp85,  0,       0, 	exp85, 	exp85, 	 0,  	  exp85,  	 "Netronics",   "Explorer 85",		GAME_NOT_WORKING)

