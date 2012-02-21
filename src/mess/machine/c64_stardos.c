/**********************************************************************

    StarPoint Software StarDOS cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "c64_stardos.h"



//**************************************************************************
//  MACROS/CONSTANTS
//**************************************************************************

#define IO1_FULL_CHARGE		27
#define IO2_FULL_CHARGE		42



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type C64_STARDOS = &device_creator<c64_stardos_cartridge_device>;


//-------------------------------------------------
//  INPUT_PORTS( c64_stardos )
//-------------------------------------------------

INPUT_CHANGED( c64_stardos_cartridge_device::reset )
{
	// TODO
}

static INPUT_PORTS_START( c64_stardos )
	PORT_START("RESET")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SPECIAL ) PORT_NAME("Reset") PORT_CHANGED(c64_stardos_cartridge_device::reset, 0)
INPUT_PORTS_END


//-------------------------------------------------
//  input_ports - device-specific input ports
//-------------------------------------------------

ioport_constructor c64_stardos_cartridge_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( c64_stardos );
}



//**************************************************************************
//  INLINE HELPERS
//**************************************************************************

//-------------------------------------------------
//  charge_io1_capacitor -
//-------------------------------------------------

inline void c64_stardos_cartridge_device::charge_io1_capacitor()
{
	m_io1_charge++;

	if (m_io1_charge >= IO1_FULL_CHARGE)
	{
		m_exrom = 0;
		m_io1_charge = 0;
	}
}


//-------------------------------------------------
//  charge_io2_capacitor -
//-------------------------------------------------

void c64_stardos_cartridge_device::charge_io2_capacitor()
{
	m_io2_charge++;

	if (m_io2_charge >= IO2_FULL_CHARGE)
	{
		m_exrom = 1;
		m_io2_charge = 0;
	}
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  c64_stardos_cartridge_device - constructor
//-------------------------------------------------

c64_stardos_cartridge_device::c64_stardos_cartridge_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, C64_STARDOS, "C64 StarDOS cartridge", tag, owner, clock),
	device_c64_expansion_card_interface(mconfig, *this),
	m_io1_charge(0),
	m_io2_charge(0)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void c64_stardos_cartridge_device::device_start()
{
	// state saving
	save_item(NAME(m_io1_charge));
	save_item(NAME(m_io2_charge));
}


//-------------------------------------------------
//  c64_cd_r - cartridge data read
//-------------------------------------------------

UINT8 c64_stardos_cartridge_device::c64_cd_r(address_space &space, offs_t offset, int roml, int romh, int io1, int io2)
{
	UINT8 data = 0;

	if (!roml || !romh)
	{
		// TODO BITSWAP8(7,6,5,4,3,1,2,0) ?
		data = m_roml[offset & 0x3fff];
	}
	else if (!io1)
	{
		charge_io1_capacitor();
	}
	else if (!io2)
	{
		charge_io2_capacitor();
	}

	return data;
}


//-------------------------------------------------
//  c64_cd_w - cartridge data write
//-------------------------------------------------

void c64_stardos_cartridge_device::c64_cd_w(address_space &space, offs_t offset, UINT8 data, int roml, int romh, int io1, int io2)
{
	if (!io1)
	{
		charge_io1_capacitor();
	}
	else if (!io2)
	{
		charge_io2_capacitor();
	}
}


//-------------------------------------------------
//  c64_game_r - GAME read
//-------------------------------------------------

int c64_stardos_cartridge_device::c64_game_r(offs_t offset, int ba, int rw, int hiram)
{
	return !(ba & rw & ((offset & 0xe000) == 0xe000) & hiram);
}
