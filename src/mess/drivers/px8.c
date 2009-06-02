/***************************************************************************

    Epson PX-8

    12/05/2009 Skeleton driver.

    Seems to be a CP/M computer.

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"


/***************************************************************************
    ADDRESS MAPS
***************************************************************************/

static ADDRESS_MAP_START( px8_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( px8_io , ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END


/***************************************************************************
    VIDEO EMULATION
***************************************************************************/

static VIDEO_START( px8 )
{
}

static VIDEO_UPDATE( px8 )
{
    return 0;
}


/***************************************************************************
    INPUT PORTS
***************************************************************************/

static INPUT_PORTS_START( px8 )
INPUT_PORTS_END


/***************************************************************************
    MACHINE DRIVERS
***************************************************************************/

static MACHINE_DRIVER_START( px8 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(px8_mem)
    MDRV_CPU_IO_MAP(px8_io)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", LCD)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(480, 64)
	MDRV_SCREEN_VISIBLE_AREA(0, 479, 0, 63)

	MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(px8)
    MDRV_VIDEO_UPDATE(px8)
MACHINE_DRIVER_END


/***************************************************************************
    ROM DEFINITIONS
***************************************************************************/

ROM_START( px8 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	/* These two roms appear to be slightly different bios versions */
	ROM_LOAD( "sys.rom", 0x0000, 0x8000, CRC(bd3e4938) SHA1(5bd48abd2a563a1ae31ff137280f40c8f756e969))
	ROM_LOAD( "px060688.epr", 0x0000, 0x8000, CRC(44308bdf) SHA1(5c4545fcf1af9931b4699436294d9b6298052a7b))

	ROM_REGION( 0x0800, "gfx1", 0 )
	ROM_LOAD( "font.rom", 0x0000, 0x0800, CRC(5b52edbd) SHA1(38197edf301bb2843bea040536af545f76b3d44f))

	ROM_REGION(0x1000, "sub", 0)
	ROM_LOAD("upd7508.bin", 0x0000, 0x1000, NO_DUMP)

	ROM_REGION(0x1000, "slave", 0)
	ROM_LOAD("hd6301.bin", 0x0000, 0x1000, NO_DUMP)
ROM_END


/***************************************************************************
    SYSTEM CONFIG
***************************************************************************/

static SYSTEM_CONFIG_START( px8 )
	CONFIG_RAM_DEFAULT(64 * 1024) /* 64KB RAM */
SYSTEM_CONFIG_END


/***************************************************************************
    GAME DRIVERS
***************************************************************************/

/*    YEAR  NAME  PARENT  COMPAT  MACHINE  INPUT  INIT  CONFIG  COMPANY  FULLNAME  FLAGS */
COMP( 1984, px8,  0,      0,      px8,     px8,   0,    px8,    "Epson", "PX-8",   GAME_NOT_WORKING )
