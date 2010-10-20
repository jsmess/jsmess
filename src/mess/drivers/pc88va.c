/********************************************************************************************

	PC-88VA (c) 1987 NEC

	A follow up of the regular PC-8801. It can also run PC-8801 software in compatible mode

	preliminary driver by Angelo Salese

	TODO:
	- Does this system have one or two CPUs? I'm prone to think that the V30 does all the job
	  and then enters into z80 compatible mode for PC-8801 emulation.

********************************************************************************************/

#include "emu.h"
#include "cpu/nec/nec.h"
//#include "cpu/z80/z80.h"

static VIDEO_START( pc88va )
{

}

static VIDEO_UPDATE( pc88va )
{
	return 0;
}

static ADDRESS_MAP_START( pc88va_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000, 0x7ffff) AM_RAM
	AM_RANGE(0x80000, 0x9ffff) AM_RAM // EMM
	AM_RANGE(0xe0000, 0xfffff) AM_ROM AM_REGION("boot_code",0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc88va_io_map, ADDRESS_SPACE_IO, 16 )
ADDRESS_MAP_END

#if 0
static ADDRESS_MAP_START( pc88va_z80_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc88va_z80_io_map, ADDRESS_SPACE_IO, 8 )
ADDRESS_MAP_END
#endif

static INPUT_PORTS_START( pc88va )
INPUT_PORTS_END

static MACHINE_CONFIG_START( pc88va, driver_device )

	MDRV_CPU_ADD("maincpu", V30, 8000000)        /* 8 MHz */
	MDRV_CPU_PROGRAM_MAP(pc88va_map)
	MDRV_CPU_IO_MAP(pc88va_io_map)

	#if 0
	MDRV_CPU_ADD("subcpu", Z80, 8000000)        /* 8 MHz */
	MDRV_CPU_PROGRAM_MAP(pc88va_z80_map)
	MDRV_CPU_IO_MAP(pc88va_z80_io_map)
	#endif

	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MDRV_PALETTE_LENGTH(32)
//	MDRV_PALETTE_INIT( pc8801 )

	MDRV_VIDEO_START( pc88va )
	MDRV_VIDEO_UPDATE( pc88va )

MACHINE_CONFIG_END


ROM_START( pc88va )
	ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASEFF )

	ROM_REGION( 0x100000, "subcpu", ROMREGION_ERASEFF )

	ROM_REGION( 0xc0000, "rom", 0 )
	ROM_LOAD( "varom00.rom",   0x10000, 0x80000, CRC(98c9959a) SHA1(bcaea28c58816602ca1e8290f534360f1ca03fe8) )	// multiple banks to be loaded at 0x0000?
	ROM_LOAD( "varom08.rom",   0x80000, 0x20000, CRC(eef6d4a0) SHA1(47e5f89f8b0ce18ff8d5d7b7aef8ca0a2a8e3345) )	// multiple banks to be loaded at 0x8000?
	ROM_LOAD( "varom1.rom",    0xa0000, 0x20000, CRC(7e767f00) SHA1(dd4f4521bfbb068f15ab3bcdb8d47c7d82b9d1d4) )

	ROM_REGION( 0x20000, "boot_code", 0 )
	ROM_COPY( "rom", 0xb0000, 0x00000, 0x10000 )
	ROM_COPY( "rom", 0xa0000, 0x10000, 0x10000 )

	ROM_REGION( 0x2000, "sub", 0 )		// not sure what this should do...
	ROM_LOAD( "vasubsys.rom", 0x0000, 0x2000, CRC(08962850) SHA1(a9375aa480f85e1422a0e1385acb0ea170c5c2e0) )

	/* No idea of the proper size: it has never been dumped */
	ROM_REGION( 0x2000, "audiocpu", 0)
	ROM_LOAD( "soundbios.rom", 0x0000, 0x2000, NO_DUMP )

	ROM_REGION( 0x50000, "gfx1", 0 )
	ROM_LOAD( "vafont.rom", 0x00000, 0x50000, BAD_DUMP CRC(b40d34e4) SHA1(a0227d1fbc2da5db4b46d8d2c7e7a9ac2d91379f) )

	/* 32 banks, to be loaded at 0xc000 - 0xffff */
	ROM_REGION( 0x80000, "dictionary", 0 )
	ROM_LOAD( "vadic.rom", 0x00000, 0x80000, CRC(a6108f4d) SHA1(3665db538598abb45d9dfe636423e6728a812b12) )
ROM_END

COMP( 1987, pc88va,         0,		0,     pc88va,   pc88va,  0,    "Nippon Electronic Company",  "PC-88VA", GAME_NOT_WORKING | GAME_NO_SOUND)
//COMP( 1988, pc88va2,      pc88va, 0,     pc88va,   pc88va,  0,    "Nippon Electronic Company",  "PC-88VA2", GAME_NOT_WORKING )
//COMP( 1988, pc88va3,      pc88va, 0,     pc88va,   pc88va,  0,    "Nippon Electronic Company",  "PC-88VA3", GAME_NOT_WORKING )
