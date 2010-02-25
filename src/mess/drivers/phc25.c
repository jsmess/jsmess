/***************************************************************************

        Sanyo PHC-25

        Skeleton driver.

****************************************************************************/

/*

	http://www.geocities.jp/sanyo_phc_25/

	Z80 @ 4 MHz
	MC6847 video
	3x 8KB bios ROM
	1x 4KB chargen ROM
	16KB RAM
	6KB video RAM

*/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "devices/messram.h"
#include "video/m6847.h"
#include "sound/ay8910.h"


static ADDRESS_MAP_START(phc25_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x5fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_RAM
	AM_RANGE(0xe800, 0xffff) AM_RAM // video RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START(phc25_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0x80)
//	AM_RANGE(0x40, 0x40) 6847
//	AM_RANGE(0x80, 0x88) keyboard
//	AM_RANGE(0xc0, 0xc1) PSG
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( phc25 )
INPUT_PORTS_END


static MACHINE_RESET(phc25)
{
}

static VIDEO_START( phc25 )
{
}

static VIDEO_UPDATE( phc25 )
{
    return 0;
}

static const mc6847_interface mc6847_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

static const ay8910_interface ay8910_intf =
{
	AY8910_LEGACY_OUTPUT,
	AY8910_DEFAULT_LOADS,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

static MACHINE_DRIVER_START( phc25 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(phc25_mem)
    MDRV_CPU_IO_MAP(phc25_io)

    MDRV_MACHINE_RESET(phc25)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(phc25)
    MDRV_VIDEO_UPDATE(phc25)

	MDRV_MC6847_ADD("mc6847", mc6847_intf)
    MDRV_MC6847_TYPE(M6847_VERSION_ORIGINAL_NTSC)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("ay8910", AY8910, 3579545/2)
	MDRV_SOUND_CONFIG(ay8910_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	
	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("16K")
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( phc25 )
    ROM_REGION( 0x8000, "maincpu", 0 )
	ROM_LOAD( "phc25rom.bin", 0x0000, 0x6000, CRC(c442ed57) SHA1(af0a6a6be8e48a56b6960f89c76bad6320ea0f55))

    ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD( "chargen.bin", 0x0000, 0x1000, NO_DUMP)
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1983, phc25,  0,       0,	phc25, 	phc25, 	 0,   "Sanyo",   "PHC-25",		GAME_NOT_WORKING | GAME_NO_SOUND)

