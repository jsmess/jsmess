/***************************************************************************

  nubus.c - NuBus bus and card emulation

  by R. Belmont, based heavily on Miodrag Milanovic's ISA8/16 implementation

***************************************************************************/

#include "emu.h"
#include "emuopts.h"
#include "machine/nubus.h"


//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type NUBUS_SLOT = &device_creator<nubus_slot_device>;

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  nubus_slot_device - constructor
//-------------------------------------------------
nubus_slot_device::nubus_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, NUBUS_SLOT, "NUBUS_SLOT", tag, owner, clock),
		device_slot_interface(mconfig, *this)
{
}

nubus_slot_device::nubus_slot_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, type, name, tag, owner, clock),
		device_slot_interface(mconfig, *this)
{
}

void nubus_slot_device::static_set_nubus_slot(device_t &device, const char *tag, const char *slottag, const char *defslot)
{
	nubus_slot_device &nubus_card = dynamic_cast<nubus_slot_device &>(device);
	nubus_card.m_nubus_tag = tag;
	nubus_card.m_nubus_slottag = slottag;
	nubus_card.m_nubus_defslot = defslot;
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void nubus_slot_device::device_start()
{
	device_nubus_card_interface *dev = dynamic_cast<device_nubus_card_interface *>(get_card_device());

	if (dev) device_nubus_card_interface::static_set_nubus_tag(*dev, m_nubus_tag, m_nubus_slottag, m_nubus_defslot);
}

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type NUBUS = &device_creator<nubus_device>;

void nubus_device::static_set_cputag(device_t &device, const char *tag)
{
	nubus_device &nubus = downcast<nubus_device &>(device);
	nubus.m_cputag = tag;
}

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void nubus_device::device_config_complete()
{
	// inherit a copy of the static data
	const nbbus_interface *intf = reinterpret_cast<const nbbus_interface *>(static_config());
	if (intf != NULL)
	{
		*static_cast<nbbus_interface *>(this) = *intf;
	}

	// or initialize to defaults if none provided
	else
	{
    	memset(&m_out_irq9_cb, 0, sizeof(m_out_irq9_cb));
    	memset(&m_out_irqa_cb, 0, sizeof(m_out_irqa_cb));
    	memset(&m_out_irqb_cb, 0, sizeof(m_out_irqb_cb));
    	memset(&m_out_irqc_cb, 0, sizeof(m_out_irqc_cb));
    	memset(&m_out_irqd_cb, 0, sizeof(m_out_irqd_cb));
    	memset(&m_out_irqe_cb, 0, sizeof(m_out_irqe_cb));
	}
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  nubus_device - constructor
//-------------------------------------------------

nubus_device::nubus_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, NUBUS, "NUBUS", tag, owner, clock)
{
}

nubus_device::nubus_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, type, name, tag, owner, clock)
{
}
//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void nubus_device::device_start()
{
	m_maincpu = machine().device(m_cputag);
	// resolve callbacks
	m_out_irq9_func.resolve(m_out_irq9_cb, *this);
	m_out_irqa_func.resolve(m_out_irqa_cb, *this);
	m_out_irqb_func.resolve(m_out_irqb_cb, *this);
	m_out_irqc_func.resolve(m_out_irqc_cb, *this);
	m_out_irqd_func.resolve(m_out_irqd_cb, *this);
	m_out_irqe_func.resolve(m_out_irqe_cb, *this);
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void nubus_device::device_reset()
{
}

void nubus_device::add_nubus_card(device_nubus_card_interface *card)
{
	m_device_list.append(*card);
}

void nubus_device::install_device(device_t *dev, offs_t start, offs_t end, offs_t mask, offs_t mirror, read32_device_func rhandler, const char* rhandler_name, write32_device_func whandler, const char *whandler_name)
{
	m_maincpu = machine().device(m_cputag);
	int buswidth = m_maincpu->memory().space_config(AS_PROGRAM)->m_databus_width;
	switch(buswidth)
	{
		case 32:
			m_maincpu->memory().space(AS_PROGRAM)->install_legacy_readwrite_handler(*dev, start, end, mask, mirror, rhandler, rhandler_name, whandler, whandler_name,0xffffffff);
			break;
		case 64:
			m_maincpu->memory().space(AS_PROGRAM)->install_legacy_readwrite_handler(*dev, start, end, mask, mirror, rhandler, rhandler_name, whandler, whandler_name,U64(0xffffffffffffffff));
			break;
		default:
			fatalerror("NUBUS: Bus width %d not supported", buswidth);
			break;
	}
}

void nubus_device::install_bank(offs_t start, offs_t end, offs_t mask, offs_t mirror, const char *tag, UINT8 *data)
{
	m_maincpu = machine().device(m_cputag);
	address_space *space = m_maincpu->memory().space(AS_PROGRAM);
	space->install_readwrite_bank(start, end, mask, mirror, tag );
	memory_set_bankptr(machine(), tag, data);
}

void nubus_device::install_rom(device_t *dev, offs_t start, offs_t end, offs_t mask, offs_t mirror, const char *tag, const char *region)
{
	m_maincpu = machine().device(m_cputag);
	astring tempstring;
	address_space *space = m_maincpu->memory().space(AS_PROGRAM);
	space->install_read_bank(start, end, mask, mirror, tag);
	space->unmap_write(start, end, mask, mirror);
	memory_set_bankptr(machine(), tag, machine().region(dev->subtag(tempstring, region))->base());
}

// interrupt request from nubus card
WRITE_LINE_MEMBER( nubus_device::irq9_w ) { m_out_irq9_func(state); }
WRITE_LINE_MEMBER( nubus_device::irqa_w ) { m_out_irqa_func(state); }
WRITE_LINE_MEMBER( nubus_device::irqb_w ) { m_out_irqb_func(state); }
WRITE_LINE_MEMBER( nubus_device::irqc_w ) { m_out_irqc_func(state); }
WRITE_LINE_MEMBER( nubus_device::irqd_w ) { m_out_irqd_func(state); }
WRITE_LINE_MEMBER( nubus_device::irqe_w ) { m_out_irqe_func(state); }

//**************************************************************************
//  DEVICE CONFIG NUBUS CARD INTERFACE
//**************************************************************************


//**************************************************************************
//  DEVICE NUBUS CARD INTERFACE
//**************************************************************************

//-------------------------------------------------
//  device_nubus_card_interface - constructor
//-------------------------------------------------

device_nubus_card_interface::device_nubus_card_interface(const machine_config &mconfig, device_t &device)
	: device_interface(device),
	  m_nubus(NULL),
	  m_nubus_tag(NULL)
{
}


//-------------------------------------------------
//  ~device_nubus_card_interface - destructor
//-------------------------------------------------

device_nubus_card_interface::~device_nubus_card_interface()
{
}

void device_nubus_card_interface::static_set_nubus_tag(device_t &device, const char *tag, const char *slottag, const char *defslot)
{
	device_nubus_card_interface &nubus_card = dynamic_cast<device_nubus_card_interface &>(device);
	nubus_card.m_nubus_tag = tag;
	nubus_card.m_nubus_slottag = slottag;
	nubus_card.m_nubus_defslot = defslot;
}

void device_nubus_card_interface::set_nubus_device()
{
	// extract the slot number from the last digit of the slot tag
	int tlen = strlen(m_nubus_slottag);
	m_slot = (m_nubus_slottag[tlen-1] - '9') + 9;

	if (m_slot < 9 || m_slot > 0xe)
	{
		fatalerror("Slot %x out of range for Apple NuBus\n", m_slot);
	}

	m_nubus = dynamic_cast<nubus_device *>(device().machine().device(m_nubus_tag));
	m_nubus->add_nubus_card(this);
}

void device_nubus_card_interface::install_declaration_rom(const char *romregion)
{
	char region_name[128];
	bool inverted = false;
	UINT8 *newrom = NULL;

	sprintf(region_name, "%s:%s:%s", m_nubus_slottag, m_nubus_defslot, romregion);

	UINT8 *rom = device().machine().region(region_name)->base();
	UINT32 romlen = device().machine().region(region_name)->bytes();

	printf("ROM length is %x, last byte is %02x\n", romlen, rom[romlen-1]);

	UINT8 byteLanes = rom[romlen-1];
	// check if all bits are inverted
	if (rom[romlen-2] == 0xff)
	{
		byteLanes ^= 0xff;
		inverted = true;
	}

	switch (byteLanes)
	{
		case 0x0f:	// easy case: all 4 lanes (still must scramble for 32-bit BE bus though)
			newrom = (UINT8 *)malloc(romlen);
			memset(newrom, 0, romlen);
			for (int i = 0; i < romlen; i++)
			{
				newrom[BYTE4_XOR_BE(i)] = rom[i];
			}
			break;

		case 0xe1:	// lane 0 only
			newrom = (UINT8 *)malloc(romlen*4);
			memset(newrom, 0, romlen*4);
			for (int i = 0; i < romlen; i++)
			{
				newrom[BYTE4_XOR_BE(i*4)] = rom[i];
			}
			romlen *= 4;
			break;

		case 0xd2:	// lane 1 only
			newrom = (UINT8 *)malloc(romlen*4);
			memset(newrom, 0, romlen*4);
			for (int i = 0; i < romlen; i++)
			{
				newrom[BYTE4_XOR_BE((i*4)+1)] = rom[i];
			}
			romlen *= 4;
			break;

		case 0xb4:	// lane 2 only
			newrom = (UINT8 *)malloc(romlen*4);
			memset(newrom, 0, romlen*4);
			for (int i = 0; i < romlen; i++)
			{
				newrom[BYTE4_XOR_BE((i*4)+2)] = rom[i];
			}
			romlen *= 4;
			break;

		case 0x78:	// lane 3 only
			newrom = (UINT8 *)malloc(romlen*4);
			memset(newrom, 0, romlen*4);
			for (int i = 0; i < romlen; i++)
			{
				newrom[BYTE4_XOR_BE((i*4)+3)] = rom[i];
			}
			romlen *= 4;
			break;

		case 0xc3:	// lanes 0, 1
			newrom = (UINT8 *)malloc(romlen*2);
			memset(newrom, 0, romlen*2);
			for (int i = 0; i < romlen; i+=2)
			{
				newrom[BYTE4_XOR_BE((i*4))] = rom[i];
				newrom[BYTE4_XOR_BE((i*4)+1)] = rom[i+1];
			}
			romlen *= 4;
			break;

		case 0x3c:	// lanes 2,3
			newrom = (UINT8 *)malloc(romlen*2);
			memset(newrom, 0, romlen*2);
			for (int i = 0; i < romlen; i+=2)
			{
				newrom[BYTE4_XOR_BE((i*4)+2)] = rom[i];
				newrom[BYTE4_XOR_BE((i*4)+3)] = rom[i+1];
			}
			romlen *= 4;
			break;

		default:
			fatalerror("NuBus: unhandled byteLanes value %02x\n", byteLanes);
			break;
	}

	// the slot manager can supposedly handle inverted ROMs by itself, but let's do it for it anyway
	if (inverted)
	{
		for (int i = 0; i < romlen; i++)
		{
			newrom[i] ^= 0xff;
		}
	}

	// now install the ROM
	UINT32 addr = get_slotspace() + 0x01000000;
	char bankname[128];
	strcpy(bankname, "rom_");
	strcat(bankname, m_nubus_slottag);
	addr -= romlen;
	printf("Installing ROM bank [%s] at %08x\n", bankname, addr);
	m_nubus->install_bank(addr, addr+romlen-1, addr+romlen-1, 0, bankname, newrom);
}

