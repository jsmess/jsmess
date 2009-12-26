/***************************************************************************

    Epson PX-8

    12/05/2009 Skeleton driver.

    Seems to be a CP/M computer.

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "cpu/m6800/m6800.h"
#include "devices/messram.h"

/***************************************************************************
    CONSTANTS
***************************************************************************/

#define XTAL_CR1		XTAL_9_8304MHz
#define XTAL_CR2		XTAL_32_768kHz

/* interrupt sources */
#define INT0_7508		0x01
#define INT1_SERIAL		0x02
#define INT2_RS232		0x04
#define INT3_BARCODE	0x08
#define INT4_FRC		0x10
#define INT5_OPTION		0x20


/***************************************************************************
    ADDRESS MAPS
***************************************************************************/

static ADDRESS_MAP_START( px8_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( px8_io , ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START( px8_slave_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x8000, 0x97ff) AM_RAM /* vram */
	AM_RANGE(0x9800, 0xefff) AM_NOP
	AM_RANGE(0xf000, 0xffff) AM_ROM AM_REGION("slave", 0) /* internal mask rom */
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


/* F4 Character Displayer ****************/
static const gfx_layout px8_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static GFXDECODE_START( px8 )
	GFXDECODE_ENTRY( "gfx1", 0x0000, px8_charlayout, 0, 1 )
GFXDECODE_END


/***************************************************************************
    MACHINE DRIVERS
***************************************************************************/

static MACHINE_DRIVER_START( px8 )
    /* main cpu (uPD70008) */
	MDRV_CPU_ADD("maincpu", Z80, XTAL_CR1 / 4) /* 2.45 MHz */
	MDRV_CPU_PROGRAM_MAP(px8_mem)
	MDRV_CPU_IO_MAP(px8_io)

    /* slave cpu (HD6303) */
	MDRV_CPU_ADD("slave", M6803, XTAL_CR1 / 4) /* 614 kHz */
	MDRV_CPU_PROGRAM_MAP(px8_slave_mem)

    /* sub CPU (uPD7508) */
//	MDRV_CPU_ADD("sub", UPD7508, 200000) /* 200 kHz */

	/* video hardware */
	MDRV_SCREEN_ADD("screen", LCD)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(480, 64)
	MDRV_SCREEN_VISIBLE_AREA(0, 479, 0, 63)

	MDRV_GFXDECODE(px8)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(px8)
	MDRV_VIDEO_UPDATE(px8)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("64K")
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
	ROM_LOAD("hd6303.bin", 0x0000, 0x1000, NO_DUMP)
ROM_END

/***************************************************************************
    GAME DRIVERS
***************************************************************************/

/*    YEAR  NAME  PARENT  COMPAT  MACHINE  INPUT  INIT  COMPANY  FULLNAME  FLAGS */
COMP( 1984, px8,  0,      0,      px8,     px8,   0,    "Epson", "PX-8",   GAME_NOT_WORKING )
