/**********************************************************************

    Commodore 1570/1571/1571CR Single Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __C1571__
#define __C1571__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "imagedev/flopdrv.h"
#include "formats/d64_dsk.h"
#include "formats/g64_dsk.h"
#include "machine/64h156.h"
#include "machine/6522via.h"
#include "machine/6526cia.h"
#include "machine/cbmiec.h"
#include "machine/wd17xx.h"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_C1571_ADD(_tag, _address) \
    MCFG_DEVICE_ADD(_tag, C1571, 0) \
	c1571_device_config::static_set_config(device, _address);



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> c1571_device_config

class c1571_device_config :   public device_config,
							  public device_config_cbm_iec_interface
{
    friend class c1571_device;

    // construction/destruction
    c1571_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);

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
	int m_address;
};


// ======================> c1571_device

class c1571_device :  public device_t,
					  public device_cbm_iec_interface
{
    friend class c1571_device_config;

    // construction/destruction
    c1571_device(running_machine &_machine, const c1571_device_config &_config);

public:
	// not really public
	static void on_disk_change(device_image_interface &image);

	DECLARE_WRITE_LINE_MEMBER( via0_irq_w );
	DECLARE_READ8_MEMBER( via0_pa_r );
	DECLARE_WRITE8_MEMBER( via0_pa_w );
	DECLARE_READ8_MEMBER( via0_pb_r );
	DECLARE_WRITE8_MEMBER( via0_pb_w );
	DECLARE_READ_LINE_MEMBER( atn_in_r );
	DECLARE_READ_LINE_MEMBER( wprt_r );
	DECLARE_WRITE_LINE_MEMBER( via1_irq_w );
	DECLARE_READ8_MEMBER( via1_pb_r );
	DECLARE_WRITE8_MEMBER( via1_pb_w );
	DECLARE_WRITE_LINE_MEMBER( cia_irq_w );
	DECLARE_WRITE_LINE_MEMBER( cia_cnt_w );
	DECLARE_WRITE_LINE_MEMBER( cia_sp_w );
	DECLARE_WRITE_LINE_MEMBER( atn_w );
	DECLARE_WRITE_LINE_MEMBER( byte_w );
	DECLARE_WRITE_LINE_MEMBER( wpt_w );

protected:
    // device-level overrides
    virtual void device_start();
	virtual void device_reset();

	// device_cbm_iec_interface overrides
	void cbm_iec_srq(int state);
	void cbm_iec_atn(int state);
	void cbm_iec_data(int state);
	void cbm_iec_reset(int state);

private:
	inline void set_iec_data();
	inline void set_iec_srq();

	required_device<cpu_device> m_maincpu;
	required_device<via6522_device> m_via0;
	required_device<via6522_device> m_via1;
	required_device<mos6526_device> m_cia;
	required_device<device_t> m_fdc;
	required_device<c64h156_device> m_ga;
	required_device<device_t> m_image;
	cbm_iec_device *m_bus;

	// signals
	int m_1_2mhz;							// clock speed

	// IEC bus
	int m_data_out;							// serial data out
	int m_ser_dir;							// fast serial direction
	int m_sp_out;							// fast serial data out
	int m_cnt_out;							// fast serial clock out

	// interrupts
	int m_via0_irq;							// VIA #0 interrupt request
	int m_via1_irq;							// VIA #1 interrupt request
	int m_cia_irq;							// CIA interrupt request

    const c1571_device_config &m_config;
};



// device type definition
extern const device_type C1571;



#endif
