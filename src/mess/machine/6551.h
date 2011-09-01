/*********************************************************************

    6551.h

    MOS Technology 6551 Asynchronous Communications Interface Adapter

*********************************************************************/

#ifndef __6551_H__
#define __6551_H__

#include "machine/serial.h"

//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_ACIA6551_ADD(_tag)	\
	MCFG_DEVICE_ADD((_tag), ACIA6551, 0)


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

// ======================> acia6551_device

class acia6551_device :  public device_t
{
public:
    // construction/destruction
    acia6551_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	/* read data register */
	DECLARE_READ8_MEMBER(data_r);
	
	/* write data register */
	DECLARE_WRITE8_MEMBER(data_w);
	
	/* connecting to serial output */
	void connect_to_serial_device(device_t *image);
	
	void receive_character(UINT8 ch);
	
	void timer_callback();
	
	serial_connection *get_connection() { return &m_connection; }
protected:
    // device-level overrides
    virtual void device_start();
	
	void refresh_ints();
	void update_data_form();
	
private:
	UINT8 m_transmit_data_register;
	UINT8 m_receive_data_register;
	UINT8 m_status_register;
	UINT8 m_command_register;
	UINT8 m_control_register;

	/* internal baud rate timer */
	emu_timer	*m_timer;

	data_form m_form;

	/* receive register */
	serial_receive_register	m_receive_reg;
	/* transmit register */
	serial_transmit_register m_transmit_reg;

	serial_connection m_connection;
};

// device type definition
extern const device_type ACIA6551;

#endif /* __6551_H__ */
