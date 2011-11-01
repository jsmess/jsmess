/**********************************************************************

    Western Digital WD2010 Winchester Disk Controller

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __WD2010__
#define __WD2010__

#define ADDRESS_MAP_MODERN

#include "emu.h"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_WD2010_ADD(_tag, _clock, _config) \
    MCFG_DEVICE_ADD(_tag, WD2010, _clock) \
	MCFG_DEVICE_CONFIG(_config)

	
#define WD2010_INTERFACE(_name) \
	const wd2010_interface (_name) =



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> wd2010_interface

struct wd2010_interface
{
	devcb_write_line	m_out_intrq_cb;
	devcb_write_line	m_out_bdrq_cb;
	devcb_write_line	m_out_bcr_cb;
};


// ======================> wd2010_device

class wd2010_device :	public device_t,
						public wd2010_interface
{
public:
	// construction/destruction
	wd2010_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();
    virtual void device_config_complete();

private:
	devcb_resolved_write_line	m_out_intrq_func;
	devcb_resolved_write_line	m_out_bdrq_func;
	devcb_resolved_write_line	m_out_bcr_func;
};


// device type definition
extern const device_type WD2010;

#endif
