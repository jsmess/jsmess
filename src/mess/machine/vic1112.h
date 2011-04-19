/**********************************************************************

    Commodore VIC-1112 IEEE-488 Interface Cartridge emulation

    SYS 45065 to start

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __VIC1112__
#define __VIC1112__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "machine/6522via.h"
#include "machine/ieee488.h"
#include "machine/devhelpr.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define VIC1112_TAG	"vic1112"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_VIC1112_ADD(_config) \
    MCFG_DEVICE_ADD(VIC1112_TAG, VIC1112, 0) \
	MCFG_DEVICE_CONFIG(_config)


#define VIC1112_INTERFACE(name) \
	const vic1112_interface (name) =


#define VIC1112_IEEE488 \
	VIC1112_TAG, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, \
	DEVCB_DEVICE_LINE_MEMBER(VIC1112_TAG, vic1112_device, srq_w), DEVCB_NULL, DEVCB_NULL


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> vic1112_interface

struct vic1112_interface
{
	const char *m_bus_tag;
};


// ======================> vic1112_device_config

class vic1112_device_config :   public device_config,
							    public vic1112_interface
{
    friend class vic1112_device;

    // construction/destruction
    vic1112_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);

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

private:
	const char *m_bus_tag;
};


// ======================> vic1112_device

class vic1112_device :  public device_t
{
    friend class vic1112_device_config;

    // construction/destruction
    vic1112_device(running_machine &_machine, const vic1112_device_config &_config);

public:
	DECLARE_WRITE_LINE_MEMBER( srq_w );

	// not really public, but won't compile otherwise
	DECLARE_WRITE_LINE_MEMBER( via0_irq_w );
	DECLARE_READ8_MEMBER( via0_pb_r );
	DECLARE_WRITE8_MEMBER( via0_pb_w );
	DECLARE_WRITE_LINE_MEMBER( via1_irq_w );
	DECLARE_READ8_MEMBER( dio_r );
	DECLARE_WRITE8_MEMBER( dio_w );
	DECLARE_WRITE_LINE_MEMBER( atn_w );
	DECLARE_READ_LINE_MEMBER( srq_r );
	DECLARE_WRITE_LINE_MEMBER( eoi_w );

protected:
    // device-level overrides
    virtual void device_start();
	virtual void device_reset();

private:
	required_device<via6522_device> m_via0;
	required_device<via6522_device> m_via1;
	device_t *m_bus;

	int m_via0_irq;
	int m_via1_irq;

    const vic1112_device_config &m_config;
};


// device type definition
extern const device_type VIC1112;



#endif
