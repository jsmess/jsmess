/**********************************************************************

    Commodore VIC-20 Standard 8K/16K ROM Cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __VIC20_STD__
#define __VIC20_STD__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "machine/vic20exp.h"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> vic20_standard_cartridge_device

class vic20_standard_cartridge_device :  public device_t,
										 public device_vic20_expansion_card_interface
{
public:
    // construction/destruction
    vic20_standard_cartridge_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

protected:
    // device-level overrides
    virtual void device_start();
	virtual void device_config_complete() { m_shortname = "vic20_standard"; }

	// device_vic20_expansion_card_interface overrides
	virtual UINT8 vic20_blk1_r(address_space &space, offs_t offset);
	virtual UINT8 vic20_blk2_r(address_space &space, offs_t offset);
	virtual UINT8 vic20_blk3_r(address_space &space, offs_t offset);
	virtual UINT8 vic20_blk5_r(address_space &space, offs_t offset);
};


// device type definition
extern const device_type VIC20_STD;



#endif
