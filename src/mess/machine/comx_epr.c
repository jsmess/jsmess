#include "comx_epr.h"



//**************************************************************************
//  MACROS/CONSTANTS
//**************************************************************************



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type COMX_EPR = &device_creator<comx_epr_device>;


//-------------------------------------------------
//  ROM( comx_epr )
//-------------------------------------------------

ROM_START( comx_epr )
	ROM_REGION( 0x2000, "c000", 0 )
	ROM_LOAD( "f&m.eprom.board.1.1.bin", 0x0000, 0x0800, CRC(a042a31a) SHA1(13831a1350aa62a87988bfcc99c4b7db8ef1cf62) )
ROM_END


//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *comx_epr_device::device_rom_region() const
{
	return ROM_NAME( comx_epr );
}


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  comx_epr_device - constructor
//-------------------------------------------------

comx_epr_device::comx_epr_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, COMX_EPR, "COMX-35 F&M EPROM Card", tag, owner, clock),
	device_comx_expansion_card_interface(mconfig, *this),
	device_slot_card_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void comx_epr_device::device_start()
{
	m_bus = machine().device<comx_expansion_bus_device>(COMX_EXPANSION_TAG);

	m_rom = subregion("c000")->base();
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void comx_epr_device::device_reset()
{
}


//-------------------------------------------------
//  comx_mrd_r - memory read
//-------------------------------------------------

UINT8 comx_epr_device::comx_mrd_r(offs_t offset)
{
	UINT8 data = 0;
	
	if (offset >= 0xc000 && offset < 0xd000)
	{
		data = m_rom[offset & 0xfff];
	}
	
	return data;
}
