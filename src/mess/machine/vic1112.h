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

#define MCFG_VIC1112_ADD(_daisy) \
    MCFG_DEVICE_ADD(VIC1112_TAG, VIC1112, 0) \
	MCFG_IEEE488_CONFIG_ADD(_daisy, vic1112_ieee488_intf)

#define VIC1112_INTERFACE(name) \
	const vic1112_interface (name) =



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> vic1112_interface

struct vic1112_interface
{
	const char *m_bus_tag;
};

// ======================> vic1112_device

class vic1112_device :  public device_t,
						public vic1112_interface
{
public:
    // construction/destruction
    vic1112_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

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

	// optional information overrides
	virtual const rom_entry *device_rom_region() const;
	virtual machine_config_constructor device_mconfig_additions() const;
protected:
    // device-level overrides
    virtual void device_start();
	virtual void device_reset();
    virtual void device_config_complete();

private:
	required_device<via6522_device> m_via0;
	required_device<via6522_device> m_via1;
	required_device<ieee488_device> m_bus;

	int m_via0_irq;
	int m_via1_irq;

   	const char *m_bus_tag;
};


// device type definition
extern const device_type VIC1112;


// IEEE-488 interface
extern const ieee488_stub_interface vic1112_ieee488_intf;



#endif
