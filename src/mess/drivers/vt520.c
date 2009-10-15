/***************************************************************************

        DEC VT520

        02/07/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "devices/messram.h"
#include "cpu/mcs51/mcs51.h"

static ADDRESS_MAP_START(vt520_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0xffff) AM_RAMBANK(1)
ADDRESS_MAP_END

/*
    On the boardthere is TC160G41AF (1222) custom chip
    doing probably all video/uart logic
    there is 43.430MHz xtal near by
*/

static READ8_HANDLER(vt520_some_r)
{
	//bit 5 0
	//bit 6 1
	return 0x40;
}

static ADDRESS_MAP_START( vt520_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x7ffb, 0x7ffb) AM_READ(vt520_some_r)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( vt520 )
INPUT_PORTS_END


static MACHINE_RESET(vt520)
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT8 *rom = memory_region(machine, "maincpu");
	memory_install_write8_handler(space, 0x0000, 0xffff, 0, 0, SMH_UNMAP);
	memory_set_bankptr(machine, 1, rom + 0x70000);
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
    MDRV_SCREEN_SIZE(802, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 802-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(vt520)
    MDRV_VIDEO_UPDATE(vt520)
	
	// On the board there are two M5M44256BJ-7 chips
	// Which are DRAM 256K x 4bit
	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("256K")
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( vt520 )
    ROM_REGION( 0x80000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "23-010ed-00.e20", 0x0000, 0x80000, CRC(2502cc22) SHA1(0437c3107412f69e09d050fef003f2a81d8a3163)) // "(C)DEC94 23-010ED-00 // 9739 D" dumped from a VT520-A4 model
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
//COMP( 1993, vt510,  0,       0,   vt520,  vt520,   0,       0,     "DEC",   "VT510",      GAME_NOT_WORKING)
COMP( 1994, vt520,  0,       0, 	vt520, 	vt520, 	 0,  	  0,  	 "DEC",   "VT520",		GAME_NOT_WORKING)
//COMP( 1994, vt525,  0,       0,   vt520,  vt520,   0,       0,     "DEC",   "VT525",      GAME_NOT_WORKING)
