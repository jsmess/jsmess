/**********************************************************************

    Commodore VIC-1111 16K RAM Expansion Cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __VIC1111__
#define __VIC1111__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "machine/vic20exp.h"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> vic1111_device

class vic1111_device :  public device_t,
						public device_vic20_expansion_card_interface
{
public:
    // construction/destruction
    vic1111_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

protected:
    // device-level overrides
    virtual void device_start();

	// device_vic20_expansion_card_interface overrides
	virtual UINT8 vic20_blk1_r(address_space &space, offs_t offset);
	virtual void vic20_blk1_w(address_space &space, offs_t offset, UINT8 data);
	virtual UINT8 vic20_blk2_r(address_space &space, offs_t offset);
	virtual void vic20_blk2_w(address_space &space, offs_t offset, UINT8 data);
};


// device type definition
extern const device_type VIC1111;



#endif
