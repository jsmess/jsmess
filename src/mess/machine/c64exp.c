/**********************************************************************

    Commodore 64 Expansion Port emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "emu.h"
#include "emuopts.h"
#include "machine/c64exp.h"


//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type C64_EXPANSION_SLOT = &device_creator<c64_expansion_slot_device>;


//**************************************************************************
//  DEVICE C64_EXPANSION CARD INTERFACE
//**************************************************************************

//-------------------------------------------------
//  device_c64_expansion_card_interface - constructor
//-------------------------------------------------

device_c64_expansion_card_interface::device_c64_expansion_card_interface(const machine_config &mconfig, device_t &device)
	: device_interface(device)
{
}


//-------------------------------------------------
//  ~device_c64_expansion_card_interface - destructor
//-------------------------------------------------

device_c64_expansion_card_interface::~device_c64_expansion_card_interface()
{
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  c64_expansion_slot_device - constructor
//-------------------------------------------------

c64_expansion_slot_device::c64_expansion_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, C64_EXPANSION_SLOT, "C64 expansion port", tag, owner, clock),
		device_slot_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  c64_expansion_slot_device - destructor
//-------------------------------------------------

c64_expansion_slot_device::~c64_expansion_slot_device()
{
}


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void c64_expansion_slot_device::device_config_complete()
{
	// inherit a copy of the static data
	const c64_expansion_slot_interface *intf = reinterpret_cast<const c64_expansion_slot_interface *>(static_config());
	if (intf != NULL)
	{
		*static_cast<c64_expansion_slot_interface *>(this) = *intf;
	}

	// or initialize to defaults if none provided
	else
	{
    	memset(&m_out_irq_cb, 0, sizeof(m_out_irq_cb));
    	memset(&m_out_nmi_cb, 0, sizeof(m_out_nmi_cb));
    	memset(&m_out_dma_cb, 0, sizeof(m_out_dma_cb));
    	memset(&m_out_reset_cb, 0, sizeof(m_out_reset_cb));
    	memset(&m_out_game_cb, 0, sizeof(m_out_game_cb));
    	memset(&m_out_exrom_cb, 0, sizeof(m_out_exrom_cb));
	}
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void c64_expansion_slot_device::device_start()
{
	m_card = dynamic_cast<device_c64_expansion_card_interface *>(get_card_device());

	// resolve callbacks
	m_out_irq_func.resolve(m_out_irq_cb, *this);
	m_out_nmi_func.resolve(m_out_nmi_cb, *this);
	m_out_dma_func.resolve(m_out_dma_cb, *this);
	m_out_reset_func.resolve(m_out_reset_cb, *this);
	m_out_game_func.resolve(m_out_game_cb, *this);
	m_out_exrom_func.resolve(m_out_exrom_cb, *this);
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void c64_expansion_slot_device::device_reset()
{
}


//-------------------------------------------------
//  roml_r - low ROM read
//-------------------------------------------------

READ8_MEMBER( c64_expansion_slot_device::roml_r )
{
	UINT8 data = 0;
	
	if (m_card != NULL)
	{
		data = m_card->c64_cd_r(offset, 0, 1, 1, 1);
	}
	
	return data;
}


//-------------------------------------------------
//  romh_r - high ROM read
//-------------------------------------------------

READ8_MEMBER( c64_expansion_slot_device::romh_r )
{
	UINT8 data = 0;
	
	if (m_card != NULL)
	{
		data = m_card->c64_cd_r(offset, 1, 0, 1, 1);
	}
	
	return data;
}


//-------------------------------------------------
//  io1_r - I/O 1 read
//-------------------------------------------------

READ8_MEMBER( c64_expansion_slot_device::io1_r )
{
	UINT8 data = 0;
	
	if (m_card != NULL)
	{
		data = m_card->c64_cd_r(offset, 1, 1, 0, 1);
	}
	
	return data;
}


//-------------------------------------------------
//  io1_w - low ROM write
//-------------------------------------------------

WRITE8_MEMBER( c64_expansion_slot_device::io1_w )
{
	if (m_card != NULL)
	{
		m_card->c64_cd_w(offset, data, 1, 1, 0, 1);
	}
}


//-------------------------------------------------
//  io2_r - I/O 2 read
//-------------------------------------------------

READ8_MEMBER( c64_expansion_slot_device::io2_r )
{
	UINT8 data = 0;
	
	if (m_card != NULL)
	{
		data = m_card->c64_cd_r(offset, 1, 1, 1, 0);
	}
	
	return data;
}


//-------------------------------------------------
//  io2_w - low ROM write
//-------------------------------------------------

WRITE8_MEMBER( c64_expansion_slot_device::io2_w )
{
	if (m_card != NULL)
	{
		m_card->c64_cd_w(offset, data, 1, 1, 1, 0);
	}
}


//-------------------------------------------------
//  screen_update -
//-------------------------------------------------

bool c64_expansion_slot_device::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	bool value = false;
	
	if (m_card != NULL)
	{
		value = m_card->c64_screen_update(screen, bitmap, cliprect);
	}
	
	return value;
}


WRITE_LINE_MEMBER( c64_expansion_slot_device::irq_w ) { m_out_irq_func(state); }
WRITE_LINE_MEMBER( c64_expansion_slot_device::nmi_w ) { m_out_nmi_func(state); }
WRITE_LINE_MEMBER( c64_expansion_slot_device::dma_w ) { m_out_dma_func(state); }
WRITE_LINE_MEMBER( c64_expansion_slot_device::reset_w ) { m_out_reset_func(state); }
WRITE_LINE_MEMBER( c64_expansion_slot_device::game_w ) { m_out_game_func(state); }
WRITE_LINE_MEMBER( c64_expansion_slot_device::exrom_w ) { m_out_exrom_func(state); }
