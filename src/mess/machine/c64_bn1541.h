/**********************************************************************

	Burst Nibbler 1541 Parallel Cable emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __C64_BN1541__
#define __C64_BN1541__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "machine/c64user.h"
#include "machine/c1541.h"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> c64_bn1541_device

class c1541_device;

class c64_bn1541_device : public device_t,
						  public device_c64_user_port_interface
{
public:
	// construction/destruction
	c64_bn1541_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	void parallel_data_w(UINT8 data);
	void parallel_strobe_w(int state);

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();
	virtual void device_config_complete() { m_shortname = "c64_bn1541"; }

	// device_c64_user_port_interface overrides
	virtual UINT8 c64_pb_r(address_space &space, offs_t offset);
	virtual void c64_pb_w(address_space &space, offs_t offset, UINT8 data);
	virtual void c64_pc2_w(int level);

private:
	c1541_device *m_drive;

	UINT8 m_data;
};


// device type definition
extern const device_type C64_BN1541;


#endif
