/***************************************************************************
    commodore b series computer

    PeT mess@utanet.at

    documentation
     vice emulator
     www.funet.fi
***************************************************************************/

/*
CBM B Series:   6509 @ 2MHz, 6545/6845 Video, 6526 CIA, 6581 SID, BASIC 4.0+
                (Sometimes called BASIC 4.5)
                Commodore differentiated between the HP (High Profile) and
                LP (Low Profile) series by naming all HP machine CBM.
                (B128-80HP was CBM128-80).  Also, any machine with optional
                8088 CPU card had 'X' after B or CBM (BX128-80).
* CBM B128-80HP 128kB, Detached Keyboard, Cream.                            GP
* CBM B128-80LP 128kB, One-Piece, Cream, New Keyboard.                      GP
* CBM B256-80HP 256kB, Detached Keyboard, Cream.
* CBM B256-80LP 256kB, One-Piece, Cream.                                    GP
* CBM B128-40   6567, 6581, 6509, 6551, 128kB.  In B128-80LP case.
  CBM B256-40   6567, 6581, 6509, 6551, 256kB.  In B128-80LP case.
* CBM B500
* CBM B500      256kB. board same as B128-80.                               GP

CBM 500 Series: 6509, 6567, 6581, 6551.
                Sometimes called PET II series.
* CBM 500       256kB. (is this the 500, or should it 515?)                 EC
* CBM 505       64kB.
* CBM 510       128kB.
* CBM P500      64kB                                                        GP

CBM 600 Series: Same as B series LP
* CBM 610       B128-80 LP                                                  CS
* CBM 620       B256-80 LP                                                  CS

CBM 700 Series: Same as B series HP.  Also named PET 700 Series
* CBM 700       B128-80 LP (Note this unit is out of place here)
* CBM 710       B128-80 HP                                                  SL
* CBM 720       B256-80 HP                                                  GP
* CBM 730       720 with 8088 coprocessor card

CBM B or II Series
B128-80LP/610/B256-80LP/620
Pet700 Series B128-80HP/710/B256-80HP/720/BX256-80HP/730
---------------------------
M6509 2 MHZ
CRTC6545 6845 video chip
RS232 Port/6551
TAPE Port
IEEE488 Port
ROM Port
Monitor Port
Audio
reset
internal user port
internal processor/dram slot
 optional 8088 cpu card
2 internal system bus slots

LP/600 series
-------------
case with integrated powersupply

HP/700 series
-------------
separated keyboard, integrated monitor, no monitor port
no standard monitor with tv frequencies, 25 character lines
with 14 lines per character (like hercules/pc mda)

B128-80LP/610
-------------
128 KB RAM

B256-80LP/620
-------------
256 KB RAM

B128-80HP/710
-------------
128 KB RAM

B256-80HP/720
-------------
256 KB RAM

BX256-80LP/730
--------------
(720 with cpu upgrade)
8088 upgrade CPU

CBM Pet II Series
500/505/515/P500/B128-40/B256-40
--------------------------------
LP/600 case
videochip vic6567
2 gameports
m6509 clock 1? MHZ

CBM 500
-------
256 KB RAM

CBM 505/P500
-------
64 KB RAM

CBM 510
-------
128 KB RAM


state
-----
keyboard
no sound
no tape support
 no system roms supporting build in tape connector found
no ieee488 support
 no floppy support
no internal userport support
no internal slot1/slot2 support
no internal cpu/dram slot support
preliminary quickloader

state 600/700
-------------
dirtybuffer based video system
 no lightpen support
 no rasterline

state 500
-----
rasterline based video system
 no lightpen support
 no rasterline support
 memory access not complete
no gameport a
 no paddles 1,2
 no joystick 1
 no 2 button joystick/mouse joystick emulation
 no mouse
 no lightpen
no gameport b
 paddles 3,4
 joystick 2
 2 button joystick/mouse joystick emulation
 no mouse

Keys
----
Some PC-Keyboards does not behave well when special two or more keys are
pressed at the same time
(with my keyboard printscreen clears the pressed pause key!)

when problems start with -log and look into error.log file
 */

#include "driver.h"
#include "mslegacy.h"
#include "cpu/m6502/m6509.h"
#include "sound/sid6581.h"
#include "machine/6526cia.h"

#define VERBOSE_DBG 0
#include "includes/cbm.h"
#include "machine/tpi6525.h"
#include "video/vic6567.h"
#include "video/crtc6845.h"
#include "includes/cbmserb.h"
#include "includes/vc1541.h"
#include "includes/vc20tape.h"

#include "includes/cbmb.h"

static ADDRESS_MAP_START( cbmb_readmem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x00000, 0xf07ff) AM_READ( MRA8_RAM )
#if 0
	AM_RANGE(0xf0800, 0xf0fff) AM_READ( MRA8_ROM )
#endif
	AM_RANGE(0xf1000, 0xf1fff) AM_READ( MRA8_ROM ) /* cartridges or ram */
	AM_RANGE(0xf2000, 0xf3fff) AM_READ( MRA8_ROM ) /* cartridges or ram */
	AM_RANGE(0xf4000, 0xf5fff) AM_READ( MRA8_ROM )
	AM_RANGE(0xf6000, 0xf7fff) AM_READ( MRA8_ROM )
	AM_RANGE(0xf8000, 0xfbfff) AM_READ( MRA8_ROM )
	/*  {0xfc000, 0xfcfff, MRA8_ROM }, */
	AM_RANGE(0xfd000, 0xfd7ff) AM_READ( MRA8_ROM )
//  AM_RANGE(0xfd800, 0xfd8ff)
	AM_RANGE(0xfd801, 0xfd801) AM_MIRROR( 0xfe ) AM_READ( crtc6845_0_register_r )
	/* disk units */
	AM_RANGE(0xfda00, 0xfdaff) AM_READ( sid6581_0_port_r )
	/* db00 coprocessor */
	AM_RANGE(0xfdc00, 0xfdcff) AM_READ( cia_0_r )
	/* dd00 acia */
	AM_RANGE(0xfde00, 0xfdeff) AM_READ( tpi6525_0_port_r)
	AM_RANGE(0xfdf00, 0xfdfff) AM_READ( tpi6525_1_port_r)
	AM_RANGE(0xfe000, 0xfffff) AM_READ( MRA8_ROM )
ADDRESS_MAP_END

static ADDRESS_MAP_START( cbmb_writemem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x00000, 0x0ffff) AM_WRITE( MWA8_NOP )
	AM_RANGE(0x10000, 0x4ffff) AM_WRITE( MWA8_RAM )
	AM_RANGE(0x50002, 0x5ffff) AM_WRITE( MWA8_NOP )
	AM_RANGE(0x60000, 0xf07ff) AM_WRITE( MWA8_RAM )
	AM_RANGE(0xf1000, 0xf1fff) AM_WRITE( MWA8_ROM ) /* cartridges */
	AM_RANGE(0xf2000, 0xf3fff) AM_WRITE( MWA8_ROM ) /* cartridges */
	AM_RANGE(0xf4000, 0xf5fff) AM_WRITE( MWA8_ROM )
	AM_RANGE(0xf6000, 0xf7fff) AM_WRITE( MWA8_ROM )
	AM_RANGE(0xf8000, 0xfbfff) AM_WRITE( MWA8_ROM) AM_BASE( &cbmb_basic )
	AM_RANGE(0xfd000, 0xfd7ff) AM_WRITE( MWA8_RAM) AM_BASE( &videoram) AM_SIZE(&videoram_size ) /* VIDEORAM */
	AM_RANGE(0xfd800, 0xfd800) AM_MIRROR( 0xfe ) AM_WRITE( crtc6845_0_address_w )
	AM_RANGE(0xfd801, 0xfd801) AM_MIRROR( 0xfe ) AM_WRITE( crtc6845_0_register_w )
	/* disk units */
	AM_RANGE(0xfda00, 0xfdaff) AM_WRITE( sid6581_0_port_w)
	/* db00 coprocessor */
	AM_RANGE(0xfdc00, 0xfdcff) AM_WRITE( cia_0_w)
	/* dd00 acia */
	AM_RANGE(0xfde00, 0xfdeff) AM_WRITE( tpi6525_0_port_w)
	AM_RANGE(0xfdf00, 0xfdfff) AM_WRITE( tpi6525_1_port_w)
	AM_RANGE(0xfe000, 0xfffff) AM_WRITE( MWA8_ROM) AM_BASE( &cbmb_kernal )
ADDRESS_MAP_END

static ADDRESS_MAP_START( cbm500_readmem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x00000, 0xf07ff) AM_READ( MRA8_RAM )
#if 0
	AM_RANGE(0xf0800, 0xf0fff) AM_READ( MRA8_ROM )
#endif
	AM_RANGE(0xf1000, 0xf1fff) AM_READ( MRA8_ROM ) /* cartridges or ram */
	AM_RANGE(0xf2000, 0xf3fff) AM_READ( MRA8_ROM ) /* cartridges or ram */
	AM_RANGE(0xf4000, 0xf5fff) AM_READ( MRA8_ROM )
	AM_RANGE(0xf6000, 0xf7fff) AM_READ( MRA8_ROM )
	AM_RANGE(0xf8000, 0xfbfff) AM_READ( MRA8_ROM )
	/*  {0xfc000, 0xfcfff, MRA8_ROM }, */
	AM_RANGE(0xfd000, 0xfd3ff) AM_READ( MRA8_RAM ) /* videoram */
	AM_RANGE(0xfd400, 0xfd7ff) AM_READ( MRA8_RAM ) /* colorram */
	AM_RANGE(0xfd800, 0xfd8ff) AM_READ( vic2_port_r )
	/* disk units */
	AM_RANGE(0xfda00, 0xfdaff) AM_READ( sid6581_0_port_r )
	/* db00 coprocessor */
	AM_RANGE(0xfdc00, 0xfdcff) AM_READ( cia_0_r )
	/* dd00 acia */
	AM_RANGE(0xfde00, 0xfdeff) AM_READ( tpi6525_0_port_r)
	AM_RANGE(0xfdf00, 0xfdfff) AM_READ( tpi6525_1_port_r)
	AM_RANGE(0xfe000, 0xfffff) AM_READ( MRA8_ROM )
ADDRESS_MAP_END

static ADDRESS_MAP_START( cbm500_writemem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x00000, 0x1ffff) AM_WRITE( MWA8_RAM )
	AM_RANGE(0x20000, 0x2ffff) AM_WRITE( MWA8_NOP )
	AM_RANGE(0x30000, 0x7ffff) AM_WRITE( MWA8_RAM )
	AM_RANGE(0x80000, 0x8ffff) AM_WRITE( MWA8_NOP )
	AM_RANGE(0x90000, 0xf07ff) AM_WRITE( MWA8_RAM )
	AM_RANGE(0xf1000, 0xf1fff) AM_WRITE( MWA8_ROM ) /* cartridges */
	AM_RANGE(0xf2000, 0xf3fff) AM_WRITE( MWA8_ROM ) /* cartridges */
	AM_RANGE(0xf4000, 0xf5fff) AM_WRITE( MWA8_ROM )
	AM_RANGE(0xf6000, 0xf7fff) AM_WRITE( MWA8_ROM )
	AM_RANGE(0xf8000, 0xfbfff) AM_WRITE( MWA8_ROM) AM_BASE( &cbmb_basic )
	AM_RANGE(0xfd000, 0xfd3ff) AM_WRITE( MWA8_RAM) AM_BASE( &cbmb_videoram )
	AM_RANGE(0xfd400, 0xfd7ff) AM_WRITE( cbmb_colorram_w) AM_BASE( &cbmb_colorram )
	AM_RANGE(0xfd800, 0xfd8ff) AM_WRITE( vic2_port_w )
	/* disk units */
	AM_RANGE(0xfda00, 0xfdaff) AM_WRITE( sid6581_0_port_w)
	/* db00 coprocessor */
	AM_RANGE(0xfdc00, 0xfdcff) AM_WRITE( cia_0_w)
	/* dd00 acia */
	AM_RANGE(0xfde00, 0xfdeff) AM_WRITE( tpi6525_0_port_w)
	AM_RANGE(0xfdf00, 0xfdfff) AM_WRITE( tpi6525_1_port_w)
	AM_RANGE(0xfe000, 0xfffff) AM_WRITE( MWA8_ROM) AM_BASE( &cbmb_kernal )
ADDRESS_MAP_END

#define CBMB_KEYBOARD \
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("HOME CLR") PORT_CODE(KEYCODE_INSERT)\
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CRSR Up") PORT_CODE(KEYCODE_UP)\
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CRSR Down") PORT_CODE(KEYCODE_DOWN)\
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("f10") PORT_CODE(KEYCODE_F10)\
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("f9") PORT_CODE(KEYCODE_F9)\
	PORT_START /* 1 */ \
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("f8") PORT_CODE(KEYCODE_F8)\
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("f7") PORT_CODE(KEYCODE_F7)\
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("f6") PORT_CODE(KEYCODE_F6)\
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("f5") PORT_CODE(KEYCODE_F5)\
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("f4") PORT_CODE(KEYCODE_F4)\
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("f3") PORT_CODE(KEYCODE_F3)\
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("f2") PORT_CODE(KEYCODE_F2)\
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("f1") PORT_CODE(KEYCODE_F1)\
	PORT_START /* 2 */ \
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("NUM /") PORT_CODE(KEYCODE_SLASH_PAD)\
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("NUM *") PORT_CODE(KEYCODE_ASTERISK)\
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("NUM CE") PORT_CODE(KEYCODE_PGDN)\
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("NUM ?") PORT_CODE(KEYCODE_END)\
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CRSR Left") PORT_CODE(KEYCODE_LEFT)\
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("= +") PORT_CODE(KEYCODE_EQUALS)\
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0 )") PORT_CODE(KEYCODE_0)\
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9 (") PORT_CODE(KEYCODE_9)\
	PORT_START /* 3 */ \
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8 *") PORT_CODE(KEYCODE_8)\
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7 &") PORT_CODE(KEYCODE_7)\
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5 %") PORT_CODE(KEYCODE_5)\
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4 $") PORT_CODE(KEYCODE_4)\
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3 #") PORT_CODE(KEYCODE_3)\
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2 @") PORT_CODE(KEYCODE_2)\
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1 !") PORT_CODE(KEYCODE_1)\
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC)\
	PORT_START /* 4 */ \
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("NUM -") PORT_CODE(KEYCODE_MINUS_PAD)\
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("NUM 9") PORT_CODE(KEYCODE_9_PAD)\
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("NUM 8") PORT_CODE(KEYCODE_8_PAD)\
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("NUM 7") PORT_CODE(KEYCODE_7_PAD)\
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CRSR Right") PORT_CODE(KEYCODE_RIGHT)\
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Arrow-Left Pound") PORT_CODE(KEYCODE_TILDE)\
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS)\
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)\
	PORT_START /* 5 */ \
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)\
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)\
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6 Arrow-Up") PORT_CODE(KEYCODE_6)\
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)\
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)\
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)\
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)\
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB)\
	PORT_START /* 6 */ \
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("NUM +") PORT_CODE(KEYCODE_PLUS_PAD)\
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("NUM 6") PORT_CODE(KEYCODE_6_PAD)\
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("NUM 5") PORT_CODE(KEYCODE_5_PAD)\
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("NUM 4") PORT_CODE(KEYCODE_4_PAD)\
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("DEL INS") PORT_CODE(KEYCODE_BACKSPACE)\
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("]") PORT_CODE(KEYCODE_CLOSEBRACE)\
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)\
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)\
	PORT_START /* 7 */ \
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)\
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)\
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)\
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)\
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)\
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)\
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)\
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNUSED)\
	PORT_START /* 8 */ \
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("NUM Enter") PORT_CODE(KEYCODE_ENTER_PAD)\
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("NUM 3") PORT_CODE(KEYCODE_3_PAD)\
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("NUM 2") PORT_CODE(KEYCODE_2_PAD)\
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("NUM 1") PORT_CODE(KEYCODE_1_PAD)\
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CBM") PORT_CODE(KEYCODE_RALT)\
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER)\
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("[") PORT_CODE(KEYCODE_OPENBRACE)\
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("; :") PORT_CODE(KEYCODE_COLON)\
	PORT_START /* 9 */ \
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA)\
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)\
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)\
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)\
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)\
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)\
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)\
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT)\
	PORT_START /* 10 */ \
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED)\
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("NUM 00") PORT_CODE(KEYCODE_NUMLOCK)\
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("NUM .") PORT_CODE(KEYCODE_DEL_PAD)\
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("NUM 0") PORT_CODE(KEYCODE_0_PAD)\
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNUSED)\
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("PI") PORT_CODE(KEYCODE_BACKSLASH)\
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("\' \"") PORT_CODE(KEYCODE_QUOTE)\
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH)\
	PORT_START /* 11 */ \
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP)\
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE)\
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)\
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)\
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)\
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)\
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_UNUSED)\
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL)\
	PORT_START /* 12 */ \
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Quickload") PORT_CODE(KEYCODE_F8) \
	PORT_BIT (0x200, 0x200, IPT_UNUSED) /* ntsc */ \
	PORT_BIT (0x100, 0x000, IPT_UNUSED) /* cbm600 */ \
	PORT_DIPNAME( 0x0004, 0, "SHIFT-LOCK (switch)") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE\
	PORT_DIPSETTING(  0, DEF_STR( Off ) )\
	PORT_DIPSETTING(4, DEF_STR( On ) )\


static INPUT_PORTS_START (cbm600)
	PORT_START /* 0 */
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("STOP RUN") PORT_CODE(KEYCODE_DEL)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("GRAPH NORM") PORT_CODE(KEYCODE_PGUP)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RVS OFF") PORT_CODE(KEYCODE_HOME)
	CBMB_KEYBOARD
INPUT_PORTS_END

static INPUT_PORTS_START (cbm600pal)
	PORT_START /* 0 */
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("STOP RUN") PORT_CODE(KEYCODE_DEL)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("GRAPH NORM") PORT_CODE(KEYCODE_HOME)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ASCII DIN") PORT_CODE(KEYCODE_PGUP)
	CBMB_KEYBOARD
INPUT_PORTS_END

static INPUT_PORTS_START (cbm700)
	PORT_START /* 0 */
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("STOP RUN") PORT_CODE(KEYCODE_DEL)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RVS OFF") PORT_CODE(KEYCODE_HOME)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("GRAPH NORM") PORT_CODE(KEYCODE_PGUP)
	CBMB_KEYBOARD
INPUT_PORTS_END

static INPUT_PORTS_START (cbm500)
	PORT_START /* 0 */
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("STOP RUN") PORT_CODE(KEYCODE_DEL)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RVS OFF") PORT_CODE(KEYCODE_HOME)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("GRAPH NORM") PORT_CODE(KEYCODE_PGUP)
	CBMB_KEYBOARD
	/*C64_DIPS */
INPUT_PORTS_END

static const unsigned char cbm700_palette[] =
{
	0,0,0, /* black */
	0,0x80,0, /* green */
};

static const unsigned short cbmb_colortable[] = {
	0, 1
};

static const gfx_layout cbm600_charlayout =
{
	8,16,
	256,                                    /* 256 characters */
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

static const gfx_layout cbm700_charlayout =
{
	9,16,
	256,                                    /* 256 characters */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0,1,2,3,4,5,6,7,7 }, // 8.column will be cleared in cbm700_vh_start
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8
	},
	8*16
};

static GFXDECODE_START( cbm600 )
	GFXDECODE_ENTRY( REGION_GFX1, 0x0000, cbm600_charlayout, 0, 1 )
	GFXDECODE_ENTRY( REGION_GFX1, 0x1000, cbm600_charlayout, 0, 1 )
GFXDECODE_END

static GFXDECODE_START( cbm700 )
	GFXDECODE_ENTRY( REGION_GFX1, 0x0000, cbm700_charlayout, 0, 1 )
	GFXDECODE_ENTRY( REGION_GFX1, 0x1000, cbm700_charlayout, 0, 1 )
GFXDECODE_END

static PALETTE_INIT( cbm700 )
{
	palette_set_colors_rgb(machine, 0, cbm700_palette, sizeof(cbm700_palette) / 3);
    memcpy(colortable,cbmb_colortable,sizeof(cbmb_colortable));
}

ROM_START (cbm610)
	ROM_REGION (0x100000, REGION_CPU1, 0)
	ROM_LOAD ("901243.04a", 0xf8000, 0x2000, CRC(b0dcb56d) SHA1(08d333208060ee2ce84d4532028d94f71c016b96))
	ROM_LOAD ("901242.04a", 0xfa000, 0x2000, CRC(de04ea4f) SHA1(7c6de17d46a3343dc597d9b9519cf63037b31908))
	ROM_LOAD ("901244.04a", 0xfe000, 0x2000, CRC(09a5667e) SHA1(abb26418b9e1614a8f52bdeee0822d4a96071439))
	ROM_REGION (0x2000, REGION_GFX1, 0)
    ROM_LOAD ("901237.01", 0x0000, 0x1000, CRC(1acf5098) SHA1(e63bf18da48e5a53c99ef127c1ae721333d1d102))
ROM_END

ROM_START (cbm620)
	ROM_REGION (0x100000, REGION_CPU1, 0)
    ROM_LOAD ("901241.03", 0xf8000, 0x2000, CRC(5c1f3347) SHA1(2d46be2cd89594b718cdd0a86d51b6f628343f42))
    ROM_LOAD ("901240.03", 0xfa000, 0x2000, CRC(72aa44e1) SHA1(0d7f77746290afba8d0abeb87c9caab9a3ad89ce))
    ROM_LOAD ("901244.04a", 0xfe000, 0x2000, CRC(09a5667e) SHA1(abb26418b9e1614a8f52bdeee0822d4a96071439))
	ROM_REGION (0x2000, REGION_GFX1, 0)
    ROM_LOAD ("901237.01", 0x0000, 0x1000, CRC(1acf5098) SHA1(e63bf18da48e5a53c99ef127c1ae721333d1d102))
ROM_END

ROM_START (cbm620hu)
	ROM_REGION (0x100000, REGION_CPU1, 0)
	ROM_LOAD ("610u60.bin", 0xf8000, 0x4000, CRC(8eed0d7e) SHA1(9d06c5c3c012204eaaef8b24b1801759b62bf57e))
	ROM_LOAD ("kernhun.bin", 0xfe000, 0x2000, CRC(0ea8ca4d) SHA1(9977c9f1136ee9c04963e0b50ae0c056efa5663f))
	ROM_REGION (0x2000, REGION_GFX1, 0)
	ROM_LOAD ("charhun.bin", 0x0000, 0x2000, CRC(1fb5e596) SHA1(3254e069f8691b30679b19a9505b6afdfedce6ac))
ROM_END

ROM_START (cbm710)
	ROM_REGION (0x100000, REGION_CPU1, 0)
	ROM_LOAD ("901243.04a", 0xf8000, 0x2000, CRC(b0dcb56d) SHA1(08d333208060ee2ce84d4532028d94f71c016b96))
	ROM_LOAD ("901242.04a", 0xfa000, 0x2000, CRC(de04ea4f) SHA1(7c6de17d46a3343dc597d9b9519cf63037b31908))
	ROM_LOAD ("901244.04a", 0xfe000, 0x2000, CRC(09a5667e) SHA1(abb26418b9e1614a8f52bdeee0822d4a96071439))
	ROM_REGION (0x2000, REGION_GFX1, 0)
    ROM_LOAD ("901232.01", 0x0000, 0x1000, CRC(3a350bc3) SHA1(e7f3cbc8e282f79a00c3e95d75c8d725ee3c6287))
ROM_END

ROM_START (cbm720)
	ROM_REGION (0x100000, REGION_CPU1, 0)
    ROM_LOAD ("901241.03", 0xf8000, 0x2000, CRC(5c1f3347) SHA1(2d46be2cd89594b718cdd0a86d51b6f628343f42))
    ROM_LOAD ("901240.03", 0xfa000, 0x2000, CRC(72aa44e1) SHA1(0d7f77746290afba8d0abeb87c9caab9a3ad89ce))
    ROM_LOAD ("901244.04a", 0xfe000, 0x2000, CRC(09a5667e) SHA1(abb26418b9e1614a8f52bdeee0822d4a96071439))
	ROM_REGION (0x2000, REGION_GFX1, 0)
    ROM_LOAD ("901232.01", 0x0000, 0x1000, CRC(3a350bc3) SHA1(e7f3cbc8e282f79a00c3e95d75c8d725ee3c6287))
ROM_END

ROM_START (cbm720se)
	ROM_REGION (0x100000, REGION_CPU1, 0)
    ROM_LOAD ("901241.03", 0xf8000, 0x2000, CRC(5c1f3347) SHA1(2d46be2cd89594b718cdd0a86d51b6f628343f42))
    ROM_LOAD ("901240.03", 0xfa000, 0x2000, CRC(72aa44e1) SHA1(0d7f77746290afba8d0abeb87c9caab9a3ad89ce))
    ROM_LOAD ("901244.03", 0xfe000, 0x2000, CRC(87bc142b) SHA1(fa711f6082741b05a9c80744f5aee68dc8c1dcf4))
	ROM_REGION (0x2000, REGION_GFX1, 0)
    ROM_LOAD ("901233.03", 0x0000, 0x1000, CRC(09518b19) SHA1(2e28491e31e2c0a3b6db388055216140a637cd09))
ROM_END


ROM_START (cbm500)
	ROM_REGION (0x101000, REGION_CPU1, 0)
	ROM_LOAD ("901236.02", 0xf8000, 0x2000, CRC(c62ab16f) SHA1(f50240407bade901144f7e9f489fa9c607834eca))
	ROM_LOAD ("901235.02", 0xfa000, 0x2000, CRC(20b7df33) SHA1(1b9a55f12f8cf025754d8029cc5324b474c35841))
	ROM_LOAD ("901234.02", 0xfe000, 0x2000, CRC(f46bbd2b) SHA1(097197d4d08e0b82e0466a5f1fbd49a24f3d2523))
	ROM_LOAD ("901225.01", 0x100000, 0x1000, CRC(ec4272ee) SHA1(adc7c31e18c7c7413d54802ef2f4193da14711aa))
ROM_END

#if 0
/* in c16 and some other commodore machines:
   cbm version in kernel at 0xff80 (offset 0x3f80)
   0x80 means pal version */

    /* scrap */
	 /* 0000 1fff --> 0000
                      inverted 2000
        2000 3fff --> 4000
                      inverted 6000 */

	 /* 128 kb basic version */
    ROM_LOAD ("b128-8000.901243-02b.bin", 0xf8000, 0x2000, CRC(9d0366f9))
    ROM_LOAD ("b128-a000.901242-02b.bin", 0xfa000, 0x2000, CRC(837978b5))
	 /* merged df83bbb9 */

    ROM_LOAD ("b128-8000.901243-04a.bin", 0xf8000, 0x2000, CRC(b0dcb56d) SHA1(08d333208060ee2ce84d4532028d94f71c016b96))
    ROM_LOAD ("b128-a000.901242-04a.bin", 0xfa000, 0x2000, CRC(de04ea4f) SHA1(7c6de17d46a3343dc597d9b9519cf63037b31908))
	 /* merged a8ff9372 */

	 /* some additions to 901242-04a */
    ROM_LOAD ("b128-a000.901242-04_3f.bin", 0xfa000, 0x2000, CRC(5a680d2a))

     /* 256 kbyte basic version */
    ROM_LOAD ("b256-8000.610u60.bin", 0xf8000, 0x4000, CRC(8eed0d7e) SHA1(9d06c5c3c012204eaaef8b24b1801759b62bf57e))

    ROM_LOAD ("b256-8000.901241-03.bin", 0xf8000, 0x2000, CRC(5c1f3347) SHA1(2d46be2cd89594b718cdd0a86d51b6f628343f42))
    ROM_LOAD ("b256-a000.901240-03.bin", 0xfa000, 0x2000, CRC(72aa44e1) SHA1(0d7f77746290afba8d0abeb87c9caab9a3ad89ce))
	 /* merged 5db15870 */

     /* monitor instead of tape */
    ROM_LOAD ("kernal.901244-03b.bin", 0xfe000, 0x2000, CRC(4276dbba))
     /* modified 03b for usage of vc1541 on tape port ??? */
    ROM_LOAD ("kernelnew", 0xfe000, 0x2000, CRC(19bf247e))
    ROM_LOAD ("kernal.901244-04a.bin", 0xfe000, 0x2000, CRC(09a5667e) SHA1(abb26418b9e1614a8f52bdeee0822d4a96071439))
    ROM_LOAD ("kernal.hungarian.bin", 0xfe000, 0x2000, CRC(0ea8ca4d) SHA1(9977c9f1136ee9c04963e0b50ae0c056efa5663f))


	 /* 600 8x16 chars for 8x8 size
        128 ascii, 128 ascii graphics
        inversion logic in hardware */
    ROM_LOAD ("characters.901237-01.bin", 0x0000, 0x1000, CRC(1acf5098) SHA1(e63bf18da48e5a53c99ef127c1ae721333d1d102))
	 /* packing 128 national, national graphics, ascii, ascii graphics */
    ROM_LOAD ("characters-hungarian.bin", 0x0000, 0x2000, CRC(1fb5e596) SHA1(3254e069f8691b30679b19a9505b6afdfedce6ac))
	 /* 700 8x16 chars for 9x14 size*/
    ROM_LOAD ("characters.901232-01.bin", 0x0000, 0x1000, CRC(3a350bc3) SHA1(e7f3cbc8e282f79a00c3e95d75c8d725ee3c6287))

    ROM_LOAD ("vt52emu.bin", 0xf4000, 0x2000, CRC(b3b6173a))
	 /* load address 0xf4000? */
    ROM_LOAD ("moni.bin", 0xfe000, 0x2000, CRC(43b08d1f))

    ROM_LOAD ("profitext.bin", 0xf2000, 0x2000, CRC(ac622a2b))
	 /* address ?*/
    ROM_LOAD ("sfd1001-copy-u59.bin", 0xf1000, 0x1000, CRC(1c0fd916))

    ROM_LOAD ("", 0xfe000, 0x2000, CRC())

	 /* 500 */
	 /* 128 basic, other colors than cbmb series basic */
    ROM_LOAD ("basic-lo.901236-02.bin", 0xf8000, 0x2000, CRC(c62ab16f) SHA1(f50240407bade901144f7e9f489fa9c607834eca))
    ROM_LOAD ("basic-hi.901235-02.bin", 0xfa000, 0x2000, CRC(20b7df33) SHA1(1b9a55f12f8cf025754d8029cc5324b474c35841))
     /* monitor instead of tape */
    ROM_LOAD ("kernal.901234-02.bin", 0xfe000, 0x2000, CRC(f46bbd2b) SHA1(097197d4d08e0b82e0466a5f1fbd49a24f3d2523))
    ROM_LOAD ("characters.901225-01.bin", 0x100000, 0x1000, CRC(ec4272ee) SHA1(adc7c31e18c7c7413d54802ef2f4193da14711aa))

#endif


static MACHINE_DRIVER_START( cbm600 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6509, 7833600)        /* 7.8336 Mhz */
	MDRV_CPU_PROGRAM_MAP(cbmb_readmem, cbmb_writemem)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(0)

	MDRV_MACHINE_RESET( cbmb )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 200)
	MDRV_SCREEN_VISIBLE_AREA(0, 640 - 1, 0, 200 - 1)
	MDRV_GFXDECODE( cbm600 )
	MDRV_PALETTE_LENGTH(sizeof (cbm700_palette) / sizeof (cbm700_palette[0]) / 3)
	MDRV_COLORTABLE_LENGTH(sizeof (cbmb_colortable) / sizeof(cbmb_colortable[0]))
	MDRV_PALETTE_INIT( cbm700 )

	MDRV_VIDEO_START( generic )
	MDRV_VIDEO_UPDATE( crtc6845 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SID6581, 1000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( cbm600pal )
	MDRV_IMPORT_FROM( cbm600 )
	MDRV_SCREEN_REFRESH_RATE(50)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( cbm700 )
	MDRV_IMPORT_FROM( cbm600pal )
	MDRV_SCREEN_SIZE(720, 350)
	MDRV_SCREEN_VISIBLE_AREA(0, 720 - 1, 0, 350 - 1)
	MDRV_GFXDECODE( cbm700 )

	MDRV_VIDEO_START( cbm700 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( cbm500 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6509, VIC6567_CLOCK)        /* 7.8336 Mhz */
	MDRV_CPU_PROGRAM_MAP(cbm500_readmem, cbm500_writemem)
	MDRV_CPU_PERIODIC_INT(vic2_raster_irq, VIC2_HRETRACERATE)
	MDRV_SCREEN_REFRESH_RATE(VIC6567_VRETRACERATE)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(0)

	MDRV_MACHINE_RESET( cbmb )

	MDRV_IMPORT_FROM( vh_vic2 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SID6581, 1000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

static DRIVER_INIT( cbm500 )	{ cbm500_driver_init(); }
static DRIVER_INIT( cbm600 )	{ cbm600_driver_init(); }
static DRIVER_INIT( cbm600hu )	{ cbm600hu_driver_init(); }
static DRIVER_INIT( cbm600pal )	{ cbm600pal_driver_init(); }
static DRIVER_INIT( cbm700 )	{ cbm700_driver_init(); }

static void cbmb_cbmcartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "crt,10,20,40,60"); break;

		default:										cbmcartslot_device_getinfo(devclass, state, info); break;
	}
}

static void cbmb_quickload_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "p00,prg"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_QUICKLOAD_LOAD:				info->f = (genf *) quickload_load_cbmb; break;

		/* --- the following bits of info are returned as doubles --- */
		case DEVINFO_FLOAT_QUICKLOAD_DELAY:				info->d = CBM_QUICKLOAD_DELAY; break;

		default:										quickload_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(cbmb)
	CONFIG_DEVICE(cbmb_cbmcartslot_getinfo)
	CONFIG_DEVICE(cbmb_quickload_getinfo)
#ifdef PET_TEST_CODE
	/* monitor OR tape routine in kernel */
	CONFIG_DEVICE(cbmfloppy_device_getinfo)
#endif
SYSTEM_CONFIG_END

static void cbm500_quickload_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "p00,prg"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_QUICKLOAD_LOAD:				info->f = (genf *) quickload_load_cbm500; break;

		/* --- the following bits of info are returned as doubles --- */
		case DEVINFO_FLOAT_QUICKLOAD_DELAY:				info->d = CBM_QUICKLOAD_DELAY; break;

		default:										quickload_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(cbm500)
	CONFIG_DEVICE(cbmb_cbmcartslot_getinfo)
	CONFIG_DEVICE(cbm500_quickload_getinfo)
#ifdef PET_TEST_CODE
	/* monitor OR tape routine in kernel */
	CONFIG_DEVICE(cbmfloppy_device_getinfo)
#endif
SYSTEM_CONFIG_END

/*     YEAR     NAME      PARENT    COMPAT  MACHINE     INPUT       INIT        CONFIG  COMPANY                             FULLNAME */
COMP (1983,	cbm500,	  0,		0,		cbm500,		cbm500,		cbm500,		cbm500,	"Commodore Business Machines Co.",	"Commodore B128-40/Pet-II/P500 60Hz",		GAME_NOT_WORKING)
COMP (1983,	cbm610,   0,		0,		cbm600, 	cbm600, 	cbm600, 	cbmb,	"Commodore Business Machines Co.",  "Commodore B128-80LP/610 60Hz",             GAME_NOT_WORKING)
COMP (1983,	cbm620,	  cbm610,	0,		cbm600pal,	cbm600pal,	cbm600pal,	cbmb,	"Commodore Business Machines Co.",	"Commodore B256-80LP/620 50Hz",	GAME_NOT_WORKING)
COMP (1983,	cbm620hu, cbm610,	0,		cbm600pal,	cbm600pal,	cbm600hu,	cbmb,	"Commodore Business Machines Co.",	"Commodore B256-80LP/620 Hungarian 50Hz",	GAME_NOT_WORKING)
COMP (1983,	cbm710,   cbm610,   0,		cbm700, 	cbm700, 	cbm700, 	cbmb,	"Commodore Business Machines Co.",  "Commodore B128-80HP/710",                  GAME_NOT_WORKING)
COMP (1983,	cbm720,	  cbm610,	0,		cbm700,		cbm700,		cbm700,		cbmb,	"Commodore Business Machines Co.",	"Commodore B256-80HP/720",					GAME_NOT_WORKING)
COMP (1983,	cbm720se, cbm610,	0,		cbm700,     cbm700,		cbm700,		cbmb,	"Commodore Business Machines Co.",	"Commodore B256-80HP/720 Swedish/Finnish",	GAME_NOT_WORKING)
#if 0
COMP (1983,	cbm730,   cbm610,   0,		cbmbx, 		cbmb, 		cbmb, 		cbmb,    "Commodore Business Machines Co.",	"Commodore BX128-80HP/BX256-80HP/730", GAME_NOT_WORKING)
#endif
