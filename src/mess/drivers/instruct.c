/***************************************************************************

    Signetics Intructor 50

    08/04/2010 Skeleton driver.
    20/05/2012 Connected digits, system boots. [Robbbert]

    The eprom and 128 bytes of ram are in a 2656 chip. There is no
    useful info on this device on the net, particularly where in the
    memory map the ram resides.

    The Instructor has 512 bytes of ram, plus the 128 bytes mentioned
    above. The clock speed is supposedly 875kHz.

    There is a big lack of info on this computer. No schematics, no i/o
    list or memory map. There is a block diagram which imparts little.

    ToDO:
    - Keys are not correct. Matrix to be worked out, and the polarity.
      So far pressing random keys results in 'Error 2' or a freeze.
    - Key names to be found out.
    - Connect slide switches and 8 round leds.
    - See if it had a speaker or a cassette interface.

****************************************************************************/

#include "emu.h"
#include "cpu/s2650/s2650.h"
#include "instruct.lh"

class instruct_state : public driver_device
{
public:
	instruct_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
	m_maincpu(*this, "maincpu"),
	m_p_ram(*this, "p_ram")
	{ }

	required_device<cpu_device> m_maincpu;
	DECLARE_READ8_MEMBER(instruct_fc_r);
	DECLARE_READ8_MEMBER(instruct_fd_r);
	DECLARE_READ8_MEMBER(portfe_r);
	DECLARE_READ8_MEMBER(instruct_sense_r);
	DECLARE_WRITE8_MEMBER(instruct_f8_w);
	DECLARE_WRITE8_MEMBER(portf9_w);
	DECLARE_WRITE8_MEMBER(portfa_w);
	virtual void machine_reset();
	required_shared_ptr<UINT8> m_p_ram;
	UINT8 m_digit;
	bool m_valid_digit;
};

WRITE8_MEMBER( instruct_state::instruct_f8_w )
{
	printf("%X ",data);
}

WRITE8_MEMBER( instruct_state::portf9_w )
{
	if (m_valid_digit)
		output_set_digit_value(m_digit, data);
	m_valid_digit = false;
}

WRITE8_MEMBER( instruct_state::portfa_w )
{
	m_digit = data;
	m_valid_digit = true;
}

READ8_MEMBER( instruct_state::instruct_fc_r )
{
	return 0;
}

READ8_MEMBER( instruct_state::instruct_fd_r )
{
	return 0;
}

READ8_MEMBER( instruct_state::portfe_r )
{
	UINT8 data = 0xff, i;

	for (i = 0; i < 8; i++)
	{
		if (BIT(m_digit, i))
		{
			char kbdrow[6];
			sprintf(kbdrow,"X%X",i);
			data = ioport(kbdrow)->read();
			break;
		}
	}

	return data;
}

READ8_MEMBER( instruct_state::instruct_sense_r )
{
	return 0;
}

static ADDRESS_MAP_START( instruct_mem, AS_PROGRAM, 8, instruct_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x1fff) AM_RAM AM_SHARE("p_ram")
ADDRESS_MAP_END

static ADDRESS_MAP_START( instruct_io, AS_IO, 8, instruct_state )
	ADDRESS_MAP_UNMAP_HIGH
	//AM_RANGE(0xf8, 0xf8) AM_WRITE(instruct_f8_w)
	AM_RANGE(0xf9, 0xf9) AM_WRITE(portf9_w)
	AM_RANGE(0xfa, 0xfa) AM_WRITE(portfa_w)
	//AM_RANGE(0xfc, 0xfc) AM_READ(instruct_fc_r)
	//AM_RANGE(0xfd, 0xfd) AM_READ(instruct_fd_r)
	AM_RANGE(0xfe, 0xfe) AM_READ(portfe_r)
	//AM_RANGE(S2650_SENSE_PORT, S2650_SENSE_PORT) AM_READ(instruct_sense_r)
	AM_RANGE(0x102, 0x103) AM_NOP // stops error log filling up while using debug
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( instruct )
	PORT_START("X0")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U0") PORT_CODE(KEYCODE_G) PORT_CHAR('G')

	PORT_START("X1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U1") PORT_CODE(KEYCODE_H) PORT_CHAR('H')

	PORT_START("X2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U2") PORT_CODE(KEYCODE_I) PORT_CHAR('I')

	PORT_START("X3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CHAR('3')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U3") PORT_CODE(KEYCODE_J) PORT_CHAR('J')

	PORT_START("X4")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U4") PORT_CODE(KEYCODE_K) PORT_CHAR('K')

	PORT_START("X5")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U5") PORT_CODE(KEYCODE_L) PORT_CHAR('L')

	PORT_START("X6")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(UTF8_UP) PORT_CODE(KEYCODE_UP) PORT_CHAR('^')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U6") PORT_CODE(KEYCODE_M) PORT_CHAR('M')

	PORT_START("X7")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(UTF8_DOWN) PORT_CODE(KEYCODE_DOWN) PORT_CHAR('V')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CHAR('7')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U7") PORT_CODE(KEYCODE_N) PORT_CHAR('N')
INPUT_PORTS_END


void instruct_state::machine_reset()
{
	// copy the roms into ram
	UINT8* ROM = memregion("maincpu")->base();
	memcpy(m_p_ram, ROM, 0x0800);
	memcpy(m_p_ram+0x1800, ROM, 0x0800);
}


static MACHINE_CONFIG_START( instruct, instruct_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",S2650, XTAL_1MHz)
	MCFG_CPU_PROGRAM_MAP(instruct_mem)
	MCFG_CPU_IO_MAP(instruct_io)

	/* video hardware */
	MCFG_DEFAULT_LAYOUT(layout_instruct)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( instruct )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "instruct.rom", 0x0000, 0x0800, CRC(131715a6) SHA1(4930b87d09046113ab172ba3fb31f5e455068ec7) )
ROM_END

/* Driver */

/*    YEAR  NAME       PARENT   COMPAT   MACHINE    INPUT     INIT    COMPANY     FULLNAME                    FLAGS */
COMP( 1978, instruct,  0,       0,       instruct,  instruct, 0,    "Signetics", "Signetics Instructor 50", GAME_NOT_WORKING | GAME_NO_SOUND )
