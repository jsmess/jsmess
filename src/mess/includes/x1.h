/*****************************************************************************
 *
 * includes/x1.h
 *
 ****************************************************************************/

#ifndef X1_H_
#define X1_H_

#include "cpu/z80/z80daisy.h"

extern UINT8 x1_key_irq_flag;
extern UINT8 x1_key_irq_vector;

// ======================>  x1_keyboard_device_config

class x1_keyboard_device_config :	public device_config,
								public device_config_z80daisy_interface
{
	friend class x1_keyboard_device;

	// construction/destruction
	x1_keyboard_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);

public:
	// allocators
	static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
	virtual device_t *alloc_device(running_machine &machine) const;

	// basic information getters
	virtual const char *name() const { return "X1 Keyboard"; }
};



// ======================> x1_keyboard_device

class x1_keyboard_device :	public device_t,
						public device_z80daisy_interface
{
	friend class x1_keyboard_device_config;

	// construction/destruction
	x1_keyboard_device(running_machine &_machine, const x1_keyboard_device_config &_config);

private:
	virtual void device_start();
	// z80daisy_interface overrides
	virtual int z80daisy_irq_state();
	virtual int z80daisy_irq_ack();
	virtual void z80daisy_irq_reti();

	// internal state
	const x1_keyboard_device_config &m_config;
};

extern const device_type X1_KEYBOARD;

#endif /* X1_H_ */