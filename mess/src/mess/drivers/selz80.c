/***************************************************************************

        SEL Z80 Trainer (LEHRSYSTEME)

        31/08/2010 Skeleton driver.
        23/06/2011 Working [Robbbert]

No diagram has been found. The following is guesswork.

It seems that the keyboard controller has certain similarities to the 8279.
Therefore copied a bunch of code from sdk86 - and it worked.

Test sequence: Press Q, enter an address, press J to show contents, press
               minus or plus to cycle through addresses.

ToDo:
- Artwork
- Various missing LEDs and switches
- Keys are a guess, need to be confirmed.
- Needs to be tested by a subject-matter expert.

****************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "selz80.lh"

#define MACHINE_RESET_MEMBER(name) void name::machine_reset()

class selz80_state : public driver_device
{
public:
	selz80_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

	DECLARE_READ8_MEMBER(selz80_00_r);
	DECLARE_READ8_MEMBER(selz80_01_r);
	DECLARE_WRITE8_MEMBER(selz80_01_w);
	DECLARE_WRITE8_MEMBER(selz80_00_w);
	UINT8 m_digit;
	UINT8 m_segment;
	UINT8 m_kbd;
	UINT8 m_keytemp; // to see when key gets released
	virtual void machine_reset();
};



READ8_MEMBER( selz80_state::selz80_00_r )
{
		UINT8 ret = m_kbd;
		m_kbd = 0xff;
		return ret;
}

READ8_MEMBER( selz80_state::selz80_01_r )
{
	return (m_kbd==0xff) ? 0 : 7;
}

WRITE8_MEMBER( selz80_state::selz80_01_w )
{
	if ((data & 0xc0)==0x80)
	{
		m_digit = data & 7;
		output_set_digit_value(m_digit, m_segment);
	}
}

WRITE8_MEMBER( selz80_state::selz80_00_w )
{
	m_segment = BITSWAP8(data, 3, 2, 1, 0, 7, 6, 5, 4);
	output_set_digit_value(m_digit, m_segment);
}

static ADDRESS_MAP_START(selz80_mem, AS_PROGRAM, 8, selz80_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x1000, 0x1fff) AM_RAM
	AM_RANGE(0xa000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(selz80_io, AS_IO, 8, selz80_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_READWRITE(selz80_00_r,selz80_00_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(selz80_01_r,selz80_01_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( selz80 )
/* 2 x 16-key pads
RS SS RN MM     C(IS)  D(FL)  E(FL') F
EX BP TW TR     8(IX)  9(IY)  A(PC)  B(SP)
RL IN -  +      4(AF') 5(BC') 6(DE') 7(HL')
RG EN SA SD     0(AF)  1(BC)  2(DE)  3(HL)
  */
	PORT_START("X0")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E FL'") PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D FL") PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C IS") PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("MM") PORT_CODE(KEYCODE_Q)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RN") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SS") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RS") PORT_CODE(KEYCODE_T)

	PORT_START("X1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B SP") PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A PC") PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 IY") PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 IX") PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TR") PORT_CODE(KEYCODE_Y)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TW") PORT_CODE(KEYCODE_U)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("BP") PORT_CODE(KEYCODE_I)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("EX") PORT_CODE(KEYCODE_O)

	PORT_START("X2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 HL'") PORT_CODE(KEYCODE_7) PORT_CHAR('7')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 DE'") PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 BC'") PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 AF'") PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("+") PORT_CODE(KEYCODE_EQUALS)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("IN") PORT_CODE(KEYCODE_S)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RL") PORT_CODE(KEYCODE_G)

	PORT_START("X3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 HL") PORT_CODE(KEYCODE_3) PORT_CHAR('3')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 DE") PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 BC") PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0 AF") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SD") PORT_CODE(KEYCODE_H)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SA") PORT_CODE(KEYCODE_J)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("EN") PORT_CODE(KEYCODE_K)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RG") PORT_CODE(KEYCODE_L)
INPUT_PORTS_END

static TIMER_DEVICE_CALLBACK( selz80_keyscan )
{
	selz80_state *state = timer.machine().driver_data<selz80_state>();
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
				for (i = 0; i < 8; i++)
				{
					if (!BIT(keyscan, i))
					{
						keytemp = i+16;
						i = 9;
					}
				}
			}
			else
			{
				keyscan = input_port_read(timer.machine(), "X3");
				if (keyscan < 0xff)
				{
					for (i = 0; i < 8; i++)
					{
						if (!BIT(keyscan, i))
						{
							keytemp = i+24;
							i = 9;
						}
					}
				}
			}
		}
	}

	if ((state->m_kbd == 0xff) && (keytemp < 0xff) && (keytemp != state->m_keytemp))
	{
		state->m_kbd = keytemp;
		state->m_keytemp = keytemp;
	}
	else
	if (keytemp==0xff)
	{
		state->m_keytemp = 0xfe;
	}
}

MACHINE_RESET_MEMBER( selz80_state )
{
}

static MACHINE_CONFIG_START( selz80, selz80_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",Z80, XTAL_4MHz)
	MCFG_CPU_PROGRAM_MAP(selz80_mem)
	MCFG_CPU_IO_MAP(selz80_io)

	/* video hardware */
	MCFG_DEFAULT_LAYOUT(layout_selz80)

	MCFG_TIMER_ADD_PERIODIC("keyscan", selz80_keyscan, attotime::from_hz(15))
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( selz80 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "z80-trainer.rom", 0x0000, 0x1000, CRC(eed1755f) SHA1(72e6ebfccb0e50034660bc36db1a741932311ce1))
	ROM_LOAD( "term80-a000.bin", 0xa000, 0x2000, CRC(0a58c0a7) SHA1(d1b4b3b2ad0d084175b1ff6966653d8b20025252))
	ROM_LOAD( "term80-e000.bin", 0xe000, 0x2000, CRC(158e08e6) SHA1(f1add43bcf8744a01238fb893ee284872d434db5))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1985, selz80,  0,       0,     selz80,    selz80,   0,       "SEL", "Z80 Trainer", GAME_NO_SOUND_HW)
