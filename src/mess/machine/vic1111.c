/**********************************************************************

    Commodore VIC-1111 16K RAM Expansion Cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "vic1111.h"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type VIC1111 = &device_creator<vic1111_device>;



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  vic1111_device - constructor
//-------------------------------------------------

vic1111_device::vic1111_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, VIC1111, "VIC1111", tag, owner, clock),
	  device_vic20_expansion_card_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void vic1111_device::device_start()
{
	// allocate memory
	m_ram = auto_alloc_array(machine(), UINT8, 0x4000);
}


//-------------------------------------------------
//  vic20_blk1_r - block 1 read
//-------------------------------------------------

UINT8 vic1111_device::vic20_blk1_r(address_space &space, offs_t offset)
{
	return m_ram[offset & 0x1fff];
}


//-------------------------------------------------
//  vic20_blk1_w - block 1 write
//-------------------------------------------------

void vic1111_device::vic20_blk1_w(address_space &space, offs_t offset, UINT8 data)
{
	m_ram[offset & 0x1fff] = data;
}


//-------------------------------------------------
//  vic20_blk2_r - block 2 read
//-------------------------------------------------

UINT8 vic1111_device::vic20_blk2_r(address_space &space, offs_t offset)
{
	return m_ram[0x2000 | (offset & 0x1fff)];
}


//-------------------------------------------------
//  vic20_blk2_w - block 2 write
//-------------------------------------------------

void vic1111_device::vic20_blk2_w(address_space &space, offs_t offset, UINT8 data)
{
	m_ram[0x2000 | (offset & 0x1fff)] = data;
}
