/***************************************************************************

		Systec Z80
        
		More data :
			http://www.hd64180-cpm.de/html/systecz80.html

        30/08/2010 Skeleton driver

		Systec Platine 1

		SYSTEC 155.1L
		EPROM 2764 CP/M LOADER 155 / 9600 Baud
		Z8400APS CPU
		Z8420APS PIO
		Z8430APS CTC
		Z8470APS DART

		Systec Platine 2

		SYSTEC 100.3B
		MB8877A FDC Controller
		FDC9229BT SMC 8608
		Z8410APS DMA
		Z8420APS PIO

		MB8877A Compatible FD1793		
****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(systec_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( systec_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( systec )
INPUT_PORTS_END

static MACHINE_RESET(systec)
{
}

static VIDEO_START( systec )
{
}

static VIDEO_UPDATE( systec )
{
	return 0;
}

static MACHINE_DRIVER_START( systec )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", Z80, XTAL_16MHz / 4)
    MDRV_CPU_PROGRAM_MAP(systec_mem)
	MDRV_CPU_IO_MAP(systec_io)

    MDRV_MACHINE_RESET(systec)

    /* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 192) /* border size not accurate */
	MDRV_SCREEN_VISIBLE_AREA(0, 256 - 1, 0, 192 - 1)

    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(systec)
    MDRV_VIDEO_UPDATE(systec)
MACHINE_DRIVER_END


/* ROM definition */
ROM_START( systec )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "systec.bin",   0x0000, 0x2000, CRC(967108ab) SHA1(a414db032ca7db0f9fdbe22aa68a099a93efb593))
ROM_END

/* Driver */

/*   YEAR  NAME    PARENT  COMPAT   MACHINE  INPUT  INIT  		COMPANY   FULLNAME       FLAGS */
COMP( 19??, systec,  0,       0, 	systec, 	systec, 	 0,  	  "Systec",   "Systec Z80",		GAME_NOT_WORKING | GAME_NO_SOUND)
