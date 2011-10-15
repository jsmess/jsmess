/**********************************************************************

    Luxor ABC 1600 Expansion Bus emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "abc1600_bus.h"



//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type ABC1600BUS_SLOT = &device_creator<abc1600bus_slot_device>;



//**************************************************************************
//  DEVICE ABC1600BUS CARD INTERFACE
//**************************************************************************

//-------------------------------------------------
//  device_abc1600bus_card_interface - constructor
//-------------------------------------------------

device_abc1600bus_card_interface::device_abc1600bus_card_interface(const machine_config &mconfig, device_t &device)
	: device_interface(device)
{
}


//-------------------------------------------------
//  ~device_abc1600bus_card_interface - destructor
//-------------------------------------------------

device_abc1600bus_card_interface::~device_abc1600bus_card_interface()
{
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  abc1600bus_slot_device - constructor
//-------------------------------------------------

abc1600bus_slot_device::abc1600bus_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, ABC1600BUS_SLOT, "ABC 1600 bus slot", tag, owner, clock),
		device_slot_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void abc1600bus_slot_device::device_config_complete()
{
	// inherit a copy of the static data
	const abc1600bus_interface *intf = reinterpret_cast<const abc1600bus_interface *>(static_config());
	if (intf != NULL)
	{
		*static_cast<abc1600bus_interface *>(this) = *intf;
	}

	// or initialize to defaults if none provided
	else
	{
    	memset(&m_out_int_cb, 0, sizeof(m_out_int_cb));
    	memset(&m_out_pren_cb, 0, sizeof(m_out_pren_cb));
    	memset(&m_out_trrq_cb, 0, sizeof(m_out_trrq_cb));
    	memset(&m_out_nmi_cb, 0, sizeof(m_out_nmi_cb));
    	memset(&m_out_xint2_cb, 0, sizeof(m_out_xint2_cb));
    	memset(&m_out_xint3_cb, 0, sizeof(m_out_xint3_cb));
    	memset(&m_out_xint4_cb, 0, sizeof(m_out_xint4_cb));
    	memset(&m_out_xint5_cb, 0, sizeof(m_out_xint5_cb));
 	}
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void abc1600bus_slot_device::device_start()
{
	// resolve callbacks
	m_out_int_func.resolve(m_out_int_cb, *this);
	m_out_pren_func.resolve(m_out_pren_cb, *this);
	m_out_trrq_func.resolve(m_out_trrq_cb, *this);
	m_out_nmi_func.resolve(m_out_nmi_cb, *this);
	m_out_xint2_func.resolve(m_out_xint2_cb, *this);
	m_out_xint3_func.resolve(m_out_xint3_cb, *this);
	m_out_xint4_func.resolve(m_out_xint4_cb, *this);
	m_out_xint5_func.resolve(m_out_xint5_cb, *this);
}


//-------------------------------------------------
//  cs_w -
//-------------------------------------------------

WRITE8_MEMBER( abc1600bus_slot_device::cs_w )
{
	if (m_card != NULL)
	{
		m_card->abc1600bus_cs(data);
	}
}


//-------------------------------------------------
//  rst_r -
//-------------------------------------------------

READ8_MEMBER( abc1600bus_slot_device::rst_r )
{
	if (m_card != NULL)
	{
		m_card->abc1600bus_rst(0);
		m_card->abc1600bus_rst(1);
	}

	return 0xff;
}


//-------------------------------------------------
//  inp_r -
//-------------------------------------------------

READ8_MEMBER( abc1600bus_slot_device::inp_r )
{
	UINT8 data = 0xff;

	if (m_card != NULL)
	{
		data &= m_card->abc1600bus_inp();
	}

	return data;
}


//-------------------------------------------------
//  utp_w -
//-------------------------------------------------

WRITE8_MEMBER( abc1600bus_slot_device::out_w )
{
	if (m_card != NULL)
	{
		m_card->abc1600bus_out(data);
	}
}


//-------------------------------------------------
//  stat_r -
//-------------------------------------------------

READ8_MEMBER( abc1600bus_slot_device::stat_r )
{
	UINT8 data = 0xff;

	if (m_card != NULL)
	{
		data &= m_card->abc1600bus_stat();
	}

	return data;
}


//-------------------------------------------------
//  stat_r -
//-------------------------------------------------

READ8_MEMBER( abc1600bus_slot_device::ops_r )
{
	UINT8 data = 0xff;

	if (m_card != NULL)
	{
		data &= m_card->abc1600bus_ops();
	}

	return data;
}


//-------------------------------------------------
//  c1_w -
//-------------------------------------------------

WRITE8_MEMBER( abc1600bus_slot_device::c1_w )
{
	if (m_card != NULL)
	{
		m_card->abc1600bus_c1(data);
	}
}


//-------------------------------------------------
//  c2_w -
//-------------------------------------------------

WRITE8_MEMBER( abc1600bus_slot_device::c2_w )
{
	if (m_card != NULL)
	{
		m_card->abc1600bus_c2(data);
	}
}


//-------------------------------------------------
//  c3_w -
//-------------------------------------------------

WRITE8_MEMBER( abc1600bus_slot_device::c3_w )
{
	if (m_card != NULL)
	{
		m_card->abc1600bus_c3(data);
	}
}


//-------------------------------------------------
//  c4_w -
//-------------------------------------------------

WRITE8_MEMBER( abc1600bus_slot_device::c4_w )
{
	if (m_card != NULL)
	{
		m_card->abc1600bus_c4(data);
	}
}


//-------------------------------------------------
//  tren_w - transfer enable
//-------------------------------------------------

WRITE_LINE_MEMBER( abc1600bus_slot_device::tren_w )
{
	if (m_card != NULL)
	{
		m_card->abc1600bus_tren(state);
	}
}


//-------------------------------------------------
//  prac_w - peripheral acknowledge
//-------------------------------------------------

WRITE_LINE_MEMBER( abc1600bus_slot_device::prac_w )
{
	if (m_card != NULL)
	{
		m_card->abc1600bus_prac(state);
	}
}


//-------------------------------------------------
//  int_w - interrupt request
//-------------------------------------------------

WRITE_LINE_MEMBER( abc1600bus_slot_device::int_w )
{
	m_out_int_func(state);
}


//-------------------------------------------------
//  pren_w - peripheral enable
//-------------------------------------------------

WRITE_LINE_MEMBER( abc1600bus_slot_device::pren_w )
{
	m_out_pren_func(state);
}


//-------------------------------------------------
//  trrq_w - transfer request
//-------------------------------------------------

WRITE_LINE_MEMBER( abc1600bus_slot_device::trrq_w )
{
	m_out_trrq_func(state);
}


//-------------------------------------------------
//  nmi_w - non-maskable interrupt
//-------------------------------------------------

WRITE_LINE_MEMBER( abc1600bus_slot_device::nmi_w )
{
	m_out_nmi_func(state);
}


//-------------------------------------------------
//  xint2_w - expansion interrupt
//-------------------------------------------------

WRITE_LINE_MEMBER( abc1600bus_slot_device::xint2_w )
{
	m_out_xint2_func(state);
}


//-------------------------------------------------
//  xint3_w - expansion interrupt
//-------------------------------------------------

WRITE_LINE_MEMBER( abc1600bus_slot_device::xint3_w )
{
	m_out_xint3_func(state);
}


//-------------------------------------------------
//  xint4_w - expansion interrupt
//-------------------------------------------------

WRITE_LINE_MEMBER( abc1600bus_slot_device::xint4_w )
{
	m_out_xint4_func(state);
}


//-------------------------------------------------
//  xint5_w - expansion interrupt
//-------------------------------------------------

WRITE_LINE_MEMBER( abc1600bus_slot_device::xint5_w )
{
	m_out_xint5_func(state);
}
