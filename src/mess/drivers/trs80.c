/***************************************************************************
TRS80 memory map

0000-2fff ROM                 R   D0-D7
3000-37ff ROM on Model III        R   D0-D7
          unused on Model I
37de      UART status             R/W D0-D7
37df      UART data           R/W D0-D7
37e0      interrupt latch address (lnw80 = for the realtime clock)
37e1      select disk drive 0         W
37e2      cassette drive latch address    W
37e3      select disk drive 1         W
37e4      select which cassette unit      W   D0-D1 (D0 selects unit 1, D1 selects unit 2)
37e5      select disk drive 2         W
37e7      select disk drive 3         W
37e0-37e3 floppy motor            W   D0-D3
          or floppy head select   W   D3
37e8      send a byte to printer          W   D0-D7
37e8      read printer status             R   D7
37ec-37ef FDC WD179x              R/W D0-D7
37ec      command             W   D0-D7
37ec      status              R   D0-D7
37ed      track               R/W D0-D7
37ee      sector              R/W D0-D7
37ef      data                R/W D0-D7
3800-38ff keyboard matrix         R   D0-D7
3900-3bff unused - kbd mirrored
3c00-3fff video RAM               R/W D0-D5,D7 (or D0-D7)
4000-ffff RAM

Interrupts:
IRQ mode 1
NMI

I/O ports
FF:
- bits 0 and 1 are for writing a cassette
- bit 2 must be high to turn the cassette player on, enables cassette data paths on a system-80
- bit 3 switches the display between 64 or 32 characters per line
- bit 7 is for reading from a cassette
FE:
- bit 0 is for selecting inverse video of the whole screen on a lnw80
- bit 2 enables colour on a lnw80
- bit 3 is for selecting roms (low) or 16k hires area (high) on a lnw80
- bit 4 selects internal cassette player (low) or external unit (high) on a system-80
FD:
- bit 7 for reading the printer status on a system-80
- all bits for writing to a printer on a system-80
F9:
- UART data (write) status (read) on a system-80
F8:
- UART data (read) status (write) on a system-80
EB:
- UART data (read and write) on a Model III/4
EA:
- UART status (read and write) on a Model III/4
E9:
- UART Configuration jumpers (read) on a Model III/4
E8:
- UART Modem Status register (read) on a Model III/4
- UART Master Reset (write) on a Model III/4
***************************************************************************

Not dumped:
 TRS80 Japanese bios
 TRS80 Katakana Character Generator
 TRS80 Model III and 4 Character Generators

Not emulated:
 TRS80 Japanese kana/ascii switch and alternate keyboard
 TRS80 Model III and 4 hardware above what is in a Model I.
 LNW80 1.77 / 4.0 MHz switch
 LNW80 Colour board
 LNW80 Hires graphics
 LNW80 24x80 screen

***************************************************************************/

/* Core includes */
#include "driver.h"
#include "deprecat.h"
#include "includes/trs80.h"

/* Components */
#include "machine/wd17xx.h"
#include "sound/speaker.h"

/* Devices */
#include "devices/basicdsk.h"
#include "devices/cassette.h"
#include "formats/trs_cas.h"


#define FW	TRS80_FONT_W
#define FH	TRS80_FONT_H

static READ8_HANDLER (trs80_wd179x_r)
{
	if (input_port_read(space->machine, "CONFIG") & 0x80)
	{
		return wd17xx_status_r(space, offset);
	}
	else
	{
		return 0xff;
	}
}

static ADDRESS_MAP_START( mem_level1, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x3800, 0x38ff) AM_READ(trs80_keyboard_r)
	AM_RANGE(0x3c00, 0x3fff) AM_READWRITE(SMH_RAM, trs80_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x4000, 0x7fff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( io_level1, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0xfe, 0xfe) AM_READ(trs80_port_xx_r)
	AM_RANGE(0xff, 0xff) AM_READWRITE(trs80_port_ff_r, trs80_port_ff_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mem_model1, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x2fff) AM_ROM
	AM_RANGE(0x3000, 0x37df) AM_NOP
	AM_RANGE(0x37e0, 0x37e3) AM_READWRITE(trs80_irq_status_r, trs80_motor_w)
	AM_RANGE(0x37e4, 0x37e7) AM_NOP
	AM_RANGE(0x37e8, 0x37eb) AM_READWRITE(trs80_printer_r, trs80_printer_w)
	AM_RANGE(0x37ec, 0x37ec) AM_READWRITE(trs80_wd179x_r, wd17xx_command_w)
	AM_RANGE(0x37ed, 0x37ed) AM_READWRITE(wd17xx_track_r, wd17xx_track_w)
	AM_RANGE(0x37ee, 0x37ee) AM_READWRITE(wd17xx_sector_r, wd17xx_sector_w)
	AM_RANGE(0x37ef, 0x37ef) AM_READWRITE(wd17xx_data_r, wd17xx_data_w)
	AM_RANGE(0x37f0, 0x37ff) AM_NOP
	AM_RANGE(0x3800, 0x38ff) AM_READ(trs80_keyboard_r)
	AM_RANGE(0x3900, 0x3bff) AM_NOP
	AM_RANGE(0x3c00, 0x3fff) AM_READWRITE(SMH_RAM, trs80_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x4000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( io_model1, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0xfe, 0xfe) AM_READ(trs80_port_xx_r)
	AM_RANGE(0xff, 0xff) AM_READWRITE(trs80_port_ff_r, trs80_port_ff_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mem_model3, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x37ff) AM_ROM
	AM_RANGE(0x3800, 0x38ff) AM_READ(trs80_keyboard_r)
	AM_RANGE(0x3c00, 0x3fff) AM_READWRITE(SMH_RAM, trs80_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x4000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( io_model3, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	/* the ports marked NOP are not emulated yet */
//	AM_RANGE(0xe0, 0xe3) AM_READWRITE(trs80_irq_status_r, trs80_irq_mask_w)	// needs to be fixed because it breaks the keyboard
	AM_RANGE(0xe4, 0xe4) AM_READWRITE(SMH_NOP,trs80_motor_w)
	AM_RANGE(0xe8, 0xe8) AM_NOP
	AM_RANGE(0xe9, 0xe9) AM_WRITENOP
	AM_RANGE(0xea, 0xea) AM_NOP
	AM_RANGE(0xeb, 0xeb) AM_NOP
	AM_RANGE(0xec, 0xec) AM_WRITENOP
	AM_RANGE(0xf0, 0xf0) AM_READWRITE(trs80_wd179x_r, wd17xx_command_w)
	AM_RANGE(0xf1, 0xf1) AM_READWRITE(wd17xx_track_r, wd17xx_track_w)
	AM_RANGE(0xf2, 0xf2) AM_READWRITE(wd17xx_sector_r, wd17xx_sector_w)
	AM_RANGE(0xf3, 0xf3) AM_READWRITE(wd17xx_data_r, wd17xx_data_w)
	AM_RANGE(0xf4, 0xf4) AM_WRITENOP
	AM_RANGE(0xf8, 0xf8) AM_WRITENOP
	AM_RANGE(0xff, 0xff) AM_READWRITE(trs80_port_ff_r, trs80_port_ff_w)
ADDRESS_MAP_END



/**************************************************************************
   w/o SHIFT                             with SHIFT
   +-------------------------------+     +-------------------------------+
   | 0   1   2   3   4   5   6   7 |     | 0   1   2   3   4   5   6   7 |
+--+---+---+---+---+---+---+---+---+  +--+---+---+---+---+---+---+---+---+
|0 | @ | A | B | C | D | E | F | G |  |0 | ` | a | b | c | d | e | f | g |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|1 | H | I | J | K | L | M | N | O |  |1 | h | i | j | k | l | m | n | o |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|2 | P | Q | R | S | T | U | V | W |  |2 | p | q | r | s | t | u | v | w |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|3 | X | Y | Z | [ | \ | ] | ^ | _ |  |3 | x | y | z | { | | | } | ~ |   |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|4 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |  |4 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|5 | 8 | 9 | : | ; | , | - | . | / |  |5 | 8 | 9 | * | + | < | = | > | ? |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|6 |ENT|CLR|BRK|UP |DN |LFT|RGT|SPC|  |6 |ENT|CLR|BRK|UP |DN |LFT|RGT|SPC|
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|7 |SHF|ALT|PUP|PDN|INS|DEL|CTL|END|  |7 |SHF|ALT|PUP|PDN|INS|DEL|CTL|END|
+--+---+---+---+---+---+---+---+---+  +--+---+---+---+---+---+---+---+---+
NB: row 7 contains some originally unused bits
    only the shift bit was there in the TRS80
	
2008-05 FP: 
NB2: 3:3 -> 3:7 have no correspondent keys (see 
	below) in the usual 53-keys keyboard ( + 
	12-keys keypad) for Model I, III and 4. Where 
	are the symbols above coming from?
NB3: the 12-keys keypad present in later models 
	is mapped to the corrispondent keys of the 
	keyboard: '0' -> '9', 'Enter', '.'
NB4: when it was added a 15-key keypad, there were
	three functions key 'F1', 'F2', 'F3'. I found no
	doc about their position in the matrix above, 
	but the schematics of the clone System-80 MkII
	(which had 4 function keys) put these keys in
	3:4 -> 3:7. Right now they're not implemented 
	below.

***************************************************************************/

static INPUT_PORTS_START( trs80 )
	PORT_START("CONFIG") /* IN0 */
	PORT_CONFNAME(	  0x80, 0x00,	"Floppy Disc Drives")
	PORT_CONFSETTING(	0x00, DEF_STR( Off ) )
	PORT_CONFSETTING(	0x80, DEF_STR( On ) )
	PORT_CONFNAME(	  0x40, 0x40,	"Video RAM") PORT_CODE(KEYCODE_F1)
	PORT_CONFSETTING(	0x40, "7 bit" )
	PORT_CONFSETTING(	0x00, "8 bit" )
	PORT_CONFNAME(	  0x20, 0x00,	"Virtual Tape") PORT_CODE(KEYCODE_F2)
	PORT_CONFSETTING(	0x00, DEF_STR( Off ) )
	PORT_CONFSETTING(	0x20, DEF_STR( On ) )
	PORT_BIT(	  0x08, 0x00, IPT_KEYBOARD) PORT_NAME("NMI") PORT_CODE(KEYCODE_F4)
	PORT_BIT(	  0x04, 0x00, IPT_KEYBOARD) PORT_NAME("Tape start") PORT_CODE(KEYCODE_F5)
	PORT_BIT(	  0x02, 0x00, IPT_KEYBOARD) PORT_NAME("Tape stop") PORT_CODE(KEYCODE_F6)
	PORT_BIT(	  0x01, 0x00, IPT_KEYBOARD) PORT_NAME("Tape rewind") PORT_CODE(KEYCODE_F7)

	PORT_START("LINE0") /* KEY ROW 0 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE)		PORT_CHAR('@')
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_A)				PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) 			PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) 			PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) 			PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) 			PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) 			PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) 			PORT_CHAR('g') PORT_CHAR('G')

	PORT_START("LINE1") /* KEY ROW 1 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_H) 			PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) 			PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) 			PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) 			PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) 			PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_M) 			PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_N) 			PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) 			PORT_CHAR('o') PORT_CHAR('O')

	PORT_START("LINE2") /* KEY ROW 2 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_P) 			PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) 			PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) 			PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) 			PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) 			PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_U) 			PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) 			PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) 			PORT_CHAR('w') PORT_CHAR('W')

	PORT_START("LINE3") /* KEY ROW 3 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) 			PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) 			PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z) 			PORT_CHAR('z') PORT_CHAR('Z')
	/* on Model I and Model III keyboards, there are only 53 keys (+ 12 keypad keys) and these are not connected:
	on Model I, they produce arrows and '_', on Model III either produce garbage or overlap with other keys;
	on Model 4 (which has a 15-key with 3 function keys) here are mapped 'F1', 'F2', 'F3'    */
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("(n/c)")
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("(n/c)")
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("(n/c)")
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("(n/c)")
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("(n/c)")

	PORT_START("LINE4") /* KEY ROW 4 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_0)				PORT_CHAR('0') PORT_CHAR(UCHAR_MAMEKEY(0_PAD))
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) 			PORT_CHAR('1') PORT_CHAR('!') PORT_CHAR(UCHAR_MAMEKEY(1_PAD))
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) 			PORT_CHAR('2') PORT_CHAR('"') PORT_CHAR(UCHAR_MAMEKEY(2_PAD))
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) 			PORT_CHAR('3') PORT_CHAR('#') PORT_CHAR(UCHAR_MAMEKEY(3_PAD))
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) 			PORT_CHAR('4') PORT_CHAR('$') PORT_CHAR(UCHAR_MAMEKEY(4_PAD))
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_5) 			PORT_CHAR('5') PORT_CHAR('%') PORT_CHAR(UCHAR_MAMEKEY(5_PAD))
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) 			PORT_CHAR('6') PORT_CHAR('&') PORT_CHAR(UCHAR_MAMEKEY(6_PAD))
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) 			PORT_CHAR('7') PORT_CHAR('\'') PORT_CHAR(UCHAR_MAMEKEY(7_PAD))

	PORT_START("LINE5") /* KEY ROW 5 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_8) 			PORT_CHAR('8') PORT_CHAR('(') PORT_CHAR(UCHAR_MAMEKEY(8_PAD))
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_9) 			PORT_CHAR('9') PORT_CHAR(')') PORT_CHAR(UCHAR_MAMEKEY(9_PAD))
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS) 		PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON) 		PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA) 		PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS) 		PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP)			PORT_CHAR('.') PORT_CHAR('>') PORT_CHAR(UCHAR_MAMEKEY(DEL_PAD))
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH) 		PORT_CHAR('/') PORT_CHAR('?')

	PORT_START("LINE6") /* KEY ROW 6 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER)							PORT_CHAR(13) PORT_CHAR(UCHAR_MAMEKEY(ENTER_PAD))
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("Clear") PORT_CODE(KEYCODE_HOME)		PORT_CHAR(UCHAR_MAMEKEY(F8)) // 3rd line, 1st key from right
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("Break") PORT_CODE(KEYCODE_END)		PORT_CHAR(UCHAR_MAMEKEY(F9)) // 1st line, 1st key from right
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP)	PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_DOWN)							PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	/* backspace do the same as cursor left */
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_LEFT)							PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_RIGHT)							PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE)							PORT_CHAR(' ')

	PORT_START("LINE7") /* KEY ROW 7 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_CODE(KEYCODE_LSHIFT)						PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x02, 0x00, IPT_UNUSED)
	PORT_BIT(0x04, 0x00, IPT_UNUSED)
	PORT_BIT(0x08, 0x00, IPT_UNUSED)
	PORT_BIT(0x10, 0x00, IPT_UNUSED)
	PORT_BIT(0x20, 0x00, IPT_UNUSED)
	PORT_BIT(0x40, 0x00, IPT_UNUSED)
	PORT_BIT(0x80, 0x00, IPT_UNUSED)
#if 0
	/* 2008-05 FP: in the pictures I found of Model I, III, 4 and 4P keyboards, there are only 53 keys 
	(+ 12 keypad keys) and these are not connected. They also have no effect on any TRS-80 model or clone.
	I have no idea where these originally came from */
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("7.1: (ALT)") PORT_CODE(KEYCODE_LALT)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("7.2: (PGUP)") PORT_CODE(KEYCODE_PGUP)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("7.3: (PGDN)") PORT_CODE(KEYCODE_PGDN)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("7.4: (INS)") PORT_CODE(KEYCODE_INSERT)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("7.5: (DEL)") PORT_CODE(KEYCODE_DEL)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("7.6: (CTRL)") PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("7.7: (ALTGR)") PORT_CODE(KEYCODE_RALT)
#endif
INPUT_PORTS_END


static const gfx_layout trs80_charlayout =
{
	FW,FH,			/* 6 x 12 characters */
	256,			/* 256 characters */
	1,				/* 1 bits per pixel */
	{ 0 },			/* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5 },
	/* y offsets */
	{  0*8, 1*8, 2*8, 3*8, 4*8, 5*8,
	   6*8, 7*8, 8*8, 9*8,10*8,11*8 },
	8*FH		   /* every char takes FH bytes */
};

static GFXDECODE_START( trs80 )
	GFXDECODE_ENTRY( "gfx1", 0, trs80_charlayout, 0, 1 )
GFXDECODE_END

static const gfx_layout ht1080z_charlayout =
{
	6,12,			/* 6 x 12 characters */
	128,			/* 128 characters */
	1,				/* 1 bits per pixel */
	{ 0 },			/* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5 },
	/* y offsets */
	{  0*8, 1*8, 2*8, 3*8, 4*8, 5*8,
	   6*8, 7*8, 8*8, 9*8,10*8,11*8 },
	8*16		   /* every char takes FH bytes */
};

static GFXDECODE_START( ht1080z )
	GFXDECODE_ENTRY( "gfx1", 0, ht1080z_charlayout, 0, 1 )
GFXDECODE_END

static const INT16 speaker_levels[3] = {0.0*32767,0.46*32767,0.85*32767};

static const speaker_interface trs80_speaker_interface =
{
	3,				/* optional: number of different levels */
	speaker_levels	/* optional: level lookup table */
};

static const cassette_config trs80l2_cassette_config =
{
	trs80l2_cassette_formats,
	NULL,
	CASSETTE_PLAY
};

static MACHINE_DRIVER_START( level1 )
	/* basic machine hardware */
	MDRV_CPU_ADD("main", Z80, 1796000)        /* 1.796 MHz */
	MDRV_CPU_PROGRAM_MAP(mem_level1, 0)
	MDRV_CPU_IO_MAP(io_level1, 0)
	MDRV_CPU_VBLANK_INT("main", trs80_frame_interrupt)
	MDRV_CPU_PERIODIC_INT(trs80_timer_interrupt, 40)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_START( trs80 )
	MDRV_MACHINE_RESET( trs80 )

    /* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(64*FW, 16*FH)
	MDRV_SCREEN_VISIBLE_AREA(0*FW,64*FW-1,0*FH,16*FH-1)
	MDRV_GFXDECODE( trs80 )
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_UPDATE( trs80 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_CONFIG(trs80_speaker_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* devices */
	MDRV_QUICKLOAD_ADD(trs80_cmd, "cmd", 0.5)

	MDRV_CASSETTE_ADD( "cassette", default_cassette_config )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( model1 )
	MDRV_IMPORT_FROM( level1 )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP( mem_model1, 0 )
	MDRV_CPU_IO_MAP( io_model1, 0 )

	MDRV_CASSETTE_MODIFY( "cassette", trs80l2_cassette_config )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( model3 )
	MDRV_IMPORT_FROM( level1 )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP( mem_model3, 0 )
	MDRV_CPU_IO_MAP( io_model3, 0 )
	MDRV_CPU_VBLANK_INT_HACK(trs80_frame_interrupt, 2)

	MDRV_CASSETTE_MODIFY( "cassette", trs80l2_cassette_config )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ht1080z )
	MDRV_IMPORT_FROM( model1 )
	MDRV_GFXDECODE( ht1080z )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ht108064 )
	MDRV_IMPORT_FROM( model3 )
	MDRV_GFXDECODE( ht1080z )
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(trs80)
	ROM_REGION(0x10000, "main",0)
	ROM_LOAD("level1.rom",  0x0000, 0x1000, CRC(70d06dff) SHA1(20d75478fbf42214381e05b14f57072f3970f765))

	ROM_REGION(0x00c00, "gfx1",0)
	ROM_LOAD("trs80m1.chr", 0x0800, 0x0400, CRC(0033f2b9) SHA1(0d2cd4197d54e2e872b515bbfdaa98efe502eda7))
ROM_END

ROM_START(trs80l2)
	ROM_REGION(0x10000, "main",0)
	ROM_SYSTEM_BIOS(0, "level2", "Radio Shack Level II Basic")
	ROMX_LOAD("trs80.z33",   0x0000, 0x1000, CRC(37c59db2) SHA1(e8f8f6a4460a6f6755873580be6ff70cebe14969), ROM_BIOS(1))
	ROMX_LOAD("trs80.z34",   0x1000, 0x1000, CRC(05818718) SHA1(43c538ca77623af6417474ca5b95fb94205500c1), ROM_BIOS(1))
	ROMX_LOAD("trs80.zl2",   0x2000, 0x1000, CRC(306e5d66) SHA1(1e1abcfb5b02d4567cf6a81ffc35318723442369), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "rsl2", "R/S L2 Basic")
	ROMX_LOAD("trs80alt.z33",0x0000, 0x1000, CRC(be46faf5) SHA1(0e63fc11e207bfd5288118be5d263e7428cc128b), ROM_BIOS(2))
	ROMX_LOAD("trs80alt.z34",0x1000, 0x1000, CRC(6c791c2d) SHA1(2a38e0a248f6619d38f1a108eea7b95761cf2aee), ROM_BIOS(2))
	ROMX_LOAD("trs80alt.zl2",0x2000, 0x1000, CRC(55b3ad13) SHA1(6279f6a68f927ea8628458b278616736f0b3c339), ROM_BIOS(2))

	ROM_REGION(0x00c00, "gfx1",0)
	ROM_LOAD("trs80m1.chr", 0x0800, 0x0400, CRC(0033f2b9) SHA1(0d2cd4197d54e2e872b515bbfdaa98efe502eda7))
ROM_END

ROM_START(sys80)
	ROM_REGION(0x10000, "main",0)
	ROM_LOAD("sys80rom.1",  0x0000, 0x1000, CRC(8f5214de) SHA1(d8c052be5a2d0ec74433043684791d0554bf203b))
	ROM_LOAD("sys80rom.2",  0x1000, 0x1000, CRC(46e88fbf) SHA1(a3ca32757f269e09316e1e91ba1502774e2f5155))
	ROM_LOAD("trs80.zl2",  0x2000, 0x1000, CRC(306e5d66) SHA1(1e1abcfb5b02d4567cf6a81ffc35318723442369))

	ROM_REGION(0x00c00, "gfx1",0)
	ROM_LOAD("trs80m1.chr", 0x0800, 0x0400, CRC(0033f2b9) SHA1(0d2cd4197d54e2e872b515bbfdaa98efe502eda7))
ROM_END

ROM_START(lnw80)
	ROM_REGION(0x10000, "main",0)
	ROM_LOAD("lnw_a.bin",  0x0000, 0x0800, CRC(e09f7e91) SHA1(cd28e72efcfebde6cf1c7dbec4a4880a69e683da))
	ROM_LOAD("lnw_a1.bin", 0x0800, 0x0800, CRC(ac297d99) SHA1(ccf31d3f9d02c3b68a0ee3be4984424df0e83ab0))
	ROM_LOAD("lnw_b.bin",  0x1000, 0x0800, CRC(c4303568) SHA1(13e3d81c6f0de0e93956fa58c465b5368ea51682))
	ROM_LOAD("lnw_b1.bin", 0x1800, 0x0800, CRC(3a5ea239) SHA1(8c489670977892d7f2bfb098f5df0b4dfa8fbba6))
	ROM_LOAD("lnw_c.bin",  0x2000, 0x0800, CRC(2ba025d7) SHA1(232efbe23c3f5c2c6655466ebc0a51cf3697be9b))
	ROM_LOAD("lnw_c1.bin", 0x2800, 0x0800, CRC(ed547445) SHA1(20102de89a3ee4a65366bc2d62be94da984a156b))

	ROM_REGION(0x01000, "gfx1",0)
	ROM_LOAD("lnw_chr.bin",0x0800, 0x0400, CRC(c89b27df) SHA1(be2a009a07e4378d070002a558705e9a0de59389))
	ROM_IGNORE( 0x400 )		/* leave out unused ff's */
ROM_END

ROM_START(trs80m3)
	ROM_REGION(0x10000, "main",0)
	ROM_LOAD("trs80m3.rom", 0x0000, 0x3800, CRC(bddbf843) SHA1(04a1f062cf73c3931c038434e3f299482b6bf613))

	ROM_REGION(0x00c00, "gfx1",0)
	ROM_LOAD("trs80m1.chr", 0x0800, 0x0400, CRC(0033f2b9) SHA1(0d2cd4197d54e2e872b515bbfdaa98efe502eda7))
ROM_END

ROM_START(trs80m4)
	ROM_REGION(0x10000, "main",0)
	ROM_LOAD("trs80m4.rom", 0x0000, 0x3800, CRC(1a92d54d) SHA1(752555fdd0ff23abc9f35c6e03d9d9b4c0e9677b))

	ROM_REGION(0x00c00, "gfx1",0)
	/* this rom unlikely to be the correct one, but it will do for now */
	ROM_LOAD("trs80m1.chr", 0x0800, 0x0400, CRC(0033f2b9) SHA1(0d2cd4197d54e2e872b515bbfdaa98efe502eda7))
ROM_END

ROM_START(ht1080z)
	ROM_REGION(0x10000, "main",0)
	ROM_LOAD("ht1080z.rom", 0x0000, 0x3000, CRC(2bfef8f7) SHA1(7a350925fd05c20a3c95118c1ae56040c621be8f))
	ROM_REGION(0x00800, "gfx1",0)
	ROM_LOAD("ht1080-1.chr", 0x0000, 0x0800, CRC(e8c59d4f) SHA1(a15f30a543e53d3e30927a2e5b766fcf80f0ae31))
ROM_END

ROM_START(ht1080z2)
	ROM_REGION(0x10000, "main",0)
	ROM_LOAD("ht1080z.rom", 0x0000, 0x3000, CRC(2bfef8f7) SHA1(7a350925fd05c20a3c95118c1ae56040c621be8f))
	ROM_REGION(0x00800, "gfx1",0)
	ROM_LOAD("ht1080-2.chr", 0x0000, 0x0800, CRC(6728f0ab) SHA1(1ba949f8596f1976546f99a3fdcd3beb7aded2c5))
ROM_END

ROM_START(ht108064)
	ROM_REGION(0x10000, "main",0)
	ROM_LOAD("ht1080z.64", 0x0000, 0x3000, CRC(48985a30) SHA1(e84cf3121f9e0bb9e1b01b095f7a9581dcfaaae4))
	ROM_LOAD("ht1080z.ext", 0x3000, 0x0800, CRC(fc12bd28) SHA1(0da93a311f99ec7a1e77486afe800a937778e73b))
	ROM_REGION(0x00800, "gfx1",0)
	ROM_LOAD("ht1080-3.chr", 0x0000, 0x0800, CRC(e76b73a4) SHA1(6361ee9667bf59d50059d09b0baf8672fdb2e8af))
ROM_END


static void trs8012_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(trs80_floppy); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "dsk"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

static SYSTEM_CONFIG_START(trs8012)
	CONFIG_DEVICE(trs8012_floppy_getinfo)
SYSTEM_CONFIG_END


/*    YEAR  NAME      PARENT  COMPAT  MACHINE   INPUT  INIT  CONFIG   COMPANY  FULLNAME */
COMP( 1977, trs80,    0,	     0,		level1,   trs80, trs80,    0,		"Tandy Radio Shack",  "TRS-80 Model I (Level I Basic)" , 0)
COMP( 1978, trs80l2,  trs80,	 0,		model1,   trs80, trs80,    trs8012,	"Tandy Radio Shack",  "TRS-80 Model I (Level II Basic)" , 0)
COMP( 1980, sys80,    trs80,	 0,		model1,   trs80, trs80,    trs8012,	"EACA Computers Ltd.","System-80" , 0)
COMP( 1981, lnw80,    trs80,	 0,		model1,   trs80, lnw80,    trs8012,	"LNW Research","LNW-80", 0 )
COMP( 1980, trs80m3,  trs80,	 0,		model3,   trs80, trs80,    trs8012,	"Tandy Radio Shack",  "TRS-80 Model III", 0 )
COMP( 1980, trs80m4,  trs80,	 0,		model3,   trs80, trs80,    trs8012,	"Tandy Radio Shack",  "TRS-80 Model 4", 0 )
COMP( 1983, ht1080z,  trs80,	 0,		ht1080z,  trs80, ht1080z,  trs8012,	"Hiradastechnika Szovetkezet",  "HT-1080Z Series I" , 0)
COMP( 1984, ht1080z2, trs80,	 0,		ht1080z,  trs80, ht1080z,  trs8012,	"Hiradastechnika Szovetkezet",  "HT-1080Z Series II" , 0)
COMP( 1985, ht108064, trs80,	 0,		ht108064, trs80, ht108064, trs8012,	"Hiradastechnika Szovetkezet",  "HT-1080Z/64" , 0)
