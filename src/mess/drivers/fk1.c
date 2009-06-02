/***************************************************************************

        FK-1

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(fk1_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x07ff ) AM_ROM
	AM_RANGE( 0x0800, 0xffff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( fk1_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( fk1 )
INPUT_PORTS_END


static MACHINE_RESET(fk1)
{
}

static VIDEO_START( fk1 )
{
}

static VIDEO_UPDATE( fk1 )
{
    return 0;
}

static MACHINE_DRIVER_START( fk1 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(fk1_mem)
    MDRV_CPU_IO_MAP(fk1_io)

    MDRV_MACHINE_RESET(fk1)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(fk1)
    MDRV_VIDEO_UPDATE(fk1)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(fk1)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( fk1 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
  ROM_LOAD( "fk1.rom", 0x0000, 0x0800, CRC(145561f8) SHA1(a4eb17d773e51b34620c508b6cebcb4531ae99c2))

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, fk1,  0,       0, 	fk1, 	fk1, 	 0,  	  fk1,  	 "Statni statek Klicany",   "FK-1",		GAME_NOT_WORKING)

