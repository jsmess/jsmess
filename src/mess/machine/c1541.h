/**********************************************************************

    Commodore 1540/1541/1541C/1541-II Single Disk Drive emulation

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
#include "machine/c64_bn1541.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define C1541_TAG			"c1541"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> c1541_device

class c1541_device :  public device_t,
					  public device_cbm_iec_interface,
					  public device_c64_floppy_parallel_interface
{
public:
    // construction/destruction
	c1541_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock);
    c1541_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	enum
	{
		TYPE_1540 = 0,
		TYPE_1541,
		TYPE_1541C,
		TYPE_1541II,
		TYPE_SX1541,
		TYPE_OC118
	};

	// not really public
	static void on_disk_change(device_image_interface &image);

	DECLARE_WRITE_LINE_MEMBER( via0_irq_w );
	DECLARE_READ8_MEMBER( via0_pa_r );
	DECLARE_WRITE8_MEMBER( via0_pa_w );
	DECLARE_READ8_MEMBER( via0_pb_r );
	DECLARE_WRITE8_MEMBER( via0_pb_w );
	DECLARE_WRITE_LINE_MEMBER( via0_ca2_w );
	DECLARE_READ_LINE_MEMBER( atn_in_r );
	DECLARE_WRITE_LINE_MEMBER( via1_irq_w );
	DECLARE_READ8_MEMBER( via1_pb_r );
	DECLARE_WRITE8_MEMBER( via1_pb_w );
	DECLARE_WRITE_LINE_MEMBER( atn_w );
	DECLARE_WRITE_LINE_MEMBER( byte_w );

	// optional information overrides
	virtual const rom_entry *device_rom_region() const;
	virtual machine_config_constructor device_mconfig_additions() const;

protected:
    // device-level overrides
    virtual void device_start();
	virtual void device_reset();
    virtual void device_config_complete();

	// device_cbm_iec_interface overrides
	void cbm_iec_atn(int state);
	void cbm_iec_reset(int state);

	// device_c64_floppy_parallel_interface overrides
	void parallel_data_w(UINT8 data);
	void parallel_strobe_w(int state);

	inline void set_iec_data();

	required_device<cpu_device> m_maincpu;
	required_device<via6522_device> m_via0;
	required_device<via6522_device> m_via1;
	required_device<c64h156_device> m_ga;
	required_device<device_t> m_image;

	// IEC bus
	int m_data_out;							// serial data out

	// interrupts
	int m_via0_irq;							// VIA #0 interrupt request
	int m_via1_irq;							// VIA #1 interrupt request

	int m_variant;
};


// ======================> c1540_device

class c1540_device :  public c1541_device
{
public:
    // construction/destruction
    c1540_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
};


// ======================> c1541c_device

class c1541c_device :  public c1541_device
{
public:
    // construction/destruction
    c1541c_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	// not really public
	DECLARE_READ8_MEMBER( via0_pa_r );
};


// ======================> c1541ii_device

class c1541ii_device :  public c1541_device
{
public:
    // construction/destruction
    c1541ii_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
};


// ======================> sx1541_device

class sx1541_device :  public c1541_device
{
public:
    // construction/destruction
    sx1541_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
};


// ======================> oc118_device

class oc118_device :  public c1541_device
{
public:
    // construction/destruction
    oc118_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
};


// device type definition
extern const device_type C1540;
extern const device_type C1541;
extern const device_type C1541C;
extern const device_type C1541II;
extern const device_type SX1541;
extern const device_type OC118;



#endif
