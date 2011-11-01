/**********************************************************************

    Western Digital WD2010 Winchester Disk Controller

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "machine/wd2010.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************




//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type WD2010 = &device_creator<wd2010_device>;


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void wd2010_device::device_config_complete()
{
	// inherit a copy of the static data
	const wd2010_interface *intf = reinterpret_cast<const wd2010_interface *>(static_config());
	if (intf != NULL)
		*static_cast<wd2010_interface *>(this) = *intf;

	// or initialize to defaults if none provided
	else
	{
		memset(&m_out_intrq_cb, 0, sizeof(m_out_intrq_cb));
		memset(&m_out_bdrq_cb, 0, sizeof(m_out_bdrq_cb));
		memset(&m_out_bcr_cb, 0, sizeof(m_out_bcr_cb));
	}
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  wd2010_device - constructor
//-------------------------------------------------

wd2010_device::wd2010_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, WD2010, "Western Digital WD2010", tag, owner, clock)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void wd2010_device::device_start()
{
	// resolve callbacks
	m_out_intrq_func.resolve(m_out_intrq_cb, *this);
	m_out_bdrq_func.resolve(m_out_bdrq_cb, *this);
	m_out_bcr_func.resolve(m_out_bcr_cb, *this);
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void wd2010_device::device_reset()
{
}
