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
	DECLARE_READ8_MEMBER( io_r );
	DECLARE_WRITE8_MEMBER( io_w );
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

WRITE8_MEMBER( slc1_state::io_w )
{
	UINT8 upper = offset >> 8;
//  if (upper == 0x2f) return;


	speaker_level_w(m_speaker, (upper == 0x29));

//  output_set_led_value(0, BIT(data, 4));

	// This computer sets each segment, one at a time. */

	if ((offset & 0xff) < 8)
	{
		static const UINT8 segments[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
		UINT8 digit = data & 7;
		UINT8 segment = segments[offset];
		UINT8 segdata = output_get_digit_value(digit);

		if (BIT(data, 7))
			segdata |= segment;
		else
			segdata &= ~segment;

		output_set_digit_value(digit, segdata);
	}
}



/***************************************************************************

    Keyboard

***************************************************************************/

READ8_MEMBER( slc1_state::io_r )
{
	UINT8 data = 0xff, upper = (offset >> 8) & 7;

	if (upper == 5) data &= input_port_read(machine(), "X0");
	if (upper == 6) data &= input_port_read(machine(), "X1");
	if (upper == 7) data &= input_port_read(machine(), "X2");
	return data;
}



/***************************************************************************

    Machine

***************************************************************************/

MACHINE_START_MEMBER( slc1_state )
{
}

MACHINE_RESET_MEMBER( slc1_state )
{
}



/***************************************************************************

    Address Map

***************************************************************************/

static ADDRESS_MAP_START( slc1_map, AS_PROGRAM, 8, slc1_state )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0x4fff)
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x5000, 0x53ff) AM_RAM AM_MIRROR(0xc00)
ADDRESS_MAP_END

static ADDRESS_MAP_START( slc1_io, AS_IO, 8, slc1_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(io_r,io_w)
ADDRESS_MAP_END


/**************************************************************************

    Keyboard Layout

***************************************************************************/

static INPUT_PORTS_START( slc1 )
	PORT_START("X0")
	PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 B D 4 T") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 A C 3 L") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 9 B 2 S") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0 8 A 1 B") PORT_CODE(KEYCODE_0)

	PORT_START("X1")
	PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 F H 8 GO") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 E G 7 BL") PORT_CODE(KEYCODE_6)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 D F 6 K DEL") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 C E 5 D INS") PORT_CODE(KEYCODE_4)

	PORT_START("X2")
	PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Seq C BG") PORT_CODE(KEYCODE_Q)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("+-1 A SS") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Fu St DP") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ADR Z BP") PORT_CODE(KEYCODE_R)
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
