/**********************************************************************

    Commodore VIC-20 Expansion Port emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "emu.h"
#include "emuopts.h"
#include "machine/vic20exp.h"


//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type VIC20_EXPANSION_SLOT = &device_creator<vic20_expansion_slot_device>;


//**************************************************************************
//  DEVICE VIC20_EXPANSION CARD INTERFACE
//**************************************************************************

//-------------------------------------------------
//  device_vic20_expansion_card_interface - constructor
//-------------------------------------------------

device_vic20_expansion_card_interface::device_vic20_expansion_card_interface(const machine_config &mconfig, device_t &device)
	: device_slot_card_interface(mconfig,device)
{
	m_slot = dynamic_cast<vic20_expansion_slot_device *>(device.owner());
}


//-------------------------------------------------
//  ~device_vic20_expansion_card_interface - destructor
//-------------------------------------------------

device_vic20_expansion_card_interface::~device_vic20_expansion_card_interface()
{
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  vic20_expansion_slot_device - constructor
//-------------------------------------------------

vic20_expansion_slot_device::vic20_expansion_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, VIC20_EXPANSION_SLOT, "VIC-20 expansion port", tag, owner, clock),
		device_slot_interface(mconfig, *this),
		device_image_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  vic20_expansion_slot_device - destructor
//-------------------------------------------------

vic20_expansion_slot_device::~vic20_expansion_slot_device()
{
}


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void vic20_expansion_slot_device::device_config_complete()
{
	// inherit a copy of the static data
	const vic20_expansion_slot_interface *intf = reinterpret_cast<const vic20_expansion_slot_interface *>(static_config());
	if (intf != NULL)
	{
		*static_cast<vic20_expansion_slot_interface *>(this) = *intf;
	}

	// or initialize to defaults if none provided
	else
	{
    	memset(&m_out_irq_cb, 0, sizeof(m_out_irq_cb));
    	memset(&m_out_nmi_cb, 0, sizeof(m_out_nmi_cb));
    	memset(&m_out_res_cb, 0, sizeof(m_out_res_cb));
	}
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void vic20_expansion_slot_device::device_start()
{
	m_cart = dynamic_cast<device_vic20_expansion_card_interface *>(get_card_device());

	// resolve callbacks
	m_out_irq_func.resolve(m_out_irq_cb, *this);
	m_out_nmi_func.resolve(m_out_nmi_cb, *this);
	m_out_res_func.resolve(m_out_res_cb, *this);
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void vic20_expansion_slot_device::device_reset()
{
}


//-------------------------------------------------
//  call_load -
//-------------------------------------------------

bool vic20_expansion_slot_device::call_load()
{
	// TODO
	return IMAGE_INIT_FAIL;
}


//-------------------------------------------------
//  call_softlist_load -
//-------------------------------------------------

bool vic20_expansion_slot_device::call_softlist_load(char *swlist, char *swname, rom_entry *start_entry)
{
	load_software_part_region(this, swlist, swname, start_entry);

	return true;
}


//-------------------------------------------------
//  get_default_card_software -
//-------------------------------------------------

const char * vic20_expansion_slot_device::get_default_card_software(const machine_config &config, emu_options &options) const
{
	return software_get_default_slot(config, options, this, "standard");
}


//-------------------------------------------------
//  get_cart_base -
//-------------------------------------------------

UINT8* vic20_expansion_slot_device::get_cart_base()
{
	return NULL;
}


//-------------------------------------------------
//  screen_update -
//-------------------------------------------------

UINT32 vic20_expansion_slot_device::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	bool value = false;

	if (m_cart != NULL)
	{
		value = m_cart->vic20_screen_update(screen, bitmap, cliprect);
	}

	return value;
}


READ8_MEMBER( vic20_expansion_slot_device::ram1_r ) { UINT8 data = 0; if (m_cart != NULL) data = m_cart->vic20_ram1_r(space, offset); return data; }
WRITE8_MEMBER( vic20_expansion_slot_device::ram1_w ) { if (m_cart != NULL) m_cart->vic20_ram1_w(space, offset, data); }
READ8_MEMBER( vic20_expansion_slot_device::ram2_r ) { UINT8 data = 0; if (m_cart != NULL) data = m_cart->vic20_ram2_r(space, offset); return data; }
WRITE8_MEMBER( vic20_expansion_slot_device::ram2_w ) { if (m_cart != NULL) m_cart->vic20_ram2_w(space, offset, data); }
READ8_MEMBER( vic20_expansion_slot_device::ram3_r ) { UINT8 data = 0; if (m_cart != NULL) data = m_cart->vic20_ram3_r(space, offset); return data; }
WRITE8_MEMBER( vic20_expansion_slot_device::ram3_w ) { if (m_cart != NULL) m_cart->vic20_ram3_w(space, offset, data); }
READ8_MEMBER( vic20_expansion_slot_device::blk1_r ) { UINT8 data = 0; if (m_cart != NULL) data = m_cart->vic20_blk1_r(space, offset); return data; }
WRITE8_MEMBER( vic20_expansion_slot_device::blk1_w ) { if (m_cart != NULL) m_cart->vic20_blk1_w(space, offset, data); }
READ8_MEMBER( vic20_expansion_slot_device::blk2_r ) { UINT8 data = 0; if (m_cart != NULL) data = m_cart->vic20_blk2_r(space, offset); return data; }
WRITE8_MEMBER( vic20_expansion_slot_device::blk2_w ) { if (m_cart != NULL) m_cart->vic20_blk2_w(space, offset, data); }
READ8_MEMBER( vic20_expansion_slot_device::blk3_r ) { UINT8 data = 0; if (m_cart != NULL) data = m_cart->vic20_blk3_r(space, offset); return data; }
WRITE8_MEMBER( vic20_expansion_slot_device::blk3_w ) { if (m_cart != NULL) m_cart->vic20_blk3_w(space, offset, data); }
READ8_MEMBER( vic20_expansion_slot_device::blk5_r ) { UINT8 data = 0; if (m_cart != NULL) data = m_cart->vic20_blk5_r(space, offset); return data; }
WRITE8_MEMBER( vic20_expansion_slot_device::blk5_w ) { if (m_cart != NULL) m_cart->vic20_blk5_w(space, offset, data); }
READ8_MEMBER( vic20_expansion_slot_device::io2_r ) { UINT8 data = 0; if (m_cart != NULL) data = m_cart->vic20_io2_r(space, offset); return data; }
WRITE8_MEMBER( vic20_expansion_slot_device::io2_w ) { if (m_cart != NULL) m_cart->vic20_io2_w(space, offset, data); }
READ8_MEMBER( vic20_expansion_slot_device::io3_r ) { UINT8 data = 0; if (m_cart != NULL) data = m_cart->vic20_io3_r(space, offset); return data; }
WRITE8_MEMBER( vic20_expansion_slot_device::io3_w ) { if (m_cart != NULL) m_cart->vic20_io3_w(space, offset, data); }

WRITE_LINE_MEMBER( vic20_expansion_slot_device::irq_w ) { m_out_irq_func(state); }
WRITE_LINE_MEMBER( vic20_expansion_slot_device::nmi_w ) { m_out_nmi_func(state); }
WRITE_LINE_MEMBER( vic20_expansion_slot_device::res_w ) { m_out_res_func(state); }
