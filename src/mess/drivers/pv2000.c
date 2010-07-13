/***************************************************************************
   
        Driver for Casio PV-2000

        13/07/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(pv2000_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( pv2000_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( pv2000 )
INPUT_PORTS_END


static MACHINE_RESET(pv2000) 
{	
}

static VIDEO_START( pv2000 )
{
}

static VIDEO_UPDATE( pv2000 )
{
    return 0;
}

static MACHINE_DRIVER_START( pv2000 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(pv2000_mem)
    MDRV_CPU_IO_MAP(pv2000_io)	

    MDRV_MACHINE_RESET(pv2000)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(pv2000)
    MDRV_VIDEO_UPDATE(pv2000)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( pv2000 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "ipl.rom", 0x0000, 0x4000, CRC(78ecfb99) SHA1(cdf54cb713f65fd1197002cc5082eaafe13625aa))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( ????, pv2000,  0,       0, 	pv2000, 	pv2000, 	 0,   "Casio",   "PV-2000",		GAME_NOT_WORKING | GAME_NO_SOUND)

