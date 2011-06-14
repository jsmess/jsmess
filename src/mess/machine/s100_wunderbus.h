/**********************************************************************

	Morrow Designs Wunderbus I/O card emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __S100_WUNDERBUS__
#define __S100_WUNDERBUS__

#include "emu.h"
#include "machine/ins8250.h"
#include "machine/pic8259.h"
#include "machine/s100.h"
#include "machine/upd1990a.h"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> s100_wunderbus_device

class s100_wunderbus_device :
		public device_t,
		public device_s100_card_interface,
		public device_slot_card_interface
{
public:
	DECLARE_READ8_MEMBER( read );
	DECLARE_WRITE8_MEMBER( write );

	// construction/destruction
	s100_wunderbus_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual machine_config_constructor device_mconfig_additions() const;
	virtual ioport_constructor device_input_ports() const;

	// not really public
	DECLARE_WRITE_LINE_MEMBER( pic_int_w );
	DECLARE_WRITE_LINE_MEMBER( rtc_tp_w );
	required_device<device_t> m_pic;
	required_device<device_t> m_ace1;
	required_device<device_t> m_ace2;
	required_device<device_t> m_ace3;
	required_device<upd1990a_device> m_rtc;

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();

	// device_s100_card_interface overrides
	virtual void vi0_w(int state);
	virtual void vi1_w(int state);
	virtual void vi2_w(int state);

private:
	// internal state
	UINT8 m_wb_group;
	int m_rtc_tp;
};


// device type definition
extern const device_type S100_WUNDERBUS;


#endif
