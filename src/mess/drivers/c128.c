/***************************************************************************
    commodore c128 home computer

    PeT mess@utanet.at

    documentation:
     iDOC (http://www.softwolves.pp.se/idoc)
           Christian Janoff  mepk@c64.org
***************************************************************************/

/*
------------------------------------
c128    commodore c128 (ntsc version)
c128ger commodore c128 german (pal version)
------------------------------------
(preliminary version)

if the game runs to fast with the ntsc version, try the pal version!
flickering affects in one video version, try the other video version

c128
 hardware emulation mode for c64
  displayed over din/tv connector only
  c64 cartridges
  c64 userport
  only the c64 standard keys available
 country support (keyboard, character rom, kernel (keyboard polling))
  character rom german
  character rom french, belgium, italian
  editor, kernel for german
  editor, kernel for swedish
  editor, kernel for finish
  editor, kernel for norwegian
  editor, kernel for french
  editor, kernel for italian
 enhanced keyboard
 m8502 processor (additional port pin)
 additional vdc videochip for 80 column modes with own connector (rgbi)
  16k byte ram for this chip
 128 kbyte ram, 64 kbyte rom, 8k byte charrom, 2k byte static colorram
  (1 mbyte ram possible)
 sid6581 soundchip
 c64 expansion port with some additional pins
 z80 cpu for CPM mode
c128d
 c1571 floppy drive build in
 64kb vdc ram
 sid8580 sound chip
c128cr/c128dcr
 cost reduced
 only modified for cheaper production (newer rams, ...)

state
-----
uses c64 emulation for c64 mode
so only notes for the additional subsystems here

rasterline based video system
 no cpu holding
 imperfect scrolling support (when 40 columns or 25 lines)
 lightpen support not finished
 rasterline not finished
vdc emulation
 dirtybuffered video system
 text mode
  only standard 8x8 characters supported
 graphic mode not tested
 lightpen not supported
 scrolling not supported
z80 emulation
 floppy simulation not enough for booting CPM
 so simplified z80 memory management not tested
no cpu clock doubling
no internal function rom
c64 mode
 differences to real c64???
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
 (not working, cia timing, cpu timing?)
serial bus
 simple disk drives
 no printer or other devices
expansion modules
 no c128 modules
expansion modules c64
 rom cartridges (exrom)
 ultimax rom cartridges (game)
 c64 cartridges (only standard rom cartridges)
 no other rom cartridges (bankswitching logic in it, switching exrom, game)
 no ieee488 support
 no cpm cartridge
 no speech cartridge (no circuit diagram found)
 no fm sound cartridge
 no other expansion modules
no userport
 no rs232/v.24 interface
quickloader

Keys
----
Some PC-Keyboards does not behave well when special two or more keys are
pressed at the same time
(with my keyboard printscreen clears the pressed pause key!)

shift-cbm switches between upper-only and normal character set
(when wrong characters on screen this can help)
run (shift-stop) loads pogram from type and starts it
esc-x switch between two videosystems

additional keys (to c64) are in c64mode not useable

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
loads first rom when you press quickload key (numeric slash)

c64 mode
--------
hold down commodore key while reseting or turning on
type go64 at the c128 command mode

cpm mode
--------
cpm disk must be inserted in device 8
turn on computer
or
type boot at the c128 command mode

when problems start with -log and look into error.log file

rom dumping
-----------

Dumping of the roms from the running machine:
in the monitor program
s "drive:name",device,start,end

s "0:basic",8,f4000,fc000
s "0:editor",8,fc000,fd000
s "0:kernel",8,ee000,f0000
s "0:char128",8,ed000,ee000

*z80bios missing (funet says only 1 version!?)
I don't know, maybe there is a cpm utility allowing saving
the memory area 0-0xfff of bank 0.
(I don't want to develope (and cant test) this short complicated
program)

*c64basic and kernel (only 1 version!?)
in c64 mode
poke43,0:poke44,160:poke45,0:poke46,192:save"0:basic64",8

in c64 mode
for i=0 to 8191:poke 32*256+i, peek(224*256+i): next
poke43,0:poke44,32:poke45,0:poke46,64:save"0:kernel64",8

*c64 charset (swedish version or original c64 version)
in c128 mode
monitor
a 2000 sei
lda #33
sta 1
ldy #0
sty fa
sty fc
lda #c0
sta fd
lda #d0
sta fb
ldx #10
lda (fa),y
sta (fc),y
iny
bne 2015
inc fb
inc fd
dex
bne 2015
lda #37
sta 1
cli
rts
(additional enter to end assembler input)
x (to leave monitor)
go64 (answer with y)
sys 32*256
poke 43,0:poke44,192:poke45,0:poke46,208:save"0:char64",8

or in c64 mode
load the program in the attachment
load"savechar64",8,1
sys 32*256
poke 43,0:poke44,192:poke45,0:poke46,208:save"0:char64",8

c128d floppy disk bios:
I think you have to download a program
copying the bios to puffers.
Then you could read this buffer into the computer, or write
these buffers to disk.

Transportation to your pc:
1571 writing to mfm encoded disketts (in cpm mode only, or use program)
 maybe the IBM CPM-86 formats are like the standard DOS formats.
 but using dd may create images known by some other emulators.
1581 writes mfm encoded:
 can one of these drives to a format know by linux?
Some years ago I build a simple adapter pc/parport to vc1541
 floppy disk drive.

Dumping roms with epromer
-------------------------
c128
U18       (read compatible 2764?) 8kB c64 character rom, c128 character rom
U32 23128 (read compatible 27128?) 16kB c64 Basic, c64 Kernel
U33 23128 (read compatible 27128?) 16kB c128 Basic at 0x4000
U34 23128 (read compatible 27128?) 16kB c128 Basic at 0x8000
U35 23128 (read compatible 27128?) 16kB c128 Editor, Z80Bios, C128 Kernel
c128 cost reduced
U18       (read compatible 2764?) 8kB c64 character rom, c128 character rom
U32 23256 (read compatible 27256?) 32kB c64 Basic, c64 Kernel, c128 Editor, Z80Bios, C128 Kernel
U34 23256 (read compatible 27256?) 32kB C128 Basic
c128dcr
as c128 cr
U102 23256 (read compatible 27256?) 32kB 1571 system rom

*/

#include "driver.h"
#include "includes/c128.h"
#include "includes/c64.h"

#include "sound/sid6581.h"
#include "machine/6526cia.h"

#define VERBOSE_DBG 0
#include "includes/cbm.h"
#include "machine/cbmipt.h"
#include "video/vic6567.h"
#include "video/vdc8563.h"
#include "includes/cbmserb.h"
#include "includes/vc1541.h"
#include "includes/vc20tape.h"


/* shares ram with m8502
 * how to bankswitch ?
 * bank 0
 * 0x0000, 0x03ff bios rom
 * 0x1400, 0x1bff vdc videoram
 * 0x1c00, 0x23ff vdc colorram
 * 0x2c00, 0x2fff vic2e videoram
 * 0xff00, 0xff04 mmu registers
 * else ram (dram bank 0?)
 * bank 1
 * 0x0000-0xedff ram (dram bank 1?)
 * 0xe000-0xffff ram as bank 0
 */
static ADDRESS_MAP_START(c128_z80_mem , ADDRESS_SPACE_PROGRAM, 8)
#if 1
	AM_RANGE(0x0000, 0x0fff) AM_READWRITE(SMH_BANK10, c128_write_0000)
	AM_RANGE(0x1000, 0xbfff) AM_READWRITE(SMH_BANK11, c128_write_1000)
	AM_RANGE(0xc000, 0xffff) AM_RAM
#else
	/* best to do reuse bankswitching numbers */
	AM_RANGE(0x0000, 0x03ff) AM_READWRITE(SMH_BANK10, SMH_BANK1)
	AM_RANGE(0x0400, 0x0fff) AM_READWRITE(SMH_BANK11, SMH_BANK2)
	AM_RANGE(0x1000, 0x1fff) AM_RAMBANK(3)
	AM_RANGE(0x2000, 0x3fff) AM_RAMBANK(4)
	AM_RANGE(0x4000, 0xbfff) AM_RAMBANK(5)
	AM_RANGE(0xc000, 0xdfff) AM_RAMBANK(6)
	AM_RANGE(0xe000, 0xefff) AM_RAMBANK(7)
	AM_RANGE(0xf000, 0xfeff) AM_RAMBANK(8)
	AM_RANGE(0xff00, 0xff04) AM_READWRITE(c128_mmu8722_ff00_r, c128_mmu8722_ff00_w)
	AM_RANGE(0xff05, 0xffff) AM_RAMBANK(9)
#endif

#if 0
	AM_RANGE(0x10000, 0x1ffff) AM_WRITE(SMH_RAM)
	AM_RANGE(0x20000, 0xfffff) AM_WRITE(SMH_RAM)	   /* or nothing */
	AM_RANGE(0x100000, 0x107fff) AM_WRITE(SMH_ROM) AM_BASE(&c128_basic)	/* maps to 0x4000 */
	AM_RANGE(0x108000, 0x109fff) AM_WRITE(SMH_ROM) AM_BASE(&c64_basic)	/* maps to 0xa000 */
	AM_RANGE(0x10a000, 0x10bfff) AM_WRITE(SMH_ROM) AM_BASE(&c64_kernal)	/* maps to 0xe000 */
	AM_RANGE(0x10c000, 0x10cfff) AM_WRITE(SMH_ROM) AM_BASE(&c128_editor)
	AM_RANGE(0x10d000, 0x10dfff) AM_WRITE(SMH_ROM) AM_BASE(&c128_z80)		/* maps to z80 0 */
	AM_RANGE(0x10e000, 0x10ffff) AM_WRITE(SMH_ROM) AM_BASE(&c128_kernal)
	AM_RANGE(0x110000, 0x117fff) AM_WRITE(SMH_ROM) AM_BASE(&c128_internal_function)
	AM_RANGE(0x118000, 0x11ffff) AM_WRITE(SMH_ROM) AM_BASE(&c128_external_function)
	AM_RANGE(0x120000, 0x120fff) AM_WRITE(SMH_ROM) AM_BASE(&c64_chargen)
	AM_RANGE(0x121000, 0x121fff) AM_WRITE(SMH_ROM) AM_BASE(&c128_chargen)
	AM_RANGE(0x122000, 0x1227ff) AM_WRITE(SMH_ROM) AM_BASE(&c64_colorram)
	AM_RANGE(0x122800, 0x1327ff) AM_WRITE(SMH_ROM) AM_BASE(&c128_vdcram)
	/* 2 kbyte by 8 bits, only 1 kbyte by 4 bits used) */
#endif
ADDRESS_MAP_END

static ADDRESS_MAP_START( c128_z80_io , ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x1000, 0x13ff) AM_READWRITE(c64_colorram_read, c64_colorram_write)
	AM_RANGE(0xd000, 0xd3ff) AM_READWRITE(vic2_port_r, vic2_port_w)
	AM_RANGE(0xd400, 0xd4ff) AM_READWRITE(sid6581_0_port_r, sid6581_0_port_w)
	AM_RANGE(0xd500, 0xd5ff) AM_READWRITE(c128_mmu8722_port_r, c128_mmu8722_port_w)
	AM_RANGE(0xd600, 0xd7ff) AM_READWRITE(vdc8563_port_r, vdc8563_port_w)
	AM_RANGE(0xdc00, 0xdcff) AM_READWRITE(cia_0_r, cia_0_w)
	AM_RANGE(0xdd00, 0xddff) AM_READWRITE(cia_1_r, cia_1_w)
/*  AM_RANGE(0xdf00, 0xdfff) AM_READWRITE(dma_port_r, dma_port_w) */
ADDRESS_MAP_END

static ADDRESS_MAP_START( c128_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x00ff) AM_RAMBANK(1)
	AM_RANGE(0x0100, 0x01ff) AM_RAMBANK(2)
	AM_RANGE(0x0200, 0x03ff) AM_RAMBANK(3)
	AM_RANGE(0x0400, 0x0fff) AM_RAMBANK(4)
	AM_RANGE(0x1000, 0x1fff) AM_RAMBANK(5)
	AM_RANGE(0x2000, 0x3fff) AM_RAMBANK(6)

	AM_RANGE(0x4000, 0x7fff) AM_READWRITE( SMH_BANK7, c128_write_4000 )
	AM_RANGE(0x8000, 0x9fff) AM_READWRITE( SMH_BANK8, c128_write_8000 )
	AM_RANGE(0xa000, 0xbfff) AM_READWRITE( SMH_BANK9, c128_write_a000 )

	AM_RANGE(0xc000, 0xcfff) AM_READWRITE( SMH_BANK12, c128_write_c000 )
	AM_RANGE(0xd000, 0xdfff) AM_READWRITE( SMH_BANK13, c128_write_d000 )
	AM_RANGE(0xe000, 0xfeff) AM_READWRITE( SMH_BANK14, c128_write_e000 )
	AM_RANGE(0xff00, 0xff04) AM_READWRITE( SMH_BANK15, c128_write_ff00 )	   /* mmu c128 modus */
	AM_RANGE(0xff05, 0xffff) AM_READWRITE( SMH_BANK16, c128_write_ff05 )
ADDRESS_MAP_END


/*************************************
 *
 *  Input Ports
 *
 *************************************/


static INPUT_PORTS_START( c128 )
	PORT_INCLUDE( common_cbm_keyboard )		/* ROW0 -> ROW7 */

	PORT_START_TAG( "KP0" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1_PAD)				PORT_CHAR(UCHAR_MAMEKEY(1_PAD))
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(7_PAD))
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(4_PAD))
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2_PAD)				PORT_CHAR(UCHAR_MAMEKEY(2_PAD))
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F5)				PORT_CHAR('\t')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(5_PAD))
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(8_PAD))
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Help") PORT_CODE(KEYCODE_F7) PORT_CHAR(UCHAR_MAMEKEY(PGUP))

	PORT_START_TAG( "KP1" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(3_PAD))
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(9_PAD))
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(6_PAD))
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_ENTER_PAD) 		PORT_CHAR(UCHAR_MAMEKEY(ENTER_PAD))
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Line Feed") PORT_CODE(KEYCODE_F8) PORT_CHAR(UCHAR_MAMEKEY(PGDN))
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_PLUS_PAD)			PORT_CHAR(UCHAR_MAMEKEY(MINUS_PAD))
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS_PAD) 		PORT_CHAR(UCHAR_MAMEKEY(PLUS_PAD))
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_ESC)				PORT_CHAR(UCHAR_MAMEKEY(ESC))

	PORT_START_TAG( "KP2" )
	PORT_DIPNAME( 0x80, 0x00, "NO SCROLL (switch)") PORT_CODE(KEYCODE_F9)
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_RIGHT) 			PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_LEFT)				PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_DOWN)				PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_UP)				PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_DEL_PAD)			PORT_CHAR(UCHAR_MAMEKEY(DEL_PAD))
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0_PAD) 			PORT_CHAR(UCHAR_MAMEKEY(0_PAD))
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Alt") PORT_CODE(KEYCODE_F6) PORT_CHAR(UCHAR_MAMEKEY(LALT))
	
	PORT_INCLUDE( c128_special )			/* SPECIAL */

	PORT_INCLUDE( c64_controls )			/* JOY0, JOY1, PADDLE0 -> PADDLE3, TRACKX, TRACKY, TRACKIPT */

	PORT_INCLUDE( c64_control_cfg )			/* DSW */

	PORT_INCLUDE( c128_config )				/* CFG */
INPUT_PORTS_END


static INPUT_PORTS_START( c128ger )
	PORT_INCLUDE( c128 )

	PORT_MODIFY( "ROW1" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Z  { Y }") PORT_CODE(KEYCODE_Z)					PORT_CHAR('Z')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("3  #  { 3  Paragraph }") PORT_CODE(KEYCODE_3)		PORT_CHAR('3') PORT_CHAR('#')
	PORT_MODIFY( "ROW3" )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Y  { Z }") PORT_CODE(KEYCODE_Y)					PORT_CHAR('Y')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("7  '  { 7  / }") PORT_CODE(KEYCODE_7)				PORT_CHAR('7') PORT_CHAR('\'')
	PORT_MODIFY( "ROW4" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("0  { = }") PORT_CODE(KEYCODE_0)					PORT_CHAR('0')
	PORT_MODIFY( "ROW5" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(",  <  { ; }") PORT_CODE(KEYCODE_COMMA)			PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Paragraph  \xE2\x86\x91  { \xc3\xbc }") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR(0x00A7) PORT_CHAR(0x2191)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(":  [  { \xc3\xa4 }") PORT_CODE(KEYCODE_COLON)		PORT_CHAR(':') PORT_CHAR('[')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(".  >  { : }") PORT_CODE(KEYCODE_STOP)				PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("-  { '  ` }") PORT_CODE(KEYCODE_EQUALS)			PORT_CHAR('-')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("+  { \xc3\x9f ? }") PORT_CODE(KEYCODE_MINUS)		PORT_CHAR('+')
	PORT_MODIFY( "ROW6" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("/  ?  { -  _ }") PORT_CODE(KEYCODE_SLASH)					PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Sum  Pi  { ] \\ }") PORT_CODE(KEYCODE_DEL)				PORT_CHAR(0x03A3) PORT_CHAR(0x03C0)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("=  { # ' }") PORT_CODE(KEYCODE_BACKSLASH)					PORT_CHAR('=')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(";  ]  { \xc3\xb6 }") PORT_CODE(KEYCODE_QUOTE)				PORT_CHAR(';') PORT_CHAR(']')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("*  `  { +  * }") PORT_CODE(KEYCODE_CLOSEBRACE)			PORT_CHAR('*') PORT_CHAR('`')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\\  { [  \xE2\x86\x91 }") PORT_CODE(KEYCODE_BACKSLASH2)	PORT_CHAR('\\')
	PORT_MODIFY( "ROW7" )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("_  { <  > }") PORT_CODE(KEYCODE_TILDE)					PORT_CHAR('_')

	PORT_MODIFY( "SPECIAL" )
	PORT_DIPNAME( 0x20, 0x00, "ASCII DIN (switch)")
	PORT_DIPSETTING(	0x00, "ASCII")
	PORT_DIPSETTING(	0x20, "DIN")
INPUT_PORTS_END


static INPUT_PORTS_START( c128fra )
	PORT_INCLUDE( c128 )

	PORT_MODIFY( "ROW1" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Z  { W }") PORT_CODE(KEYCODE_Z)				PORT_CHAR('Z')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("4  $  { '  4 }") PORT_CODE(KEYCODE_4)			PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("A  { Q }") PORT_CODE(KEYCODE_A)				PORT_CHAR('A')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("W  { Z }") PORT_CODE(KEYCODE_W)				PORT_CHAR('W')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("3  #  { \"  3 }") PORT_CODE(KEYCODE_3)		PORT_CHAR('3') PORT_CHAR('#')
	PORT_MODIFY( "ROW2" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("6  &  { Paragraph  6 }") PORT_CODE(KEYCODE_6)	PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("5  %  { (  5 }") PORT_CODE(KEYCODE_5)			PORT_CHAR('5') PORT_CHAR('%')
	PORT_MODIFY( "ROW3" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("8  (  { !  8 }") PORT_CODE(KEYCODE_8)			PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("7  '  { \xc3\xa8  7 }") PORT_CODE(KEYCODE_7)	PORT_CHAR('7') PORT_CHAR('\'')
	PORT_MODIFY( "ROW4" )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("K  Large-  { \\ }") PORT_CODE(KEYCODE_K)		PORT_CHAR('K')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("M  Large-/  { ,  ? }") PORT_CODE(KEYCODE_M)	PORT_CHAR('M')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("0  { \xc3\xa0  0 }") PORT_CODE(KEYCODE_0)		PORT_CHAR('0')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("9  )  { \xc3\xa7  9 }") PORT_CODE(KEYCODE_9)	PORT_CHAR('9') PORT_CHAR(')')
	PORT_MODIFY( "ROW5" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(",  <  { ;  . }") PORT_CODE(KEYCODE_COMMA)						PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("@  \xc3\xbb  { ^  \xc2\xa8 }") PORT_CODE(KEYCODE_OPENBRACE)	PORT_CHAR('@') PORT_CHAR(0x00FB)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(":  [  { \xc3\xb9  % }") PORT_CODE(KEYCODE_COLON)				PORT_CHAR(':') PORT_CHAR('[')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(".  >  { :  / }") PORT_CODE(KEYCODE_STOP)						PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("-  \xc2\xb0  { -  _ }") PORT_CODE(KEYCODE_EQUALS)				PORT_CHAR('-') PORT_CHAR('\xB0')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("+  \xc3\xab  { )  \xc2\xb0 }") PORT_CODE(KEYCODE_MINUS)		PORT_CHAR('+') PORT_CHAR(0x00EB)
	PORT_MODIFY( "ROW6" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("/  ?  { =  + }") PORT_CODE(KEYCODE_SLASH)						PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x91  Pi  { *  ] }") PORT_CODE(KEYCODE_DEL)			PORT_CHAR(0x2191) PORT_CHAR(0x03C0)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("=  {\xE2\x86\x91  \\ }") PORT_CODE(KEYCODE_BACKSLASH)			PORT_CHAR('=')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(";  ]  { M  Large-/ }") PORT_CODE(KEYCODE_QUOTE)				PORT_CHAR(';') PORT_CHAR(']')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("*  `  { $  [ }") PORT_CODE(KEYCODE_CLOSEBRACE)				PORT_CHAR('*') PORT_CHAR('`')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\\  { @  # }") PORT_CODE(KEYCODE_BACKSLASH)					PORT_CHAR('\\')
	PORT_MODIFY( "ROW7" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Q  { A }") PORT_CODE(KEYCODE_Q)				PORT_CHAR('Q')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("2  \"  { \xc3\xa9  2 }") PORT_CODE(KEYCODE_2)	PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("_   { <  > }") PORT_CODE(KEYCODE_TILDE)		PORT_CHAR('_')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("1  !  { &  1 }") PORT_CODE(KEYCODE_1)			PORT_CHAR('1') PORT_CHAR('!')

	PORT_MODIFY( "SPECIAL" )
	PORT_DIPNAME( 0x20, 0x00, "ASCII ?French? (switch)")
	PORT_DIPSETTING(	0x00, "ASCII")
	PORT_DIPSETTING(	0x20, "?French?")
INPUT_PORTS_END


static INPUT_PORTS_START( c128ita )
	PORT_INCLUDE( c128 )

	PORT_MODIFY( "ROW1" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Z  { W }") PORT_CODE(KEYCODE_Z)						PORT_CHAR('Z')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("4  $  { '  4 }") PORT_CODE(KEYCODE_4)					PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("W  { Z }") PORT_CODE(KEYCODE_W)						PORT_CHAR('W')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("3  #  { \"  3 }") PORT_CODE(KEYCODE_3)				PORT_CHAR('3') PORT_CHAR('#')
	PORT_MODIFY( "ROW2" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("6  &  { _  6 }") PORT_CODE(KEYCODE_6)					PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("5  %  { (  5 }") PORT_CODE(KEYCODE_5)					PORT_CHAR('5') PORT_CHAR('%')
	PORT_MODIFY( "ROW3" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("8  (  { &  8 }") PORT_CODE(KEYCODE_8)					PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("7  '  { \xc3\xa8  7 }") PORT_CODE(KEYCODE_7)			PORT_CHAR('7') PORT_CHAR('\'')
	PORT_MODIFY( "ROW4" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("M  Large-/  { ,  ? }") PORT_CODE(KEYCODE_M)			PORT_CHAR('M')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("0  { \xc3\xa0  0 }") PORT_CODE(KEYCODE_0)				PORT_CHAR('0')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("9  )  { \xc3\xa7  9 }") PORT_CODE(KEYCODE_9)			PORT_CHAR('9') PORT_CHAR(')')
	PORT_MODIFY( "ROW5" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(",  <   { ;  . }") PORT_CODE(KEYCODE_COMMA)			PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("@  \xc3\xbb  { \xc3\xac  = }") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('@') PORT_CHAR(0x00FB)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(":  [  { \xc3\xb9  % }") PORT_CODE(KEYCODE_COLON)		PORT_CHAR(':') PORT_CHAR('[')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(".  >  { :  / }") PORT_CODE(KEYCODE_STOP)				PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("-  \xc2\xb0  { -  + }") PORT_CODE(KEYCODE_EQUALS)		PORT_CHAR('-') PORT_CHAR('\xb0')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("+  \xc3\xab  { )  \xc2\xb0 }") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('+') PORT_CHAR(0x00EB)
	PORT_MODIFY( "ROW6" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("/  ?  { \xc3\xb2  ! }") PORT_CODE(KEYCODE_SLASH)		PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x91  Pi  { *  ] }") PORT_CODE(KEYCODE_DEL)	PORT_CHAR(0x2191) PORT_CHAR(0x03C0)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("=  { \xE2\x86\x91  \\ }") PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('=')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(";  ]  { M }") PORT_CODE(KEYCODE_QUOTE)				PORT_CHAR(';') PORT_CHAR(']')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("*  `  { $  [ }") PORT_CODE(KEYCODE_CLOSEBRACE)		PORT_CHAR('*') PORT_CHAR('`')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\\  { @  # }") PORT_CODE(KEYCODE_BACKSLASH2)			PORT_CHAR('\\')
	PORT_MODIFY( "ROW7" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("2  \"  { \xc3\xa9  2 }") PORT_CODE(KEYCODE_2)			PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("_  { <  > }") PORT_CODE(KEYCODE_TILDE)				PORT_CHAR('_')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("1  !  { \xc2\xa3  1 }") PORT_CODE(KEYCODE_1)			PORT_CHAR('1') PORT_CHAR('!')

	PORT_MODIFY( "SPECIAL" )
	PORT_DIPNAME( 0x20, 0x00, "ASCII Italian (switch)")
	PORT_DIPSETTING( 0x00, "ASCII")
	PORT_DIPSETTING( 0x20, DEF_STR( Italian ))
INPUT_PORTS_END


static INPUT_PORTS_START( c128swe )
	PORT_INCLUDE( c128 )

	PORT_MODIFY( "ROW1" )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("3  #  { 3  Paragraph }") PORT_CODE(KEYCODE_3)		PORT_CHAR('3') PORT_CHAR('#')
	PORT_MODIFY( "ROW3" )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("7  '  { 7  / }") PORT_CODE(KEYCODE_7)				PORT_CHAR('7') PORT_CHAR('\'')
	PORT_MODIFY( "ROW5" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("]  { \xc3\xa2 }") PORT_CODE(KEYCODE_OPENBRACE)	PORT_CHAR(']')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("[  { \xc3\xa4 }") PORT_CODE(KEYCODE_COLON)		PORT_CHAR('[')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS)									PORT_CHAR('=')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS)										PORT_CHAR('-')
	PORT_MODIFY( "ROW6" )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(";  +") PORT_CODE(KEYCODE_BACKSLASH)				PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xc2\xa3  { \xc3\xb6 }") PORT_CODE(KEYCODE_QUOTE)	PORT_CHAR('\xA3')
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("@") PORT_CODE(KEYCODE_CLOSEBRACE)					PORT_CHAR('@')
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(":  *") PORT_CODE(KEYCODE_BACKSLASH2)				PORT_CHAR(':') PORT_CHAR('*')
	PORT_MODIFY( "ROW7" )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("_  { <  > }") PORT_CODE(KEYCODE_TILDE)			PORT_CHAR('_')

	PORT_MODIFY( "SPECIAL" )
	PORT_DIPNAME( 0x20, 0x00, "ASCII Swedish/Finnish (switch)")
	PORT_DIPSETTING( 0x00, "ASCII")
	PORT_DIPSETTING( 0x20, "Swedish/Finnish")
INPUT_PORTS_END



static PALETTE_INIT( c128 )
{
	int i;

	for ( i = 0; i < sizeof(vic2_palette) / 3; i++ ) {
		palette_set_color_rgb(machine, i, vic2_palette[i*3], vic2_palette[i*3+1], vic2_palette[i*3+2]);
	}

	for ( i = 0; i < sizeof(vdc8563_palette) / 3; i++ ) {
		palette_set_color_rgb(machine, i + sizeof(vic2_palette) / 3, vdc8563_palette[i*3], vdc8563_palette[i*3+1], vdc8563_palette[i*3+2]);
	}
}

static const gfx_layout c128_charlayout =
{
	8,16,
	512,                                    /* 256 characters */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0,1,2,3,4,5,6,7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8
	},
	8*16
};

static const gfx_layout c128graphic_charlayout =
{
	8,1,
	256,                                    /* 256 characters */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0,1,2,3,4,5,6,7 },
	/* y offsets */
	{ 0 },
	8
};

static GFXDECODE_START( c128 )
	GFXDECODE_ENTRY( REGION_GFX1, 0x0000, c128_charlayout, 0, 0x100 )
	GFXDECODE_ENTRY( REGION_GFX2, 0x0000, c128graphic_charlayout, 0, 0x100 )
GFXDECODE_END

#if 0
/* usa first 318018-02 318019-02 318020-03 rev 0*/

/* usa mid 318018-03 318019-03 318020-04 */

/* usa last 318018-04 318019-04 318020-05 rev 1*/
/* usa dcr 318022-02 318023-02, same as usa last */

/* between rev0 and rev1 252343-03+252343-04 */

	 ROM_LOAD ("basic-4000.318018-02.bin", 0x100000, 0x4000, CRC(2ee6e2fa))
	 ROM_LOAD ("basic-8000.318019-02.bin", 0x104000, 0x4000, CRC(d551fce0))
     /* same as above, but in one chip */
     ROM_LOAD ("basic.318022-01.bin", 0x100000, 0x8000, CRC(e857df90))

	/* maybe 318018-03+318019-03 */
	 ROM_LOAD ("basic.252343-03.bin", 0x100000, 0x8000, CRC(bc07ed87))

	/* 1986 final upgrade */
	 ROM_LOAD ("basic-4000.318018-04.bin", 0x100000, 0x4000, CRC(9f9c355b))
	 ROM_LOAD ("basic-8000.318019-04.bin", 0x104000, 0x4000, CRC(6e2c91a7))
     /* same as above, but in one chip */
	 ROM_LOAD ("basic.318022-02.bin", 0x100000, 0x8000, CRC(af1ae1e8))

	 ROM_LOAD ("64c.251913-01.bin", 0x108000, 0x4000, CRC(0010ec31))

	 /* editor, z80 bios, c128kernel */
	 ROM_LOAD ("kernal.318020-03.bin", 0x10c000, 0x4000, CRC(1e94bb02))
	 ROM_LOAD ("kernal.318020-05.bin", 0x10c000, 0x4000, CRC(ba456b8e))
	 ROM_LOAD ("kernal.german.315078-01.bin", 0x10c000, 0x4000, CRC(a51e2168))
	 ROM_LOAD ("kernal.german.315078-02.bin", 0x10c000, 0x4000, CRC(b275bb2e))
	 /* 0x3e086a24 z80bios 0xca5e1179*/
	 ROM_LOAD ("french.bin", 0x10c000, 0x4000, CRC(2df282b8))
	 /* 0x71002a97 z80bios 0x167b8364*/
	 ROM_LOAD ("finnish1.bin", 0x10c000, 0x4000, CRC(d3ecea84))
	 /* 0xb7ff5efe z80bios 0x5ce42fc8 */
	 ROM_LOAD ("finnish2.bin", 0x10c000, 0x4000, CRC(9526fac4))
	/* 0x8df58148 z80bios 0x7b0d2140 */
	 ROM_LOAD ("italian.bin", 0x10c000, 0x4000, CRC(74d6b084))
	 /* 0x84c55911 z80bios 0x3ba48012 */
	 ROM_LOAD ("norwegian.bin", 0x10c000, 0x4000, CRC(a5406848))

	 /* c64 basic, c64 kernel, editor, z80 bios, c128kernel */
	/* 252913-01+318020-05 */
	 ROM_LOAD ("complete.318023-02.bin", 0x100000, 0x8000, CRC(eedc120a))
	/* 252913-01+0x98f2a2ed maybe 318020-04*/
	 ROM_LOAD ("complete.252343-04.bin", 0x108000, 0x8000, CRC(cc6bdb69))
	/* 251913-01+0xbff7550b */
	 ROM_LOAD ("complete.german.318077-01.bin", 0x108000, 0x8000, CRC(eb6e2c8f))
     /* chip label says Ker.Sw/Fi  */
	/* 901226.01+ 0xf10c2c25 +0x1cf7f729 */
	 ROM_LOAD ("complete.swedish.318034-01.bin", 0x108000, 0x8000, CRC(cb4e1719))

	 ROM_LOAD ("characters.390059-01.bin", 0x120000, 0x2000, CRC(6aaaafe6))
	 ROM_LOAD ("characters.german.315079-01.bin", 0x120000, 0x2000, CRC(fe5a2db1))
	 /* chip label says I/F/B (belgium, italian, french)  characters */
     /* italian and french verified to be the same*/
	 ROM_LOAD ("characters.french.325167-01.bin", 0x120000, 0x2000, CRC(bad36b88))

	 /* only parts of system roms, so not found in any c128 variant */
	 ROM_LOAD ("editor.finnish1.bin", 0x10c000, 0x1000, CRC(71002a97))
	 ROM_LOAD ("editor.finnish2.bin", 0x10c000, 0x1000, CRC(b7ff5efe))
	 ROM_LOAD ("editor.french.bin", 0x10c000, 0x1000, CRC(3e086a24))
	 ROM_LOAD ("editor.italian.bin", 0x10c000, 0x1000, CRC(8df58148))
	 ROM_LOAD ("editor.norwegian.bin", 0x10c000, 0x1000, CRC(84c55911))

	 ROM_LOAD ("kernalpart.finnish1.bin", 0x10e000, 0x2000, CRC(167b8364))
	 ROM_LOAD ("kernalpart.finnish2.bin", 0x10e000, 0x2000, CRC(5ce42fc8))
	 ROM_LOAD ("kernalpart.french.bin", 0x10e000, 0x2000, CRC(ca5e1179))
	 ROM_LOAD ("kernalpart.italian.bin", 0x10e000, 0x2000, CRC(7b0d2140))
	 ROM_LOAD ("kernalpart.norwegian.bin", 0x10e000, 0x2000, CRC(3ba48012))

	 ROM_LOAD ("z80bios.bin", 0x10d000, 0x1000, CRC(c38d83c6))

	 /* function rom in internal socket */
	 ROM_LOAD("super_chip.bin", 0x110000, 0x8000, CRC(a66f73c5))
#endif

ROM_START (c128)
	ROM_REGION (0x132800, REGION_CPU1, 0)
	ROM_LOAD ("318018.04", 0x100000, 0x4000, CRC(9f9c355b) SHA1(d53a7884404f7d18ebd60dd3080c8f8d71067441))
	ROM_LOAD ("318019.04", 0x104000, 0x4000, CRC(6e2c91a7) SHA1(c4fb4a714e48a7bf6c28659de0302183a0e0d6c0))
	ROM_LOAD ("251913.01", 0x108000, 0x4000, CRC(0010ec31) SHA1(765372a0e16cbb0adf23a07b80f6b682b39fbf88))
	ROM_LOAD ("318020.05", 0x10c000, 0x4000, CRC(ba456b8e) SHA1(ceb6e1a1bf7e08eb9cbc651afa29e26adccf38ab))
	ROM_LOAD ("390059.01", 0x120000, 0x2000, CRC(6aaaafe6) SHA1(29ed066d513f2d5c09ff26d9166ba23c2afb2b3f))
	ROM_REGION (0x10000, REGION_CPU2, ROMREGION_ERASEFF)
	ROM_REGION (0x2000, REGION_GFX1, ROMREGION_ERASEFF)
	ROM_REGION (0x100, REGION_GFX2, ROMREGION_ERASEFF)
ROM_END

ROM_START (c128d)
	ROM_REGION (0x132800, REGION_CPU1, 0)
	ROM_LOAD ("318022.02", 0x100000, 0x8000, CRC(af1ae1e8) SHA1(953dcdf5784a6b39ef84dd6fd968c7a03d8d6816))
	ROM_LOAD ("318023.02", 0x108000, 0x8000, CRC(eedc120a) SHA1(f98c5a986b532c78bb68df9ec6dbcf876913b99f))
	ROM_LOAD ("390059.01", 0x120000, 0x2000, CRC(6aaaafe6) SHA1(29ed066d513f2d5c09ff26d9166ba23c2afb2b3f))
	ROM_REGION (0x10000, REGION_CPU2, ROMREGION_ERASEFF)
	C1571_ROM(REGION_CPU3)
	ROM_REGION (0x2000, REGION_GFX1, ROMREGION_ERASEFF)
	ROM_REGION (0x100, REGION_GFX2, ROMREGION_ERASEFF)
ROM_END

// submitted as cost reduced set!
ROM_START (c128dita)
	ROM_REGION (0x132800, REGION_CPU1, 0)
	ROM_LOAD ("318022.02", 0x100000, 0x8000, CRC(af1ae1e8) SHA1(953dcdf5784a6b39ef84dd6fd968c7a03d8d6816))

    // in a cost reduced set this should be 1 rom
	ROM_LOAD ("251913.01", 0x108000, 0x4000, CRC(0010ec31))
//  ROM_LOAD ("901226.01", 0x108000, 0x2000, CRC(f833d117))
//  ROM_LOAD( "kern128d.ita", 0x10a000, 0x2000, CRC(f1098d37 ))
	ROM_LOAD ("318079.01", 0x10c000, 0x4000, CRC(66673e8b))

    ROM_LOAD ("325167.01", 0x120000, 0x2000, CRC(bad36b88)) // taken from funet
    // normally 1 rom
//    ROM_LOAD ("325167.01b", 0x120000, 0x1000, CRC(ec4272ee)) //standard c64 901226.01
//    ROM_LOAD ("325167.01b", 0x121000, 0x1000, CRC(2bc73556)) // bad dump

	ROM_REGION (0x10000, REGION_CPU2, ROMREGION_ERASEFF)
    // not included in submission
//  C1571_ROM(REGION_CPU3)
	ROM_REGION (0x2000, REGION_GFX1, ROMREGION_ERASEFF)
	ROM_REGION (0x100, REGION_GFX2, ROMREGION_ERASEFF)
ROM_END

ROM_START (c128ger)
	 /* c128d german */
	ROM_REGION (0x132800, REGION_CPU1, 0)
	ROM_LOAD ("318022.02", 0x100000, 0x8000, CRC(af1ae1e8) SHA1(953dcdf5784a6b39ef84dd6fd968c7a03d8d6816))
	ROM_LOAD ("318077.01", 0x108000, 0x8000, CRC(eb6e2c8f) SHA1(6b3d891fedabb5335f388a5d2a71378472ea60f4))
	ROM_LOAD ("315079.01", 0x120000, 0x2000, CRC(fe5a2db1) SHA1(638f8aff51c2ac4f99a55b12c4f8c985ef4bebd3))
	ROM_REGION (0x10000, REGION_CPU2, ROMREGION_ERASEFF)
	ROM_REGION (0x2000, REGION_GFX1, ROMREGION_ERASEFF)
	ROM_REGION (0x100, REGION_GFX2, ROMREGION_ERASEFF)
ROM_END

ROM_START (c128fra)
	ROM_REGION (0x132800, REGION_CPU1, 0)
	ROM_LOAD ("318018.04", 0x100000, 0x4000, CRC(9f9c355b) SHA1(d53a7884404f7d18ebd60dd3080c8f8d71067441))
	ROM_LOAD ("318019.04", 0x104000, 0x4000, CRC(6e2c91a7) SHA1(c4fb4a714e48a7bf6c28659de0302183a0e0d6c0))
	ROM_LOAD ("251913.01", 0x108000, 0x4000, CRC(0010ec31) SHA1(765372a0e16cbb0adf23a07b80f6b682b39fbf88))
#if 1
	ROM_LOAD ("french.bin", 0x10c000, 0x4000, CRC(2df282b8) SHA1(a0680d04db3232fa9f58598e5c9f09c4fe94f601))
#else
	ROM_LOAD ("editor.french.bin", 0x10c000, 0x1000, CRC(3e086a24))
	ROM_LOAD ("z80bios.bin", 0x10d000, 0x1000, CRC(c38d83c6))
	ROM_LOAD ("kernalpart.french.bin", 0x10e000, 0x2000, CRC(ca5e1179))
#endif
	ROM_LOAD ("325167.01", 0x120000, 0x2000, CRC(bad36b88) SHA1(9119b27a1bf885fa4c76fff5d858c74c194dd2b8))
	ROM_REGION (0x10000, REGION_CPU2, ROMREGION_ERASEFF)
	ROM_REGION (0x2000, REGION_GFX1, ROMREGION_ERASEFF)
	ROM_REGION (0x100, REGION_GFX2, ROMREGION_ERASEFF)
ROM_END

ROM_START (c128ita)
	ROM_REGION (0x132800, REGION_CPU1, 0)
	/* original 318022-01 */
	ROM_LOAD ("318018.04", 0x100000, 0x4000, CRC(9f9c355b) SHA1(d53a7884404f7d18ebd60dd3080c8f8d71067441))
	ROM_LOAD ("318019.04", 0x104000, 0x4000, CRC(6e2c91a7) SHA1(c4fb4a714e48a7bf6c28659de0302183a0e0d6c0))
	ROM_LOAD ("251913.01", 0x108000, 0x4000, CRC(0010ec31) SHA1(765372a0e16cbb0adf23a07b80f6b682b39fbf88))
	ROM_LOAD ("italian.bin", 0x10c000, 0x4000, CRC(74d6b084) SHA1(592a626eb2b5372596ac374d3505c3ce78dd040f))
	ROM_LOAD ("325167.01", 0x120000, 0x2000, CRC(bad36b88) SHA1(9119b27a1bf885fa4c76fff5d858c74c194dd2b8))
	ROM_REGION (0x10000, REGION_CPU2, ROMREGION_ERASEFF)
	ROM_REGION (0x2000, REGION_GFX1, ROMREGION_ERASEFF)
	ROM_REGION (0x100, REGION_GFX2, ROMREGION_ERASEFF)
ROM_END

ROM_START (c128swe)
	ROM_REGION (0x132800, REGION_CPU1, 0)
	ROM_LOAD ("318022.02", 0x100000, 0x8000, CRC(af1ae1e8) SHA1(953dcdf5784a6b39ef84dd6fd968c7a03d8d6816))
	ROM_LOAD ("318034.01", 0x108000, 0x8000, CRC(cb4e1719) SHA1(9b0a0cef56d00035c611e07170f051ee5e63aa3a))
	ROM_LOAD ("325181.01", 0x120000, 0x2000, CRC(7a70d9b8) SHA1(aca3f7321ee7e6152f1f0afad646ae41964de4fb))
	ROM_REGION (0x10000, REGION_CPU2, ROMREGION_ERASEFF)
	ROM_REGION (0x2000, REGION_GFX1, ROMREGION_ERASEFF)
	ROM_REGION (0x100, REGION_GFX2, ROMREGION_ERASEFF)
ROM_END

ROM_START (c128nor)
	ROM_REGION (0x132800, REGION_CPU1, 0)
	ROM_LOAD ("318018.04", 0x100000, 0x4000, BAD_DUMP CRC(9f9c355b) SHA1(d53a7884404f7d18ebd60dd3080c8f8d71067441))
	ROM_LOAD ("318019.04", 0x104000, 0x4000, BAD_DUMP CRC(6e2c91a7) SHA1(c4fb4a714e48a7bf6c28659de0302183a0e0d6c0))
	ROM_LOAD ("251913.01", 0x108000, 0x4000, BAD_DUMP CRC(0010ec31) SHA1(765372a0e16cbb0adf23a07b80f6b682b39fbf88))
	ROM_LOAD ("nor.bin", 0x10c000, 0x4000, BAD_DUMP CRC(a5406848) SHA1(00fe2fd610a812121befab1e7238fa882c0f8257))
	/* standard c64, vic20 based norwegian */
	ROM_LOAD ("char.nor", 0x120000, 0x2000, BAD_DUMP CRC(ba95c625) SHA1(5a87faa457979e7b6f434251a9e32f4483b337b3))
	ROM_REGION (0x10000, REGION_CPU2, ROMREGION_ERASEFF)
	ROM_REGION (0x2000, REGION_GFX1, ROMREGION_ERASEFF)
	ROM_REGION (0x100, REGION_GFX2, ROMREGION_ERASEFF)
ROM_END

static const SID6581_interface c128_sound_interface =
{
	c64_paddle_read
};



static MACHINE_DRIVER_START( c128 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, VIC6567_CLOCK)
	MDRV_CPU_PROGRAM_MAP( c128_z80_mem, 0 )
	MDRV_CPU_IO_MAP( c128_z80_io, 0 )
	MDRV_CPU_VBLANK_INT("main", c64_frame_interrupt)
	MDRV_CPU_PERIODIC_INT(vic2_raster_irq, VIC2_HRETRACERATE)

	MDRV_CPU_ADD_TAG("m8502", M8502, VIC6567_CLOCK)
	MDRV_CPU_PROGRAM_MAP( c128_mem, 0 )
	MDRV_CPU_VBLANK_INT("main", c64_frame_interrupt)
	MDRV_CPU_PERIODIC_INT(vic2_raster_irq, VIC2_HRETRACERATE)

	MDRV_INTERLEAVE(0)

	MDRV_MACHINE_RESET( c128 )

    /* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(VIC6567_VRETRACERATE)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(656, 216)
	MDRV_SCREEN_VISIBLE_AREA(0, 656 - 1, 0, 216 - 1)
	MDRV_GFXDECODE( c128 )
	MDRV_PALETTE_LENGTH((sizeof (vic2_palette) +sizeof(vdc8563_palette))/ sizeof (vic2_palette[0]) / 3 )
	MDRV_PALETTE_INIT( c128 )

	MDRV_VIDEO_START( c128 )
	MDRV_VIDEO_UPDATE( c128 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD_TAG("sid6581", SID6581, VIC6567_CLOCK)
	MDRV_SOUND_CONFIG(c128_sound_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD_TAG("dac", DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
	MDRV_QUICKLOAD_ADD(cbm_c64, "p00,prg", CBM_QUICKLOAD_DELAY_SECONDS)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( c128d )
	MDRV_IMPORT_FROM( c128 )
	MDRV_IMPORT_FROM( cpu_c1571 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( c128pal )
	MDRV_IMPORT_FROM( c128 )
	MDRV_CPU_REPLACE( "main", Z80, VIC6569_CLOCK )
	MDRV_CPU_REPLACE( "m8502", M8502, VIC6569_CLOCK )
	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_REFRESH_RATE(VIC6569_VRETRACERATE)

	/* sound hardware */
	MDRV_SOUND_REPLACE("sid6581", SID6581, VIC6569_CLOCK)
	MDRV_SOUND_CONFIG(c128_sound_interface)
MACHINE_DRIVER_END

static void c128_cbmcartslot_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "crt,80"); break;

		default:										cbmcartslot_device_getinfo(devclass, state, info); break;
	}
}

static SYSTEM_CONFIG_START(c128)
	CONFIG_DEVICE(c128_cbmcartslot_getinfo)
	CONFIG_DEVICE(cbmfloppy_device_getinfo)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(c128d)
	CONFIG_DEVICE(c1571_device_getinfo)
SYSTEM_CONFIG_END

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT       INIT        CONFIG  COMPANY   FULLNAME */
COMP (1985, c128,		0,		0,		c128,		c128,		c128,		c128,	"Commodore Business Machines Co.","Commodore 128 NTSC", 0)
COMP (1985, c128ger,	c128,	0,		c128pal,	c128ger,	c128pal,	c128,	"Commodore Business Machines Co.","Commodore 128 German (PAL)", 0)
COMP (1985, c128fra,	c128,	0,		c128pal,	c128fra,	c128pal,	c128,	"Commodore Business Machines Co.","Commodore 128 French (PAL)", 0)
COMP (1985, c128ita,	c128,	0,		c128pal,	c128ita,	c128pal,	c128,	"Commodore Business Machines Co.","Commodore 128 Italian (PAL)", 0)
COMP (1985, c128swe,	c128,	0,		c128pal,	c128swe,	c128pal,	c128,	"Commodore Business Machines Co.","Commodore 128 Swedish (PAL)", 0)
/* other countries spanish, belgium, norwegian */
/* please leave the following as testdriver */
COMP (1985, c128nor,	c128,	0,		c128pal,	c128ita,	c128pal,	c128,	"Commodore Business Machines Co.","Commodore 128 Norwegian (PAL)", GAME_NOT_WORKING)
COMP (1985, c128d,		c128,	0,		c128d,		c128,		c128,		c128d,	"Commodore Business Machines Co.","Commodore 128D NTSC", GAME_NOT_WORKING)
//COMP(1985,c128dita,   c128,   0,      c128d,      c128,       c128,       c128d,  "Commodore Business Machines Co.","Commodore 128D Italian (PAL)", GAME_NOT_WORKING)
COMP (1985, c128dita,	c128,	0,		c128pal,	c128ita,	c128pal,	c128d,	"Commodore Business Machines Co.","Commodore 128D Italian (PAL)", GAME_NOT_WORKING)
