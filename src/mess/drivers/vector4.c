/***************************************************************************
   
        Vector 4

        08/12/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(vector4_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xdfff) AM_RAM
	AM_RANGE(0xe000, 0xefff) AM_ROM
	AM_RANGE(0xf000, 0xf7ff) AM_RAM
	AM_RANGE(0xf800, 0xf8ff) AM_ROM
	AM_RANGE(0xf900, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( vector4_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( vector4 )
INPUT_PORTS_END


static MACHINE_RESET(vector4) 
{	
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_PC, 0xe000);
}

static VIDEO_START( vector4 )
{
}

static VIDEO_UPDATE( vector4 )
{
    return 0;
}

static MACHINE_DRIVER_START( vector4 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(vector4_mem)
    MDRV_CPU_IO_MAP(vector4_io)	

    MDRV_MACHINE_RESET(vector4)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(vector4)
    MDRV_VIDEO_UPDATE(vector4)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(vector4)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( vector4 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
    ROM_LOAD( "vg-em-43.bin", 0xe000, 0x1000, CRC(29a0fcee) SHA1(ca44de527f525b72f78b1c084c39aa6ce21731b5))
//  ROM_LOAD( "vg40cl_ihl.bin", 0xe000, 0x0400, CRC(dcaf79e6) SHA1(63619ddb12ff51e5862902fb1b33a6630f555ad7))
//  ROM_LOAD( "vg40ch_ihl.bin", 0xe400, 0x0400, CRC(3ff97d70) SHA1(b401e49aa97ac106c2fd5ee72d89e683ebe34e34))
//  ROM_LOAD( "vg-zcb50.bin", 0xe000, 0x1000, CRC(22d692ce) SHA1(cbb21b0acc98983bf5febd59ff67615d71596e36))
    ROM_LOAD( "mfdc.bin", 0xf800, 0x0100, CRC(d82a40d6) SHA1(cd1ef5fb0312cd1640e0853d2442d7d858bc3e3b))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, vector4,  0,       0, 	vector4, 	vector4, 	 0,  	  vector4,  	 "Vector Graphics",   "Vector 4",		GAME_NOT_WORKING)

