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
//  DEVICE ABCBUS CARD INTERFACE
//**************************************************************************

//-------------------------------------------------
//  device_abcbus_card_interface - constructor
//-------------------------------------------------

device_abcbus_card_interface::device_abcbus_card_interface(const machine_config &mconfig, device_t &device)
	: device_slot_card_interface(mconfig, device)
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
//  abcbus_slot_device - constructor
//-------------------------------------------------

abcbus_slot_device::abcbus_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, ABCBUS_SLOT, "ABC bus slot", tag, owner, clock),
		device_slot_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void abcbus_slot_device::device_config_complete()
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


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void abcbus_slot_device::device_start()
{
	m_card = dynamic_cast<device_abcbus_card_interface *>(get_card_device());

	// resolve callbacks
	m_out_int_func.resolve(m_out_int_cb, *this);
	m_out_nmi_func.resolve(m_out_nmi_cb, *this);
	m_out_rdy_func.resolve(m_out_rdy_cb, *this);
	m_out_resin_func.resolve(m_out_resin_cb, *this);
}


//-------------------------------------------------
//  cs_w -
//-------------------------------------------------

WRITE8_MEMBER( abcbus_slot_device::cs_w )
{
	if (m_card != NULL)
	{
		m_card->abcbus_cs(data);
	}
}


//-------------------------------------------------
//  rst_r -
//-------------------------------------------------

READ8_MEMBER( abcbus_slot_device::rst_r )
{
	if (m_card != NULL)
	{
		m_card->abcbus_rst(0);
		m_card->abcbus_rst(1);
	}

	return 0xff;
}


//-------------------------------------------------
//  inp_r -
//-------------------------------------------------

READ8_MEMBER( abcbus_slot_device::inp_r )
{
	UINT8 data = 0xff;

	if (m_card != NULL)
	{
		data &= m_card->abcbus_inp();
	}

	return data;
}


//-------------------------------------------------
//  utp_w -
//-------------------------------------------------

WRITE8_MEMBER( abcbus_slot_device::utp_w )
{
	if (m_card != NULL)
	{
		m_card->abcbus_utp(data);
	}
}


//-------------------------------------------------
//  stat_r -
//-------------------------------------------------

READ8_MEMBER( abcbus_slot_device::stat_r )
{
	UINT8 data = 0xff;

	if (m_card != NULL)
	{
		data &= m_card->abcbus_stat();
	}

	return data;
}


//-------------------------------------------------
//  c1_w -
//-------------------------------------------------

WRITE8_MEMBER( abcbus_slot_device::c1_w )
{
	if (m_card != NULL)
	{
		m_card->abcbus_c1(data);
	}
}


//-------------------------------------------------
//  c2_w -
//-------------------------------------------------

WRITE8_MEMBER( abcbus_slot_device::c2_w )
{
	if (m_card != NULL)
	{
		m_card->abcbus_c2(data);
	}
}


//-------------------------------------------------
//  c3_w -
//-------------------------------------------------

WRITE8_MEMBER( abcbus_slot_device::c3_w )
{
	if (m_card != NULL)
	{
		m_card->abcbus_c3(data);
	}
}


//-------------------------------------------------
//  c4_w -
//-------------------------------------------------

WRITE8_MEMBER( abcbus_slot_device::c4_w )
{
	if (m_card != NULL)
	{
		m_card->abcbus_c4(data);
	}
}


//-------------------------------------------------
//  xmemfl_r -
//-------------------------------------------------

READ8_MEMBER( abcbus_slot_device::xmemfl_r )
{
	UINT8 data = 0xff;

	if (m_card != NULL)
	{
		data &= m_card->abcbus_xmemfl(offset);
	}

	return data;
}


//-------------------------------------------------
//  xmemw_w -
//-------------------------------------------------

WRITE8_MEMBER( abcbus_slot_device::xmemw_w )
{
	if (m_card != NULL)
	{
		m_card->abcbus_xmemw(offset, data);
	}
}


//-------------------------------------------------
//  int_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( abcbus_slot_device::int_w )
{
	m_out_int_func(state);
}


//-------------------------------------------------
//  nmi_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( abcbus_slot_device::nmi_w )
{
	m_out_nmi_func(state);
}


//-------------------------------------------------
//  rdy_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( abcbus_slot_device::rdy_w )
{
	m_out_rdy_func(state);
}


//-------------------------------------------------
//  resin_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( abcbus_slot_device::resin_w )
{
	m_out_resin_func(state);
}
