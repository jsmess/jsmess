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
Screen Size (normal TV (horicontal),4/3 ratio)
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
(and therefor cpu level emulation is needed)

Roms
----
.bin .rom .a0 .20 .40 .60 .prg
files with boot-sign in it
  recogniced as roms

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
#include "includes/vc20.h"
#include "machine/6522via.h"
#include "includes/cbmserb.h"
#include "includes/vc1541.h"
#include "includes/vc20tape.h"
#include "includes/vic6560.h"
#include "devices/cartslot.h"

static ADDRESS_MAP_START( vc20_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x03ff) AM_RAM AM_BASE(&vc20_memory)
#if 0
	AM_RANGE(0x0400, 0x0fff) AM_READ(MRA8_RAM)	/* ram, rom or nothing; I think read 0xff! */
#endif
	AM_RANGE(0x1000, 0x1fff) AM_RAM
#if 0
	AM_RANGE(0x2000, 0x3fff) AM_READ(MRA8_RAM)	/* ram, rom or nothing */
	AM_RANGE(0x4000, 0x5fff) AM_READ(MRA8_RAM)	/* ram, rom or nothing */
	AM_RANGE(0x6000, 0x7fff) AM_READ(MRA8_RAM)	/* ram, rom or nothing */
#endif
	AM_RANGE(0x8000, 0x8fff) AM_ROM
	AM_RANGE(0x9000, 0x900f) AM_READWRITE( vic6560_port_r, vic6560_port_w )
	AM_RANGE(0x9010, 0x910f) AM_NOP
	AM_RANGE(0x9110, 0x911f) AM_READWRITE( via_0_r, via_0_w )
	AM_RANGE(0x9120, 0x912f) AM_READWRITE( via_1_r, via_1_w )
	AM_RANGE(0x9130, 0x93ff) AM_NOP
	AM_RANGE(0x9400, 0x97ff) AM_READWRITE(MRA8_RAM, vc20_write_9400) AM_BASE(&vc20_memory_9400)	/*color ram 4 bit */
	AM_RANGE(0x9800, 0x9fff) AM_NOP
#if 0
	AM_RANGE(0xa000, 0xbfff) AM_ROM		   /* or nothing */
#endif
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( vc20i_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x03ff) AM_RAM AM_BASE(&vc20_memory)
#if 0
	AM_RANGE(0x0400, 0x0fff) AM_READ(MRA8_RAM)	/* ram, rom or nothing; I think read 0xff! */
#endif
	AM_RANGE(0x1000, 0x1fff) AM_RAM
#if 0
	AM_RANGE(0x2000, 0x3fff) AM_READ(MRA8_RAM)	/* ram, rom or nothing */
	AM_RANGE(0x4000, 0x5fff) AM_READ(MRA8_RAM)	/* ram, rom or nothing */
	AM_RANGE(0x6000, 0x7fff) AM_READ(MRA8_RAM)	/* ram, rom or nothing */
#endif
	AM_RANGE(0x8000, 0x8fff) AM_ROM
	AM_RANGE(0x9000, 0x900f) AM_READWRITE( vic6560_port_r, vic6560_port_w )
	AM_RANGE(0x9010, 0x910f) AM_NOP
	AM_RANGE(0x9110, 0x911f) AM_READWRITE( via_0_r, via_0_w )
	AM_RANGE(0x9120, 0x912f) AM_READWRITE( via_1_r, via_1_w )
	AM_RANGE(0x9400, 0x97ff) AM_READWRITE( MRA8_RAM, vc20_write_9400) AM_BASE(&vc20_memory_9400)	/* color ram 4 bit */
	AM_RANGE(0x9800, 0x980f) AM_READWRITE( via_4_r, via_4_w )
	AM_RANGE(0x9810, 0x981f) AM_READWRITE( via_5_r, via_5_w )
	AM_RANGE(0xa000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END

#define DIPS_HELPER(bit, name, keycode) \
   PORT_BIT(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(name) PORT_CODE(keycode)

#define DIPS_JOY \
	PORT_START\
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_NAME("Joystick Up")  PORT_8WAY\
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_NAME("Joystick Down")  PORT_8WAY\
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_NAME("Joystick Left")  PORT_8WAY\
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_NAME("Joystick Right")  PORT_8WAY\
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Joystick Button") \
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Paddle 1 Button") PORT_CODE(KEYCODE_INSERT) \
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Paddle 2 Button") PORT_CODE(KEYCODE_DEL) \
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Lightpen Signal") PORT_CODE(KEYCODE_LALT) \
	PORT_START \
	PORT_BIT(0xff,128,IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(20) PORT_MINMAX(0,255) PORT_CODE_DEC(KEYCODE_HOME) PORT_CODE_INC(KEYCODE_PGUP) PORT_REVERSE\
	PORT_START \
	PORT_BIT(0xff,128,IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(20) PORT_MINMAX(0,255) PORT_CODE_DEC(KEYCODE_END) PORT_CODE_INC(KEYCODE_PGDN) PORT_PLAYER(2) PORT_REVERSE

#define DIPS_INPUT \
	PORT_START \
	DIPS_HELPER( 0x8000, "Arrow-Left",        KEYCODE_TILDE)\
	DIPS_HELPER( 0x4000, "1 !   BLK",         KEYCODE_1)\
	DIPS_HELPER( 0x2000, "2 \"   WHT",        KEYCODE_2)\
	DIPS_HELPER( 0x1000, "3 #   RED",         KEYCODE_3)\
	DIPS_HELPER( 0x0800, "4 $   CYN",         KEYCODE_4)\
	DIPS_HELPER( 0x0400, "5 %   PUR",         KEYCODE_5)\
	DIPS_HELPER( 0x0200, "6 &   GRN",         KEYCODE_6)\
	DIPS_HELPER( 0x0100, "7 '   BLU",         KEYCODE_7)\
	DIPS_HELPER( 0x0080, "8 (   YEL",         KEYCODE_8)\
	DIPS_HELPER( 0x0040, "9 )   RVS-ON",      KEYCODE_9)\
	DIPS_HELPER( 0x0020, "0     RVS-OFF",     KEYCODE_0)\
	DIPS_HELPER( 0x0010, "+",                 KEYCODE_PLUS_PAD)\
	DIPS_HELPER( 0x0008, "-",                 KEYCODE_MINUS_PAD)\
	DIPS_HELPER( 0x0004, "Pound",             KEYCODE_MINUS)\
	DIPS_HELPER( 0x0002, "HOME CLR",          KEYCODE_EQUALS)\
	DIPS_HELPER( 0x0001, "DEL INST",          KEYCODE_BACKSPACE)\
	PORT_START \
	DIPS_HELPER( 0x8000, "CTRL",              KEYCODE_RCONTROL)\
	DIPS_HELPER( 0x4000, "Q",                 KEYCODE_Q)\
	DIPS_HELPER( 0x2000, "W",                 KEYCODE_W)\
	DIPS_HELPER( 0x1000, "E",                 KEYCODE_E)\
	DIPS_HELPER( 0x0800, "R",                 KEYCODE_R)\
	DIPS_HELPER( 0x0400, "T",                 KEYCODE_T)\
	DIPS_HELPER( 0x0200, "Y",                 KEYCODE_Y)\
	DIPS_HELPER( 0x0100, "U",                 KEYCODE_U)\
	DIPS_HELPER( 0x0080, "I",                 KEYCODE_I)\
	DIPS_HELPER( 0x0040, "O",                 KEYCODE_O)\
	DIPS_HELPER( 0x0020, "P",                 KEYCODE_P)\
	DIPS_HELPER( 0x0010, "At",                KEYCODE_OPENBRACE)\
	DIPS_HELPER( 0x0008, "*",                 KEYCODE_ASTERISK)\
	DIPS_HELPER( 0x0004, "Arrow-Up Pi",       KEYCODE_CLOSEBRACE)\
	DIPS_HELPER( 0x0002, "RESTORE",               KEYCODE_PRTSCR)\
	PORT_START \
	DIPS_HELPER( 0x8000, "STOP RUN",          KEYCODE_TAB)\
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("SHIFT-LOCK (switch)") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE\
	DIPS_HELPER( 0x2000, "A",                 KEYCODE_A)\
	DIPS_HELPER( 0x1000, "S",                 KEYCODE_S)\
	DIPS_HELPER( 0x0800, "D",                 KEYCODE_D)\
	DIPS_HELPER( 0x0400, "F",                 KEYCODE_F)\
	DIPS_HELPER( 0x0200, "G",                 KEYCODE_G)\
	DIPS_HELPER( 0x0100, "H",                 KEYCODE_H)\
	DIPS_HELPER( 0x0080, "J",                 KEYCODE_J)\
	DIPS_HELPER( 0x0040, "K",                 KEYCODE_K)\
	DIPS_HELPER( 0x0020, "L",                 KEYCODE_L)\
	DIPS_HELPER( 0x0010, ": [",               KEYCODE_COLON)\
	DIPS_HELPER( 0x0008, "; ]",               KEYCODE_QUOTE)\
	DIPS_HELPER( 0x0004, "=",                 KEYCODE_BACKSLASH)\
	DIPS_HELPER( 0x0002, "RETURN",            KEYCODE_ENTER)\
	PORT_START \
	DIPS_HELPER( 0x8000, "CBM",               KEYCODE_RALT)\
	DIPS_HELPER( 0x4000, "Left-Shift",        KEYCODE_LSHIFT)\
	DIPS_HELPER( 0x2000, "Z",                 KEYCODE_Z)\
	DIPS_HELPER( 0x1000, "X",                 KEYCODE_X)\
	DIPS_HELPER( 0x0800, "C",                 KEYCODE_C)\
	DIPS_HELPER( 0x0400, "V",                 KEYCODE_V)\
	DIPS_HELPER( 0x0200, "B",                 KEYCODE_B)\
	DIPS_HELPER( 0x0100, "N",                 KEYCODE_N)\
	DIPS_HELPER( 0x0080, "M",                 KEYCODE_M)\
	DIPS_HELPER( 0x0040, ", <",               KEYCODE_COMMA)\
	DIPS_HELPER( 0x0020, ". >",               KEYCODE_STOP)\
	DIPS_HELPER( 0x0010, "/ ?",               KEYCODE_SLASH)\
	DIPS_HELPER( 0x0008, "Right-Shift",       KEYCODE_RSHIFT)\
	DIPS_HELPER( 0x0004, "CRSR-DOWN UP",      KEYCODE_2_PAD)\
	DIPS_HELPER( 0x0002, "CRSR-RIGHT LEFT",   KEYCODE_6_PAD)\
	PORT_START \
	DIPS_HELPER( 0x8000, "Space",             KEYCODE_SPACE)\
	DIPS_HELPER( 0x4000, "f1 f2",             KEYCODE_F1)\
	DIPS_HELPER( 0x2000, "f3 f4",             KEYCODE_F2)\
	DIPS_HELPER( 0x1000, "f5 f6",             KEYCODE_F3)\
	DIPS_HELPER( 0x0800, "f7 f8",             KEYCODE_F4)\
	DIPS_HELPER( 0x0400, "Special CRSR Up",       KEYCODE_8_PAD)\
	DIPS_HELPER( 0x0200, "Special CRSR Left",     KEYCODE_4_PAD)\
	DIPS_HELPER( 0x08, "Quickload",       KEYCODE_F8)\
	DIPS_HELPER( 0x04, "Tape Drive Play",       KEYCODE_F5)\
	DIPS_HELPER( 0x02, "Tape Drive Record",     KEYCODE_F6)\
	DIPS_HELPER( 0x01, "Tape Drive Stop",       KEYCODE_F7)

#define DIPS_BOTH \
	PORT_START \
	PORT_DIPNAME ( 0x07, 0, "RAM Cartridge")\
	PORT_DIPSETTING(	0, DEF_STR( None ) )\
	PORT_DIPSETTING(	1, "3k" )\
	PORT_DIPSETTING(	2, "8k" )\
	PORT_DIPSETTING(	3, "16k" )\
	PORT_DIPSETTING(	4, "32k" )\
	PORT_DIPSETTING(	5, "Custom" )\
	PORT_DIPNAME   ( 0x08, 0, " Ram at 0x0400")\
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )\
	PORT_DIPSETTING( 0x08, DEF_STR( Yes ) )\
	PORT_DIPNAME   ( 0x10, 0, " Ram at 0x2000")\
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )\
	PORT_DIPSETTING( 0x10, DEF_STR( Yes ) )\
	PORT_DIPNAME   ( 0x20, 0, " Ram at 0x4000")\
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )\
	PORT_DIPSETTING( 0x20, DEF_STR( Yes ) )\
	PORT_DIPNAME   ( 0x40, 0, " Ram at 0x6000")\
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )\
	PORT_DIPSETTING( 0x40, DEF_STR( Yes ) )\
	PORT_DIPNAME   ( 0x80, 0, " Ram at 0xa000")\
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )\
	PORT_DIPSETTING( 0x80, DEF_STR( Yes ) )\
	PORT_START \
	PORT_DIPNAME   ( 0x80, 0x80, DEF_STR( Joystick ))\
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )\
	PORT_DIPSETTING( 0x80, DEF_STR( Yes ) )\
	PORT_DIPNAME   ( 0x40, 0x40, "Paddles")\
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )\
	PORT_DIPSETTING( 0x40, DEF_STR( Yes ) )\
	PORT_DIPNAME   ( 0x20, 0x00, "Lightpen")\
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )\
	PORT_DIPSETTING( 0x20, DEF_STR( Yes ) )\
	PORT_DIPNAME   ( 0x10, 0x10, " Draw Pointer")\
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )\
	PORT_DIPSETTING( 0x10, DEF_STR( Yes ) )\
	PORT_DIPNAME   ( 0x08, 0x08, "Tape Drive/Device 1")\
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )\
	PORT_DIPSETTING( 0x08, DEF_STR( Yes ) )\
	PORT_DIPNAME   ( 0x04, 0x00, " Tape Sound")\
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )\
	PORT_DIPSETTING( 0x04, DEF_STR( Yes ) )\
	PORT_DIPNAME ( 0x02, 0x02, "Serial/Dev 8/VC1541 Floppy")\
	PORT_DIPSETTING(  0, DEF_STR( No ) )\
	PORT_DIPSETTING(0x02, DEF_STR( Yes ) )\
	PORT_DIPNAME ( 0x01, 0x01, "Serial/Dev 9/VC1541 Floppy")\
	PORT_DIPSETTING(  0, DEF_STR( No ) )\
	PORT_DIPSETTING(  1, DEF_STR( Yes ) )\

INPUT_PORTS_START (vic20)
	DIPS_JOY
	PORT_START							   /* in 16 lightpen X */
	PORT_BIT (0xff, 0, IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(2) PORT_MINMAX(0,(VIC6560_MAME_XSIZE - 1)) PORT_CODE_DEC(KEYCODE_LEFT) PORT_CODE_INC(KEYCODE_RIGHT) PORT_CODE_DEC(JOYCODE_1_LEFT) PORT_CODE_INC(JOYCODE_1_RIGHT) PORT_PLAYER(3)
	PORT_START							   /* in 17 lightpen Y */
	PORT_BIT (0xff, 0, IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(2) PORT_MINMAX(0,(VIC6560_MAME_YSIZE - 1)) PORT_CODE_DEC(KEYCODE_UP) PORT_CODE_INC(KEYCODE_DOWN) PORT_CODE_DEC(JOYCODE_1_UP) PORT_CODE_INC(JOYCODE_1_DOWN) PORT_PLAYER(4)
	DIPS_INPUT
	DIPS_BOTH
INPUT_PORTS_END

// some different labels to vic20
INPUT_PORTS_START (vic1001)
	DIPS_JOY
	PORT_START							   /* in 16 lightpen X */
	PORT_BIT (0xff, 0, IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(2) PORT_MINMAX(0,(VIC6560_MAME_XSIZE - 1)) PORT_CODE_DEC(KEYCODE_LEFT) PORT_CODE_INC(KEYCODE_RIGHT) PORT_CODE_DEC(JOYCODE_1_LEFT) PORT_CODE_INC(JOYCODE_1_RIGHT) PORT_PLAYER(3)
	PORT_START							   /* in 17 lightpen Y */
	PORT_BIT (0xff, 0, IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(2) PORT_MINMAX(0,(VIC6560_MAME_YSIZE - 1)) PORT_CODE_DEC(KEYCODE_UP) PORT_CODE_INC(KEYCODE_DOWN) PORT_CODE_DEC(JOYCODE_1_UP) PORT_CODE_INC(JOYCODE_1_DOWN) PORT_PLAYER(4)
	PORT_START 
	DIPS_HELPER( 0x8000, "Arrow-Left",        KEYCODE_TILDE)
	DIPS_HELPER( 0x4000, "1 !   BLK",         KEYCODE_1)
	DIPS_HELPER( 0x2000, "2 \"   WHT",        KEYCODE_2)
	DIPS_HELPER( 0x1000, "3 #   RED",         KEYCODE_3)
	DIPS_HELPER( 0x0800, "4 $   CYN",         KEYCODE_4)
	DIPS_HELPER( 0x0400, "5 %   PUR",         KEYCODE_5)
	DIPS_HELPER( 0x0200, "6 &   GRN",         KEYCODE_6)
	DIPS_HELPER( 0x0100, "7 '   BLU",         KEYCODE_7)
	DIPS_HELPER( 0x0080, "8 (   YEL",         KEYCODE_8)
	DIPS_HELPER( 0x0040, "9 )",		KEYCODE_9)
	DIPS_HELPER( 0x0020, "0",		KEYCODE_0)
	DIPS_HELPER( 0x0010, "+",                 KEYCODE_PLUS_PAD)
	DIPS_HELPER( 0x0008, "-",                 KEYCODE_MINUS_PAD)
	DIPS_HELPER( 0x0004, "Yen",		KEYCODE_MINUS)
	DIPS_HELPER( 0x0002, "HOME CLR",          KEYCODE_EQUALS)
	DIPS_HELPER( 0x0001, "DEL INST",          KEYCODE_BACKSPACE)
	PORT_START 
	DIPS_HELPER( 0x8000, "CTRL",              KEYCODE_RCONTROL)
	DIPS_HELPER( 0x4000, "Q",                 KEYCODE_Q)
	DIPS_HELPER( 0x2000, "W",                 KEYCODE_W)
	DIPS_HELPER( 0x1000, "E",                 KEYCODE_E)
	DIPS_HELPER( 0x0800, "R",                 KEYCODE_R)
	DIPS_HELPER( 0x0400, "T",                 KEYCODE_T)
	DIPS_HELPER( 0x0200, "Y",                 KEYCODE_Y)
	DIPS_HELPER( 0x0100, "U",                 KEYCODE_U)
	DIPS_HELPER( 0x0080, "I",                 KEYCODE_I)
	DIPS_HELPER( 0x0040, "O",                 KEYCODE_O)
	DIPS_HELPER( 0x0020, "P",                 KEYCODE_P)
	DIPS_HELPER( 0x0010, "At",                KEYCODE_OPENBRACE)
	DIPS_HELPER( 0x0008, "*",                 KEYCODE_ASTERISK)
	DIPS_HELPER( 0x0004, "Arrow-Up Pi",       KEYCODE_CLOSEBRACE)
	DIPS_HELPER( 0x0002, "RESTORE",               KEYCODE_PRTSCR)
	PORT_START 
	DIPS_HELPER( 0x8000, "STOP RUN",          KEYCODE_TAB)
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("SHIFT-LOCK (switch)") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE
	DIPS_HELPER( 0x2000, "A",                 KEYCODE_A)
	DIPS_HELPER( 0x1000, "S",                 KEYCODE_S)
	DIPS_HELPER( 0x0800, "D",                 KEYCODE_D)
	DIPS_HELPER( 0x0400, "F",                 KEYCODE_F)
	DIPS_HELPER( 0x0200, "G",                 KEYCODE_G)
	DIPS_HELPER( 0x0100, "H",                 KEYCODE_H)
	DIPS_HELPER( 0x0080, "J",                 KEYCODE_J)
	DIPS_HELPER( 0x0040, "K",                 KEYCODE_K)
	DIPS_HELPER( 0x0020, "L",                 KEYCODE_L)
	DIPS_HELPER( 0x0010, ": [",               KEYCODE_COLON)
	DIPS_HELPER( 0x0008, "; ]",               KEYCODE_QUOTE)
	DIPS_HELPER( 0x0004, "=",                 KEYCODE_BACKSLASH)
	DIPS_HELPER( 0x0002, "RETURN",            KEYCODE_ENTER)
	PORT_START 
	DIPS_HELPER( 0x8000, "CBM",               KEYCODE_RALT)
	DIPS_HELPER( 0x4000, "Left-Shift",        KEYCODE_LSHIFT)
	DIPS_HELPER( 0x2000, "Z",                 KEYCODE_Z)
	DIPS_HELPER( 0x1000, "X",                 KEYCODE_X)
	DIPS_HELPER( 0x0800, "C",                 KEYCODE_C)
	DIPS_HELPER( 0x0400, "V",                 KEYCODE_V)
	DIPS_HELPER( 0x0200, "B",                 KEYCODE_B)
	DIPS_HELPER( 0x0100, "N",                 KEYCODE_N)
	DIPS_HELPER( 0x0080, "M",                 KEYCODE_M)
	DIPS_HELPER( 0x0040, ", <",               KEYCODE_COMMA)
	DIPS_HELPER( 0x0020, ". >",               KEYCODE_STOP)
	DIPS_HELPER( 0x0010, "/ ?",               KEYCODE_SLASH)
	DIPS_HELPER( 0x0008, "Right-Shift",       KEYCODE_RSHIFT)
	DIPS_HELPER( 0x0004, "CRSR-DOWN UP",      KEYCODE_2_PAD)
	DIPS_HELPER( 0x0002, "CRSR-RIGHT LEFT",   KEYCODE_6_PAD)
	PORT_START
	DIPS_HELPER( 0x8000, "Space",             KEYCODE_SPACE)
	DIPS_HELPER( 0x4000, "f1 f2",             KEYCODE_F1)
	DIPS_HELPER( 0x2000, "f3 f4",             KEYCODE_F2)
	DIPS_HELPER( 0x1000, "f5 f6",             KEYCODE_F3)
	DIPS_HELPER( 0x0800, "f7 f8",             KEYCODE_F4)
	DIPS_HELPER( 0x0400, "Special CRSR Up",       KEYCODE_8_PAD)
	DIPS_HELPER( 0x0200, "Special CRSR Left",     KEYCODE_4_PAD)
	DIPS_HELPER( 0x08, "Quickload",       KEYCODE_F8)
	DIPS_HELPER( 0x04, "Tape Drive Play",       KEYCODE_F5)
	DIPS_HELPER( 0x02, "Tape Drive Record",     KEYCODE_F6)
	DIPS_HELPER( 0x01, "Tape Drive Stop",       KEYCODE_F7)
	DIPS_BOTH
INPUT_PORTS_END


INPUT_PORTS_START (vic20i)
	DIPS_JOY
	PORT_START							   /* in 16 lightpen X */
	PORT_BIT (0xff, 0, IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(2) PORT_MINMAX(0,(VIC6560_MAME_XSIZE - 1)) PORT_CODE_DEC(KEYCODE_LEFT) PORT_CODE_INC(KEYCODE_RIGHT) PORT_CODE_DEC(JOYCODE_1_LEFT) PORT_CODE_INC(JOYCODE_1_RIGHT) PORT_PLAYER(3)
	PORT_START							   /* in 17 lightpen Y */
	PORT_BIT (0xff, 0, IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(2) PORT_MINMAX(0,(VIC6560_MAME_YSIZE - 1)) PORT_CODE_DEC(KEYCODE_UP) PORT_CODE_INC(KEYCODE_DOWN) PORT_CODE_DEC(JOYCODE_1_UP) PORT_CODE_INC(JOYCODE_1_DOWN) PORT_PLAYER(4)
	DIPS_INPUT
	PORT_START
	PORT_DIPNAME ( 0x07, 0, "RAM Cartridge")
	PORT_DIPSETTING(	0, DEF_STR( None ) )
	PORT_DIPSETTING(	1, "3k" )
	PORT_DIPSETTING(	2, "8k" )
	PORT_DIPSETTING(	3, "16k" )
	PORT_DIPSETTING(	4, "32k" )
	PORT_DIPSETTING(	5, "Custom" )
	PORT_DIPNAME   ( 0x08, 0, " Ram at 0x0400")
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )
	PORT_DIPSETTING( 0x08, DEF_STR( Yes ) )
	PORT_DIPNAME   ( 0x10, 0, " Ram at 0x2000")
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )
	PORT_DIPSETTING( 0x10, DEF_STR( Yes ) )
	PORT_DIPNAME   ( 0x20, 0, " Ram at 0x4000")
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )
	PORT_DIPSETTING( 0x20, DEF_STR( Yes ) )
	PORT_DIPNAME   ( 0x40, 0, " Ram at 0x6000")
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )
	PORT_DIPSETTING( 0x40, DEF_STR( Yes ) )
	PORT_DIPNAME   ( 0x80, 0, " Ram at 0xa000")
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )
	PORT_DIPSETTING( 0x80, DEF_STR( Yes ) )
	PORT_START
	PORT_DIPNAME   ( 0x80, 0x80, DEF_STR( Joystick ))
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )
	PORT_DIPSETTING( 0x80, DEF_STR( Yes ) )
	PORT_DIPNAME   ( 0x40, 0x40, "Paddles")
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )
	PORT_DIPSETTING( 0x40, DEF_STR( Yes ) )
	PORT_DIPNAME   ( 0x20, 0x00, "Lightpen")
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )
	PORT_DIPSETTING( 0x20, DEF_STR( Yes ) )
	PORT_DIPNAME   ( 0x10, 0x10, " Draw Pointer")
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )
	PORT_DIPSETTING( 0x10, DEF_STR( Yes ) )
	PORT_DIPNAME   ( 0x08, 0x08, "Tape Drive/Device 1")
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )
	PORT_DIPSETTING( 0x08, DEF_STR( Yes ) )
	PORT_DIPNAME   ( 0x04, 0x00, " Tape Sound")
	PORT_DIPSETTING( 0x00, DEF_STR( No ) )
	PORT_DIPSETTING( 0x04, DEF_STR( Yes ) )
	PORT_DIPNAME ( 0x02, 0x02, "IEEE/Dev 8/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(0x02, DEF_STR( Yes ) )
#if 1
	/* ieee simu currently not a bus, so only 1 device */
	PORT_BIT( 0x01, 0,	IPT_UNUSED )
#else
	PORT_DIPNAME ( 0x01, 0x01, "IEEE/Dev 9/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(  1, DEF_STR( Yes ) )
#endif
INPUT_PORTS_END

INPUT_PORTS_START (vc20)
	DIPS_JOY
	PORT_START							   /* in 16 lightpen X */
	PORT_BIT (0xff, 0, IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(2) PORT_MINMAX(0,(VIC6561_MAME_XSIZE - 1)) PORT_CODE_DEC(KEYCODE_LEFT) PORT_CODE_INC(KEYCODE_RIGHT) PORT_CODE_DEC(JOYCODE_1_LEFT) PORT_CODE_INC(JOYCODE_1_RIGHT) PORT_PLAYER(3)
	PORT_START							   /* in 17 lightpen Y */
	PORT_BIT (0x1ff, 0, IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(2) PORT_MINMAX(0,(VIC6561_MAME_YSIZE - 1)) PORT_CODE_DEC(KEYCODE_UP) PORT_CODE_INC(KEYCODE_DOWN) PORT_CODE_DEC(JOYCODE_1_UP) PORT_CODE_INC(JOYCODE_1_DOWN) PORT_PLAYER(4)
	DIPS_INPUT
	DIPS_BOTH
INPUT_PORTS_END

// some different labels to vc20
INPUT_PORTS_START (vic20swe)
	DIPS_JOY
	PORT_START							   /* in 16 lightpen X */
	PORT_BIT (0xff, 0, IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(2) PORT_MINMAX(0,(VIC6561_MAME_XSIZE - 1)) PORT_CODE_DEC(KEYCODE_LEFT) PORT_CODE_INC(KEYCODE_RIGHT) PORT_CODE_DEC(JOYCODE_1_LEFT) PORT_CODE_INC(JOYCODE_1_RIGHT) PORT_PLAYER(3)
	PORT_START							   /* in 17 lightpen Y */
	PORT_BIT (0x1ff, 0, IPT_PADDLE) PORT_SENSITIVITY(30) PORT_KEYDELTA(2) PORT_MINMAX(0,(VIC6561_MAME_YSIZE - 1)) PORT_CODE_DEC(KEYCODE_UP) PORT_CODE_INC(KEYCODE_DOWN) PORT_CODE_DEC(JOYCODE_1_UP) PORT_CODE_INC(JOYCODE_1_DOWN) PORT_PLAYER(4)
	PORT_START 
	DIPS_HELPER( 0x8000, "Arrow-Left",        KEYCODE_TILDE)
	DIPS_HELPER( 0x4000, "1 !   BLK",         KEYCODE_1)
	DIPS_HELPER( 0x2000, "2 \"   WHT",        KEYCODE_2)
	DIPS_HELPER( 0x1000, "3 #   RED",         KEYCODE_3)
	DIPS_HELPER( 0x0800, "4 $   CYN",         KEYCODE_4)
	DIPS_HELPER( 0x0400, "5 %   PUR",         KEYCODE_5)
	DIPS_HELPER( 0x0200, "6 &   GRN",         KEYCODE_6)
	DIPS_HELPER( 0x0100, "7 '   BLU",         KEYCODE_7)
	DIPS_HELPER( 0x0080, "8 (   YEL",         KEYCODE_8)
	DIPS_HELPER( 0x0040, "9 )   RVS-ON",      KEYCODE_9)
	DIPS_HELPER( 0x0020, "0     RVS-OFF",     KEYCODE_0)
	DIPS_HELPER( 0x0010, "-",                 KEYCODE_PLUS_PAD)
	DIPS_HELPER( 0x0008, "=",                 KEYCODE_MINUS_PAD)
	DIPS_HELPER( 0x0004, ":     *",             KEYCODE_MINUS)
	DIPS_HELPER( 0x0002, "HOME CLR",          KEYCODE_EQUALS)
	DIPS_HELPER( 0x0001, "DEL INST",          KEYCODE_BACKSPACE)
	PORT_START 
	DIPS_HELPER( 0x8000, "CTRL",              KEYCODE_RCONTROL)
	DIPS_HELPER( 0x4000, "Q",                 KEYCODE_Q)
	DIPS_HELPER( 0x2000, "W",                 KEYCODE_W)
	DIPS_HELPER( 0x1000, "E",                 KEYCODE_E)
	DIPS_HELPER( 0x0800, "R",                 KEYCODE_R)
	DIPS_HELPER( 0x0400, "T",                 KEYCODE_T)
	DIPS_HELPER( 0x0200, "Y",                 KEYCODE_Y)
	DIPS_HELPER( 0x0100, "U",                 KEYCODE_U)
	DIPS_HELPER( 0x0080, "I",                 KEYCODE_I)
	DIPS_HELPER( 0x0040, "O",                 KEYCODE_O)
	DIPS_HELPER( 0x0020, "P",                 KEYCODE_P)
	DIPS_HELPER( 0x0010, "Overcircle-A",                KEYCODE_OPENBRACE)
	DIPS_HELPER( 0x0008, "At",                 KEYCODE_ASTERISK)
	DIPS_HELPER( 0x0004, "Arrow-Up Pi",       KEYCODE_CLOSEBRACE)
	DIPS_HELPER( 0x0002, "RESTORE",               KEYCODE_PRTSCR)
	PORT_START 
	DIPS_HELPER( 0x8000, "STOP RUN",          KEYCODE_TAB)
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("SHIFT-LOCK (switch)") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE
	DIPS_HELPER( 0x2000, "A",                 KEYCODE_A)
	DIPS_HELPER( 0x1000, "S",                 KEYCODE_S)
	DIPS_HELPER( 0x0800, "D",                 KEYCODE_D)
	DIPS_HELPER( 0x0400, "F",                 KEYCODE_F)
	DIPS_HELPER( 0x0200, "G",                 KEYCODE_G)
	DIPS_HELPER( 0x0100, "H",                 KEYCODE_H)
	DIPS_HELPER( 0x0080, "J",                 KEYCODE_J)
	DIPS_HELPER( 0x0040, "K",                 KEYCODE_K)
	DIPS_HELPER( 0x0020, "L",                 KEYCODE_L)
	DIPS_HELPER( 0x0010, "Diaresis-O",               KEYCODE_COLON)
	DIPS_HELPER( 0x0008, "Diaresis-A",               KEYCODE_QUOTE)
	DIPS_HELPER( 0x0004, ";        +",                 KEYCODE_BACKSLASH)
	DIPS_HELPER( 0x0002, "RETURN",            KEYCODE_ENTER)
	PORT_START 
	DIPS_HELPER( 0x8000, "CBM",               KEYCODE_RALT)
	DIPS_HELPER( 0x4000, "Left-Shift",        KEYCODE_LSHIFT)
	DIPS_HELPER( 0x2000, "Z",                 KEYCODE_Z)
	DIPS_HELPER( 0x1000, "X",                 KEYCODE_X)
	DIPS_HELPER( 0x0800, "C",                 KEYCODE_C)
	DIPS_HELPER( 0x0400, "V",                 KEYCODE_V)
	DIPS_HELPER( 0x0200, "B",                 KEYCODE_B)
	DIPS_HELPER( 0x0100, "N",                 KEYCODE_N)
	DIPS_HELPER( 0x0080, "M",                 KEYCODE_M)
	DIPS_HELPER( 0x0040, ", <",               KEYCODE_COMMA)
	DIPS_HELPER( 0x0020, ". >",               KEYCODE_STOP)
	DIPS_HELPER( 0x0010, "/ ?",               KEYCODE_SLASH)
	DIPS_HELPER( 0x0008, "Right-Shift",       KEYCODE_RSHIFT)
	DIPS_HELPER( 0x0004, "CRSR-DOWN UP",      KEYCODE_2_PAD)
	DIPS_HELPER( 0x0002, "CRSR-RIGHT LEFT",   KEYCODE_6_PAD)
	PORT_START 
	DIPS_HELPER( 0x8000, "Space",             KEYCODE_SPACE)
	DIPS_HELPER( 0x4000, "f1 f2",             KEYCODE_F1)
	DIPS_HELPER( 0x2000, "f3 f4",             KEYCODE_F2)
	DIPS_HELPER( 0x1000, "f5 f6",             KEYCODE_F3)
	DIPS_HELPER( 0x0800, "f7 f8",             KEYCODE_F4)
	DIPS_HELPER( 0x0400, "Special CRSR Up",       KEYCODE_8_PAD)
	DIPS_HELPER( 0x0200, "Special CRSR Left",     KEYCODE_4_PAD)
	DIPS_HELPER( 0x08, "Quickload",       KEYCODE_F8)
	DIPS_HELPER( 0x04, "Tape Drive Play",       KEYCODE_F5)
	DIPS_HELPER( 0x02, "Tape Drive Record",     KEYCODE_F6)
	DIPS_HELPER( 0x01, "Tape Drive Stop",       KEYCODE_F7)
	DIPS_BOTH
INPUT_PORTS_END


/* Initialise the vc20 palette */
static PALETTE_INIT( vc20 )
{
	palette_set_colors(machine, 0, vic6560_palette, sizeof(vic6560_palette) / 3);
/*	memcpy(sys_colortable,colortable,sizeof(colortable)); */
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
	ROM_REGION (0x10000, REGION_CPU1,0)
	ROM_LOAD ("901460.03", 0x8000, 0x1000, CRC(83e032a6) SHA1(4fd85ab6647ee2ac7ba40f729323f2472d35b9b4))
	ROM_LOAD ("901486.01", 0xc000, 0x2000, CRC(db4c43c1) SHA1(587d1e90950675ab6b12d91248a3f0d640d02e8d))
	ROM_LOAD ("901486.06", 0xe000, 0x2000, CRC(e5e7c174) SHA1(06de7ec017a5e78bd6746d89c2ecebb646efeb19))
ROM_END

ROM_START (vic1001)
	ROM_REGION (0x10000, REGION_CPU1,0)
	ROM_LOAD ("901460.02", 0x8000, 0x1000, CRC(fcfd8a4b) SHA1(dae61ac03065aa2904af5c123ce821855898c555))
	ROM_LOAD ("901486.01", 0xc000, 0x2000, CRC(db4c43c1) SHA1(587d1e90950675ab6b12d91248a3f0d640d02e8d))
	ROM_LOAD ("901486.02", 0xe000, 0x2000, CRC(336900d7) SHA1(c9ead45e6674d1042ca6199160e8583c23aeac22))
ROM_END

ROM_START (vic20swe)
	ROM_REGION (0x10000, REGION_CPU1,0)
	ROM_LOAD ("nec22101.207", 0x8000, 0x1000, CRC(d808551d) SHA1(f403f0b0ce5922bd61bbd768bdd6f0b38e648c9f))
	ROM_LOAD ("901486.01", 0xc000, 0x2000, CRC(db4c43c1) SHA1(587d1e90950675ab6b12d91248a3f0d640d02e8d))
	ROM_LOAD ("nec22081.206", 0xe000, 0x2000, CRC(b2a60662) SHA1(cb3e2f6e661ea7f567977751846ce9ad524651a3))
ROM_END

ROM_START (vic20v)
	ROM_REGION (0x10000, REGION_CPU1,0)
	ROM_LOAD ("901460.03", 0x8000, 0x1000, CRC(83e032a6) SHA1(4fd85ab6647ee2ac7ba40f729323f2472d35b9b4))
	ROM_LOAD ("901486.01", 0xc000, 0x2000, CRC(db4c43c1) SHA1(587d1e90950675ab6b12d91248a3f0d640d02e8d))
	ROM_LOAD ("901486.06", 0xe000, 0x2000, CRC(e5e7c174) SHA1(06de7ec017a5e78bd6746d89c2ecebb646efeb19))
	VC1540_ROM (REGION_CPU2)
ROM_END

ROM_START (vic20i)
	ROM_REGION (0x10000, REGION_CPU1,0)
	ROM_LOAD ("901460.03", 0x8000, 0x1000, CRC(83e032a6) SHA1(4fd85ab6647ee2ac7ba40f729323f2472d35b9b4))
	ROM_LOAD ("325329.04", 0xb000, 0x800, CRC(d37b6335) SHA1(828c965829d21c60e8c2d083caee045c639a270f))
	ROM_LOAD ("901486.01", 0xc000, 0x2000, CRC(db4c43c1) SHA1(587d1e90950675ab6b12d91248a3f0d640d02e8d))
	ROM_LOAD ("901486.06", 0xe000, 0x2000, CRC(e5e7c174) SHA1(06de7ec017a5e78bd6746d89c2ecebb646efeb19))
/*	C2031_ROM (REGION_CPU2) */
ROM_END

ROM_START (vc20)
	ROM_REGION (0x10000, REGION_CPU1,0)
	ROM_LOAD ("901460.03", 0x8000, 0x1000, CRC(83e032a6) SHA1(4fd85ab6647ee2ac7ba40f729323f2472d35b9b4))
	ROM_LOAD ("901486.01", 0xc000, 0x2000, CRC(db4c43c1) SHA1(587d1e90950675ab6b12d91248a3f0d640d02e8d))
	ROM_LOAD ("901486.07", 0xe000, 0x2000, CRC(4be07cb4) SHA1(ce0137ed69f003a299f43538fa9eee27898e621e))
ROM_END

ROM_START (vc20v)
	ROM_REGION (0x10000, REGION_CPU1,0)
	ROM_LOAD ("901460.03", 0x8000, 0x1000, CRC(83e032a6) SHA1(4fd85ab6647ee2ac7ba40f729323f2472d35b9b4))
	ROM_LOAD ("901486.01", 0xc000, 0x2000, CRC(db4c43c1) SHA1(587d1e90950675ab6b12d91248a3f0d640d02e8d))
	ROM_LOAD ("901486.07", 0xe000, 0x2000, CRC(4be07cb4) SHA1(ce0137ed69f003a299f43538fa9eee27898e621e))
	VC1541_ROM (REGION_CPU2)
ROM_END

static MACHINE_DRIVER_START( vic20 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6502, VIC6560_CLOCK)        /* 7.8336 Mhz */
	MDRV_CPU_PROGRAM_MAP(vc20_mem, 0)
	MDRV_CPU_VBLANK_INT(vc20_frame_interrupt, 1)
	MDRV_CPU_PERIODIC_INT(vic656x_raster_interrupt, TIME_IN_HZ(VIC656X_HRETRACERATE))
	MDRV_SCREEN_REFRESH_RATE(VIC6560_VRETRACERATE)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(0)

	MDRV_MACHINE_RESET( vc20 )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE((VIC6560_XSIZE + 7) & ~7, VIC6560_YSIZE)
	MDRV_SCREEN_VISIBLE_AREA(VIC6560_MAME_XPOS, VIC6560_MAME_XPOS + VIC6560_MAME_XSIZE - 1, VIC6560_MAME_YPOS, VIC6560_MAME_YPOS + VIC6560_MAME_YSIZE - 1)
	MDRV_PALETTE_LENGTH(sizeof (vic6560_palette) / sizeof (vic6560_palette[0]) / 3)
	MDRV_PALETTE_INIT( vc20 )

	MDRV_VIDEO_START( vic6560 )
	MDRV_VIDEO_UPDATE( vic6560 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(vic6560_sound_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( vic20v )
	MDRV_IMPORT_FROM( vic20 )
	MDRV_IMPORT_FROM( cpu_vc1540 )
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

	MDRV_SCREEN_REFRESH_RATE(VIC6561_VRETRACERATE)
	MDRV_SCREEN_SIZE((VIC6561_XSIZE + 7) & ~7, VIC6561_YSIZE)
	MDRV_SCREEN_VISIBLE_AREA(VIC6561_MAME_XPOS, VIC6561_MAME_XPOS + VIC6561_MAME_XSIZE - 1, VIC6561_MAME_YPOS, VIC6561_MAME_YPOS + VIC6561_MAME_YSIZE - 1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( vc20v )
	MDRV_IMPORT_FROM( vc20 )
	MDRV_IMPORT_FROM( cpu_vc1540 )
#ifdef CPU_SYNC
	MDRV_INTERLEAVE(1)
#else
	MDRV_INTERLEAVE(3000)
#endif
MACHINE_DRIVER_END

static void cbmvc20_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_INIT:							info->init = device_init_vc20_rom; break;
		case DEVINFO_PTR_LOAD:							info->load = device_load_vc20_rom; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "a0,20,40,60,rom,bin"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

static void vc20_quickload_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "p00,prg"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_QUICKLOAD_LOAD:				info->f = (genf *) quickload_load_cbm_vc20; break;

		/* --- the following bits of info are returned as doubles --- */
		case DEVINFO_FLOAT_QUICKLOAD_DELAY:				info->d = CBM_QUICKLOAD_DELAY; break;

		default:										quickload_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(vc20)
	CONFIG_DEVICE(cbmvc20_cartslot_getinfo)
	CONFIG_DEVICE(cbmfloppy_device_getinfo)
	CONFIG_DEVICE(vc20_quickload_getinfo)
	CONFIG_DEVICE(vc20tape_device_getinfo)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(vc20v)
	CONFIG_DEVICE(cbmvc20_cartslot_getinfo)
	CONFIG_DEVICE(vc20_quickload_getinfo)
	CONFIG_DEVICE(vc20tape_device_getinfo)
	CONFIG_DEVICE(vc1541_device_getinfo)
SYSTEM_CONFIG_END

#ifdef PET_TEST_CODE
SYSTEM_CONFIG_START(vc20i)
	CONFIG_DEVICE(cbmvc20_cartslot_getinfo)
	CONFIG_DEVICE(vc20_quickload_getinfo)
	CONFIG_DEVICE(vc20tape_device_getinfo)
	CONFIG_DEVICE(c2031_device_getinfo)
SYSTEM_CONFIG_END
#endif

/*		YEAR	NAME		PARENT	COMPAT	MACHINE INPUT		INIT	CONFIG     COMPANY 							FULLNAME */
COMP ( 1981,	vic20,		0,		0,		vic20,	vic20,		vic20,	vc20,      "Commodore Business Machines Co.",  "VIC20 (NTSC)", GAME_IMPERFECT_SOUND)
COMP ( 1981,	vic20i, 	vic20,	0,		vic20i, vic20i, 	vic20i, vc20,      "Commodore Business Machines Co.",  "VIC20 (NTSC), IEEE488 Interface (SYS45065)",   GAME_IMPERFECT_SOUND)
COMP ( 1981,	vic1001,	vic20,	0,		vic20,	vic1001,	vic20,	vc20,      "Commodore Business Machines Co.",  "VIC1001 (NTSC)", GAME_IMPERFECT_SOUND)
COMP ( 1981,	vc20,		vic20,	0,		vc20,	vc20,		vc20,	vc20,      "Commodore Business Machines Co.",  "VIC20/VC20(German) PAL",       GAME_IMPERFECT_SOUND)
COMP ( 1981,	vic20swe,	vic20,	0,		vc20,	vic20swe,	vc20,	vc20,      "Commodore Business Machines Co.",  "VIC20 PAL, Swedish Expansion Kit", GAME_IMPERFECT_SOUND)
// please leave the following as testdriver only
COMP ( 1981,	vic20v, 	vic20,	0,		vic20v, vic20,		vic20,	vc20v,     "Commodore Business Machines Co.",  "VIC20 (NTSC), VC1540", GAME_IMPERFECT_SOUND | GAME_NOT_WORKING )
COMP ( 1981,	vc20v,		vic20,	0,		vc20v,	vic20,		vc20,	vc20v,     "Commodore Business Machines Co.",  "VC20 (PAL), VC1541", GAME_IMPERFECT_SOUND | GAME_NOT_WORKING )
