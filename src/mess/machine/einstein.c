#include "emu.h"
#include "includes/einstein.h"

/****************************************************************
    EINSTEIN NON-Z80 DEVICES DAISY CHAIN SUPPORT
****************************************************************/

//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  einstein_keyboard_daisy_device_config - constructor
//-------------------------------------------------

einstein_keyboard_daisy_device_config::einstein_keyboard_daisy_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
	: device_config(mconfig, static_alloc_device_config, "Einstein keyboard daisy chain", tag, owner, clock),
	  device_config_z80daisy_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  static_alloc_device_config - allocate a new
//  configuration object
//-------------------------------------------------

device_config *einstein_keyboard_daisy_device_config::static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
{
	return global_alloc(einstein_keyboard_daisy_device_config(mconfig, tag, owner, clock));
}


//-------------------------------------------------
//  alloc_device - allocate a new device object
//-------------------------------------------------

device_t *einstein_keyboard_daisy_device_config::alloc_device(running_machine &machine) const
{
	return auto_alloc(machine, einstein_keyboard_daisy_device(machine, *this));
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  z80ctc_device - constructor
//-------------------------------------------------

einstein_keyboard_daisy_device::einstein_keyboard_daisy_device(running_machine &_machine, const einstein_keyboard_daisy_device_config &_config)
	: device_t(_machine, _config),
	  device_z80daisy_interface(_machine, _config, *this),
	  m_config(_config)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void einstein_keyboard_daisy_device::device_start()
{
}

//**************************************************************************
//  DAISY CHAIN INTERFACE
//**************************************************************************

//-------------------------------------------------
//  z80daisy_irq_state - return the overall IRQ
//  state for this device
//-------------------------------------------------

int einstein_keyboard_daisy_device::z80daisy_irq_state()
{
	einstein_state *einstein = device().machine().driver_data<einstein_state>();

	if (einstein->m_interrupt & einstein->m_interrupt_mask & EINSTEIN_KEY_INT)
		return Z80_DAISY_INT;

	return 0;
}


//-------------------------------------------------
//  z80daisy_irq_ack - acknowledge an IRQ and
//  return the appropriate vector
//-------------------------------------------------

int einstein_keyboard_daisy_device::z80daisy_irq_ack()
{
	return 0xf7;
}

//-------------------------------------------------
//  z80daisy_irq_reti - clear the interrupt
//  pending state to allow other interrupts through
//-------------------------------------------------

void einstein_keyboard_daisy_device::z80daisy_irq_reti()
{
}

const device_type EINSTEIN_KEYBOARD_DAISY = einstein_keyboard_daisy_device_config::static_alloc_device_config;

//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  einstein_adc_daisy_device_config - constructor
//-------------------------------------------------

einstein_adc_daisy_device_config::einstein_adc_daisy_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
	: device_config(mconfig, static_alloc_device_config, "Einstein ADC daisy chain", tag, owner, clock),
	  device_config_z80daisy_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  static_alloc_device_config - allocate a new
//  configuration object
//-------------------------------------------------

device_config *einstein_adc_daisy_device_config::static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
{
	return global_alloc(einstein_adc_daisy_device_config(mconfig, tag, owner, clock));
}


//-------------------------------------------------
//  alloc_device - allocate a new device object
//-------------------------------------------------

device_t *einstein_adc_daisy_device_config::alloc_device(running_machine &machine) const
{
	return auto_alloc(machine, einstein_adc_daisy_device(machine, *this));
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  z80ctc_device - constructor
//-------------------------------------------------

einstein_adc_daisy_device::einstein_adc_daisy_device(running_machine &_machine, const einstein_adc_daisy_device_config &_config)
	: device_t(_machine, _config),
	  device_z80daisy_interface(_machine, _config, *this),
	  m_config(_config)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void einstein_adc_daisy_device::device_start()
{
}

//**************************************************************************
//  DAISY CHAIN INTERFACE
//**************************************************************************

//-------------------------------------------------
//  z80daisy_irq_state - return the overall IRQ
//  state for this device
//-------------------------------------------------

int einstein_adc_daisy_device::z80daisy_irq_state()
{
	einstein_state *einstein = device().machine().driver_data<einstein_state>();

	if (einstein->m_interrupt & einstein->m_interrupt_mask & EINSTEIN_ADC_INT)
		return Z80_DAISY_INT;

	return 0;
}


//-------------------------------------------------
//  z80daisy_irq_ack - acknowledge an IRQ and
//  return the appropriate vector
//-------------------------------------------------

int einstein_adc_daisy_device::z80daisy_irq_ack()
{
	return 0xfb;
}

//-------------------------------------------------
//  z80daisy_irq_reti - clear the interrupt
//  pending state to allow other interrupts through
//-------------------------------------------------

void einstein_adc_daisy_device::z80daisy_irq_reti()
{
}

const device_type EINSTEIN_ADC_DAISY = einstein_adc_daisy_device_config::static_alloc_device_config;

//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  einstein_fire_daisy_device_config - constructor
//-------------------------------------------------

einstein_fire_daisy_device_config::einstein_fire_daisy_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
	: device_config(mconfig, static_alloc_device_config, "Einstein fire button daisy chain", tag, owner, clock),
	  device_config_z80daisy_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  static_alloc_device_config - allocate a new
//  configuration object
//-------------------------------------------------

device_config *einstein_fire_daisy_device_config::static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
{
	return global_alloc(einstein_fire_daisy_device_config(mconfig, tag, owner, clock));
}


//-------------------------------------------------
//  alloc_device - allocate a new device object
//-------------------------------------------------

device_t *einstein_fire_daisy_device_config::alloc_device(running_machine &machine) const
{
	return auto_alloc(machine, einstein_fire_daisy_device(machine, *this));
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  z80ctc_device - constructor
//-------------------------------------------------

einstein_fire_daisy_device::einstein_fire_daisy_device(running_machine &_machine, const einstein_fire_daisy_device_config &_config)
	: device_t(_machine, _config),
	  device_z80daisy_interface(_machine, _config, *this),
	  m_config(_config)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void einstein_fire_daisy_device::device_start()
{
}

//**************************************************************************
//  DAISY CHAIN INTERFACE
//**************************************************************************

//-------------------------------------------------
//  z80daisy_irq_state - return the overall IRQ
//  state for this device
//-------------------------------------------------

int einstein_fire_daisy_device::z80daisy_irq_state()
{
  einstein_state *einstein = device().machine().driver_data<einstein_state>();

  if (einstein->m_interrupt & einstein->m_interrupt_mask & EINSTEIN_FIRE_INT)
      return Z80_DAISY_INT;

  return 0;
}


//-------------------------------------------------
//  z80daisy_irq_ack - acknowledge an IRQ and
//  return the appropriate vector
//-------------------------------------------------

int einstein_fire_daisy_device::z80daisy_irq_ack()
{
	return 0xfd;
}

//-------------------------------------------------
//  z80daisy_irq_reti - clear the interrupt
//  pending state to allow other interrupts through
//-------------------------------------------------

void einstein_fire_daisy_device::z80daisy_irq_reti()
{
}

const device_type EINSTEIN_FIRE_DAISY = einstein_fire_daisy_device_config::static_alloc_device_config;
