/**********************************************************************

    LH5810/LH5811 Input/Output Port Controller

**********************************************************************/

#pragma once

#ifndef __LH5810__
#define __LH5810__

#include "emu.h"

//*************************************************************************
//  MACROS / CONSTANTS
//*************************************************************************

enum
{
	LH5810_RESET = 4,
	LH5810_U,
	LH5810_L,
	LH5820_F,
	LH5810_OPC,
	LH5810_G,
	LH5810_MSK,
	LH5810_IF,
	LH5810_DDA,
	LH5810_DDB,
	LH5810_OPA,
	LH5810_OPB
};


//*************************************************************************
//  INTERFACE CONFIGURATION MACROS
//*************************************************************************

#define MCFG_LH5810_ADD(_tag, _config) \
	MCFG_DEVICE_ADD((_tag), LH5810, 0)	\
	MCFG_DEVICE_CONFIG(_config)



//*************************************************************************
//  TYPE DEFINITIONS
//*************************************************************************

// ======================> lh5810_interface

struct lh5810_interface
{
	devcb_read8			m_porta_r_func;		//port A read
	devcb_write8		m_porta_w_func;		//port A write
	devcb_read8			m_portb_r_func;		//port B read
	devcb_write8		m_portb_w_func;		//port B write
	devcb_write8		m_portc_w_func;		//port C write

	devcb_write_line	m_out_int_func;		//IRQ callback
};



// ======================> lh5810_device_config

class lh5810_device_config :   public device_config,
                               public lh5810_interface
{
    friend class lh5810_device;

    // construction/destruction
    lh5810_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);

public:
    // allocators
    static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
    virtual device_t *alloc_device(running_machine &machine) const;

protected:
	// device_config overrides
	virtual void device_config_complete();
};


// ======================> lh5810_device

class lh5810_device :	public device_t
{
    friend class lh5810_device_config;

    // construction/destruction
    lh5810_device(running_machine &_machine, const lh5810_device_config &_config);

public:
    DECLARE_READ8_MEMBER( data_r );
    DECLARE_WRITE8_MEMBER( data_w );

protected:
    // device-level overrides
    virtual void device_start();
    virtual void device_reset();

private:

	devcb_resolved_read8		m_porta_r_func;
	devcb_resolved_write8		m_porta_w_func;
	devcb_resolved_read8		m_portb_r_func;
	devcb_resolved_write8		m_portb_w_func;
	devcb_resolved_write8		m_portc_w_func;
	devcb_resolved_write_line	m_out_int_func;

	UINT8 m_reg[0x10];
	UINT8 m_irq;

	const lh5810_device_config &m_config;
};


// device type definition
extern const device_type LH5810;

#endif
