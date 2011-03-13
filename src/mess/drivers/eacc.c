/******************************************************************************
*
*    EA Car Computer
*        by Robbbert, March 2011.
*
*    Described in Electronics Australia magazine during 1982.
*
*    The only RAM is the 128 bytes that comes inside the CPU.
*
*    This computer is mounted in a car, and various sensors (fuel flow, etc)
*    are connected up. By pressing the appropriate buttons various statistics
*    may be obtained.
*
*    Memory Map
*    0000-03FF internal ram
*    4000-7FFF ROM
*    8000-BFFF 6821
*    C000-FFFF ROM (mirror)
*
*    The ROM was typed in from the dump in the magazine article, therefore
*    it is marked as BAD_DUMP.
*
*    ToDo:
*    - Proper artwork
*    - Change 5th digit to 8 discrete LEDs
*    - Find out how to use it! It *seems* to work but need to check.
*
******************************************************************************/
#define ADDRESS_MAP_MODERN

/* Core includes */
#include "emu.h"
#include "cpu/m6800/m6800.h"
#include "eacc.lh"
#include "machine/6821pia.h"


class eacc_state : public driver_device
{
public:
	eacc_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
	m_maincpu(*this, "maincpu"),
	m_pia(*this, "pia")
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_pia;
	DECLARE_READ_LINE_MEMBER( eacc_cb1_r );
	DECLARE_READ_LINE_MEMBER( eacc_distance_r );
	DECLARE_READ_LINE_MEMBER( eacc_fuel_sensor_r );
	DECLARE_READ8_MEMBER( eacc_keyboard_r );
	DECLARE_WRITE_LINE_MEMBER( eacc_cb2_w );
	DECLARE_WRITE8_MEMBER( eacc_multiplex_w );
	DECLARE_WRITE8_MEMBER( eacc_segment_w );
	UINT8 m_cb1;
	UINT8 m_cb2;
	UINT8 m_multiplex;
	UINT8 m_segment;
	void eacc_display();
	virtual void machine_reset();
};




/******************************************************************************
 Address Maps
******************************************************************************/

static ADDRESS_MAP_START(eacc_mem, ADDRESS_SPACE_PROGRAM, 8, eacc_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xc7ff) // A11,A12,A13 not connected
	AM_RANGE(0x0000, 0x007f) AM_RAM // internal
	AM_RANGE(0xf800, 0xffff) AM_ROM AM_MIRROR(0x8000)
	AM_RANGE(0x8000, 0x8003) AM_MIRROR(0x7fc) AM_DEVREADWRITE_LEGACY("pia", pia6821_r, pia6821_w)
ADDRESS_MAP_END


/******************************************************************************
 Input Ports
******************************************************************************/

static INPUT_PORTS_START(eacc)
	PORT_START("X0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT( 0xf8, 0, IPT_UNUSED )

	PORT_START("X1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3')
	PORT_BIT( 0xf8, 0, IPT_UNUSED )

	PORT_START("X2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT( 0xf8, 0, IPT_UNUSED )

	PORT_START("X3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT( 0xf8, 0, IPT_UNUSED )
INPUT_PORTS_END

void eacc_state::machine_reset()
{
	m_cb2 = 0;
}

static TIMER_DEVICE_CALLBACK( eacc_cb1 )
{
	eacc_state *state = timer.machine->driver_data<eacc_state>();
	state->m_cb1 ^= 1; // 15hz
	//printf("%X ",state->m_cb1);
}

static TIMER_DEVICE_CALLBACK( eacc_nmi )
{
	eacc_state *state = timer.machine->driver_data<eacc_state>();
	if (state->m_cb2)
		cpu_set_input_line(timer.machine->device("maincpu"), INPUT_LINE_NMI, ASSERT_LINE);
}

READ_LINE_MEMBER( eacc_state::eacc_cb1_r )
{
	return m_cb1;
}

READ_LINE_MEMBER( eacc_state::eacc_distance_r )
{
	return 0; // needs random pulses
}

READ_LINE_MEMBER( eacc_state::eacc_fuel_sensor_r )
{
	return 0; // needs random pulses
}

WRITE_LINE_MEMBER( eacc_state::eacc_cb2_w )
{
	m_cb2 = state;
	//printf("%X ",m_cb2);
}

READ8_MEMBER( eacc_state::eacc_keyboard_r )
{
	UINT8 data = m_multiplex;

	if (BIT(m_multiplex, 3))
		data |= input_port_read(machine, "X0");
	if (BIT(m_multiplex, 4))
		data |= input_port_read(machine, "X1");
	if (BIT(m_multiplex, 5))
		data |= input_port_read(machine, "X2");
	if (BIT(m_multiplex, 6))
		data |= input_port_read(machine, "X3");
	//printf("%X ",data);
	return data | m_multiplex;
}

void eacc_state::eacc_display()
{
	for (UINT8 i = 3; i < 8; i++)
		if (BIT(m_multiplex, i))
			output_set_digit_value(i-3, m_segment);
}

WRITE8_MEMBER( eacc_state::eacc_segment_w )
{
    //d7 segment dot
    //d6 segment c
    //d5 segment d
    //d4 segment e
    //d3 segment a
    //d2 segment b
    //d1 segment f
    //d0 segment g

	m_segment = BITSWAP8(data, 7, 0, 1, 4, 5, 6, 2, 3);
	//printf("%X ",data);
	eacc_display();
}

WRITE8_MEMBER( eacc_state::eacc_multiplex_w )
{
	cpu_set_input_line(machine->device("maincpu"), INPUT_LINE_NMI, CLEAR_LINE);
	m_multiplex = data & 0xf8;
	eacc_display();
}

static const pia6821_interface eacc_mc6821_intf =
{
	DEVCB_NULL,	/* port A input */
	DEVCB_DRIVER_MEMBER(eacc_state, eacc_keyboard_r),	/* port B input - PB0,1,2 keyboard */
	DEVCB_DRIVER_LINE_MEMBER(eacc_state, eacc_distance_r),	/* CA1 input - pulses as car moves */
	DEVCB_DRIVER_LINE_MEMBER(eacc_state, eacc_cb1_r),	/* CB1 input - NMI pulse at 15Hz */
	DEVCB_DRIVER_LINE_MEMBER(eacc_state, eacc_fuel_sensor_r),	/* CA2 input - pulses as fuel consumed */
	DEVCB_NULL,						/* CB2 input */
	DEVCB_DRIVER_MEMBER(eacc_state, eacc_segment_w),	/* port A output */
	DEVCB_DRIVER_MEMBER(eacc_state, eacc_multiplex_w),	/* port B output */
	DEVCB_NULL,						/* CA2 output */
	DEVCB_DRIVER_LINE_MEMBER(eacc_state, eacc_cb2_w),	/* CB2 output - high after boot complete */
	DEVCB_CPU_INPUT_LINE("maincpu", M6800_IRQ_LINE),	/* IRQA output */
	DEVCB_CPU_INPUT_LINE("maincpu", M6800_IRQ_LINE)		/* IRQB output */
};


/******************************************************************************
 Machine Drivers
******************************************************************************/

static MACHINE_CONFIG_START( eacc, eacc_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M6802, XTAL_3_579545MHz)  /* Divided by 4 inside the m6802*/
	MCFG_CPU_PROGRAM_MAP(eacc_mem)

	MCFG_DEFAULT_LAYOUT(layout_eacc)

	MCFG_PIA6821_ADD("pia", eacc_mc6821_intf)
	MCFG_TIMER_ADD_PERIODIC("eacc_nmi", eacc_nmi, attotime::from_hz(600) )
	MCFG_TIMER_ADD_PERIODIC("eacc_cb1", eacc_cb1, attotime::from_hz(30) )
MACHINE_CONFIG_END



/******************************************************************************
 ROM Definitions
******************************************************************************/

ROM_START(eacc)
	ROM_REGION(0x10000, "maincpu", 0)
	ROM_LOAD("eacc.bin", 0x4000, 0x0800, BAD_DUMP CRC(287a63c0) SHA1(f61b397d33ea40e5742e34d5f5468572125e8b39) )
ROM_END


/******************************************************************************
 Drivers
******************************************************************************/

/*    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT   INIT      COMPANY                     FULLNAME                            FLAGS */
COMP( 1982, eacc,       0,          0,      eacc,       eacc,   0,     "Electronics Australia", "EA Car Computer", GAME_NOT_WORKING | GAME_NO_SOUND_HW)

