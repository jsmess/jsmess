/**********************************************************************

    Commodore VIC-10 Standard 8K/16K ROM Cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __VIC10_STD__
#define __VIC10_STD__


#include "emu.h"
#include "machine/vic10exp.h"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> vic10_standard_cartridge_device

class vic10_standard_cartridge_device :  public device_t,
										 public device_vic10_expansion_card_interface
{
public:
    // construction/destruction
    vic10_standard_cartridge_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

protected:
    // device-level overrides
    virtual void device_start();
	virtual void device_config_complete() { m_shortname = "vic10_standard"; }

	// device_vic10_expansion_card_interface overrides
	virtual UINT8 vic10_exram_r(address_space &space, offs_t offset);
	virtual void vic10_exram_w(address_space &space, offs_t offset, UINT8 data);
	virtual UINT8 vic10_lorom_r(address_space &space, offs_t offset);
	virtual UINT8 vic10_uprom_r(address_space &space, offs_t offset);
};


// device type definition
extern const device_type VIC10_STD;



#endif
