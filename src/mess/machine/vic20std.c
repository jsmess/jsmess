/**********************************************************************

    Commodore VIC-20 Standard 8K/16K ROM Cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "vic20std.h"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type VIC20_STD = &device_creator<vic20_standard_cartridge_device>;



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  vic20_standard_cartridge_device - constructor
//-------------------------------------------------

vic20_standard_cartridge_device::vic20_standard_cartridge_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, VIC20_STD, "VIC-20 Standard Cartridge", tag, owner, clock),
	  device_vic20_expansion_card_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void vic20_standard_cartridge_device::device_start()
{
}


//-------------------------------------------------
//  vic20_blk1_r - block 1 read
//-------------------------------------------------

UINT8 vic20_standard_cartridge_device::vic20_blk1_r(address_space &space, offs_t offset)
{
	UINT8 data = 0;

	if (m_blk1 != NULL)
	{
		data = m_blk1[offset & 0x1fff];
	}

	return data;
}


//-------------------------------------------------
//  vic20_blk2_r - block 2 read
//-------------------------------------------------

UINT8 vic20_standard_cartridge_device::vic20_blk2_r(address_space &space, offs_t offset)
{
	UINT8 data = 0;

	if (m_blk2 != NULL)
	{
		data = m_blk2[offset & 0x1fff];
	}

	return data;
}


//-------------------------------------------------
//  vic20_blk3_r - block 3 read
//-------------------------------------------------

UINT8 vic20_standard_cartridge_device::vic20_blk3_r(address_space &space, offs_t offset)
{
	UINT8 data = 0;

	if (m_blk3 != NULL)
	{
		data = m_blk3[offset & 0x1fff];
	}

	return data;
}


//-------------------------------------------------
//  vic20_blk5_r - block 5 read
//-------------------------------------------------

UINT8 vic20_standard_cartridge_device::vic20_blk5_r(address_space &space, offs_t offset)
{
	UINT8 data = 0;

	if (m_blk5 != NULL)
	{
		data = m_blk5[offset & 0x1fff];
	}

	return data;
}
