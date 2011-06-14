/**********************************************************************

	S-100 (IEEE-696/1983) bus emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "emu.h"
#include "emuopts.h"
#include "machine/s100.h"


//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type S100_SLOT = &device_creator<s100_slot_device>;

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  s100_slot_device - constructor
//-------------------------------------------------
s100_slot_device::s100_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, S100_SLOT, "S100 slot", tag, owner, clock),
		device_slot_interface(mconfig, *this)
{
}

void s100_slot_device::static_set_s100_slot(device_t &device, const char *tag, int num)
{
	s100_slot_device &s100_card = dynamic_cast<s100_slot_device &>(device);
	s100_card.m_bus_tag = tag;
	s100_card.m_bus_num = num;
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void s100_slot_device::device_start()
{
	m_bus = machine().device<s100_device>(m_bus_tag);
	device_s100_card_interface *dev = dynamic_cast<device_s100_card_interface *>(get_card_device());
	if (dev) m_bus->add_s100_card(dev, m_bus_num);
}



//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type S100 = &device_creator<s100_device>;


void s100_device::static_set_cputag(device_t &device, const char *tag)
{
	s100_device &s100 = downcast<s100_device &>(device);
	s100.m_cputag = tag;
}


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void s100_device::device_config_complete()
{
	// inherit a copy of the static data
	const s100_bus_interface *intf = reinterpret_cast<const s100_bus_interface *>(static_config());
	if (intf != NULL)
	{
		*static_cast<s100_bus_interface *>(this) = *intf;
	}

	// or initialize to defaults if none provided
	else
	{
    	memset(&m_out_int_cb, 0, sizeof(m_out_int_cb));
    	memset(&m_out_nmi_cb, 0, sizeof(m_out_nmi_cb));
    	memset(&m_out_vi0_cb, 0, sizeof(m_out_vi0_cb));
    	memset(&m_out_vi1_cb, 0, sizeof(m_out_vi1_cb));
    	memset(&m_out_vi2_cb, 0, sizeof(m_out_vi2_cb));
    	memset(&m_out_vi3_cb, 0, sizeof(m_out_vi3_cb));
    	memset(&m_out_vi4_cb, 0, sizeof(m_out_vi4_cb));
    	memset(&m_out_vi5_cb, 0, sizeof(m_out_vi5_cb));
    	memset(&m_out_vi6_cb, 0, sizeof(m_out_vi6_cb));
    	memset(&m_out_vi7_cb, 0, sizeof(m_out_vi7_cb));
    	memset(&m_out_dma0_cb, 0, sizeof(m_out_dma0_cb));
    	memset(&m_out_dma1_cb, 0, sizeof(m_out_dma1_cb));
    	memset(&m_out_dma2_cb, 0, sizeof(m_out_dma2_cb));
    	memset(&m_out_dma3_cb, 0, sizeof(m_out_dma3_cb));
    	memset(&m_out_rdy_cb, 0, sizeof(m_out_rdy_cb));
    	memset(&m_out_hold_cb, 0, sizeof(m_out_hold_cb));
    	memset(&m_out_error_cb, 0, sizeof(m_out_error_cb));
	}
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  s100_device - constructor
//-------------------------------------------------

s100_device::s100_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, S100, "S100", tag, owner, clock)
{
	for (int i = 0; i < MAX_S100_SLOTS; i++)
		m_s100_device[i] = NULL;
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void s100_device::device_start()
{
	m_maincpu = machine().device(m_cputag);

	// resolve callbacks
	m_out_int_func.resolve(m_out_int_cb, *this);
	m_out_nmi_func.resolve(m_out_nmi_cb, *this);
	m_out_vi0_func.resolve(m_out_vi0_cb, *this);
	m_out_vi1_func.resolve(m_out_vi1_cb, *this);
	m_out_vi2_func.resolve(m_out_vi2_cb, *this);
	m_out_vi3_func.resolve(m_out_vi3_cb, *this);
	m_out_vi4_func.resolve(m_out_vi4_cb, *this);
	m_out_vi5_func.resolve(m_out_vi5_cb, *this);
	m_out_vi6_func.resolve(m_out_vi6_cb, *this);
	m_out_vi7_func.resolve(m_out_vi7_cb, *this);
	m_out_dma0_func.resolve(m_out_dma0_cb, *this);
	m_out_dma1_func.resolve(m_out_dma1_cb, *this);
	m_out_dma2_func.resolve(m_out_dma2_cb, *this);
	m_out_dma3_func.resolve(m_out_dma3_cb, *this);
	m_out_rdy_func.resolve(m_out_rdy_cb, *this);
	m_out_hold_func.resolve(m_out_hold_cb, *this);
	m_out_error_func.resolve(m_out_error_cb, *this);
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void s100_device::device_reset()
{
}


//-------------------------------------------------
//  add_s100_card - add S100 card
//-------------------------------------------------

void s100_device::add_s100_card(device_s100_card_interface *card, int pos)
{
	m_s100_device[pos] = card;
}


//-------------------------------------------------
//  smemr_r - memory read
//-------------------------------------------------

READ8_MEMBER( s100_device::smemr_r )
{
	UINT8 data = 0;
	
	for (int i = 0; i < MAX_S100_SLOTS; i++)
	{
		if (m_s100_device[i] != NULL)
		{
			data |= m_s100_device[i]->s100_smemr_r(offset);
		}
	}
	
	return data;
}


//-------------------------------------------------
//  mwrt_w - memory write
//-------------------------------------------------

WRITE8_MEMBER( s100_device::mwrt_w )
{
	for (int i = 0; i < MAX_S100_SLOTS; i++)
	{
		if (m_s100_device[i] != NULL)
		{
			m_s100_device[i]->s100_mwrt_w(offset, data);
		}
	}
}


//-------------------------------------------------
//  sinp_r - I/O read
//-------------------------------------------------

READ8_MEMBER( s100_device::sinp_r )
{
	UINT8 data = 0;
	
	for (int i = 0; i < MAX_S100_SLOTS; i++)
	{
		if (m_s100_device[i] != NULL)
		{
			data |= m_s100_device[i]->s100_sinp_r(offset);
		}
	}
	
	return data;
}


//-------------------------------------------------
//  sout_w - I/O write
//-------------------------------------------------

WRITE8_MEMBER( s100_device::sout_w )
{
	for (int i = 0; i < MAX_S100_SLOTS; i++)
	{
		if (m_s100_device[i] != NULL)
		{
			m_s100_device[i]->s100_sout_w(offset, data);
		}
	}
}


WRITE_LINE_MEMBER( s100_device::int_w ) { m_out_nmi_func(state); }
WRITE_LINE_MEMBER( s100_device::nmi_w ) { m_out_nmi_func(state); }
WRITE_LINE_MEMBER( s100_device::vi0_w ) { m_out_vi0_func(state); }
WRITE_LINE_MEMBER( s100_device::vi1_w ) { m_out_vi1_func(state); }
WRITE_LINE_MEMBER( s100_device::vi2_w ) { m_out_vi2_func(state); }
WRITE_LINE_MEMBER( s100_device::vi3_w ) { m_out_vi3_func(state); }
WRITE_LINE_MEMBER( s100_device::vi4_w ) { m_out_vi4_func(state); }
WRITE_LINE_MEMBER( s100_device::vi5_w ) { m_out_vi5_func(state); }
WRITE_LINE_MEMBER( s100_device::vi6_w ) { m_out_vi6_func(state); }
WRITE_LINE_MEMBER( s100_device::vi7_w ) { m_out_vi7_func(state); }
WRITE_LINE_MEMBER( s100_device::dma0_w ) { m_out_dma0_func(state); }
WRITE_LINE_MEMBER( s100_device::dma1_w ) { m_out_dma1_func(state); }
WRITE_LINE_MEMBER( s100_device::dma2_w ) { m_out_dma2_func(state); }
WRITE_LINE_MEMBER( s100_device::dma3_w ) { m_out_dma3_func(state); }
WRITE_LINE_MEMBER( s100_device::rdy_w ) { m_out_rdy_func(state); }
WRITE_LINE_MEMBER( s100_device::hold_w ) { m_out_hold_func(state); }
WRITE_LINE_MEMBER( s100_device::error_w ) { m_out_error_func(state); }



//**************************************************************************
//  DEVICE S100 CARD INTERFACE
//**************************************************************************

//-------------------------------------------------
//  device_s100_card_interface - constructor
//-------------------------------------------------

device_s100_card_interface::device_s100_card_interface(const machine_config &mconfig, device_t &device)
	: device_interface(device)
{
}


//-------------------------------------------------
//  ~device_s100_card_interface - destructor
//-------------------------------------------------

device_s100_card_interface::~device_s100_card_interface()
{
}
