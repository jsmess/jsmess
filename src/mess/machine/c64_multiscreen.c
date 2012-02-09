/**********************************************************************

    Multiscreen cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

	PCB Layout
	----------

					|===================|
					|           ROM6    |
					|  MC14066          |
					|                   |
	|===============|           ROM5    |
	|=|                                 |
	|=|                                 |
	|=|    RAM         BAT      ROM4    |
	|=|                                 |
	|=|                                 |
	|=|    ROM0                 ROM3    |
	|=|                                 |
	|=|                                 |
	|===============|  LS138    ROM2    |
					|  LS138            |
					|  LS174            |
					|  LS133    ROM1    |
					|===================|

	BAT   - BR2325 lithium battery
	RAM   - ? 8Kx8 RAM
	ROM0  - ? 16Kx8 EPROM
	ROM1  - ? 32Kx8 EPROM
	ROM2  - ? 32Kx8 EPROM
	ROM3  - ? 32Kx8 EPROM
	ROM4  - not populated
	ROM5  - not populated
	ROM6  - not populated

*/

/*

	TODO:
	
	- M6802 board

*/

#include "c64_multiscreen.h"



//**************************************************************************
//  MACROS/CONSTANTS
//**************************************************************************

#define BANK_RAM	0x0d



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type C64_MULTISCREEN = &device_creator<c64_multiscreen_cartridge_device>;



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  c64_multiscreen_cartridge_device - constructor
//-------------------------------------------------

c64_multiscreen_cartridge_device::c64_multiscreen_cartridge_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, C64_MULTISCREEN, "C64 Multiscreen cartridge", tag, owner, clock),
	device_c64_expansion_card_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void c64_multiscreen_cartridge_device::device_start()
{
	// state saving
	save_item(NAME(m_bank));
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void c64_multiscreen_cartridge_device::device_reset()
{
	m_bank = 0;
}


//-------------------------------------------------
//  c64_cd_r - cartridge data read
//-------------------------------------------------

UINT8 c64_multiscreen_cartridge_device::c64_cd_r(address_space &space, offs_t offset, int roml, int romh, int io1, int io2)
{
	UINT8 data = 0;
	
	if (!roml)
	{
		int bank = m_bank & 0x0f;

		if (bank == BANK_RAM)
		{
			data = m_ram[offset & 0x1fff];
		}
		else
		{
			data = m_roml[(bank << 14) | (offset & 0x3fff)];
		}
	}
	else if (!romh)
	{
		int bank = m_bank & 0x0f;
		
		if (bank == BANK_RAM)
		{
			data = m_roml[offset & 0x3fff];
		}
		else
		{
			data = m_roml[(bank << 14) | (offset & 0x3fff)];
		}
	}

	return data;
}


//-------------------------------------------------
//  c64_cd_w - cartridge data write
//-------------------------------------------------

void c64_multiscreen_cartridge_device::c64_cd_w(address_space &space, offs_t offset, UINT8 data, int roml, int romh, int io1, int io2)
{
	if (offset >= 0x8000 && offset < 0xa000)
	{
		int bank = m_bank & 0x0f;
		
		if (bank == BANK_RAM)
		{
			m_ram[offset & 0x1fff] = data;
		}
	}
	else if (!io2)
	{
		m_bank = data;
	}
}
