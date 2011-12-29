/***************************************************************************

        Schachcomputer SC1

        12/05/2009 Skeleton driver.

****************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "sound/speaker.h"
#include "sc1.lh"


class sc1_state : public driver_device
{
public:
	sc1_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
	m_maincpu(*this, "maincpu"),
	m_speaker(*this, SPEAKER_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_speaker;
	DECLARE_WRITE8_MEMBER(sc1_fc_w);
	DECLARE_WRITE8_MEMBER(sc1v2_07_w);
	DECLARE_WRITE8_MEMBER(sc1v2_io_w);
	DECLARE_READ8_MEMBER(sc1v2_io_r);
private:
	UINT8 m_digit;
};

/***************************************************************************

    Display

***************************************************************************/

WRITE8_MEMBER( sc1_state::sc1v2_io_w )
{
	bool segonoff = BIT(data, 7);
	//bool busyled = BIT(data, 4);
	data &= 15;

	if (data < 8)
		m_digit = data;
	else
	if (data < 12)
	{
		speaker_level_w(m_speaker, BIT(data, 1) );
		return;
	}
	else
	if (offset == 0x2f07)
		return;

	UINT8 segdata = output_get_digit_value(m_digit);
	UINT8 segnum  = offset & 7;
	UINT8 segmask = 1 << segnum;

	if (segonoff)
		segdata |= segmask;
	else
		segdata &= ~segmask;

	output_set_digit_value(m_digit, segdata);

	//output_set_value("busyled", busyled);
}


/***************************************************************************

    Keyboard

***************************************************************************/

READ8_MEMBER( sc1_state::sc1v2_io_r )
{
	UINT8 data = 0xff, upper = (offset >> 8) & 7;

	if (upper == 3)
		data &= input_port_read(machine(), "X0");
	else
	if (upper == 4)
		data &= input_port_read(machine(), "X1");
	else
	if (upper == 5)
		data &= input_port_read(machine(), "X2");

	return data;
}

WRITE8_MEMBER( sc1_state::sc1_fc_w )
{
//  if (BIT(data, 3))
//      speaker_level_w(m_speaker, BIT(data, 1));
}

WRITE8_MEMBER( sc1_state::sc1v2_07_w )
{
	if (BIT(data, 3))
		speaker_level_w(m_speaker, BIT(data, 1));
}


static ADDRESS_MAP_START(sc1_mem, AS_PROGRAM, 8, sc1_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x0fff ) AM_ROM
	AM_RANGE( 0x4000, 0x5fff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START(sc1_io, AS_IO, 8, sc1_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE( 0xfc, 0xfc ) AM_WRITE(sc1_fc_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( sc1 )
	PORT_START("X0")
	PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D4 T") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C3 L") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B2 S") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A1 B") PORT_CODE(KEYCODE_1)
	//PORT_BIT(0x1, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E5 D") PORT_CODE(KEYCODE_5)
	//PORT_BIT(0x2, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F6 K") PORT_CODE(KEYCODE_6)
	//PORT_BIT(0x4, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G7")   PORT_CODE(KEYCODE_7)
	//PORT_BIT(0x8, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H8")   PORT_CODE(KEYCODE_8)

	PORT_START("X1")
	PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E5 D") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F6 K") PORT_CODE(KEYCODE_6)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G7")   PORT_CODE(KEYCODE_7)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H8")   PORT_CODE(KEYCODE_8)

	PORT_START("X2")
	PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C")    PORT_CODE(KEYCODE_C)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A")    PORT_CODE(KEYCODE_A)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("St")   PORT_CODE(KEYCODE_S)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z")    PORT_CODE(KEYCODE_Z)
INPUT_PORTS_END


static MACHINE_RESET(sc1)
{
}

static MACHINE_CONFIG_START( sc1, sc1_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",Z80, XTAL_4MHz)
	MCFG_CPU_PROGRAM_MAP(sc1_mem)
	MCFG_CPU_IO_MAP(sc1_io)

	MCFG_MACHINE_RESET(sc1)

	/* video hardware */
	MCFG_DEFAULT_LAYOUT(layout_sc1)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(SPEAKER_TAG, SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( sc1 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "sc1.rom", 0x0000, 0x1000, CRC(26965b23) SHA1(01568911446eda9f05ec136df53da147b7c6f2bf))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY                           FULLNAME       FLAGS */
COMP( 19??, sc1,    0,      0,       sc1,       sc1,     0,  "VEB Mikroelektronik Erfurt", "Schachcomputer SC1", GAME_NOT_WORKING )
