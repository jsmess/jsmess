/**********************************************************************

    Commodore VIC-20 Expansion Port emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "emu.h"
#include "emuopts.h"
#include "formats/cbm_crt.h"
#include "machine/vic20exp.h"
#include "formats/imageutl.h"



//**************************************************************************
//  MACROS/CONSTANTS
//**************************************************************************

#define LOG 0


// slot names for the VIC-20 cartridge types
const char *CRT_VIC20_SLOT_NAMES[_CRT_VIC20_COUNT] =
{
	UNSUPPORTED,
	"standard",
	UNSUPPORTED,
	UNSUPPORTED,
	UNSUPPORTED
};



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
	: device_slot_card_interface(mconfig, device),
	  m_blk1(NULL),
	  m_blk2(NULL),
	  m_blk3(NULL),
	  m_blk5(NULL),
	  m_ram(NULL)
{
	m_slot = dynamic_cast<vic20_expansion_slot_device *>(device.owner());
}


//-------------------------------------------------
//  vic20_blk1_pointer - get block 1 pointer
//-------------------------------------------------

UINT8* device_vic20_expansion_card_interface::vic20_blk1_pointer(running_machine &machine, size_t size)
{
	if (m_blk1 == NULL)
	{
		m_blk1 = auto_alloc_array(machine, UINT8, size);
	}

	return m_blk1;
}


//-------------------------------------------------
//  vic20_blk2_pointer - get block 2 pointer
//-------------------------------------------------

UINT8* device_vic20_expansion_card_interface::vic20_blk2_pointer(running_machine &machine, size_t size)
{
	if (m_blk2 == NULL)
	{
		m_blk2 = auto_alloc_array(machine, UINT8, size);
	}

	return m_blk2;
}


//-------------------------------------------------
//  vic20_blk3_pointer - get block 3 pointer
//-------------------------------------------------

UINT8* device_vic20_expansion_card_interface::vic20_blk3_pointer(running_machine &machine, size_t size)
{
	if (m_blk3 == NULL)
	{
		m_blk3 = auto_alloc_array(machine, UINT8, size);
	}

	return m_blk3;
}


//-------------------------------------------------
//  vic20_blk5_pointer - get block 5 pointer
//-------------------------------------------------

UINT8* device_vic20_expansion_card_interface::vic20_blk5_pointer(running_machine &machine, size_t size)
{
	if (m_blk5 == NULL)
	{
		m_blk5 = auto_alloc_array(machine, UINT8, size);
	}

	return m_blk5;
}


//-------------------------------------------------
//  vic20_ram_pointer - get RAM pointer
//-------------------------------------------------

UINT8* device_vic20_expansion_card_interface::vic20_ram_pointer(running_machine &machine, size_t size)
{
	if (m_ram == NULL)
	{
		m_ram = auto_alloc_array(machine, UINT8, size);
	}

	return m_ram;
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

	// set brief and instance name
	update_names();
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
//  call_load -
//-------------------------------------------------

bool vic20_expansion_slot_device::call_load()
{
	if (m_cart)
	{
		size_t size = 0;

		if (software_entry() == NULL)
		{
			size = length();

			if (!mame_stricmp(filetype(), "20")) fread(m_cart->vic20_blk1_pointer(machine(), size), size);
			else if (!mame_stricmp(filetype(), "40")) fread(m_cart->vic20_blk2_pointer(machine(), size), size);
			else if (!mame_stricmp(filetype(), "60")) fread(m_cart->vic20_blk3_pointer(machine(), size), size);
			else if (!mame_stricmp(filetype(), "70")) fread(m_cart->vic20_blk3_pointer(machine(), size + 0x1000) + 0x1000, size);
			else if (!mame_stricmp(filetype(), "a0")) fread(m_cart->vic20_blk5_pointer(machine(), size), size);
			else if (!mame_stricmp(filetype(), "b0")) fread(m_cart->vic20_blk5_pointer(machine(), size + 0x1000) + 0x1000, size);
			else if (!mame_stricmp(filetype(), "crt"))
			{
				// read the header
				cbm_crt_header header;
				fread(&header, CRT_HEADER_LENGTH);

				if (memcmp(header.signature, CRT_SIGNATURE, 16) != 0)
					return IMAGE_INIT_FAIL;

				UINT16 hardware = pick_integer_be(header.hardware, 0, 2);

				// TODO support other cartridge hardware
				if (hardware != CRT_VIC20_STANDARD)
					return IMAGE_INIT_FAIL;

				if (LOG)
				{
					logerror("Name: %s\n", header.name);
					logerror("Hardware: %04x\n", hardware);
					logerror("Slot device: %s\n", CRT_VIC20_SLOT_NAMES[hardware]);
				}

				// determine ROM region lengths
				size_t blk1_size = 0;
				size_t blk2_size = 0;
				size_t blk3_size = 0;
				size_t blk5_size = 0;

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
					case 0x2000: blk1_size += size; break;
					case 0x4000: blk2_size += size; break;
					case 0x6000: blk3_size += size; break;
					case 0x7000: blk3_size += 0x2000; break;
					case 0xa000: blk5_size += size; break;
					case 0xb000: blk5_size += 0x2000; break;
					default: logerror("Invalid CHIP loading address!\n"); break;
					}

					fseek(size, SEEK_CUR);
				}

				// allocate cartridge memory
				UINT8 *blk1 = NULL;
				UINT8 *blk2 = NULL;
				UINT8 *blk3 = NULL;
				UINT8 *blk5 = NULL;

				if (blk1_size) blk1 = m_cart->vic20_blk1_pointer(machine(), blk1_size);
				if (blk2_size) blk2 = m_cart->vic20_blk2_pointer(machine(), blk2_size);
				if (blk3_size) blk3 = m_cart->vic20_blk3_pointer(machine(), blk3_size);
				if (blk5_size) blk5 = m_cart->vic20_blk5_pointer(machine(), blk5_size);

				// read the data
				offs_t blk1_offset = 0;
				offs_t blk2_offset = 0;
				offs_t blk3_offset = 0;
				offs_t blk5_offset = 0;

				fseek(CRT_HEADER_LENGTH, SEEK_SET);

				while (!image_feof())
				{
					cbm_crt_chip chip;
					fread(&chip, CRT_CHIP_LENGTH);

					UINT16 address = pick_integer_be(chip.start_address, 0, 2);
					UINT16 size = pick_integer_be(chip.image_size, 0, 2);

					switch (address)
					{
					case 0x2000: fread(blk1 + blk1_offset, size); blk1_offset += size; break;
					case 0x4000: fread(blk2 + blk2_offset, size); blk2_offset += size; break;
					case 0x6000: fread(blk3 + blk3_offset, size); blk3_offset += size; break;
					case 0x7000: fread(blk3 + blk3_offset + 0x1000, size); blk3_offset += 0x2000; break;
					case 0xa000: fread(blk5 + blk5_offset, size); blk5_offset += size; break;
					case 0xb000: fread(blk5 + blk5_offset + 0x1000, size); blk5_offset += 0x2000; break;
					}
				}
			}
		}
		else
		{
			size = get_software_region_length("blk1");
			if (size) memcpy(m_cart->vic20_blk1_pointer(machine(), size), get_software_region("blk1"), size);

			size = get_software_region_length("blk2");
			if (size) memcpy(m_cart->vic20_blk2_pointer(machine(), size), get_software_region("blk2"), size);

			size = get_software_region_length("blk3");
			if (size) memcpy(m_cart->vic20_blk3_pointer(machine(), size), get_software_region("blk3"), size);

			size = get_software_region_length("blk5");
			if (size) memcpy(m_cart->vic20_blk5_pointer(machine(), size), get_software_region("blk5"), size);

			size = get_software_region_length("ram");
			if (size) memcpy(m_cart->vic20_ram_pointer(machine(), size), get_software_region("ram"), size);
		}
	}

	return IMAGE_INIT_PASS;
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

const char * vic20_expansion_slot_device::get_default_card_software(const machine_config &config, emu_options &options)
{
	return software_get_default_slot(config, options, this, "standard");
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
