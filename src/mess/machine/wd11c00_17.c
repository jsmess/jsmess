/**********************************************************************

    Western Digital WD11C00-17 PC/XT Host Interface Logic Device

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "machine/wd11c00_17.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************




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
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  wd11c00_17_device - constructor
//-------------------------------------------------

wd11c00_17_device::wd11c00_17_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, WD11C00_17, "Western Digital WD11C00-17", tag, owner, clock)
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
}


//-------------------------------------------------
//  read -
//-------------------------------------------------

READ8_MEMBER( wd11c00_17_device::read )
{
	return 0;
}


//-------------------------------------------------
//  write -
//-------------------------------------------------

WRITE8_MEMBER( wd11c00_17_device::write )
{
}


//-------------------------------------------------
//  dack_r -
//-------------------------------------------------

READ8_MEMBER( wd11c00_17_device::dack_r )
{
	return 0;
}


//-------------------------------------------------
//  dack_w -
//-------------------------------------------------

WRITE8_MEMBER( wd11c00_17_device::dack_w )
{
}
