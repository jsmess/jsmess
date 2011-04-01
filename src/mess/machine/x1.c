#include "emu.h"
#include "includes/x1.h"

//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  x1_keyboard_device_config - constructor
//-------------------------------------------------

x1_keyboard_device_config::x1_keyboard_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
	: device_config(mconfig, static_alloc_device_config, "X1 Keyboard", tag, owner, clock),
	  device_config_z80daisy_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  static_alloc_device_config - allocate a new
//  configuration object
//-------------------------------------------------

device_config *x1_keyboard_device_config::static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
{
	return global_alloc(x1_keyboard_device_config(mconfig, tag, owner, clock));
}


//-------------------------------------------------
//  alloc_device - allocate a new device object
//-------------------------------------------------

device_t *x1_keyboard_device_config::alloc_device(running_machine &machine) const
{
	return auto_alloc(machine, x1_keyboard_device(machine, *this));
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  z80ctc_device - constructor
//-------------------------------------------------

x1_keyboard_device::x1_keyboard_device(running_machine &_machine, const x1_keyboard_device_config &_config)
	: device_t(_machine, _config),
	  device_z80daisy_interface(_machine, _config, *this),
	  m_config(_config)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void x1_keyboard_device::device_start()
{
}

//**************************************************************************
//  DAISY CHAIN INTERFACE
//**************************************************************************

//-------------------------------------------------
//  z80daisy_irq_state - return the overall IRQ
//  state for this device
//-------------------------------------------------

int x1_keyboard_device::z80daisy_irq_state()
{
	x1_state *state = m_machine.driver_data<x1_state>();
	if(state->m_key_irq_flag != 0)
		return Z80_DAISY_INT;
	return 0;
}


//-------------------------------------------------
//  z80daisy_irq_ack - acknowledge an IRQ and
//  return the appropriate vector
//-------------------------------------------------

int x1_keyboard_device::z80daisy_irq_ack()
{
	x1_state *state = m_machine.driver_data<x1_state>();
	state->m_key_irq_flag = 0;
	cputag_set_input_line(device().machine(),"maincpu",INPUT_LINE_IRQ0,CLEAR_LINE);
	return state->m_key_irq_vector;
}

//-------------------------------------------------
//  z80daisy_irq_reti - clear the interrupt
//  pending state to allow other interrupts through
//-------------------------------------------------

void x1_keyboard_device::z80daisy_irq_reti()
{
}

const device_type X1_KEYBOARD = x1_keyboard_device_config::static_alloc_device_config;
