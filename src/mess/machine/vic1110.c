/**********************************************************************

    Commodore VIC-1110 8K RAM Expansion Cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "vic1110.h"


//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

enum
{
	BLK1 = 0x07,
	BLK2 = 0x0b,
	BLK3 = 0x0d,
	BLK5 = 0x0e
};



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type VIC1110 = &device_creator<vic1110_device>;



//-------------------------------------------------
//  INPUT_PORTS( vic1110 )
//-------------------------------------------------

INPUT_PORTS_START( vic1110 )
	PORT_START("SW")
	PORT_DIPNAME( 0x0f, BLK1, "Memory Location" ) PORT_DIPLOCATION("SW:1,2,3,4")
	PORT_DIPSETTING(    BLK1, "$2000-$3FFF" )
	PORT_DIPSETTING(    BLK2, "$4000-$5FFF" )
	PORT_DIPSETTING(    BLK3, "$6000-$7FFF" )
	PORT_DIPSETTING(    BLK5, "$A000-B3FFF" )
INPUT_PORTS_END

	
//-------------------------------------------------
//  input_ports - device-specific input ports
//-------------------------------------------------

ioport_constructor vic1110_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( vic1110 );
}




//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  vic1110_device - constructor
//-------------------------------------------------

vic1110_device::vic1110_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, VIC1110, "VIC1110", tag, owner, clock),
	  device_vic20_expansion_card_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void vic1110_device::device_start()
{
	// allocate memory
	m_ram = auto_alloc_array(machine(), UINT8, 0x2000);
}


//-------------------------------------------------
//  vic20_blk1_r - block 1 read
//-------------------------------------------------

UINT8 vic1110_device::vic20_blk1_r(address_space &space, offs_t offset)
{
	UINT8 data = 0;
	
	if (input_port_read(*this, "SW") == BLK1)
	{
		data = m_ram[offset & 0x1fff];
	}
	
	return data;
}


//-------------------------------------------------
//  vic20_blk1_w - block 1 write
//-------------------------------------------------

void vic1110_device::vic20_blk1_w(address_space &space, offs_t offset, UINT8 data)
{
	if (input_port_read(*this, "SW") == BLK1)
	{
		m_ram[offset & 0x1fff] = data;
	}
}


//-------------------------------------------------
//  vic20_blk2_r - block 2 read
//-------------------------------------------------

UINT8 vic1110_device::vic20_blk2_r(address_space &space, offs_t offset)
{
	UINT8 data = 0;
	
	if (input_port_read(*this, "SW") == BLK2)
	{
		data = m_ram[offset & 0x1fff];
	}
	
	return data;
}


//-------------------------------------------------
//  vic20_blk2_w - block 2 write
//-------------------------------------------------

void vic1110_device::vic20_blk2_w(address_space &space, offs_t offset, UINT8 data)
{
	if (input_port_read(*this, "SW") == BLK2)
	{
		m_ram[offset & 0x1fff] = data;
	}
}


//-------------------------------------------------
//  vic20_blk3_r - block 3 read
//-------------------------------------------------

UINT8 vic1110_device::vic20_blk3_r(address_space &space, offs_t offset)
{
	UINT8 data = 0;
	
	if (input_port_read(*this, "SW") == BLK3)
	{
		data = m_ram[offset & 0x1fff];
	}
	
	return data;
}


//-------------------------------------------------
//  vic20_blk3_w - block 3 write
//-------------------------------------------------

void vic1110_device::vic20_blk3_w(address_space &space, offs_t offset, UINT8 data)
{
	if (input_port_read(*this, "SW") == BLK3)
	{
		m_ram[offset & 0x1fff] = data;
	}
}


//-------------------------------------------------
//  vic20_blk5_r - block 5 read
//-------------------------------------------------

UINT8 vic1110_device::vic20_blk5_r(address_space &space, offs_t offset)
{
	UINT8 data = 0;
	
	if (input_port_read(*this, "SW") == BLK5)
	{
		data = m_ram[offset & 0x1fff];
	}
	
	return data;
}


//-------------------------------------------------
//  vic20_blk5_w - block 5 write
//-------------------------------------------------

void vic1110_device::vic20_blk5_w(address_space &space, offs_t offset, UINT8 data)
{
	if (input_port_read(*this, "SW") == BLK5)
	{
		m_ram[offset & 0x1fff] = data;
	}
}
