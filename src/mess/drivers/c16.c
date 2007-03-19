/***************************************************************************
	commodore c16 home computer

	PeT mess@utanet.at

	documentation
	 www.funet.fi

***************************************************************************/

/*
------------------------------------
c16 commodore c16/c116/c232/c264 (pal version)
plus4	commodore plus4 (ntsc version)
c364 commodore c364/v364 prototype (ntsc version)
------------------------------------
(beta version)

if the game runs to fast with the ntsc version, try the pal version!
flickering affects in one video version, try the other video version

c16(pal version, ntsc version?):
 design like the vc20/c64
 sequel to c64
 other keyboardlayout,
 worse soundchip,
 more colors, but no sprites,
 other tape and gameports plugs
 16 kbyte ram
 newer basic and kernal (ieee floppy support)

c116(pal version, ntsc version?):
 100% softwarecompatible to c16
 small and flat
 gummi keys

232:
 video system versions (?)
 plus4 case
 32 kbyte ram
 userport?, acia6551 chip?

264:
 video system versions (?)
 plus4 case
 64 kbyte ram
 userport?, acia6551 chip?

plus4(pal version, ntsc version?):
 in emu ntsc version!
 case like c116, but with normal keys
 64 kbyte ram
 userport
 build in additional rom with programs

c364 prototype:
 video system versions (?)
 like plus4, but with additional numeric keyblock
 slightly modified kernel rom
 build in speech hardware/rom

state
-----
rasterline based video system
 imperfect scrolling support (when 40 columns or 25 lines)
 lightpen support missing?
 should be enough for 95% of the games and programs
imperfect sound
keyboard, joystick 1 and 2
no speech hardware (c364)
simple tape support
serial bus
 simple disk drives
 no printer or other devices
expansion modules
 rom cartridges
 simple ieee488 floppy support (c1551 floppy disk drive)
 no other expansion modules
no userport (plus4)
 no rs232/v.24 interface
quickloader

some unsolved problems
 memory check by c16 kernel will not recognize more memory without restart of mess
 cpu clock switching/changing (overclocking should it be)

Keys
----
Some PC-Keyboards does not behave well when special two or more keys are
pressed at the same time
(with my keyboard printscreen clears the pressed pause key!)

shift-cbm switches between upper-only and normal character set
(when wrong characters on screen this can help)
run (shift-stop) loads first program from device 8 (dload"*) and starts it
stop-reset activates monitor (use x to leave it)

Tape
----
(DAC 1 volume in noise volume)
loading of wav, prg and prg files in zip archiv
commandline -cassette image
wav:
 8 or 16(not tested) bit, mono, 5000 Hz minimum
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
 loads file from rom directory (*.prg,*p00) (must NOT be specified on commandline)
 or file from d64 image (here also directory LOAD"$",8 supported)
use DLOAD"filename"
or LOAD"filename",8
or LOAD"filename",8,1 (for loading machine language programs at their address)
for loading
type RUN or the appropriate sys call to start them

several programs rely on more features
(loading other file types, writing, ...)

most games rely on starting own programs in the floppy drive
(and therefor cpu level emulation is needed)

Roms
----
.bin .rom .lo .hi .prg
files with boot-sign in it
  recogniced as roms

.prg files loaded at address in its first two bytes
.bin, .rom, .lo , .hi roms loaded to cs1 low, cs1 high, cs2 low, cs2 high
 address accordingly to order in command line

Quickloader
-----------
.prg files supported
loads program into memory and sets program end pointer
(works with most programs)
program ready to get started with RUN
loads first rom when you press quickload key (f8)

when problems start with -log and look into error.log file
 */


#include "driver.h"
#include "sound/sid6581.h"

#define VERBOSE_DBG 0
#include "includes/cbm.h"
#include "includes/c16.h"
#include "includes/cbmserb.h"
#include "includes/vc1541.h"
#include "includes/vc20tape.h"
#include "includes/ted7360.h"
#include "devices/cartslot.h"
#include "inputx.h"

/*
 * commodore c16/c116/plus 4
 * 16 KByte (C16/C116) or 32 KByte or 64 KByte (plus4) RAM
 * 32 KByte Rom (C16/C116) 64 KByte Rom (plus4)
 * availability to append additional 64 KByte Rom
 *
 * ports 0xfd00 till 0xff3f are always read/writeable for the cpu
 * for the video interface chip it seams to read from
 * ram or from rom in this	area
 *
 * writes go always to ram
 * only 16 KByte Ram mapped to 0x4000,0x8000,0xc000
 * only 32 KByte Ram mapped to 0x8000
 *
 * rom bank at 0x8000: 16K Byte(low bank)
 * first: basic
 * second(plus 4 only): plus4 rom low
 * third: expansion slot
 * fourth: expansion slot
 * rom bank at 0xc000: 16K Byte(high bank)
 * first: kernal
 * second(plus 4 only): plus4 rom high
 * third: expansion slot
 * fourth: expansion slot
 * writes to 0xfddx select rom banks:
 * address line 0 and 1: rom bank low
 * address line 2 and 3: rom bank high
 *
 * writes to 0xff3e switches to roms (0x8000 till 0xfd00, 0xff40 till 0xffff)
 * writes to 0xff3f switches to rams
 *
 * at 0xfc00 till 0xfcff is ram or rom kernal readable
 */

static ADDRESS_MAP_START( c16_readmem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_READ( MRA8_BANK9)
	AM_RANGE(0x4000, 0x7fff) AM_READ( MRA8_BANK1)	   /* only ram memory configuration */
	AM_RANGE(0x8000, 0xbfff) AM_READ( MRA8_BANK2)
	AM_RANGE(0xc000, 0xfbff) AM_READ( MRA8_BANK3)
	AM_RANGE(0xfc00, 0xfcff) AM_READ( MRA8_BANK4)
	AM_RANGE(0xfd10, 0xfd1f) AM_READ( c16_fd1x_r)
	AM_RANGE(0xfd30, 0xfd3f) AM_READ( c16_6529_port_r) /* 6529 keyboard matrix */
#if 0
	AM_RANGE( 0xfd40, 0xfd5f) AM_READ( sid6581_0_port_r ) /* sidcard, eoroidpro ... */
	AM_RANGE(0xfec0, 0xfedf) AM_READ( c16_iec9_port_r) /* configured in c16_common_init */
	AM_RANGE(0xfee0, 0xfeff) AM_READ( c16_iec8_port_r) /* configured in c16_common_init */
#endif
	AM_RANGE(0xff00, 0xff1f) AM_READ( ted7360_port_r)
	AM_RANGE(0xff20, 0xffff) AM_READ( MRA8_BANK8)
/*	{ 0x10000, 0x3ffff, MRA8_ROM }, */
ADDRESS_MAP_END

static ADDRESS_MAP_START( c16_writemem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_WRITE( MWA8_BANK9)
	AM_RANGE(0x4000, 0x7fff) AM_WRITE( MWA8_BANK5)
	AM_RANGE(0x8000, 0xbfff) AM_WRITE( MWA8_BANK6)
	AM_RANGE(0xc000, 0xfcff) AM_WRITE( MWA8_BANK7)
#if 0
	AM_RANGE(0x4000, 0x7fff) AM_WRITE( c16_write_4000)  /*configured in c16_common_init */
	AM_RANGE(0x8000, 0xbfff) AM_WRITE( c16_write_8000)  /*configured in c16_common_init */
	AM_RANGE(0xc000, 0xfcff) AM_WRITE( c16_write_c000)  /*configured in c16_common_init */
#endif
	AM_RANGE(0xfd30, 0xfd3f) AM_WRITE( c16_6529_port_w) /* 6529 keyboard matrix */
#if 0
	AM_RANGE(0xfd40, 0xfd5f) AM_WRITE( sid6581_0_port_w)
#endif
	AM_RANGE(0xfdd0, 0xfddf) AM_WRITE( c16_select_roms) /* rom chips selection */
#if 0
	AM_RANGE(0xfec0, 0xfedf) AM_WRITE( c16_iec9_port_w) /*configured in c16_common_init */
	AM_RANGE(0xfee0, 0xfeff) AM_WRITE( c16_iec8_port_w) /*configured in c16_common_init */
#endif
	AM_RANGE(0xff00, 0xff1f) AM_WRITE( ted7360_port_w)
#if 0
	AM_RANGE(0xff20, 0xff3d) AM_WRITE( c16_write_ff20)  /*configure in c16_common_init */
#endif
	AM_RANGE(0xff3e, 0xff3e) AM_WRITE( c16_switch_to_rom)
	AM_RANGE(0xff3f, 0xff3f) AM_WRITE( c16_switch_to_ram)
#if 0
	AM_RANGE(0xff40, 0xffff) AM_WRITE( c16_write_ff40)  /*configure in c16_common_init */
//	{0x10000, 0x3ffff, MWA8_ROM},
#endif
ADDRESS_MAP_END

static ADDRESS_MAP_START( plus4_readmem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x7fff) AM_READ( MRA8_BANK9)
	AM_RANGE(0x8000, 0xbfff) AM_READ( MRA8_BANK2)
	AM_RANGE(0xc000, 0xfbff) AM_READ( MRA8_BANK3)
	AM_RANGE(0xfc00, 0xfcff) AM_READ( MRA8_BANK4)
	AM_RANGE(0xfd00, 0xfd0f) AM_READ( c16_6551_port_r)
	AM_RANGE(0xfd10, 0xfd1f) AM_READ( plus4_6529_port_r)
	AM_RANGE(0xfd30, 0xfd3f) AM_READ( c16_6529_port_r) /* 6529 keyboard matrix */
#if 0
	AM_RANGE( 0xfd40, 0xfd5f) AM_READ( sid6581_0_port_r ) /* sidcard, eoroidpro ... */
	AM_RANGE(0xfec0, 0xfedf) AM_READ( c16_iec9_port_r) /* configured in c16_common_init */
	AM_RANGE(0xfee0, 0xfeff) AM_READ( c16_iec8_port_r) /* configured in c16_common_init */
#endif
	AM_RANGE(0xff00, 0xff1f) AM_READ( ted7360_port_r)
	AM_RANGE(0xff20, 0xffff) AM_READ( MRA8_BANK8)
/*	{ 0x10000, 0x3ffff, MRA8_ROM }, */
ADDRESS_MAP_END

static ADDRESS_MAP_START( plus4_writemem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0xfcff) AM_WRITE( MWA8_BANK9)
	AM_RANGE(0xfd00, 0xfd0f) AM_WRITE( c16_6551_port_w)
	AM_RANGE(0xfd10, 0xfd1f) AM_WRITE( plus4_6529_port_w)
	AM_RANGE(0xfd30, 0xfd3f) AM_WRITE( c16_6529_port_w) /* 6529 keyboard matrix */
#if 0
	AM_RANGE(0xfd40, 0xfd5f) AM_WRITE( sid6581_0_port_w)
#endif
	AM_RANGE(0xfdd0, 0xfddf) AM_WRITE( c16_select_roms) /* rom chips selection */
#if 0
	AM_RANGE(0xfec0, 0xfedf) AM_WRITE( c16_iec9_port_w) /*configured in c16_common_init */
	AM_RANGE(0xfee0, 0xfeff) AM_WRITE( c16_iec8_port_w) /*configured in c16_common_init */
#endif
	AM_RANGE(0xff00, 0xff1f) AM_WRITE( ted7360_port_w)
	AM_RANGE(0xff20, 0xff3d) AM_WRITE( MWA8_RAM)
	AM_RANGE(0xff3e, 0xff3e) AM_WRITE( c16_switch_to_rom)
	AM_RANGE(0xff3f, 0xff3f) AM_WRITE( c16_switch_to_ram)
	AM_RANGE(0xff40, 0xffff) AM_WRITE( MWA8_RAM)
//	{0x10000, 0x3ffff, MWA8_ROM},
ADDRESS_MAP_END

static ADDRESS_MAP_START( c364_readmem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x7fff) AM_READ( MRA8_BANK9)
	AM_RANGE(0x8000, 0xbfff) AM_READ( MRA8_BANK2)
	AM_RANGE(0xc000, 0xfbff) AM_READ( MRA8_BANK3)
	AM_RANGE(0xfc00, 0xfcff) AM_READ( MRA8_BANK4)
	AM_RANGE(0xfd00, 0xfd0f) AM_READ( c16_6551_port_r)
	AM_RANGE(0xfd10, 0xfd1f) AM_READ( plus4_6529_port_r)
	AM_RANGE(0xfd20, 0xfd2f) AM_READ( c364_speech_r )
	AM_RANGE(0xfd30, 0xfd3f) AM_READ( c16_6529_port_r) /* 6529 keyboard matrix */
#if 0
	AM_RANGE( 0xfd40, 0xfd5f) AM_READ( sid6581_0_port_r ) /* sidcard, eoroidpro ... */
	AM_RANGE(0xfec0, 0xfedf) AM_READ( c16_iec9_port_r) /* configured in c16_common_init */
	AM_RANGE(0xfee0, 0xfeff) AM_READ( c16_iec8_port_r) /* configured in c16_common_init */
#endif
	AM_RANGE(0xff00, 0xff1f) AM_READ( ted7360_port_r)
	AM_RANGE(0xff20, 0xffff) AM_READ( MRA8_BANK8)
/*	{ 0x10000, 0x3ffff, MRA8_ROM }, */
ADDRESS_MAP_END

static ADDRESS_MAP_START( c364_writemem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0xfcff) AM_WRITE( MWA8_BANK9)
	AM_RANGE(0xfd00, 0xfd0f) AM_WRITE( c16_6551_port_w)
	AM_RANGE(0xfd10, 0xfd1f) AM_WRITE( plus4_6529_port_w)
	AM_RANGE(0xfd20, 0xfd2f) AM_WRITE( c364_speech_w )
	AM_RANGE(0xfd30, 0xfd3f) AM_WRITE( c16_6529_port_w) /* 6529 keyboard matrix */
#if 0
	AM_RANGE(0xfd40, 0xfd5f) AM_WRITE( sid6581_0_port_w)
#endif
	AM_RANGE(0xfdd0, 0xfddf) AM_WRITE( c16_select_roms) /* rom chips selection */
#if 0
	AM_RANGE(0xfec0, 0xfedf) AM_WRITE( c16_iec9_port_w) /*configured in c16_common_init */
	AM_RANGE(0xfee0, 0xfeff) AM_WRITE( c16_iec8_port_w) /*configured in c16_common_init */
#endif
	AM_RANGE(0xff00, 0xff1f) AM_WRITE( ted7360_port_w)
	AM_RANGE(0xff20, 0xff3d) AM_WRITE( MWA8_RAM)
	AM_RANGE(0xff3e, 0xff3e) AM_WRITE( c16_switch_to_rom)
	AM_RANGE(0xff3f, 0xff3f) AM_WRITE( c16_switch_to_ram)
	AM_RANGE(0xff40, 0xffff) AM_WRITE( MWA8_RAM)
//	{0x10000, 0x3ffff, MWA8_ROM},
ADDRESS_MAP_END

#define DIPS_HELPER(bit, name, keycode) \
   PORT_BIT(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(name) PORT_CODE(keycode)

static INPUT_PORTS_START( ports_both )
	PORT_START
	PORT_BIT( 8, IP_ACTIVE_HIGH, IPT_BUTTON1)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_8WAY
	PORT_BIT ( 0x7, 0x0,	 IPT_UNUSED )
	PORT_START
	PORT_BIT( 8, IP_ACTIVE_HIGH, IPT_BUTTON1)			PORT_NAME("P2 Button") PORT_CODE(KEYCODE_LALT) PORT_CODE(JOYCODE_2_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT)	PORT_NAME("P2 Left") PORT_CODE(KEYCODE_DEL) PORT_CODE(JOYCODE_2_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT)	PORT_NAME("P2 Right") PORT_CODE(KEYCODE_PGDN) PORT_CODE(JOYCODE_2_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP)	PORT_NAME("P2 Up") PORT_CODE(KEYCODE_HOME) PORT_CODE(JOYCODE_2_UP) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN)	PORT_NAME("P2 Down") PORT_CODE(KEYCODE_END) PORT_CODE(JOYCODE_2_DOWN) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT ( 0x7, 0x0,	 IPT_UNUSED )
	PORT_START
	DIPS_HELPER( 0x8000, "ESC", KEYCODE_TILDE)
	DIPS_HELPER( 0x4000, "1 !   BLK   ORNG",	KEYCODE_1)	PORT_CHAR('1')	PORT_CHAR('!')
	DIPS_HELPER( 0x2000, "2 \"   WHT   BRN",	KEYCODE_2)	PORT_CHAR('2')	PORT_CHAR('\"')
	DIPS_HELPER( 0x1000, "3 #   RED   L GRN",	KEYCODE_3)	PORT_CHAR('3')	PORT_CHAR('#')	
	DIPS_HELPER( 0x0800, "4 $   CYN   PINK",	KEYCODE_4)	PORT_CHAR('4')	PORT_CHAR('$')
	DIPS_HELPER( 0x0400, "5 %   PUR   BL GRN",	KEYCODE_5)	PORT_CHAR('5')	PORT_CHAR('%')
	DIPS_HELPER( 0x0200, "6 &   GRN   L BLU",	KEYCODE_6)	PORT_CHAR('6')	PORT_CHAR('&')
	DIPS_HELPER( 0x0100, "7 '   BLU   D BLU",	KEYCODE_7)	PORT_CHAR('7')	PORT_CHAR('\'')
	DIPS_HELPER( 0x0080, "8 (   YEL   L GRN",	KEYCODE_8)	PORT_CHAR('8')	PORT_CHAR('(')
	DIPS_HELPER( 0x0040, "9 )   RVS-ON",		KEYCODE_9)	PORT_CHAR('9')	PORT_CHAR(')')
	DIPS_HELPER( 0x0020, "0 ^   RVS-OFF",		KEYCODE_0)	PORT_CHAR('0')	PORT_CHAR('^')
	DIPS_HELPER( 0x0010, "Cursor Left", KEYCODE_4_PAD)
	DIPS_HELPER( 0x0008, "Cursor Right", KEYCODE_6_PAD)
	DIPS_HELPER( 0x0004, "Cursor Up", KEYCODE_8_PAD)
	DIPS_HELPER( 0x0002, "Cursor Down", KEYCODE_2_PAD)
	DIPS_HELPER( 0x0001, "DEL INST", KEYCODE_BACKSPACE)
	PORT_START
	DIPS_HELPER( 0x8000, "CTRL", KEYCODE_RCONTROL)
	DIPS_HELPER( 0x4000, "Q", KEYCODE_Q)					PORT_CHAR('Q')
	DIPS_HELPER( 0x2000, "W", KEYCODE_W)					PORT_CHAR('W')
	DIPS_HELPER( 0x1000, "E", KEYCODE_E)					PORT_CHAR('E')
	DIPS_HELPER( 0x0800, "R", KEYCODE_R)					PORT_CHAR('R')
	DIPS_HELPER( 0x0400, "T", KEYCODE_T)					PORT_CHAR('T')
	DIPS_HELPER( 0x0200, "Y", KEYCODE_Y)					PORT_CHAR('Y')
	DIPS_HELPER( 0x0100, "U", KEYCODE_U)					PORT_CHAR('U')
	DIPS_HELPER( 0x0080, "I", KEYCODE_I)					PORT_CHAR('I')
	DIPS_HELPER( 0x0040, "O", KEYCODE_O)					PORT_CHAR('O')
	DIPS_HELPER( 0x0020, "P", KEYCODE_P)					PORT_CHAR('P')
	DIPS_HELPER( 0x0010, "@", KEYCODE_OPENBRACE)			PORT_CHAR('@')
	DIPS_HELPER( 0x0008, "+", KEYCODE_CLOSEBRACE)			PORT_CHAR('+')
	DIPS_HELPER( 0x0004, "-", KEYCODE_MINUS)				PORT_CHAR('-')
	DIPS_HELPER( 0x0002, "HOME CLEAR", KEYCODE_EQUALS)		PORT_CHAR(UCHAR_MAMEKEY(HOME))
	DIPS_HELPER( 0x0001, "STOP RUN", KEYCODE_TAB)
	PORT_START
	PORT_BIT ( 0x8000, 0, IPT_DIPSWITCH_NAME) PORT_NAME("SHIFT-Lock (switch)") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE
	PORT_DIPSETTING(  0, DEF_STR(Off) )
	PORT_DIPSETTING( 0x8000, DEF_STR(On) )
	/*PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("SHIFT-LOCK (switch)") PORT_CODE(KEYCODE_CAPSLOCK) */
	DIPS_HELPER( 0x4000, "A", KEYCODE_A)					PORT_CHAR('A')
	DIPS_HELPER( 0x2000, "S", KEYCODE_S)					PORT_CHAR('S')
	DIPS_HELPER( 0x1000, "D", KEYCODE_D)					PORT_CHAR('D')
	DIPS_HELPER( 0x0800, "F", KEYCODE_F)					PORT_CHAR('F')
	DIPS_HELPER( 0x0400, "G", KEYCODE_G)					PORT_CHAR('G')
	DIPS_HELPER( 0x0200, "H", KEYCODE_H)					PORT_CHAR('H')
	DIPS_HELPER( 0x0100, "J", KEYCODE_J)					PORT_CHAR('J')
	DIPS_HELPER( 0x0080, "K", KEYCODE_K)					PORT_CHAR('K')
	DIPS_HELPER( 0x0040, "L", KEYCODE_L)					PORT_CHAR('L')
	DIPS_HELPER( 0x0020, ": [", KEYCODE_COLON)				PORT_CHAR(':') PORT_CHAR('[')
	DIPS_HELPER( 0x0010, "; ]", KEYCODE_QUOTE)				PORT_CHAR(';') PORT_CHAR(']')
	DIPS_HELPER( 0x0008, "*", KEYCODE_BACKSLASH)			PORT_CHAR('*')
	DIPS_HELPER( 0x0004, "RETURN",KEYCODE_ENTER)			PORT_CHAR('\r')
	DIPS_HELPER( 0x0002, "CBM", KEYCODE_RALT)
	DIPS_HELPER( 0x0001, "Left-Shift", KEYCODE_LSHIFT)		PORT_CHAR(UCHAR_SHIFT_1)
	PORT_START
	DIPS_HELPER( 0x8000, "Z", KEYCODE_Z)					PORT_CHAR('Z')
	DIPS_HELPER( 0x4000, "X", KEYCODE_X)					PORT_CHAR('X')
	DIPS_HELPER( 0x2000, "C", KEYCODE_C)					PORT_CHAR('C')
	DIPS_HELPER( 0x1000, "V", KEYCODE_V)					PORT_CHAR('V')
	DIPS_HELPER( 0x0800, "B", KEYCODE_B)					PORT_CHAR('B')
	DIPS_HELPER( 0x0400, "N", KEYCODE_N)					PORT_CHAR('N')
	DIPS_HELPER( 0x0200, "M", KEYCODE_M)					PORT_CHAR('M')
	DIPS_HELPER( 0x0100, ", <   FLASH-ON", KEYCODE_COMMA)	PORT_CHAR(',') PORT_CHAR('<')
	DIPS_HELPER( 0x0080, ". >   FLASH-OFF", KEYCODE_STOP)	PORT_CHAR('.') PORT_CHAR('>')
	DIPS_HELPER( 0x0040, "/ ?", KEYCODE_SLASH)				PORT_CHAR('/') PORT_CHAR('?')
	DIPS_HELPER( 0x0020, "Right-Shift", KEYCODE_RSHIFT)		PORT_CHAR(UCHAR_SHIFT_1)
	DIPS_HELPER( 0x0010, "Pound", KEYCODE_INSERT)			PORT_CHAR(0xA3)
	DIPS_HELPER( 0x0008, "= Pi Arrow-Left", KEYCODE_PGUP)	PORT_CHAR('=')
	DIPS_HELPER( 0x0004, "Space", KEYCODE_SPACE)			PORT_CHAR(' ')
	DIPS_HELPER( 0x0002, "f1 f4", KEYCODE_F1)				PORT_CHAR(UCHAR_MAMEKEY(F1)) PORT_CHAR(UCHAR_MAMEKEY(F4))
	DIPS_HELPER( 0x0001, "f2 f5", KEYCODE_F2)				PORT_CHAR(UCHAR_MAMEKEY(F2)) PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_START 
	DIPS_HELPER( 0x8000, "f3 f6", KEYCODE_F3)				PORT_CHAR(UCHAR_MAMEKEY(F3)) PORT_CHAR(UCHAR_MAMEKEY(F6))
	DIPS_HELPER( 0x4000, "HELP f7", KEYCODE_F4)
	PORT_DIPNAME ( 0x2000, 0, "Swap Gameport 1 and 2") PORT_CODE(KEYCODE_NUMLOCK)
	PORT_DIPSETTING(  0, DEF_STR(No) )
	PORT_DIPSETTING( 0x2000, DEF_STR(Yes) )
	/*PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPF_TOGGLE) PORT_NAME("Swap Gameport 1 and 2") PORT_CODE(KEYCODE_NUMLOCK)*/
	DIPS_HELPER( 0x08, "Quickload", KEYCODE_F8)
	DIPS_HELPER( 0x04, "Tape Drive Play",       KEYCODE_F5)
	DIPS_HELPER( 0x02, "Tape Drive Record",     KEYCODE_F6)
	DIPS_HELPER( 0x01, "Tape Drive Stop",       KEYCODE_F7)
	PORT_START 
	PORT_DIPNAME ( 0x80, 0x80, "Joystick 1")
	PORT_DIPSETTING(  0, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
	PORT_DIPNAME ( 0x40, 0x40, "Joystick 2")
	PORT_DIPSETTING(  0, DEF_STR( Off ) )
	PORT_DIPSETTING(0x40, DEF_STR( On ) )
	PORT_DIPNAME   ( 0x20, 0x20, "Tape Drive/Device 1")
	PORT_DIPSETTING(  0, DEF_STR( Off ) )
	PORT_DIPSETTING(0x20, DEF_STR( On ) )
	PORT_DIPNAME   ( 0x10, 0x00, " Tape Sound")
	PORT_DIPSETTING(  0, DEF_STR( Off ) )
	PORT_DIPSETTING(0x10, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START (c16)
	PORT_INCLUDE( ports_both )
	PORT_START
	PORT_BIT (0xc0, 0x0, IPT_UNUSED)		   /* no real floppy */
	PORT_DIPNAME ( 0x38, 8, "Device 8")\
	PORT_DIPSETTING(  0, DEF_STR( None ) )\
	PORT_DIPSETTING( 8, "C1551 Floppy Drive Simulation" )\
	PORT_DIPSETTING( 0x18, "Serial Bus/VC1541 Floppy Drive Simulation" )\
	PORT_DIPNAME ( 0x07, 0x01, "Device 9")\
	PORT_DIPSETTING(  0, DEF_STR( None ) )\
	PORT_DIPSETTING(  1, "C1551 Floppy Drive Simulation" )\
	PORT_DIPSETTING(  3, "Serial Bus/VC1541 Floppy Drive Simulation" )\
	PORT_START
	PORT_DIPNAME ( 0x80, 0x80, "Sidcard")
	PORT_DIPSETTING(  0, DEF_STR(Off) )
	PORT_DIPSETTING( 0x80, DEF_STR(On) )
	PORT_DIPNAME ( 0x40, 0, " SID 0xd400 hack")
	PORT_DIPSETTING(  0, DEF_STR(Off) )
	PORT_DIPSETTING( 0x40, DEF_STR(On) )
	PORT_BIT (0x10, 0x0, IPT_UNUSED)		   /* pal */
	PORT_BIT (0xc, 0x0, IPT_UNUSED) 	   /* c16 */
INPUT_PORTS_END

INPUT_PORTS_START (c16c)
	PORT_INCLUDE( ports_both )
	PORT_START
	PORT_BIT (0xc0, 0x40, IPT_UNUSED)		   /* c1551 floppy */
	PORT_BIT (0x38, 0x10, IPT_UNUSED)
	PORT_BIT (0x7, 0x0, IPT_UNUSED)
	PORT_START
	PORT_DIPNAME ( 0x80, 0x80, "Sidcard")
	PORT_DIPSETTING(  0, DEF_STR(Off) )
	PORT_DIPSETTING( 0x80, DEF_STR(On) )
	PORT_DIPNAME ( 0x40, 0, " SID 0xd400 hack")
	PORT_DIPSETTING(  0, DEF_STR(Off) )
	PORT_DIPSETTING( 0x40, DEF_STR(On) )
	PORT_BIT (0x10, 0x0, IPT_UNUSED)		   /* pal */
	PORT_BIT (0xc, 0x0, IPT_UNUSED) 	   /* c16 */
INPUT_PORTS_END

INPUT_PORTS_START (c16v)
	PORT_INCLUDE( ports_both )
	PORT_START
	PORT_BIT (0xc0, 0x80, IPT_UNUSED)		   /* vc1541 floppy */
	PORT_BIT (0x38, 0x20, IPT_UNUSED)
	PORT_BIT (0x7, 0x0, IPT_UNUSED)
	PORT_START
	PORT_DIPNAME ( 0x80, 0x80, "Sidcard")
	PORT_DIPSETTING(  0, DEF_STR(Off) )
	PORT_DIPSETTING( 0x80, DEF_STR(On) )
	PORT_DIPNAME ( 0x40, 0, " SID 0xd400 hack")
	PORT_DIPSETTING(  0, DEF_STR(Off) )
	PORT_DIPSETTING( 0x40, DEF_STR(On) )
	PORT_BIT (0x10, 0x0, IPT_UNUSED)		   /* pal */
	PORT_BIT (0xc, 0x0, IPT_UNUSED) 	   /* c16 */
INPUT_PORTS_END

INPUT_PORTS_START (plus4)
	PORT_INCLUDE( ports_both )
	PORT_START
	PORT_BIT (0xc0, 0x00, IPT_UNUSED)		   /* no real floppy */
	PORT_DIPNAME ( 0x38, 0x8, "Device 8")\
	PORT_DIPSETTING(  0, DEF_STR( None ) )\
	PORT_DIPSETTING(  8, "C1551 Floppy Drive Simulation" )\
	PORT_DIPSETTING(  0x18, "Serial Bus/VC1541 Floppy Drive Simulation" )\
	PORT_DIPNAME ( 0x07, 0x01, "Device 9")\
	PORT_DIPSETTING(  0, DEF_STR( None ) )\
	PORT_DIPSETTING(  1, "C1551 Floppy Drive Simulation" )\
	PORT_DIPSETTING(  3, "Serial Bus/VC1541 Floppy Drive Simulation" )\
	PORT_START
	PORT_DIPNAME ( 0x80, 0x80, "Sidcard")
	PORT_DIPSETTING(  0, DEF_STR(Off) )
	PORT_DIPSETTING( 0x80, DEF_STR(On) )
	PORT_DIPNAME ( 0x40, 0, " SID 0xd400 hack")
	PORT_DIPSETTING(  0, DEF_STR(Off) )
	PORT_DIPSETTING( 0x40, DEF_STR(On) )
	PORT_BIT (0x10, 0x10, IPT_UNUSED)		   /* ntsc */
	PORT_BIT (0xc, 0x4, IPT_UNUSED) 	   /* plus4 */
INPUT_PORTS_END

INPUT_PORTS_START (plus4c)
	PORT_INCLUDE( ports_both )
	PORT_START
	PORT_BIT (0xc0, 0x40, IPT_UNUSED)		   /* c1551 floppy */
	PORT_BIT (0x38, 0x10, IPT_UNUSED)
	PORT_BIT (0x7, 0x0, IPT_UNUSED)
	PORT_START
	PORT_DIPNAME ( 0x80, 0x80, "Sidcard")
	PORT_DIPSETTING(  0, DEF_STR(Off) )
	PORT_DIPSETTING( 0x80, DEF_STR(On) )
	PORT_DIPNAME ( 0x40, 0, " SID 0xd400 hack")
	PORT_DIPSETTING(  0, DEF_STR(Off) )
	PORT_DIPSETTING( 0x40, DEF_STR(On) )
	PORT_BIT (0x10, 0x10, IPT_UNUSED)		   /* ntsc */
	PORT_BIT (0xc, 0x4, IPT_UNUSED) 	   /* plus4 */
INPUT_PORTS_END

INPUT_PORTS_START (plus4v)
	PORT_INCLUDE( ports_both )
	PORT_START
	PORT_BIT (0xc0, 0x80, IPT_UNUSED)		   /* vc1541 floppy */
	PORT_BIT (0x38, 0x20, IPT_UNUSED)
	PORT_BIT (0x7, 0x0, IPT_UNUSED)
	PORT_START
	PORT_DIPNAME ( 0x80, 0x80, "Sidcard")
	PORT_DIPSETTING(  0, DEF_STR(Off) )
	PORT_DIPSETTING( 0x80, DEF_STR(On) )
	PORT_DIPNAME ( 0x40, 0, " SID 0xd400 hack")
	PORT_DIPSETTING(  0, DEF_STR(Off) )
	PORT_DIPSETTING( 0x40, DEF_STR(On) )
	PORT_BIT (0x10, 0x10, IPT_UNUSED)		   /* ntsc */
	PORT_BIT (0xc, 0x4, IPT_UNUSED) 	   /* plus4 */
INPUT_PORTS_END

#if 0
INPUT_PORTS_START (c364)
	PORT_INCLUDE( ports_both )
	PORT_START
	PORT_BIT (0xc0, 0x00, IPT_UNUSED)		   /* no real floppy */
	PORT_DIPNAME ( 0x38, 0x10, "Device 8")\
	PORT_DIPSETTING(  0, DEF_STR( None ) )\
	PORT_DIPSETTING(  8, "C1551 Floppy Drive Simulation" )\
	PORT_DIPSETTING(  0x18, "Serial Bus/VC1541 Floppy Drive Simulation" )\
	PORT_DIPNAME ( 0x03, 0x01, "Device 9")\
	PORT_DIPSETTING(  0, DEF_STR( None ) )\
	PORT_DIPSETTING(  1, "C1551 Floppy Drive Simulation" )\
	PORT_DIPSETTING(  3, "Serial Bus/VC1541 Floppy Drive Simulation" )\
	PORT_START
	PORT_DIPNAME ( 0x80, 0x80, "Sidcard")
	PORT_DIPSETTING(  0, DEF_STR(Off) )
	PORT_DIPSETTING( 0x80, DEF_STR(On) )
	PORT_DIPNAME ( 0x40, 0, " SID 0xd400 hack")
	PORT_DIPSETTING(  0, DEF_STR(Off) )
	PORT_DIPSETTING( 0x40, DEF_STR(On) )
	PORT_BIT (0x10, 0x10, IPT_UNUSED)		   /* ntsc */
	PORT_BIT (0xc, 0x8, IPT_UNUSED) 	   /* 364 */
	 /* numeric block
		hardware wired to other keys?
		@ + - =
		7 8 9 *
		4 5 6 /
		1 2 3
		? ? ? ?
		( 0 , . Return ???) */
INPUT_PORTS_END
#endif

/* Initialise the c16 palette */
static PALETTE_INIT( c16 )
{
	palette_set_colors(machine, 0, ted7360_palette, sizeof(ted7360_palette) / 3);
}

#if 0
/* cbm version in kernel at 0xff80 (offset 0x3f80)
   0x80 means pal version */

	 /* basic */
	 ROM_LOAD ("318006.01", 0x10000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd))

	 /* kernal pal */
	 ROM_LOAD("318004.05",    0x14000, 0x4000, CRC(71c07bd4) SHA1(7c7e07f016391174a557e790c4ef1cbe33512cdb))
	 ROM_LOAD ("318004.03", 0x14000, 0x4000, CRC(77bab934))

	 /* kernal ntsc */
	 ROM_LOAD ("318005.05", 0x14000, 0x4000, CRC(70295038) SHA1(a3d9e5be091b98de39a046ab167fb7632d053682))
	 ROM_LOAD ("318005.04", 0x14000, 0x4000, CRC(799a633d))

	 /* 3plus1 program */
	 ROM_LOAD ("317053.01", 0x18000, 0x4000, CRC(4fd1d8cb) SHA1(3b69f6e7cb4c18bb08e203fb18b7dabfa853390f))
	 ROM_LOAD ("317054.01", 0x1c000, 0x4000, CRC(109de2fc) SHA1(0ad7ac2db7da692d972e586ca0dfd747d82c7693))

	 /* same as 109de2fc, but saved from running machine, so
		io area different ! */
	 ROM_LOAD ("3plus1hi.rom", 0x1c000, 0x4000, CRC(aab61387))
	 /* same but lo and hi in one rom */
	 ROM_LOAD ("3plus1.rom", 0x18000, 0x8000, CRC(7d464449))
#endif

ROM_START (c16)
	 ROM_REGION (0x40000, REGION_CPU1, 0)
	 ROM_LOAD ("318006.01", 0x10000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd))
	 ROM_LOAD("318004.05",    0x14000, 0x4000, CRC(71c07bd4) SHA1(7c7e07f016391174a557e790c4ef1cbe33512cdb))
ROM_END

ROM_START (c16hun)
	 ROM_REGION (0x40000, REGION_CPU1, 0)
	 ROM_LOAD ("318006.01", 0x10000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd))
	 ROM_LOAD("hungary.bin",    0x14000, 0x4000, CRC(775f60c5) SHA1(20cf3c4bf6c54ef09799af41887218933f2e27ee))
ROM_END

ROM_START (c16c)
	 ROM_REGION (0x40000, REGION_CPU1, 0)
	 ROM_LOAD ("318006.01", 0x10000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd))
	 ROM_LOAD("318004.05",    0x14000, 0x4000, CRC(71c07bd4) SHA1(7c7e07f016391174a557e790c4ef1cbe33512cdb))
	 C1551_ROM (REGION_CPU2)
ROM_END

ROM_START (c16v)
	 ROM_REGION (0x40000, REGION_CPU1, 0)
	 ROM_LOAD ("318006.01", 0x10000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd))
	 ROM_LOAD("318004.05",    0x14000, 0x4000, CRC(71c07bd4) SHA1(7c7e07f016391174a557e790c4ef1cbe33512cdb))
	 VC1541_ROM (REGION_CPU2)
ROM_END

ROM_START (plus4)
	 ROM_REGION (0x40000, REGION_CPU1, 0)
	 ROM_LOAD ("318006.01", 0x10000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd))
	 ROM_LOAD ("318005.05", 0x14000, 0x4000, CRC(70295038) SHA1(a3d9e5be091b98de39a046ab167fb7632d053682))
	 ROM_LOAD ("317053.01", 0x18000, 0x4000, CRC(4fd1d8cb) SHA1(3b69f6e7cb4c18bb08e203fb18b7dabfa853390f))
	 ROM_LOAD ("317054.01", 0x1c000, 0x4000, CRC(109de2fc) SHA1(0ad7ac2db7da692d972e586ca0dfd747d82c7693))
ROM_END

ROM_START (plus4c)
	 ROM_REGION (0x40000, REGION_CPU1, 0)
	 ROM_LOAD ("318006.01", 0x10000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd))
	 ROM_LOAD ("318005.05", 0x14000, 0x4000, CRC(70295038) SHA1(a3d9e5be091b98de39a046ab167fb7632d053682))
	 ROM_LOAD ("317053.01", 0x18000, 0x4000, CRC(4fd1d8cb) SHA1(3b69f6e7cb4c18bb08e203fb18b7dabfa853390f))
	 ROM_LOAD ("317054.01", 0x1c000, 0x4000, CRC(109de2fc) SHA1(0ad7ac2db7da692d972e586ca0dfd747d82c7693))
	 C1551_ROM (REGION_CPU2)
ROM_END

ROM_START (plus4v)
	 ROM_REGION (0x40000, REGION_CPU1, 0)
	 ROM_LOAD ("318006.01", 0x10000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd))
	 ROM_LOAD ("318005.05", 0x14000, 0x4000, CRC(70295038) SHA1(a3d9e5be091b98de39a046ab167fb7632d053682))
	 ROM_LOAD ("317053.01", 0x18000, 0x4000, CRC(4fd1d8cb) SHA1(3b69f6e7cb4c18bb08e203fb18b7dabfa853390f))
	 ROM_LOAD ("317054.01", 0x1c000, 0x4000, CRC(109de2fc) SHA1(0ad7ac2db7da692d972e586ca0dfd747d82c7693))
	 VC1541_ROM (REGION_CPU2)
ROM_END

ROM_START (c364)
	 ROM_REGION (0x40000, REGION_CPU1, 0)
	 ROM_LOAD ("318006.01", 0x10000, 0x4000, CRC(74eaae87) SHA1(161c96b4ad20f3a4f2321808e37a5ded26a135dd))
	 ROM_LOAD ("kern364p.bin", 0x14000, 0x4000, CRC(84fd4f7a) SHA1(b9a5b5dacd57ca117ef0b3af29e91998bf4d7e5f))
	 ROM_LOAD ("317053.01", 0x18000, 0x4000, CRC(4fd1d8cb) SHA1(3b69f6e7cb4c18bb08e203fb18b7dabfa853390f))
	 ROM_LOAD ("317054.01", 0x1c000, 0x4000, CRC(109de2fc) SHA1(0ad7ac2db7da692d972e586ca0dfd747d82c7693))
	 /* at address 0x20000 not so good */
	 ROM_LOAD ("spk3cc4.bin", 0x28000, 0x4000, CRC(5227c2ee) SHA1(59af401cbb2194f689898271c6e8aafa28a7af11))
ROM_END



static MACHINE_DRIVER_START( c16 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M7501, 1400000)        /* 7.8336 Mhz */
	MDRV_CPU_PROGRAM_MAP(c16_readmem, c16_writemem)
	MDRV_CPU_VBLANK_INT(c16_frame_interrupt, 1)
	MDRV_CPU_PERIODIC_INT(ted7360_raster_interrupt, TIME_IN_HZ(TED7360_HRETRACERATE))
	MDRV_SCREEN_REFRESH_RATE(TED7360PAL_VRETRACERATE)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET( c16 )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(336, 216)
	MDRV_SCREEN_VISIBLE_AREA(0, 336 - 1, 0, 216 - 1)
	MDRV_PALETTE_LENGTH(sizeof (ted7360_palette) / sizeof (ted7360_palette[0]) / 3)
	MDRV_PALETTE_INIT(c16)

	MDRV_VIDEO_START( ted7360 )
	MDRV_VIDEO_UPDATE( ted7360 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD_TAG("ted7360", CUSTOM, 0)
	MDRV_SOUND_CONFIG(ted7360_sound_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD_TAG("sid", SID8580, TED7360PAL_CLOCK/4)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( c16c )
	MDRV_IMPORT_FROM( c16 )
	MDRV_IMPORT_FROM( cpu_c1551 )
#ifdef CPU_SYNC
	MDRV_INTERLEAVE(1)
#else
	MDRV_INTERLEAVE(100)
#endif
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( c16v )
	MDRV_IMPORT_FROM( c16 )
	MDRV_IMPORT_FROM( cpu_vc1541 )
#ifdef CPU_SYNC
	MDRV_INTERLEAVE(1)
#else
	MDRV_INTERLEAVE(5000)
#endif
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( plus4 )
	MDRV_IMPORT_FROM( c16 )
	MDRV_CPU_REPLACE( "main", M7501, 1200000)
	MDRV_CPU_PROGRAM_MAP( plus4_readmem, plus4_writemem )
	MDRV_SCREEN_REFRESH_RATE(TED7360NTSC_VRETRACERATE)

	MDRV_SOUND_REPLACE("sid", SID8580, TED7360NTSC_CLOCK/4)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( plus4c )
	MDRV_IMPORT_FROM( plus4 )
	MDRV_IMPORT_FROM( cpu_c1551 )
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
#ifdef CPU_SYNC
	MDRV_INTERLEAVE(1)
#else
	MDRV_INTERLEAVE(1000)
#endif
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( plus4v )
	MDRV_IMPORT_FROM( plus4 )
	MDRV_IMPORT_FROM( cpu_vc1541 )
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
#ifdef CPU_SYNC
	MDRV_INTERLEAVE(1)
#else
	MDRV_INTERLEAVE(5000)
#endif
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( c364 )
	MDRV_IMPORT_FROM( plus4 )
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP(c364_readmem, c364_writemem)
MACHINE_DRIVER_END

DRIVER_INIT( c16 )		{ c16_driver_init(); }
DRIVER_INIT( c16hun )	{ c16_driver_init(); }
DRIVER_INIT( c16c )		{ c16_driver_init(); }
DRIVER_INIT( c16v )		{ c16_driver_init(); }
DRIVER_INIT( plus4 )	{ c16_driver_init(); }
DRIVER_INIT( plus4c )	{ c16_driver_init(); }
DRIVER_INIT( plus4v )	{ c16_driver_init(); }
DRIVER_INIT( c364 )		{ c16_driver_init(); }

static void c16cart_device_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_c16_rom; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "bin,rom"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

static void c16_quickload_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "p00,prg"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_QUICKLOAD_LOAD:				info->f = (genf *) quickload_load_cbm_c16; break;

		/* --- the following bits of info are returned as doubles --- */
		case DEVINFO_FLOAT_QUICKLOAD_DELAY:				info->d = CBM_QUICKLOAD_DELAY; break;

		default:										quickload_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(c16)
	CONFIG_DEVICE(c16cart_device_getinfo)
	CONFIG_DEVICE(cbmfloppy_device_getinfo)
	CONFIG_DEVICE(c16_quickload_getinfo)
	CONFIG_DEVICE(vc20tape_device_getinfo)
	CONFIG_RAM(16 * 1024)
	CONFIG_RAM(32 * 1024)
	CONFIG_RAM_DEFAULT(64 * 1024)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(c16c)
	CONFIG_DEVICE(c16cart_device_getinfo)
	CONFIG_DEVICE(c16_quickload_getinfo)
	CONFIG_DEVICE(vc20tape_device_getinfo)
	CONFIG_DEVICE(c1551_device_getinfo)
	CONFIG_RAM(16 * 1024)
	CONFIG_RAM(32 * 1024)
	CONFIG_RAM_DEFAULT(64 * 1024)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(c16v)
	CONFIG_DEVICE(c16cart_device_getinfo)
	CONFIG_DEVICE(c16_quickload_getinfo)
	CONFIG_DEVICE(vc20tape_device_getinfo)
	CONFIG_DEVICE(vc1541_device_getinfo)
	CONFIG_RAM(16 * 1024)
	CONFIG_RAM(32 * 1024)
	CONFIG_RAM_DEFAULT(64 * 1024)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(plus)
	CONFIG_DEVICE(c16cart_device_getinfo)
	CONFIG_DEVICE(cbmfloppy_device_getinfo)
	CONFIG_DEVICE(c16_quickload_getinfo)
	CONFIG_DEVICE(vc20tape_device_getinfo)
	CONFIG_RAM_DEFAULT(64 * 1024)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(plusc)
	CONFIG_DEVICE(c16cart_device_getinfo)
	CONFIG_DEVICE(c16_quickload_getinfo)
	CONFIG_DEVICE(vc20tape_device_getinfo)
	CONFIG_DEVICE(c1551_device_getinfo)
	CONFIG_RAM_DEFAULT(64 * 1024)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(plusv)
	CONFIG_DEVICE(c16cart_device_getinfo)
	CONFIG_DEVICE(c16_quickload_getinfo)
	CONFIG_DEVICE(vc20tape_device_getinfo)
	CONFIG_DEVICE(vc1541_device_getinfo)
	CONFIG_RAM_DEFAULT(64 * 1024)
SYSTEM_CONFIG_END

/*		YEAR	NAME	PARENT	COMPAT	MACHINE INPUT	INIT	CONFIG   COMPANY 								FULLNAME */
COMP ( 1984,	c16,	0,		0,		c16,	c16,	c16,	c16,     "Commodore Business Machines Co.",      "Commodore 16/116/232/264 (PAL)", 0)
COMP ( 1984,	c16hun, c16,	0,		c16,	c16,	c16,	c16,     "Commodore Business Machines Co.",      "Commodore 16 Novotrade (PAL, Hungarian Character Set)", 0)
COMP ( 1984,	c16c,	c16,	0,		c16c,	c16c,	c16,	c16c,    "Commodore Business Machines Co.",      "Commodore 16/116/232/264 (PAL), 1551", GAME_NOT_WORKING)
COMP ( 1984,	plus4,	c16,	0,		plus4,	plus4,	plus4,	plus,    "Commodore Business Machines Co.",      "Commodore +4 (NTSC)", 0)
COMP ( 1984,	plus4c, c16,	0,		plus4c, plus4c, plus4,	plusc,   "Commodore Business Machines Co.",      "Commodore +4 (NTSC), 1551", GAME_NOT_WORKING)
COMP ( 1984,	c364,	c16,	0,		c364,	plus4,	plus4,	plusv,   "Commodore Business Machines Co.",      "Commodore 364 (Prototype)", GAME_IMPERFECT_SOUND)
COMP ( 1984,	c16v,	c16,	0,		c16v,	c16v,	c16,	c16v,    "Commodore Business Machines Co.",      "Commodore 16/116/232/264 (PAL), VC1541", GAME_NOT_WORKING)
COMP ( 1984,	plus4v, c16,	0,		plus4v, plus4v, plus4,	plusv,   "Commodore Business Machines Co.",      "Commodore +4 (NTSC), VC1541", GAME_NOT_WORKING)
