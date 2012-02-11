#ifndef __SERIAL_H__
#define __SERIAL_H__

#include "emu.h"

#define MCFG_SERIAL_PORT_ADD(_tag, _intf, _slot_intf, _def_slot, _def_inp) \
	MCFG_DEVICE_ADD(_tag, SERIAL_PORT, 0) \
	MCFG_DEVICE_CONFIG(_intf) \
	MCFG_DEVICE_SLOT_INTERFACE(_slot_intf, _def_slot, _def_inp)

#define MCFG_RS232_PORT_ADD(_tag, _intf, _slot_intf, _def_slot, _def_inp) \
	MCFG_DEVICE_ADD(_tag, RS232_PORT, 0) \
	MCFG_DEVICE_CONFIG(_intf) \
	MCFG_DEVICE_SLOT_INTERFACE(_slot_intf, _def_slot, _def_inp)

struct serial_port_interface
{
	devcb_write8		m_out_rx8_cb;
	devcb_write_line	m_out_rx_cb;
};

class device_serial_port_interface : public device_slot_card_interface
{
public:
	device_serial_port_interface(const machine_config &mconfig, device_t &device);
	virtual ~device_serial_port_interface();

	virtual void tx8(UINT8 data) { m_tdata = data; }
	virtual UINT8 rx8() { return m_rdata; }
	virtual void tx(UINT8 state) { m_tbit = state; }
	virtual int rx() { return m_rbit; }
protected:
	UINT8 m_rdata;
	UINT8 m_rbit;
	UINT8 m_tdata;
	UINT8 m_tbit;
};

class serial_port_device : public device_t,
							public serial_port_interface,
							public device_slot_interface
{
public:
	serial_port_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	virtual ~serial_port_device();

	DECLARE_WRITE8_MEMBER( tx8 ) { if(m_dev) m_dev->tx8(data); }
	DECLARE_READ8_MEMBER( rx8 )  { return (m_dev) ? m_dev->rx8() : 0; }
	
	DECLARE_WRITE_LINE_MEMBER( tx ) { if(m_dev) m_dev->tx(state); }
	DECLARE_READ_LINE_MEMBER( rx )  { return (m_dev) ? m_dev->rx() : 0; }
	
	void out_rx8(UINT8 data) { m_out_rx8_func(0, data); }
	void out_rx(UINT8 param)  { m_out_rx_func(param); }
protected:
	virtual void device_start();
	virtual void device_config_complete();

private:
	devcb_resolved_write8 m_out_rx8_func;
	devcb_resolved_write_line m_out_rx_func;

	device_serial_port_interface *m_dev;
};

extern const device_type SERIAL_PORT;

struct rs232_port_interface
{
	devcb_write8		m_out_rx8_cb;
	devcb_write_line	m_out_rx_cb;
	devcb_write_line	m_out_dcd_cb;
	devcb_write_line	m_out_dsr_cb;
	devcb_write_line	m_out_ri_cb;
	devcb_write_line	m_out_cts_cb;
};

class device_rs232_port_interface : public device_serial_port_interface
{
public:
	device_rs232_port_interface(const machine_config &mconfig, device_t &device);
	virtual ~device_rs232_port_interface();

	virtual void dtr_w(UINT8 state) { m_dtr = state; }
	virtual void rts_w(UINT8 state) { m_rts = state; }

	virtual UINT8 dcd_r() { return m_dcd; }
	virtual UINT8 dsr_r() { return m_dsr; }
	virtual UINT8 ri_r()  { return m_ri; }
	virtual UINT8 cts_r() { return m_cts; }
protected:
	UINT8 m_dtr;
	UINT8 m_rts;
	UINT8 m_dcd;
	UINT8 m_dsr;
	UINT8 m_ri;
	UINT8 m_cts;
};

class rs232_port_device : public serial_port_device,
							public rs232_port_interface
{
public:
	rs232_port_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	virtual ~rs232_port_device();

	DECLARE_WRITE_LINE_MEMBER( dtr_w ) { if (m_dev) m_dev->dtr_w(state); }
	DECLARE_WRITE_LINE_MEMBER( rts_w ) { if (m_dev) m_dev->rts_w(state); }

	DECLARE_READ_LINE_MEMBER( dcd_r ) { return (m_dev) ? m_dev->dcd_r() : 0; }
	DECLARE_READ_LINE_MEMBER( dsr_r ) { return (m_dev) ? m_dev->dsr_r() : 0; }
	DECLARE_READ_LINE_MEMBER( ri_r )  { return (m_dev) ? m_dev->ri_r() : 0; }
	DECLARE_READ_LINE_MEMBER( cts_r ) { return (m_dev) ? m_dev->cts_r() : 0; }

	void out_dcd(UINT8 param) { m_out_dcd_func(param); }
	void out_dsr(UINT8 param) { m_out_dsr_func(param); }
	void out_ri(UINT8 param)  { m_out_ri_func(param); }
	void out_cts(UINT8 param) { m_out_cts_func(param); }

protected:
	virtual void device_start();
	virtual void device_config_complete();

private:
	device_rs232_port_interface *m_dev;

	devcb_resolved_write_line m_out_dcd_func;
	devcb_resolved_write_line m_out_dsr_func;
	devcb_resolved_write_line m_out_ri_func;
	devcb_resolved_write_line m_out_cts_func;
};

extern const device_type RS232_PORT;

#endif
