/**********************************************************************

    Commodore VIC-20/64 Control Port emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "machine/vic20ctrl.h"



//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type VIC20_CONTROL_PORT = &device_creator<vic20_control_port_device>;



//**************************************************************************
//  CARD INTERFACE
//**************************************************************************

//-------------------------------------------------
//  device_vic20_control_port_interface - constructor
//-------------------------------------------------

device_vic20_control_port_interface::device_vic20_control_port_interface(const machine_config &mconfig, device_t &device)
	: device_slot_card_interface(mconfig,device)
{
	m_port = dynamic_cast<vic20_control_port_device *>(device.owner());
}


//-------------------------------------------------
//  ~device_vic20_control_port_interface - destructor
//-------------------------------------------------

device_vic20_control_port_interface::~device_vic20_control_port_interface()
{
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  vic20_control_port_device - constructor
//-------------------------------------------------

vic20_control_port_device::vic20_control_port_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, VIC20_CONTROL_PORT, "VIC20 control port", tag, owner, clock),
		device_slot_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  vic20_control_port_device - destructor
//-------------------------------------------------

vic20_control_port_device::~vic20_control_port_device()
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void vic20_control_port_device::device_start()
{
	m_device = dynamic_cast<device_vic20_control_port_interface *>(get_card_device());
}


READ8_MEMBER( vic20_control_port_device::joy_r ) { UINT8 data = 0xff; if (m_device != NULL) data = m_device->vic20_joy_r(); return data; }
READ8_MEMBER( vic20_control_port_device::pot_x_r ) { UINT8 data = 0xff; if (m_device != NULL) data = m_device->vic20_pot_x_r(); return data; }
READ8_MEMBER( vic20_control_port_device::pot_y_r ) { UINT8 data = 0xff; if (m_device != NULL) data = m_device->vic20_pot_y_r(); return data; }
