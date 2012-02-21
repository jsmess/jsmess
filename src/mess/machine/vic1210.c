/**********************************************************************

    Commodore VIC-1210 3K RAM Expansion Cartridge emulation
    Commodore VIC-1211A Super Expander with 3K RAM Cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "vic1210.h"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type VIC1210 = &device_creator<vic1210_device>;



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  vic1210_device - constructor
//-------------------------------------------------

vic1210_device::vic1210_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, VIC1210, "VIC1210", tag, owner, clock),
	  device_vic20_expansion_card_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void vic1210_device::device_start()
{
	// allocate memory
	m_ram = auto_alloc_array(machine(), UINT8, 0xc00);
}


//-------------------------------------------------
//  vic20_ram1_r - RAM 1 read
//-------------------------------------------------

UINT8 vic1210_device::vic20_ram1_r(address_space &space, offs_t offset)
{
	return m_ram[offset & 0x3ff];
}


//-------------------------------------------------
//  vic20_ram1_w - RAM 1 write
//-------------------------------------------------

void vic1210_device::vic20_ram1_w(address_space &space, offs_t offset, UINT8 data)
{
	m_ram[offset & 0x3ff] = data;
}


//-------------------------------------------------
//  vic20_ram2_r - RAM 2 read
//-------------------------------------------------

UINT8 vic1210_device::vic20_ram2_r(address_space &space, offs_t offset)
{
	return m_ram[0x400 | (offset & 0x3ff)];
}


//-------------------------------------------------
//  vic20_ram2_w - RAM 2 write
//-------------------------------------------------

void vic1210_device::vic20_ram2_w(address_space &space, offs_t offset, UINT8 data)
{
	m_ram[0x400 | (offset & 0x3ff)] = data;
}


//-------------------------------------------------
//  vic20_ram3_r - RAM 3 read
//-------------------------------------------------

UINT8 vic1210_device::vic20_ram3_r(address_space &space, offs_t offset)
{
	return m_ram[0x800 | (offset & 0x3ff)];
}


//-------------------------------------------------
//  vic20_ram3_w - RAM 3 write
//-------------------------------------------------

void vic1210_device::vic20_ram3_w(address_space &space, offs_t offset, UINT8 data)
{
	m_ram[0x800 | (offset & 0x3ff)] = data;
}


//-------------------------------------------------
//  vic20_blk5_r - block 5 read
//-------------------------------------------------

UINT8 vic1210_device::vic20_blk5_r(address_space &space, offs_t offset)
{
	UINT8 data = 0;

	if (m_blk5)
	{
		data = m_blk5[offset & 0xfff];
	}

	return data;
}
