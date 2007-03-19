/***************************************************************************
	commodore pet series computer

	PeT mess@utanet.at

	documentation
	 vice emulator
     www.funet.fi
	 andre fachat (vice emulator, docu, web site, excellent keyboard pictures)

***************************************************************************/

/*
PET 2000 Series:Renamed to CBM 20XX, XX = RAM, when Philips forbid PET use.
                Most CBM renamed units powered up in lowercase and had a
                different keyboard config, while the PET machines booted in
                uppercase. B and N notation alternately put after RAM amount
                in name (PET 2001B-32 = PET 2001-32B)
                Black (B) or Blue (N) Trim, 9" (9) or 12" (2) screen,
                Built-In Cassette with Chiclet Keys (C),
                Business Style Keyboard with No Graphics on Keys (K), or
                Home Computer with Number Keys and Graphics on Keys (H),
                Green/White screen (G) or Black/White screen (W)
* PET 2001-4K   4kB, CB                                                     GP
* PET 2001-8K   8kB, CN9                                                    GP
* PET 2001-8C   8kB, CN9W, SN#0620733, No "WAIT 6502,X"                     GL
* PET 2001-8C   8kB, CB9G, SN#0629836, No "WAIT 6502,X"                     GL
  PET 2001-16K  16kB, CN9
  PET 2001-32K  32kB, CN9
  PET 2001B-8   8kB, K2
  PET 2001B-16  16kB, K2
  PET 2001B-32  32kB, BK9W, boots in lowercase                              RB
  PET 2001B-32  32kB, K2
  PET 2001N-8   8kB, H2
* PET 2001N-16  16kB, H9                                                    CH
  PET 2001N-16  16kB, H2
* PET 2001N-32  32kB, H, BASIC 4.0,                                         CS
* PET 2001NT    Teacher's PET.  Same as 2001N, just rebadged
* MDS 6500      Modified 2001N-32 with matching 2040 drive.  500 made.      GP

CBM 3000 Series: 40 Col. Screen, BASIC 2.0-2.3, Same Board as Thin 4000
                 3001 series in Germany were just 2001's with big Keyboard.
* CBM 3008      8kB, 9" Screen.                                             EG
* CBM 3016      16kB
* CBM 3032      32kB.                                                       SL

CBM 4000 Thin Series: 9" Screen, 40 Column Only, Basic 4.0.
CBM 4000 Fat Series:  12" Screen, Upgradeable to 80 Column, When upgraded
                      to 80 Columns, the systems were 8000's.
  CBM 4004      4kB, One Piece.
* CBM 4008      8kB, One Piece.                                             SF
* CBM 4016      16kB, One Piece.                                            KK
* CBM 4032      32kB, One Piece                                             JB
* CBM 4064      Educator 64 in 40XX case. green screen (no Fat option)      GP
CBM 8000 Series:12" Screen, 80 Column, BASIC 4.0
                SK means "SoftKey", or "Separated Keyboard"  All -SK and d
                units were enclosed in CBM 700/B series HP cases.
  CBM 8008      8kB, One Piece
  CBM 8016      16kB, One Piece
* CBM 8032      32kB, One Piece                                             GP
* CBM 8032-32 B 8032 in Higher Profile case (HP).  Could install LP drives. GP
* CBM 8032 SK   32kB, Detached Keyboard, SK = SoftKey or Separated Keyboard.EG
  CBM 8096      96kB, 8032 with 64kB ram card
* CBM 8096 SK   96kB, Detached Keyboard.
* CBM 8096d     8096 + 8250LP                                               SL
* CBM 8296      128kB, Detached Keyboard, Brown like 64, LOS-96 OS          TL
* CBM 8296d     8296 + 8250LP                                               SL
* "CASSIE"      Synergistics Inc. rebadged 8032                             AH

SuperPet Series:Sold in Germany as MMF (MicroMainFrame) 9000
                Machines sold in Italy had 134kB of RAM.
* CBM SP9000    Dual uP 6502/6809, 96kB RAM, business keyboard.             GP

CBM 200 Series
* CBM 200       CBM 8032 SK                                                 VM
  CBM 210       ???
* CBM 220       CBM 8096 SK



basically 3 types of motherboards
no crtc (only 60 hz?)
crtc 40 columns ( 50 or 60 hz )
crtc 80 columns ( 60 or 60 hz )
(board version able to do 40 and 80 columns)

3 types of basic roms
basic 1 (only 40 columns, no crtc, with graphics)
basic 2 (only 40 columns version, no crtc)
basic 4

2 types of keyboard and roms
normal (with graphic) (80 columns versions only by 3 parties)
business
different mapping/system roms!

state
-----
keyboard
no sound (were available)
no tape drives
no ieee488 interface
 no floppy disk support
quickloader

Keys
----
Some PC-Keyboards does not behave well when special two or more keys are
pressed at the same time
(with my keyboard printscreen clears the pressed pause key!)

when problems start with -log and look into error.log file
 */

#include "driver.h"

#define VERBOSE_DBG 0
#include "includes/cbm.h"
#include "machine/6821pia.h"
#include "machine/6522via.h"
#include "video/generic.h"
#include "includes/pet.h"
#include "includes/crtc6845.h"
#include "includes/cbmserb.h"
#include "includes/cbmieeeb.h"
/*#include "includes/vc1541.h" */

static ADDRESS_MAP_START(pet_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x8000, 0x83ff) AM_RAM AM_BASE(&videoram) AM_SIZE(&videoram_size )
	AM_RANGE(0xa000, 0xe7ff) AM_ROM
	AM_RANGE(0xe810, 0xe813) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE(0xe820, 0xe823) AM_READWRITE(pia_1_r, pia_1_w)
	AM_RANGE(0xe840, 0xe84f) AM_READWRITE(via_0_r, via_0_w)
/*	{0xe900, 0xe91f, cbm_ieee_state }, // for debugging */
	AM_RANGE(0xf000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( pet40_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x8000, 0x83ff) AM_RAM AM_BASE(&videoram) AM_SIZE(&videoram_size )
	AM_RANGE(0xa000, 0xe7ff) AM_ROM
	AM_RANGE(0xe810, 0xe813) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE(0xe820, 0xe823) AM_READWRITE(pia_1_r, pia_1_w)
	AM_RANGE(0xe840, 0xe84f) AM_READWRITE(via_0_r, via_0_w)
	AM_RANGE(0xe880, 0xe881) AM_READWRITE(crtc6845_0_port_r, crtc6845_0_port_w)
	AM_RANGE(0xf000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( pet80_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x8000, 0x8fff) AM_RAMBANK(1)
	AM_RANGE(0x9000, 0x9fff) AM_RAMBANK(2)
	AM_RANGE(0xa000, 0xafff) AM_RAMBANK(3)
	AM_RANGE(0xb000, 0xbfff) AM_RAMBANK(4)
	AM_RANGE(0xc000, 0xe7ff) AM_RAMBANK(6)
#if 1
	AM_RANGE(0xe800, 0xefff) AM_RAMBANK(7)
#else
	AM_RANGE(0xe810, 0xe813) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE(0xe820, 0xe823) AM_READWRITE(pia_1_r, pia_1_w)
	AM_RANGE(0xe840, 0xe84f) AM_READWRITE(via_0_r, via_0_w)
	AM_RANGE(0xe880, 0xe881) AM_READWRITE(crtc6845_0_port_r, crtc6845_0_port_w)
#endif
	AM_RANGE(0xf000, 0xffff) AM_READ(MRA8_BANK8)
	AM_RANGE(0xf000, 0xffef) AM_WRITE(MWA8_BANK8)
	AM_RANGE(0xfff1, 0xffff) AM_WRITE(MWA8_BANK9)
ADDRESS_MAP_END


/* 0xe880 crtc
   0xefe0 6702 encoder
   0xeff0 acia6551

   0xeff8 super pet system latch
61432        SuperPET system latch
        bit 0    1=6502, 0=6809
        bit 1    0=read only
        bit 3    diagnostic sense: set to 1 to switch to 6502

61436        SuperPET bank select latch
        bit 0-3  bank
        bit 7    1=enable system latch

*/
static ADDRESS_MAP_START( superpet_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x7fff) AM_RAM AM_SHARE(1) AM_BASE(&pet_memory)
	AM_RANGE(0x8000, 0x87ff) AM_RAM AM_SHARE(2) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0xa000, 0xe7ff) AM_ROM
	AM_RANGE(0xe810, 0xe813) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE(0xe820, 0xe823) AM_READWRITE(pia_1_r, pia_1_w)
	AM_RANGE(0xe840, 0xe84f) AM_READWRITE(via_0_r, via_0_w)
	AM_RANGE(0xe880, 0xe881) AM_READWRITE(crtc6845_0_port_r, crtc6845_0_port_w)
	/* 0xefe0, 0xefe3, mos 6702 */
	/* 0xeff0, 0xeff3, acia6551 */
	AM_RANGE(0xeff8, 0xefff) AM_READWRITE(superpet_r, superpet_w)
	AM_RANGE(0xf000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( superpet_m6809_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x7fff) AM_RAM AM_SHARE(1)	/* same memory as m6502 */
	AM_RANGE(0x8000, 0x87ff) AM_RAM AM_SHARE(2)	/* same memory as m6502 */
    AM_RANGE(0x9000, 0x9fff) AM_RAMBANK(1)	/* 64 kbyte ram turned in */
	AM_RANGE(0xa000, 0xe7ff) AM_ROM
	AM_RANGE(0xe810, 0xe813) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE(0xe820, 0xe823) AM_READWRITE(pia_1_r, pia_1_w)
	AM_RANGE(0xe840, 0xe84f) AM_READWRITE(via_0_r, via_0_w)
	AM_RANGE(0xe880, 0xe881) AM_READWRITE(crtc6845_0_port_r, crtc6845_0_port_w)
	AM_RANGE(0xeff8, 0xefff) AM_READWRITE(superpet_r, superpet_w)
	AM_RANGE(0xf000, 0xffff) AM_ROM
ADDRESS_MAP_END

#define DIPS_HELPER(bit, name, keycode) \
   PORT_BIT(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(name) PORT_CODE(keycode)

#define PET_KEYBOARD \
	PORT_START \
	DIPS_HELPER( 0x8000, "At", KEYCODE_TILDE)\
	DIPS_HELPER( 0x4000, "!", KEYCODE_1)\
	DIPS_HELPER( 0x2000, "\"", KEYCODE_2)\
	DIPS_HELPER( 0x1000, "#", KEYCODE_3)\
	DIPS_HELPER( 0x0800, "$", KEYCODE_4)\
	DIPS_HELPER( 0x0400, "%", KEYCODE_5)\
	DIPS_HELPER( 0x0200, "'", KEYCODE_6)\
	DIPS_HELPER( 0x0100, "&", KEYCODE_7)\
	DIPS_HELPER( 0x0080, "\\", KEYCODE_8)\
	DIPS_HELPER( 0x0040, "(", KEYCODE_9)\
	DIPS_HELPER( 0x0020, ")", KEYCODE_0)\
	DIPS_HELPER( 0x0010, "Arrow-Left", KEYCODE_MINUS)\
	DIPS_HELPER( 0x0008, "[", KEYCODE_EQUALS)\
	DIPS_HELPER( 0x0004, "]", KEYCODE_BACKSPACE)\
	DIPS_HELPER( 0x0002, "RVS OFF", KEYCODE_TAB)\
	DIPS_HELPER( 0x0001, "Q", KEYCODE_Q)\
	PORT_START \
	DIPS_HELPER( 0x8000, "W", KEYCODE_W)\
	DIPS_HELPER( 0x4000, "E", KEYCODE_E)\
	DIPS_HELPER( 0x2000, "R", KEYCODE_R)\
	DIPS_HELPER( 0x1000, "T", KEYCODE_T)\
	DIPS_HELPER( 0x0800, "Y", KEYCODE_Y)\
	DIPS_HELPER( 0x0400, "U", KEYCODE_U)\
	DIPS_HELPER( 0x0200, "I", KEYCODE_I)\
	DIPS_HELPER( 0x0100, "O", KEYCODE_O)\
	DIPS_HELPER( 0x0080, "P", KEYCODE_P)\
	DIPS_HELPER( 0x0040, "Arrow-Up Pi", KEYCODE_OPENBRACE)\
    DIPS_HELPER( 0x0020, "<", KEYCODE_CLOSEBRACE)\
    DIPS_HELPER( 0x0010, ">", KEYCODE_BACKSLASH)\
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(left shift)SHIFT-LOCK (switch)") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE\
	DIPS_HELPER( 0x0004, "A", KEYCODE_A)\
	DIPS_HELPER( 0x0002, "S", KEYCODE_S)\
	DIPS_HELPER( 0x0001, "D", KEYCODE_D)\
	PORT_START \
	DIPS_HELPER( 0x8000, "F", KEYCODE_F)\
	DIPS_HELPER( 0x4000, "G", KEYCODE_G)\
	DIPS_HELPER( 0x2000, "H", KEYCODE_H)\
	DIPS_HELPER( 0x1000, "J", KEYCODE_J)\
	DIPS_HELPER( 0x0800, "K", KEYCODE_K)\
	DIPS_HELPER( 0x0400, "L", KEYCODE_L)\
	DIPS_HELPER( 0x0200, ":", KEYCODE_COLON)\
	DIPS_HELPER( 0x0100, "STOP RUN", KEYCODE_QUOTE)\
    DIPS_HELPER( 0x0080, "RETURN",KEYCODE_ENTER)\
	DIPS_HELPER( 0x0040, "Left-Shift", KEYCODE_LSHIFT)\
	DIPS_HELPER( 0x0020, "Z", KEYCODE_Z)\
	DIPS_HELPER( 0x0010, "X", KEYCODE_X)\
	DIPS_HELPER( 0x0008, "C", KEYCODE_C)\
	DIPS_HELPER( 0x0004, "V", KEYCODE_V)\
	DIPS_HELPER( 0x0002, "B", KEYCODE_B)\
	DIPS_HELPER( 0x0001, "N", KEYCODE_N)\
	PORT_START \
	DIPS_HELPER( 0x8000, "M", KEYCODE_M)\
	DIPS_HELPER( 0x4000, ",", KEYCODE_COMMA)\
	DIPS_HELPER( 0x2000, ";", KEYCODE_STOP)\
	DIPS_HELPER( 0x1000, "?", KEYCODE_SLASH)\
	DIPS_HELPER( 0x0800, "Right-Shift", KEYCODE_RSHIFT)\
	DIPS_HELPER( 0x0400, "Space", KEYCODE_SPACE)\
	DIPS_HELPER( 0x0200, "HOME CLR", KEYCODE_HOME)\
	DIPS_HELPER( 0x0100, "DOWN UP", KEYCODE_DOWN)\
	DIPS_HELPER( 0x0080, "RIGHT LEFT", KEYCODE_RIGHT)\
	DIPS_HELPER( 0x0040, "DEL INST", KEYCODE_DEL)\
	DIPS_HELPER( 0x0020, "NUM 7", KEYCODE_7_PAD)\
	DIPS_HELPER( 0x0010, "NUM 8", KEYCODE_8_PAD)\
	DIPS_HELPER( 0x0008, "NUM 9", KEYCODE_9_PAD)\
	DIPS_HELPER( 0x0004, "NUM /", KEYCODE_SLASH_PAD)\
	DIPS_HELPER( 0x0002, "NUM 4", KEYCODE_4_PAD)\
	DIPS_HELPER( 0x0001, "NUM 5", KEYCODE_5_PAD)\
	PORT_START \
	DIPS_HELPER( 0x8000, "NUM 6", KEYCODE_6_PAD)\
	DIPS_HELPER( 0x4000, "NUM *", KEYCODE_ASTERISK)\
	DIPS_HELPER( 0x2000, "NUM 1", KEYCODE_1_PAD)\
	DIPS_HELPER( 0x1000, "NUM 2", KEYCODE_2_PAD)\
	DIPS_HELPER( 0x0800, "NUM 3", KEYCODE_3_PAD)\
	DIPS_HELPER( 0x0400, "NUM +", KEYCODE_PLUS_PAD)\
	DIPS_HELPER( 0x0200, "NUM 0", KEYCODE_0_PAD)\
	DIPS_HELPER( 0x0100, "NUM .", KEYCODE_DEL_PAD)\
	DIPS_HELPER( 0x0080, "NUM -", KEYCODE_MINUS_PAD)\
	DIPS_HELPER( 0x0040, "NUM =", KEYCODE_ENTER_PAD)\
	DIPS_HELPER( 0x0020, "(right-shift cursor-down)Special CRSR Up", KEYCODE_UP)\
	DIPS_HELPER( 0x0010, "(right-shift cursor-right)Special CRSR Left", KEYCODE_LEFT)\

#define PET_B_KEYBOARD \
	PORT_START \
	DIPS_HELPER( 0x8000, "Arrow-Left", KEYCODE_TILDE)\
	DIPS_HELPER( 0x4000, "1 !", KEYCODE_1)\
	DIPS_HELPER( 0x2000, "2 \"", KEYCODE_2)\
	DIPS_HELPER( 0x1000, "3 #", KEYCODE_3)\
	DIPS_HELPER( 0x0800, "4 $", KEYCODE_4)\
	DIPS_HELPER( 0x0400, "5 %", KEYCODE_5)\
	DIPS_HELPER( 0x0200, "6 &", KEYCODE_6)\
	DIPS_HELPER( 0x0100, "7 '", KEYCODE_7)\
	DIPS_HELPER( 0x0080, "8 (", KEYCODE_8)\
	DIPS_HELPER( 0x0040, "9 )", KEYCODE_9)\
	DIPS_HELPER( 0x0020, "0", KEYCODE_0)\
	DIPS_HELPER( 0x0010, ": *", KEYCODE_MINUS)\
	DIPS_HELPER( 0x0008, "- =", KEYCODE_EQUALS)\
	DIPS_HELPER( 0x0004, "Arrow-Up", KEYCODE_BACKSPACE)\
	DIPS_HELPER( 0x0002, "CRSR RIGHT LEFT", KEYCODE_RIGHT)\
	DIPS_HELPER( 0x0001, "STOP RUN", KEYCODE_END)\
	PORT_START \
	DIPS_HELPER( 0x8000, "TAB", KEYCODE_TAB)\
	DIPS_HELPER( 0x4000, "Q", KEYCODE_Q)\
	DIPS_HELPER( 0x2000, "W", KEYCODE_W)\
	DIPS_HELPER( 0x1000, "E", KEYCODE_E)\
	DIPS_HELPER( 0x0800, "R", KEYCODE_R)\
	DIPS_HELPER( 0x0400, "T", KEYCODE_T)\
	DIPS_HELPER( 0x0200, "Y", KEYCODE_Y)\
	DIPS_HELPER( 0x0100, "U", KEYCODE_U)\
	DIPS_HELPER( 0x0080, "I", KEYCODE_I)\
	DIPS_HELPER( 0x0040, "O", KEYCODE_O)\
	DIPS_HELPER( 0x0020, "P", KEYCODE_P)\
	DIPS_HELPER( 0x0010, "[", KEYCODE_OPENBRACE)\
    DIPS_HELPER( 0x0008, "\\", KEYCODE_CLOSEBRACE)\
    DIPS_HELPER( 0x0004, "CRSR DOWN UP", KEYCODE_DOWN)\
    DIPS_HELPER( 0x0002, "DEL INST", KEYCODE_DEL)\
    DIPS_HELPER( 0x0001, "ESC",KEYCODE_ESC)\
	PORT_START \
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(left shift)SHIFT-LOCK (switch)") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE\
	DIPS_HELPER( 0x4000, "A", KEYCODE_A)\
	DIPS_HELPER( 0x2000, "S", KEYCODE_S)\
	DIPS_HELPER( 0x1000, "D", KEYCODE_D)\
	DIPS_HELPER( 0x0800, "F", KEYCODE_F)\
	DIPS_HELPER( 0x0400, "G", KEYCODE_G)\
	DIPS_HELPER( 0x0200, "H", KEYCODE_H)\
	DIPS_HELPER( 0x0100, "J", KEYCODE_J)\
	DIPS_HELPER( 0x0080, "K", KEYCODE_K)\
	DIPS_HELPER( 0x0040, "L", KEYCODE_L)\
	DIPS_HELPER( 0x0020, "; +", KEYCODE_COLON)\
	DIPS_HELPER( 0x0010, "At", KEYCODE_QUOTE)\
	DIPS_HELPER( 0x0008, "]", KEYCODE_BACKSLASH)\
    DIPS_HELPER( 0x0004, "RETURN",KEYCODE_ENTER)\
    DIPS_HELPER( 0x0002, "RVS Off",KEYCODE_INSERT)\
	DIPS_HELPER( 0x0001, "Left-Shift", KEYCODE_LSHIFT)\
	PORT_START \
	DIPS_HELPER( 0x8000, "Z", KEYCODE_Z)\
	DIPS_HELPER( 0x4000, "X", KEYCODE_X)\
	DIPS_HELPER( 0x2000, "C", KEYCODE_C)\
	DIPS_HELPER( 0x1000, "V", KEYCODE_V)\
	DIPS_HELPER( 0x0800, "B", KEYCODE_B)\
	DIPS_HELPER( 0x0400, "N", KEYCODE_N)\
	DIPS_HELPER( 0x0200, "M", KEYCODE_M)\
	DIPS_HELPER( 0x0100, ", <", KEYCODE_COMMA)\
	DIPS_HELPER( 0x0080, ". >", KEYCODE_STOP)\
	DIPS_HELPER( 0x0040, "/ ?", KEYCODE_SLASH)\
	DIPS_HELPER( 0x0020, "Right-Shift", KEYCODE_RSHIFT)\
	DIPS_HELPER( 0x0010, "REPEAT", KEYCODE_LALT)\
	DIPS_HELPER( 0x0008, "HOME CLR", KEYCODE_HOME)\
	DIPS_HELPER( 0x0004, "Space", KEYCODE_SPACE)\
	DIPS_HELPER( 0x0002, "NUM 7", KEYCODE_7_PAD)\
	DIPS_HELPER( 0x0001, "NUM 8", KEYCODE_8_PAD)\
	PORT_START \
	DIPS_HELPER( 0x8000, "NUM 9", KEYCODE_9_PAD)\
	DIPS_HELPER( 0x4000, "NUM 4", KEYCODE_4_PAD)\
	DIPS_HELPER( 0x2000, "NUM 5", KEYCODE_5_PAD)\
	DIPS_HELPER( 0x1000, "NUM 6", KEYCODE_6_PAD)\
	DIPS_HELPER( 0x0800, "NUM 1", KEYCODE_1_PAD)\
	DIPS_HELPER( 0x0400, "NUM 2", KEYCODE_2_PAD)\
	DIPS_HELPER( 0x0200, "NUM 3", KEYCODE_3_PAD)\
	DIPS_HELPER( 0x0100, "NUM 0", KEYCODE_0_PAD)\
	DIPS_HELPER( 0x0080, "NUM .", KEYCODE_DEL_PAD)\
	DIPS_HELPER( 0x0040, "(right-shift cursor-down)Special CRSR Up", KEYCODE_UP)\
	DIPS_HELPER( 0x0020, "(right-shift cursor-right)Special CRSR Left", KEYCODE_LEFT)\

INPUT_PORTS_START (pet)
	PET_KEYBOARD
    PORT_START
    DIPS_HELPER( 0x8000, "Quickload", KEYCODE_F8)
#ifdef PET_TEST_CODE
	PORT_DIPNAME   ( 0x4000, 0x4000, "Tape Drive/Device 1")
	PORT_DIPSETTING(  0, DEF_STR( Off ) )
	PORT_DIPSETTING(0x4000, DEF_STR( On ) )
	PORT_DIPNAME   ( 0x2000, 0x00, " Tape Sound")
	PORT_DIPSETTING(  0, DEF_STR( Off ) )
	PORT_DIPSETTING(0x2000, DEF_STR( On ) )
	DIPS_HELPER( 0x1000, "Tape Drive Play",       KEYCODE_F5)
	DIPS_HELPER( 0x0800, "Tape Drive Record",     KEYCODE_F6)
	DIPS_HELPER( 0x0400, "Tape Drive Stop",       KEYCODE_F7)
#endif
	PORT_BIT (0x200, 0x000, IPT_UNUSED) /* normal keyboard/bios */
	PORT_DIPNAME   ( 0x180, 0x180, "Memory")
	PORT_DIPSETTING(  0, "4KByte" )
	PORT_DIPSETTING(  0x80, "8KByte" )
	PORT_DIPSETTING(  0x100, "16KByte" )
	PORT_DIPSETTING(  0x180, "32KByte" )
	PORT_BIT (8, 0, IPT_UNUSED) /* no 8096 hardware */
	PORT_BIT (4, 0, IPT_UNUSED) /* no superpet switch */
	PORT_DIPNAME ( 0x02, 0x02, "IEEE488 Bus/Dev 8/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(0x02, DEF_STR( Yes ) )
	PORT_DIPNAME ( 0x01, 0x00, "IEEE488 Bus/Dev 9/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(  1, DEF_STR( Yes ) )
INPUT_PORTS_END

INPUT_PORTS_START (petb)
	PET_B_KEYBOARD
    PORT_START
    DIPS_HELPER( 0x8000, "Quickload", KEYCODE_F8)
#ifdef PET_TEST_CODE
	PORT_DIPNAME   ( 0x4000, 0x4000, "Tape Drive/Device 1")
	PORT_DIPSETTING(  0, DEF_STR( Off ) )
	PORT_DIPSETTING(0x4000, DEF_STR( On ) )
	PORT_DIPNAME   ( 0x2000, 0x00, " Tape Sound")
	PORT_DIPSETTING(  0, DEF_STR( Off ) )
	PORT_DIPSETTING(0x2000, DEF_STR( On ) )
	DIPS_HELPER( 0x1000, "Tape Drive Play",       KEYCODE_F5)
	DIPS_HELPER( 0x0800, "Tape Drive Record",     KEYCODE_F6)
	DIPS_HELPER( 0x0400, "Tape Drive Stop",       KEYCODE_F7)
#endif
	PORT_BIT (0x200, 0x200, IPT_UNUSED) /* business keyboard/bios */
	PORT_DIPNAME   ( 0x180, 0x180, "Memory")
	PORT_DIPSETTING(  0, "4KByte" )
	PORT_DIPSETTING(  0x80, "8KByte" )
	PORT_DIPSETTING(  0x100, "16KByte" )
	PORT_DIPSETTING(  0x180, "32KByte" )
	PORT_BIT (8, 0, IPT_UNUSED) /* no 8096 hardware */
	PORT_BIT (4, 0, IPT_UNUSED) /* no superpet switch */
	PORT_DIPNAME ( 0x02, 0x02, "IEEE488 Bus/Dev 8/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(0x02, DEF_STR( Yes ) )
	PORT_DIPNAME ( 0x01, 0x00, "IEEE488 Bus/Dev 9/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(  1, DEF_STR( Yes ) )
INPUT_PORTS_END

INPUT_PORTS_START (cbm8096)
	PET_B_KEYBOARD
    PORT_START
    DIPS_HELPER( 0x8000, "Quickload", KEYCODE_F8)
#ifdef PET_TEST_CODE
	PORT_DIPNAME   ( 0x4000, 0x4000, "Tape Drive/Device 1")
	PORT_DIPSETTING(  0, DEF_STR( Off ) )
	PORT_DIPSETTING(0x4000, DEF_STR( On ) )
	PORT_DIPNAME   ( 0x2000, 0x00, " Tape Sound")
	PORT_DIPSETTING(  0, DEF_STR( Off ) )
	PORT_DIPSETTING(0x2000, DEF_STR( On ) )
	DIPS_HELPER( 0x1000, "Tape Drive Play",       KEYCODE_F5)
	DIPS_HELPER( 0x0800, "Tape Drive Record",     KEYCODE_F6)
	DIPS_HELPER( 0x0400, "Tape Drive Stop",       KEYCODE_F7)
#endif
	PORT_BIT (0x200, 0x200, IPT_UNUSED) /* business keyboard/bios */
	PORT_BIT (0x180, 0x180, IPT_UNUSED) /* 32 kb Memory */
	PORT_DIPNAME ( 0x08, 0x08, "CBM8096, 8296 Expansion Memory")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(0x08, DEF_STR( Yes ) )
	PORT_BIT (4, 0, IPT_UNUSED) /* no superpet switch */
	PORT_DIPNAME ( 0x02, 0x02, "IEEE488 Bus/Dev 8/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(0x02, DEF_STR( Yes ) )
	PORT_DIPNAME ( 0x01, 0x00, "IEEE488 Bus/Dev 9/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(  1, DEF_STR( Yes ) )
INPUT_PORTS_END

INPUT_PORTS_START (superpet)
	PET_B_KEYBOARD
    PORT_START
    DIPS_HELPER( 0x8000, "Quickload", KEYCODE_F8)
#ifdef PET_TEST_CODE
	PORT_DIPNAME   ( 0x4000, 0x4000, "Tape Drive/Device 1")
	PORT_DIPSETTING(  0, DEF_STR( Off ) )
	PORT_DIPSETTING(0x4000, DEF_STR( On ) )
	PORT_DIPNAME   ( 0x2000, 0x00, " Tape Sound")
	PORT_DIPSETTING(  0, DEF_STR( Off ) )
	PORT_DIPSETTING(0x2000, DEF_STR( On ) )
	DIPS_HELPER( 0x1000, "Tape Drive Play",       KEYCODE_F5)
	DIPS_HELPER( 0x0800, "Tape Drive Record",     KEYCODE_F6)
	DIPS_HELPER( 0x0400, "Tape Drive Stop",       KEYCODE_F7)
#endif
	PORT_BIT (0x200, 0x200, IPT_UNUSED) /* business keyboard/bios */
	PORT_BIT (0x180, 0x180, IPT_UNUSED) /* 32KByte */
	PORT_BIT (8, 0, IPT_UNUSED) /* no 8096 hardware */
	PORT_DIPNAME   ( 4, 4, "CPU Select")
	PORT_DIPSETTING( 0, "M6502" )
	PORT_DIPSETTING( 4, "M6809" )
	PORT_DIPNAME ( 0x02, 0x02, "IEEE488 Bus/Dev 8/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(0x02, DEF_STR( Yes ) )
	PORT_DIPNAME ( 0x01, 0x00, "IEEE488 Bus/Dev 9/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(  1, DEF_STR( Yes ) )
INPUT_PORTS_END

unsigned char pet_palette[] =
{
	0,0,0, /* black */
	0,0x80,0, /* green */
};

static unsigned short pet_colortable[][2] = {
	{ 0, 1 },
	/* reverse */
	{ 1, 0 }
};

static gfx_layout pet_charlayout =
{
	8,8,
	256,                                    /* 256 characters */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0,1,2,3,4,5,6,7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	},
	8*8
};

static gfx_layout pet80_charlayout =
{
	8,16,
	256,                                    /* 256 characters */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0,1,2,3,4,5,6,7 },
	/* y offsets */
	{
		0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8
	},
	8*16
};

static gfx_decode pet_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &pet_charlayout,                     0, 1 },
	{ REGION_GFX1, 0x0800, &pet_charlayout,                     0, 1 },
    { -1 } /* end of array */
};

static gfx_decode pet80_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &pet80_charlayout,                     0, 1 },
	{ REGION_GFX1, 0x1000, &pet80_charlayout,                     0, 1 },
    { -1 } /* end of array */
};

static gfx_decode superpet_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &pet80_charlayout,                     0, 1 },
	{ REGION_GFX1, 0x1000, &pet80_charlayout,                     0, 1 },
	{ REGION_GFX1, 0x2000, &pet80_charlayout,                     0, 1 },
	{ REGION_GFX1, 0x3000, &pet80_charlayout,                     0, 1 },
    { -1 } /* end of array */
};

static PALETTE_INIT( pet )
{
	palette_set_colors(machine, 0, pet_palette, sizeof(pet_palette) / 3);
    memcpy(colortable, pet_colortable, sizeof(colortable));
}

/* basic 1 */
ROM_START (pet)
	ROM_REGION (0x10000, REGION_CPU1, 0)
    ROM_LOAD ("901447.09", 0xc000, 0x800, CRC(03cf16d0) SHA1(1330580c0614d3556a389da4649488ba04a60908))
    ROM_LOAD ("901447.02", 0xc800, 0x800, CRC(69fd8a8f) SHA1(70c0f4fa67a70995b168668c957c3fcf2c8641bd))
    ROM_LOAD ("901447.03", 0xd000, 0x800, CRC(d349f2d4) SHA1(4bf2c20c51a63d213886957485ebef336bb803d0))
    ROM_LOAD ("901447.04", 0xd800, 0x800, CRC(850544eb) SHA1(d293972d529023d8fd1f493149e4777b5c253a69))
    ROM_LOAD ("901447.05", 0xe000, 0x800, CRC(9e1c5cea) SHA1(f02f5fb492ba93dbbd390f24c10f7a832dec432a))
    ROM_LOAD ("901447.06", 0xf000, 0x800, CRC(661a814a) SHA1(960717282878e7de893d87242ddf9d1512be162e))
    ROM_LOAD ("901447.07", 0xf800, 0x800, CRC(c4f47ad1) SHA1(d440f2510bc52e20c3d6bc8b9ded9cea7f462a9c))
	ROM_REGION (0x1000, REGION_GFX1, 0)
    ROM_LOAD ("901447.08", 0x0000, 0x800, CRC(54f32f45) SHA1(3e067cc621e4beafca2b90cb8f6dba975df2855b))
ROM_END

/* basic 2 */
ROM_START (pet2)
	ROM_REGION (0x10000, REGION_CPU1, 0)
    ROM_LOAD ("901465.01", 0xc000, 0x1000, CRC(63a7fe4a) SHA1(3622111f486d0e137022523657394befa92bde44))
    ROM_LOAD ("901465.02", 0xd000, 0x1000, CRC(ae4cb035) SHA1(1bc0ebf27c9bb62ad71bca40313e874234cab6ac))
    ROM_LOAD ("901447.24", 0xe000, 0x800, CRC(e459ab32) SHA1(5e5502ce32f5a7e387d65efe058916282041e54b))
    ROM_LOAD ("901465.03", 0xf000, 0x1000, CRC(f02238e2) SHA1(38742bdf449f629bcba6276ef24d3daeb7da6e84))
	ROM_REGION (0x1000, REGION_GFX1, 0)
    ROM_LOAD ("901447.08", 0x0000, 0x800, CRC(54f32f45) SHA1(3e067cc621e4beafca2b90cb8f6dba975df2855b))
ROM_END

/* basic 2 business */
ROM_START (pet2b)
	ROM_REGION (0x10000, REGION_CPU1, 0)
    ROM_LOAD ("901465.01", 0xc000, 0x1000, CRC(63a7fe4a) SHA1(3622111f486d0e137022523657394befa92bde44))
    ROM_LOAD ("901465.02", 0xd000, 0x1000, CRC(ae4cb035) SHA1(1bc0ebf27c9bb62ad71bca40313e874234cab6ac))
    ROM_LOAD ("901474.01", 0xe000, 0x800, CRC(05db957e) SHA1(174ace3a8c0348cd21d39cc864e2adc58b0101a9))
    ROM_LOAD ("901465.03", 0xf000, 0x1000, CRC(f02238e2) SHA1(38742bdf449f629bcba6276ef24d3daeb7da6e84))
	ROM_REGION (0x1000, REGION_GFX1, 0)
    ROM_LOAD ("901447.10", 0x0000, 0x800, CRC(d8408674) SHA1(0157a2d55b7ac4eaeb38475889ebeea52e2593db))
ROM_END

/* basic 4 business */
ROM_START (pet4b)
	ROM_REGION (0x10000, REGION_CPU1, 0)
    ROM_LOAD ("901465.23", 0xb000, 0x1000, CRC(ae3deac0) SHA1(975ee25e28ff302879424587e5fb4ba19f403adc))
    ROM_LOAD ("901465.20", 0xc000, 0x1000, CRC(0fc17b9c) SHA1(242f98298931d21eaacb55fe635e44b7fc192b0a))
    ROM_LOAD ("901465.21", 0xd000, 0x1000, CRC(36d91855) SHA1(1bb236c72c726e8fb029c68f9bfa5ee803faf0a8))
    ROM_LOAD ("901474.02", 0xe000, 0x800, CRC(75ff4af7) SHA1(0ca5c4e8f532f914cb0bf86ea9900f20f0a655ce))
    ROM_LOAD ("901465.22", 0xf000, 0x1000, CRC(cc5298a1) SHA1(96a0fa56e0c937da92971d9c99d504e44e898806))
	ROM_REGION (0x1000, REGION_GFX1, 0)
    ROM_LOAD ("901447.10", 0x0000, 0x800, CRC(d8408674) SHA1(0157a2d55b7ac4eaeb38475889ebeea52e2593db))
ROM_END

/* basic 4 crtc*/
ROM_START (pet4)
	ROM_REGION (0x10000, REGION_CPU1, 0)
    ROM_LOAD ("901465.23", 0xb000, 0x1000, CRC(ae3deac0) SHA1(975ee25e28ff302879424587e5fb4ba19f403adc))
    ROM_LOAD ("901465.20", 0xc000, 0x1000, CRC(0fc17b9c) SHA1(242f98298931d21eaacb55fe635e44b7fc192b0a))
    ROM_LOAD ("901465.21", 0xd000, 0x1000, CRC(36d91855) SHA1(1bb236c72c726e8fb029c68f9bfa5ee803faf0a8))
    ROM_LOAD ("901499.01", 0xe000, 0x800, CRC(5f85bdf8) SHA1(8cbf086c1ce4dfb2a2fe24c47476dfb878493dee))
    ROM_LOAD ("901465.22", 0xf000, 0x1000, CRC(cc5298a1) SHA1(96a0fa56e0c937da92971d9c99d504e44e898806))
	ROM_REGION (0x1000, REGION_GFX1, 0)
    ROM_LOAD ("901447.08", 0x0000, 0x800, CRC(54f32f45) SHA1(3e067cc621e4beafca2b90cb8f6dba975df2855b))
ROM_END

/* basic 4 crtc 50 hz */
ROM_START (pet4pal)
	ROM_REGION (0x10000, REGION_CPU1, 0)
    ROM_LOAD ("901465.23", 0xb000, 0x1000, CRC(ae3deac0) SHA1(975ee25e28ff302879424587e5fb4ba19f403adc))
    ROM_LOAD ("901465.20", 0xc000, 0x1000, CRC(0fc17b9c) SHA1(242f98298931d21eaacb55fe635e44b7fc192b0a))
    ROM_LOAD ("901465.21", 0xd000, 0x1000, CRC(36d91855) SHA1(1bb236c72c726e8fb029c68f9bfa5ee803faf0a8))
    ROM_LOAD ("901498.01", 0xe000, 0x800, CRC(3370e359) SHA1(05af284c914d53a52987b5f602466de75765f650))
    ROM_LOAD ("901465.22", 0xf000, 0x1000, CRC(cc5298a1) SHA1(96a0fa56e0c937da92971d9c99d504e44e898806))
	ROM_REGION (0x1000, REGION_GFX1, 0)
    ROM_LOAD ("901447.08", 0x0000, 0x800, CRC(54f32f45) SHA1(3e067cc621e4beafca2b90cb8f6dba975df2855b))
ROM_END

/* basic 4 business 80 columns */
ROM_START (pet80)
	ROM_REGION (0x20000, REGION_CPU1, 0)
    ROM_LOAD ("901465.23", 0xb000, 0x1000, CRC(ae3deac0) SHA1(975ee25e28ff302879424587e5fb4ba19f403adc))
    ROM_LOAD ("901465.20", 0xc000, 0x1000, CRC(0fc17b9c) SHA1(242f98298931d21eaacb55fe635e44b7fc192b0a))
    ROM_LOAD ("901465.21", 0xd000, 0x1000, CRC(36d91855) SHA1(1bb236c72c726e8fb029c68f9bfa5ee803faf0a8))
    ROM_LOAD ("901474.03", 0xe000, 0x800, CRC(5674dd5e) SHA1(c605fa343fd77c73cbe1e0e9567e2f014f6e7e30))
    ROM_LOAD ("901465.22", 0xf000, 0x1000, CRC(cc5298a1) SHA1(96a0fa56e0c937da92971d9c99d504e44e898806))
	ROM_REGION (0x2000, REGION_GFX1, 0)
    ROM_LOAD ("901447.10", 0x0000, 0x800, CRC(d8408674) SHA1(0157a2d55b7ac4eaeb38475889ebeea52e2593db))
ROM_END

/* basic 4 business 80 columns 50 hz */
ROM_START (pet80pal)
	ROM_REGION (0x20000, REGION_CPU1, 0)
    ROM_LOAD ("901465.23", 0xb000, 0x1000, CRC(ae3deac0) SHA1(975ee25e28ff302879424587e5fb4ba19f403adc))
    ROM_LOAD ("901465.20", 0xc000, 0x1000, CRC(0fc17b9c) SHA1(242f98298931d21eaacb55fe635e44b7fc192b0a))
    ROM_LOAD ("901465.21", 0xd000, 0x1000, CRC(36d91855) SHA1(1bb236c72c726e8fb029c68f9bfa5ee803faf0a8))
    ROM_LOAD ("901474.04", 0xe000, 0x800, CRC(abb000e7) SHA1(66887061b6c4ebef7d6efb90af9afd5e2c3b08ba))
    ROM_LOAD ("901465.22", 0xf000, 0x1000, CRC(cc5298a1) SHA1(96a0fa56e0c937da92971d9c99d504e44e898806))
	ROM_REGION (0x2000, REGION_GFX1, 0)
    ROM_LOAD ("901447.10", 0x0000, 0x800, CRC(d8408674) SHA1(0157a2d55b7ac4eaeb38475889ebeea52e2593db))
ROM_END

ROM_START (cbm80ger)
	ROM_REGION (0x20000, REGION_CPU1, 0)
	ROM_LOAD ("901465.23", 0xb000, 0x1000, CRC(ae3deac0) SHA1(975ee25e28ff302879424587e5fb4ba19f403adc))
	ROM_LOAD ("901465.20", 0xc000, 0x1000, CRC(0fc17b9c) SHA1(242f98298931d21eaacb55fe635e44b7fc192b0a))
	ROM_LOAD ("901465.21", 0xd000, 0x1000, CRC(36d91855) SHA1(1bb236c72c726e8fb029c68f9bfa5ee803faf0a8))
	ROM_LOAD ("german.bin", 0xe000, 0x800, CRC(1c1e597d) SHA1(7ac75ed73832847623c9f4f197fe7fb1a73bb41c))
	ROM_LOAD ("901465.22", 0xf000, 0x1000, CRC(cc5298a1) SHA1(96a0fa56e0c937da92971d9c99d504e44e898806))
	ROM_REGION (0x2000, REGION_GFX1, 0)
	ROM_LOAD ("chargen.de", 0x0000, 0x800, CRC(3bb8cb87) SHA1(a4f0df13473d7f9cd31fd62cfcab11318e2fb1dc))
ROM_END

ROM_START (cbm80swe)
	ROM_REGION (0x20000, REGION_CPU1, 0)
    ROM_LOAD ("901465.23", 0xb000, 0x1000, CRC(ae3deac0) SHA1(975ee25e28ff302879424587e5fb4ba19f403adc))
    ROM_LOAD ("901465.20", 0xc000, 0x1000, CRC(0fc17b9c) SHA1(242f98298931d21eaacb55fe635e44b7fc192b0a))
    ROM_LOAD ("901465.21", 0xd000, 0x1000, CRC(36d91855) SHA1(1bb236c72c726e8fb029c68f9bfa5ee803faf0a8))
    ROM_LOAD ("editswe.bin", 0xe000, 0x800, CRC(75901dd7) SHA1(2ead0d83255a344a42bb786428353ca48d446d03))
    ROM_LOAD ("901465.22", 0xf000, 0x1000, CRC(cc5298a1) SHA1(96a0fa56e0c937da92971d9c99d504e44e898806))
	ROM_REGION (0x2000, REGION_GFX1, 0)
    ROM_LOAD ("901447.14", 0x0000, 0x800, CRC(48c77d29) SHA1(aa7c8ff844d16ec05e2b32acc586c58d9e35388c))
ROM_END

ROM_START (superpet)
	ROM_REGION (0x10000, REGION_CPU1, 0)
    ROM_LOAD ("901465.23", 0xb000, 0x1000, CRC(ae3deac0) SHA1(975ee25e28ff302879424587e5fb4ba19f403adc))
    ROM_LOAD ("901465.20", 0xc000, 0x1000, CRC(0fc17b9c) SHA1(242f98298931d21eaacb55fe635e44b7fc192b0a))
    ROM_LOAD ("901465.21", 0xd000, 0x1000, CRC(36d91855) SHA1(1bb236c72c726e8fb029c68f9bfa5ee803faf0a8))
    ROM_LOAD ("901474.04", 0xe000, 0x800, CRC(abb000e7) SHA1(66887061b6c4ebef7d6efb90af9afd5e2c3b08ba))
    ROM_LOAD ("901465.22", 0xf000, 0x1000, CRC(cc5298a1) SHA1(96a0fa56e0c937da92971d9c99d504e44e898806))
	ROM_REGION (0x10000, REGION_CPU2, 0)
    ROM_LOAD ("901898.01", 0xa000, 0x1000, CRC(728a998b) SHA1(0414b3ab847c8977eb05c2fcc72efcf2f9d92871))
    ROM_LOAD ("901898.02", 0xb000, 0x1000, CRC(6beb7c62) SHA1(df154939b934d0aeeb376813ec1ba0d43c2a3378))
    ROM_LOAD ("901898.03", 0xc000, 0x1000, CRC(5db4983d) SHA1(6c5b0cce97068f8841112ba6d5cd8e568b562fa3))
    ROM_LOAD ("901898.04", 0xd000, 0x1000, CRC(f55fc559) SHA1(b42a2050a319a1ffca7868a8d8d635fadd37ec37))
    ROM_LOAD ("901897.01", 0xe000, 0x800, CRC(b2cee903) SHA1(e8ce8347451a001214a5e71a13081b38b4be23bc))
    ROM_LOAD ("901898.05", 0xf000, 0x1000, CRC(f42df0cb) SHA1(9b4a5134d20345171e7303445f87c4e0b9addc96))
	ROM_REGION (0x4000, REGION_GFX1, 0)
    ROM_LOAD ("901640.01", 0x0000, 0x1000, CRC(ee8229c4) SHA1(bf346f11595a3e65e55d6aeeaa2c0cec807b66c7))
ROM_END

/* swedish m6809 roms needed */
ROM_START (mmf9000)
	ROM_REGION (0x10000, REGION_CPU1, 0)
    ROM_LOAD ("901465.23", 0xb000, 0x1000, CRC(ae3deac0) SHA1(975ee25e28ff302879424587e5fb4ba19f403adc))
    ROM_LOAD ("901465.20", 0xc000, 0x1000, CRC(0fc17b9c) SHA1(242f98298931d21eaacb55fe635e44b7fc192b0a))
    ROM_LOAD ("901465.21", 0xd000, 0x1000, CRC(36d91855) SHA1(1bb236c72c726e8fb029c68f9bfa5ee803faf0a8))
    ROM_LOAD ("editswe.bin", 0xe000, 0x800, CRC(75901dd7) SHA1(2ead0d83255a344a42bb786428353ca48d446d03))
    ROM_LOAD ("901465.22", 0xf000, 0x1000, CRC(cc5298a1) SHA1(96a0fa56e0c937da92971d9c99d504e44e898806))
	ROM_REGION (0x20000, REGION_CPU2, 0)
    ROM_LOAD ("901898.01", 0xa000, 0x1000, CRC(728a998b) SHA1(0414b3ab847c8977eb05c2fcc72efcf2f9d92871))
    ROM_LOAD ("901898.02", 0xb000, 0x1000, CRC(6beb7c62) SHA1(df154939b934d0aeeb376813ec1ba0d43c2a3378))
    ROM_LOAD ("901898.03", 0xc000, 0x1000, CRC(5db4983d) SHA1(6c5b0cce97068f8841112ba6d5cd8e568b562fa3))
    ROM_LOAD ("901898.04", 0xd000, 0x1000, CRC(f55fc559) SHA1(b42a2050a319a1ffca7868a8d8d635fadd37ec37))
    ROM_LOAD ("901897.01", 0xe000, 0x800, CRC(b2cee903) SHA1(e8ce8347451a001214a5e71a13081b38b4be23bc))
    ROM_LOAD ("901898.05", 0xf000, 0x1000, CRC(f42df0cb) SHA1(9b4a5134d20345171e7303445f87c4e0b9addc96))
	ROM_REGION (0x4000, REGION_GFX1, 0)
    ROM_LOAD("charswe.bin", 0x0000, 0x1000, CRC(da1cd630))
ROM_END

#if 0
/* in c16 and some other commodore machines:
   cbm version in kernel at 0xff80 (offset 0x3f80)
   0x80 means pal version */

    /* 901447-09 + 901447-02 + 901447-03 + 901447-04 */
    ROM_LOAD ("basic1", 0xc000, 0x2000, CRC(aff78300))
    /* same as 901439-01, maybe same as 6540-011 */
    ROM_LOAD ("rom-1-c000.901447-01.bin", 0xc000, 0x800, CRC(a055e33a))
    /* same as 901439-09, 6540-019 */
    ROM_LOAD ("rom-1-c000.901447-09.bin", 0xc000, 0x800, CRC(03cf16d0))
    /* same as 901439-05, 6540-012 */
    ROM_LOAD ("rom-1-c800.901447-02.bin", 0xc800, 0x800, CRC(69fd8a8f))
    /* same as 901439-02, 6540-013 */
    ROM_LOAD ("rom-1-d000.901447-03.bin", 0xd000, 0x800, CRC(d349f2d4))
    /* same as 901439-06, 6540-014 */
    ROM_LOAD ("rom-1-d800.901447-04.bin", 0xd800, 0x800, CRC(850544eb))

	/* 901465-01 + 901465-02 */
    ROM_LOAD ("basic2", 0xc000, 0x2000, CRC(cf35e68b))
    /* 6540-020 + 6540-021 */
    ROM_LOAD ("basic-2-c000.901465-01.bin", 0xc000, 0x1000, CRC(63a7fe4a))
    /* 6540-022 + 6540-023 */
    ROM_LOAD ("basic-2-d000.901465-02.bin", 0xd000, 0x1000, CRC(ae4cb035))

	/* 901465-23 901465-20 901465-21 */
    ROM_LOAD ("basic4", 0xb000, 0x3000, CRC(2a940f0a))
    ROM_LOAD ("basic-4-b000.901465-19.bin", 0xb000, 0x1000, CRC(3a5f5721))
    ROM_LOAD ("basic-4-b000.901465-23.bin", 0xb000, 0x1000, CRC(ae3deac0))
    ROM_LOAD ("basic-4-c000.901465-20.bin", 0xc000, 0x1000, CRC(0fc17b9c))
    ROM_LOAD ("basic-4-d000.901465-21.bin", 0xd000, 0x1000, CRC(36d91855))

    /* same as 901439-03, 6540-015 */
    ROM_LOAD ("rom-1-e000.901447-05.bin", 0xe000, 0x800, CRC(9e1c5cea))

    ROM_LOAD ("edit-2-b.901474-01.bin", 0xe000, 0x800, CRC(05db957e))
    /* same as 6540-024 */
    ROM_LOAD ("edit-2-n.901447-24.bin", 0xe000, 0x800, CRC(e459ab32))

    ROM_LOAD ("edit-4-40-n-50hz.901498-01.bin", 0xe000, 0x800, CRC(3370e359))
    ROM_LOAD ("edit-4-40-n-60hz.901499-01.bin", 0xe000, 0x800, CRC(5f85bdf8))
    ROM_LOAD ("edit-4-b.901474-02.bin", 0xe000, 0x800, CRC(75ff4af7))

    ROM_LOAD ("edit-4-80-b-60hz.901474-03.bin", 0xe000, 0x800, CRC(5674dd5e))
    /* week 36 year 81 */
    ROM_LOAD ("edit-4-80-b-50hz.901474-04-3681.bin", 0xe000, 0x800, CRC(c1ffca3a))
    ROM_LOAD ("edit-4-80-b-50hz.901474-04.bin", 0xe000, 0x800, CRC(abb000e7))
    ROM_LOAD ("edit-4-80-b-50hz.901474-04?.bin", 0xe000, 0x800, CRC(845a44e6))
    ROM_LOAD ("edit-4-80-b-50hz.german.bin", 0xe000, 0x800, CRC(1c1e597d))
    ROM_LOAD ("edit-4-80-b-50hz.swedish.bin", 0xe000, 0x800, CRC(75901dd7))

	/* 901447-06 + 901447-07 */
    ROM_LOAD ("kernal1", 0xf000, 0x1000, CRC(f0186492))
    /* same as 901439-04, 6540-016 */
    ROM_LOAD ("rom-1-f000.901447-06.bin", 0xf000, 0x800, CRC(661a814a))
    /* same as 904139-07, 6540-018 */
    ROM_LOAD ("rom-1-f800.901447-07.bin", 0xf800, 0x800, CRC(c4f47ad1))

    ROM_LOAD ("kernal-2.901465-03.bin", 0xf000, 0x1000, CRC(f02238e2))

    ROM_LOAD ("kernal-4.901465-22.bin", 0xf000, 0x1000, CRC(cc5298a1))

	/* graphics */
    /* 6540-010 = 901439-08 */

    ROM_LOAD ("characters-1.901447-08.bin", 0x0000, 0x800, CRC(54f32f45))
	/* business */
	/* vice chargen */
    ROM_LOAD ("characters-2.901447-10.bin", 0x0000, 0x800, CRC(d8408674))
    ROM_LOAD ("chargen.de", 0x0000, 0x800, CRC(3bb8cb87))
    ROM_LOAD ("characters-hungarian.bin", 0x0000, 0x800, CRC(a02d8122))
    ROM_LOAD ("characters-swedish.901447-14.bin", 0x0000, 0x800, CRC(48c77d29))

    ROM_LOAD ("", 0xe000, 0x800, CRC())

	/* editor rom */
    ROM_LOAD ("Execudesk.bin", 0xe000, 0x1000, CRC(bef0eaa1))

    ROM_LOAD ("PaperClip.bin", 0xa000, 0x1000, CRC(8fb11d4b))

	/* superpet */
    ROM_LOAD ("waterloo-a000.901898-01.bin", 0xa000, 0x1000, CRC(728a998b))
    ROM_LOAD ("waterloo-b000.901898-02.bin", 0xb000, 0x1000, CRC(6beb7c62))
    ROM_LOAD ("waterloo-c000.901898-03.bin", 0xc000, 0x1000, CRC(5db4983d))
    ROM_LOAD ("waterloo-d000.901898-04.bin", 0xd000, 0x1000, CRC(f55fc559))
    ROM_LOAD ("waterloo-e000.901897-01.bin", 0xe000, 0x800, CRC(b2cee903))
    ROM_LOAD ("waterloo-f000.901898-05.bin", 0xf000, 0x1000, CRC(f42df0cb))
    /* 256 chars commodore pet, 256 chars ascii m6809 */
    ROM_LOAD ("characters.901640-01.bin", 0x0000, 0x1000, CRC(ee8229c4))
    /* 901447-14 and the 256 chars ascii from 901640-01 */
    ROM_LOAD ("characters.swedish.bin", 0x0000, 0x1000, CRC(da1cd630))

	/* scrap */
	/* fixed bits */
    ROM_LOAD ("324878-01.bin", 0x?000, 0x2000, CRC(d262bacd))
    ROM_LOAD ("324878-02.bin", 0x?000, 0x2000, CRC(5e00476d))
#endif

static MACHINE_DRIVER_START( pet )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6502, 7833600)        /* 7.8336 Mhz */
	MDRV_CPU_PROGRAM_MAP(pet_mem, 0)
	MDRV_CPU_VBLANK_INT(pet_frame_interrupt, 1)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(0)

	MDRV_MACHINE_RESET( pet )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(320, 200)
	MDRV_SCREEN_VISIBLE_AREA(0, 320 - 1, 0, 200 - 1)
	MDRV_GFXDECODE( pet_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(sizeof (pet_palette) / sizeof (pet_palette[0]) / 3)
	MDRV_COLORTABLE_LENGTH(sizeof (pet_colortable) / sizeof(pet_colortable[0][0]))
	MDRV_PALETTE_INIT( pet )

	MDRV_VIDEO_START( generic )
	MDRV_VIDEO_UPDATE( pet )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pet40 )
	MDRV_IMPORT_FROM( pet )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP( pet40_mem, 0 )
	MDRV_VIDEO_UPDATE( pet40 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pet40pal )
	MDRV_IMPORT_FROM( pet40 )
	MDRV_SCREEN_REFRESH_RATE(50)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pet80 )
	MDRV_IMPORT_FROM( pet )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP( pet80_mem, 0 )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 250)
	MDRV_SCREEN_VISIBLE_AREA(0, 640 - 1, 0, 250 - 1)
	MDRV_GFXDECODE( pet80_gfxdecodeinfo )
	MDRV_VIDEO_UPDATE( pet80 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pet80pal )
	MDRV_IMPORT_FROM( pet80 )
	MDRV_SCREEN_REFRESH_RATE(50)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( superpet )
	MDRV_IMPORT_FROM( pet80 )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP( superpet_mem, 0 )

	/* m6809 cpu */
	MDRV_CPU_ADD_TAG("main", M6809, 1000000)
	MDRV_CPU_PROGRAM_MAP(superpet_m6809_mem, 0)
	MDRV_CPU_VBLANK_INT(pet_frame_interrupt, 1)

	MDRV_SCREEN_REFRESH_RATE(50)

    /* video hardware */
	MDRV_GFXDECODE( superpet_gfxdecodeinfo )
MACHINE_DRIVER_END

#define rom_cbm30 rom_pet2
#define rom_cbm30 rom_pet2
#define rom_cbm30b rom_pet2b
#define rom_cbm40 rom_pet4
#define rom_cbm40pal rom_pet4pal
#define rom_cbm40b rom_pet4b
#define rom_cbm80 rom_pet80
#define rom_cbm80pal rom_pet80pal

static void pet_cbmcartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "crt,a0,b0"); break;

		default:										cbmcartslot_device_getinfo(devclass, state, info); break;
	}
}

static void pet_quickload_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "p00,prg"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_QUICKLOAD_LOAD:				info->f = (genf *) quickload_load_cbm_pet; break;

		/* --- the following bits of info are returned as doubles --- */
		case DEVINFO_FLOAT_QUICKLOAD_DELAY:				info->d = CBM_QUICKLOAD_DELAY; break;

		default:										quickload_device_getinfo(devclass, state, info); break;
	}
}

static void pet1_quickload_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "p00,prg"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_QUICKLOAD_LOAD:				info->f = (genf *) quickload_load_cbm_pet1; break;

		/* --- the following bits of info are returned as doubles --- */
		case DEVINFO_FLOAT_QUICKLOAD_DELAY:				info->d = CBM_QUICKLOAD_DELAY; break;

		default:										quickload_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(pet)
	CONFIG_DEVICE(pet_cbmcartslot_getinfo)
	CONFIG_DEVICE(cbmfloppy_device_getinfo)
	CONFIG_DEVICE(pet1_quickload_getinfo)
	CONFIG_RAM(4 * 1024)
	CONFIG_RAM(8 * 1024)
	CONFIG_RAM(16 * 1024)
	CONFIG_RAM_DEFAULT(32 * 1024)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(pet2)
	CONFIG_DEVICE(pet_cbmcartslot_getinfo)
	CONFIG_DEVICE(cbmfloppy_device_getinfo)
	CONFIG_DEVICE(pet_quickload_getinfo)
	CONFIG_RAM(4 * 1024)
	CONFIG_RAM(8 * 1024)
	CONFIG_RAM(16 * 1024)
	CONFIG_RAM_DEFAULT(32 * 1024)
SYSTEM_CONFIG_END

static void pet4_cbmcartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "crt,a0"); break;

		default:										cbmcartslot_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(pet4)
	CONFIG_DEVICE(pet4_cbmcartslot_getinfo)
	CONFIG_DEVICE(cbmfloppy_device_getinfo)
	CONFIG_DEVICE(pet_quickload_getinfo)
	CONFIG_RAM(4 * 1024)
	CONFIG_RAM(8 * 1024)
	CONFIG_RAM(16 * 1024)
	CONFIG_RAM_DEFAULT(32 * 1024)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(pet4_32)
	CONFIG_DEVICE(pet4_cbmcartslot_getinfo)
	CONFIG_DEVICE(cbmfloppy_device_getinfo)
	CONFIG_DEVICE(pet_quickload_getinfo)
	CONFIG_RAM_DEFAULT(32 * 1024)
SYSTEM_CONFIG_END

/*    YEAR  NAME		COMPAT	PARENT	MACHINE 	INPUT	 INIT	  CONFIG	COMPANY							  FULLNAME */
COMP (1977,	pet,		0,		0,		pet,		pet,	 pet1,	  pet,		"Commodore Business Machines Co.",  "PET2001/CBM20xx Series (Basic 1)",            GAME_NO_SOUND)
COMP (1979,	cbm30,		0,		pet,	pet,		pet,	 pet,	  pet2,		"Commodore Business Machines Co.",  "Commodore 30xx (Basic 2)",                    GAME_NO_SOUND)
COMP (1979,	cbm30b, 	0,		pet,	pet,		petb,	 pet,	  pet2,		"Commodore Business Machines Co.",  "Commodore 30xx (Basic 2) (business keyboard)",GAME_NO_SOUND)
COMP (1982,	cbm40,		0,		pet,	pet40,		pet,	 pet40,   pet4,		"Commodore Business Machines Co.",  "Commodore 40xx FAT (CRTC) 60Hz",              GAME_NO_SOUND)
COMP (1982,	cbm40pal,	0,		pet,	pet40pal,	pet,	 pet40,   pet4,		"Commodore Business Machines Co.",  "Commodore 40xx FAT (CRTC) 50Hz",              GAME_NO_SOUND)
COMP (1979,	cbm40b, 	0,		pet,	pet,		petb,	 pet,	  pet4,		"Commodore Business Machines Co.",  "Commodore 40xx THIN (business keyboard)",     GAME_NO_SOUND)
COMP (1981,	cbm80,		0,		pet,	pet80,		cbm8096, cbm80,   pet4_32,	"Commodore Business Machines Co.",  "Commodore 80xx 60Hz",                         GAME_NO_SOUND)
COMP (1981,	cbm80pal,	0,		pet,	pet80pal,	cbm8096, cbm80,   pet4_32,	"Commodore Business Machines Co.",  "Commodore 80xx 50Hz",                         GAME_NO_SOUND)
COMP (1981,	cbm80ger,	0,		pet,	pet80pal,	cbm8096, cbm80,   pet4_32,	"Commodore Business Machines Co.",  "Commodore 80xx German (50Hz)",                GAME_NO_SOUND)
COMP (1981,	cbm80swe,	0,		pet,	pet80pal,	cbm8096, cbm80,   pet4_32,	"Commodore Business Machines Co.",  "Commodore 80xx Swedish (50Hz)",               GAME_NO_SOUND)
COMP (1981,	superpet,	0,		pet,	superpet,	superpet,superpet,pet4_32,	"Commodore Business Machines Co.",  "Commodore SP9000/MMF9000 (50Hz)",             GAME_NO_SOUND|GAME_NOT_WORKING)

// please leave the following as testdriver only
COMP (198?, 	mmf9000,	0,		pet,	superpet,	superpet,superpet,pet4,		"Commodore Business Machines Co.",  "MMF9000 (50Hz) Swedish", 0)
