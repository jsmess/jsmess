/***************************************************************************

        Elektor Junior

        17/07/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "machine/6532riot.h"
#include "junior.lh"


class junior_state : public driver_device
{
public:
	junior_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 m_port_a;
	UINT8 m_port_b;
	UINT8 m_led_time[6];
};




 static ADDRESS_MAP_START(junior_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_GLOBAL_MASK(0x1FFF)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x03ff) AM_RAM // 1K RAM
	AM_RANGE(0x1a00, 0x1a7f) AM_RAM // 6532 RAM
	AM_RANGE(0x1a80, 0x1aff) AM_DEVREADWRITE("riot", riot6532_r, riot6532_w)
	AM_RANGE(0x1c00, 0x1fff) AM_ROM	// Monitor
ADDRESS_MAP_END


static INPUT_CHANGED( junior_reset )
{
	if (newval == 0)
		field->port->machine().firstcpu->reset();
}


/* Input ports */
static INPUT_PORTS_START( junior )
PORT_START("LINE0")			/* IN0 keys row 0 */
	PORT_BIT( 0x80, 0x00, IPT_UNUSED )
	PORT_BIT( 0x40, 0x40, IPT_KEYBOARD ) PORT_NAME("0.6: 0") PORT_CODE(KEYCODE_0)
	PORT_BIT( 0x20, 0x20, IPT_KEYBOARD ) PORT_NAME("0.5: 1") PORT_CODE(KEYCODE_1)
	PORT_BIT( 0x10, 0x10, IPT_KEYBOARD ) PORT_NAME("0.4: 2") PORT_CODE(KEYCODE_2)
	PORT_BIT( 0x08, 0x08, IPT_KEYBOARD ) PORT_NAME("0.3: 3") PORT_CODE(KEYCODE_3)
	PORT_BIT( 0x04, 0x04, IPT_KEYBOARD ) PORT_NAME("0.2: 4") PORT_CODE(KEYCODE_4)
	PORT_BIT( 0x02, 0x02, IPT_KEYBOARD ) PORT_NAME("0.1: 5") PORT_CODE(KEYCODE_5)
	PORT_BIT( 0x01, 0x01, IPT_KEYBOARD ) PORT_NAME("0.0: 6") PORT_CODE(KEYCODE_6)

	PORT_START("LINE1")			/* IN1 keys row 1 */
	PORT_BIT( 0x80, 0x00, IPT_UNUSED )
	PORT_BIT( 0x40, 0x40, IPT_KEYBOARD ) PORT_NAME("1.6: 7") PORT_CODE(KEYCODE_7)
	PORT_BIT( 0x20, 0x20, IPT_KEYBOARD ) PORT_NAME("1.5: 8") PORT_CODE(KEYCODE_8)
	PORT_BIT( 0x10, 0x10, IPT_KEYBOARD ) PORT_NAME("1.4: 9") PORT_CODE(KEYCODE_9)
	PORT_BIT( 0x08, 0x08, IPT_KEYBOARD ) PORT_NAME("1.3: A") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x04, 0x04, IPT_KEYBOARD ) PORT_NAME("1.2: B") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x02, 0x02, IPT_KEYBOARD ) PORT_NAME("1.1: C") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x01, 0x01, IPT_KEYBOARD ) PORT_NAME("1.0: D") PORT_CODE(KEYCODE_D)

	PORT_START("LINE2")			/* IN2 keys row 2 */
	PORT_BIT( 0x80, 0x00, IPT_UNUSED )
	PORT_BIT( 0x40, 0x40, IPT_KEYBOARD ) PORT_NAME("2.6: E") PORT_CODE(KEYCODE_E)
	PORT_BIT( 0x20, 0x20, IPT_KEYBOARD ) PORT_NAME("2.5: F") PORT_CODE(KEYCODE_F)
	PORT_BIT( 0x10, 0x10, IPT_KEYBOARD ) PORT_NAME("2.4: AD (F1)") PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0x08, 0x08, IPT_KEYBOARD ) PORT_NAME("2.3: DA (F2)") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x04, 0x04, IPT_KEYBOARD ) PORT_NAME("2.2: +  (CR)") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT( 0x02, 0x02, IPT_KEYBOARD ) PORT_NAME("2.1: GO (F5)") PORT_CODE(KEYCODE_F5)
	PORT_BIT( 0x01, 0x01, IPT_KEYBOARD ) PORT_NAME("2.0: PC (F6)") PORT_CODE(KEYCODE_F6)

	PORT_START("LINE3")			/* IN3 STEP and RESET keys, MODE switch */
	PORT_BIT( 0x80, 0x00, IPT_UNUSED )
	PORT_BIT( 0x40, 0x40, IPT_KEYBOARD ) PORT_NAME("sw1: ST (F7)") PORT_CODE(KEYCODE_F7)
	PORT_BIT( 0x20, 0x20, IPT_KEYBOARD ) PORT_NAME("sw2: RST (F3)") PORT_CODE(KEYCODE_F3) PORT_CHANGED(junior_reset, NULL)
	PORT_DIPNAME(0x10, 0x10, "sw3: SS (NumLock)") PORT_CODE(KEYCODE_NUMLOCK) PORT_TOGGLE
	PORT_DIPSETTING( 0x00, "single step")
	PORT_DIPSETTING( 0x10, "run")
	PORT_BIT( 0x08, 0x00, IPT_UNUSED )
	PORT_BIT( 0x04, 0x00, IPT_UNUSED )
	PORT_BIT( 0x02, 0x00, IPT_UNUSED )
	PORT_BIT( 0x01, 0x00, IPT_UNUSED )
INPUT_PORTS_END



static READ8_DEVICE_HANDLER(junior_riot_a_r)
{
	junior_state *state = device->machine().driver_data<junior_state>();
	UINT8	data = 0xff;

	switch( ( state->m_port_b >> 1 ) & 0x0f )
	{
	case 0:
		data = input_port_read(device->machine(), "LINE0");
		break;
	case 1:
		data = input_port_read(device->machine(), "LINE1");
		break;
	case 2:
		data = input_port_read(device->machine(), "LINE2");
		break;
	}
	return data;

}


static READ8_DEVICE_HANDLER(junior_riot_b_r)
{
	if ( riot6532_portb_out_get(device) & 0x20 )
		return 0xFF;

	return 0x7F;

}


static WRITE8_DEVICE_HANDLER(junior_riot_a_w)
{
	junior_state *state = device->machine().driver_data<junior_state>();
	UINT8 idx = ( state->m_port_b >> 1 ) & 0x0f;

	state->m_port_a = data;

	if ((idx >= 4 && idx < 10) & ( state->m_port_a != 0xff ))
	{
		output_set_digit_value( idx-4, state->m_port_a ^ 0x7f );
		state->m_led_time[idx - 4] = 10;
	}
}


static WRITE8_DEVICE_HANDLER(junior_riot_b_w)
{
	junior_state *state = device->machine().driver_data<junior_state>();
	UINT8 newdata = data;
	UINT8 idx = ( newdata >> 1 ) & 0x0f;

	state->m_port_b = newdata;

	if ((idx >= 4 && idx < 10) & ( state->m_port_a != 0xff ))
	{
		output_set_digit_value( idx-4, state->m_port_a ^ 0x7f );
		state->m_led_time[idx - 4] = 10;
	}
}


static WRITE_LINE_DEVICE_HANDLER( junior_riot_irq )
{
	cputag_set_input_line(device->machine(), "maincpu", M6502_IRQ_LINE, state ? HOLD_LINE : CLEAR_LINE);
}


static const riot6532_interface junior_riot_interface =
{
	DEVCB_HANDLER(junior_riot_a_r),
	DEVCB_HANDLER(junior_riot_b_r),
	DEVCB_HANDLER(junior_riot_a_w),
	DEVCB_HANDLER(junior_riot_b_w),
	DEVCB_LINE(junior_riot_irq)
};


static TIMER_DEVICE_CALLBACK( junior_update_leds )
{
	junior_state *state = timer.machine().driver_data<junior_state>();
	int i;

	for ( i = 0; i < 6; i++ )
	{
		if ( state->m_led_time[i] )
			state->m_led_time[i]--;
		else
			output_set_digit_value( i, 0 );
	}
}


static MACHINE_START( junior )
{
	junior_state *state = machine.driver_data<junior_state>();
	state_save_register_item(machine, "junior", NULL, 0, state->m_port_a );
	state_save_register_item(machine, "junior", NULL, 0, state->m_port_b );
}


static MACHINE_RESET(junior)
{
	junior_state *state = machine.driver_data<junior_state>();
	int i;

	for ( i = 0; i < 6; i++ )
	{
		state->m_led_time[i] = 0;
	}
}


static MACHINE_CONFIG_START( junior, junior_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",M6502, XTAL_1MHz)
    MCFG_CPU_PROGRAM_MAP(junior_mem)
	MCFG_QUANTUM_TIME(attotime::from_hz(50))

	MCFG_MACHINE_START( junior )
    MCFG_MACHINE_RESET(junior)

    /* video hardware */
    MCFG_DEFAULT_LAYOUT( layout_junior )

    MCFG_RIOT6532_ADD("riot", XTAL_1MHz, junior_riot_interface)

	MCFG_TIMER_ADD_PERIODIC("led_timer", junior_update_leds, attotime::from_hz(50))
MACHINE_CONFIG_END


/* ROM definition */
ROM_START( junior )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
    ROM_DEFAULT_BIOS("orig")
    ROM_SYSTEM_BIOS( 0, "orig", "Original ESS503" )
    ROMX_LOAD( "ess503.ic2", 0x1c00, 0x0400, CRC(9e804f8c) SHA1(181bdb69fb4711cb008e7966747d4775a5e3ef69), ROM_BIOS(1))
    ROM_SYSTEM_BIOS( 1, "mod-orig", "Mod-Original (2708)" )
    ROMX_LOAD( "junior-mod.ic2", 0x1c00, 0x0400, CRC(ee8aa69d) SHA1(a132a51603f1a841c354815e6d868b335ac84364), ROM_BIOS(2))
    ROM_SYSTEM_BIOS( 2, "2732", "Just monitor (2732) " )
    ROMX_LOAD( "junior27321a.ic2", 0x1c00, 0x0400, CRC(e22f24cc) SHA1(a6edb52a9eea5e99624c128065e748e5a3fb2e4c), ROM_BIOS(3))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1980, junior,  0,       0,	junior, 	junior, 	 0,   "Elektor Electronics",   "Junior Computer",		GAME_SUPPORTS_SAVE | GAME_NO_SOUND)

