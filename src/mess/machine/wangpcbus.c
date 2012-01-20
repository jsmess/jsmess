/**********************************************************************

    Wang Professional Computer bus emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "emu.h"
#include "emuopts.h"
#include "machine/wangpcbus.h"


//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type WANGPC_BUS_SLOT = &device_creator<wangpcbus_slot_device>;



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  wangpcbus_slot_device - constructor
//-------------------------------------------------

wangpcbus_slot_device::wangpcbus_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, WANGPC_BUS_SLOT, "Wang PC bus slot", tag, owner, clock),
		device_slot_interface(mconfig, *this)
{
}

void wangpcbus_slot_device::static_set_wangpcbus_slot(device_t &device, int sid)
{
	wangpcbus_slot_device &wangpcbus_card = dynamic_cast<wangpcbus_slot_device &>(device);
	wangpcbus_card.m_sid = sid;
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void wangpcbus_slot_device::device_start()
{
	m_bus = machine().device<wangpcbus_device>(WANGPC_BUS_TAG);
	device_wangpcbus_card_interface *dev = dynamic_cast<device_wangpcbus_card_interface *>(get_card_device());
	if (dev) m_bus->add_wangpcbus_card(dev);
}



//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type WANGPC_BUS = &device_creator<wangpcbus_device>;


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void wangpcbus_device::device_config_complete()
{
	// inherit a copy of the static data
	const wangpcbus_interface *intf = reinterpret_cast<const wangpcbus_interface *>(static_config());
	if (intf != NULL)
	{
		*static_cast<wangpcbus_interface *>(this) = *intf;
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
    	memset(&m_out_ioerror_cb, 0, sizeof(m_out_ioerror_cb));
	}
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  wangpcbus_device - constructor
//-------------------------------------------------

wangpcbus_device::wangpcbus_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, WANGPC_BUS, "Wang PC bus", tag, owner, clock)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void wangpcbus_device::device_start()
{
	// resolve callbacks
	m_out_irq2_func.resolve(m_out_irq2_cb, *this);
	m_out_irq3_func.resolve(m_out_irq3_cb, *this);
	m_out_irq4_func.resolve(m_out_irq4_cb, *this);
	m_out_irq5_func.resolve(m_out_irq5_cb, *this);
	m_out_irq6_func.resolve(m_out_irq6_cb, *this);
	m_out_irq7_func.resolve(m_out_irq7_cb, *this);
	m_out_drq1_func.resolve(m_out_drq1_cb, *this);
	m_out_drq2_func.resolve(m_out_drq2_cb, *this);
	m_out_drq3_func.resolve(m_out_drq3_cb, *this);
	m_out_ioerror_func.resolve(m_out_ioerror_cb, *this);
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void wangpcbus_device::device_reset()
{
}


//-------------------------------------------------
//  add_wangpcbus_card - add S100 card
//-------------------------------------------------

void wangpcbus_device::add_wangpcbus_card(device_wangpcbus_card_interface *card)
{
	m_device_list.append(*card);
}


//-------------------------------------------------
//  mrdc_r - memory read
//-------------------------------------------------

READ8_MEMBER( wangpcbus_device::mrdc_r )
{
	UINT8 data = 0;

	device_wangpcbus_card_interface *entry = m_device_list.first();

	while (entry)
	{
		data |= entry->wangpcbus_mrdc_r(offset);
		entry = entry->next();
	}

	return data;
}


//-------------------------------------------------
//  amwc_w - memory write
//-------------------------------------------------

WRITE8_MEMBER( wangpcbus_device::amwc_w )
{
	device_wangpcbus_card_interface *entry = m_device_list.first();

	while (entry)
	{
		entry->wangpcbus_amwc_w(offset, data);
		entry = entry->next();
	}
}


//-------------------------------------------------
//  iorc_r - I/O read
//-------------------------------------------------

READ8_MEMBER( wangpcbus_device::iorc_r )
{
	UINT8 data = 0;

	device_wangpcbus_card_interface *entry = m_device_list.first();

	while (entry)
	{
		data |= entry->wangpcbus_iorc_r(offset);
		entry = entry->next();
	}

	return data;
}


//-------------------------------------------------
//  aiowc_w - I/O write
//-------------------------------------------------

WRITE8_MEMBER( wangpcbus_device::aiowc_w )
{
	device_wangpcbus_card_interface *entry = m_device_list.first();

	while (entry)
	{
		entry->wangpcbus_aiowc_w(offset, data);
		entry = entry->next();
	}
}


WRITE_LINE_MEMBER( wangpcbus_device::irq2_w ) { m_out_irq2_func(state); }
WRITE_LINE_MEMBER( wangpcbus_device::irq3_w ) { m_out_irq3_func(state); }
WRITE_LINE_MEMBER( wangpcbus_device::irq4_w ) { m_out_irq4_func(state); }
WRITE_LINE_MEMBER( wangpcbus_device::irq5_w ) { m_out_irq5_func(state); }
WRITE_LINE_MEMBER( wangpcbus_device::irq6_w ) { m_out_irq6_func(state); }
WRITE_LINE_MEMBER( wangpcbus_device::irq7_w ) { m_out_irq7_func(state); }
WRITE_LINE_MEMBER( wangpcbus_device::drq1_w ) { m_out_drq1_func(state); }
WRITE_LINE_MEMBER( wangpcbus_device::drq2_w ) { m_out_drq2_func(state); }
WRITE_LINE_MEMBER( wangpcbus_device::drq3_w ) { m_out_drq3_func(state); }
WRITE_LINE_MEMBER( wangpcbus_device::ioerror_w ) { m_out_ioerror_func(state); }


//-------------------------------------------------
//  dack_r - DMA read
//-------------------------------------------------

UINT8 wangpcbus_device::dack_r(int line)
{
	UINT8 retVal = 0xff;
	device_wangpcbus_card_interface *entry = m_device_list.first();
	
	while (entry)
	{
		if (entry->wangpcbus_have_dack(line)) 
		{
			retVal = entry->wangpcbus_dack_r(line);
			break;
		}
		
		entry = entry->next();
	}
	
	return retVal;
}


//-------------------------------------------------
//  dack_w - DMA write
//-------------------------------------------------

void wangpcbus_device::dack_w(int line,UINT8 data)
{
	device_wangpcbus_card_interface *entry = m_device_list.first();
	
	while (entry)
	{
		if (entry->wangpcbus_have_dack(line))
		{
			entry->wangpcbus_dack_w(line,data);
		}
		
		entry = entry->next();
	}
}


//-------------------------------------------------
//  tc_w - terminal count
//-------------------------------------------------

void wangpcbus_device::tc_w(int state)
{
	device_wangpcbus_card_interface *entry = m_device_list.first();
	
	while (entry)
	{
		entry->wangpcbus_tc_w(state);
		entry = entry->next();
	}
}


//**************************************************************************
//  DEVICE S100 CARD INTERFACE
//**************************************************************************

//-------------------------------------------------
//  device_wangpcbus_card_interface - constructor
//-------------------------------------------------

device_wangpcbus_card_interface::device_wangpcbus_card_interface(const machine_config &mconfig, device_t &device)
	: device_slot_card_interface(mconfig, device)
{
}


//-------------------------------------------------
//  ~device_wangpcbus_card_interface - destructor
//-------------------------------------------------

device_wangpcbus_card_interface::~device_wangpcbus_card_interface()
{
}
