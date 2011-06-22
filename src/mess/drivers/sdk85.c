/***************************************************************************

        Intel SDK-85

        09/12/2009 Skeleton driver.

        22/06/2011 Working [Robbbert]

This is an evaluation kit for the 8085 cpu.

There is no speaker or storage facility in the standard kit.

Download the User Manual to get the operating procedures.

An example is Press SUBST key, enter an address, press NEXT key, enter data,
then press NEXT to increment the address.

ToDo:
- Artwork
- Proper emulation of the 8279 chip (there's only just enough to get this
  driver to work atm)

****************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "sdk85.lh"

#define MACHINE_RESET_MEMBER(name) void name::machine_reset()

class sdk85_state : public driver_device
{
public:
	sdk85_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

	DECLARE_READ8_MEMBER(keyboard_r);
	DECLARE_READ8_MEMBER(status_r);
	DECLARE_WRITE8_MEMBER(command_w);
	DECLARE_WRITE8_MEMBER(data_w);
	UINT8 m_digit; // next digit to display
	UINT8 m_kbd; // keyscan code being returned
	UINT8 m_keytemp; // to see when key gets released
	bool m_kbd_ready; // get ready to return keyscan code
	virtual void machine_reset();
};

READ8_MEMBER( sdk85_state::keyboard_r )
{
	if (m_kbd_ready)
	{
		UINT8 ret = m_kbd;
		m_kbd_ready = FALSE;
		m_kbd = 0xff;
		return ret;
	}
	else
		return 0;
}

READ8_MEMBER( sdk85_state::status_r )
{
	return 0;
}

WRITE8_MEMBER( sdk85_state::command_w )
{
	if ((data & 0x90)==0x90)
		m_digit = data & 7;
	else
	if (data==0x40)
		m_kbd_ready = TRUE;
}

WRITE8_MEMBER( sdk85_state::data_w )
{
	if (m_digit < 6)
	{
		output_set_digit_value(m_digit, BITSWAP8(data, 3, 2, 1, 0, 7, 6, 5, 4)^0xff);
		m_digit++;
	}
	else
		m_digit = 15;
}


static ADDRESS_MAP_START(sdk85_mem, AS_PROGRAM, 8, sdk85_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x07ff) AM_ROM // Monitor rom (A14)
	AM_RANGE(0x0800, 0x0fff) AM_ROM // Expansion rom (A15)
	AM_RANGE(0x1800, 0x1800) AM_READWRITE(keyboard_r,data_w)
	AM_RANGE(0x1900, 0x1900) AM_READWRITE(status_r,command_w)
	//AM_RANGE(0x1800, 0x1fff) AM_RAM // i8279 (A13)
	AM_RANGE(0x2000, 0x27ff) AM_RAM // i8155 (A16)
	AM_RANGE(0x2800, 0x2fff) AM_RAM // i8155 (A17)
ADDRESS_MAP_END

static ADDRESS_MAP_START(sdk85_io, AS_IO, 8, sdk85_state)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( sdk85 )
	PORT_START("X0")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CHAR('3')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CHAR('7')

	PORT_START("X1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('F')

	PORT_START("X2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("EXEC") PORT_CODE(KEYCODE_Q)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NEXT") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("GO") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SUBST") PORT_CODE(KEYCODE_T)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("EXAM") PORT_CODE(KEYCODE_Y)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SINGLE") PORT_CODE(KEYCODE_U)
	PORT_BIT(0xC0, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END


MACHINE_RESET_MEMBER( sdk85_state )
{
	m_digit = 15;
	m_kbd_ready = FALSE;
	m_kbd = 0xff;
	m_keytemp = 0xfe;
}

// This is an internal function of the 8279 chip
static TIMER_DEVICE_CALLBACK( sdk85_keyscan )
{
	sdk85_state *state = timer.machine().driver_data<sdk85_state>();
	UINT8 keyscan, keytemp = 0xff,i;
	keyscan = input_port_read(timer.machine(), "X0");
	if (keyscan < 0xff)
	{
		for (i = 0; i < 8; i++)
		{
			if (!BIT(keyscan, i))
			{
				keytemp = i;
				i = 9;
			}
		}
	}
	else
	{
		keyscan = input_port_read(timer.machine(), "X1");
		if (keyscan < 0xff)
		{
			for (i = 0; i < 8; i++)
			{
				if (!BIT(keyscan, i))
				{
					keytemp = i+8;
					i = 9;
				}
			}
		}
		else
		{
			keyscan = input_port_read(timer.machine(), "X2");
			if (keyscan < 0xff)
			{
				for (i = 0; i < 6; i++)
				{
					if (!BIT(keyscan, i))
					{
						keytemp = i+16;
						i = 9;
					}
				}
			}
		}
	}

	device_t *cpu = timer.machine().device( "maincpu" );

	if ((state->m_kbd == 0xff) && (keytemp < 0xff) && (keytemp != state->m_keytemp))
	{
		state->m_kbd = keytemp;
		state->m_keytemp = keytemp;
		device_set_input_line(cpu, I8085_RST55_LINE, HOLD_LINE);
	}
	else
	if (keytemp==0xff)
	{
		state->m_keytemp = 0xfe;
		device_set_input_line(cpu, I8085_RST55_LINE, CLEAR_LINE);
	}
}


static MACHINE_CONFIG_START( sdk85, sdk85_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", I8085A, XTAL_2MHz)
	MCFG_CPU_PROGRAM_MAP(sdk85_mem)
	MCFG_CPU_IO_MAP(sdk85_io)

	/* video hardware */
	MCFG_DEFAULT_LAYOUT(layout_sdk85)

	MCFG_TIMER_ADD_PERIODIC("keyscan", sdk85_keyscan, attotime::from_hz(15))
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( sdk85 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "sdk85.a14", 0x0000, 0x0800, CRC(9d5a983f) SHA1(54e218560fbec009ac3de5cfb64b920241ef2eeb))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT   COMPANY   FULLNAME       FLAGS */
COMP( 1977, sdk85,  0,       0,      sdk85,     sdk85,     0,  "Intel",   "SDK-85", GAME_NO_SOUND_HW)

