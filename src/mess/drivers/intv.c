/************************************************************************
 *  Mattel Intellivision + Keyboard Component Drivers
 *
 *  Frank Palazzolo
 *  Kyle Davis
 *
 *  TBD:
 *          Map game controllers correctly (right controller + 16 way)
 *          Add tape support (intvkbd)
 *          Add runtime tape loading
 *          Fix memory system workaround
 *            (memory handler stuff in CP1610, debugger, and shared mem)
 *          STIC
 *            reenable dirty support
 *          Cleanup
 *            Separate stic & video better, get rid of *2 for kbd comp
 *          Add better runtime cart loading
 *          Switch to tilemap system
 *      Add IntelliVoice Support
 * Note from kevtris about IntelliVoice Hookup:
<kevtris> the intv uses a special chip
<kevtris> called the SPB640
<kevtris> it is really cool and lame at the same time
<kevtris> it holds 64 10 bit words, which it simply serializes and sends to the SP0256
<kevtris> the SP0256-012 just blindly jumps to 02000h or something when you try to play one of the samples
<kevtris> the SPB640 sits at 2000h and when it sees that address come up, it will send out the 640 bits to the '256
<kevtris> this means you cannot use any gotos/gosubs in your speech data, but that is OK
<kevtris> it's just a single serial stream.  the intv simply watches the buffer state and refills it periodically.  there's enough buffer to keep it full for 1 frame
<kevtris> that's about it
<kevtris> the samples are stored in the game ROMs, and are easy to extract
 *
 ************************************************************************/

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "cpu/cp1610/cp1610.h"
#include "includes/intv.h"
#include "imagedev/cartslot.h"
#include "sound/ay8910.h"

#ifndef VERBOSE
#ifdef MAME_DEBUG
#define VERBOSE 1
#else
#define VERBOSE 0
#endif
#endif

static const unsigned char intv_colors[] =
{
	0x00, 0x00, 0x00, /* BLACK */
	0x00, 0x2D, 0xFF, /* BLUE */
	0xFF, 0x3D, 0x10, /* RED */
	0xC9, 0xCF, 0xAB, /* TAN */
	0x38, 0x6B, 0x3F, /* DARK GREEN */
	0x00, 0xA7, 0x56, /* GREEN */
	0xFA, 0xEA, 0x50, /* YELLOW */
	0xFF, 0xFC, 0xFF, /* WHITE */
	0xBD, 0xAC, 0xC8, /* GRAY */
	0x24, 0xB8, 0xFF, /* CYAN */
	0xFF, 0xB4, 0x1F, /* ORANGE */
	0x54, 0x6E, 0x00, /* BROWN */
	0xFF, 0x4E, 0x57, /* PINK */
	0xA4, 0x96, 0xFF, /* LIGHT BLUE */
	0x75, 0xCC, 0x80, /* YELLOW GREEN */
	0xB5, 0x1A, 0x58  /* PURPLE */
};

static PALETTE_INIT( intv )
{
	int k = 0;

	UINT8 i, j, r, g, b;
	/* Two copies of everything (why?) */

	machine.colortable = colortable_alloc(machine, 32);

	for ( i = 0; i < 16; i++ )
	{
		r = intv_colors[i*3]; g = intv_colors[i*3+1]; b = intv_colors[i*3+2];
		colortable_palette_set_color(machine.colortable, i, MAKE_RGB(r, g, b));
		colortable_palette_set_color(machine.colortable, i + 16, MAKE_RGB(r, g, b));
	}

	for(i=0;i<16;i++)
	{
		for(j=0;j<16;j++)
		{
		colortable_entry_set_value(machine.colortable, k++, i);
		colortable_entry_set_value(machine.colortable, k++, j);
		}
	}

	for(i=0;i<16;i++)
	{
		for(j=16;j<32;j++)
		{
			colortable_entry_set_value(machine.colortable, k++, i);
			colortable_entry_set_value(machine.colortable, k++, j);
		}
	}
}

static const ay8910_interface intv_ay8910_interface =
{
	AY8910_LEGACY_OUTPUT,
	AY8910_DEFAULT_LOADS,
	DEVCB_MEMORY_HANDLER("maincpu", PROGRAM, intv_right_control_r),
	DEVCB_MEMORY_HANDLER("maincpu", PROGRAM, intv_left_control_r),
	DEVCB_NULL,
	DEVCB_NULL
};

/* graphics output */

static const gfx_layout intv_gromlayout =
{
	16, 16,
	256,
	1,
	{ 0 },
	{ 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7},
	{ 0*16, 0*16, 1*16, 1*16, 2*16, 2*16, 3*16, 3*16,
	  4*16, 4*16, 5*16, 5*16, 6*16, 6*16, 7*16, 7*16 },
	8 * 16
};

static const gfx_layout intvkbd_charlayout =
{
	8, 8,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8 * 8
};

static GFXDECODE_START( intv )
	GFXDECODE_ENTRY( "maincpu", 0x3000<<1, intv_gromlayout, 0, 256 )
GFXDECODE_END

static GFXDECODE_START( intvkbd )
	GFXDECODE_ENTRY( "maincpu", 0x3000<<1, intv_gromlayout, 0, 256 )
	GFXDECODE_ENTRY( "gfx1", 0x0000, intvkbd_charlayout, 0, 256 )
GFXDECODE_END

static INPUT_PORTS_START( intv )
	PORT_START("IN0")	/* Right Player Controller Starts Here */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("1") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("2") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("3") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("4") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("5") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("6") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("7") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("8") PORT_CODE(KEYCODE_8_PAD)

	PORT_START("IN1")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("9") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("DEL") PORT_CODE(KEYCODE_MINUS_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("0") PORT_CODE(KEYCODE_0_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_NAME("ENTER") PORT_CODE(KEYCODE_PLUS_PAD)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("But1")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("But2")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("But3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("IN2")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("UP") PORT_CODE(KEYCODE_UP)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("PGUP") PORT_CODE(KEYCODE_PGUP)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("RIGHT") PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("PGDN") PORT_CODE(KEYCODE_PGDN)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("DOWN") PORT_CODE(KEYCODE_DOWN)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("END") PORT_CODE(KEYCODE_END)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("LEFT") PORT_CODE(KEYCODE_LEFT)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("HOME") PORT_CODE(KEYCODE_HOME)

	PORT_START("IN3") /* Left Player Controller Starts Here */

	PORT_START("IN4")

	PORT_START("IN5")
INPUT_PORTS_END

/*
        Bit 7   Bit 6   Bit 5   Bit 4   Bit 3   Bit 2   Bit 1   Bit 0

 Row 0  NC      NC      NC      NC      NC      NC      CTRL    SHIFT
 Row 1  NC      NC      NC      NC      NC      NC      RPT     LOCK
 Row 2  NC      /       ,       N       V       X       NC      SPC
 Row 3  (right) .       M       B       C       Z       NC      CLS
 Row 4  (down)  ;       K       H       F       S       NC      TAB
 Row 5  ]       P       I       Y       R       W       NC      Q
 Row 6  (up)    -       9       7       5       3       NC      1
 Row 7  =       0       8       6       4       2       NC      [
 Row 8  (return)(left)  O       U       T       E       NC      ESC
 Row 9  DEL     '       L       J       G       D       NC      A

2008-05 FP:
The keyboard layout is quite strange, with '[' and ']' at the two ends of the 1st row,
'Esc' in the 2nd row (between 'Tab' and 'Q'), and with Cursor keys and 'Enter' where
you would expect the braces. Moreover, Shift + Cursor keys produce characters.
The emulated layout moves 'Esc', '[' and ']' to their usual position.

Moreover, a small note on natural keyboard support: currently
- "Clear Screen" is mapped to 'Left Alt'
- "Repeat" is mapped to 'F1'
- "Lock" is mapped to 'F2'
*/

static INPUT_PORTS_START( intvkbd )
	PORT_START("ROW0")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)

	PORT_START("ROW1")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("REPEAT") PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LOCK") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_MAMEKEY(F2))

	PORT_START("ROW2")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH)	PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA)	PORT_CHAR(',')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N)		PORT_CHAR('N')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V)		PORT_CHAR('V')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X)		PORT_CHAR('X')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE)	PORT_CHAR(' ')

	PORT_START("ROW3")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_RIGHT)	PORT_CHAR(UCHAR_MAMEKEY(RIGHT)) PORT_CHAR('>')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP)	PORT_CHAR('.')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M)		PORT_CHAR('M')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B)		PORT_CHAR('B')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C)		PORT_CHAR('C')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z)		PORT_CHAR('Z')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Clear Screen") PORT_CODE(KEYCODE_LALT) PORT_CHAR(UCHAR_MAMEKEY(LALT))

	PORT_START("ROW4")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_DOWN)	PORT_CHAR(UCHAR_MAMEKEY(DOWN)) PORT_CHAR('\\')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON)	PORT_CHAR(';') PORT_CHAR(':')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K)		PORT_CHAR('K')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H)		PORT_CHAR('H')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F)		PORT_CHAR('F')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S)		PORT_CHAR('S')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TAB) 	PORT_CHAR('\t')

	PORT_START("ROW5")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P)		PORT_CHAR('P')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I)		PORT_CHAR('I')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y)		PORT_CHAR('Y')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R)		PORT_CHAR('R')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W)		PORT_CHAR('W')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q)		PORT_CHAR('Q')

	PORT_START("ROW6")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_UP)	PORT_CHAR(UCHAR_MAMEKEY(UP)) PORT_CHAR('|')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS)	PORT_CHAR('_') PORT_CHAR('-')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9)		PORT_CHAR('9') PORT_CHAR('(')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7)		PORT_CHAR('7') PORT_CHAR('&')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5)		PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3)		PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1)		PORT_CHAR('1') PORT_CHAR('!')

	PORT_START("ROW7")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS)	PORT_CHAR('=') PORT_CHAR('+')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0)		PORT_CHAR('O') PORT_CHAR(')')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8)		PORT_CHAR('8') PORT_CHAR('*')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6)		PORT_CHAR('6') PORT_CHAR('\xA2')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4)		PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2)		PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR('{') // this one would be 1st row, 1st key (at 'Esc' position)

	PORT_START("ROW8")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LEFT)	PORT_CHAR(UCHAR_MAMEKEY(LEFT)) PORT_CHAR('<')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O)		PORT_CHAR('O')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U)		PORT_CHAR('U')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T)		PORT_CHAR('T')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E)		PORT_CHAR('E')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ESC) 	PORT_CHAR(UCHAR_MAMEKEY(ESC)) // this one would be 2nd row, 2nd key (between 'Tab' and 'Q')

	PORT_START("ROW9")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Del") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE)	PORT_CHAR('\'') PORT_CHAR('"')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L)		PORT_CHAR('L')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J)		PORT_CHAR('J')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G)		PORT_CHAR('G')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D)		PORT_CHAR('D')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NC")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A)		PORT_CHAR('A')

	PORT_START("TEST")	/* For tape drive testing... */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_0_PAD)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_7_PAD)

	/* 2008-05 FP: I include here the controller inputs to make happy the read_handler.
    Please remove this (and re-tag accordingly the inputs above) if intv_right_control_r
    is supposed to scan the keyboard inputs when the Keyboard Component is connected */
	PORT_INCLUDE( intv )
INPUT_PORTS_END

static ADDRESS_MAP_START( intv_mem , AS_PROGRAM, 16)
	AM_RANGE(0x0000, 0x003f) AM_READWRITE( intv_stic_r, intv_stic_w )
	AM_RANGE(0x0100, 0x01ef) AM_READWRITE( intv_ram8_r, intv_ram8_w )
	AM_RANGE(0x01f0, 0x01ff) AM_DEVREADWRITE("ay8910", AY8914_directread_port_0_lsb_r, AY8914_directwrite_port_0_lsb_w )
	AM_RANGE(0x0200, 0x035f) AM_READWRITE( intv_ram16_r, intv_ram16_w )
	AM_RANGE(0x1000, 0x1fff) AM_ROM	AM_REGION("maincpu", 0x1000<<1)	/* Exec ROM, 10-bits wide */
	AM_RANGE(0x3000, 0x37ff) AM_ROM	AM_REGION("maincpu", 0x3000<<1)	/* GROM,     8-bits wide */
	AM_RANGE(0x3800, 0x39ff) AM_READWRITE( intv_gram_r, intv_gram_w )		/* GRAM,     8-bits wide */
	AM_RANGE(0x4800, 0x7fff) AM_ROM		/* Cartridges? */
ADDRESS_MAP_END

static ADDRESS_MAP_START( intvkbd_mem , AS_PROGRAM, 16)
	AM_RANGE(0x0000, 0x003f) AM_READWRITE( intv_stic_r, intv_stic_w )
	AM_RANGE(0x0100, 0x01ef) AM_READWRITE( intv_ram8_r, intv_ram8_w )
	AM_RANGE(0x01f0, 0x01ff) AM_DEVREADWRITE("ay8910", AY8914_directread_port_0_lsb_r, AY8914_directwrite_port_0_lsb_w )
	AM_RANGE(0x0200, 0x035f) AM_READWRITE( intv_ram16_r, intv_ram16_w )
	AM_RANGE(0x1000, 0x1fff) AM_ROM	AM_REGION("maincpu", 0x1000<<1)	/* Exec ROM, 10-bits wide */
	AM_RANGE(0x3000, 0x37ff) AM_ROM	AM_REGION("maincpu", 0x3000<<1)	/* GROM,     8-bits wide */
	AM_RANGE(0x3800, 0x39ff) AM_READWRITE( intv_gram_r, intv_gram_w )	/* GRAM,     8-bits wide */
	AM_RANGE(0x4800, 0x6fff) AM_ROM		/* Cartridges? */
	AM_RANGE(0x7000, 0x7fff) AM_ROM	AM_REGION("maincpu", 0x7000<<1)	/* Keyboard ROM */
	AM_RANGE(0x8000, 0xbfff) AM_RAM_WRITE( intvkbd_dualport16_w ) AM_BASE_MEMBER(intv_state, m_intvkbd_dualport_ram)	/* Dual-port RAM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( intv2_mem , AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH  /* Required because of probing */
	AM_RANGE( 0x0000, 0x3fff) AM_READWRITE( intvkbd_dualport8_lsb_r, intvkbd_dualport8_lsb_w )	/* Dual-port RAM */
	AM_RANGE( 0x4000, 0x7fff) AM_READWRITE( intvkbd_dualport8_msb_r, intvkbd_dualport8_msb_w )	/* Dual-port RAM */
	AM_RANGE( 0xb7f8, 0xb7ff) AM_RAM	/* ??? */
	AM_RANGE( 0xb800, 0xbfff) AM_RAM AM_BASE_MEMBER(intv_state, m_videoram) /* Text Display */
	AM_RANGE( 0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END

/* This is needed because MAME core does not allow PULSE_LINE.
    The time interval is not critical, although it should be below 1000. */

static TIMER_CALLBACK(intv_interrupt2_complete)
{
	cputag_set_input_line(machine, "keyboard", 0, CLEAR_LINE);
}

static INTERRUPT_GEN( intv_interrupt2 )
{
	cputag_set_input_line(device->machine(), "keyboard", 0, ASSERT_LINE);
	device->machine().scheduler().timer_set(device->machine().device<cpu_device>("keyboard")->cycles_to_attotime(100), FUNC(intv_interrupt2_complete));
}

static MACHINE_CONFIG_START( intv, intv_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", CP1610, XTAL_3_579545MHz/4)        /* Colorburst/4 */
	MCFG_CPU_PROGRAM_MAP(intv_mem)
	MCFG_CPU_VBLANK_INT("screen", intv_interrupt)
	MCFG_QUANTUM_TIME(attotime::from_hz(60))

	MCFG_MACHINE_RESET( intv )

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(59.92)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(40*8, 24*8)
	MCFG_SCREEN_VISIBLE_AREA(0, 40*8-1, 0, 24*8-1)
	MCFG_SCREEN_UPDATE( generic_bitmapped )

	MCFG_GFXDECODE( intv )
	MCFG_PALETTE_LENGTH(0x400)
	MCFG_PALETTE_INIT( intv )

	MCFG_VIDEO_START( intv )

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("ay8910", AY8910, XTAL_3_579545MHz/2)
	MCFG_SOUND_CONFIG(intv_ay8910_interface)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* cartridge */
	MCFG_CARTSLOT_ADD("cart")
	MCFG_CARTSLOT_EXTENSION_LIST("int,rom,bin,itv")
	MCFG_CARTSLOT_LOAD(intv_cart)
MACHINE_CONFIG_END


static MACHINE_CONFIG_DERIVED( intvkbd, intv )
	MCFG_CPU_MODIFY( "maincpu" )
	MCFG_CPU_PROGRAM_MAP(intvkbd_mem)

	MCFG_CPU_ADD("keyboard", M6502, XTAL_3_579545MHz/2)	/* Colorburst/2 */
	MCFG_CPU_PROGRAM_MAP(intv2_mem)
	MCFG_CPU_VBLANK_INT("screen", intv_interrupt2)

	MCFG_QUANTUM_TIME(attotime::from_hz(6000))

	/* video hardware */
	MCFG_GFXDECODE(intvkbd)
	MCFG_SCREEN_MODIFY("screen")
	MCFG_SCREEN_UPDATE(intvkbd)

	/* cartridge */
	MCFG_DEVICE_REMOVE("cart")
	MCFG_CARTSLOT_ADD("cart1")
	MCFG_CARTSLOT_EXTENSION_LIST("int,rom,bin,itv")
	MCFG_CARTSLOT_NOT_MANDATORY
	MCFG_CARTSLOT_LOAD(intvkbd_cart)
	MCFG_CARTSLOT_ADD("cart2")
	MCFG_CARTSLOT_EXTENSION_LIST("int,rom,bin,itv")
	MCFG_CARTSLOT_NOT_MANDATORY
	MCFG_CARTSLOT_LOAD(intvkbd_cart)
MACHINE_CONFIG_END

ROM_START(intv)
	ROM_REGION(0x10000<<1,"maincpu", ROMREGION_ERASEFF)
	ROM_LOAD16_WORD( "exec.bin", (0x1000<<1)+0, 0x2000, CRC(cbce86f7) SHA1(5a65b922b562cb1f57dab51b73151283f0e20c7a))
	ROM_LOAD16_BYTE( "grom.bin", (0x3000<<1)+1, 0x0800, CRC(683a4158) SHA1(f9608bb4ad1cfe3640d02844c7ad8e0bcd974917))
ROM_END

ROM_START(intvsrs)
	ROM_REGION(0x10000<<1,"maincpu", ROMREGION_ERASEFF)
	ROM_LOAD16_WORD( "searsexc.bin", (0x1000<<1)+0, 0x2000, CRC(ea552a22) SHA1(834339de056d42a35571cae7fd5b04d1344001e9))
	ROM_LOAD16_BYTE( "grom.bin", (0x3000<<1)+1, 0x0800, CRC(683a4158) SHA1(f9608bb4ad1cfe3640d02844c7ad8e0bcd974917))
ROM_END

ROM_START(intvkbd)
	ROM_REGION(0x10000<<1,"maincpu", ROMREGION_ERASEFF)
	ROM_LOAD16_WORD( "exec.bin", 0x1000<<1, 0x2000, CRC(cbce86f7) SHA1(5a65b922b562cb1f57dab51b73151283f0e20c7a))
	ROM_LOAD16_BYTE( "grom.bin", (0x3000<<1)+1, 0x0800, CRC(683a4158) SHA1(f9608bb4ad1cfe3640d02844c7ad8e0bcd974917))
	ROM_LOAD16_WORD( "024.u60",  0x7000<<1, 0x1000, CRC(4f7998ec) SHA1(ec006d0ae9002e9d56d83a71f5f2eddd6a456a40))
	ROM_LOAD16_BYTE( "4d72.u62", 0x7800<<1, 0x0800, CRC(aa57c594) SHA1(741860d489d90f5882ca53daa3169b6abacdf130))
	ROM_LOAD16_BYTE( "4d71.u63", (0x7800<<1)+1, 0x0800, CRC(069b2f0b) SHA1(070850bb32f8474107cc52c5183cfaa32d640f9a))

	ROM_REGION(0x10000,"keyboard",0)
	ROM_LOAD( "0104.u20",  0xc000, 0x1000, CRC(5c6f1256) SHA1(271931fb354dfae6a1a5697ee888924a89a15ca8))
	ROM_RELOAD( 0xe000, 0x1000 )
	ROM_LOAD("cpu2d.u21",  0xd000, 0x1000, CRC(2c2dba33) SHA1(0db5d177fec3f8ae89abeef2e6900ad4f3460266))
	ROM_RELOAD( 0xf000, 0x1000 )

	ROM_REGION(0x00800,"gfx1",0)
	ROM_LOAD( "4c52.u34",  0x0000, 0x0800, CRC(cbeb2e96) SHA1(f0e17adcd278fb376c9f90833c7fbbb60193dbe3))
ROM_END


#ifdef UNUSED_FUNCTION
static void intvkbd_cassette_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
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
		case MESS_DEVINFO_INT_RESET_ON_LOAD:					info->i = 1; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "tap"); break;
	}
}
#endif


/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME  PARENT  COMPAT  MACHINE  INPUT   INIT     COMPANY      FULLNAME */
CONS( 1979, intv,	0,	0,	intv,     intv, 	0,			"Mattel",    "Intellivision", 0 )
CONS( 1981, intvsrs,	intv,	0,	intv,     intv, 	0,	"Mattel",    "Intellivision (Sears)", 0 )
COMP( 1981, intvkbd,	intv,	0,	intvkbd,  intvkbd,	0,	"Mattel",    "Intellivision Keyboard Component (Unreleased)", GAME_NOT_WORKING)
