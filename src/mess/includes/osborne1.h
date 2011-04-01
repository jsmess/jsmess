/*****************************************************************************
 *
 * includes/osborne1.h
 *
 ****************************************************************************/

#ifndef OSBORNE1_H_
#define OSBORNE1_H_

#include "machine/6821pia.h"

class osborne1_state : public driver_device
{
public:
	osborne1_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8	m_bank2_enabled;
	UINT8	m_bank3_enabled;
	UINT8	*m_bank4_ptr;
	UINT8	*m_empty_4K;
	/* IRQ states */
	int		m_pia_0_irq_state;
	int		m_pia_1_irq_state;
	/* video related */
	UINT8	m_new_start_x;
	UINT8	m_new_start_y;
	emu_timer	*m_video_timer;
	UINT8	*m_charrom;
	UINT8	m_charline;
	UINT8	m_start_y;
	/* bankswitch setting */
	UINT8	m_bankswitch;
	UINT8	m_in_irq_handler;
	/* beep state */
	UINT8	m_beep;
};


/*----------- defined in machine/osborne1.c -----------*/

extern const pia6821_interface osborne1_ieee_pia_config;
extern const pia6821_interface osborne1_video_pia_config;

WRITE8_HANDLER( osborne1_0000_w );
WRITE8_HANDLER( osborne1_1000_w );
READ8_HANDLER( osborne1_2000_r );
WRITE8_HANDLER( osborne1_2000_w );
WRITE8_HANDLER( osborne1_3000_w );
WRITE8_HANDLER( osborne1_videoram_w );

WRITE8_HANDLER( osborne1_bankswitch_w );

DRIVER_INIT( osborne1 );
MACHINE_RESET( osborne1 );

// ======================>  osborne1_daisy_device_config

class osborne1_daisy_device_config :	public device_config,
								public device_config_z80daisy_interface
{
	friend class osborne1_daisy_device;

	// construction/destruction
	osborne1_daisy_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);

public:
	// allocators
	static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
	virtual device_t *alloc_device(running_machine &machine) const;
};



// ======================> osborne1_daisy_device

class osborne1_daisy_device :	public device_t,
						public device_z80daisy_interface
{
	friend class osborne1_daisy_device_config;

	// construction/destruction
	osborne1_daisy_device(running_machine &_machine, const osborne1_daisy_device_config &_config);

private:
	virtual void device_start();
	// z80daisy_interface overrides
	virtual int z80daisy_irq_state();
	virtual int z80daisy_irq_ack();
	virtual void z80daisy_irq_reti();

	// internal state
	const osborne1_daisy_device_config &m_config;
};

extern const device_type OSBORNE1_DAISY;

#endif /* OSBORNE1_H_ */
