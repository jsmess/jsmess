/**********************************************************************

    Western Digital WD11C00-17 PC/XT Host Interface Logic Device

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "machine/wd11c00_17.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

// status register
#define STATUS_IRQ		0x20
#define STATUS_DRQ		0x10
#define STATUS_BUSY		0x08
#define STATUS_C_D		0x04
#define STATUS_I_O		0x02
#define STATUS_REQ		0x01


// mask register
#define MASK_IRQ		0x02
#define MASK_DMA		0x01



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type WD11C00_17 = &device_creator<wd11c00_17_device>;


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void wd11c00_17_device::device_config_complete()
{
	// inherit a copy of the static data
	const wd11c00_17_interface *intf = reinterpret_cast<const wd11c00_17_interface *>(static_config());
	if (intf != NULL)
		*static_cast<wd11c00_17_interface *>(this) = *intf;

	// or initialize to defaults if none provided
	else
	{
		memset(&m_out_irq5_cb, 0, sizeof(m_out_irq5_cb));
		memset(&m_out_drq3_cb, 0, sizeof(m_out_drq3_cb));
		memset(&m_out_mr_cb, 0, sizeof(m_out_mr_cb));
		memset(&m_in_rd322_cb, 0, sizeof(m_in_rd322_cb));
	}
}



//**************************************************************************
//  INLINE HELPERS
//**************************************************************************

//-------------------------------------------------
//  check_interrupt -
//-------------------------------------------------

inline void wd11c00_17_device::check_interrupt()
{
	int irq = ((m_status & STATUS_IRQ) && (m_mask & MASK_IRQ)) ? ASSERT_LINE : CLEAR_LINE;
	int drq = ((m_status & STATUS_DRQ) && (m_mask & MASK_DMA)) ? ASSERT_LINE : CLEAR_LINE;
	
	m_out_irq5_func(irq);
	m_out_drq3_func(drq);
}

	
//-------------------------------------------------
//  increment_address -
//-------------------------------------------------

inline void wd11c00_17_device::increment_address()
{
	m_ra++;
	
	if (BIT(m_ra, 10))
	{
		m_status &= ~STATUS_DRQ;
		
		check_interrupt();
	}
}


//-------------------------------------------------
//  read_data -
//-------------------------------------------------

inline UINT8 wd11c00_17_device::read_data()
{
	UINT8 data = 0;
	
	if (m_select)
	{
		data = m_ram[m_ra & 0x3ff];
		
		increment_address();
	}

	return data;
}


//-------------------------------------------------
//  write_data -
//-------------------------------------------------

inline void wd11c00_17_device::write_data(UINT8 data)
{
	if (m_select)
	{
		m_ram[m_ra & 0x3ff] = data;
		
		increment_address();
	}
}


//-------------------------------------------------
//  software_reset -
//-------------------------------------------------

inline void wd11c00_17_device::software_reset()
{
	m_out_mr_func(ASSERT_LINE);
	m_out_mr_func(CLEAR_LINE);
	
	device_reset();
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  wd11c00_17_device - constructor
//-------------------------------------------------

wd11c00_17_device::wd11c00_17_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, WD11C00_17, "Western Digital WD11C00-17", tag, owner, clock),
	  m_status(0)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void wd11c00_17_device::device_start()
{
	// resolve callbacks
	m_out_irq5_func.resolve(m_out_irq5_cb, *this);
	m_out_drq3_func.resolve(m_out_drq3_cb, *this);
	m_out_mr_func.resolve(m_out_mr_cb, *this);
	m_in_rd322_func.resolve(m_in_rd322_cb, *this);
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void wd11c00_17_device::device_reset()
{
	m_select = 0;
	m_mask = 0;
	m_ra = 0;
}


//-------------------------------------------------
//  read -
//-------------------------------------------------

READ8_MEMBER( wd11c00_17_device::read )
{
	UINT8 data = 0xff;
	
	switch (offset)
	{
	case 0: // Read Data, Board to Host
		data = read_data();
		break;

	case 1: // Read Board Hardware Status
		data = m_status;
		break;

	case 2: // Read Drive Configuration Information
		data = m_in_rd322_func(0);
		break;

	case 3: // Not Used
		break;
	}
	
	return data;
}


//-------------------------------------------------
//  write -
//-------------------------------------------------

WRITE8_MEMBER( wd11c00_17_device::write )
{
	switch (offset)
	{
	case 0: // Write Data, Host to Board
		write_data(data);
		break;

	case 1: // Board Software Reset
		software_reset();
		break;

	case 2:	// Board Select
		m_select = 1;
		break;

	case 3: // Set/Reset DMA, IRQ Masks
		m_mask = data;
		check_interrupt();
		break;
	}
}


//-------------------------------------------------
//  dack_r -
//-------------------------------------------------

UINT8 wd11c00_17_device::dack_r()
{
	return read_data();
}


//-------------------------------------------------
//  dack_w -
//-------------------------------------------------

void wd11c00_17_device::dack_w(UINT8 data)
{
	write_data(data);
}


//-------------------------------------------------
//  ireq_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( wd11c00_17_device::ireq_w )
{
	if (state) m_status |= STATUS_REQ; else m_status &= ~STATUS_REQ;
}


//-------------------------------------------------
//  io_w -
//-------------------------------------------------
	
WRITE_LINE_MEMBER( wd11c00_17_device::io_w )
{
	if (state) m_status |= STATUS_I_O; else m_status &= ~STATUS_I_O;
}


//-------------------------------------------------
//  cd_w -
//-------------------------------------------------
	
WRITE_LINE_MEMBER( wd11c00_17_device::cd_w )
{
	if (state) m_status |= STATUS_C_D; else m_status &= ~STATUS_C_D;
}


//-------------------------------------------------
//  busy_w -
//-------------------------------------------------
	
WRITE_LINE_MEMBER( wd11c00_17_device::busy_w )
{
	if (state) m_status |= STATUS_BUSY; else m_status &= ~STATUS_BUSY;
}


//-------------------------------------------------
//  clct_w -
//-------------------------------------------------
	
WRITE_LINE_MEMBER( wd11c00_17_device::clct_w )
{
	m_ra &= 0xff00;
}
