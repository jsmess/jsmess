/***************************************************************************

  a2bus.c - Apple II slot bus and card emulation

  by R. Belmont

***************************************************************************/

#include "emu.h"
#include "emuopts.h"
#include "machine/a2bus.h"


//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type A2BUS_SLOT = &device_creator<a2bus_slot_device>;

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  a2bus_slot_device - constructor
//-------------------------------------------------
a2bus_slot_device::a2bus_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, A2BUS_SLOT, "A2BUS_SLOT", tag, owner, clock),
		device_slot_interface(mconfig, *this)
{
}

a2bus_slot_device::a2bus_slot_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, type, name, tag, owner, clock),
		device_slot_interface(mconfig, *this)
{
}

void a2bus_slot_device::static_set_a2bus_slot(device_t &device, const char *tag, const char *slottag)
{
	a2bus_slot_device &a2bus_card = dynamic_cast<a2bus_slot_device &>(device);
	a2bus_card.m_a2bus_tag = tag;
	a2bus_card.m_a2bus_slottag = slottag;
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void a2bus_slot_device::device_start()
{
	device_a2bus_card_interface *dev = dynamic_cast<device_a2bus_card_interface *>(get_card_device());

	if (dev) device_a2bus_card_interface::static_set_a2bus_tag(*dev, m_a2bus_tag, m_a2bus_slottag);
}

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type A2BUS = &device_creator<a2bus_device>;

void a2bus_device::static_set_cputag(device_t &device, const char *tag)
{
	a2bus_device &a2bus = downcast<a2bus_device &>(device);
	a2bus.m_cputag = tag;
}

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void a2bus_device::device_config_complete()
{
	// inherit a copy of the static data
	const a2bus_interface *intf = reinterpret_cast<const a2bus_interface *>(static_config());
	if (intf != NULL)
	{
		*static_cast<a2bus_interface *>(this) = *intf;
	}

	// or initialize to defaults if none provided
	else
	{
    	memset(&m_out_irq_cb, 0, sizeof(m_out_irq_cb));
    	memset(&m_out_nmi_cb, 0, sizeof(m_out_nmi_cb));
	}
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  a2bus_device - constructor
//-------------------------------------------------

a2bus_device::a2bus_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, A2BUS, "A2BUS", tag, owner, clock)
{
}

a2bus_device::a2bus_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, type, name, tag, owner, clock)
{
}
//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void a2bus_device::device_start()
{
	m_maincpu = machine().device(m_cputag);
	// resolve callbacks
	m_out_irq_func.resolve(m_out_irq_cb, *this);
	m_out_irq_func.resolve(m_out_nmi_cb, *this);
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void a2bus_device::device_reset()
{
}

void a2bus_device::add_a2bus_card(device_a2bus_card_interface *card)
{
	m_device_list.append(*card);
}

void a2bus_device::set_irq_line(int state)
{
    m_out_irq_func(state);
}

void a2bus_device::set_nmi_line(int state)
{
    m_out_nmi_func(state);
}

// interrupt request from a2bus card
WRITE_LINE_MEMBER( a2bus_device::irq_w ) { m_out_irq_func(state); }
WRITE_LINE_MEMBER( a2bus_device::nmi_w ) { m_out_nmi_func(state); }

//**************************************************************************
//  DEVICE CONFIG A2BUS CARD INTERFACE
//**************************************************************************


//**************************************************************************
//  DEVICE A2BUS CARD INTERFACE
//**************************************************************************

//-------------------------------------------------
//  device_a2bus_card_interface - constructor
//-------------------------------------------------

device_a2bus_card_interface::device_a2bus_card_interface(const machine_config &mconfig, device_t &device)
	: device_slot_card_interface(mconfig, device),
	  m_a2bus(NULL),
	  m_a2bus_tag(NULL)
{
}


//-------------------------------------------------
//  ~device_a2bus_card_interface - destructor
//-------------------------------------------------

device_a2bus_card_interface::~device_a2bus_card_interface()
{
}

void device_a2bus_card_interface::static_set_a2bus_tag(device_t &device, const char *tag, const char *slottag)
{
	device_a2bus_card_interface &a2bus_card = dynamic_cast<device_a2bus_card_interface &>(device);
	a2bus_card.m_a2bus_tag = tag;
	a2bus_card.m_a2bus_slottag = slottag;
}

void device_a2bus_card_interface::set_a2bus_device()
{
	// extract the slot number from the last digit of the slot tag
	int tlen = strlen(m_a2bus_slottag);

	m_slot = (m_a2bus_slottag[tlen-1] - '0');

	if (m_slot < 0 || m_slot > 7)
	{
		fatalerror("Slot %x out of range for Apple II Bus\n", m_slot);
	}

	m_a2bus = dynamic_cast<a2bus_device *>(device().machine().device(m_a2bus_tag));
	m_a2bus->add_a2bus_card(this);
}


