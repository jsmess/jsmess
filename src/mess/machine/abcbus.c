/**********************************************************************

    Luxor ABC (Databoard 4680) Bus emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "abcbus.h"



//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type ABCBUS_SLOT = &device_creator<abcbus_slot_device>;



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  abcbus_slot_device - constructor
//-------------------------------------------------

abcbus_slot_device::abcbus_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, ABCBUS_SLOT, "ABC bus slot", tag, owner, clock),
		device_slot_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  static_set_abcbus_slot - 
//-------------------------------------------------

void abcbus_slot_device::static_set_abcbus_slot(device_t &device, const char *tag, int num)
{
	abcbus_slot_device &abcbus_card = dynamic_cast<abcbus_slot_device &>(device);
	abcbus_card.m_bus_tag = tag;
	abcbus_card.m_bus_num = num;
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void abcbus_slot_device::device_start()
{
	m_bus = machine().device<abcbus_device>(m_bus_tag);
	device_abcbus_card_interface *dev = dynamic_cast<device_abcbus_card_interface *>(get_card_device());
	if (dev) m_bus->add_abcbus_card(dev, m_bus_num);
}



//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type ABCBUS = &device_creator<abcbus_device>;


void abcbus_device::static_set_cputag(device_t &device, const char *tag)
{
	abcbus_device &abcbus = downcast<abcbus_device &>(device);
	abcbus.m_cputag = tag;
}


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void abcbus_device::device_config_complete()
{
	// inherit a copy of the static data
	const abcbus_interface *intf = reinterpret_cast<const abcbus_interface *>(static_config());
	if (intf != NULL)
	{
		*static_cast<abcbus_interface *>(this) = *intf;
	}

	// or initialize to defaults if none provided
	else
	{
    	memset(&m_out_int_cb, 0, sizeof(m_out_int_cb));
    	memset(&m_out_nmi_cb, 0, sizeof(m_out_nmi_cb));
    	memset(&m_out_rdy_cb, 0, sizeof(m_out_rdy_cb));
    	memset(&m_out_resin_cb, 0, sizeof(m_out_resin_cb));
	}
}



//**************************************************************************
//  DEVICE ABCBUS CARD INTERFACE
//**************************************************************************

//-------------------------------------------------
//  device_abcbus_card_interface - constructor
//-------------------------------------------------

device_abcbus_card_interface::device_abcbus_card_interface(const machine_config &mconfig, device_t &device)
	: device_interface(device)
{
}


//-------------------------------------------------
//  ~device_abcbus_card_interface - destructor
//-------------------------------------------------

device_abcbus_card_interface::~device_abcbus_card_interface()
{
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  abcbus_device - constructor
//-------------------------------------------------

abcbus_device::abcbus_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, ABCBUS, "ABC bus", tag, owner, clock)
{
	for (int i = 0; i < MAX_ABCBUS_SLOTS; i++)
		m_abcbus_device[i] = NULL;
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void abcbus_device::device_start()
{
	m_maincpu = machine().device(m_cputag);

	// resolve callbacks
	m_out_int_func.resolve(m_out_int_cb, *this);
	m_out_nmi_func.resolve(m_out_nmi_cb, *this);
	m_out_rdy_func.resolve(m_out_rdy_cb, *this);
	m_out_resin_func.resolve(m_out_resin_cb, *this);
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void abcbus_device::device_reset()
{
}


//-------------------------------------------------
//  add_abcbus_card - add ABC bus card
//-------------------------------------------------

void abcbus_device::add_abcbus_card(device_abcbus_card_interface *card, int pos)
{
	m_abcbus_device[pos] = card;
}

	
//-------------------------------------------------
//  cs_w -
//-------------------------------------------------

WRITE8_MEMBER( abcbus_device::cs_w )
{
	for (int i = 0; i < MAX_ABCBUS_SLOTS; i++)
	{
		if (m_abcbus_device[i] != NULL)
		{
			m_abcbus_device[i]->abcbus_cs(data);
		}
	}
}


//-------------------------------------------------
//  rst_r -
//-------------------------------------------------

READ8_MEMBER( abcbus_device::rst_r )
{
	for (int i = 0; i < MAX_ABCBUS_SLOTS; i++)
	{
		if (m_abcbus_device[i] != NULL)
		{
			m_abcbus_device[i]->abcbus_rst(0);
			m_abcbus_device[i]->abcbus_rst(1);
		}
	}

	return 0xff;
}


//-------------------------------------------------
//  inp_r -
//-------------------------------------------------

READ8_MEMBER( abcbus_device::inp_r )
{
	UINT8 data = 0xff;

	for (int i = 0; i < MAX_ABCBUS_SLOTS; i++)
	{
		if (m_abcbus_device[i] != NULL)
		{
			data &= m_abcbus_device[i]->abcbus_inp();
		}
	}

	return data;
}


//-------------------------------------------------
//  utp_w -
//-------------------------------------------------

WRITE8_MEMBER( abcbus_device::utp_w )
{
	for (int i = 0; i < MAX_ABCBUS_SLOTS; i++)
	{
		if (m_abcbus_device[i] != NULL)
		{
			m_abcbus_device[i]->abcbus_utp(data);
		}
	}
}


//-------------------------------------------------
//  stat_r -
//-------------------------------------------------

READ8_MEMBER( abcbus_device::stat_r )
{
	UINT8 data = 0xff;

	for (int i = 0; i < MAX_ABCBUS_SLOTS; i++)
	{
		if (m_abcbus_device[i] != NULL)
		{
			data &= m_abcbus_device[i]->abcbus_stat();
		}
	}

	return data;
}


//-------------------------------------------------
//  c1_w -
//-------------------------------------------------

WRITE8_MEMBER( abcbus_device::c1_w )
{
	for (int i = 0; i < MAX_ABCBUS_SLOTS; i++)
	{
		if (m_abcbus_device[i] != NULL)
		{
			m_abcbus_device[i]->abcbus_c1(data);
		}
	}
}


//-------------------------------------------------
//  c2_w -
//-------------------------------------------------

WRITE8_MEMBER( abcbus_device::c2_w )
{
	for (int i = 0; i < MAX_ABCBUS_SLOTS; i++)
	{
		if (m_abcbus_device[i] != NULL)
		{
			m_abcbus_device[i]->abcbus_c2(data);
		}
	}
}


//-------------------------------------------------
//  c3_w -
//-------------------------------------------------

WRITE8_MEMBER( abcbus_device::c3_w )
{
	for (int i = 0; i < MAX_ABCBUS_SLOTS; i++)
	{
		if (m_abcbus_device[i] != NULL)
		{
			m_abcbus_device[i]->abcbus_c3(data);
		}
	}
}


//-------------------------------------------------
//  c4_w -
//-------------------------------------------------

WRITE8_MEMBER( abcbus_device::c4_w )
{
	for (int i = 0; i < MAX_ABCBUS_SLOTS; i++)
	{
		if (m_abcbus_device[i] != NULL)
		{
			m_abcbus_device[i]->abcbus_c4(data);
		}
	}
}


//-------------------------------------------------
//  int_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( abcbus_device::int_w )
{
	m_out_int_func(state);
}


//-------------------------------------------------
//  nmi_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( abcbus_device::nmi_w )
{
	m_out_nmi_func(state);
}


//-------------------------------------------------
//  rdy_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( abcbus_device::rdy_w )
{
	m_out_rdy_func(state);
}


//-------------------------------------------------
//  resin_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( abcbus_device::resin_w )
{
	m_out_resin_func(state);
}
