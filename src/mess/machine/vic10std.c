/**********************************************************************

    Commodore VIC-10 Standard 8K/16K ROM Cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "vic10std.h"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type VIC10_STD = &device_creator<vic10_standard_cartridge_device>;



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  vic10_standard_cartridge_device - constructor
//-------------------------------------------------

vic10_standard_cartridge_device::vic10_standard_cartridge_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, VIC10_STD, "Standard Cartridge", tag, owner, clock),
	  device_vic10_expansion_card_interface(mconfig, *this),
	  m_exram(NULL),
	  m_lorom(NULL),
	  m_uprom(NULL)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void vic10_standard_cartridge_device::device_start()
{
}


//-------------------------------------------------
//  vic10_exram_pointer - get expanded RAM pointer
//-------------------------------------------------

UINT8* vic10_standard_cartridge_device::vic10_exram_pointer()
{
	if (m_exram == NULL)
	{
		m_exram = auto_alloc_array(machine(), UINT8, 0x800);
	}

	return m_exram;
}


//-------------------------------------------------
//  vic10_exram_r - expanded RAM read
//-------------------------------------------------

UINT8 vic10_standard_cartridge_device::vic10_exram_r(address_space &space, offs_t offset)
{
	UINT8 data = 0;

	if (m_exram != NULL)
	{
		data = m_exram[offset & 0x7ff];
	}

	return data;
}


//-------------------------------------------------
//  vic10_exram_w - expanded RAM write
//-------------------------------------------------

void vic10_standard_cartridge_device::vic10_exram_w(address_space &space, offs_t offset, UINT8 data)
{
	if (m_exram != NULL)
	{
		m_exram[offset & 0x7ff] = data;
	}
}


//-------------------------------------------------
//  vic10_lorom_pointer - get lower ROM pointer
//-------------------------------------------------

UINT8* vic10_standard_cartridge_device::vic10_lorom_pointer()
{
	if (m_lorom == NULL)
	{
		m_lorom = auto_alloc_array(machine(), UINT8, 0x2000);
	}

	return m_lorom;
}


//-------------------------------------------------
//  vic10_lorom_r - lower ROM read
//-------------------------------------------------

UINT8 vic10_standard_cartridge_device::vic10_lorom_r(address_space &space, offs_t offset)
{
	UINT8 data = 0;

	if (m_lorom != NULL)
	{
		data = m_lorom[offset & 0x1fff];
	}

	return data;
}


//-------------------------------------------------
//  vic10_uprom_pointer - get upper ROM pointer
//-------------------------------------------------

UINT8* vic10_standard_cartridge_device::vic10_uprom_pointer()
{
	if (m_uprom == NULL)
	{
		m_uprom = auto_alloc_array(machine(), UINT8, 0x2000);
	}

	return m_uprom;
}


//-------------------------------------------------
//  vic10_uprom_r - upper ROM read
//-------------------------------------------------

UINT8 vic10_standard_cartridge_device::vic10_uprom_r(address_space &space, offs_t offset)
{
	UINT8 data = 0;

	if (m_uprom != NULL)
	{
		data = m_uprom[offset & 0x1fff];
	}

	return data;
}
