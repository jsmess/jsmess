/**********************************************************************

    Kingsoft 4-Player Adapter emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __C64_4KSA__
#define __C64_4KSA__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "machine/c64user.h"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> c64_4ksa_device

class c64_4ksa_device : public device_t,
						public device_c64_user_port_interface
{
public:
	// construction/destruction
	c64_4ksa_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual ioport_constructor device_input_ports() const;

	INPUT_CHANGED_MEMBER( fire4 );

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_config_complete() { m_shortname = "c64_4ksa"; }

	// device_c64_user_port_interface overrides
	virtual UINT8 c64_pb_r(address_space &space, offs_t offset);
	virtual int c64_pa2_r();
	virtual void c64_cnt1_w(int level);
};


// device type definition
extern const device_type C64_4KSA;


#endif
