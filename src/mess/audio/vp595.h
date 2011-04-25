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

// ======================> vp595_device_config

class vp595_device_config :   public device_config
{
    friend class vp595_device;

    // construction/destruction
    vp595_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);

public:
    // allocators
    static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
    virtual device_t *alloc_device(running_machine &machine) const;

	// optional information overrides
	virtual machine_config_constructor device_mconfig_additions() const;
};


// ======================> vp595_device

class vp595_device :	public device_t
{
    friend class vp595_device_config;

    // construction/destruction
    vp595_device(running_machine &_machine, const vp595_device_config &_config);

public:
	DECLARE_WRITE8_MEMBER( latch_w );
	DECLARE_WRITE_LINE_MEMBER( q_w );

	void install_write_handlers(address_space *space, bool enabled);

protected:
    // device-level overrides
    virtual void device_start();

private:
	required_device<cdp1863_device> m_pfg;

	const vp595_device_config &m_config;
};


// device type definition
extern const device_type VP595;



#endif
