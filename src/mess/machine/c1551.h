/**********************************************************************

    Commodore 1551 Single Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __C1551__
#define __C1551__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "imagedev/flopdrv.h"
#include "formats/d64_dsk.h"
#include "formats/g64_dsk.h"
#include "machine/64h156.h"
#include "machine/6525tpi.h"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_C1551_ADD(_tag, _address) \
    MCFG_DEVICE_ADD(_tag, C1551, 0) \
	c1551_device_config::static_set_config(device, _address);



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> c1551_device_config

class c1551_device_config :   public device_config
{
    friend class c1551_device;

    // construction/destruction
    c1551_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);

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
	
private:
	const char *m_cpu_tag;
	int m_address;
};


// ======================> c1551_device

class c1551_device :  public device_t
{
    friend class c1551_device_config;

    // construction/destruction
    c1551_device(running_machine &_machine, const c1551_device_config &_config);

public:
	// not really public
	static void on_disk_change(device_image_interface &image);

	DECLARE_READ8_MEMBER( port_r );
	DECLARE_WRITE8_MEMBER( port_w );
	DECLARE_READ8_MEMBER( tcbm_data_r );
	DECLARE_WRITE8_MEMBER( tcbm_data_w );
	DECLARE_READ8_MEMBER( yb_r );
	DECLARE_WRITE8_MEMBER( yb_w );
	DECLARE_READ8_MEMBER( tpi0_pc_r );
	DECLARE_WRITE8_MEMBER( tpi0_pc_w );
	DECLARE_READ8_MEMBER( tpi1_pa_r );
	DECLARE_WRITE8_MEMBER( tpi1_pa_w );
	DECLARE_READ8_MEMBER( tpi1_pb_r );
	DECLARE_READ8_MEMBER( tpi1_pc_r );
	DECLARE_WRITE8_MEMBER( tpi1_pc_w );

protected:
    // device-level overrides
    virtual void device_start();
	virtual void device_reset();
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);

private:
	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_tpi0;
	required_device<device_t> m_tpi1;
	required_device<c64h156_device> m_ga;
	required_device<device_t> m_image;

	// TCBM bus
	UINT8 m_tcbm_data;						// data
	int m_status;							// status
	int m_dav;								// data valid
	int m_ack;								// acknowledge

	// timers
	emu_timer *m_irq_timer;

    const c1551_device_config &m_config;
};



// device type definition
extern const device_type C1551;



#endif
