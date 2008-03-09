/***************************************************************************
    commodore c16 home computer

    PeT mess@utanet.at

    documentation
     www.funet.fi

***************************************************************************/

/*
------------------------------------
c16 commodore c16/c116/c232/c264 (pal version)
plus4   commodore plus4 (ntsc version)
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
 100% software compatible to c16
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
(and therefore cpu level emulation is needed)

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
#include "video/ted7360.h"
#include "devices/cartslot.h"
#include "mslegacy.h"

/*
 * commodore c16/c116/plus 4
 * 16 KByte (C16/C116) or 32 KByte or 64 KByte (plus4) RAM
 * 32 KByte Rom (C16/C116) 64 KByte Rom (plus4)
 * availability to append additional 64 KByte Rom
 *
 * ports 0xfd00 till 0xff3f are always read/writeable for the cpu
 * for the video interface chip it seams to read from
 * ram or from rom in this  area
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
/*  { 0x10000, 0x3ffff, MRA8_ROM }, */
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
//  {0x10000, 0x3ffff, MWA8_ROM},
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
/*  { 0x10000, 0x3ffff, MRA8_ROM }, */
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
//  {0x10000, 0x3ffff, MWA8_ROM},
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
/*  { 0x10000, 0x3ffff, MRA8_ROM }, */
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
//  {0x10000, 0x3ffff, MWA8_ROM},
ADDRESS_MAP_END

static INPUT_PORTS_START( ports_both )
	PORT_START
	PORT_BIT( 8, IP_ACTIVE_HIGH, IPT_BUTTON1)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_8WAY
	PORT_BIT ( 0x7, 0x0,	 IPT_UNUSED )
	PORT_START
	PORT_BIT( 8, IP_ACTIVE_HIGH, IPT_BUTTON1)			PORT_NAME("P2 Button") PORT_CODE(KEYCODE_LALT) PORT_CODE(JOYCODE_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT)	PORT_NAME("P2 Left") PORT_CODE(KEYCODE_DEL) PORT_CODE(JOYCODE_X_LEFT_SWITCH ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT)	PORT_NAME("P2 Right") PORT_CODE(KEYCODE_PGDN) PORT_CODE(JOYCODE_X_RIGHT_SWITCH ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP)	PORT_NAME("P2 Up") PORT_CODE(KEYCODE_HOME) PORT_CODE(JOYCODE_Y_UP_SWITCH) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN)	PORT_NAME("P2 Down") PORT_CODE(KEYCODE_END) PORT_CODE(JOYCODE_Y_DOWN_SWITCH) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT ( 0x7, 0x0,	 IPT_UNUSED )
	PORT_START
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_TILDE)
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1 !   BLK   ORNG") PORT_CODE(KEYCODE_1)	PORT_CHAR('1')	PORT_CHAR('!')
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2 \"   WHT   BRN") PORT_CODE(KEYCODE_2)	PORT_CHAR('2')	PORT_CHAR('\"')
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3 #   RED   L GRN") PORT_CODE(KEYCODE_3)	PORT_CHAR('3')	PORT_CHAR('#')
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4 $   CYN   PINK") PORT_CODE(KEYCODE_4)	PORT_CHAR('4')	PORT_CHAR('$')
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5 %   PUR   BL GRN") PORT_CODE(KEYCODE_5)	PORT_CHAR('5')	PORT_CHAR('%')
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6 &   GRN   L BLU") PORT_CODE(KEYCODE_6)	PORT_CHAR('6')	PORT_CHAR('&')
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7 '   BLU   D BLU") PORT_CODE(KEYCODE_7)	PORT_CHAR('7')	PORT_CHAR('\'')
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8 (   YEL   L GRN") PORT_CODE(KEYCODE_8)	PORT_CHAR('8')	PORT_CHAR('(')
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9 )   RVS-ON") PORT_CODE(KEYCODE_9)	PORT_CHAR('9')	PORT_CHAR(')')
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0 ^   RVS-OFF") PORT_CODE(KEYCODE_0)	PORT_CHAR('0')	PORT_CHAR('^')
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Cursor Left") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Cursor Right") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Cursor Up") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Cursor Down") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("DEL INST") PORT_CODE(KEYCODE_BACKSPACE)
	PORT_START
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_RCONTROL)
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)					PORT_CHAR('Q')
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)					PORT_CHAR('W')
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)					PORT_CHAR('E')
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)					PORT_CHAR('R')
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)					PORT_CHAR('T')
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)					PORT_CHAR('Y')
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)					PORT_CHAR('U')
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)					PORT_CHAR('I')
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)					PORT_CHAR('O')
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)					PORT_CHAR('P')
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("@") PORT_CODE(KEYCODE_OPENBRACE)			PORT_CHAR('@')
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("+") PORT_CODE(KEYCODE_CLOSEBRACE)			PORT_CHAR('+')
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS)				PORT_CHAR('-')
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("HOME CLEAR") PORT_CODE(KEYCODE_EQUALS)		PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("STOP RUN") PORT_CODE(KEYCODE_TAB)
	PORT_START
	PORT_BIT ( 0x8000, 0, IPT_DIPSWITCH_NAME) PORT_NAME("SHIFT-Lock (switch)") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE
	PORT_DIPSETTING(  0, DEF_STR(Off) )
	PORT_DIPSETTING( 0x8000, DEF_STR(On) )
	/*PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("SHIFT-LOCK (switch)") PORT_CODE(KEYCODE_CAPSLOCK) */
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)					PORT_CHAR('A')
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)					PORT_CHAR('S')
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)					PORT_CHAR('D')
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)					PORT_CHAR('F')
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)					PORT_CHAR('G')
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)					PORT_CHAR('H')
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)					PORT_CHAR('J')
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)					PORT_CHAR('K')
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)					PORT_CHAR('L')
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(": [") PORT_CODE(KEYCODE_COLON)				PORT_CHAR(':') PORT_CHAR('[')
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("; ]") PORT_CODE(KEYCODE_QUOTE)				PORT_CHAR(';') PORT_CHAR(']')
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("*") PORT_CODE(KEYCODE_BACKSLASH)			PORT_CHAR('*')
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER)			PORT_CHAR('\r')
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CBM") PORT_CODE(KEYCODE_RALT)
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Left-Shift") PORT_CODE(KEYCODE_LSHIFT)		PORT_CHAR(UCHAR_SHIFT_1)
	PORT_START
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)					PORT_CHAR('Z')
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)					PORT_CHAR('X')
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)					PORT_CHAR('C')
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)					PORT_CHAR('V')
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)					PORT_CHAR('B')
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)					PORT_CHAR('N')
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)					PORT_CHAR('M')
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(". >   FLASH-OFF") PORT_CODE(KEYCODE_COMMA)	PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(". >   FLASH-OFF") PORT_CODE(KEYCODE_STOP)	PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH)				PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Right-Shift") PORT_CODE(KEYCODE_RSHIFT)		PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Pound") PORT_CODE(KEYCODE_INSERT)			PORT_CHAR(0xA3)
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("= Pi Arrow-Left") PORT_CODE(KEYCODE_PGUP)	PORT_CHAR('=')
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE)			PORT_CHAR(' ')
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("f1 f4") PORT_CODE(KEYCODE_F1)				PORT_CHAR(UCHAR_MAMEKEY(F1)) PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("f2 f5") PORT_CODE(KEYCODE_F2)				PORT_CHAR(UCHAR_MAMEKEY(F2)) PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_START
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("f3 f6") PORT_CODE(KEYCODE_F3)				PORT_CHAR(UCHAR_MAMEKEY(F3)) PORT_CHAR(UCHAR_MAMEKEY(F6))
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("HELP f7") PORT_CODE(KEYCODE_F4)
	PORT_DIPNAME ( 0x2000, 0, "Swap Gameport 1 and 2") PORT_CODE(KEYCODE_NUMLOCK)
	PORT_DIPSETTING(  0, DEF_STR(No) )
	PORT_DIPSETTING( 0x2000, DEF_STR(Yes) )
	/*PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPF_TOGGLE) PORT_NAME("Swap Gameport 1 and 2") PORT_CODE(KEYCODE_NUMLOCK)*/
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Quickload") PORT_CODE(KEYCODE_F8)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Tape Drive Play") PORT_CODE(KEYCODE_F5)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Tape Drive Record") PORT_CODE(KEYCODE_F6)
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Tape Drive Stop") PORT_CODE(KEYCODE_F7)
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

static INPUT_PORTS_START (c16)
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

static INPUT_PORTS_START (c16c)
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

static INPUT_PORTS_START (c16v)
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

static INPUT_PORTS_START (plus4)
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

static INPUT_PORTS_START (plus4c)
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

static INPUT_PORTS_START (plus4v)
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
static INPUT_PORTS_START (c364)
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
	palette_set_colors_rgb(machine, 0, ted7360_palette, sizeof(ted7360_palette) / 3);
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
	MDRV_CPU_VBLANK_INT("main", c16_frame_interrupt)
	MDRV_CPU_PERIODIC_INT(ted7360_raster_interrupt, TED7360_HRETRACERATE)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET( c16 )

    /* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(TED7360PAL_VRETRACERATE)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
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
	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_REFRESH_RATE(TED7360NTSC_VRETRACERATE)

	MDRV_SOUND_REPLACE("sid", SID8580, TED7360NTSC_CLOCK/4)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( plus4c )
	MDRV_IMPORT_FROM( plus4 )
	MDRV_IMPORT_FROM( cpu_c1551 )
	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
#ifdef CPU_SYNC
	MDRV_INTERLEAVE(1)
#else
	MDRV_INTERLEAVE(1000)
#endif
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( plus4v )
	MDRV_IMPORT_FROM( plus4 )
	MDRV_IMPORT_FROM( cpu_vc1541 )
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
#ifdef CPU_SYNC
	MDRV_INTERLEAVE(1)
#else
	MDRV_INTERLEAVE(5000)
#endif
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( c364 )
	MDRV_IMPORT_FROM( plus4 )
	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP(c364_readmem, c364_writemem)
MACHINE_DRIVER_END

static DRIVER_INIT( c16 )		{ c16_driver_init(machine); }
#ifdef UNUSED_FUNCTION
DRIVER_INIT( c16hun )	{ c16_driver_init(machine); }
DRIVER_INIT( c16c )		{ c16_driver_init(machine); }
DRIVER_INIT( c16v )		{ c16_driver_init(machine); }
#endif
static DRIVER_INIT( plus4 )	{ c16_driver_init(machine); }
#ifdef UNUSED_FUNCTION
DRIVER_INIT( plus4c )	{ c16_driver_init(machine); }
DRIVER_INIT( plus4v )	{ c16_driver_init(machine); }
DRIVER_INIT( c364 )		{ c16_driver_init(machine); }
#endif

static void c16cart_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:							info->load = device_load_c16_rom; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "bin,rom"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

static void c16_quickload_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "p00,prg"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_QUICKLOAD_LOAD:				info->f = (genf *) quickload_load_cbm_c16; break;

		/* --- the following bits of info are returned as doubles --- */
		case MESS_DEVINFO_FLOAT_QUICKLOAD_DELAY:				info->d = CBM_QUICKLOAD_DELAY; break;

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

/*      YEAR    NAME    PARENT  COMPAT  MACHINE INPUT   INIT    CONFIG   COMPANY                                FULLNAME */
COMP ( 1984,	c16,	0,		0,		c16,	c16,	c16,	c16,     "Commodore Business Machines Co.",      "Commodore 16/116/232/264 (PAL)", 0)
COMP ( 1984,	c16hun, c16,	0,		c16,	c16,	c16,	c16,     "Commodore Business Machines Co.",      "Commodore 16 Novotrade (PAL, Hungarian Character Set)", 0)
COMP ( 1984,	c16c,	c16,	0,		c16c,	c16c,	c16,	c16c,    "Commodore Business Machines Co.",      "Commodore 16/116/232/264 (PAL), 1551", GAME_NOT_WORKING)
COMP ( 1984,	plus4,	c16,	0,		plus4,	plus4,	plus4,	plus,    "Commodore Business Machines Co.",      "Commodore +4 (NTSC)", 0)
COMP ( 1984,	plus4c, c16,	0,		plus4c, plus4c, plus4,	plusc,   "Commodore Business Machines Co.",      "Commodore +4 (NTSC), 1551", GAME_NOT_WORKING)
COMP ( 1984,	c364,	c16,	0,		c364,	plus4,	plus4,	plusv,   "Commodore Business Machines Co.",      "Commodore 364 (Prototype)", GAME_IMPERFECT_SOUND)
COMP ( 1984,	c16v,	c16,	0,		c16v,	c16v,	c16,	c16v,    "Commodore Business Machines Co.",      "Commodore 16/116/232/264 (PAL), VC1541", GAME_NOT_WORKING)
COMP ( 1984,	plus4v, c16,	0,		plus4v, plus4v, plus4,	plusv,   "Commodore Business Machines Co.",      "Commodore +4 (NTSC), VC1541", GAME_NOT_WORKING)
