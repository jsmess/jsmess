/**********************************************************************

    Commodore 8280 Dual 8" Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __C8280__
#define __C8280__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "imagedev/flopdrv.h"
#include "machine/6532riot.h"
#include "machine/ieee488.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define C8280_TAG			"c8280"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_C8280_ADD(_tag, _address) \
    MCFG_DEVICE_ADD(_tag, C8280, 0) \
	c8280_device::static_set_config(*device, _address);



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> c8280_device

class c8280_device :  public device_t,
					  public device_ieee488_interface
{
public:
    // construction/destruction
    c8280_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// inline configuration helpers
	static void static_set_config(device_t &device, int address);

	// optional information overrides
	virtual const rom_entry *device_rom_region() const;
	virtual machine_config_constructor device_mconfig_additions() const;

protected:
    // device-level overrides
    virtual void device_start();
	virtual void device_reset();
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);
    virtual void device_config_complete();

	// device_ieee488_interface overrides
	void ieee488_atn(int state);
	void ieee488_ifc(int state);

	required_device<cpu_device> m_maincpu;
	required_device<cpu_device> m_fdccpu;
	required_device<riot6532_device> m_riot0;
	required_device<riot6532_device> m_riot1;
	required_device<device_t> m_image0;
	required_device<device_t> m_image1;
	required_device<ieee488_device> m_bus;

	int m_address;
};


// device type definition
extern const device_type C8280;



#endif
