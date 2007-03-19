/***************************************************************************
	commodore c64 home computer

	PeT mess@utanet.at

    documentation
     www.funet.fi
***************************************************************************/

/*
------------------------------------
max     commodore max (vic10/ultimax/vickie prototype)
c64		commodore c64 (ntsc version)
c64pal	commodore c64 (pal version)
c64gs   commodore c64 game system (ntsc version)
sx64    commodore sx64 (pal version)
------------------------------------
(preliminary version)

if the game runs to fast with the ntsc version, try the pal version!

c64
 design like the vic20
 better videochip with sprites
 famous sid6581 sound chip
 64 kbyte ram
 2nd gameport
Educator 64-1
 standard c64
 bios color bios (as in pet64 series) when delivered with green monitor
max  (vic10,ultimax,vickey prototype)
 delivered in japan only?
 (all modules should work with c64)
 cartridges neccessary
 low cost c64
 flat design
 only 4 kbyte sram
 simplier banking chip
  no portlines from cpu
 only 1 cia6526 chip
  restore key connection?
  no serial bus
  no userport
 keyboard
 tape port
 2 gameports
  lightpen (port a only) and joystick mentioned in advertisement
  paddles
 cartridge/expansion port (some signals different to c64)
 no rom on board (minibasic with kernel delivered as cartridge?)
c64gs
 game console without keyboard
 standard c64 mainboard!
 modified kernal
 basic rom
 2. cia yes
 no userport
 no cbm serial port
 no keyboard connector
 no tapeport
cbm4064/pet64/educator64-2
 build in green monitor
 other case
 differences, versions???
(sx100 sx64 like prototype with build in black/white monitor)
sx64
 movable compact (and heavy) all in one comp
 build in vc1541
 build in small color monitor
 no tape connector
dx64 prototype
 two build in vc1541 (or 2 drives driven by one vc1541 circuit)

state
-----
rasterline based video system
 no cpu holding
 imperfect scrolling support (when 40 columns or 25 lines)
 lightpen support not finished
 rasterline not finished
no sound
cia6526's look in machine/cia6526.c
keyboard
gameport a
 paddles 1,2
 joystick 1
 2 button joystick/mouse joystick emulation
 no mouse
 lightpen (not finished)
gameport b
 paddles 3,4
 joystick 2
 2 button joystick/mouse joystick emulation
 no mouse
simple tape support
 (not working, cia timing?)
serial bus
 simple disk drives
 no printer or other devices
expansion modules c64
 rom cartridges (exrom)
 ultimax rom cartridges (game)
 no other rom cartridges (bankswitching logic in it, switching exrom, game)
 no ieee488 support
 no cpm cartridge
 no speech cartridge (no circuit diagram found)
 no fm sound cartridge
 no other expansion modules
expansion modules ultimax
 ultimax rom cartridges
 no other expansion modules
no userport
 no rs232/v.24 interface
no super cpu modification
no second sid modification
quickloader

Keys
----
Some PC-Keyboards does not behave well when special two or more keys are
pressed at the same time
(with my keyboard printscreen clears the pressed pause key!)

shift-cbm switches between upper-only and normal character set
(when wrong characters on screen this can help)
run (shift-stop) loads pogram from type and starts it

Lightpen
--------
Paddle 5 x-axe
Paddle 6 y-axe

Tape
----
(DAC 1 volume in noise volume)
loading of wav, prg and prg files in zip archiv
commandline -cassette image
wav:
 8 or 16(not tested) bit, mono, 125000 Hz minimum
 has the same problems like an original tape drive (tone head must
 be adjusted to get working(no load error,...) wav-files)
zip:
 must be placed in current directory
 prg's are played in the order of the files in zip file

use LOAD or LOAD"" or LOAD"",1 for loading of normal programs
use LOAD"",1,1 for loading programs to their special address

several programs relies on more features
(loading other file types, writing, ...)

Discs
-----
only file load from drive 8 and 9 implemented
 loads file from rom directory (*.prg,*.p00) (must NOT be specified on commandline)
 or file from d64 image (here also directory LOAD"$",8 supported)
use LOAD"filename",8
or LOAD"filename",8,1 (for loading machine language programs at their address)
for loading
type RUN or the appropriate sys call to start them

several programs rely on more features
(loading other file types, writing, ...)

most games rely on starting own programs in the floppy drive
(and therefor cpu level emulation is needed)

Roms
----
.prg
.crt
.80 .90 .a0 .b0 .e0 .f0
files with boot-sign in it
  recogniced as roms

.prg files loaded at address in its first two bytes
.?0 files to address specified in extension
.crt roms to addresses in crt file

Quickloader
-----------
.prg and .p00 files supported
loads program into memory and sets program end pointer
(works with most programs)
program ready to get started with RUN
loads first rom when you press quickload key (f8)

when problems start with -log and look into error.log file
 */

#include "driver.h"
#include "inputx.h"
#include "sound/sid6581.h"
#include "machine/6526cia.h"

#define VERBOSE_DBG 0
#include "includes/cbm.h"
#include "includes/vic6567.h"
#include "includes/cbmserb.h"
#include "includes/vc1541.h"
#include "includes/vc20tape.h"

#include "includes/c64.h"

static ADDRESS_MAP_START(ultimax_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x0fff) AM_RAM AM_BASE(&c64_memory)
	AM_RANGE(0x8000, 0x9fff) AM_ROM AM_BASE(&c64_roml)
	AM_RANGE(0xd000, 0xd3ff) AM_READWRITE(vic2_port_r, vic2_port_w)
	AM_RANGE(0xd400, 0xd7ff) AM_READWRITE(sid6581_0_port_r, sid6581_0_port_w)
	AM_RANGE(0xd800, 0xdbff) AM_READWRITE(MRA8_RAM, c64_colorram_write) AM_BASE(&c64_colorram) /* colorram  */
	AM_RANGE(0xdc00, 0xdcff) AM_READWRITE(cia_0_r, cia_0_w)
	AM_RANGE(0xe000, 0xffff) AM_ROM AM_BASE( &c64_romh)	   /* ram or kernel rom */
ADDRESS_MAP_END

static ADDRESS_MAP_START(c64_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x7fff) AM_RAM AM_BASE(&c64_memory)
	AM_RANGE(0x8000, 0x9fff) AM_READWRITE(MRA8_BANK1, MWA8_BANK2)	   /* ram or external roml */
	AM_RANGE(0xa000, 0xbfff) AM_READWRITE(MRA8_BANK3, MWA8_RAM)	   /* ram or basic rom or external romh */
	AM_RANGE(0xc000, 0xcfff) AM_RAM
#if 1
	AM_RANGE(0xd000, 0xdfff) AM_READWRITE(MRA8_BANK5, MWA8_BANK6)
#else
/* dram */
/* or character rom */
	AM_RANGE(0xd000, 0xd3ff) AM_READWRITE(MRA8_BANK9, vic2_port_w)
	AM_RANGE(0xd400, 0xd7ff) AM_READWRITE(MRA8_BANK10, sid6581_0_port_w)
	AM_RANGE(0xd800, 0xdbff) AM_READWRITE(MRA8_BANK11, c64_colorram_write)		   /* colorram  */
	AM_RANGE(0xdc00, 0xdcff) AM_READWRITE(MRA8_BANK12, cia_0_w)
	AM_RANGE(0xdd00, 0xddff) AM_READWRITE(MRA8_BANK13, cia_1_w)
	AM_RANGE(0xde00, 0xdeff) AM_READ(MRA8_BANK14)		   /* csline expansion port */
	AM_RANGE(0xdf00, 0xdfff) AM_READ(MRA8_BANK15)		   /* csline expansion port */
#endif
	AM_RANGE(0xe000, 0xffff) AM_READWRITE(MRA8_BANK7, MWA8_BANK8)	   /* ram or kernel rom or external romh */
ADDRESS_MAP_END

#define DIPS_HELPER(bit, name, keycode) \
   PORT_BIT(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(name) PORT_CODE(keycode)

INPUT_PORTS_START(c64_keyboard)
	PORT_START																										\
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Arrow-Left") PORT_CODE(KEYCODE_TILDE)
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1 !   BLK   ORNG") PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2 \"   WHT   BRN") PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3 #   RED   L RED") PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4 $   CYN   D GREY") PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5 %   PUR   GREY") PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6 &   GRN   L GRN") PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7 '   BLU   L BLU") PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8 (   YEL   L GREY") PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9 )   RVS-ON") PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0     RVS-OFF") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("+") PORT_CODE(KEYCODE_PLUS_PAD) PORT_CHAR('+')
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS_PAD) PORT_CHAR('-')
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Pound") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('\xA3')
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("HOME CLR") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("DEL INST") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8) PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	PORT_START
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_RCONTROL) PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("At") PORT_CODE(KEYCODE_OPENBRACE)
    PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("*") PORT_CODE(KEYCODE_ASTERISK) PORT_CHAR('*')
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Arrow-Up Pi") PORT_CODE(KEYCODE_CLOSEBRACE)
    PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RESTORE") PORT_CODE(KEYCODE_PRTSCR)
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("STOP RUN") PORT_CODE(KEYCODE_TAB)
	PORT_START																			
	PORT_DIPNAME( 0x8000, IP_ACTIVE_HIGH,												 "SHIFT-LOCK (switch)") PORT_CODE(KEYCODE_CAPSLOCK)
	PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )																		
	PORT_DIPSETTING( 0x8000, DEF_STR( On ) )																		
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(": [") PORT_CODE(KEYCODE_COLON) PORT_CHAR(':') PORT_CHAR('[')
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("; ]") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('I') PORT_CHAR('i')
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("=") PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('=')
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR('\r')
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CBM") PORT_CODE(KEYCODE_RALT)
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Left-Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_START
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Right-Shift") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CRSR-DOWN UP") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CRSR-RIGHT LEFT") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F1 F2") PORT_CODE(KEYCODE_F1) PORT_CHAR(UCHAR_MAMEKEY(F1)) PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F3 F4") PORT_CODE(KEYCODE_F2) PORT_CHAR(UCHAR_MAMEKEY(F3)) PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_START
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F5 F6") PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F5)) PORT_CHAR(UCHAR_MAMEKEY(F6))
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F7 F8") PORT_CODE(KEYCODE_F4) PORT_CHAR(UCHAR_MAMEKEY(F7)) PORT_CHAR(UCHAR_MAMEKEY(F8))
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(Right-Shift Cursor-Down)Special CRSR Up") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(Right-Shift Cursor-Right)Special CRSR Left") PORT_CODE(KEYCODE_4_PAD)
INPUT_PORTS_END

INPUT_PORTS_START(vic64s_keyboard)
	PORT_START
	DIPS_HELPER( 0x8000, "Arrow-Left", KEYCODE_TILDE)
	DIPS_HELPER( 0x4000, "1 !   BLK   ORNG", KEYCODE_1)
	DIPS_HELPER( 0x2000, "2 \"   WHT   BRN", KEYCODE_2)
	DIPS_HELPER( 0x1000, "3 #   RED   L RED", KEYCODE_3)
	DIPS_HELPER( 0x0800, "4 $   CYN   D GREY", KEYCODE_4)
	DIPS_HELPER( 0x0400, "5 %   PUR   GREY", KEYCODE_5)
	DIPS_HELPER( 0x0200, "6 &   GRN   L GRN", KEYCODE_6)
	DIPS_HELPER( 0x0100, "7 '   BLU   L BLU", KEYCODE_7)
	DIPS_HELPER( 0x0080, "8 (   YEL   L GREY", KEYCODE_8)
	DIPS_HELPER( 0x0040, "9 )   RVS-ON", KEYCODE_9)
	DIPS_HELPER( 0x0020, "0     RVS-OFF", KEYCODE_0)
	DIPS_HELPER( 0x0010, "-", KEYCODE_PLUS_PAD)
	DIPS_HELPER( 0x0008, "=", KEYCODE_MINUS_PAD)
	DIPS_HELPER( 0x0004, ": *", KEYCODE_MINUS)
	DIPS_HELPER( 0x0002, "HOME CLR", KEYCODE_EQUALS)
	DIPS_HELPER( 0x0001, "DEL INST", KEYCODE_BACKSPACE)
	PORT_START
	DIPS_HELPER( 0x8000, "CTRL", KEYCODE_RCONTROL)
	DIPS_HELPER( 0x4000, "Q", KEYCODE_Q)
	DIPS_HELPER( 0x2000, "W", KEYCODE_W)
	DIPS_HELPER( 0x1000, "E", KEYCODE_E)
	DIPS_HELPER( 0x0800, "R", KEYCODE_R)
	DIPS_HELPER( 0x0400, "T", KEYCODE_T)
	DIPS_HELPER( 0x0200, "Y", KEYCODE_Y)
	DIPS_HELPER( 0x0100, "U", KEYCODE_U)
	DIPS_HELPER( 0x0080, "I", KEYCODE_I)
	DIPS_HELPER( 0x0040, "O", KEYCODE_O)
	DIPS_HELPER( 0x0020, "P", KEYCODE_P)
	DIPS_HELPER( 0x0010, "Overcircle-A", KEYCODE_OPENBRACE)
    DIPS_HELPER( 0x0008, "At", KEYCODE_ASTERISK)
	DIPS_HELPER( 0x0004, "Arrow-Up Pi",KEYCODE_CLOSEBRACE)
    DIPS_HELPER( 0x0002, "RESTORE", KEYCODE_PRTSCR)
	DIPS_HELPER( 0x0001, "STOP RUN", KEYCODE_TAB)
	PORT_START 
	PORT_DIPNAME( 0x8000, IP_ACTIVE_HIGH, "SHIFT-LOCK (switch)") PORT_CODE(KEYCODE_CAPSLOCK)
	PORT_DIPSETTING(  0, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x8000, DEF_STR( On ) )
	DIPS_HELPER( 0x4000, "A", KEYCODE_A)
	DIPS_HELPER( 0x2000, "S", KEYCODE_S)
	DIPS_HELPER( 0x1000, "D", KEYCODE_D)
	DIPS_HELPER( 0x0800, "F", KEYCODE_F)
	DIPS_HELPER( 0x0400, "G", KEYCODE_G)
	DIPS_HELPER( 0x0200, "H", KEYCODE_H)
	DIPS_HELPER( 0x0100, "J", KEYCODE_J)
	DIPS_HELPER( 0x0080, "K", KEYCODE_K)
	DIPS_HELPER( 0x0040, "L", KEYCODE_L)
	DIPS_HELPER( 0x0020, "Diaresis-O", KEYCODE_COLON)
	DIPS_HELPER( 0x0010, "Diaresis-A", KEYCODE_QUOTE)
	DIPS_HELPER( 0x0008, "; +", KEYCODE_BACKSLASH)
	DIPS_HELPER( 0x0004, "RETURN",KEYCODE_ENTER)
	DIPS_HELPER( 0x0002, "CBM", KEYCODE_RALT)
	DIPS_HELPER( 0x0001, "Left-Shift", KEYCODE_LSHIFT)
	PORT_START
	DIPS_HELPER( 0x8000, "Z", KEYCODE_Z)
	DIPS_HELPER( 0x4000, "X", KEYCODE_X)
	DIPS_HELPER( 0x2000, "C", KEYCODE_C)
	DIPS_HELPER( 0x1000, "V", KEYCODE_V)
	DIPS_HELPER( 0x0800, "B", KEYCODE_B)
	DIPS_HELPER( 0x0400, "N", KEYCODE_N)
	DIPS_HELPER( 0x0200, "M", KEYCODE_M)
	DIPS_HELPER( 0x0100, ", <", KEYCODE_COMMA)
	DIPS_HELPER( 0x0080, ". >", KEYCODE_STOP)
	DIPS_HELPER( 0x0040, "/ ?", KEYCODE_SLASH)
	DIPS_HELPER( 0x0020, "Right-Shift", KEYCODE_RSHIFT)
	DIPS_HELPER( 0x0010, "CRSR-DOWN UP", KEYCODE_2_PAD)
	DIPS_HELPER( 0x0008, "CRSR-RIGHT LEFT", KEYCODE_6_PAD)
	DIPS_HELPER( 0x0004, "Space", KEYCODE_SPACE)
	DIPS_HELPER( 0x0002, "f1 f2", KEYCODE_F1)
	DIPS_HELPER( 0x0001, "f3 f4", KEYCODE_F2)
	PORT_START
	DIPS_HELPER( 0x8000, "f5 f6", KEYCODE_F3)
	DIPS_HELPER( 0x4000, "f7 f8", KEYCODE_F4)
	DIPS_HELPER( 0x2000, "(Right-Shift Cursor-Down)Special CRSR Up", 
				 KEYCODE_8_PAD)
	DIPS_HELPER( 0x1000, "(Right-Shift Cursor-Right)Special CRSR Left",
				 KEYCODE_4_PAD)
INPUT_PORTS_END

INPUT_PORTS_START (ultimax)
	 C64_DIPS
	PORT_START
	DIPS_HELPER( 0x8000, "Quickload", KEYCODE_F8)
	PORT_DIPNAME   ( 0x4000, 0x4000, "Tape Drive/Device 1")
	PORT_DIPSETTING(  0, DEF_STR( Off ) )
	PORT_DIPSETTING(0x4000, DEF_STR( On ) )
	PORT_DIPNAME   ( 0x2000, 0x00, " Tape Sound")
	PORT_DIPSETTING(  0, DEF_STR( Off ) )
	PORT_DIPSETTING(0x2000, DEF_STR( On ) )
	DIPS_HELPER( 0x1000, "Tape Drive Play",       KEYCODE_F5)
	DIPS_HELPER( 0x0800, "Tape Drive Record",     KEYCODE_F6)
	DIPS_HELPER( 0x0400, "Tape Drive Stop",       KEYCODE_F7)
	PORT_DIPNAME   ( 0x80, 0x00, "Sid Chip Type")
	PORT_DIPSETTING(  0, "MOS6581" )
	PORT_DIPSETTING(0x80, "MOS8580" )
	 PORT_BIT (0x1c, 0x4, IPT_UNUSED)	   /* only ultimax cartridges */
	 PORT_BIT (0x2, 0x0, IPT_UNUSED)		   /* no serial bus */
	 PORT_BIT (0x1, 0x0, IPT_UNUSED)
	PORT_INCLUDE(c64_keyboard)
INPUT_PORTS_END

INPUT_PORTS_START (c64gs)
	 C64_DIPS
	 PORT_START
	 PORT_BIT (0xff00, 0x0, IPT_UNUSED)
	PORT_DIPNAME   ( 0x80, 0x00, "Sid Chip Type")
	PORT_DIPSETTING(  0, "MOS6581" )
	PORT_DIPSETTING(0x80, "MOS8580" )
	 PORT_DIPNAME (0x1c, 0x00, "Cartridge Type")
	 PORT_DIPSETTING (0, "Automatic")
	 PORT_DIPSETTING (4, "Ultimax (GAME)")
	 PORT_DIPSETTING (8, "C64 (EXROM)")
#ifdef PET_TEST_CODE
	 PORT_DIPSETTING (0x10, "CBM Supergames")
	 PORT_DIPSETTING (0x14, "Ocean Robocop2")
#endif
	 PORT_BIT (0x2, 0x0, IPT_UNUSED)		   /* no serial bus */
	 PORT_BIT (0x1, 0x0, IPT_UNUSED)
	 PORT_START /* no keyboard */
	 PORT_BIT (0xffff, 0x0, IPT_UNUSED)
	 PORT_START
	 PORT_BIT (0xffff, 0x0, IPT_UNUSED)
	 PORT_START
	 PORT_BIT (0xffff, 0x0, IPT_UNUSED)
	 PORT_START
	 PORT_BIT (0xffff, 0x0, IPT_UNUSED)
	 PORT_START
	 PORT_BIT (0xf000, 0x0, IPT_UNUSED)
INPUT_PORTS_END

INPUT_PORTS_START (c64)
	 C64_DIPS
	 PORT_START
	 DIPS_HELPER( 0x8000, "Quickload", KEYCODE_F8)
	 PORT_DIPNAME   ( 0x4000, 0x4000, "Tape Drive/Device 1")
	 PORT_DIPSETTING(  0, DEF_STR( Off ) )
	 PORT_DIPSETTING(0x4000, DEF_STR( On ) )
	 PORT_DIPNAME   ( 0x2000, 0x00, " Tape Sound")
	 PORT_DIPSETTING(  0, DEF_STR( Off ) )
	 PORT_DIPSETTING(0x2000, DEF_STR( On ) )
	 DIPS_HELPER( 0x1000, "Tape Drive Play",       KEYCODE_F5)
	 DIPS_HELPER( 0x0800, "Tape Drive Record",     KEYCODE_F6)
	 DIPS_HELPER( 0x0400, "Tape Drive Stop",       KEYCODE_F7)
	PORT_DIPNAME   ( 0x80, 0x00, "Sid Chip Type")
	PORT_DIPSETTING(  0, "MOS6581" )
	PORT_DIPSETTING(0x80, "MOS8580" )
	 PORT_DIPNAME (0x1c, 0x00, "Cartridge Type")
	 PORT_DIPSETTING (0, "Automatic")
	 PORT_DIPSETTING (4, "Ultimax (GAME)")
	 PORT_DIPSETTING (8, "C64 (EXROM)")
#ifdef PET_TEST_CODE
	 PORT_DIPSETTING (0x10, "CBM Supergames")
	 PORT_DIPSETTING (0x14, "Ocean Robocop2")
#endif
	PORT_INCLUDE(c64_keyboard)
INPUT_PORTS_END

INPUT_PORTS_START (vic64s)
	 C64_DIPS
	 PORT_START
	 DIPS_HELPER( 0x8000, "Quickload", KEYCODE_F8)
	 PORT_DIPNAME   ( 0x4000, 0x4000, "Tape Drive/Device 1")
	 PORT_DIPSETTING(  0, DEF_STR( Off ) )
	 PORT_DIPSETTING(0x4000, DEF_STR( On ) )
	 PORT_DIPNAME   ( 0x2000, 0x00, " Tape Sound")
	 PORT_DIPSETTING(  0, DEF_STR( Off ) )
	 PORT_DIPSETTING(0x2000, DEF_STR( On ) )
	 DIPS_HELPER( 0x1000, "Tape Drive Play",       KEYCODE_F5)
	 DIPS_HELPER( 0x0800, "Tape Drive Record",     KEYCODE_F6)
	 DIPS_HELPER( 0x0400, "Tape Drive Stop",       KEYCODE_F7)
	PORT_DIPNAME   ( 0x80, 0x00, "Sid Chip Type")
	PORT_DIPSETTING(  0, "MOS6581" )
	PORT_DIPSETTING(0x80, "MOS8580" )
	 PORT_DIPNAME (0x1c, 0x00, "Cartridge Type")
	 PORT_DIPSETTING (0, "Automatic")
	 PORT_DIPSETTING (4, "Ultimax (GAME)")
	 PORT_DIPSETTING (8, "C64 (EXROM)")
#ifdef PET_TEST_CODE
	 PORT_DIPSETTING (0x10, "CBM Supergames")
	 PORT_DIPSETTING (0x14, "Ocean Robocop2")
#endif
	 PORT_INCLUDE(vic64s_keyboard)
INPUT_PORTS_END

INPUT_PORTS_START (sx64)
	C64_DIPS
	PORT_START
	DIPS_HELPER( 0x8000, "Quickload", KEYCODE_F8)
	PORT_BIT (0x7f00, 0x0, IPT_UNUSED) /* no tape */
	PORT_DIPNAME   ( 0x80, 0x00, "Sid Chip Type")
	PORT_DIPSETTING(  0, "MOS6581" )
	PORT_DIPSETTING(0x80, "MOS8580" )
	PORT_DIPNAME (0x1c, 0x00, "Cartridge Type")
	PORT_DIPSETTING (0, "Automatic")
	PORT_DIPSETTING (4, "Ultimax (GAME)")
	PORT_DIPSETTING (8, "C64 (EXROM)")
#ifdef PET_TEST_CODE
	PORT_DIPSETTING (0x10, "CBM Supergames")
	PORT_DIPSETTING (0x14, "Ocean Robocop2")
#endif
	/* 1 vc1541 build in, device number selectable 8,9,10,11 */
	PORT_INCLUDE(c64_keyboard)
INPUT_PORTS_END

INPUT_PORTS_START (vip64)
	 C64_DIPS
	 PORT_START
	 DIPS_HELPER( 0x8000, "Quickload", KEYCODE_F8)
	 PORT_BIT (0x7f00, 0x0, IPT_UNUSED) /* no tape */
	PORT_DIPNAME   ( 0x80, 0x00, "Sid Chip Type")
	PORT_DIPSETTING(  0, "MOS6581" )
	PORT_DIPSETTING(0x80, "MOS8580" )
	 PORT_DIPNAME (0x1c, 0x00, "Cartridge Type")
	 PORT_DIPSETTING (0, "Automatic")
	 PORT_DIPSETTING (4, "Ultimax (GAME)")
	 PORT_DIPSETTING (8, "C64 (EXROM)")
#ifdef PET_TEST_CODE
	 PORT_DIPSETTING (0x10, "CBM Supergames")
	 PORT_DIPSETTING (0x14, "Ocean Robocop2")
#endif
	 /* 1 vc1541 build in, device number selectable 8,9,10,11 */
	 PORT_INCLUDE(vic64s_keyboard)
INPUT_PORTS_END

static PALETTE_INIT( pet64 )
{
	int i;
	for (i=0; i<16; i++)
		palette_set_color(machine, i, 0, vic2_palette[i*3+1], 0);
}

ROM_START (ultimax)
	ROM_REGION (0x10000, REGION_CPU1, 0)
ROM_END

ROM_START (c64gs)
	ROM_REGION (0x19400, REGION_CPU1, 0)
	/* standard basic, modified kernel */
	ROM_LOAD ("390852.01", 0x10000, 0x4000, CRC(b0a9c2da) SHA1(21940ef5f1bfe67d7537164f7ca130a1095b067a))
	ROM_LOAD ("901225.01", 0x14000, 0x1000, CRC(ec4272ee) SHA1(adc7c31e18c7c7413d54802ef2f4193da14711aa))
ROM_END

ROM_START (c64)
	ROM_REGION (0x19400, REGION_CPU1, 0)
	ROM_LOAD ("901226.01", 0x10000, 0x2000, CRC(f833d117) SHA1(79015323128650c742a3694c9429aa91f355905e))
	ROM_LOAD ("901227.03", 0x12000, 0x2000, CRC(dbe3e7c7) SHA1(1d503e56df85a62fee696e7618dc5b4e781df1bb))
	ROM_LOAD ("901225.01", 0x14000, 0x1000, CRC(ec4272ee) SHA1(adc7c31e18c7c7413d54802ef2f4193da14711aa))
ROM_END

ROM_START (c64pal)
	ROM_REGION (0x19400, REGION_CPU1, 0)
	ROM_LOAD ("901226.01", 0x10000, 0x2000, CRC(f833d117) SHA1(79015323128650c742a3694c9429aa91f355905e))
	ROM_LOAD ("901227.03", 0x12000, 0x2000, CRC(dbe3e7c7) SHA1(1d503e56df85a62fee696e7618dc5b4e781df1bb))
	ROM_LOAD ("901225.01", 0x14000, 0x1000, CRC(ec4272ee) SHA1(adc7c31e18c7c7413d54802ef2f4193da14711aa))
ROM_END

ROM_START (vic64s)
	ROM_REGION (0x19400, REGION_CPU1, 0)
	ROM_LOAD ("901226.01",	0x10000, 0x2000, CRC(f833d117) SHA1(79015323128650c742a3694c9429aa91f355905e))
	ROM_LOAD ("kernel.swe",	0x12000, 0x2000, CRC(f10c2c25) SHA1(e4f52d9b36c030eb94524eb49f6f0774c1d02e5e))
	ROM_LOAD ("charswe.bin",0x14000, 0x1000, CRC(bee9b3fd) SHA1(446ae58f7110d74d434301491209299f66798d8a))
ROM_END

ROM_START (sx64)
	ROM_REGION (0x19400, REGION_CPU1, 0)
	ROM_LOAD ("901226.01", 0x10000, 0x2000, CRC(f833d117) SHA1(79015323128650c742a3694c9429aa91f355905e))
	ROM_LOAD( "251104.04", 0x12000, 0x2000, CRC(2c5965d4) SHA1(aa136e91ecf3c5ac64f696b3dbcbfc5ba0871c98))
	ROM_LOAD ("901225.01", 0x14000, 0x1000, CRC(ec4272ee) SHA1(adc7c31e18c7c7413d54802ef2f4193da14711aa))
	VC1541_ROM (REGION_CPU2)
ROM_END

ROM_START (dx64)
	ROM_REGION (0x19400, REGION_CPU1, 0)
    ROM_LOAD ("901226.01", 0x10000, 0x2000, CRC(f833d117) SHA1(79015323128650c742a3694c9429aa91f355905e))
    ROM_LOAD( "dx64kern.bin",     0x12000, 0x2000, CRC(58065128))
    // vc1541 roms were not included in submission
    VC1541_ROM (REGION_CPU2)
//    VC1541_ROM (REGION_CPU3)
ROM_END

ROM_START (vip64)
	ROM_REGION (0x19400, REGION_CPU1, 0)
	ROM_LOAD ("901226.01", 0x10000, 0x2000, CRC(f833d117) SHA1(79015323128650c742a3694c9429aa91f355905e))
	ROM_LOAD( "kernelsx.swe",   0x12000, 0x2000, CRC(7858d3d7))
	ROM_LOAD ("charswe.bin", 0x14000, 0x1000, CRC(bee9b3fd) SHA1(446ae58f7110d74d434301491209299f66798d8a))
	VC1541_ROM (REGION_CPU2)
ROM_END

ROM_START (pet64)
	ROM_REGION (0x19400, REGION_CPU1, 0)
	ROM_LOAD ("901226.01", 0x10000, 0x2000, CRC(f833d117) SHA1(79015323128650c742a3694c9429aa91f355905e))
	ROM_LOAD( "901246.01", 0x12000, 0x2000, CRC(789c8cc5) SHA1(6c4fa9465f6091b174df27dfe679499df447503c))
	ROM_LOAD ("901225.01", 0x14000, 0x1000, CRC(ec4272ee) SHA1(adc7c31e18c7c7413d54802ef2f4193da14711aa))
ROM_END

#if 0
ROM_START (flash8)
	ROM_REGION (0x1009400, REGION_CPU1, 0)
#if 1
    ROM_LOAD ("flash8", 0x010000, 0x002000, CRC(3c4fb703)) // basic
    ROM_CONTINUE( 0x014000, 0x001000) // empty
    ROM_CONTINUE( 0x014000, 0x001000) // characterset
    ROM_CONTINUE( 0x012000, 0x002000) // c64 mode kernel
    ROM_CONTINUE( 0x015000, 0x002000) // kernel
#else
	ROM_LOAD ("flash8", 0x012000-0x6000, 0x008000, CRC(3c4fb703))
#endif
ROM_END
#endif

#if 0
     /* character rom */
	 ROM_LOAD ("901225.01", 0x14000, 0x1000, CRC(ec4272ee) SHA1(adc7c31e18c7c7413d54802ef2f4193da14711aa))
	 ROM_LOAD ("charswe.bin", 0x14000, 0x1000, CRC(bee9b3fd) SHA1(446ae58f7110d74d434301491209299f66798d8a))

	/* basic */
	 ROM_LOAD ("901226.01", 0x10000, 0x2000, CRC(f833d117) SHA1(79015323128650c742a3694c9429aa91f355905e))

/* in c16 and some other commodore machines:
   cbm version in kernel at 0xff80 (offset 0x3f80)
   0x80 means pal version */

	 /* scrap */
     /* modified for alec 64, not booting */
	 ROM_LOAD( "alec64.e0",   0x12000, 0x2000, CRC(2b1b7381 ))
     /* unique copyright, else speeddos? */
	 ROM_LOAD( "a.e0", 0x12000, 0x2000, CRC(b8f49365 ))
	 /* ? */
	 ROM_LOAD( "kernelx.e0",  0x12000, 0x2000, CRC(beed6d49 ))
	 ROM_LOAD( "kernelx2.e0",  0x12000, 0x2000, CRC(cfb58230 ))
	 /* basic x 2 */
	 ROM_LOAD( "frodo.e0",    0x12000, 0x2000, CRC(6ec94629 ))

     /* commodore versions */
	 /* 901227-01 */
	 ROM_LOAD( "901227.01",  0x12000, 0x2000, CRC(dce782fa ))
     /* 901227-02 */
	 ROM_LOAD( "901227.02", 0x12000, 0x2000, CRC(a5c687b3 ))
     /* 901227-03 */
	 ROM_LOAD( "901227.03",   0x12000, 0x2000, CRC(dbe3e7c7) SHA1(1d503e56df85a62fee696e7618dc5b4e781df1bb))
	 /* 901227-03? swedish  */
	 ROM_LOAD( "kernel.swe",   0x12000, 0x2000, CRC(f10c2c25) SHA1(e4f52d9b36c030eb94524eb49f6f0774c1d02e5e))
	 /* c64c 901225-01 + 901227-03 */
	 ROM_LOAD ("251913.01", 0x10000, 0x4000, CRC(0010ec31))
     /* c64gs 901225-01 with other fillbyte, modified kernel */
	 ROM_LOAD ("390852.01", 0x10000, 0x4000, CRC(b0a9c2da) SHA1(21940ef5f1bfe67d7537164f7ca130a1095b067a))
	 /* sx64 */
	 ROM_LOAD( "251104.04",     0x12000, 0x2000, CRC(2c5965d4 ))
     /* 251104.04? swedish */
	 ROM_LOAD( "kernel.swe",   0x12000, 0x2000, CRC(7858d3d7 ))
	 /* 4064, Pet64, Educator 64 */
	 ROM_LOAD( "901246.01",     0x12000, 0x2000, CRC(789c8cc5 ))

	 /* few differences to above versions */
	 ROM_LOAD( "901227.02b",  0x12000, 0x2000, CRC(f80eb87b ))
	 ROM_LOAD( "901227.03b",  0x12000, 0x2000, CRC(8e5c500d ))
	 ROM_LOAD( "901227.03c",  0x12000, 0x2000, CRC(c13310c2 ))

     /* 64er system v1
        ieee interface extension for c64 and vc1541!? */
     ROM_LOAD( "64ersys1.e0", 0x12000, 0x2000, CRC(97d9a4df ))
	 /* 64er system v3 */
	 ROM_LOAD( "64ersys3.e0", 0x12000, 0x2000, CRC(5096b3bd ))

	 /* exos v3 */
	 ROM_LOAD( "exosv3.e0",   0x12000, 0x2000, CRC(4e54d020 ))
     /* 2 bytes different */
	 ROM_LOAD( "exosv3.e0",   0x12000, 0x2000, CRC(26f3339e ))

	 /* jiffydos v6.01 by cmd */
	 ROM_LOAD( "jiffy.e0",    0x12000, 0x2000, CRC(2f79984c ))

	 /* dolphin with dolphin vc1541 */
	 ROM_LOAD( "mager.e0",    0x12000, 0x2000, CRC(c9bb21bc ))
	 ROM_LOAD( "dos20.e0",    0x12000, 0x2000, CRC(ffaeb9bc ))

	 /* speeddos plus
		parallel interface on userport to modified vc1541 !? */
	 ROM_LOAD( "speeddos.e0", 0x12000, 0x2000, CRC(8438e77b ))
	 /* speeddos plus + */
	 ROM_LOAD( "speeddos.e0", 0x12000, 0x2000, CRC(10aee0ae ))
	 /* speeddos plus and 80 column text */
	 ROM_LOAD( "rom80.e0",    0x12000, 0x2000, CRC(e801dadc ))
#endif

static SID6581_interface c64_sound_interface =
{
	c64_paddle_read
};

static MACHINE_DRIVER_START( c64 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6510, VIC6567_CLOCK)
	MDRV_CPU_PROGRAM_MAP(c64_mem, 0)
	MDRV_CPU_VBLANK_INT(c64_frame_interrupt, 1)
	MDRV_CPU_PERIODIC_INT(vic2_raster_irq, TIME_IN_HZ(VIC2_HRETRACERATE))
	MDRV_SCREEN_REFRESH_RATE(VIC6567_VRETRACERATE)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_START( c64 )

	/* video hardware */
	MDRV_IMPORT_FROM( vh_vic2 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD_TAG("sid", SID6581, VIC6567_CLOCK)
	MDRV_SOUND_CONFIG(c64_sound_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_SOUND_ADD_TAG("dac", DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ultimax )
	MDRV_IMPORT_FROM( c64 )
	MDRV_CPU_REPLACE( "main", M6510, 1000000)
	MDRV_CPU_PROGRAM_MAP( ultimax_mem, 0 )

	MDRV_SOUND_REPLACE("sid", SID6581, 1000000)
	MDRV_SOUND_CONFIG(c64_sound_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pet64 )
	MDRV_IMPORT_FROM( c64 )
	MDRV_PALETTE_INIT( pet64 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( c64pal )
	MDRV_IMPORT_FROM( c64 )
	MDRV_CPU_REPLACE( "main", M6510, VIC6569_CLOCK)
	MDRV_SCREEN_REFRESH_RATE(VIC6569_VRETRACERATE)

	MDRV_SOUND_REPLACE("sid", SID6581, VIC6569_CLOCK)
	MDRV_SOUND_CONFIG(c64_sound_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( c64gs )
	MDRV_IMPORT_FROM( c64pal )
	MDRV_SOUND_REMOVE( "dac" )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( sx64 )
	MDRV_IMPORT_FROM( c64pal )
	MDRV_IMPORT_FROM( cpu_vc1541 )
	MDRV_SOUND_REMOVE( "dac" )
#ifdef CPU_SYNC
	MDRV_INTERLEAVE(1)
#else
	MDRV_INTERLEAVE(3000)
#endif
MACHINE_DRIVER_END

#define rom_max rom_ultimax
#define rom_cbm4064 rom_pet64

static void c64_cbmcartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "crt,80"); break;

		default:										cbmcartslot_device_getinfo(devclass, state, info); break;
	}
}

static void c64_quickload_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "p00,prg"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_QUICKLOAD_LOAD:				info->f = (genf *) quickload_load_cbm_c64; break;

		/* --- the following bits of info are returned as doubles --- */
		case DEVINFO_FLOAT_QUICKLOAD_DELAY:				info->d = CBM_QUICKLOAD_DELAY; break;

		default:										quickload_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(c64)
	CONFIG_DEVICE(c64_cbmcartslot_getinfo)
	CONFIG_DEVICE(cbmfloppy_device_getinfo)
	CONFIG_DEVICE(c64_quickload_getinfo)
	CONFIG_DEVICE(vc20tape_device_getinfo)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(sx64)
	CONFIG_DEVICE(c64_cbmcartslot_getinfo)
	CONFIG_DEVICE(c64_quickload_getinfo)
	CONFIG_DEVICE(vc1541_device_getinfo)
SYSTEM_CONFIG_END

static void ultimax_cbmcartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_MUST_BE_LOADED:				info->i = 1; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "crt,e0,f0"); break;

		default:										cbmcartslot_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(ultimax)
	CONFIG_DEVICE(ultimax_cbmcartslot_getinfo)
	CONFIG_DEVICE(c64_quickload_getinfo)
	CONFIG_DEVICE(vc20tape_device_getinfo)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(c64gs)
	CONFIG_DEVICE(c64_cbmcartslot_getinfo)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*   YEAR  NAME		PARENT	COMPAT	MACHINE 		INPUT	INIT	CONFIG		COMPANY 						   FULLNAME */
COMP(1982, max,		0,		0,		ultimax,		ultimax,ultimax,ultimax,	"Commodore Business Machines Co.", "Commodore Max (Ultimax/VC10)", 0)
COMP(1982, c64,		0,		0,		c64,			c64,	c64,	c64,		"Commodore Business Machines Co.", "Commodore 64 (NTSC)", 0)
COMP(1982, cbm4064,	c64,	0,		pet64,			c64,	c64,	c64,		"Commodore Business Machines Co.", "CBM4064/PET64/Educator64 (NTSC)", 0)
COMP(1982, c64pal, 	c64,	0,		c64pal, 		c64,	c64pal, c64,		"Commodore Business Machines Co.", "Commodore 64/VC64/VIC64 (PAL)", 0)
COMP(1982, vic64s, 	c64,	0,		c64pal, 		vic64s,	c64pal, c64,		"Commodore Business Machines Co.", "Commodore 64 Swedish (PAL)", 0)
CONS(1987, c64gs,	c64,	0,		c64gs,			c64gs,	c64gs,	c64gs,		"Commodore Business Machines Co.", "C64GS (PAL)", 0)

/* testdrivers */
COMP(1983, sx64,	c64,	0,		sx64,			sx64,	sx64,	sx64,		"Commodore Business Machines Co.", "SX64 (PAL)",                      GAME_NOT_WORKING)
COMP(1983, vip64,	c64,	0,		sx64,			vip64,	sx64,	sx64,		"Commodore Business Machines Co.", "VIP64 (SX64 PAL), Swedish Expansion Kit", GAME_NOT_WORKING)
// sx64 with second disk drive
COMP(198?, dx64,	c64,	0,		sx64,			sx64,	sx64,	sx64,		"Commodore Business Machines Co.", "DX64 (Prototype, PAL)",                      GAME_NOT_WORKING)
/*c64 II (cbm named it still c64) */
/*c64c (bios in 1 chip) */
/*c64g late 8500/8580 based c64, sold at aldi/germany */
/*c64cgs late c64, sold in ireland, gs bios?, but with keyboard */
