/*****************************************************************************
 *
 * machine/ser_mouse.h
 *
 ****************************************************************************/

#ifndef SER_MOUSE_H_
#define SER_MOUSE_H_

#include "emu.h"
#include "machine/serial.h"

class serial_mouse_device :
		public device_t,
		public device_rs232_port_interface
{
public:
	serial_mouse_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	virtual ioport_constructor device_input_ports() const;

	virtual void dtr_w(UINT8 state) { m_dtr = state; check_state(); m_old_dtr = state; }
	virtual void rts_w(UINT8 state) { m_rts = state; check_state(); m_old_rts = state; }

protected:
	virtual void device_start();
	virtual void device_reset();
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);
private:
	typedef enum
	{
		TYPE_MICROSOFT_MOUSE,
		TYPE_MOUSE_SYSTEMS
	} PC_MOUSE_PROTOCOL;

	void queue_data(int data);
	void check_state();

	PC_MOUSE_PROTOCOL m_protocol;
	UINT8 m_old_dtr, m_old_rts;

	UINT8 m_queue[256];
	UINT8 m_head, m_tail, m_mb;

	emu_timer *m_timer;
	rs232_port_device *m_owner;
};

extern const device_type SERIAL_MOUSE;

#endif /* SER_MOUSE_H_ */
