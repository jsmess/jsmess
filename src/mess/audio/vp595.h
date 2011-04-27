/**********************************************************************

    RCA VP595 - VIP Simple Sound System emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __VP595__
#define __VP595__

#include "emu.h"
#include "sound/cdp1863.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define VP595_TAG	"vp595"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_VP595_ADD() \
	MCFG_DEVICE_ADD(VP595_TAG, VP595, 0)



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> vp595_device

class vp595_device :	public device_t
{
public:
    // construction/destruction
    vp595_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	DECLARE_WRITE8_MEMBER( latch_w );
	DECLARE_WRITE_LINE_MEMBER( q_w );

	void install_write_handlers(address_space *space, bool enabled);

protected:
    // device-level overrides
    virtual void device_start();
	virtual machine_config_constructor device_mconfig_additions() const;

private:
	required_device<cdp1863_device> m_pfg;
};


// device type definition
extern const device_type VP595;



#endif
