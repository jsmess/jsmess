/***************************************************************************

        ISA bus device

***************************************************************************/

#include "emu.h"
#include "machine/isa.h"


//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type ISA8 = &device_creator<isa8_device>;

void isa8_device::static_set_cputag(device_t &device, const char *tag)
{
	isa8_device &isa = downcast<isa8_device &>(device);
	isa.m_cputag = tag;
}

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void isa8_device::device_config_complete()
{
	// inherit a copy of the static data
	const isabus_interface *intf = reinterpret_cast<const isabus_interface *>(static_config());
	if (intf != NULL)
	{
		*static_cast<isabus_interface *>(this) = *intf;
	}

	// or initialize to defaults if none provided
	else
	{
    	memset(&m_out_irq2_cb, 0, sizeof(m_out_irq2_cb));
    	memset(&m_out_irq3_cb, 0, sizeof(m_out_irq3_cb));
    	memset(&m_out_irq4_cb, 0, sizeof(m_out_irq4_cb));
    	memset(&m_out_irq5_cb, 0, sizeof(m_out_irq5_cb));
    	memset(&m_out_irq6_cb, 0, sizeof(m_out_irq6_cb));
    	memset(&m_out_irq7_cb, 0, sizeof(m_out_irq7_cb));
    	memset(&m_out_drq1_cb, 0, sizeof(m_out_drq1_cb));
    	memset(&m_out_drq2_cb, 0, sizeof(m_out_drq2_cb));
    	memset(&m_out_drq3_cb, 0, sizeof(m_out_drq3_cb));
	}
}


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  isa8_device - constructor
//-------------------------------------------------

isa8_device::isa8_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, ISA8, "ISA8", tag, owner, clock)
{
	for(int i = 0; i < 8; i++)
		m_isa_device[i] = NULL;
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void isa8_device::device_start()
{
	m_maincpu = machine().device(m_cputag);
	// resolve callbacks
	devcb_resolve_write_line(&m_out_irq2_func, &m_out_irq2_cb, this);
	devcb_resolve_write_line(&m_out_irq3_func, &m_out_irq3_cb, this);
	devcb_resolve_write_line(&m_out_irq4_func, &m_out_irq4_cb, this);
	devcb_resolve_write_line(&m_out_irq5_func, &m_out_irq5_cb, this);
	devcb_resolve_write_line(&m_out_irq6_func, &m_out_irq6_cb, this);
	devcb_resolve_write_line(&m_out_irq7_func, &m_out_irq7_cb, this);
	devcb_resolve_write_line(&m_out_drq1_func, &m_out_drq1_cb, this);
	devcb_resolve_write_line(&m_out_drq2_func, &m_out_drq2_cb, this);
	devcb_resolve_write_line(&m_out_drq3_func, &m_out_drq3_cb, this);
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void isa8_device::device_reset()
{
}

void isa8_device::add_isa_card(device_isa8_card_interface *card,int pos)
{
	m_isa_device[pos] = card;
}

void isa8_device::install_device(device_t *dev, offs_t start, offs_t end, offs_t mask, offs_t mirror, read8_device_func rhandler, const char* rhandler_name, write8_device_func whandler, const char *whandler_name)
{
	m_maincpu = machine().device(m_cputag);
	int buswidth = m_maincpu->memory().space_config(AS_PROGRAM)->m_databus_width;
	switch(buswidth)
	{
		case 8:
			m_maincpu->memory().space(AS_IO)->install_legacy_readwrite_handler(*dev, start, end, mask, mirror, rhandler, rhandler_name, whandler, whandler_name,0);
			break;
		case 16:
			m_maincpu->memory().space(AS_IO)->install_legacy_readwrite_handler(*dev, start, end, mask, mirror, rhandler, rhandler_name, whandler, whandler_name,0xffff);
			break;
		default:
			fatalerror("ISA8: Bus width %d not supported", buswidth);
			break;
	}
}

void isa8_device::install_bank(offs_t start, offs_t end, offs_t mask, offs_t mirror, const char *tag, UINT8 *data)
{
	address_space *space = m_maincpu->memory().space(AS_PROGRAM);
	space->install_readwrite_bank(start, end, mask, mirror, tag );
	memory_set_bankptr(machine(), tag, data);
}

void isa8_device::install_rom(device_t *dev, offs_t start, offs_t end, offs_t mask, offs_t mirror, const char *tag, const char *region)
{
	astring tempstring;
	address_space *space = m_maincpu->memory().space(AS_PROGRAM);
	space->install_read_bank(start, end, mask, mirror, tag);
	space->unmap_write(start, end, mask, mirror);
	memory_set_bankptr(machine(), tag, machine().region(dev->subtag(tempstring, region))->base());
}

// interrupt request from isa card
WRITE_LINE_MEMBER( isa8_device::irq2_w ) { devcb_call_write_line(&m_out_irq2_func, state); }
WRITE_LINE_MEMBER( isa8_device::irq3_w ) { devcb_call_write_line(&m_out_irq3_func, state); }
WRITE_LINE_MEMBER( isa8_device::irq4_w ) { devcb_call_write_line(&m_out_irq4_func, state); }
WRITE_LINE_MEMBER( isa8_device::irq5_w ) { devcb_call_write_line(&m_out_irq5_func, state); }
WRITE_LINE_MEMBER( isa8_device::irq6_w ) { devcb_call_write_line(&m_out_irq6_func, state); }
WRITE_LINE_MEMBER( isa8_device::irq7_w ) { devcb_call_write_line(&m_out_irq7_func, state); }

// dma request from isa card
WRITE_LINE_MEMBER( isa8_device::drq1_w ) { devcb_call_write_line(&m_out_drq1_func, state); }
WRITE_LINE_MEMBER( isa8_device::drq2_w ) { devcb_call_write_line(&m_out_drq2_func, state); }
WRITE_LINE_MEMBER( isa8_device::drq3_w ) { devcb_call_write_line(&m_out_drq3_func, state); }

UINT8 isa8_device::dack_r(int line)
{
	UINT8 retVal = 0xff;
	for(int i=0;i<8;i++) {
		if (m_isa_device[i] != NULL && m_isa_device[i]->have_dack(line)) {
			retVal = m_isa_device[i]->dack_r(line);
		}
	}
	return retVal;
}

void isa8_device::dack_w(int line,UINT8 data)
{
	for(int i=0;i<8;i++) {
		if (m_isa_device[i] != NULL && m_isa_device[i]->have_dack(line)) {
			m_isa_device[i]->dack_w(line,data);
		}
	}
}

void isa8_device::eop_w(int state)
{
	for(int i=0;i<8;i++) {
		if (m_isa_device[i] != NULL) m_isa_device[i]->eop_w(state);
	}
}


//**************************************************************************
//  DEVICE CONFIG ISA8 CARD INTERFACE
//**************************************************************************

void device_isa8_card_interface::static_set_isa8_tag(device_t &device, const char *tag)
{
	device_isa8_card_interface &isa_card = dynamic_cast<device_isa8_card_interface &>(device);
	isa_card.m_isa_tag = tag;
}

void device_isa8_card_interface::static_set_isa8_num(device_t &device, int num)
{
	device_isa8_card_interface &isa_card = dynamic_cast<device_isa8_card_interface &>(device);
	isa_card.m_isa_num = num;
}


//**************************************************************************
//  DEVICE ISA8 CARD INTERFACE
//**************************************************************************

//-------------------------------------------------
//  device_isa8_card_interface - constructor
//-------------------------------------------------

device_isa8_card_interface::device_isa8_card_interface(const machine_config &mconfig, device_t &device)
	: device_interface(device)
{
}


//-------------------------------------------------
//  ~device_isa8_card_interface - destructor
//-------------------------------------------------

device_isa8_card_interface::~device_isa8_card_interface()
{
}

UINT8 device_isa8_card_interface::dack_r(int line)
{
	return 0;
}
void device_isa8_card_interface::dack_w(int line,UINT8 data)
{
}
void device_isa8_card_interface::eop_w(int state)
{
}

bool device_isa8_card_interface::have_dack(int line)
{
	return FALSE;
}
