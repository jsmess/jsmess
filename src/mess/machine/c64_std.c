/**********************************************************************

    Commodore 64 Standard 8K/16K cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "c64_std.h"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type C64_STD = &device_creator<c64_standard_cartridge_device>;



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  c64_standard_cartridge_device - constructor
//-------------------------------------------------

c64_standard_cartridge_device::c64_standard_cartridge_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, C64_STD, "Standard cartridge", tag, owner, clock),
	device_c64_expansion_card_interface(mconfig, *this),
	m_roml(NULL),
	m_romh(NULL)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void c64_standard_cartridge_device::device_start()
{
}


//-------------------------------------------------
//  c64_cd_r - cartridge data read
//-------------------------------------------------

UINT8 c64_standard_cartridge_device::c64_cd_r(offs_t offset, int roml, int romh, int io1, int io2)
{
	UINT8 data = 0;

	if (!roml && m_roml)
	{
		data = m_roml[offset & 0x1fff];
	}
	else if (!romh && m_romh)
	{
		data = m_romh[offset & 0x1fff];
	}

	return data;
}


//-------------------------------------------------
//  c64_roml_pointer - get low ROM pointer
//-------------------------------------------------

UINT8* c64_standard_cartridge_device::c64_roml_pointer(size_t size)
{
	if (m_roml == NULL)
	{
		m_roml = auto_alloc_array(machine(), UINT8, size);
	}

	return m_roml;
}


//-------------------------------------------------
//  c64_romh_pointer - get high ROM pointer
//-------------------------------------------------

UINT8* c64_standard_cartridge_device::c64_romh_pointer(size_t size)
{
	if (m_romh == NULL)
	{
		m_romh = auto_alloc_array(machine(), UINT8, size);
	}

	return m_romh;
}
