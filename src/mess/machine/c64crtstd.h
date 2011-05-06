/**********************************************************************

    Commodore 64 standard 8K/16K cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __C64_CARTRIDGE_STANDARD__
#define __C64_CARTRIDGE_STANDARD__

#include "emu.h"
#include "c64expp.h"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_C64_CARTRIDGE_8K_ADD(_tag, _roml, _roml_size) \
	MCFG_DEVICE_ADD(_tag, C64_CARTRIDGE_STANDARD, 0) \
	c64_cartridge_standard_device::static_set_config(*device, _roml, _roml_size, NULL, 0);


#define MCFG_C64_CARTRIDGE_16K_ADD(_tag, _roml, _roml_size, _romh, _romh_size) \
	MCFG_DEVICE_ADD(_tag, C64_CARTRIDGE_STANDARD, 0) \
	c64_cartridge_standard_device::static_set_config(*device, _roml, _roml_size, _romh, _romh_size);

	

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> c64_cartridge_standard_device

class c64_cartridge_standard_device :  public device_t,
									   public device_c64_expansion_port_interface
{
public:
    // construction/destruction
    c64_cartridge_standard_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// inline configuration helpers
	static void static_set_config(device_t &device, const UINT8 *roml, size_t roml_size, const UINT8 *romh, size_t romh_size);

protected:
    // device-level overrides
    virtual void device_start();

	// device_c64_expansion_port_interface overrides
	int c64_game_r() { return 0; }
	int c64_exrom_r() { return 0; }
	UINT8 c64_roml_r(offs_t offset) { if (m_roml != NULL) return m_roml[offset & m_roml_mask]; else return 0; }
	UINT8 c64_romh_r(offs_t offset) { if (m_romh != NULL) return m_romh[offset & m_romh_mask]; else return 0; }
	
private:
	const UINT8 *m_roml;
	const UINT8 *m_romh;
	
	size_t m_roml_mask;
	size_t m_romh_mask;
};


// device type definition
extern const device_type C64_CARTRIDGE_STANDARD;



#endif
