/**********************************************************************

    Commodore 64 standard 8K/16K cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "c64crtstd.h"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type C64_CARTRIDGE_STANDARD = &device_creator<c64_cartridge_standard_device>;


//-------------------------------------------------
//  static_set_config - configuration helper
//-------------------------------------------------

void c64_cartridge_standard_device::static_set_config(device_t &device, const UINT8 *roml, size_t roml_size, const UINT8 *romh, size_t romh_size)
{
	c64_cartridge_standard_device &cartridge = downcast<c64_cartridge_standard_device &>(device);

	cartridge.m_roml = roml;
	cartridge.m_roml_mask = roml_size - 1;
	cartridge.m_romh = romh;
	cartridge.m_romh_mask = romh_size - 1;
}


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  c64_cartridge_standard_device - constructor
//-------------------------------------------------

c64_cartridge_standard_device::c64_cartridge_standard_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, C64_CARTRIDGE_STANDARD, "Standard 8K/16K cartridge", tag, owner, clock),
	  device_c64_expansion_port_interface(mconfig, *this),
	  m_roml(NULL),
	  m_romh(NULL),
	  m_roml_mask(0x1fff),
	  m_romh_mask(0x1fff)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void c64_cartridge_standard_device::device_start()
{
}
