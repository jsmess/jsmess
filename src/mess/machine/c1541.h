/**********************************************************************

    Commodore 1540/1541/1541C/1541-II/2031 Single Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __C1541__
#define __C1541__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "imagedev/flopdrv.h"
#include "formats/d64_dsk.h"
#include "formats/g64_dsk.h"
#include "machine/64h156.h"
#include "machine/6522via.h"
#include "machine/cbmiec.h"
#include "machine/ieee488.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define C1541_TAG			"c1541"
#define C2031_TAG			"c2031"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_C1540_ADD(_tag, _address) \
    MCFG_DEVICE_ADD(_tag, C1540, 0) \
	c1541_device_config::static_set_config(device, _address, c1541_device_config::TYPE_1540);

#define MCFG_C1541_ADD(_tag, _address) \
    MCFG_DEVICE_ADD(_tag, C1541, 0) \
	c1541_device_config::static_set_config(device, _address, c1541_device_config::TYPE_1541);

#define MCFG_C1541C_ADD(_tag, _address) \
    MCFG_DEVICE_ADD(_tag, C1541C, 0) \
	c1541_device_config::static_set_config(device, _address, c1541_device_config::TYPE_1541C);

#define MCFG_C1541II_ADD(_tag, _address) \
    MCFG_DEVICE_ADD(_tag, C1541II, 0) \
	c1541_device_config::static_set_config(device, _address, c1541_device_config::TYPE_1541II);

#define MCFG_SX1541_ADD(_tag, _address) \
    MCFG_DEVICE_ADD(_tag, SX1541, 0) \
	c1541_device_config::static_set_config(device, _address, c1541_device_config::TYPE_SX1541);

#define MCFG_OC118_ADD(_tag, _address) \
    MCFG_DEVICE_ADD(_tag, OC118, 0) \
	c1541_device_config::static_set_config(device, _address, c1541_device_config::TYPE_OC118);

#define MCFG_C2031_ADD(_tag, _address) \
    MCFG_DEVICE_ADD(_tag, C2031, 0) \
	c1541_device_config::static_set_config(device, _address, c1541_device_config::TYPE_C2031);



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> c1541_device_config

class c1541_device_config :   public device_config,
							  public device_config_cbm_iec_interface
{
    friend class c1540_device;
    friend class c1541_device;
    friend class c1541c_device;
    friend class c1541ii_device;
    friend class sx1541_device;
    friend class oc118_device;
    friend class c2031_device;

    // construction/destruction
    c1541_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);

public:
	enum
	{
		TYPE_1540 = 0,
		TYPE_1541,
		TYPE_1541C,
		TYPE_1541II,
		TYPE_SX1541,
		TYPE_OC118,
		TYPE_C2031
	};
	
	// allocators
    static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
    virtual device_t *alloc_device(running_machine &machine) const;

	// inline configuration helpers
	static void static_set_config(device_config *device, int address, int variant);

	// optional information overrides
	virtual const rom_entry *device_rom_region() const;
	virtual machine_config_constructor device_mconfig_additions() const;

protected:
	// device_config overrides
    virtual void device_config_complete();
	
	int m_address;
	int m_variant;
};


// ======================> c1541_device

class c1541_device :  public device_t,
					  public device_cbm_iec_interface
{
    friend class c1541_device_config;

    // construction/destruction
    c1541_device(running_machine &_machine, const c1541_device_config &_config);

public:
	// not really public
	static void on_disk_change(device_image_interface &image);

	DECLARE_WRITE_LINE_MEMBER( via0_irq_w );
	DECLARE_READ8_MEMBER( via0_pa_r );
	DECLARE_WRITE8_MEMBER( via0_pa_w );
	DECLARE_READ8_MEMBER( via0_pb_r );
	DECLARE_WRITE8_MEMBER( via0_pb_w );
	DECLARE_READ_LINE_MEMBER( atn_in_r );
	DECLARE_WRITE_LINE_MEMBER( via1_irq_w );
	DECLARE_READ8_MEMBER( via1_pb_r );
	DECLARE_WRITE8_MEMBER( via1_pb_w );
	DECLARE_WRITE_LINE_MEMBER( atn_w );
	DECLARE_WRITE_LINE_MEMBER( byte_w );

protected:
    // device-level overrides
    virtual void device_start();
	virtual void device_reset();

	// device_cbm_iec_interface overrides
	void cbm_iec_atn(int state);
	void cbm_iec_reset(int state);

	inline void set_iec_data();

	required_device<cpu_device> m_maincpu;
	required_device<via6522_device> m_via0;
	required_device<via6522_device> m_via1;
	required_device<c64h156_device> m_ga;
	required_device<device_t> m_image;
	required_device<cbm_iec_device> m_bus;

	// IEC bus
	int m_data_out;							// serial data out

	// interrupts
	int m_via0_irq;							// VIA #0 interrupt request
	int m_via1_irq;							// VIA #1 interrupt request

    const c1541_device_config &m_config;
};


// ======================> c1540_device

class c1540_device :  public c1541_device
{
    friend class c1541_device_config;

    // construction/destruction
    c1540_device(running_machine &_machine, const c1541_device_config &_config);
};


// ======================> c1541c_device

class c1541c_device :  public c1541_device
{
    friend class c1541_device_config;

    // construction/destruction
    c1541c_device(running_machine &_machine, const c1541_device_config &_config);

public:
	// not really public
	DECLARE_READ8_MEMBER( via0_pa_r );
};


// ======================> c1541ii_device

class c1541ii_device :  public c1541_device
{
    friend class c1541_device_config;

    // construction/destruction
    c1541ii_device(running_machine &_machine, const c1541_device_config &_config);
};


// ======================> sx1541_device

class sx1541_device :  public c1541_device
{
    friend class c1541_device_config;

    // construction/destruction
    sx1541_device(running_machine &_machine, const c1541_device_config &_config);
};


// ======================> oc118_device

class oc118_device :  public c1541_device
{
    friend class c1541_device_config;

    // construction/destruction
    oc118_device(running_machine &_machine, const c1541_device_config &_config);
};


// ======================> c2031_device

class c2031_device :  public c1541_device,
					  public device_ieee488_interface
{
    friend class c1541_device_config;

    // construction/destruction
    c2031_device(running_machine &_machine, const c1541_device_config &_config);

public:
	// not really public
	DECLARE_READ8_MEMBER( via0_pa_r );
	DECLARE_WRITE8_MEMBER( via0_pa_w );
	DECLARE_READ8_MEMBER( via0_pb_r );
	DECLARE_WRITE8_MEMBER( via0_pb_w );
	DECLARE_READ_LINE_MEMBER( via0_ca1_r );
	DECLARE_READ_LINE_MEMBER( via0_ca2_r );

protected:
	// device_ieee488_interface overrides
	void ieee488_atn(int state);
	void ieee488_ifc(int state);

	required_device<ieee488_device> m_bus;

	int m_nrfd_out;				// not ready for data
	int m_ndac_out;				// not data accepted
	int m_atna;					// attention acknowledge
};


// device type definition
extern const device_type C1540;
extern const device_type C1541;
extern const device_type C1541C;
extern const device_type C1541II;
extern const device_type SX1541;
extern const device_type OC118;
extern const device_type C2031;



#endif
