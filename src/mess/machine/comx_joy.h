/**********************************************************************

    COMX-35 F&M Joycard emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __COMX_JOY__
#define __COMX_JOY__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "machine/comxexp.h"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> comx_joy_device

class comx_joy_device : public device_t,
					    public device_comx_expansion_card_interface
{
public:
	// construction/destruction
	comx_joy_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual ioport_constructor device_input_ports() const;

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();

	// device_comx_expansion_card_interface overrides
	virtual UINT8 comx_io_r(offs_t offset);

private:
	// internal state
	comx_expansion_slot_device *m_slot;
};


// device type definition
extern const device_type COMX_JOY;


#endif
