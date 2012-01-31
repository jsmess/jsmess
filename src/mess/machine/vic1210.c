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
const device_type VIC1211A = &device_creator<vic1211a_device>;


//-------------------------------------------------
//  ROM( vic1211a )
//-------------------------------------------------

ROM_START( vic1211a )
	ROM_REGION( 0x2000, "blk5", 0 )
	ROM_LOAD( "325323-01", 0x0000, 0x1000, CRC(c576ba40) SHA1(aa92ac00a945d8496d7f2bf1c869736ee840d7c5) )
ROM_END


//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *vic1211a_device::device_rom_region() const
{
	return ROM_NAME( vic1211a );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  vic1210_device - constructor
//-------------------------------------------------

vic1210_device::vic1210_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, type, name, tag, owner, clock),
	  device_vic20_expansion_card_interface(mconfig, *this)
{
}


vic1210_device::vic1210_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, VIC1210, "VIC1210", tag, owner, clock),
	  device_vic20_expansion_card_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  vic1211a_device - constructor
//-------------------------------------------------

vic1211a_device::vic1211a_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : vic1210_device(mconfig, VIC1211A, "VIC1211A", tag, owner, clock)
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
//  device_start - device-specific startup
//-------------------------------------------------

void vic1211a_device::device_start()
{
	// allocate memory
	m_ram = auto_alloc_array(machine(), UINT8, 0xc00);
	m_rom = subregion("blk5")->base();
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

UINT8 vic1211a_device::vic20_blk5_r(address_space &space, offs_t offset)
{
	return m_rom[offset & 0xfff];
}
