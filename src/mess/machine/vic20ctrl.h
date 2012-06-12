/**********************************************************************

    Commodore VIC-20/64 Control Port emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************


**********************************************************************/

#pragma once

#ifndef __VIC20_CONTROL_PORT__
#define __VIC20_CONTROL_PORT__

#include "emu.h"



//**************************************************************************
//  CONSTANTS
//**************************************************************************

#define VIC20_CONTROL_PORT_1_TAG		"joy1"
#define VIC20_CONTROL_PORT_2_TAG		"joy2"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_VIC20_CONTROL_PORT_ADD(_tag, _slot_intf, _def_slot, _def_inp) \
    MCFG_DEVICE_ADD(_tag, VIC20_CONTROL_PORT, 0) \
	MCFG_DEVICE_SLOT_INTERFACE(_slot_intf, _def_slot, _def_inp, false)



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> vic20_control_port_device

class device_vic20_control_port_interface;

class vic20_control_port_device : public device_t,
						     	  public device_slot_interface
{
public:
	// construction/destruction
	vic20_control_port_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	virtual ~vic20_control_port_device();

	// computer interface
	DECLARE_READ8_MEMBER( joy_r );
	DECLARE_READ8_MEMBER( pot_x_r );
	DECLARE_READ8_MEMBER( pot_y_r );

protected:
	// device-level overrides
	virtual void device_start();

	device_vic20_control_port_interface *m_device;
};


// ======================> device_vic20_control_port_interface

// class representing interface-specific live vic20_expansion card
class device_vic20_control_port_interface : public device_slot_card_interface
{
public:
	// construction/destruction
	device_vic20_control_port_interface(const machine_config &mconfig, device_t &device);
	virtual ~device_vic20_control_port_interface();

	virtual UINT8 vic20_joy_r() { return 0xff; };
	virtual UINT8 vic20_pot_x_r() { return 0xff; };
	virtual UINT8 vic20_pot_y_r() { return 0xff; };

protected:
	vic20_control_port_device *m_port;
};


// device type definition
extern const device_type VIC20_CONTROL_PORT;



#endif
