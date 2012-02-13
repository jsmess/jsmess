/**********************************************************************

    Commodore 64 Expansion Port emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "emu.h"
#include "emuopts.h"
#include "machine/c64exp.h"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type C64_EXPANSION_SLOT = &device_creator<c64_expansion_slot_device>;



//**************************************************************************
//  DEVICE C64_EXPANSION CARD INTERFACE
//**************************************************************************

//-------------------------------------------------
//  device_c64_expansion_card_interface - constructor
//-------------------------------------------------

device_c64_expansion_card_interface::device_c64_expansion_card_interface(const machine_config &mconfig, device_t &device)
	: device_slot_card_interface(mconfig, device),
	  m_roml(NULL),
	  m_romh(NULL),
	  m_ram(NULL),
	  m_roml_mask(0),
	  m_romh_mask(0),
	  m_ram_mask(0),
	  m_game(1),
	  m_exrom(1)
{
	m_slot = dynamic_cast<c64_expansion_slot_device *>(device.owner());
}


//-------------------------------------------------
//  ~device_c64_expansion_card_interface - destructor
//-------------------------------------------------

device_c64_expansion_card_interface::~device_c64_expansion_card_interface()
{
}


//-------------------------------------------------
//  c64_roml_pointer - get low ROM pointer
//-------------------------------------------------

UINT8* device_c64_expansion_card_interface::c64_roml_pointer(running_machine &machine, size_t size)
{
	if (m_roml == NULL)
	{
		m_roml = auto_alloc_array(machine, UINT8, size);
		m_roml_mask = size - 1;
	}

	return m_roml;
}


//-------------------------------------------------
//  c64_romh_pointer - get high ROM pointer
//-------------------------------------------------

UINT8* device_c64_expansion_card_interface::c64_romh_pointer(running_machine &machine, size_t size)
{
	if (m_romh == NULL)
	{
		m_romh = auto_alloc_array(machine, UINT8, size);
		m_romh_mask = size - 1;
	}

	return m_romh;
}


//-------------------------------------------------
//  c64_ram_pointer - get RAM pointer
//-------------------------------------------------

UINT8* device_c64_expansion_card_interface::c64_ram_pointer(running_machine &machine, size_t size)
{
	if (m_ram == NULL)
	{
		m_ram = auto_alloc_array(machine, UINT8, size);
		m_ram_mask = size - 1;
	}

	return m_ram;
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  c64_expansion_slot_device - constructor
//-------------------------------------------------

c64_expansion_slot_device::c64_expansion_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, C64_EXPANSION_SLOT, "C64 expansion port", tag, owner, clock),
		device_slot_interface(mconfig, *this),
		device_image_interface(mconfig, *this)
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
	}

	// set brief and instance name
	update_names();
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void c64_expansion_slot_device::device_start()
{
	m_cart = dynamic_cast<device_c64_expansion_card_interface *>(get_card_device());

	// resolve callbacks
	m_out_irq_func.resolve(m_out_irq_cb, *this);
	m_out_nmi_func.resolve(m_out_nmi_cb, *this);
	m_out_dma_func.resolve(m_out_dma_cb, *this);
	m_out_reset_func.resolve(m_out_reset_cb, *this);
}


//-------------------------------------------------
//  call_load -
//-------------------------------------------------

bool c64_expansion_slot_device::call_load()
{
	if (m_cart)
	{
		size_t size = 0;

		if (software_entry() == NULL)
		{
			size = length();

			if (!mame_stricmp(filetype(), "80"))
			{
				fread(m_cart->c64_roml_pointer(machine(), 0x2000), 0x2000);
				m_cart->c64_exrom_w(0);

				if (size == 0x4000)
				{
					fread(m_cart->c64_romh_pointer(machine(), 0x2000), 0x2000);
					m_cart->c64_game_w(0);
				}
			}
			else if (!mame_stricmp(filetype(), "a0"))
			{
				fread(m_cart->c64_romh_pointer(machine(), 0x2000), size);

				m_cart->c64_game_w(0);
				m_cart->c64_exrom_w(0);
			}
			else if (!mame_stricmp(filetype(), "e0"))
			{
				fread(m_cart->c64_romh_pointer(machine(), 0x2000), size);

				m_cart->c64_game_w(0);
			}
		}
		else
		{
			if (get_feature("slot") == NULL)
			{
				size = get_software_region_length("roml");

				switch (size)
				{
				case 0x2000:
					memcpy(m_cart->c64_roml_pointer(machine(), 0x2000), get_software_region("roml"), size);

					size = get_software_region_length("romh");
					if (size) memcpy(m_cart->c64_romh_pointer(machine(), 0x2000), get_software_region("romh"), size);
					break;

				case 0x4000:
					memcpy(m_cart->c64_roml_pointer(machine(), 0x2000), get_software_region("roml"), 0x2000);
					memcpy(m_cart->c64_romh_pointer(machine(), 0x2000), get_software_region("roml") + 0x2000, 0x2000);
					break;

				default:
					memcpy(m_cart->c64_roml_pointer(machine(), size), get_software_region("roml"), size);
					break;
				}
			}
			else
			{
				size = get_software_region_length("roml");
				if (size) memcpy(m_cart->c64_roml_pointer(machine(), size), get_software_region("roml"), size);
				
				size = get_software_region_length("romh");
				if (size) memcpy(m_cart->c64_romh_pointer(machine(), size), get_software_region("romh"), size);

				size = get_software_region_length("ram");
				if (size) memset(m_cart->c64_ram_pointer(machine(), size), 0, size);
			}

			m_cart->c64_game_w(atol(get_feature("game")));
			m_cart->c64_exrom_w(atol(get_feature("exrom")));
		}
	}

	return IMAGE_INIT_PASS;
}


//-------------------------------------------------
//  call_softlist_load -
//-------------------------------------------------

bool c64_expansion_slot_device::call_softlist_load(char *swlist, char *swname, rom_entry *start_entry)
{
	load_software_part_region(this, swlist, swname, start_entry);

	return true;
}


//-------------------------------------------------
//  get_default_card_software -
//-------------------------------------------------

const char * c64_expansion_slot_device::get_default_card_software(const machine_config &config, emu_options &options) const
{
	return software_get_default_slot(config, options, this, "standard");
}


//-------------------------------------------------
//  read - 
//-------------------------------------------------

READ8_MEMBER( c64_expansion_slot_device::read )
{
	UINT8 data = 0;

	if (m_cart != NULL)
	{
		data = m_cart->c64_cd_r(space, offset, 1, 1, 1, 1);
	}

	return data;
}


//-------------------------------------------------
//  write - 
//-------------------------------------------------

WRITE8_MEMBER( c64_expansion_slot_device::write )
{
	if (m_cart != NULL)
	{
		m_cart->c64_cd_w(space, offset, data, 1, 1, 1, 1);
	}
}


//-------------------------------------------------
//  roml_r - low ROM read
//-------------------------------------------------

READ8_MEMBER( c64_expansion_slot_device::roml_r )
{
	UINT8 data = 0;

	if (m_cart != NULL)
	{
		data = m_cart->c64_cd_r(space, offset, 0, 1, 1, 1);
	}

	return data;
}


//-------------------------------------------------
//  romh_r - high ROM read
//-------------------------------------------------

READ8_MEMBER( c64_expansion_slot_device::romh_r )
{
	UINT8 data = 0;

	if (m_cart != NULL)
	{
		data = m_cart->c64_cd_r(space, offset, 1, 0, 1, 1);
	}

	return data;
}


//-------------------------------------------------
//  io1_r - I/O 1 read
//-------------------------------------------------

READ8_MEMBER( c64_expansion_slot_device::io1_r )
{
	UINT8 data = 0;

	if (m_cart != NULL)
	{
		data = m_cart->c64_cd_r(space, offset, 1, 1, 0, 1);
	}

	return data;
}


//-------------------------------------------------
//  io1_w - low ROM write
//-------------------------------------------------

WRITE8_MEMBER( c64_expansion_slot_device::io1_w )
{
	if (m_cart != NULL)
	{
		m_cart->c64_cd_w(space, offset, data, 1, 1, 0, 1);
	}
}


//-------------------------------------------------
//  io2_r - I/O 2 read
//-------------------------------------------------

READ8_MEMBER( c64_expansion_slot_device::io2_r )
{
	UINT8 data = 0;

	if (m_cart != NULL)
	{
		data = m_cart->c64_cd_r(space, offset, 1, 1, 1, 0);
	}

	return data;
}


//-------------------------------------------------
//  io2_w - low ROM write
//-------------------------------------------------

WRITE8_MEMBER( c64_expansion_slot_device::io2_w )
{
	if (m_cart != NULL)
	{
		m_cart->c64_cd_w(space, offset, data, 1, 1, 1, 0);
	}
}


//-------------------------------------------------
//  game_r - GAME read
//-------------------------------------------------

int c64_expansion_slot_device::game_r(offs_t offset, int ba, int rw)
{
	int state = 1;

	if (m_cart != NULL)
	{
		state = m_cart->c64_game_r(offset, ba, rw);
	}

	return state;
}


//-------------------------------------------------
//  game_r - EXROM read
//-------------------------------------------------

READ_LINE_MEMBER( c64_expansion_slot_device::exrom_r )
{
	int state = 1;

	if (m_cart != NULL)
	{
		state = m_cart->c64_exrom_r();
	}

	return state;
}


//-------------------------------------------------
//  screen_update -
//-------------------------------------------------

UINT32 c64_expansion_slot_device::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	bool value = false;

	if (m_cart != NULL)
	{
		value = m_cart->c64_screen_update(screen, bitmap, cliprect);
	}

	return value;
}


WRITE_LINE_MEMBER( c64_expansion_slot_device::irq_w ) { m_out_irq_func(state); }
WRITE_LINE_MEMBER( c64_expansion_slot_device::nmi_w ) { m_out_nmi_func(state); }
WRITE_LINE_MEMBER( c64_expansion_slot_device::dma_w ) { m_out_dma_func(state); }
WRITE_LINE_MEMBER( c64_expansion_slot_device::reset_w ) { m_out_reset_func(state); }
