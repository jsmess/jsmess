/***************************************************************************

        Schachcomputer SC2

        Node:
         The HW is very similar to Fidelity Chess Challenger series (see fidelz80.c)

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/z80pio.h"
#include "sound/beep.h"
#include "sc2.lh"

class sc2_state : public driver_device
{
public:
	sc2_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 m_kp_matrix;
	UINT8 m_led_7seg_data[4];
	UINT8 m_led_selected;
	UINT8 m_digit_data;
	UINT8 m_beep_state;

	device_t *m_beep;
};

static READ8_HANDLER ( sc2_beep )
{
	sc2_state *state = space->machine().driver_data<sc2_state>();

	if (!space->debugger_access())
	{
		state->m_beep_state = ~state->m_beep_state;

		beep_set_state(state->m_beep, state->m_beep_state);
	}

	return 0xff;
}

static ADDRESS_MAP_START(sc2_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x0fff ) AM_ROM
	AM_RANGE( 0x1000, 0x13ff ) AM_RAM
	AM_RANGE( 0x2000, 0x33ff ) AM_ROM
	AM_RANGE( 0x3c00, 0x3c00 ) AM_READ(sc2_beep)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sc2_io , AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x03) AM_MIRROR(0xfc) AM_DEVREADWRITE("z80pio", z80pio_cd_ba_r, z80pio_cd_ba_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( sc2 )
	PORT_START("LINE1")
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_UNUSED
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)

	PORT_START("LINE2")
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("A1") PORT_CODE(KEYCODE_1)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("B2") PORT_CODE(KEYCODE_2)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("C3") PORT_CODE(KEYCODE_3)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("D4") PORT_CODE(KEYCODE_4)

	PORT_START("LINE3")
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("E5") PORT_CODE(KEYCODE_5)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("F6") PORT_CODE(KEYCODE_6)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("G7") PORT_CODE(KEYCODE_7)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("H8") PORT_CODE(KEYCODE_8)

	PORT_START("LINE4")
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("K") PORT_CODE(KEYCODE_K)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("W") PORT_CODE(KEYCODE_W)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYPAD) PORT_NAME("P") PORT_CODE(KEYCODE_O)
INPUT_PORTS_END

static MACHINE_START(sc2)
{
	sc2_state *state = machine.driver_data<sc2_state>();

	state->m_beep = machine.device("beep");

	state->save_item(NAME(state->m_led_7seg_data));
	state->save_item(NAME(state->m_kp_matrix));
	state->save_item(NAME(state->m_led_selected));
	state->save_item(NAME(state->m_digit_data));
	state->save_item(NAME(state->m_beep_state));
}

static MACHINE_RESET(sc2)
{
	sc2_state *state = machine.driver_data<sc2_state>();

	state->m_kp_matrix = 0;
	state->m_led_selected = 0;
	state->m_digit_data = 0;
	state->m_beep_state = 0;
	memset(state->m_led_7seg_data, 0, ARRAY_LENGTH(state->m_led_7seg_data));
}

static void sc2_update_display(running_machine &machine)
{
	sc2_state *state = machine.driver_data<sc2_state>();
	UINT8 digit_data = BITSWAP8( state->m_digit_data,7,0,1,2,3,4,5,6 ) & 0x7f;

	if (!(state->m_led_selected&0x01))
	{
		output_set_digit_value(0, digit_data);
		state->m_led_7seg_data[0] = digit_data;

		output_set_led_value(0, (state->m_digit_data & 0x80) ? 1 : 0);
	}
	if (!(state->m_led_selected&0x02))
	{
		output_set_digit_value(1, digit_data);
		state->m_led_7seg_data[1] = digit_data;

		output_set_led_value(1, (state->m_digit_data & 0x80) ? 1 : 0);
	}
	if (!(state->m_led_selected&0x04))
	{
		output_set_digit_value(2, digit_data);
		state->m_led_7seg_data[2] = digit_data;
	}
	if (!(state->m_led_selected&0x08))
	{
		output_set_digit_value(3, digit_data);
		state->m_led_7seg_data[3] = digit_data;
	}
}

static READ8_DEVICE_HANDLER( pio_port_a_r )
{
	sc2_state *state = device->machine().driver_data<sc2_state>();

	return state->m_digit_data;
}

static READ8_DEVICE_HANDLER( pio_port_b_r )
{
	sc2_state *state = device->machine().driver_data<sc2_state>();

	UINT8 data = state->m_led_selected & 0x0f;

	if ((state->m_kp_matrix&0x01))
	{
		data |= input_port_read(device->machine(), "LINE1");
	}
	if ((state->m_kp_matrix&0x02))
	{
		data |= input_port_read(device->machine(), "LINE2");
	}
	if ((state->m_kp_matrix&0x04))
	{
		data |= input_port_read(device->machine(), "LINE3");
	}
	if ((state->m_kp_matrix&0x08))
	{
		data |= input_port_read(device->machine(), "LINE4");
	}

	return data;
}

static WRITE8_DEVICE_HANDLER( pio_port_a_w )
{
	sc2_state *state = device->machine().driver_data<sc2_state>();

	state->m_digit_data = data;
}

static WRITE8_DEVICE_HANDLER( pio_port_b_w )
{
	sc2_state *state = device->machine().driver_data<sc2_state>();

	if (data != 0xf1 && data != 0xf2 && data != 0xf4 && data != 0xf8)
	{
		state->m_led_selected = data;
		sc2_update_display(device->machine());
	}
	else
		state->m_kp_matrix = data;
};

static Z80PIO_INTERFACE( pio_intf )
{
	DEVCB_NULL,						/* callback when change interrupt status */
	DEVCB_HANDLER(pio_port_a_r),	/* port A read callback */
	DEVCB_HANDLER(pio_port_a_w),	/* port A write callback */
	DEVCB_NULL,						/* portA ready active callback */
	DEVCB_HANDLER(pio_port_b_r),	/* port B read callback */
	DEVCB_HANDLER(pio_port_b_w),	/* port B write callback */
	DEVCB_NULL						/* portB ready active callback */
};

static MACHINE_CONFIG_START( sc2, sc2_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MCFG_CPU_PROGRAM_MAP(sc2_mem)
    MCFG_CPU_IO_MAP(sc2_io)

    MCFG_MACHINE_START(sc2)
    MCFG_MACHINE_RESET(sc2)

    /* video hardware */
	MCFG_DEFAULT_LAYOUT(layout_sc2)

	/* devices */
	MCFG_Z80PIO_ADD("z80pio", XTAL_4MHz, pio_intf)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO( "mono" )
	MCFG_SOUND_ADD( "beep", BEEP, 0 )
	MCFG_SOUND_ROUTE( ALL_OUTPUTS, "mono", 1.00 )
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( sc2 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "bm008.bin", 0x0000, 0x0400, CRC(3023ea82) SHA1(07020d153d802c672c39e1af3c716dbe35e23f08))
	ROM_LOAD( "bm009.bin", 0x0400, 0x0400, CRC(6a34814e) SHA1(e58ae6615297b028db135a48a8f9e186a4220f4f))
	ROM_LOAD( "bm010.bin", 0x0800, 0x0400, CRC(deab0373) SHA1(81c9a7197eef8d9131e47ecd2ec35b943caee54e))
	ROM_LOAD( "bm011.bin", 0x0c00, 0x0400, CRC(c8282339) SHA1(8d6b8861281e967a77609b6d77e80afd47d28ed2))
	ROM_LOAD( "bm012.bin", 0x2000, 0x0400, CRC(2e6a4294) SHA1(7b9bd191c9ec73139a65c3a339ab88e1f3eb5ed2))
	ROM_LOAD( "bm013.bin", 0x2400, 0x0400, CRC(3e02eb42) SHA1(2e4a9a8fd04c202c9518550d7e8cf9bfea394153))
	ROM_LOAD( "bm014.bin", 0x2800, 0x0400, CRC(538d449e) SHA1(c4186995b69e97740e01eaff84a20d49d03d180f))
	ROM_LOAD( "bm015.bin", 0x2c00, 0x0400, CRC(b4991dca) SHA1(6a6cdddf5c4afa24773acf693f58c34b99c8d328))
	ROM_LOAD( "bm016.bin", 0x3000, 0x0400, CRC(4fe0853a) SHA1(c2253e320778b0ea468fb54f26ae83d07f9700e6))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1981, sc2,  0,       0,	sc2,	sc2,	 0, 			 "VEB Mikroelektronik Erfurt",   "Schachcomputer SC2",		GAME_SUPPORTS_SAVE)

