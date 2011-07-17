/***************************************************************************

2011-07-16 SLC1 skeleton driver [Robbbert]

http://www.jens-mueller.org/jkcemu/slc1.html

This computer is both a Z80 trainer, and a chess computer.

Hardware
    4 Kbytes ROM in the address range 0000-0FFF
    1 Kbyte RAM in the address range 5000-53ff
    6-digit 7-segment display
    Busy LED
    Keyboard with 12 keys 

ToDo:
- Everything

This source was copied from tec1.c, so most of the code is wrong.

***************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "sound/speaker.h"
#include "slc1.lh"

#define MACHINE_RESET_MEMBER(name) void name::machine_reset()
#define MACHINE_START_MEMBER(name) void name::machine_start()

class slc1_state : public driver_device
{
public:
	slc1_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_maincpu(*this, "maincpu"),
		  m_speaker(*this, SPEAKER_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_speaker;
	emu_timer *m_kbd_timer;
	DECLARE_READ8_MEMBER( slc1_kbd_r );
	DECLARE_WRITE8_MEMBER( slc1_digit_w );
	DECLARE_WRITE8_MEMBER( digit_w );
	DECLARE_WRITE8_MEMBER( slc1_segment_w );
	UINT8 m_kbd;
	UINT8 m_segment;
	UINT8 m_digit;
	UINT8 m_kbd_row;
	UINT8 m_refresh[6];
	UINT8 slc1_convert_col_to_bin( UINT8 col, UINT8 row );
	virtual void machine_reset();
	virtual void machine_start();
};




/***************************************************************************

    Display

***************************************************************************/

WRITE8_MEMBER( slc1_state::slc1_segment_w )
{
	m_segment = BITSWAP8(data, 4, 2, 1, 6, 7, 5, 3, 0);
}

WRITE8_MEMBER( slc1_state::digit_w )
{
	output_set_digit_value(offset, data);
}	

WRITE8_MEMBER( slc1_state::slc1_digit_w )
{
	if ((data & 0xfd) == 0x29)
		speaker_level_w(m_speaker, BIT(data, 1));

	m_digit = data & 0x3f;
}


/***************************************************************************

    Keyboard

***************************************************************************/

READ8_MEMBER( slc1_state::slc1_kbd_r )
{
	return m_kbd | input_port_read(machine(), "SHIFT");
}



/***************************************************************************

    Machine

***************************************************************************/

MACHINE_START_MEMBER( slc1_state )
{
}

MACHINE_RESET_MEMBER( slc1_state )
{
	m_kbd = 0;
}



/***************************************************************************

    Address Map

***************************************************************************/

static ADDRESS_MAP_START( slc1_map, AS_PROGRAM, 8, slc1_state )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xffff)
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x5000, 0x53ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( slc1_io, AS_IO, 8, slc1_state )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x05) AM_WRITE(digit_w)
	AM_RANGE(0x07, 0x07) AM_WRITE(slc1_digit_w)
	AM_RANGE(0xf6, 0xf6) AM_READ(slc1_kbd_r)
	AM_RANGE(0xf5, 0xf5) AM_WRITE(slc1_digit_w)
	AM_RANGE(0xf4, 0xf4) AM_WRITE(slc1_segment_w)
ADDRESS_MAP_END


/**************************************************************************

    Keyboard Layout

***************************************************************************/

static INPUT_PORTS_START( slc1 )
	PORT_START("LINE0") /* KEY ROW 0 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)	PORT_CHAR('0')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)	PORT_CHAR('4')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)	PORT_CHAR('8')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)	PORT_CHAR('C')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("+") PORT_CODE(KEYCODE_EQUALS)	PORT_CHAR('=')

	PORT_START("LINE1") /* KEY ROW 1 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)	PORT_CHAR('1')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)	PORT_CHAR('5')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)	PORT_CHAR('9')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)	PORT_CHAR('D')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS)	PORT_CHAR('-')

	PORT_START("LINE2") /* KEY ROW 2 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)	PORT_CHAR('2')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)	PORT_CHAR('6')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)	PORT_CHAR('A')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)	PORT_CHAR('E')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("GO") PORT_CODE(KEYCODE_ENTER)	PORT_CHAR(13)

	PORT_START("LINE3") /* KEY ROW 3 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)	PORT_CHAR('3')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)	PORT_CHAR('7')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)	PORT_CHAR('B')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)	PORT_CHAR('F')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("AD") PORT_CODE(KEYCODE_J)	PORT_CHAR('J')

	PORT_START("SHIFT")
	PORT_BIT(0x1f, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT)
	PORT_BIT(0xc0, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END


/***************************************************************************

    Machine driver

***************************************************************************/

static MACHINE_CONFIG_START( slc1, slc1_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z80, 2500000)
	MCFG_CPU_PROGRAM_MAP(slc1_map)
	MCFG_CPU_IO_MAP(slc1_io)

	/* video hardware */
	MCFG_DEFAULT_LAYOUT(layout_slc1)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(SPEAKER_TAG, SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_CONFIG_END


/***************************************************************************

    Game driver

***************************************************************************/

ROM_START(slc1)
	ROM_REGION(0x10000, "maincpu", ROMREGION_ERASEFF)
	ROM_LOAD("slc1_0000.bin",   0x0000, 0x1000, CRC(06d32967) SHA1(f25eac66a4fca9383964d509c671a7ad2e020e7e) )
ROM_END


/*    YEAR  NAME      PARENT  COMPAT  MACHINE     INPUT    INIT      COMPANY    FULLNAME */
COMP( 1989, slc1,     0,      0,      slc1,       slc1,    0,      "Dr. Dieter Scheuschner",  "SLC-1" , GAME_NOT_WORKING )
