/***************************************************************************

	IBM PC AT compatibles 8042 keyboard controller

***************************************************************************/

#include "emu.h"
#include "at_keybc.h"
#include "cpu/mcs48/mcs48.h"


//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type AT_KEYBOARD_CONTROLLER = at_keyboard_controller_device_config::static_alloc_device_config;

// i/o map for the 8042
static ADDRESS_MAP_START( at_keybc_io, AS_IO, 8 )
	AM_RANGE(MCS48_PORT_T0, MCS48_PORT_T0) AM_DEVREAD_MODERN(DEVICE_SELF_OWNER, at_keyboard_controller_device, t0_r)
	AM_RANGE(MCS48_PORT_T1, MCS48_PORT_T1) AM_DEVREAD_MODERN(DEVICE_SELF_OWNER, at_keyboard_controller_device, t1_r)
	AM_RANGE(MCS48_PORT_P1, MCS48_PORT_P1) AM_DEVREAD_MODERN(DEVICE_SELF_OWNER, at_keyboard_controller_device, p1_r)
	AM_RANGE(MCS48_PORT_P2, MCS48_PORT_P2) AM_DEVREADWRITE_MODERN(DEVICE_SELF_OWNER, at_keyboard_controller_device, p2_r, p2_w)
ADDRESS_MAP_END

// machine fragment
static MACHINE_CONFIG_FRAGMENT( at_keybc )
	MCFG_CPU_ADD("at_keybc", I8042, DERIVED_CLOCK(1,1))
	MCFG_CPU_IO_MAP(at_keybc_io)
MACHINE_CONFIG_END

// rom definition for the 8042 internal rom
ROM_START( at_keybc )
	ROM_REGION(0x800, "at_keybc", ROMREGION_LOADBYNAME)

	// unknown controller bios, (c) 1985, 1986 PTL
	ROM_LOAD("yan25d05.bin", 0x000, 0x800, CRC(70c798f1) SHA1(ae9a79c7184a17331b70a50035ff63c757df094c))

	// 1983 ibm controller bios
	ROM_LOAD("1503033.bin", 0x000, 0x800, CRC(5a81c0d2) SHA1(0100f8789fb4de74706ae7f9473a12ec2b9bd729))
ROM_END


//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  at_keyboard_controller_device_config - constructor
//-------------------------------------------------

at_keyboard_controller_device_config::at_keyboard_controller_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
	: device_config(mconfig, static_alloc_device_config, "AT Keyboard Controller", tag, owner, clock)
{
	m_shortname = "at_keybc";
}

//-------------------------------------------------
//  static_alloc_device_config - allocate a new
//  configuration object
//-------------------------------------------------

device_config *at_keyboard_controller_device_config::static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
{
	return global_alloc(at_keyboard_controller_device_config(mconfig, tag, owner, clock));
}

//-------------------------------------------------
//  alloc_device - allocate a new device object
//-------------------------------------------------

device_t *at_keyboard_controller_device_config::alloc_device(running_machine &machine) const
{
	return auto_alloc(machine, at_keyboard_controller_device(machine, *this));
}

//-------------------------------------------------
//  rom_region - return a pointer to the device's
//  internal ROM region
//-------------------------------------------------

const rom_entry *at_keyboard_controller_device_config::device_rom_region() const
{
	return ROM_NAME(at_keybc);
}

//-------------------------------------------------
//  machine_config_additions - return a pointer to
//  the device's machine fragment
//-------------------------------------------------

machine_config_constructor at_keyboard_controller_device_config::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME(at_keybc);
}

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void at_keyboard_controller_device_config::device_config_complete()
{
	// inherit a copy of the static data
	const at_keyboard_controller_interface *intf = reinterpret_cast<const at_keyboard_controller_interface *>(static_config());

	if (intf != NULL)
	{
		*static_cast<at_keyboard_controller_interface *>(this) = *intf;
	}

	// or initialize to defaults if none provided
	else
	{
		memset(&m_system_reset_func, 0, sizeof(m_system_reset_func));
		memset(&m_gate_a20_func, 0, sizeof(m_gate_a20_func));
		memset(&m_input_buffer_full_func, 0, sizeof(m_input_buffer_full_func));
		memset(&m_output_buffer_empty_func, 0, sizeof(m_output_buffer_empty_func));
		memset(&m_keyboard_clock_func, 0, sizeof(m_keyboard_clock_func));
		memset(&m_keyboard_data_func, 0, sizeof(m_keyboard_data_func));
	}
}


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  at_keyboard_controller_device - constructor
//-------------------------------------------------

at_keyboard_controller_device::at_keyboard_controller_device(running_machine &_machine, const at_keyboard_controller_device_config &config)
	: device_t(_machine, config),
	  m_config(config),
	  m_cpu(NULL)
{
}

/*-------------------------------------------------
    device_start - device-specific startup
-------------------------------------------------*/

void at_keyboard_controller_device::device_start()
{
	// find our cpu
	m_cpu = downcast<device_t *>(subdevice("at_keybc"));

	// resolve callbacks
	devcb_resolve_write_line(&m_system_reset_func, &m_config.m_system_reset_func, this);
	devcb_resolve_write_line(&m_gate_a20_func, &m_config.m_gate_a20_func, this);
	devcb_resolve_write_line(&m_input_buffer_full_func, &m_config.m_input_buffer_full_func, this);
	devcb_resolve_write_line(&m_output_buffer_empty_func, &m_config.m_output_buffer_empty_func, this);
	devcb_resolve_write_line(&m_keyboard_clock_func, &m_config.m_keyboard_clock_func, this);
	devcb_resolve_write_line(&m_keyboard_data_func, &m_config.m_keyboard_data_func, this);

	// register for save states
	save_item(NAME(m_clock_signal));
	save_item(NAME(m_data_signal));
}

/*-------------------------------------------------
    device_reset - device-specific reset
-------------------------------------------------*/

void at_keyboard_controller_device::device_reset()
{
}


//**************************************************************************
//  INTERNAL 8042 READ/WRITE HANDLERS
//**************************************************************************

READ8_MEMBER( at_keyboard_controller_device::t0_r )
{
	return m_clock_signal;
}

READ8_MEMBER( at_keyboard_controller_device::t1_r )
{
	return m_data_signal;
}

/*
	Port 1 (Input port)
	0 - P10 - Undefined
	1 - P11 - Undefined
	2 - P12 - Undefined
	3 - P13 - Undefined
	4 - P14 - External RAM (1 = Enable external RAM, 0 = Disable external RAM)
	5 - P15 - Manufacturing setting (1 = Setting enabled, 0 = Setting disabled)
	6 - P16 - Display type switch (1 = Monochrome display, 0 = Color display)
	7 - P17 - Keyboard inhibit switch (1 = Keyboard enabled, 0 = Keyboard inhibited)
*/
READ8_MEMBER( at_keyboard_controller_device::p1_r )
{
	return 0xbf;
}

READ8_MEMBER( at_keyboard_controller_device::p2_r )
{
	return 0xff;
}

/*
	Port 2 (Output port)
	0 - P20 - System Reset (1 = Normal, 0 = Reset computer)
	1 - P21 - Gate A20
	2 - P22 - Undefined
	3 - P23 - Undefined
	4 - P24 - Input Buffer Full
	5 - P25 - Output Buffer Empty
	6 - P26 - Keyboard Clock (1 = Pull Clock low, 0 = High-Z)
	7 - P27 - Keyboard Data (1 = Pull Data low, 0 = High-Z)
*/
WRITE8_MEMBER( at_keyboard_controller_device::p2_w )
{
	devcb_call_write_line(&m_system_reset_func, BIT(data, 0) ? CLEAR_LINE : ASSERT_LINE);
	devcb_call_write_line(&m_gate_a20_func, BIT(data, 1) ? ASSERT_LINE : CLEAR_LINE);
	devcb_call_write_line(&m_input_buffer_full_func, BIT(data, 4) ? ASSERT_LINE : CLEAR_LINE);
	devcb_call_write_line(&m_output_buffer_empty_func, BIT(data, 5) ? ASSERT_LINE : CLEAR_LINE);

	m_clock_signal = !BIT(data, 6);
	m_data_signal = BIT(data, 7);

	devcb_call_write_line(&m_keyboard_data_func, m_data_signal);
	devcb_call_write_line(&m_keyboard_clock_func, m_clock_signal);
}


//**************************************************************************
//  READ/WRITE HANDLERS
//**************************************************************************

READ8_MEMBER( at_keyboard_controller_device::data_r )
{
	return upi41_master_r(m_cpu, 0);
}

WRITE8_MEMBER( at_keyboard_controller_device::data_w )
{
	upi41_master_w(m_cpu, 0, data);
}

READ8_MEMBER( at_keyboard_controller_device::status_r )
{
	return upi41_master_r(m_cpu, 1);
}

WRITE8_MEMBER( at_keyboard_controller_device::command_w )
{
	upi41_master_w(m_cpu, 1, data);
}

WRITE_LINE_MEMBER( at_keyboard_controller_device::keyboard_clock_w )
{
	m_clock_signal = state;
}

WRITE_LINE_MEMBER( at_keyboard_controller_device::keyboard_data_w )
{
	m_data_signal = state;
}
