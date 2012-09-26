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
	osborne1_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

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


// ======================> osborne1_daisy_device

class osborne1_daisy_device :	public device_t,
						public device_z80daisy_interface
{
public:
	// construction/destruction
	osborne1_daisy_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

private:
	virtual void device_start();
	// z80daisy_interface overrides
	virtual int z80daisy_irq_state();
	virtual int z80daisy_irq_ack();
	virtual void z80daisy_irq_reti();
};

extern const device_type OSBORNE1_DAISY;

#endif /* OSBORNE1_H_ */
