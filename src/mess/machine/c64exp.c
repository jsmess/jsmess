/**********************************************************************

    Commodore 64 Expansion Port emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "emu.h"
#include "emuopts.h"
#include "machine/c64exp.h"
#include "formats/cbm_crt.h"
#include "formats/imageutl.h"



//**************************************************************************
//  MACROS/CONSTANTS
//**************************************************************************

#define LOG 0



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
				fread(m_cart->c64_romh_pointer(machine(), 0x2000), 0x2000);

				m_cart->c64_exrom_w(0);
				m_cart->c64_game_w(0);
			}
			else if (!mame_stricmp(filetype(), "e0"))
			{
				fread(m_cart->c64_romh_pointer(machine(), 0x2000), 0x2000);

				m_cart->c64_game_w(0);
			}
			else if (!mame_stricmp(filetype(), "crt"))
			{
				// read the header
				cbm_crt_header header;
				fread(&header, CRT_HEADER_LENGTH);
				
				if (memcmp(header.signature, CRT_SIGNATURE, 16) != 0)
					return IMAGE_INIT_FAIL;
				
				UINT16 hardware = pick_integer_be(header.hardware, 0, 2);
				
				// TODO support other cartridge hardware
				if (hardware != CRT_C64_STANDARD)
					return IMAGE_INIT_FAIL;
				
				if (LOG)
				{
					logerror("Name: %s\n", header.name);
					logerror("Hardware: %04x\n", hardware);
					logerror("Slot device: %s\n", CRT_C64_SLOT_NAMES[hardware]);
					logerror("EXROM: %u\n", header.exrom);
					logerror("GAME: %u\n", header.game);
				}
			
				// determine ROM region lengths
				size_t roml_size = 0;
				size_t romh_size = 0;
				
				while (!image_feof())
				{
					cbm_crt_chip chip;
					fread(&chip, CRT_CHIP_LENGTH);
					
					UINT16 address = pick_integer_be(chip.start_address, 0, 2);
					UINT16 size = pick_integer_be(chip.image_size, 0, 2);
					UINT16 type = pick_integer_be(chip.chip_type, 0, 2);
					
					if (LOG)
					{
						logerror("CHIP Address: %04x\n", address);
						logerror("CHIP Size: %04x\n", size);
						logerror("CHIP Type: %04x\n", type);
					}
					
					switch (address)
					{
					case 0x8000: roml_size += size; break;
					case 0xa000: romh_size += size; break;
					case 0xe000: romh_size += size; break;
					default: logerror("Invalid CHIP loading address!\n"); break;
					}
					
					fseek(size, SEEK_CUR);
				}

				// allocate cartridge memory
				UINT8 *roml = NULL;
				UINT8 *romh = NULL;
				
				if (roml_size) roml = m_cart->c64_roml_pointer(machine(), roml_size);
				if (romh_size) romh = m_cart->c64_romh_pointer(machine(), romh_size);
				
				// read the data
				offs_t roml_offset = 0;
				offs_t romh_offset = 0;
				
				fseek(CRT_HEADER_LENGTH, SEEK_SET);
				
				while (!image_feof())
				{
					cbm_crt_chip chip;
					fread(&chip, CRT_CHIP_LENGTH);
					
					UINT16 address = pick_integer_be(chip.start_address, 0, 2);
					UINT16 size = pick_integer_be(chip.image_size, 0, 2);

					switch (address)
					{
					case 0x8000: fread(roml + roml_offset, size); roml_offset += size; break;
					case 0xa000: fread(romh + romh_offset, size); romh_offset += size; break;
					case 0xe000: fread(romh + romh_offset, size); romh_offset += size; break;
					}
				}
				
				m_cart->c64_exrom_w(header.exrom);
				m_cart->c64_game_w(header.game);
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

			m_cart->c64_exrom_w(atol(get_feature("exrom")));
			m_cart->c64_game_w(atol(get_feature("game")));
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
//  cd_r - 
//-------------------------------------------------

UINT8 c64_expansion_slot_device::cd_r(address_space &space, offs_t offset, int roml, int romh, int io1, int io2)
{
	UINT8 data = 0;

	if (m_cart != NULL)
	{
		data = m_cart->c64_cd_r(space, offset, roml, romh, io1, io2);
	}

	return data;
}


//-------------------------------------------------
//  cd_w - 
//-------------------------------------------------

void c64_expansion_slot_device::cd_w(address_space &space, offs_t offset, UINT8 data, int roml, int romh, int io1, int io2)
{
	if (m_cart != NULL)
	{
		m_cart->c64_cd_w(space, offset, data, roml, romh, io1, io2);
	}
}


//-------------------------------------------------
//  game_r - GAME read
//-------------------------------------------------

int c64_expansion_slot_device::game_r(offs_t offset, int ba, int rw, int hiram)
{
	int state = 1;

	if (m_cart != NULL)
	{
		state = m_cart->c64_game_r(offset, ba, rw, hiram);
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
