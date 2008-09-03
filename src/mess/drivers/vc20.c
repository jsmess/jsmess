/***************************************************************************

    commodore vic20 home computer
    PeT mess@utanet.at

    documentation
     Marko.Makela@HUT.FI (vic6560)
     www.funet.fi

***************************************************************************/

/*
------------------------------------
vic20 commodore vic20 (ntsc version)
vc20  commodore vc20  (pal version)
------------------------------------

vic20 ntsc-version
 when screen is two wide right or low,
  or screen doesn't fitt in visible area
  or gameplay is too fast
 try the pal version

vc20  pal-version

a normal or good written program does not rely on the video system

vic20cr
 cost reduced
 only modified for cheaper production (newer rams, ...)
 (2 2kx8 rams instead of 8 1kx4 rams, ...)

State
-----

rasterline based video system
 should be enough for all vic20 games and programs
imperfect sound
timer system only 98% accurate
simple tape support
serial bus
 simple disk support
 no printer or other devices
no userport
 no rs232/v.24 interface
keyboard
gameport
 joystick
 paddles
 lightpen
expansion slot
 ram and rom cartridges
 no special expansion modules like ieee488 interface
Quickloader

for a more complete vic20 emulation take a look at the very good
vice emulator

Video
-----
NTSC:
Screen Size (normal TV (horizontal),4/3 ratio)
pixel ratio: about 7/5 !
so no standard Vesa Resolution is good
tweaked mode 256x256 acceptable
best define own display mode (when graphic driver supports this)
PAL:
pixel ratio about 13/10 !
good (but only part of screen filled) 1024x768
acceptable 800x600
better define own display mode (when graphic driver supports this)

XMESS:
use -scalewidth 3 -scaleheight 2 for acceptable display
better define own display mode

Keys
----
Some PC-Keyboards does not behave well when special two or more keys are
pressed at the same time
(with my keyboard printscreen clears the pressed pause key!)

stop-restore in much cases prompt will appear
shift-cbm switches between upper-only and normal character set
(when wrong characters on screen this can help)
run (shift-stop) load and start program from tape

Lightpen
--------
Paddle 3 x-axe
Paddle 4 y-axe

Tape
----
(DAC 1 volume in noise volume)
loading of wav, prg and prg files in zip archiv
commandline -cassette image
wav:
 8 or 16(not tested) bit, mono, 12500 Hz minimum
 has the same problems like an original tape drive (tone head must
 be adjusted to get working (no load error,...) wav-files)
zip:
 must be placed in current directory
 prg's are played in the order of the files in zip file

use LOAD or LOAD"" or LOAD"",1 for loading of normal programs
use LOAD"",1,1 for loading programs to their special address

several programs rely on more features
(loading other file types, writing, ...)

Discs
-----
only file load from drive 8 and 9 implemented
 loads file from rom directory (*.prg,*.p00)(must NOT be specified on commandline)
 or file from d64 image (here also directory command LOAD"$",8 supported)
LOAD"filename",8
or LOAD"filename",8,1 (for loading machine language programs at their address)
for loading
type RUN or the appropriate SYS call to start them

several programs rely on more features
(loading other file types, writing, ...)

some games rely on starting own programs in the floppy drive
(and therefore CPU level emulation is needed)

Roms
----
.bin .rom .a0 .20 .40 .60 .prg
files with boot-sign in it
  recognized as roms

.20 files loaded at 0x2000
.40 files loaded at 0x4000
.60 files loaded at 0x6000
.a0 files loaded at 0xa000
.prg files loaded at address in its first two bytes
.bin .rom files loaded at 0x4000 when 0x4000 bytes long
else loaded at 0xa000

Quickloader
-----------
.prg files supported
loads program into memory and sets program end pointer
(works with most programs)
program ready to get started with RUN
currently loads first rom when you press quickload key (f8)

when problems start with -log and look into error.log file
 */


#include "driver.h"

#define VERBOSE_DBG 0
#include "includes/cbm.h"
#include "machine/6522via.h"
#include "includes/cbmserb.h"
#include "includes/vc1541.h"
#include "machine/cbmipt.h"
#include "video/vic6560.h"

#include "includes/vc20.h"


static ADDRESS_MAP_START( vc20_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x03ff) AM_RAM
	AM_RANGE(0x0400, 0x0fff) AM_ROMBANK(1) AM_WRITE( vc20_0400_w )	/* ram, rom or nothing */
	AM_RANGE(0x1000, 0x1fff) AM_RAM
	AM_RANGE(0x2000, 0x3fff) AM_ROMBANK(2) AM_WRITE( vc20_2000_w )	/* ram, rom or nothing */
	AM_RANGE(0x4000, 0x5fff) AM_ROMBANK(3) AM_WRITE( vc20_4000_w )	/* ram, rom or nothing */
	AM_RANGE(0x6000, 0x7fff) AM_ROMBANK(4) AM_WRITE( vc20_6000_w )	/* ram, rom or nothing */
	AM_RANGE(0x8000, 0x8fff) AM_ROM
	AM_RANGE(0x9000, 0x900f) AM_READWRITE( vic6560_port_r, vic6560_port_w )
	AM_RANGE(0x9010, 0x910f) AM_NOP
	AM_RANGE(0x9110, 0x911f) AM_READWRITE( via_0_r, via_0_w )
	AM_RANGE(0x9120, 0x912f) AM_READWRITE( via_1_r, via_1_w )
	AM_RANGE(0x9130, 0x93ff) AM_NOP
	AM_RANGE(0x9400, 0x97ff) AM_READWRITE(SMH_RAM, vc20_write_9400) AM_BASE(&vc20_memory_9400)	/*color ram 4 bit */
	AM_RANGE(0x9800, 0x9fff) AM_RAM
	AM_RANGE(0xa000, 0xbfff) AM_ROMBANK(5)
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( vc20i_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x03ff) AM_RAM
	AM_RANGE(0x0400, 0x0fff) AM_ROMBANK(1) AM_WRITE( vc20_0400_w )	/* ram, rom or nothing */
	AM_RANGE(0x1000, 0x1fff) AM_RAM
	AM_RANGE(0x2000, 0x3fff) AM_ROMBANK(2) AM_WRITE( vc20_2000_w )	/* ram, rom or nothing */
	AM_RANGE(0x4000, 0x5fff) AM_ROMBANK(3) AM_WRITE( vc20_4000_w )	/* ram, rom or nothing */
	AM_RANGE(0x6000, 0x7fff) AM_ROMBANK(4) AM_WRITE( vc20_6000_w )	/* ram, rom or nothing */
	AM_RANGE(0x8000, 0x8fff) AM_ROM
	AM_RANGE(0x9000, 0x900f) AM_READWRITE( vic6560_port_r, vic6560_port_w )
	AM_RANGE(0x9010, 0x910f) AM_NOP
	AM_RANGE(0x9110, 0x911f) AM_READWRITE( via_0_r, via_0_w )
	AM_RANGE(0x9120, 0x912f) AM_READWRITE( via_1_r, via_1_w )
	AM_RANGE(0x9400, 0x97ff) AM_READWRITE( SMH_RAM, vc20_write_9400) AM_BASE(&vc20_memory_9400)	/* color ram 4 bit */
	AM_RANGE(0x9800, 0x980f) AM_READWRITE( via_4_r, via_4_w )
	AM_RANGE(0x9810, 0x981f) AM_READWRITE( via_5_r, via_5_w )
	AM_RANGE(0xa000, 0xbfff) AM_ROMBANK(5)
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END


/*************************************
 *
 *  Input Ports
 *
 *************************************/

/* 2008-05 FP: These lightpen input ports, would better 
fit machine/cbmipt.c but PORT_MINMAX requires vic6560.c 
constants. Therefore, they will stay here, atm    */

static INPUT_PORTS_START( vic_lightpen_6560 )
	PORT_START( "LIGHTX" )
	PORT_BIT (0xff, 0, IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(2) PORT_MINMAX(0,(VIC6560_MAME_XSIZE - 1)) PORT_CODE_DEC(KEYCODE_LEFT) PORT_CODE_INC(KEYCODE_RIGHT) PORT_CODE_DEC(JOYCODE_X_LEFT_SWITCH) PORT_CODE_INC(JOYCODE_X_RIGHT_SWITCH) PORT_PLAYER(3)

	PORT_START( "LIGHTY" )
	PORT_BIT (0xff, 0, IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(2) PORT_MINMAX(0,(VIC6560_MAME_YSIZE - 1)) PORT_CODE_DEC(KEYCODE_UP) PORT_CODE_INC(KEYCODE_DOWN) PORT_CODE_DEC(JOYCODE_Y_UP_SWITCH) PORT_CODE_INC(JOYCODE_Y_DOWN_SWITCH) PORT_PLAYER(4)
INPUT_PORTS_END


static INPUT_PORTS_START( vic_lightpen_6561 )
	PORT_START( "LIGHTX" )
	PORT_BIT (0xff, 0, IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(2) PORT_MINMAX(0,(VIC6560_MAME_XSIZE - 1)) PORT_CODE_DEC(KEYCODE_LEFT) PORT_CODE_INC(KEYCODE_RIGHT) PORT_CODE_DEC(JOYCODE_X_LEFT_SWITCH) PORT_CODE_INC(JOYCODE_X_RIGHT_SWITCH) PORT_PLAYER(3)

	PORT_START( "LIGHTY" )
	PORT_BIT (0x1ff, 0, IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(2) PORT_MINMAX(0,(VIC6561_MAME_YSIZE - 1)) PORT_CODE_DEC(KEYCODE_UP) PORT_CODE_INC(KEYCODE_DOWN) PORT_CODE_DEC(JOYCODE_Y_UP_SWITCH) PORT_CODE_INC(JOYCODE_Y_DOWN_SWITCH) PORT_PLAYER(4)
INPUT_PORTS_END


static INPUT_PORTS_START (vic20)
	PORT_INCLUDE( vic_keyboard )		/* ROW0 -> ROW7 */

	PORT_INCLUDE( vic_special )			/* SPECIAL */

	PORT_INCLUDE( vic_controls )		/* JOY, PADDLE0, PADDLE1 */
	
	/* Lightpen 6560 */
	PORT_INCLUDE( vic_lightpen_6560 )	/* LIGHTX, LIGHTY */

	PORT_INCLUDE( vic_expansion )		/* EXP */

	PORT_INCLUDE( vic_config )			/* CFG */
INPUT_PORTS_END


static INPUT_PORTS_START (vic1001)
	PORT_INCLUDE( vic20 )

	PORT_MODIFY( "ROW0" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('\xA5')
INPUT_PORTS_END


static INPUT_PORTS_START (vic20i)
	PORT_INCLUDE( vic20 )

	PORT_MODIFY( "CFG" )
	PORT_DIPNAME( 0x02, 0x02, "IEEE/Dev 8/Floppy Sim")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
#if 1
	/* ieee simu currently not a bus, so only 1 device */
	PORT_BIT( 0x01, 0x00, IPT_UNUSED )
#else
	PORT_DIPNAME( 0x01, 0x01, "IEEE/Dev 9/Floppy Sim")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Yes ) )
#endif
INPUT_PORTS_END


static INPUT_PORTS_START (vc20)
	PORT_INCLUDE( vic_keyboard )		/* ROW0 -> ROW7 */

	PORT_INCLUDE( vic_special )			/* SPECIAL */

	PORT_INCLUDE( vic_controls )		/* JOY, PADDLE0, PADDLE1 */
		
	/* Lightpen 6561 */
	PORT_INCLUDE( vic_lightpen_6561 )	/* LIGHTX, LIGHTY */

	PORT_INCLUDE( vic_expansion )		/* EXP */

	PORT_INCLUDE( vic_config )			/* CFG */
INPUT_PORTS_END


static INPUT_PORTS_START (vic20swe)
	PORT_INCLUDE( vc20 )

	PORT_MODIFY( "ROW0" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH2)		PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS)			PORT_CHAR('-')
	PORT_MODIFY( "ROW1" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE)		PORT_CHAR('@')	
	PORT_MODIFY( "ROW2" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE)			PORT_CHAR(0x00C4)
	PORT_MODIFY( "ROW5" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH)		PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON)			PORT_CHAR(0x00D6)
	PORT_MODIFY( "ROW6" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE)		PORT_CHAR(0x00C5)
	PORT_MODIFY( "ROW7" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS)			PORT_CHAR('=')
INPUT_PORTS_END



/* Initialise the vc20 palette */
static PALETTE_INIT( vc20 )
{
	int i;

	for ( i = 0; i < sizeof(vic6560_palette) / 3; i++ ) {
		palette_set_color_rgb(machine, i, vic6560_palette[i*3], vic6560_palette[i*3+1], vic6560_palette[i*3+2]);
	}
}

#if 0
	/* chargen */
	ROM_LOAD ("901460.03", 0x8000, 0x1000, CRC(83e032a6))
	/* basic */
	ROM_LOAD ("901486.01", 0xc000, 0x2000, CRC(db4c43c1))
	/* kernel ntsc-m of vic1001? */
	ROM_LOAD ("901486.02", 0xe000, 0x2000, CRC(336900d7))
	/* kernel ntsc */
	ROM_LOAD ("901486.06", 0xe000, 0x2000, CRC(e5e7c174))
	/* kernel pal */
	ROM_LOAD ("901486.07", 0xe000, 0x2000, CRC(4be07cb4))

	/* patched pal system for swedish/finish keyboard and chars */
	/* but in rom? (maybe patched means in this case nec version) */
	ROM_LOAD ("nec22101.207", 0x8000, 0x1000, CRC(d808551d))
	ROM_LOAD ("nec22081.206", 0xe000, 0x2000, CRC(b2a60662))

	/* ieee488 cartridge */
	ROM_LOAD ("325329-04.bin", 0xb000, 0x800, CRC(d37b6335))
#endif

ROM_START (vic20)
	ROM_REGION (0x10000, "main",0)
	ROM_FILL( 0x0000, 0x8000, 0xFF )
	ROM_LOAD ("901460.03", 0x8000, 0x1000, CRC(83e032a6) SHA1(4fd85ab6647ee2ac7ba40f729323f2472d35b9b4))
	ROM_FILL( 0x9000, 0x3000, 0xFF )
	ROM_LOAD ("901486.01", 0xc000, 0x2000, CRC(db4c43c1) SHA1(587d1e90950675ab6b12d91248a3f0d640d02e8d))
	ROM_LOAD ("901486.06", 0xe000, 0x2000, CRC(e5e7c174) SHA1(06de7ec017a5e78bd6746d89c2ecebb646efeb19))
ROM_END

ROM_START (vic1001)
	ROM_REGION (0x10000, "main",0)
	ROM_FILL( 0x0000, 0x8000, 0xFF )
	ROM_LOAD ("901460.02", 0x8000, 0x1000, CRC(fcfd8a4b) SHA1(dae61ac03065aa2904af5c123ce821855898c555))
	ROM_FILL( 0x9000, 0x3000, 0xFF )
	ROM_LOAD ("901486.01", 0xc000, 0x2000, CRC(db4c43c1) SHA1(587d1e90950675ab6b12d91248a3f0d640d02e8d))
	ROM_LOAD ("901486.02", 0xe000, 0x2000, CRC(336900d7) SHA1(c9ead45e6674d1042ca6199160e8583c23aeac22))
ROM_END

ROM_START (vic20swe)
	ROM_REGION (0x10000, "main",0)
	ROM_FILL( 0x0000, 0x8000, 0xFF )
	ROM_LOAD ("nec22101.207", 0x8000, 0x1000, CRC(d808551d) SHA1(f403f0b0ce5922bd61bbd768bdd6f0b38e648c9f))
	ROM_FILL( 0x9000, 0x3000, 0xFF )
	ROM_LOAD ("901486.01", 0xc000, 0x2000, CRC(db4c43c1) SHA1(587d1e90950675ab6b12d91248a3f0d640d02e8d))
	ROM_LOAD ("nec22081.206", 0xe000, 0x2000, CRC(b2a60662) SHA1(cb3e2f6e661ea7f567977751846ce9ad524651a3))
ROM_END

ROM_START (vic20v)
	ROM_REGION (0x10000, "main",0)
	ROM_FILL( 0x0000, 0x8000, 0xFF )
	ROM_LOAD ("901460.03", 0x8000, 0x1000, CRC(83e032a6) SHA1(4fd85ab6647ee2ac7ba40f729323f2472d35b9b4))
	ROM_FILL( 0x9000, 0x3000, 0xFF )
	ROM_LOAD ("901486.01", 0xc000, 0x2000, CRC(db4c43c1) SHA1(587d1e90950675ab6b12d91248a3f0d640d02e8d))
	ROM_LOAD ("901486.06", 0xe000, 0x2000, CRC(e5e7c174) SHA1(06de7ec017a5e78bd6746d89c2ecebb646efeb19))
	VC1540_ROM ("cpu_vc1540")
ROM_END

ROM_START (vic20i)
	ROM_REGION (0x10000, "main",0)
	ROM_FILL( 0x0000, 0x8000, 0xFF )
	ROM_LOAD ("901460.03", 0x8000, 0x1000, CRC(83e032a6) SHA1(4fd85ab6647ee2ac7ba40f729323f2472d35b9b4))
	ROM_FILL( 0x9000, 0x2000, 0xFF )
	ROM_LOAD ("325329.04", 0xb000, 0x800, CRC(d37b6335) SHA1(828c965829d21c60e8c2d083caee045c639a270f))
	ROM_LOAD ("901486.01", 0xc000, 0x2000, CRC(db4c43c1) SHA1(587d1e90950675ab6b12d91248a3f0d640d02e8d))
	ROM_LOAD ("901486.06", 0xe000, 0x2000, CRC(e5e7c174) SHA1(06de7ec017a5e78bd6746d89c2ecebb646efeb19))
/*	C2031_ROM ("cpu_vc1540") */
ROM_END

ROM_START (vc20)
	ROM_REGION (0x10000, "main",0)
	ROM_FILL( 0x0000, 0x8000, 0xFF )
	ROM_LOAD ("901460.03", 0x8000, 0x1000, CRC(83e032a6) SHA1(4fd85ab6647ee2ac7ba40f729323f2472d35b9b4))
	ROM_FILL( 0x9000, 0x3000, 0xFF )
	ROM_LOAD ("901486.01", 0xc000, 0x2000, CRC(db4c43c1) SHA1(587d1e90950675ab6b12d91248a3f0d640d02e8d))
	ROM_LOAD ("901486.07", 0xe000, 0x2000, CRC(4be07cb4) SHA1(ce0137ed69f003a299f43538fa9eee27898e621e))
ROM_END

ROM_START (vc20v)
	ROM_REGION (0x10000, "main",0)
	ROM_FILL( 0x0000, 0x8000, 0xFF )
	ROM_LOAD ("901460.03", 0x8000, 0x1000, CRC(83e032a6) SHA1(4fd85ab6647ee2ac7ba40f729323f2472d35b9b4))
	ROM_FILL( 0x9000, 0x3000, 0xFF )
	ROM_LOAD ("901486.01", 0xc000, 0x2000, CRC(db4c43c1) SHA1(587d1e90950675ab6b12d91248a3f0d640d02e8d))
	ROM_LOAD ("901486.07", 0xe000, 0x2000, CRC(4be07cb4) SHA1(ce0137ed69f003a299f43538fa9eee27898e621e))
	VC1541_ROM ("cpu_vc1540")
ROM_END

static MACHINE_DRIVER_START( vic20 )
	/* basic machine hardware */
	MDRV_CPU_ADD("main", M6502, VIC6560_CLOCK)        /* 7.8336 Mhz */
	MDRV_CPU_PROGRAM_MAP(vc20_mem, 0)
	MDRV_CPU_VBLANK_INT("main", vic20_frame_interrupt)
	MDRV_CPU_PERIODIC_INT(vic656x_raster_interrupt, VIC656X_HRETRACERATE)
	MDRV_INTERLEAVE(0)

	MDRV_MACHINE_RESET( vic20 )

    /* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(VIC6560_VRETRACERATE)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE((VIC6560_XSIZE + 7) & ~7, VIC6560_YSIZE)
	MDRV_SCREEN_VISIBLE_AREA(VIC6560_MAME_XPOS, VIC6560_MAME_XPOS + VIC6560_MAME_XSIZE - 1, VIC6560_MAME_YPOS, VIC6560_MAME_YPOS + VIC6560_MAME_YSIZE - 1)
	MDRV_PALETTE_LENGTH(sizeof (vic6560_palette) / sizeof (vic6560_palette[0]) / 3)
	MDRV_PALETTE_INIT( vc20 )

	MDRV_VIDEO_START( vic6560 )
	MDRV_VIDEO_UPDATE( vic6560 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("custom", CUSTOM, 0)
	MDRV_SOUND_CONFIG(vic6560_sound_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD("dac", DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
	MDRV_QUICKLOAD_ADD(cbm_vc20, "p00,prg", CBM_QUICKLOAD_DELAY_SECONDS)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( vic20v )
	MDRV_IMPORT_FROM( vic20 )
//	MDRV_IMPORT_FROM( cpu_vc1540 )
#ifdef CPU_SYNC
	MDRV_INTERLEAVE(1)
#else
	MDRV_INTERLEAVE(3000)
#endif
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( vic20i )
	MDRV_IMPORT_FROM( vic20 )
	/*MDRV_IMPORT_FROM( cpu_c2031 )*/
#if 1 || CPU_SYNC
	MDRV_INTERLEAVE(1)
#else
	MDRV_INTERLEAVE(3000)
#endif

	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP( vc20i_mem, 0 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( vc20 )
	MDRV_IMPORT_FROM( vic20 )

	MDRV_CPU_REPLACE( "main", M6502, VIC6561_CLOCK )
	MDRV_CPU_PROGRAM_MAP( vc20i_mem, 0 )

	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_REFRESH_RATE(VIC6561_VRETRACERATE)
	MDRV_SCREEN_SIZE((VIC6561_XSIZE + 7) & ~7, VIC6561_YSIZE)
	MDRV_SCREEN_VISIBLE_AREA(VIC6561_MAME_XPOS, VIC6561_MAME_XPOS + VIC6561_MAME_XSIZE - 1, VIC6561_MAME_YPOS, VIC6561_MAME_YPOS + VIC6561_MAME_YSIZE - 1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( vc20v )
	MDRV_IMPORT_FROM( vc20 )
//	MDRV_IMPORT_FROM( cpu_vc1540 )
#ifdef CPU_SYNC
	MDRV_INTERLEAVE(1)
#else
	MDRV_INTERLEAVE(3000)
#endif
MACHINE_DRIVER_END


static SYSTEM_CONFIG_START(vic20)
	CONFIG_DEVICE(vic20_cartslot_getinfo)
	CONFIG_DEVICE(cbmfloppy_device_getinfo)
	CONFIG_DEVICE(datasette_device_getinfo)
	CONFIG_RAM_DEFAULT(5 * 1024)
	CONFIG_RAM(8 * 1024)
	CONFIG_RAM(16 * 1024)
	CONFIG_RAM(24 * 1024)
	CONFIG_RAM(32 * 1024)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(vic20v)
	CONFIG_DEVICE(vic20_cartslot_getinfo)
	CONFIG_DEVICE(datasette_device_getinfo)
	CONFIG_DEVICE(vc1541_device_getinfo)
	CONFIG_RAM_DEFAULT(5 * 1024)
	CONFIG_RAM(8 * 1024)
	CONFIG_RAM(16 * 1024)
	CONFIG_RAM(24 * 1024)
	CONFIG_RAM(32 * 1024)
SYSTEM_CONFIG_END

/*      YEAR    NAME        PARENT  COMPAT  MACHINE INPUT       INIT    CONFIG     COMPANY                          FULLNAME */
COMP ( 1981,	vic20,		0,		0,		vic20,	vic20,		vic20,	vic20,      "Commodore Business Machines Co.",  "VIC20 (NTSC)", GAME_IMPERFECT_SOUND)
COMP ( 1981,	vic20i, 	vic20,	0,		vic20i, vic20i, 	vic20i, vic20,      "Commodore Business Machines Co.",  "VIC20 (NTSC), IEEE488 Interface (SYS45065)",   GAME_IMPERFECT_SOUND)
COMP ( 1981,	vic1001,	vic20,	0,		vic20,	vic1001,	vic20,	vic20,      "Commodore Business Machines Co.",  "VIC1001 (NTSC)", GAME_IMPERFECT_SOUND)
COMP ( 1981,	vc20,		vic20,	0,		vc20,	vc20,		vc20,	vic20,      "Commodore Business Machines Co.",  "VIC20/VC20(German) PAL",       GAME_IMPERFECT_SOUND)
COMP ( 1981,	vic20swe,	vic20,	0,		vc20,	vic20swe,	vc20,	vic20,      "Commodore Business Machines Co.",  "VIC20 PAL, Swedish Expansion Kit", GAME_IMPERFECT_SOUND)
COMP ( 1981,	vic20v, 	vic20,	0,		vic20v, vic20,		vic20,	vic20v,     "Commodore Business Machines Co.",  "VIC20 (NTSC), VC1540", GAME_IMPERFECT_SOUND | GAME_NOT_WORKING )
COMP ( 1981,	vc20v,		vic20,	0,		vc20v,	vic20,		vc20,	vic20v,     "Commodore Business Machines Co.",  "VC20 (PAL), VC1541", GAME_IMPERFECT_SOUND | GAME_NOT_WORKING )
