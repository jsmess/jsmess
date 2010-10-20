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
//	AM_RANGE(0x0000, 0x000f) Keyboard ROW reading
//	AM_RANGE(0x0010, 0x0010) Printer / Calendar Clock Interface
//	AM_RANGE(0x0020, 0x0021) RS-232C
//	AM_RANGE(0x0030, 0x0030) (R) DSW1 (W) Text Control Port 0
//	AM_RANGE(0x0031, 0x0031) (R) DSW2 (W) System Port 1
//	AM_RANGE(0x0032, 0x0032) (R) ? (W) System Port 2
//	AM_RANGE(0x0034, 0x0034) GVRAM Control Port 1
//	AM_RANGE(0x0035, 0x0035) GVRAM Control Port 2
//	AM_RANGE(0x0040, 0x0040) (R) System Port 4 (W) System port 3 (strobe port)
//	AM_RANGE(0x0044, 0x0045) YM2203
//	AM_RANGE(0x0046, 0x0047) YM2203 mirror
//	AM_RANGE(0x005c, 0x005c) (R) GVRAM status
//	AM_RANGE(0x005c, 0x005f) (W) GVRAM selection
//	AM_RANGE(0x0070, 0x0070) ? (*)
//	AM_RANGE(0x0071, 0x0071) Expansion ROM select (*)
//	AM_RANGE(0x0078, 0x0078) Memory offset increment (*)
//	AM_RANGE(0x0080, 0x0083) HDD control, byte access 7-0
//	AM_RANGE(0x00bc, 0x00bf) d8255 1
//	AM_RANGE(0x00e2, 0x00e3) Expansion RAM selection (*)
//	AM_RANGE(0x00e4, 0x00e4) 8214 IRQ control (*)
//	AM_RANGE(0x00e6, 0x00e6) 8214 IRQ mask (*)
//	AM_RANGE(0x00e8, 0x00e9) ? (*)
//	AM_RANGE(0x00ec, 0x00ed) ? (*)
//	AM_RANGE(0x00fc, 0x00ff) d8255 2, FDD

//	AM_RANGE(0x0100, 0x0101) Screen Control Register
//	AM_RANGE(0x0102, 0x0103) Graphic Screen Control Register
//	AM_RANGE(0x0106, 0x0107) Palette Control Register
//	AM_RANGE(0x0108, 0x0109) Direct Color Control Register
//	AM_RANGE(0x010a, 0x010b) Picture Mask Mode Register
//	AM_RANGE(0x010c, 0x010d) Color Palette Mode Register
//	AM_RANGE(0x010e, 0x010f) Backdrop Color Register
//	AM_RANGE(0x0110, 0x0111) Color Code/Plain Mask Register
//	AM_RANGE(0x0124, 0x0125) ? (related to Transparent Color of Graphic Screen 0)
//	AM_RANGE(0x0126, 0x0127) ? (related to Transparent Color of Graphic Screen 1)
//	AM_RANGE(0x012e, 0x012f) ? (related to Transparent Color of Text/Sprite)
//	AM_RANGE(0x0130, 0x0137) Picture Mask Parameter
//	AM_RANGE(0x0142, 0x0143) Text Controller (IDP) - (R) Status (W) command
//	AM_RANGE(0x0146, 0x0147) Text Controller (IDP) - (R/W) Parameter
//	AM_RANGE(0x0148, 0x0149) Text control port 1
//	AM_RANGE(0x014c, 0x014f) ? CG Port
//	AM_RANGE(0x0150, 0x0151) System Operational Mode
//	AM_RANGE(0x0152, 0x0153) Memory Map Register
//	AM_RANGE(0x0154, 0x0155) Refresh Register (wait states)
//	AM_RANGE(0x0156, 0x0157) ROM bank status
//	AM_RANGE(0x0158, 0x0159) Interruption Mode Modification
//	AM_RANGE(0x015c, 0x015f) NMI mask port (strobe port)
//	AM_RANGE(0x0160, 0x016f) DMA Controller
//	AM_RANGE(0x0184, 0x018b) IRQ Controller
//	AM_RANGE(0x0190, 0x0191) System Port 5
//	AM_RANGE(0x0196, 0x0197) Keyboard sub CPU command port
//	AM_RANGE(0x0198, 0x019b) Memory switch write inhibit/permission
//	AM_RANGE(0x01a0, 0x01a7) TCU (timer counter unit)
//	AM_RANGE(0x01a8, 0x01a9) General-purpose timer 3 control port
//	AM_RANGE(0x01b0, 0x01bb) FDC related (765)
//	AM_RANGE(0x01c0, 0x01c1) ?
//	AM_RANGE(0x01c8, 0x01cf) i8255 3 (byte access)
//	AM_RANGE(0x01d0, 0x01d1) Expansion RAM bank selection
//	AM_RANGE(0x0200, 0x021f) Frame buffer 0 control parameter
//	AM_RANGE(0x0220, 0x023f) Frame buffer 1 control parameter
//	AM_RANGE(0x0240, 0x025f) Frame buffer 2 control parameter
//	AM_RANGE(0x0260, 0x027f) Frame buffer 3 control parameter
//	AM_RANGE(0x0300, 0x033f) Palette RAM (xBBBBxRRRRxGGGG format)

//	AM_RANGE(0x0500, 0x05ff) GVRAM
//	AM_RANGE(0x1000, 0xfeff) user area (???)
//	AM_RANGE(0xff00, 0xffff) CPU internal use
ADDRESS_MAP_END
// (*) are specific N88 V1 / V2 ports

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
	ROM_LOAD( "varom00.rom",   0x00000, 0x80000, CRC(98c9959a) SHA1(bcaea28c58816602ca1e8290f534360f1ca03fe8) )	// multiple banks to be loaded at 0x0000?
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
