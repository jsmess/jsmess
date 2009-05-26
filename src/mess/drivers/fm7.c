/***********************************************************************************************

	Fujitsu Micro 7 (FM-7)

	12/05/2009 Skeleton driver.
	
	Computers in this series:
	
                 | Release |    Main CPU    |  Sub CPU  |              RAM              |
    =====================================================================================
    FM-8         | 1981-05 | M68A09 @ 1MHz  |  M6809    |    64K (main) + 48K (VRAM)    |
    FM-7         | 1982-11 | M68B09 @ 2MHz  |  M68B09   |    64K (main) + 48K (VRAM)    |
    FM-NEW7      | 1984-05 | M68B09 @ 2MHz  |  M68B09   |    64K (main) + 48K (VRAM)    |
    FM-77        | 1984-05 | M68B09 @ 2MHz  |  M68B09E  |  64/256K (main) + 48K (VRAM)  |
    FM-77AV      | 1985-10 | M68B09E @ 2MHz |  M68B09   | 128/192K (main) + 96K (VRAM)  |
    FM-77AV20    | 1986-10 | M68B09E @ 2MHz |  M68B09   | 128/192K (main) + 96K (VRAM)  |
    FM-77AV40    | 1986-10 | M68B09E @ 2MHz |  M68B09   | 192/448K (main) + 144K (VRAM) |
    FM-77AV20EX  | 1987-11 | M68B09E @ 2MHz |  M68B09   | 128/192K (main) + 96K (VRAM)  |
    FM-77AV40EX  | 1987-11 | M68B09E @ 2MHz |  M68B09   | 192/448K (main) + 144K (VRAM) |
    FM-77AV40SX  | 1988-11 | M68B09E @ 2MHz |  M68B09   | 192/448K (main) + 144K (VRAM) |

	Note: FM-77AV dumps probably come from a FM-77AV40SX. Shall we confirm that both computers
	used the same BIOS components?

	memory map info from http://www.nausicaa.net/~lgreenf/fm7page.htm
	see also http://retropc.net/ryu/xm7/xm7.shtml

************************************************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"


static ADDRESS_MAP_START( fm7_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
/*
   0000 - 7FFF: (RAM) BASIC working area, user's area
   8000 - FBFF: (ROM) F-BASIC ROM
   FC00 - FC7F: Shared RAM between main and sub CPU
   FD00 - FDFF: I/O space (6809 uses memory-mapped I/O)
   FE00 - FFEF: Boot rom
   FFF0 - FFFF: Interrupt vector table
*/
ADDRESS_MAP_END

static ADDRESS_MAP_START( fm7_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START( fm7_sub_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
/*
   0000 - 3FFF: Video RAM bank 0 (Blue plane)
   4000 - 7FFF: Video RAM bank 1 (Red plane)
   8000 - BFFF: Video RAM bank 2 (Green plane)
   C000 - C2FF: (RAM) working area
   C300 - C37F: Shared RAM between main and sub CPU
   C400 - FFDF: (ROM) Graphics command code
   FFF0 - FFFF: Interrupt vector table
*/
ADDRESS_MAP_END

static ADDRESS_MAP_START( fm77av_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START( fm77av_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START( fm77av_sub_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( fm7 )
INPUT_PORTS_END


static MACHINE_RESET(fm7)
{
}

static VIDEO_START( fm7 )
{
}

static VIDEO_UPDATE( fm7 )
{
	return 0;
}

static MACHINE_DRIVER_START( fm7 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M6809, XTAL_2MHz)
	MDRV_CPU_PROGRAM_MAP(fm7_mem)
	MDRV_CPU_IO_MAP(fm7_io)

	MDRV_CPU_ADD("sub", M6809, XTAL_2MHz)
	MDRV_CPU_PROGRAM_MAP(fm7_sub_mem)

	MDRV_MACHINE_RESET(fm7)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(fm7)
	MDRV_VIDEO_UPDATE(fm7)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( fm77av )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M6809E, XTAL_2MHz)
	MDRV_CPU_PROGRAM_MAP(fm77av_mem)
	MDRV_CPU_IO_MAP(fm77av_io)

	MDRV_CPU_ADD("sub", M6809, XTAL_2MHz)
	MDRV_CPU_PROGRAM_MAP(fm77av_sub_mem)

	MDRV_MACHINE_RESET(fm7)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(fm7)
	MDRV_VIDEO_UPDATE(fm7)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( fm7 )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "fbasic30.rom", 0x38000,  0x7c00, CRC(a96d19b6) SHA1(8d5f0cfe7e0d39bf2ab7e4c798a13004769c28b2) )
	/* These must be loaded at 0x3fe00, but I'm not sure if they existed together in the real thing */
	ROM_SYSTEM_BIOS(0, "basic", "Basic" )
	ROMX_LOAD( "boot_bas.rom", 0x3fe00,  0x0200, CRC(c70f0c74) SHA1(53b63a301cba7e3030e79c59a4d4291eab6e64b0), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS(1, "dos", "DOS" )
	ROMX_LOAD( "boot_dos.rom", 0x3fe00,  0x0200, CRC(198614ff) SHA1(037e5881bd3fed472a210ee894a6446965a8d2ef), ROM_BIOS(2) )

	ROM_REGION( 0x20000, "sub", 0 )
	ROM_LOAD( "subsys_c.rom", 0x1d800,  0x2800, CRC(24cec93f) SHA1(50b7283db6fe1342c6063fc94046283f4feddc1c) )

	// optional Kanji rom?
ROM_END

ROM_START( fm77av )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "initiate.rom", 0x36000,  0x2000, CRC(785cb06c) SHA1(b65987e98a9564a82c85eadb86f0204eee5a5c93) )
	ROM_LOAD( "fbasic30.rom", 0x38000,  0x7c00, CRC(a96d19b6) SHA1(8d5f0cfe7e0d39bf2ab7e4c798a13004769c28b2) )

	ROM_REGION( 0x22000, "sub", 0 )
	ROM_SYSTEM_BIOS(0, "mona", "Monitor Type A" )
	ROMX_LOAD( "subsys_a.rom", 0x1e000,  0x2000, CRC(e8014fbb) SHA1(038cb0b42aee9e933b20fccd6f19942e2f476c83), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS(1, "monb", "Monitor Type B" )
	ROMX_LOAD( "subsys_b.rom", 0x1e000,  0x2000, CRC(9be69fac) SHA1(0305bdd44e7d9b7b6a17675aff0a3330a08d21a8), ROM_BIOS(2) )
	/* 4 0x800 banks to be banked at 1d800 */
	ROM_LOAD( "subsyscg.rom", 0x20000,  0x2000, CRC(e9f16c42) SHA1(8ab466b1546d023ba54987790a79e9815d2b7bb2) )

	ROM_REGION( 0x20000, "gfx1", 0 )
	ROM_LOAD( "kanji.rom", 0x0000, 0x20000, CRC(62402ac9) SHA1(bf52d22b119d54410dad4949b0687bb0edf3e143) )

	// optional dict rom?
ROM_END

ROM_START( fm7740sx )
	ROM_REGION( 0x40000, "maincpu", 0 )
	ROM_LOAD( "initiate.rom", 0x36000,  0x2000, CRC(785cb06c) SHA1(b65987e98a9564a82c85eadb86f0204eee5a5c93) )
	ROM_LOAD( "fbasic30.rom", 0x38000,  0x7c00, CRC(a96d19b6) SHA1(8d5f0cfe7e0d39bf2ab7e4c798a13004769c28b2) )

	ROM_REGION( 0x22000, "sub", 0 )
	ROM_SYSTEM_BIOS(0, "mona", "Monitor Type A" )
	ROMX_LOAD( "subsys_a.rom", 0x1e000,  0x2000, CRC(e8014fbb) SHA1(038cb0b42aee9e933b20fccd6f19942e2f476c83), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS(1, "monb", "Monitor Type B" )
	ROMX_LOAD( "subsys_b.rom", 0x1e000,  0x2000, CRC(9be69fac) SHA1(0305bdd44e7d9b7b6a17675aff0a3330a08d21a8), ROM_BIOS(2) )
	/* 4 0x800 banks to be banked at 1d800 */
	ROM_LOAD( "subsyscg.rom", 0x20000,  0x2000, CRC(e9f16c42) SHA1(8ab466b1546d023ba54987790a79e9815d2b7bb2) )

	ROM_REGION( 0x20000, "gfx1", 0 )
	ROM_LOAD( "kanji.rom",  0x0000, 0x20000, CRC(62402ac9) SHA1(bf52d22b119d54410dad4949b0687bb0edf3e143) )
	ROM_LOAD( "kanji2.rom", 0x0000, 0x20000, CRC(38644251) SHA1(ebfdc43c38e1380709ed08575c346b2467ad1592) )

	/* These should be loaded at 2e000-2ffff of maincpu, but I'm not sure if it is correct */
	ROM_REGION( 0x4c000, "additional", 0 )
	ROM_LOAD( "dicrom.rom", 0x00000, 0x40000, CRC(b142acbc) SHA1(fe9f92a8a2750bcba0a1d2895e75e83858e4f97f) )
	ROM_LOAD( "extsub.rom", 0x40000, 0x0c000, CRC(0f7fcce3) SHA1(a1304457eeb400b4edd3c20af948d66a04df255e) )
ROM_END


static SYSTEM_CONFIG_START(fm7)
SYSTEM_CONFIG_END


/* Driver */

/*    YEAR  NAME      PARENT  COMPAT  MACHINE  INPUT   INIT  CONFIG  COMPANY      FULLNAME        FLAGS */
COMP( 1982, fm7,      0,      0,      fm7,     fm7,    0,    fm7,    "Fujitsu",   "FM-7",         GAME_NOT_WORKING)
COMP( 1985, fm77av,   fm7,    0,      fm77av,  fm7,    0,    fm7,    "Fujitsu",   "FM-77AV",      GAME_NOT_WORKING)
COMP( 1985, fm7740sx, fm7,    0,      fm77av,  fm7,    0,    fm7,    "Fujitsu",   "FM-77AV40SX",  GAME_NOT_WORKING)
