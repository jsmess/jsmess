/**********************************************************************

    Western Digital WDXT-GEN ISA XT MFM Hard Disk Controller

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************

	Emulated here is the variant supplied with Amstrad PC1512/1640,
	which has a custom BIOS and is coupled with a Tandom TM262 HDD.
	
	chdman -createblankhd tandon_tm262.chd 615 4 17 512

**********************************************************************/

#pragma once

#ifndef __WDXT_GEN__
#define __WDXT_GEN__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "machine/isa.h"
#include "imagedev/harddriv.h"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> wdxt_gen_device

class wdxt_gen_device :	public device_t,
						public device_isa8_card_interface
{
public:
	// construction/destruction
	wdxt_gen_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual machine_config_constructor device_mconfig_additions() const;
	virtual const rom_entry *device_rom_region() const;

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();
	virtual void device_config_complete() { m_shortname = "wdxt_gen"; }
};


// device type definition
extern const device_type WDXT_GEN;

#endif
