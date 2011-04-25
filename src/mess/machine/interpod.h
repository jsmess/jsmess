#pragma once

#ifndef __INTERPOD__
#define __INTERPOD__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "machine/6522via.h"
#include "machine/6532riot.h"
#include "machine/6850acia.h"
#include "machine/cbmiec.h"
#include "machine/ieee488.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define INTERPOD_TAG			"interpod"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_INTERPOD_ADD(_daisy) \
    MCFG_DEVICE_ADD(INTERPOD_TAG, INTERPOD, 0) \
	MCFG_IEEE488_CONFIG_ADD(_daisy, interpod_ieee488_intf)



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> interpod_device_config

class interpod_device_config :   public device_config,
								 public device_config_cbm_iec_interface
{
    friend class interpod_device;

    // construction/destruction
    interpod_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);

public:
	// allocators
    static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
    virtual device_t *alloc_device(running_machine &machine) const;

	// optional information overrides
	virtual const rom_entry *device_rom_region() const;
	virtual machine_config_constructor device_mconfig_additions() const;

protected:
	// device_config overrides
    virtual void device_config_complete();
};


// ======================> interpod_device

class interpod_device :  public device_t,
					     public device_cbm_iec_interface
{
    friend class interpod_device_config;

    // construction/destruction
    interpod_device(running_machine &_machine, const interpod_device_config &_config);

protected:
    // device-level overrides
    virtual void device_start();
	virtual void device_reset();

	required_device<cpu_device> m_maincpu;
	required_device<via6522_device> m_via;
	required_device<riot6532_device> m_riot;
	required_device<acia6850_device> m_acia;
	required_device<cbm_iec_device> m_iec;
	required_device<ieee488_device> m_ieee;

    const interpod_device_config &m_config;
};


// device type definition
extern const device_type INTERPOD;


// IEEE-488 interface
extern const ieee488_stub_interface interpod_ieee488_intf;



#endif
