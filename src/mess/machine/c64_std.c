/**********************************************************************

    Commodore 64 Standard 8K/16K cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "c64_std.h"



//**************************************************************************
//  MACROS/CONSTANTS
//**************************************************************************

#define CARTSLOT_TAG	"cart"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type C64_STD = &device_creator<c64_standard_cartridge_device>;


//-------------------------------------------------
//  ROM( c64_std )
//-------------------------------------------------

ROM_START( c64_std )
	ROM_REGION( 0x4000, CARTSLOT_TAG, ROMREGION_ERASE00 )
	ROM_CART_LOAD( CARTSLOT_TAG, 0x0000, 0x4000, ROM_OPTIONAL )
ROM_END


//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *c64_standard_cartridge_device::device_rom_region() const
{
	return ROM_NAME( c64_std );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  c64_standard_cartridge_device - constructor
//-------------------------------------------------

c64_standard_cartridge_device::c64_standard_cartridge_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, C64_STD, "Standard cartridge", tag, owner, clock),
	device_c64_expansion_card_interface(mconfig, *this),
	m_game(1),
	m_exrom(0)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void c64_standard_cartridge_device::device_start()
{
	m_slot = dynamic_cast<c64_expansion_slot_device *>(owner());

	m_rom = subregion(CARTSLOT_TAG)->base();
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void c64_standard_cartridge_device::device_reset()
{
}


//-------------------------------------------------
//  c64_cd_r - cartridge data read
//-------------------------------------------------

UINT8 c64_standard_cartridge_device::c64_cd_r(offs_t offset, int roml, int romh, int io1, int io2)
{
	UINT8 data = 0;

	if (offset >= 0x8000 && offset < 0xc000)
	{
		data = m_rom[offset & 0x3fff];
	}

	return data;
}
