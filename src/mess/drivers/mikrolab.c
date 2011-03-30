/***************************************************************************

        Mikrolab KR580IK80

        10/12/2010 Skeleton driver.

****************************************************************************/

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "mikrolab.lh"

class mikrolab_state : public driver_device
{
public:
	mikrolab_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	DECLARE_WRITE8_MEMBER(digit_w);
	DECLARE_WRITE8_MEMBER(kp_matrix_w);
	DECLARE_READ8_MEMBER(keypad_r);

	UINT8 m_kp_matrix;
};

WRITE8_MEMBER(mikrolab_state::digit_w)
{
	output_set_digit_value(offset, data);
}

WRITE8_MEMBER(mikrolab_state::kp_matrix_w)
{
	m_kp_matrix = data;
}

READ8_MEMBER(mikrolab_state::keypad_r)
{
	UINT8 data = 0xff;

	if (BIT(m_kp_matrix, 4))
		data &= input_port_read(m_machine, "LINE1");
	if (BIT(m_kp_matrix, 5))
		data &= input_port_read(m_machine, "LINE2");
	if (BIT(m_kp_matrix, 6))
		data &= input_port_read(m_machine, "LINE3");

	return data;
}

static ADDRESS_MAP_START(mikrolab_mem, AS_PROGRAM, 8, mikrolab_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x05ff ) AM_ROM	// Monitor is up to 0x2ff
	AM_RANGE( 0x8000, 0x83f7 ) AM_RAM
	AM_RANGE( 0x83f8, 0x83ff ) AM_WRITE(digit_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mikrolab_io , AS_IO, 8, mikrolab_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0xf8, 0xf8 ) AM_READ(keypad_r)
	AM_RANGE( 0xfa, 0xfa ) AM_WRITE(kp_matrix_w)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( mikrolab )
	PORT_START("LINE1")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
	PORT_START("LINE2")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
	PORT_START("LINE3")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("RUN") PORT_CODE(KEYCODE_R)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("RESUME") PORT_CODE(KEYCODE_T)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("ADDRESS") PORT_CODE(KEYCODE_Y)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("-") PORT_CODE(KEYCODE_PGDN)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("+") PORT_CODE(KEYCODE_PGUP)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("SAVE") PORT_CODE(KEYCODE_S)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("END") PORT_CODE(KEYCODE_Z)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER)
INPUT_PORTS_END

static MACHINE_CONFIG_START( mikrolab, mikrolab_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",I8080, XTAL_2MHz)
    MCFG_CPU_PROGRAM_MAP(mikrolab_mem)
    MCFG_CPU_IO_MAP(mikrolab_io)

    /* video hardware */
    MCFG_DEFAULT_LAYOUT(layout_mikrolab)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( mikrolab )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	/* these dumps are taken from PDF so need check with real device */
	ROM_LOAD( "rom-1.bin", 0x0000, 0x0200, BAD_DUMP CRC(eed5f23b) SHA1(c82f7a16ce44c4fcbcb333245555feae1fcdf058))
	ROM_LOAD( "rom-2.bin", 0x0200, 0x0200, BAD_DUMP CRC(726a224f) SHA1(7ed8d2c6dd4fb7836475e207e1972e33a6a91d2f))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT   COMPANY   FULLNAME       FLAGS */
COMP( ????, mikrolab,  0,       0,	mikrolab,	mikrolab,	 0,	 "<unknown>",   "Mikrolab KR580IK80",		GAME_NOT_WORKING | GAME_NO_SOUND)

