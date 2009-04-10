/***************************************************************************

***************************************************************************/

/* Core includes */
#include "driver.h"
#include "cpu/z80/z80.h"
#include "sound/speaker.h"
#include "tec1.lh"

static emu_timer *tec1_kbd_timer;
static UINT8 tec1_kbd=0;
static UINT8 tec1_nmi=0;
static UINT8 tec1_segment=0;
static UINT8 tec1_digit=0;
static const device_config *tec1_speaker;


static void tec1_display(void)
{
	UINT8 i;
	for (i = 0; i < 6; i++)
		if (tec1_digit & (1 << i))
			output_set_digit_value(5-i, tec1_segment);
}

static WRITE8_HANDLER( tec1_segment_w )
{
	tec1_segment = BITSWAP8(data, 4, 2, 1, 6, 7, 5, 3, 0);
	tec1_display();
}

static WRITE8_HANDLER( tec1_digit_w )
{
	tec1_digit = data & 0x3f;
	speaker_level_w(tec1_speaker, (data & 0x80) ? 1 : 0);
	tec1_display();
}

static READ8_HANDLER( tec1_kbd_r )
{
	UINT8 data = tec1_kbd;
	cpu_set_input_line(space->machine->cpu[0], INPUT_LINE_NMI, CLEAR_LINE);
	tec1_nmi = 0;
	return data;
}
	
static TIMER_CALLBACK( tec1_kbd_callback )
{
/* 74C923 4 by 5 key encoder. If a key is pressed, scanning stops. */

	static UINT8 row=0;
	if (tec1_nmi) return;
	row++;
	row &= 3;

	/* see if a key pressed */
	if (row == 0)
		tec1_kbd = input_port_read(machine, "LINE0");
	else
	if (row == 1)
		tec1_kbd = input_port_read(machine, "LINE1");
	else
	if (row == 2)
		tec1_kbd = input_port_read(machine, "LINE2");
	else
	if (row == 3)
		tec1_kbd = input_port_read(machine, "LINE3");

	/* convert to the code that the chip uses */
	if (tec1_kbd)
	{
		if (tec1_kbd & 1)
			tec1_kbd = 0;
		else
		if (tec1_kbd & 2)
			tec1_kbd = 4;
		else
		if (tec1_kbd & 4)
			tec1_kbd = 8;
		else
		if (tec1_kbd & 8)
			tec1_kbd = 12;
		else
		if (tec1_kbd & 16)
			tec1_kbd = 16;

		tec1_kbd |= row;
		tec1_nmi = 1;
		cpu_set_input_line(machine->cpu[0], INPUT_LINE_NMI, HOLD_LINE);
	}
}

static MACHINE_RESET( tec1 )
{
	tec1_kbd_timer = timer_alloc(machine,  tec1_kbd_callback, NULL );
	timer_adjust_periodic( tec1_kbd_timer, attotime_zero, 0, ATTOTIME_IN_HZ(30) );
	tec1_speaker = devtag_get_device(machine, "speaker");
}


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

	Keyboard

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


static MACHINE_DRIVER_START( tec1 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, 500000)	/* speed can be varied between 250kHz and 2MHz */
	MDRV_CPU_PROGRAM_MAP(tec1_map, 0)
	MDRV_CPU_IO_MAP(tec1_io, 0)

	MDRV_MACHINE_RESET(tec1)

	/* video hardware */
	MDRV_DEFAULT_LAYOUT(layout_tec1)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(tec1)
	ROM_REGION(0x10000, "maincpu", 0)
	ROM_LOAD("tec1.rom",    0x0000, 0x0800, CRC(b3390c36) SHA1(18aabc68d473206b7fc4e365c6b57a4e218482c3) )
//	ROM_SYSTEM_BIOS(0, "tec1", "TEC-1")
//	ROMX_LOAD("tec1.rom",    0x0000, 0x0800, CRC(b3390c36) SHA1(18aabc68d473206b7fc4e365c6b57a4e218482c3), ROM_BIOS(1))
//	ROM_SYSTEM_BIOS(1, "tec1a", "TEC-1A")
//	ROMX_LOAD("tec1a.rom",   0x0000, 0x0800, CRC(60daea3c) SHA1(383b7e7f02e91fb18c87eb03c5949e31156771d4), ROM_BIOS(2))
ROM_END

/*    YEAR  NAME      PARENT  COMPAT  MACHINE	  INPUT    INIT      CONFIG       COMPANY  FULLNAME */
COMP( 1984, tec1,     0,      0,      tec1,       tec1,    0,        0,		"Talking Electronics Magazine",  "TEC-1" , 0 )
