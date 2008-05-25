/*
** svi318.c : driver for Spectravideo SVI-318 and SVI-328
**
** Sean Young, Tomas Karlsson
**
** Information taken from: http://www.samdal.com/sv318.htm
**
** SV-318 : 16KB Video RAM, 16KB RAM
** SV-328 : 16KB Video RAM, 64KB RAM
*/

#include "driver.h"
#include "video/mc6845.h"
#include "includes/svi318.h"
#include "video/tms9928a.h"
#include "machine/8255ppi.h"
#include "devices/basicdsk.h"
#include "devices/printer.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "formats/svi_cas.h"
#include "sound/dac.h"
#include "sound/ay8910.h"
#include "sv328806.lh"

static ADDRESS_MAP_START( svi318_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x0000, 0x7fff) AM_READWRITE( SMH_BANK1, svi318_writemem1 )
	AM_RANGE( 0x8000, 0xbfff) AM_READWRITE( SMH_BANK2, svi318_writemem2 )
	AM_RANGE( 0xc000, 0xffff) AM_READWRITE( SMH_BANK3, svi318_writemem3 )
ADDRESS_MAP_END

static ADDRESS_MAP_START( svi328_806_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x0000, 0x7fff) AM_READWRITE( SMH_BANK1, svi318_writemem1 )
	AM_RANGE( 0x8000, 0xbfff) AM_READWRITE( SMH_BANK2, svi318_writemem2 )
	AM_RANGE( 0xc000, 0xefff) AM_READWRITE( SMH_BANK3, svi318_writemem3 )
	AM_RANGE( 0xf000, 0xffff) AM_READWRITE( SMH_BANK4, svi318_writemem4 )
ADDRESS_MAP_END

static ADDRESS_MAP_START( svi318_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE( 0x00, 0x38) AM_READWRITE( svi318_io_ext_r, svi318_io_ext_w )
	AM_RANGE( 0x80, 0x80) AM_WRITE( TMS9928A_vram_w )
	AM_RANGE( 0x81, 0x81) AM_WRITE( TMS9928A_register_w )
	AM_RANGE( 0x84, 0x84) AM_READ( TMS9928A_vram_r )
	AM_RANGE( 0x85, 0x85) AM_READ( TMS9928A_register_r )
	AM_RANGE( 0x88, 0x88) AM_WRITE( AY8910_control_port_0_w )
	AM_RANGE( 0x8c, 0x8c) AM_WRITE( AY8910_write_port_0_w )
	AM_RANGE( 0x90, 0x90) AM_READ( AY8910_read_port_0_r )
	AM_RANGE( 0x96, 0x97) AM_DEVWRITE( PPI8255, "ppi8255", svi318_ppi_w )
	AM_RANGE( 0x98, 0x9a) AM_DEVREAD( PPI8255, "ppi8255", svi318_ppi_r )
ADDRESS_MAP_END

static ADDRESS_MAP_START( svi328_806_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE( 0x00, 0x58) AM_READWRITE( svi318_io_ext_r, svi318_io_ext_w )
	AM_RANGE( 0x80, 0x80) AM_WRITE( TMS9928A_vram_w )
	AM_RANGE( 0x81, 0x81) AM_WRITE( TMS9928A_register_w )
	AM_RANGE( 0x84, 0x84) AM_READ( TMS9928A_vram_r )
	AM_RANGE( 0x85, 0x85) AM_READ( TMS9928A_register_r )
	AM_RANGE( 0x88, 0x88) AM_WRITE( AY8910_control_port_0_w )
	AM_RANGE( 0x8c, 0x8c) AM_WRITE( AY8910_write_port_0_w )
	AM_RANGE( 0x90, 0x90) AM_READ( AY8910_read_port_0_r )
	AM_RANGE( 0x96, 0x97) AM_DEVWRITE( PPI8255, "ppi8255", svi318_ppi_w )
	AM_RANGE( 0x98, 0x9a) AM_DEVREAD( PPI8255, "ppi8255", svi318_ppi_r )
ADDRESS_MAP_END

/*
  Keyboard status table
       Bit#:|  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
            |     |     |     |     |     |     |     |     |
  Line:     |     |     |     |     |     |     |     |     |
   ---------+-----+-----+-----+-----+-----+-----+-----+-----|
    0       | "7" | "6" | "5" | "4" | "3" | "2" | "1" | "0" |
   ---------+-----+-----+-----+-----+-----+-----+-----+-----|
    1       | "/" | "." | "=" | "," | "'" | ":" | "9" | "8" |
   ---------+-----+-----+-----+-----+-----+-----+-----+-----|
    2       | "G" | "F" | "E" | "D" | "C" | "B" | "A" | "-" |
   ---------+-----+-----+-----+-----+-----+-----+-----+-----|
    3       | "O" | "N" | "M" | "L" | "K" | "J" | "I" | "H" |
   ---------+-----+-----+-----+-----+-----+-----+-----+-----|
    4       | "W" | "V" | "U" | "T" | "S" | "R" | "Q" | "P" |
   ---------+-----+-----+-----+-----+-----+-----+-----+-----|
    5       | UP  | BS  | "]" | "\" | "[" | "Z" | "Y" | "X" |
   ---------+-----+-----+-----+-----+-----+-----+-----+-----|
    6       |LEFT |ENTER|STOP | ESC |RGRAP|LGRAP|CTRL |SHIFT|
   ---------+-----+-----+-----+-----+-----+-----+-----+-----|
    7       |DOWN | INS | CLS | F5  | F4  | F3  | F2  | F1  |
   ---------+-----+-----+-----+-----+-----+-----+-----+-----|
    8       |RIGHT|     |PRINT| SEL |CAPS | DEL | TAB |SPACE|
   ---------+-----+-----+-----+-----+-----+-----+-----+-----|
    9*      | "7" | "6" | "5" | "4" | "3" | "2" | "1" | "0" |
   ---------+-----+-----+-----+-----+-----+-----+-----+-----|
   10*      | "," | "." | "/" | "*" | "-" | "+" | "9" | "8" |
   ----------------------------------------------------------
   * Numcerical keypad (SVI-328 only)

2008-05 FP:
Small note about natural keyboard: currently,
- "Keypad ," is not mapped
- "Left Grph" and "Right Grph" are mapped to 'Page Up' and 'Page Down'
- "Stop" is mapped to 'End'
- "Select" is mapped to 'F11'
- "CLS/HM" is mapped to 'Home'
TODO: How are multiple keys (Copy, Cut, Paste, CLS/HM) expected to 
behave? Do they need multiple mapping in natural keyboard?
*/

static INPUT_PORTS_START( svi318 )
	PORT_START_TAG("LINE0")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0)			PORT_CHAR('0') PORT_CHAR(')')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) 			PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) 			PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) 			PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) 			PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5) 			PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) 			PORT_CHAR('6') PORT_CHAR('^')
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) 			PORT_CHAR('7') PORT_CHAR('&')

	PORT_START_TAG("LINE1")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8) 			PORT_CHAR('8') PORT_CHAR('*')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9) 			PORT_CHAR('9') PORT_CHAR('(')
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON)		PORT_CHAR(':') PORT_CHAR(';')
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE)		PORT_CHAR('\'') PORT_CHAR('"')
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA)		PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS)		PORT_CHAR('=') PORT_CHAR('+')
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP)		PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH)		PORT_CHAR('/') PORT_CHAR('?')

	PORT_START_TAG("LINE2")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS)		PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A) 			PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) 			PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) 			PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) 			PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) 			PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) 			PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) 			PORT_CHAR('g') PORT_CHAR('G')

	PORT_START_TAG("LINE3")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H) 			PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) 			PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) 			PORT_CHAR('0') PORT_CHAR('J')
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) 			PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) 			PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M) 			PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N) 			PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) 			PORT_CHAR('o') PORT_CHAR('O')

	PORT_START_TAG("LINE4")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P) 			PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) 			PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) 			PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) 			PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) 			PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U) 			PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) 			PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) 			PORT_CHAR('w') PORT_CHAR('W')

	PORT_START_TAG("LINE5")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) 			PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) 			PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z) 			PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE)	PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH)	PORT_CHAR('\\') PORT_CHAR('~')
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE)	PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSPACE)	PORT_CHAR(8)
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP)		PORT_CHAR(UCHAR_MAMEKEY(UP))

	PORT_START_TAG("LINE6")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT)	PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_LCONTROL)			PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Left Grph") PORT_CODE(KEYCODE_LALT)			PORT_CHAR(UCHAR_MAMEKEY(PGUP))
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Rright Grph") PORT_CODE(KEYCODE_RALT)		PORT_CHAR(UCHAR_MAMEKEY(PGDN))
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ESC)									PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Stop") PORT_CODE(KEYCODE_END)				PORT_CHAR(UCHAR_MAMEKEY(END))
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER)								PORT_CHAR(13)
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_LEFT)		PORT_CHAR(UCHAR_MAMEKEY(LEFT))

	PORT_START_TAG("LINE7")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F1  F6") PORT_CODE(KEYCODE_F1)				PORT_CHAR(UCHAR_MAMEKEY(F1)) PORT_CHAR(UCHAR_MAMEKEY(F6))
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F2  F7") PORT_CODE(KEYCODE_F2) 				PORT_CHAR(UCHAR_MAMEKEY(F2)) PORT_CHAR(UCHAR_MAMEKEY(F7))
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F3  F8") PORT_CODE(KEYCODE_F3) 				PORT_CHAR(UCHAR_MAMEKEY(F3)) PORT_CHAR(UCHAR_MAMEKEY(F8))
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F4  F9") PORT_CODE(KEYCODE_F4) 				PORT_CHAR(UCHAR_MAMEKEY(F4)) PORT_CHAR(UCHAR_MAMEKEY(F9))
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F5  F10") PORT_CODE(KEYCODE_F5) 			PORT_CHAR(UCHAR_MAMEKEY(F5)) PORT_CHAR(UCHAR_MAMEKEY(F10))
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CLS/HM  Copy") PORT_CODE(KEYCODE_HOME)		PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Ins  Paste") PORT_CODE(KEYCODE_INSERT)		PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x93") PORT_CODE(KEYCODE_DOWN)		PORT_CHAR(UCHAR_MAMEKEY(DOWN))

	PORT_START_TAG("LINE8")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE)								PORT_CHAR(' ')
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TAB)									PORT_CHAR('\t')
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Del  Cut") PORT_CODE(KEYCODE_DEL)			PORT_CHAR(UCHAR_MAMEKEY(DEL))
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Caps Lock") PORT_CODE(KEYCODE_CAPSLOCK)		PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Select") PORT_CODE(KEYCODE_PAUSE)			PORT_CHAR(UCHAR_MAMEKEY(F11))
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Print") PORT_CODE(KEYCODE_PRTSCR)			PORT_CHAR(UCHAR_MAMEKEY(PRTSCR))
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT)		PORT_CHAR(UCHAR_MAMEKEY(RIGHT))

	PORT_START_TAG("LINE9")
	PORT_BIT (0xff, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START_TAG("LINE10")
	PORT_BIT (0xff, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START_TAG("JOYSTICKS")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP)
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN)
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT)
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT)
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_PLAYER(2)
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_PLAYER(2)
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_PLAYER(2)
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(2)

	PORT_START_TAG("BUTTONS")
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_BUTTON1)
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(2)

	PORT_START_TAG("CONFIG")
	PORT_DIPNAME( 0x20, 0x20, "Enforce 4 sprites/line")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Yes ) )
INPUT_PORTS_END


static INPUT_PORTS_START( svi328 )

	PORT_INCLUDE( svi318 )

	PORT_MODIFY("LINE9")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0_PAD)			PORT_CHAR(UCHAR_MAMEKEY(0_PAD))
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1_PAD)			PORT_CHAR(UCHAR_MAMEKEY(1_PAD))
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2_PAD)			PORT_CHAR(UCHAR_MAMEKEY(2_PAD))
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3_PAD)			PORT_CHAR(UCHAR_MAMEKEY(3_PAD))
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4_PAD)			PORT_CHAR(UCHAR_MAMEKEY(4_PAD))
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5_PAD)			PORT_CHAR(UCHAR_MAMEKEY(5_PAD))
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6_PAD)			PORT_CHAR(UCHAR_MAMEKEY(6_PAD))
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7_PAD)			PORT_CHAR(UCHAR_MAMEKEY(7_PAD))

	PORT_MODIFY("LINE10")
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8_PAD)			PORT_CHAR(UCHAR_MAMEKEY(8_PAD))
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9_PAD)			PORT_CHAR(UCHAR_MAMEKEY(9_PAD))
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_PLUS_PAD)		PORT_CHAR(UCHAR_MAMEKEY(PLUS_PAD))
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS_PAD)		PORT_CHAR(UCHAR_MAMEKEY(MINUS_PAD))
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ASTERISK) 		PORT_CHAR(UCHAR_MAMEKEY(ASTERISK))
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH_PAD)		PORT_CHAR(UCHAR_MAMEKEY(SLASH_PAD))
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER_PAD)		PORT_CHAR(UCHAR_MAMEKEY(DEL_PAD))
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Keypad ,") PORT_CODE(KEYCODE_DEL_PAD)
INPUT_PORTS_END

static const struct AY8910interface ay8910_interface =
{
	AY8910_LEGACY_OUTPUT,
	AY8910_DEFAULT_LOADS,
	svi318_psg_port_a_r,
	NULL,
	NULL,
	svi318_psg_port_b_w
};

static MACHINE_DRIVER_START( svi318 )
	/* Basic machine hardware */
	MDRV_CPU_ADD_TAG( "main", Z80, 3579545 )	/* 3.579545 Mhz */
	MDRV_CPU_PROGRAM_MAP( svi318_mem, 0 )
	MDRV_CPU_IO_MAP( svi318_io, 0 )
	MDRV_CPU_VBLANK_INT("main", svi318_interrupt)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_START( svi318_pal )
	MDRV_MACHINE_RESET( svi318 )

	MDRV_DEVICE_ADD( "ppi8255", PPI8255 )
	MDRV_DEVICE_CONFIG( svi318_ppi8255_interface )

	/* Video hardware */
	MDRV_IMPORT_FROM(tms9928a)
	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

	/* Sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD(WAVE, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD(AY8910, 1789773)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)

	/* printer */
	MDRV_DEVICE_ADD("printer", PRINTER)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( svi318n )
	MDRV_IMPORT_FROM( svi318 )

	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_REFRESH_RATE(60)

	MDRV_MACHINE_START( svi318_ntsc )
	MDRV_MACHINE_RESET( svi318 )
MACHINE_DRIVER_END

static const mc6845_interface svi806_crtc6845_interface =
{
	"svi806",
	XTAL_12MHz / 8,
	8 /*?*/,
	NULL,
	svi806_crtc6845_update_row,
	NULL,
	NULL,
	NULL,
	NULL
};

static MACHINE_DRIVER_START( svi328_806 )
	/* Basic machine hardware */
	MDRV_CPU_ADD_TAG( "main", Z80, 3579545 )	/* 3.579545 Mhz */
	MDRV_CPU_PROGRAM_MAP( svi328_806_mem, 0 )
	MDRV_CPU_IO_MAP( svi328_806_io, 0 )
	MDRV_CPU_VBLANK_INT("main", svi318_interrupt)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_START( svi318_pal )
	MDRV_MACHINE_RESET( svi328_806 )

	MDRV_DEVICE_ADD( "ppi8255", PPI8255 )
	MDRV_DEVICE_CONFIG( svi318_ppi8255_interface )

	MDRV_DEVICE_ADD( "ins8250_0", INS8250 )
	MDRV_DEVICE_CONFIG( svi318_ins8250_interface[0] )

	MDRV_DEVICE_ADD( "ins8250_1", INS8250 )
	MDRV_DEVICE_CONFIG( svi318_ins8250_interface[1] )

	/* Video hardware */
	MDRV_DEFAULT_LAYOUT( layout_sv328806 )

	MDRV_IMPORT_FROM(tms9928a)
	MDRV_SCREEN_MODIFY("main")
	MDRV_PALETTE_LENGTH(TMS9928A_PALETTE_SIZE + 2)	/* 2 additional entries for monochrome svi806 output */
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

	MDRV_SCREEN_ADD("svi806", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MDRV_SCREEN_SIZE(640, 400)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 400-1)

	MDRV_DEVICE_ADD("crtc", MC6845)
	MDRV_DEVICE_CONFIG( svi806_crtc6845_interface )

	MDRV_VIDEO_START( svi328_806 )
	MDRV_VIDEO_UPDATE( svi328_806 )

	/* Sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD(WAVE, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD(AY8910, 1789773)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)

	/* printer */
	MDRV_DEVICE_ADD("printer", PRINTER)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( svi328n_806 )
	MDRV_IMPORT_FROM( svi328_806 )

	MDRV_MACHINE_START( svi318_ntsc)

	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_REFRESH_RATE(60)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( svi318 )
	ROM_REGION(0x10000, REGION_CPU1, 0)
	ROM_SYSTEM_BIOS(0, "111", "SV BASIC v1.11")
	ROMX_LOAD("svi111.rom", 0x0000, 0x8000, CRC(bc433df6) SHA1(10349ce675f6d6d47f0976e39cb7188eba858d89), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "110", "SV BASIC v1.1")
	ROMX_LOAD("svi110.rom", 0x0000, 0x8000, CRC(709904e9) SHA1(7d8daf52f78371ca2c9443e04827c8e1f98c8f2c), ROM_BIOS(2))
	ROM_SYSTEM_BIOS(2, "100", "SV BASIC v1.0")
	ROMX_LOAD("svi100.rom", 0x0000, 0x8000, CRC(98d48655) SHA1(07758272df475e5e06187aa3574241df1b14035b), ROM_BIOS(3))
ROM_END

ROM_START( svi318n  )
	ROM_REGION(0x10000, REGION_CPU1, 0)
	ROM_SYSTEM_BIOS(0, "111", "SV BASIC v1.11")
	ROMX_LOAD("svi111.rom", 0x0000, 0x8000, CRC(bc433df6) SHA1(10349ce675f6d6d47f0976e39cb7188eba858d89), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "110", "SV BASIC v1.1")
	ROMX_LOAD("svi110.rom", 0x0000, 0x8000, CRC(709904e9) SHA1(7d8daf52f78371ca2c9443e04827c8e1f98c8f2c), ROM_BIOS(2))
	ROM_SYSTEM_BIOS(2, "100", "SV BASIC v1.0")
	ROMX_LOAD("svi100.rom", 0x0000, 0x8000, CRC(98d48655) SHA1(07758272df475e5e06187aa3574241df1b14035b), ROM_BIOS(3))
ROM_END

ROM_START( svi328 )
	ROM_REGION(0x10000, REGION_CPU1, 0)
	ROM_SYSTEM_BIOS(0, "111", "SV BASIC v1.11")
	ROMX_LOAD("svi111.rom", 0x0000, 0x8000, CRC(bc433df6) SHA1(10349ce675f6d6d47f0976e39cb7188eba858d89), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "110", "SV BASIC v1.1")
	ROMX_LOAD("svi110.rom", 0x0000, 0x8000, CRC(709904e9) SHA1(7d8daf52f78371ca2c9443e04827c8e1f98c8f2c), ROM_BIOS(2))
	ROM_SYSTEM_BIOS(2, "100", "SV BASIC v1.0")
	ROMX_LOAD("svi100.rom", 0x0000, 0x8000, CRC(98d48655) SHA1(07758272df475e5e06187aa3574241df1b14035b), ROM_BIOS(3))
ROM_END

ROM_START( svi328n )
	ROM_REGION(0x10000, REGION_CPU1, 0)
	ROM_SYSTEM_BIOS(0, "111", "SV BASIC v1.11")
	ROMX_LOAD("svi111.rom", 0x0000, 0x8000, CRC(bc433df6) SHA1(10349ce675f6d6d47f0976e39cb7188eba858d89), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "110", "SV BASIC v1.1")
	ROMX_LOAD("svi110.rom", 0x0000, 0x8000, CRC(709904e9) SHA1(7d8daf52f78371ca2c9443e04827c8e1f98c8f2c), ROM_BIOS(2))
	ROM_SYSTEM_BIOS(2, "100", "SV BASIC v1.0")
	ROMX_LOAD("svi100.rom", 0x0000, 0x8000, CRC(98d48655) SHA1(07758272df475e5e06187aa3574241df1b14035b), ROM_BIOS(3))
ROM_END

ROM_START( sv328p80 )
	ROM_REGION (0x10000, REGION_CPU1,0)
	ROM_LOAD ("svi111.rom", 0x0000, 0x8000, CRC(bc433df6) SHA1(10349ce675f6d6d47f0976e39cb7188eba858d89))
	ROM_REGION( 0x1000, REGION_GFX1, 0)
	ROM_SYSTEM_BIOS(0, "english", "English Character Set")
	ROMX_LOAD ("svi806.rom",   0x0000, 0x1000, CRC(850bc232) SHA1(ed45cb0e9bd18a9d7bd74f87e620f016a7ae840f), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "swedish", "Swedish Character Set")
	ROMX_LOAD ("svi806se.rom", 0x0000, 0x1000, CRC(daea8956) SHA1(3f16d5513ad35692488ae7d864f660e76c6e8ed3), ROM_BIOS(2))
ROM_END

ROM_START( sv328n80 )
	ROM_REGION (0x10000, REGION_CPU1,0)
	ROM_LOAD ("svi111.rom", 0x0000, 0x8000, CRC(bc433df6) SHA1(10349ce675f6d6d47f0976e39cb7188eba858d89))
	ROM_REGION( 0x1000, REGION_GFX1, 0)
	ROM_SYSTEM_BIOS(0, "english", "English Character Set")
	ROMX_LOAD ("svi806.rom",   0x0000, 0x1000, CRC(850bc232) SHA1(ed45cb0e9bd18a9d7bd74f87e620f016a7ae840f), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "swedish", "Swedish Character Set")
	ROMX_LOAD ("svi806se.rom", 0x0000, 0x1000, CRC(daea8956) SHA1(3f16d5513ad35692488ae7d864f660e76c6e8ed3), ROM_BIOS(2))
ROM_END

static void svi318_cassette_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:
			info->i = 1;
			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_CASSETTE_FORMATS:
			info->p = (void *) svi_cassette_formats;
			break;

		default:
			cassette_device_getinfo(devclass, state, info);
			break;
	}
}

static void svi318_cartslot_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:
			info->i = 1;
			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_START:
			info->start = DEVICE_START_NAME(svi318_cart);
			break;
		case MESS_DEVINFO_PTR_LOAD:
			info->load = DEVICE_IMAGE_LOAD_NAME(svi318_cart);
			break;
		case MESS_DEVINFO_PTR_UNLOAD:
			info->unload = DEVICE_IMAGE_UNLOAD_NAME(svi318_cart);
			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:
			strcpy(info->s = device_temp_str(), "rom");
			break;

		default:
			cartslot_device_getinfo(devclass, state, info);
			break;
	}
}

static void svi318_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:
			info->i = 2;
			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:
			info->load = DEVICE_IMAGE_LOAD_NAME(svi318_floppy);
			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:
			strcpy(info->s = device_temp_str(), "dsk");
			break;

		default:
			legacybasicdsk_device_getinfo(devclass, state, info);
			break;
	}
}

SYSTEM_CONFIG_START( svi318_common )
	CONFIG_DEVICE(svi318_cassette_getinfo)
	CONFIG_DEVICE(svi318_cartslot_getinfo)
	CONFIG_DEVICE(svi318_floppy_getinfo)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START( svi318 )
	CONFIG_IMPORT_FROM(svi318_common)
	CONFIG_RAM_DEFAULT(16 * 1024)
	CONFIG_RAM( 32 * 1024 )
	CONFIG_RAM( 96 * 1024 )
	CONFIG_RAM( 160 * 1024 )
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START( svi328 )
	CONFIG_IMPORT_FROM(svi318_common)
	CONFIG_RAM_DEFAULT(64 * 1024)
	CONFIG_RAM( 96 * 1024 )
	CONFIG_RAM( 160 * 1024 )
SYSTEM_CONFIG_END

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT   INIT    CONFIG  COMPANY         FULLNAME                    FLAGS */
COMP( 1983, svi318,     0,      0,      svi318,     svi318, svi318, svi318, "Spectravideo", "SVI-318 (PAL)",            0 )
COMP( 1983, svi318n,    svi318, 0,      svi318n,    svi318, svi318, svi318, "Spectravideo", "SVI-318 (NTSC)",           0 )
COMP( 1983, svi328,     svi318, 0,      svi318,     svi328, svi318, svi328, "Spectravideo", "SVI-328 (PAL)",            0 )
COMP( 1983, svi328n,    svi318, 0,      svi318n,    svi328, svi318, svi328, "Spectravideo", "SVI-328 (NTSC)",           0 )
COMP( 1983, sv328p80,   svi318, 0,      svi328_806,    svi328, svi318, svi328, "Spectravideo", "SVI-328 (PAL) + SVI-806 80 column card", 0 )
COMP( 1983, sv328n80,   svi318, 0,      svi328n_806,   svi328, svi318, svi328, "Spectravideo", "SVI-328 (NTSC) + SVI-806 80 column card", 0 )
