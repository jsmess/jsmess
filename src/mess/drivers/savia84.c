/***************************************************************************

        Savia 84

        More data at :
                http://www.nostalcomp.cz/pdfka/savia84.pdf

        05/02/2011 Skeleton driver.

        According to the schematic, the rom is at 0000-03FF (no mirrors),
        and the RAM is at 1800-1FFF (no mirrors). However the first thing
        the rom does is to jump to 2061, which would cause an instant crash.
        Perhaps it is a bad dump? It doesn't do anything sensible at the
        moment.

        All photos of this computer are of it pulled apart. There are no
        photos of it running. I assume all the LEDs are red ones. The LEDs
        down the left side I assume to be bit 0 through 7 in that order.

        ToDo:
        - Make better artwork
        - It should run but bad rom suspected

****************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/i8255a.h"
#include "savia84.lh"

class savia84_state : public driver_device
{
public:
	savia84_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		m_maincpu(*this, "maincpu"),
		m_ppi8255(*this, "ppi8255")
		{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_ppi8255;
	DECLARE_READ8_MEMBER( savia84_8255_portc_r );
	DECLARE_WRITE8_MEMBER( savia84_8255_porta_w );
	DECLARE_WRITE8_MEMBER( savia84_8255_portb_w );
	DECLARE_WRITE8_MEMBER( savia84_8255_portc_w );
	UINT8 m_kbd;
	UINT8 m_segment;
	UINT8 m_digit;
};

static ADDRESS_MAP_START(savia84_mem, AS_PROGRAM, 8, savia84_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0x7fff) // A15 not connected at the cPU
	AM_RANGE(0x0000, 0x03ff) AM_ROM AM_MIRROR(0x2000) AM_WRITENOP // see notes above
	AM_RANGE(0x1800, 0x1fff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( savia84_io , AS_IO, 8, savia84_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0x07)
	AM_RANGE(0x00, 0x03) AM_DEVREADWRITE_LEGACY("ppi8255", i8255a_r, i8255a_w) // ports F8-FB
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( savia84 )
	PORT_START("X0")
	PORT_BIT( 0x9F, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0')

	PORT_START("X1")
	PORT_BIT( 0x8F, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("S") PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("6") PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("4") PORT_CODE(KEYCODE_4) PORT_CHAR('4')

	PORT_START("X2")
	PORT_BIT( 0x8F, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("L") PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("8") PORT_CODE(KEYCODE_8) PORT_CHAR('8')

	PORT_START("X3")
	PORT_BIT( 0x9F, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("R") PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Ex") PORT_CODE(KEYCODE_X) PORT_CHAR('X')

	PORT_START("X4")
	PORT_BIT( 0x8F, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("BR") PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("D") PORT_CODE(KEYCODE_D) PORT_CHAR('D')

	PORT_START("X5")
	PORT_BIT( 0x8F, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("AD") PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("C") PORT_CODE(KEYCODE_C) PORT_CHAR('C')

	PORT_START("X6")
	PORT_BIT( 0x9F, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CHAR('9')

	PORT_START("X7")
	PORT_BIT( 0x9F, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CHAR('7')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("5") PORT_CODE(KEYCODE_5) PORT_CHAR('5')

	PORT_START("X8")
	PORT_BIT( 0x8F, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("DA") PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CHAR('3')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CHAR('1')
INPUT_PORTS_END


static MACHINE_RESET(savia84)
{
}

WRITE8_MEMBER( savia84_state::savia84_8255_porta_w ) // OUT F8 - output segments on the selected digit
{
	m_segment = ~data & 0x7f;
	if (m_digit) output_set_digit_value(m_digit, m_segment);
}

WRITE8_MEMBER( savia84_state::savia84_8255_portb_w ) // OUT F9 - light the 8 leds down the left side
{
	char ledname[8];
	for (int i = 0; i < 8; i++)
	{
		sprintf(ledname,"led%d",i);
		output_set_value(ledname, BIT(data, i));
	}
}

WRITE8_MEMBER( savia84_state::savia84_8255_portc_w ) // OUT FA - set keyboard scanning row; set digit to display
{
	m_digit = 0;
	m_kbd = data & 15;
	if (m_kbd == 0)
		m_digit = 1;
	else
	if ((m_kbd > 1) && (m_kbd < 9))
		m_digit = m_kbd;

	if (m_digit) output_set_digit_value(m_digit, m_segment);
}

READ8_MEMBER( savia84_state::savia84_8255_portc_r ) // IN FA - read keyboard
{
	if (m_kbd < 9)
	{
		char kbdrow[4];
		sprintf(kbdrow,"X%d",m_kbd);
		return input_port_read(m_machine, kbdrow);
	}
	else
		return 0xff;
}


I8255A_INTERFACE( savia84_ppi8255_interface )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(savia84_state, savia84_8255_portc_r),
	DEVCB_DRIVER_MEMBER(savia84_state, savia84_8255_porta_w),
	DEVCB_DRIVER_MEMBER(savia84_state, savia84_8255_portb_w),
	DEVCB_DRIVER_MEMBER(savia84_state, savia84_8255_portc_w)
};

static MACHINE_CONFIG_START( savia84, savia84_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",Z80, XTAL_4MHz / 2)
	MCFG_CPU_PROGRAM_MAP(savia84_mem)
	MCFG_CPU_IO_MAP(savia84_io)

	MCFG_MACHINE_RESET(savia84)

	/* video hardware */
	MCFG_DEFAULT_LAYOUT(layout_savia84)

	/* Devices */
	MCFG_I8255A_ADD( "ppi8255", savia84_ppi8255_interface )
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( savia84 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD("savia84_1kb.bin", 0x0000, 0x0400, CRC(23a5c15e) SHA1(7e769ed8960d8c591a25cfe4effffcca3077c94b)) // 2758 ROM - 1KB
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1984, savia84,  0,       0,	savia84,	savia84,	 0,   "<unknown>",   "Savia 84", GAME_NOT_WORKING | GAME_NO_SOUND_HW)

