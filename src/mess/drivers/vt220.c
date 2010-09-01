/***************************************************************************

        DEC VT220

        30/06/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/mcs51/mcs51.h"
#include "devices/messram.h"

static ADDRESS_MAP_START(vt220_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x7fff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( vt220_io , ADDRESS_SPACE_IO, 8)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( vt220 )
INPUT_PORTS_END

static MACHINE_RESET(vt220)
{
	memset(messram_get_ptr(machine->device("messram")),0,16*1024);
}

static VIDEO_START( vt220 )
{
}

static VIDEO_UPDATE( vt220 )
{
    return 0;
}


static MACHINE_CONFIG_START( vt220, driver_data_t )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", I8051, XTAL_16MHz)
    MDRV_CPU_PROGRAM_MAP(vt220_mem)
    MDRV_CPU_IO_MAP(vt220_io)

    MDRV_MACHINE_RESET(vt220)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(vt220)
    MDRV_VIDEO_UPDATE(vt220)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("16K")
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( vt220 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "23-178e6.bin", 0x0000, 0x8000, CRC(cce5088c) SHA1(4638304729d1213658a96bb22c5211322b74d8fc))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1983, vt220,  0,       0, 	vt220,	vt220,	 0, 		 "Digital Equipment Corporation",   "VT220",		GAME_NOT_WORKING | GAME_NO_SOUND)

