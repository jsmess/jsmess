/***************************************************************************

    TEC-1 driver, written by Robbbert in April, 2009 for MESS.

The TEC-1 was a single-board "computer" described in Talking Electronics
magazine, issues number 10 and 11. Talking Electronics do not have dates on
their issues, so the date is uncertain, although 1984 seems a reasonable
guess. Talking Electronics operated from Cheltenham, a suburb of Melbourne.

The hardware is quite simple, consisting of a Z80 cpu, 2x 8212 8-bit latch,
74C923 keyboard scanner, 20 push-button keys, 6-digit LED display, a speaker,
a 2k EPROM and sundry parts.

The cpu speed could be adjusted by using a potentiometer, the range being
250 kHz to 2MHz. This is a simple method of adjusting a game's difficulty.

We emulate the original version. Later enhancements included more RAM, speech
synthesis and various attachments, however I have no information on these.

Keys:
0 to 9, A to F are on the key of the same name.
AD (input an address) is the J key.
+ and - (increment / decrement address) are the - and = keys.
GO (execute program at current address) is the Enter key.
Whenever a program listing mentions RESET, do a Soft Reset.

Each key causes a beep to be heard. You may need to press more than once
to get it to register.

Inbuilt games - press the following sequence of keys:
- Welcome: RESET D 1 + 0 2 AD 0 2 7 0 GO GO
- Nim: RESET AD 3 E 0 GO GO
- Invaders: RESET AD 3 2 0 GO GO
- Luna Lander: RESET AD 4 9 0 GO GO

Not emulated / need to be checked:
- The 74C923 code may need to be revisited to improve keyboard response.
- The 10ms debounce is not emulated.
- Artwork needs to be produced.

Thanks to Chris Schwartz who dumped his ROM for me way back in the old days.
It's only taken 25 years to get around to emulating it...

***************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "sound/speaker.h"
#include "tec1.lh"

static emu_timer *tec1_kbd_timer;
static UINT8 tec1_kbd=0;
static UINT8 tec1_segment=0;
static UINT8 tec1_digit=0;
static running_device *tec1_speaker;


/***************************************************************************

    Display

***************************************************************************/

static void tec1_display(void)
{
	UINT8 i;
	for (i = 0; i < 6; i++)
		if (tec1_digit & (1 << i))
			output_set_digit_value(5-i, tec1_segment);
}

static WRITE8_HANDLER( tec1_segment_w )
{
/*  d7 segment d
    d6 segment e
    d5 segment c
    d4 segment dot
    d3 segment b
    d2 segment g
    d1 segment f
    d0 segment a */

	tec1_segment = BITSWAP8(data, 4, 2, 1, 6, 7, 5, 3, 0);
	if (tec1_digit) tec1_display();
}

static WRITE8_HANDLER( tec1_digit_w )
{
/*  d7 speaker
    d6 not used
    d5 data digit 1
    d4 data digit 2
    d3 address digit 1
    d2 address digit 2
    d1 address digit 3
    d0 address digit 4 */

	speaker_level_w(tec1_speaker, (data & 0x80) ? 1 : 0);

	tec1_digit = data & 0x3f;
	if (tec1_digit) tec1_display();
}


/***************************************************************************

    Keyboard

***************************************************************************/

static READ8_HANDLER( tec1_kbd_r )
{
	cputag_set_input_line(space->machine, "maincpu", INPUT_LINE_NMI, CLEAR_LINE);
	return tec1_kbd;
}

static UINT8 tec1_convert_col_to_bin( UINT8 col, UINT8 row )
{
	UINT8 data = row;

	if (col & 2)
		data |= 4;
	else
	if (col & 4)
		data |= 8;
	else
	if (col & 8)
		data |= 12;
	else
	if (col & 16)
		data |= 16;

	return data;
}


static const char *const keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3" };

static TIMER_CALLBACK( tec1_kbd_callback )
{
/* 74C923 4 by 5 key encoder. */


	static UINT8 row=0;

	/* if previous key is still held, bail out */
	if (input_port_read(machine, keynames[row]))
		if (tec1_convert_col_to_bin(input_port_read(machine, keynames[row]), row) == tec1_kbd)
			return;

	row++;
	row &= 3;

	/* see if a key pressed */
	if (input_port_read(machine, keynames[row]))
	{
		tec1_kbd = tec1_convert_col_to_bin(input_port_read(machine, keynames[row]), row);
		cputag_set_input_line(machine, "maincpu", INPUT_LINE_NMI, HOLD_LINE);
	}
}


/***************************************************************************

    Machine

***************************************************************************/

static MACHINE_START( tec1 )
{
	tec1_kbd_timer = timer_alloc(machine,  tec1_kbd_callback, NULL );
	timer_adjust_periodic( tec1_kbd_timer, attotime_zero, 0, ATTOTIME_IN_HZ(500) );
	tec1_speaker = machine->device("speaker");
}


/***************************************************************************

    Address Map

***************************************************************************/

static ADDRESS_MAP_START( tec1_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0x3fff)
	AM_RANGE(0x0000, 0x07ff) AM_ROM
	AM_RANGE(0x0800, 0x17ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( tec1_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0x07)
	AM_RANGE(0x00, 0x00) AM_READ(tec1_kbd_r)
	AM_RANGE(0x01, 0x01) AM_WRITE(tec1_digit_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(tec1_segment_w)
ADDRESS_MAP_END


/**************************************************************************

    Keyboard Layout

***************************************************************************/

static INPUT_PORTS_START( tec1 )
	PORT_START("LINE0") /* KEY ROW 0 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)		PORT_CHAR('0')
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)		PORT_CHAR('4')
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)		PORT_CHAR('8')
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)		PORT_CHAR('C')
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("+") PORT_CODE(KEYCODE_EQUALS)	PORT_CHAR('=')

	PORT_START("LINE1") /* KEY ROW 1 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)		PORT_CHAR('1')
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)		PORT_CHAR('5')
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)		PORT_CHAR('9')
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)		PORT_CHAR('D')
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS)	PORT_CHAR('-')

	PORT_START("LINE2") /* KEY ROW 2 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)		PORT_CHAR('2')
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)		PORT_CHAR('6')
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)		PORT_CHAR('A')
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)		PORT_CHAR('E')
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("GO") PORT_CODE(KEYCODE_ENTER)	PORT_CHAR(13)

	PORT_START("LINE3") /* KEY ROW 3 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)		PORT_CHAR('3')
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)		PORT_CHAR('7')
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)		PORT_CHAR('B')
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)		PORT_CHAR('F')
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("AD") PORT_CODE(KEYCODE_J)		PORT_CHAR('J')
INPUT_PORTS_END


/***************************************************************************

    Machine driver

***************************************************************************/

static MACHINE_CONFIG_START( tec1, driver_device )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, 500000)	/* speed can be varied between 250kHz and 2MHz */
	MDRV_CPU_PROGRAM_MAP(tec1_map)
	MDRV_CPU_IO_MAP(tec1_io)

	MDRV_MACHINE_START(tec1)

	/* video hardware */
	MDRV_DEFAULT_LAYOUT(layout_tec1)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_CONFIG_END


/***************************************************************************

    Game driver

***************************************************************************/

ROM_START(tec1)
	ROM_REGION(0x10000, "maincpu", 0)
	ROM_LOAD("tec1.rom",    0x0000, 0x0800, CRC(b3390c36) SHA1(18aabc68d473206b7fc4e365c6b57a4e218482c3) )
//  ROM_SYSTEM_BIOS(0, "tec1", "TEC-1")
//  ROMX_LOAD("tec1.rom",    0x0000, 0x0800, CRC(b3390c36) SHA1(18aabc68d473206b7fc4e365c6b57a4e218482c3), ROM_BIOS(1))
//  ROM_SYSTEM_BIOS(1, "tec1a", "TEC-1A")
//  ROMX_LOAD("tec1a.rom",   0x0000, 0x0800, CRC(60daea3c) SHA1(383b7e7f02e91fb18c87eb03c5949e31156771d4), ROM_BIOS(2))
ROM_END

/*    YEAR  NAME      PARENT  COMPAT  MACHINE     INPUT    INIT       COMPANY  FULLNAME */
COMP( 1984, tec1,     0,      0,      tec1,       tec1,    0,		"Talking Electronics magazine",  "TEC-1" , 0 )
