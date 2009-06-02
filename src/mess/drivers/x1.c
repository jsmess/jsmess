/***************************************************************************

	Sharp X1

	05/2009 Skeleton driver.

	=============  X1 series  =============
	
	X1 (CZ-800C) - November, 1982
	 * CPU: z80A @ 4MHz, 80C49 x 2 (one for key scan, the other for TV & Cas Ctrl)
	 * ROM: IPL (4KB) + chargen (2KB)
	 * RAM: Main memory (64KB) + VRAM (4KB) + RAM for PCG (6KB) + GRAM (48KB, Option)
	 * Text Mode: 80x25 or 40x25
	 * Graphic Mode: 640x200 or 320x200, 8 colors
	 * Sound: PSG 8 octave
	 * I/O Ports: Centronic ports, 2 Joystick ports, Cassette port (2700 baud)

	X1C (CZ-801C) - October, 1983
	 * same but only 48KB GRAM

	X1D (CZ-802C) - October, 1983
	 * same as X1C but with a 3" floppy drive (notice: 3" not 3" 1/2!!)

	X1Cs (CZ-803C) - June, 1984
	 * two expansion I/O ports

	X1Ck (CZ-804C) - June, 1984
	 * same as X1Cs
	 * ROM: IPL (4KB) + chargen (2KB) + Kanji 1st level

	X1F Model 10 (CZ-811C) - July, 1985
	 * Re-designed
	 * ROM: IPL (4KB) + chargen (2KB)

	X1F Model 20 (CZ-812C) - July, 1985
	 * Re-designed (same as Model 10)
	 * ROM: IPL (4KB) + chargen (2KB) + Kanji
	 * Built Tape drive plus a 5" floppy drive was available

	X1G Model 10 (CZ-820C) - July, 1986
	 * Re-designed again
	 * ROM: IPL (4KB) + chargen (2KB)

	X1G Model 30 (CZ-822C) - July, 1986
	 * Re-designed again (same as Model 10)
	 * ROM: IPL (4KB) + chargen (2KB) + Kanji
	 * Built Tape drive plus a 5" floppy drive was available

	X1twin (CZ-830C) - December, 1986
	 * Re-designed again (same as Model 10)
	 * ROM: IPL (4KB) + chargen (2KB) + Kanji
	 * Built Tape drive plus a 5" floppy drive was available
	 * It contains a PC-Engine

	=============  X1 Turbo series  =============
	
	X1turbo Model 30 (CZ-852C) - October, 1984
	 * CPU: z80A @ 4MHz, 80C49 x 2 (one for key scan, the other for TV & Cas Ctrl)
	 * ROM: IPL (32KB) + chargen (8KB) + Kanji (128KB)
	 * RAM: Main memory (64KB) + VRAM (6KB) + RAM for PCG (6KB) + GRAM (96KB)
	 * Text Mode: 80xCh or 40xCh with Ch = 10, 12, 20, 25 (same for Japanese display)
	 * Graphic Mode: 640x200 or 320x200, 8 colors
	 * Sound: PSG 8 octave
	 * I/O Ports: Centronic ports, 2 Joystick ports, built-in Cassette interface,
		2 Floppy drive for 5" disks, two expansion I/O ports

	X1turbo Model 20 (CZ-851C) - October, 1984
	 * same as Model 30, but only 1 Floppy drive is included

	X1turbo Model 10 (CZ-850C) - October, 1984
	 * same as Model 30, but Floppy drive is optional and GRAM is 48KB (it can
		be expanded to 96KB however)

	X1turbo Model 40 (CZ-862C) - July, 1985
	 * same as Model 30, but uses tv screen (you could watch television with this)

	X1turboII (CZ-856C) - November, 1985
	 * same as Model 30, but restyled, cheaper and sold with utility software

	X1turboIII (CZ-870C) - November, 1986
	 * with two High Density Floppy driver

	X1turboZ (CZ-880C) - December, 1986
	 * CPU: z80A @ 4MHz, 80C49 x 2 (one for key scan, the other for TV & Cas Ctrl)
	 * ROM: IPL (32KB) + chargen (8KB) + Kanji 1st & 2nd level
	 * RAM: Main memory (64KB) + VRAM (6KB) + RAM for PCG (6KB) + GRAM (96KB)
	 * Text Mode: 80xCh or 40xCh with Ch = 10, 12, 20, 25 (same for Japanese display)
	 * Graphic Mode: 640x200 or 320x200, 8 colors [in compatibility mode],
		640x400, 8 colors (out of 4096); 320x400, 64 colors (out of 4096);
		320x200, 4096 colors [in multimode],
	 * Sound: PSG 8 octave + FM 8 octave
	 * I/O Ports: Centronic ports, 2 Joystick ports, built-in Cassette interface,
		2 Floppy drive for HD 5" disks, two expansion I/O ports

	X1turboZII (CZ-881C) - December, 1987
	 * same as turboZ, but added 64KB expansion RAM

	X1turboZIII (CZ-888C) - December, 1988
	 * same as turboZII, but no more built-in cassette drive

	Please refer to http://www2s.biglobe.ne.jp/~ITTO/x1/x1menu.html for 
	more info

	BASIC has to be loaded from external media (tape or disk), the 
	computer only has an Initial Program Loader (IPL)

	TODO: everything, in particular to sort out the BIOS dumps 
	(especially CGROM dumps). Also, notice this is a color computer, 
	despite the skeleton code says it's black ad white

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START( x1_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( x1_io , ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( x1 )
INPUT_PORTS_END


static MACHINE_RESET( x1 )
{
}

static VIDEO_START( x1 )
{
}

static VIDEO_UPDATE( x1 )
{
	return 0;
}

static MACHINE_DRIVER_START( x1 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, XTAL_4MHz)
	MDRV_CPU_PROGRAM_MAP(x1_mem)
	MDRV_CPU_IO_MAP(x1_io)

	MDRV_MACHINE_RESET(x1)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(x1)
	MDRV_VIDEO_UPDATE(x1)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(x1)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( x1 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "default", "X1 IPL" )
	ROMX_LOAD( "ipl.x1", 0x0000, 0x1000, CRC(7b28d9de) SHA1(c4db9a6e99873808c8022afd1c50fef556a8b44d), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "alt", "X1 IPL (alt)" )
	ROMX_LOAD( "ipl_alt.x1", 0x0000, 0x1000, CRC(e70011d3) SHA1(d3395e9aeb5b8bbba7654dd471bcd8af228ee69a), ROM_BIOS(2) )

	ROM_REGION(0x0800, "cgrom", 0)
	ROM_LOAD("fnt0808.x1", 0x0000, 0x0800, CRC(e3995a57) SHA1(1c1a0d8c9f4c446ccd7470516b215ddca5052fb2) )
ROM_END

ROM_START( x1ck )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "default", "X1 IPL" )
	ROMX_LOAD( "ipl.x1", 0x0000, 0x1000, CRC(7b28d9de) SHA1(c4db9a6e99873808c8022afd1c50fef556a8b44d), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "alt", "X1 IPL (alt)" )
	ROMX_LOAD( "ipl_alt.x1", 0x0000, 0x1000, CRC(e70011d3) SHA1(d3395e9aeb5b8bbba7654dd471bcd8af228ee69a), ROM_BIOS(2) )

	ROM_REGION(0x0800, "cgrom", 0)
	ROM_LOAD("fnt0808.x1", 0x0000, 0x0800, CRC(e3995a57) SHA1(1c1a0d8c9f4c446ccd7470516b215ddca5052fb2) )

	ROM_REGION(0x4ac00, "kanji", 0)
	/* This is clearly a bad dump: size does not make sense and from 0x28000 on there are only 0xff */
	ROM_LOAD("kanji2.rom", 0x0000, 0x4ac00, BAD_DUMP CRC(33800ef2) SHA1(fc07a31ee30db312c7995e887519a9173cb38c0d) )
ROM_END

ROM_START( x1turbo )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "ipl.x1t", 0x0000, 0x8000, CRC(2e8b767c) SHA1(44620f57a25f0bcac2b57ca2b0f1ebad3bf305d3) )

	ROM_REGION(0x0800, "cgrom", 0)
	/* This should be larger... hence, we are missing something (maybe part of the other fnt roms?) */
	ROM_LOAD("fnt0808.x1", 0x0000, 0x0800, CRC(84a47530) SHA1(06c0995adc7a6609d4272417fe3570ca43bd0454) )

	ROM_REGION(0x4ac00, "kanji", 0)
	/* This is clearly a bad dump: size does not make sense and from 0x28000 on there are only 0xff */
	ROM_LOAD("kanji2.rom", 0x0000, 0x4ac00, BAD_DUMP CRC(33800ef2) SHA1(fc07a31ee30db312c7995e887519a9173cb38c0d) )
ROM_END

/* X1 Turbo Z: IPL is supposed to be the same as X1 Turbo, but which dumps should be in "cgrom"? 
Many emulators come with fnt0816.x1 & fnt1616.x1 but I am not sure about what was present on the real 
X1 Turbo / Turbo Z */
ROM_START( x1turboz )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "ipl.x1t", 0x0000, 0x8000, CRC(2e8b767c) SHA1(44620f57a25f0bcac2b57ca2b0f1ebad3bf305d3) )

	ROM_REGION(0x4c600, "cgrom", 0)
	ROM_LOAD("fnt0808.x1", 0x0000, 0x0800, CRC(84a47530) SHA1(06c0995adc7a6609d4272417fe3570ca43bd0454) )
	ROM_SYSTEM_BIOS( 0, "font1", "Font set 1" )
	ROMX_LOAD("fnt0816.x1", 0x0800, 0x1000, BAD_DUMP CRC(34818d54) SHA1(2c5fdd73249421af5509e48a94c52c4e423402bf), ROM_BIOS(1) )
	/* I strongly suspect this is not genuine */
	ROMX_LOAD("fnt1616.x1", 0x01800, 0x4ac00, BAD_DUMP CRC(46826745) SHA1(b9e6c320611f0842df6f45673c47c3e23bc14272), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "font2", "Font set 2" )
	ROMX_LOAD("fnt0816_a.x1", 0x0800, 0x1000, BAD_DUMP CRC(98db5a6b) SHA1(adf1492ef326b0f8820a3caa1915ad5ab8138f49), ROM_BIOS(2) )
	/* I strongly suspect this is not genuine */
	ROMX_LOAD("fnt1616_a.x1", 0x01800, 0x4ac00, BAD_DUMP CRC(bc5689ae) SHA1(a414116e261eb92bbdd407f63c8513257cd5a86f), ROM_BIOS(2) )

	ROM_REGION(0x4ac00, "kanji", 0)
	/* This is clearly a bad dump: size does not make sense and from 0x28000 on there are only 0xff */
	ROM_LOAD("kanji2.rom", 0x0000, 0x4ac00, BAD_DUMP CRC(33800ef2) SHA1(fc07a31ee30db312c7995e887519a9173cb38c0d) )
ROM_END


/* Driver */

/*    YEAR  NAME       PARENT  COMPAT   MACHINE  INPUT  INIT  CONFIG COMPANY   FULLNAME      FLAGS */
COMP( 1982, x1,        0,      0,       x1,      x1,    0,    x1,    "Sharp",  "X1 (CZ-800C)",         GAME_NOT_WORKING)
COMP( 1984, x1ck,      x1,     0,       x1,      x1,    0,    x1,    "Sharp",  "X1Ck (CZ-804C)",       GAME_NOT_WORKING)
COMP( 1984, x1turbo,   x1,     0,       x1,      x1,    0,    x1,    "Sharp",  "X1 Turbo (CZ-850C)",   GAME_NOT_WORKING)
COMP( 1986, x1turboz,  x1,     0,       x1,      x1,    0,    x1,    "Sharp",  "X1 Turbo Z (CZ-880C)", GAME_NOT_WORKING)
