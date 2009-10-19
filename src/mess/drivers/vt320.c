/***************************************************************************

        DEC VT320

        30/06/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/mcs51/mcs51.h"
#include "devices/messram.h"
/*

Partlist :

Siemens SAB8031A-16-P
ROMless version of the 8051 microcontroller, running at 16 MHz.
Motorola MC2681P
Dual Universal Asynchronous Receiver/Transmitter (DUART), 40-pin package.
Toshiba TC53512AP
ROM, 512K bits = 64K bytes. 28-pin package.
Toshiba TC5565APL-12, 2 off
Static RAM, 64K bit = 8K byte.
ST TDA1170N
Vertical deflection system IC.
UC 80343Q
20 pins. Unknown.
UC 80068Q
20 pins. Unknown.
Motorola SN74LS157NQST
16 pins. Quad 2-to-1 multiplexer.
Microchip ER5911
8 pins. Serial EEPROM. 1K bits = 128 bytes.
Texas Inst. 749X 75146
8 pins. Unknown.
Signetics? 74LS373N
8-bit D-type latch. This has eight inputs and eight outputs.
*/
static ADDRESS_MAP_START(vt320_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( vt320_io , ADDRESS_SPACE_IO, 8)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( vt320 )
INPUT_PORTS_END

static MACHINE_RESET(vt320)
{
	memset(messram_get_ptr(devtag_get_device(machine, "messram")),0,16*1024);
}

static VIDEO_START( vt320 )
{
}

static VIDEO_UPDATE( vt320 )
{
    return 0;
}


static MACHINE_DRIVER_START( vt320 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", I8051, XTAL_16MHz)
    MDRV_CPU_PROGRAM_MAP(vt320_mem)
    MDRV_CPU_IO_MAP(vt320_io)

    MDRV_MACHINE_RESET(vt320)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(vt320)
    MDRV_VIDEO_UPDATE(vt320)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("16K")
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( vt320 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "23-054e7.bin", 0x0000, 0x10000, CRC(be98f9a4) SHA1(b8044d42ffaadb734fbd047fbca9c8aadeb0bf6c))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( 1987, vt320,  0,       0, 	vt320, 	vt320, 	 0,  	  0,  	 "DEC",   "VT320",		GAME_NOT_WORKING)
//COMP( 1989?, vt330,  0,       0,  vt320,  vt320,   0,       0,     "DEC",   "VT330",      GAME_NOT_WORKING)
//COMP( 1989?, vt340,  0,       0,  vt320,  vt320,   0,       0,     "DEC",   "VT340",      GAME_NOT_WORKING)
//COMP( 1990?, vt340p,  0,       0,     vt320,  vt320,   0,       0,     "DEC",   "VT340+",     GAME_NOT_WORKING)

