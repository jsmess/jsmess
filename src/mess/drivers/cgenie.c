/***************************************************************************
HAD to change the PORT_ANALOG defs in this file...  please check ;-)

Colour Genie memory map

CPU #1:
0000-3fff ROM basic & bios        R   D0-D7

4000-bfff RAM
c000-dfff ROM dos                 R   D0-D7
e000-efff ROM extra               R   D0-D7
f000-f3ff color ram               W/R D0-D3
f400-f7ff font ram                W/R D0-D7
f800-f8ff keyboard matrix         R   D0-D7
ffe0-ffe3 floppy motor            W   D0-D2
          floppy head select      W   D3
ffec-ffef FDC WD179x              R/W D0-D7
          ffec command            W
          ffec status             R
          ffed track              R/W
          ffee sector             R/W
          ffef data               R/W

Interrupts:
IRQ mode 1
NMI
***************************************************************************/

#include "driver.h"
#include "includes/cgenie.h"
#include "devices/basicdsk.h"
#include "devices/cartslot.h"
#include "sound/ay8910.h"

static ADDRESS_MAP_START (cgenie_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_ROM
//  AM_RANGE(0x4000, 0xbfff) AM_RAM // set up in MACHINE_START
//  AM_RANGE(0xc000, 0xdfff) AM_ROM // installed in cgenie_init_machine
//  AM_RANGE(0xe000, 0xefff) AM_ROM // installed in cgenie_init_machine
	AM_RANGE(0xf000, 0xf3ff) AM_READWRITE( cgenie_colorram_r, cgenie_colorram_w ) AM_BASE( &colorram )
	AM_RANGE(0xf400, 0xf7ff) AM_READWRITE( cgenie_fontram_r, cgenie_fontram_w) AM_BASE( &cgenie_fontram )
	AM_RANGE(0xf800, 0xf8ff) AM_READ( cgenie_keyboard_r )
	AM_RANGE(0xf900, 0xffdf) AM_NOP
	AM_RANGE(0xffe0, 0xffe3) AM_READWRITE( cgenie_irq_status_r, cgenie_motor_w )
	AM_RANGE(0xffe4, 0xffeb) AM_NOP
	AM_RANGE(0xffec, 0xffec) AM_READWRITE( cgenie_status_r, cgenie_command_w )
	AM_RANGE(0xffe4, 0xffeb) AM_NOP
	AM_RANGE(0xffec, 0xffec) AM_WRITE( cgenie_command_w )
	AM_RANGE(0xffed, 0xffed) AM_READWRITE( cgenie_track_r, cgenie_track_w )
	AM_RANGE(0xffee, 0xffee) AM_READWRITE( cgenie_sector_r, cgenie_sector_w )
	AM_RANGE(0xffef, 0xffef) AM_READWRITE( cgenie_data_r, cgenie_data_w )
	AM_RANGE(0xfff0, 0xffff) AM_NOP
ADDRESS_MAP_END

static ADDRESS_MAP_START (cgenie_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0xf8, 0xf8) AM_READWRITE( cgenie_sh_control_port_r, cgenie_sh_control_port_w )
	AM_RANGE(0xf9, 0xf9) AM_READWRITE( cgenie_sh_data_port_r, cgenie_sh_data_port_w )
	AM_RANGE(0xfa, 0xfa) AM_READWRITE( cgenie_index_r, cgenie_index_w )
	AM_RANGE(0xfb, 0xfb) AM_READWRITE( cgenie_register_r, cgenie_register_w )
	AM_RANGE(0xff, 0xff) AM_READWRITE( cgenie_port_ff_r, cgenie_port_ff_w )
ADDRESS_MAP_END

static INPUT_PORTS_START( cgenie )
	PORT_START_TAG("DSW0") /* IN0 */
	PORT_DIPNAME( 0x80, 0x80, "Floppy Disc Drives")
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "CG-DOS ROM C000-DFFF")
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Extension  E000-EFFF")
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Video Display accuracy") PORT_CODE(KEYCODE_F5) PORT_TOGGLE
	PORT_DIPSETTING(	0x10, "TV set" )
	PORT_DIPSETTING(	0x00, "RGB monitor" )
	PORT_DIPNAME( 0x08, 0x08, "Virtual tape support") PORT_CODE(KEYCODE_F6) PORT_TOGGLE
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x08, DEF_STR( On ) )
	PORT_BIT(0x07, 0x07, IPT_UNUSED)

/**************************************************************************
   +-------------------------------+     +-------------------------------+
   | 0   1   2   3   4   5   6   7 |     | 0   1   2   3   4   5   6   7 |
+--+---+---+---+---+---+---+---+---+  +--+---+---+---+---+---+---+---+---+
|0 | @ | A | B | C | D | E | F | G |  |0 | ` | a | b | c | d | e | f | g |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|1 | H | I | J | K | L | M | N | O |  |1 | h | i | j | k | l | m | n | o |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|2 | P | Q | R | S | T | U | V | W |  |2 | p | q | r | s | t | u | v | w |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|3 | X | Y | Z | [ |F-1|F-2|F-3|F-4|  |3 | x | y | z | { |F-5|F-6|F-7|F-8|
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|4 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |  |4 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|5 | 8 | 9 | : | ; | , | - | . | / |  |5 | 8 | 9 | * | + | < | = | > | ? |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|6 |ENT|CLR|BRK|UP |DN |LFT|RGT|SPC|  |6 |ENT|CLR|BRK|UP |DN |LFT|RGT|SPC|
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|7 |SHF|ALT|PUP|PDN|INS|DEL|CTL|END|  |7 |SHF|ALT|PUP|PDN|INS|DEL|CTL|END|
+--+---+---+---+---+---+---+---+---+  +--+---+---+---+---+---+---+---+---+
***************************************************************************/

	PORT_START_TAG("ROW0") /* IN1 KEY ROW 0 */
		PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE)		PORT_CHAR('@') PORT_CHAR('`')
		PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_A)				PORT_CHAR('a') PORT_CHAR('A')
		PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) 			PORT_CHAR('b') PORT_CHAR('B')
		PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) 			PORT_CHAR('c') PORT_CHAR('C')
		PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) 			PORT_CHAR('d') PORT_CHAR('D')
		PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) 			PORT_CHAR('e') PORT_CHAR('E')
		PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) 			PORT_CHAR('f') PORT_CHAR('F')
		PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) 			PORT_CHAR('g') PORT_CHAR('G')

	PORT_START_TAG("ROW1") /* IN2 KEY ROW 1 */
		PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_H) 			PORT_CHAR('h') PORT_CHAR('H')
		PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) 			PORT_CHAR('i') PORT_CHAR('I')
		PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) 			PORT_CHAR('j') PORT_CHAR('J')
		PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) 			PORT_CHAR('k') PORT_CHAR('K')
		PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) 			PORT_CHAR('l') PORT_CHAR('L')
		PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_M) 			PORT_CHAR('m') PORT_CHAR('M')
		PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_N) 			PORT_CHAR('n') PORT_CHAR('N')
		PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) 			PORT_CHAR('o') PORT_CHAR('O')

	PORT_START_TAG("ROW2") /* IN3 KEY ROW 2 */
		PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_P) 			PORT_CHAR('p') PORT_CHAR('P')
		PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) 			PORT_CHAR('q') PORT_CHAR('Q')
		PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) 			PORT_CHAR('r') PORT_CHAR('R')
		PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) 			PORT_CHAR('s') PORT_CHAR('S')
		PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) 			PORT_CHAR('t') PORT_CHAR('T')
		PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_U) 			PORT_CHAR('u') PORT_CHAR('U')
		PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) 			PORT_CHAR('v') PORT_CHAR('V')
		PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) 			PORT_CHAR('w') PORT_CHAR('W')

	PORT_START_TAG("ROW3") /* IN4 KEY ROW 3 */
		PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) 			PORT_CHAR('x') PORT_CHAR('X')
		PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) 			PORT_CHAR('y') PORT_CHAR('Y')
		PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z) 			PORT_CHAR('z') PORT_CHAR('Z')
		PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("[  { (Not Connected)") PORT_CHAR('[') PORT_CHAR('{')
		PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_F1)			PORT_CHAR(UCHAR_MAMEKEY(F1)) PORT_CHAR(UCHAR_MAMEKEY(F5))
		PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_F2)			PORT_CHAR(UCHAR_MAMEKEY(F2)) PORT_CHAR(UCHAR_MAMEKEY(F6))
		PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_F3)			PORT_CHAR(UCHAR_MAMEKEY(F3)) PORT_CHAR(UCHAR_MAMEKEY(F7))
		PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_F4)			PORT_CHAR(UCHAR_MAMEKEY(F4)) PORT_CHAR(UCHAR_MAMEKEY(F8))

	PORT_START_TAG("ROW4") /* IN5 KEY ROW 4 */
		PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_0) 			PORT_CHAR('0')
		PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) 			PORT_CHAR('1') PORT_CHAR('!')
		PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) 			PORT_CHAR('2') PORT_CHAR('"')
		PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) 			PORT_CHAR('3') PORT_CHAR('#')
		PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) 			PORT_CHAR('4') PORT_CHAR('$')
		PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_5) 			PORT_CHAR('5') PORT_CHAR('%')
		PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) 			PORT_CHAR('6') PORT_CHAR('&')
		PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) 			PORT_CHAR('7') PORT_CHAR('\'')

	PORT_START_TAG("ROW5") /* IN6 KEY ROW 5 */
		PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_8) 			PORT_CHAR('8') PORT_CHAR('(')
		PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_9) 			PORT_CHAR('9') PORT_CHAR(')')
		PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS) 		PORT_CHAR(':') PORT_CHAR('*')
		PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON) 		PORT_CHAR(';') PORT_CHAR('+')
		PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA) 		PORT_CHAR(',') PORT_CHAR('<')
		PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE) 		PORT_CHAR('-') PORT_CHAR('=')
		PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP) 			PORT_CHAR('.') PORT_CHAR('>')
		PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH) 		PORT_CHAR('/') PORT_CHAR('?')

	PORT_START_TAG("ROW6") /* IN7 KEY ROW 6 */
		PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER)			PORT_CHAR(13)
		PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("Clear") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR(UCHAR_MAMEKEY(F10))
		PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("Break") PORT_CODE(KEYCODE_BACKSLASH2) PORT_CHAR(UCHAR_MAMEKEY(ESC))
		PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_UP)			PORT_CHAR(UCHAR_MAMEKEY(UP))
		PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_DOWN)			PORT_CHAR(UCHAR_MAMEKEY(DOWN))
		PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_LEFT)			PORT_CHAR(UCHAR_MAMEKEY(LEFT))
		PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_RIGHT)			PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
		PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE)			PORT_CHAR(' ')

	/* 2008-05 FP: Below we still miss a 'Lock' key, two 'Rst' keys (used in pair the should restart the 
	system) and, I guess, two unused inputs (according to the user manual, there are no other keys on the 
	keyboard)  */
	PORT_START_TAG("ROW7") /* IN8 KEY ROW 7 */
		PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
		PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("Mod Sel") PORT_CODE(KEYCODE_LALT)  PORT_CHAR(UCHAR_MAMEKEY(F9))
		PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("Unknown 1")
		PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("Rpt") PORT_CODE(KEYCODE_PGDN) PORT_CHAR(UCHAR_MAMEKEY(F11))
		PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(F12))
		PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("Unknown 2")
		PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("Unknown 3")
		PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("Unknown 4")

	PORT_START_TAG("JOY0") /* IN9 */
	PORT_BIT( 0xff, 0x60, IPT_AD_STICK_X) PORT_SENSITIVITY(40) PORT_KEYDELTA(0) PORT_MINMAX(0x00,0xcf ) PORT_PLAYER(1)

	PORT_START_TAG("JOY1") /* IN10 */
	PORT_BIT( 0xff, 0x60, IPT_AD_STICK_Y) PORT_SENSITIVITY(40) PORT_KEYDELTA(0) PORT_MINMAX(0x00,0xcf ) PORT_PLAYER(1) PORT_REVERSE

	PORT_START_TAG("JOY2") /* IN11 */
	PORT_BIT( 0xff, 0x60, IPT_AD_STICK_X) PORT_SENSITIVITY(40) PORT_KEYDELTA(0) PORT_MINMAX(0x00,0xcf ) PORT_PLAYER(2)

	PORT_START_TAG("JOY3") /* IN12 */
	PORT_BIT( 0xff, 0x60, IPT_AD_STICK_Y) PORT_SENSITIVITY(40) PORT_KEYDELTA(0) PORT_MINMAX(0x00,0xcf ) PORT_PLAYER(2) PORT_REVERSE

	/* Joystick Keypad */
	/* keypads were organized a 3 x 4 matrix and it looked     */
	/* exactly like a our northern telephone numerical pads    */
	/* The addressing was done with a single clear bit 0..6    */
	/* on i/o port A,  while all other bits were set.          */
	/* (e.g. 0xFE addresses keypad1 row 0, 0xEF keypad2 row 1) */

	/*       bit  0   1   2   3   */
	/* FE/F7  0  [3] [6] [9] [#]  */
	/* FD/EF  1  [2] [5] [8] [0]  */
	/* FB/DF  2  [1] [4] [7] [*]  */

	PORT_START_TAG("KP0") /* IN13 */
		PORT_BIT(0x01, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 1 [3]") PORT_CODE(KEYCODE_3_PAD) PORT_CODE(JOYCODE_BUTTON3)
		PORT_BIT(0x02, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 1 [6]") PORT_CODE(KEYCODE_6_PAD) PORT_CODE(JOYCODE_BUTTON6)
		PORT_BIT(0x04, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 1 [9]") PORT_CODE(KEYCODE_9_PAD)
		PORT_BIT(0x08, 0x00, IPT_BUTTON2) PORT_NAME("Joy 1 [#]") PORT_CODE(KEYCODE_SLASH_PAD) PORT_CODE(JOYCODE_BUTTON1)

	PORT_START_TAG("KP1")
		PORT_BIT(0x01, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 1 [2]") PORT_CODE(KEYCODE_2_PAD) PORT_CODE(JOYCODE_BUTTON2)
		PORT_BIT(0x02, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 1 [5]") PORT_CODE(KEYCODE_5_PAD) PORT_CODE(JOYCODE_BUTTON5)
		PORT_BIT(0x04, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 1 [8]") PORT_CODE(KEYCODE_8_PAD)
		PORT_BIT(0x08, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 1 [0]") PORT_CODE(KEYCODE_0_PAD)

	PORT_START_TAG("KP2") /* IN14 */
		PORT_BIT(0x01, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 1 [1]") PORT_CODE(KEYCODE_1_PAD) PORT_CODE(JOYCODE_BUTTON3)
		PORT_BIT(0x02, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 1 [4]") PORT_CODE(KEYCODE_4_PAD) PORT_CODE(JOYCODE_BUTTON6)
		PORT_BIT(0x04, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 1 [7]") PORT_CODE(KEYCODE_7_PAD)
		PORT_BIT(0x08, 0x00, IPT_BUTTON1) PORT_NAME("Joy 1 [*]") PORT_CODE(KEYCODE_ASTERISK) PORT_CODE(JOYCODE_BUTTON1)

	PORT_START_TAG("KP3")
		PORT_BIT(0x01, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 2 [3]") PORT_CODE(JOYCODE_BUTTON2)
		PORT_BIT(0x02, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 2 [6]") PORT_CODE(JOYCODE_BUTTON5)
		PORT_BIT(0x04, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 2 [9]")
		PORT_BIT(0x08, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 2 [#]") PORT_CODE(JOYCODE_BUTTON1)

	PORT_START_TAG("KP4") /* IN15 */
		PORT_BIT(0x01, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 2 [2]") PORT_CODE(JOYCODE_BUTTON2)
		PORT_BIT(0x02, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 2 [5]") PORT_CODE(JOYCODE_BUTTON5)
		PORT_BIT(0x04, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 2 [8]")
		PORT_BIT(0x08, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 2 [0]")

	PORT_START_TAG("KP5")
		PORT_BIT(0x01, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 2 [1]") PORT_CODE(JOYCODE_BUTTON1)
		PORT_BIT(0x02, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 2 [4]") PORT_CODE(JOYCODE_BUTTON4)
		PORT_BIT(0x04, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 2 [7]")
		PORT_BIT(0x08, 0x00, IPT_UNKNOWN) PORT_NAME("Joy 2 [*]") PORT_CODE(JOYCODE_BUTTON1)
INPUT_PORTS_END

static const gfx_layout cgenie_charlayout =
{
	8,8,		   /* 8*8 characters */
	384,		   /* 256 fixed + 128 defineable characters */
	1,			   /* 1 bits per pixel */
	{ 0 },		   /* no bitplanes; 1 bit per pixel */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },   /* x offsets */
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	8*8 		   /* every char takes 8 bytes */
};

static const gfx_layout cgenie_gfxlayout =
{
	8,8,			/* 4*8 characters */
	256,			/* 256 graphics patterns */
	2,				/* 2 bits per pixel */
	{ 0, 1 },		/* two bitplanes; 2 bit per pixel */
	{ 0, 0, 2, 2, 4, 4, 6, 6}, /* x offsets */
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	8*8 			/* every char takes 8 bytes */
};

static GFXDECODE_START( cgenie )
	GFXDECODE_ENTRY( REGION_GFX1, 0, cgenie_charlayout, 0, 3*16 )
	GFXDECODE_ENTRY( REGION_GFX2, 0, cgenie_gfxlayout, 3*16*2, 3*4 )
GFXDECODE_END

static const unsigned char cgenie_colors[] = {
	 0*4,  0*4,  0*4,  /* background   */

/* this is the 'RGB monitor' version, strong and clean */
	15*4, 15*4, 15*4,  /* gray         */
	 0*4, 48*4, 48*4,  /* cyan         */
	60*4,  0*4,  0*4,  /* red          */
	47*4, 47*4, 47*4,  /* white        */
	55*4, 55*4,  0*4,  /* yellow       */
	 0*4, 56*4,  0*4,  /* green        */
	42*4, 32*4,  0*4,  /* orange       */
	63*4, 63*4,  0*4,  /* light yellow */
	 0*4,  0*4, 48*4,  /* blue         */
	 0*4, 24*4, 63*4,  /* light blue   */
	60*4,  0*4, 38*4,  /* pink         */
	38*4,  0*4, 60*4,  /* purple       */
	31*4, 31*4, 31*4,  /* light gray   */
	 0*4, 63*4, 63*4,  /* light cyan   */
	58*4,  0*4, 58*4,  /* magenta      */
	63*4, 63*4, 63*4,  /* bright white */

/* this is the 'TV screen' version, weak and blurred by repeating pixels */
	15*2+80, 15*2+80, 15*2+80,	/* gray         */
	 0*2+80, 48*2+80, 48*2+80,	/* cyan         */
	60*2+80,  0*2+80,  0*2+80,	/* red          */
	47*2+80, 47*2+80, 47*2+80,	/* white        */
	55*2+80, 55*2+80,  0*2+80,	/* yellow       */
	 0*2+80, 56*2+80,  0*2+80,	/* green        */
	42*2+80, 32*2+80,  0*2+80,	/* orange       */
	63*2+80, 63*2+80,  0*2+80,	/* light yellow */
	 0*2+80,  0*2+80, 48*2+80,	/* blue         */
	 0*2+80, 24*2+80, 63*2+80,	/* light blue   */
	60*2+80,  0*2+80, 38*2+80,	/* pink         */
	38*2+80,  0*2+80, 60*2+80,	/* purple       */
	31*2+80, 31*2+80, 31*2+80,	/* light gray   */
	 0*2+80, 63*2+80, 63*2+80,	/* light cyan   */
	58*2+80,  0*2+80, 58*2+80,	/* magenta      */
	63*2+80, 63*2+80, 63*2+80,	/* bright white */

	15*2+96, 15*2+96, 15*2+96,	/* gray         */
	 0*2+96, 48*2+96, 48*2+96,	/* cyan         */
	60*2+96,  0*2+96,  0*2+96,	/* red          */
	47*2+96, 47*2+96, 47*2+96,	/* white        */
	55*2+96, 55*2+96,  0*2+96,	/* yellow       */
	 0*2+96, 56*2+96,  0*2+96,	/* green        */
	42*2+96, 32*2+96,  0*2+96,	/* orange       */
	63*2+96, 63*2+96,  0*2+96,	/* light yellow */
	 0*2+96,  0*2+96, 48*2+96,	/* blue         */
	 0*2+96, 24*2+96, 63*2+96,	/* light blue   */
	60*2+96,  0*2+96, 38*2+96,	/* pink         */
	38*2+96,  0*2+96, 60*2+96,	/* purple       */
	31*2+96, 31*2+96, 31*2+96,	/* light gray   */
	 0*2+96, 63*2+96, 63*2+96,	/* light cyan   */
	58*2+96,  0*2+96, 58*2+96,	/* magenta      */
	63*2+96, 63*2+96, 63*2+96,	/* bright white */


};

static const unsigned short cgenie_palette[] =
{
	0, 1, 0, 2, 0, 3, 0, 4, /* RGB monitor set of text colors */
	0, 5, 0, 6, 0, 7, 0, 8,
	0, 9, 0,10, 0,11, 0,12,
	0,13, 0,14, 0,15, 0,16,

	0,17, 0,18, 0,19, 0,20, /* TV set text colors: darker */
	0,21, 0,22, 0,23, 0,24,
	0,25, 0,26, 0,27, 0,28,
	0,29, 0,30, 0,31, 0,32,

	0,33, 0,34, 0,35, 0,36, /* TV set text colors: a bit brighter */
	0,37, 0,38, 0,39, 0,40,
	0,41, 0,42, 0,43, 0,44,
	0,45, 0,46, 0,47, 0,48,

	0,	  9,	7,	  6,	/* RGB monitor graphics colors */
	0,	  25,	23,   22,	/* TV set graphics colors: darker */
	0,	  41,	39,   38,	/* TV set graphics colors: a bit brighter */
};

/* Initialise the palette */
static PALETTE_INIT( cgenie )
{
	UINT8 i, r, g, b;

	machine->colortable = colortable_alloc(machine, 49);

	for ( i = 0; i < 49; i++ )
	{
		r = cgenie_colors[i*3]; g = cgenie_colors[i*3+1]; b = cgenie_colors[i*3+2];
		colortable_palette_set_color(machine->colortable, i, MAKE_RGB(r, g, b));
	}

	for(i=0; i<108; i++)
		colortable_entry_set_value(machine->colortable, i, cgenie_palette[i]);
}


static const struct AY8910interface ay8910_interface =
{
	AY8910_LEGACY_OUTPUT,
	AY8910_DEFAULT_LOADS,
	cgenie_psg_port_a_r,
	cgenie_psg_port_b_r,
	cgenie_psg_port_a_w,
	cgenie_psg_port_b_w
};



static MACHINE_DRIVER_START( cgenie )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 2216800)        /* 2,2168 Mhz */
	MDRV_CPU_PROGRAM_MAP(cgenie_mem, 0)
	MDRV_CPU_IO_MAP(cgenie_io, 0)
	MDRV_CPU_VBLANK_INT("main", cgenie_frame_interrupt)
	MDRV_CPU_PERIODIC_INT(cgenie_timer_interrupt, 40)
	MDRV_INTERLEAVE(4)

	MDRV_MACHINE_START( cgenie )
	MDRV_MACHINE_RESET( cgenie )

    /* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(48*8, (32)*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 48*8-1,0*8,32*8-1)
	MDRV_GFXDECODE( cgenie )
	MDRV_PALETTE_LENGTH(108)
	MDRV_PALETTE_INIT( cgenie )

	MDRV_VIDEO_START( cgenie )
	MDRV_VIDEO_UPDATE( cgenie )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START (cgenie)
	ROM_REGION(0x13000,REGION_CPU1,0)
	ROM_LOAD ("cgenie.rom",  0x00000, 0x4000, CRC(d359ead7) SHA1(d8c2fc389ad38c45fba0ed556a7d91abac5463f4))
	ROM_LOAD ("cgdos.rom",   0x10000, 0x2000, CRC(2a96cf74) SHA1(6dcac110f87897e1ee7521aefbb3d77a14815893))
	ROM_CART_LOAD(0, "rom", 0x12000, 0x1000, ROM_NOMIRROR | ROM_OPTIONAL)

	ROM_REGION(0x0c00,REGION_GFX1,0)
	ROM_LOAD ("cgenie1.fnt", 0x0000, 0x0800, CRC(4fed774a) SHA1(d53df8212b521892cc56be690db0bb474627d2ff))

	/* Empty memory region for the character generator */
	ROM_REGION(0x0800,REGION_GFX2,ROMREGION_ERASEFF)

ROM_END

static void cgenie_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(cgenie_floppy); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "dsk"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

static void cgenie_cassette_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_TYPE:							info->i = IO_CASSETTE; break;
		case MESS_DEVINFO_INT_READABLE:						info->i = 0;	/* INVALID */ break;
		case MESS_DEVINFO_INT_WRITEABLE:						info->i = 0;	/* INVALID */ break;
		case MESS_DEVINFO_INT_CREATABLE:						info->i = 0;	/* INVALID */ break;
		case MESS_DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(cgenie_cassette); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "cas"); break;
	}
}

SYSTEM_CONFIG_START(cgenie)
	CONFIG_DEVICE(cartslot_device_getinfo)
	CONFIG_DEVICE(cgenie_floppy_getinfo)
	CONFIG_DEVICE(cgenie_cassette_getinfo)
	CONFIG_RAM_DEFAULT	(16 * 1024)
	CONFIG_RAM			(32 * 1024)
SYSTEM_CONFIG_END

/*    YEAR  NAME      PARENT    COMPAT  MACHINE   INPUT     INIT      CONFIG     COMPANY    FULLNAME */
COMP( 1982, cgenie,   0,		0,		cgenie,   cgenie,	0,        cgenie,    "EACA Computers Ltd.",  "Colour Genie EG2000" , 0)
