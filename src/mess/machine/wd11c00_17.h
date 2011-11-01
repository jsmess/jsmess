/**********************************************************************

    Western Digital WD11C00-17 PC/XT Host Interface Logic Device

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __WD11C00_17__
#define __WD11C00_17__

#define ADDRESS_MAP_MODERN

#include "emu.h"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_WD11C00_17_ADD(_tag, _clock, _config) \
    MCFG_DEVICE_ADD(_tag, WD11C00_17, _clock) \
	MCFG_DEVICE_CONFIG(_config)

#define WD11C00_17_INTERFACE(_name) \
	const wd11c00_17_interface (_name) =



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> wd11c00_17_interface

struct wd11c00_17_interface
{
	devcb_write_line	m_out_irq5_cb;
	devcb_write_line	m_out_drq3_cb;
	devcb_write_line	m_out_mr_cb;
	devcb_read8			m_in_rd322_cb;
};


// ======================> wd11c00_17_device

class wd11c00_17_device :	public device_t,
							public wd11c00_17_interface
{
public:
	// construction/destruction
	wd11c00_17_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	
	DECLARE_READ8_MEMBER( read );
	DECLARE_WRITE8_MEMBER( write );
	DECLARE_READ8_MEMBER( dack_r );
	DECLARE_WRITE8_MEMBER( dack_w );

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();
    virtual void device_config_complete();
	
private:
	devcb_resolved_write_line	m_out_irq5_func;
	devcb_resolved_write_line	m_out_drq3_func;
	devcb_resolved_write_line	m_out_mr_func;
	devcb_resolved_read8		m_in_rd322_func;	
};


// device type definition
extern const device_type WD11C00_17;

#endif
