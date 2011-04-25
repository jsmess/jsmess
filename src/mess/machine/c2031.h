/**********************************************************************

    Commodore 2031 Single Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __C2031__
#define __C2031__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "imagedev/flopdrv.h"
#include "formats/d64_dsk.h"
#include "formats/g64_dsk.h"
#include "machine/64h156.h"
#include "machine/6522via.h"
#include "machine/ieee488.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define C2031_TAG			"c2031"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_C2031_ADD(_tag, _address) \
    MCFG_DEVICE_ADD(_tag, C2031, 0) \
	c2031_device_config::static_set_config(device, _address);



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> c2031_device_config

class c2031_device_config :   public device_config,
							  public device_config_ieee488_interface
{
    friend class c2031_device;

    // construction/destruction
    c2031_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);

public:
	// allocators
    static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
    virtual device_t *alloc_device(running_machine &machine) const;

	// inline configuration helpers
	static void static_set_config(device_config *device, int address);

	// optional information overrides
	virtual const rom_entry *device_rom_region() const;
	virtual machine_config_constructor device_mconfig_additions() const;

protected:
	// device_config overrides
    virtual void device_config_complete();

	int m_address;
};


// ======================> c2031_device

class c2031_device :  public device_t,
					  public device_ieee488_interface
{
    friend class c2031_device_config;

    // construction/destruction
    c2031_device(running_machine &_machine, const c2031_device_config &_config);

public:
	// not really public
	static void on_disk_change(device_image_interface &image);

	DECLARE_WRITE_LINE_MEMBER( via0_irq_w );
	DECLARE_READ8_MEMBER( via0_pa_r );
	DECLARE_WRITE8_MEMBER( via0_pa_w );
	DECLARE_READ8_MEMBER( via0_pb_r );
	DECLARE_WRITE8_MEMBER( via0_pb_w );
	DECLARE_READ_LINE_MEMBER( via0_ca1_r );
	DECLARE_READ_LINE_MEMBER( via0_ca2_r );
	DECLARE_WRITE_LINE_MEMBER( via1_irq_w );
	DECLARE_READ8_MEMBER( via1_pb_r );
	DECLARE_WRITE8_MEMBER( via1_pb_w );
	DECLARE_WRITE_LINE_MEMBER( byte_w );

protected:
    // device-level overrides
    virtual void device_start();
	virtual void device_reset();

	// device_ieee488_interface overrides
	void ieee488_atn(int state);
	void ieee488_ifc(int state);

	inline int get_device_number();

	required_device<cpu_device> m_maincpu;
	required_device<via6522_device> m_via0;
	required_device<via6522_device> m_via1;
	required_device<c64h156_device> m_ga;
	required_device<device_t> m_image;
	required_device<ieee488_device> m_bus;

	// IEEE-488 bus
	int m_nrfd_out;				// not ready for data
	int m_ndac_out;				// not data accepted
	int m_atna;					// attention acknowledge

	// interrupts
	int m_via0_irq;							// VIA #0 interrupt request
	int m_via1_irq;							// VIA #1 interrupt request

    const c2031_device_config &m_config;
};


// device type definition
extern const device_type C2031;



#endif
