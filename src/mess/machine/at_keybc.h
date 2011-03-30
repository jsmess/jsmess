/***************************************************************************

	IBM PC AT compatibles 8042 keyboard controller

***************************************************************************/

#pragma once

#ifndef __AT_KEYBC_H__
#define __AT_KEYBC_H__

#include "emu.h"


//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_AT_KEYBOARD_CONTROLLER_ADD(_tag, _clock, _interface) \
    MCFG_DEVICE_ADD(_tag, AT_KEYBOARD_CONTROLLER, _clock) \
    MCFG_DEVICE_CONFIG(_interface)


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> at_keyboard_controller_interface

struct at_keyboard_controller_interface
{
	// interface to the host pc
	devcb_write_line	m_system_reset_func;
	devcb_write_line	m_gate_a20_func;
	devcb_write_line	m_input_buffer_full_func;
	devcb_write_line	m_output_buffer_empty_func;

	// interface to the keyboard
	devcb_write_line	m_keyboard_clock_func;
	devcb_write_line	m_keyboard_data_func;
};

// ======================> at_keyboard_controller_device_config

class at_keyboard_controller_device_config :	public device_config,
												public at_keyboard_controller_interface
{
	friend class at_keyboard_controller_device;

	// construction/destruction
	at_keyboard_controller_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);

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


// ======================> at_keyboard_controller_device

class at_keyboard_controller_device : public device_t
{
	friend class at_keyboard_controller_device_config;

	// construction/destruction
	at_keyboard_controller_device(running_machine &_machine, const at_keyboard_controller_device_config &config);

public:
	// internal 8042 interface
	DECLARE_READ8_MEMBER( t0_r );
	DECLARE_READ8_MEMBER( t1_r );
	DECLARE_READ8_MEMBER( p1_r );
	DECLARE_READ8_MEMBER( p2_r );
	DECLARE_WRITE8_MEMBER( p2_w );

	// interface to the host pc
	DECLARE_READ8_MEMBER( data_r );
	DECLARE_WRITE8_MEMBER( data_w );
	DECLARE_READ8_MEMBER( status_r );
	DECLARE_WRITE8_MEMBER( command_w );

	// interface to the keyboard
	DECLARE_WRITE_LINE_MEMBER( keyboard_clock_w );
	DECLARE_WRITE_LINE_MEMBER( keyboard_data_w );

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();

private:
	// internal state
	const at_keyboard_controller_device_config &m_config;

	device_t *m_cpu;

	devcb_resolved_write_line	m_system_reset_func;
	devcb_resolved_write_line	m_gate_a20_func;
	devcb_resolved_write_line	m_input_buffer_full_func;
	devcb_resolved_write_line	m_output_buffer_empty_func;
	devcb_resolved_write_line	m_keyboard_clock_func;
	devcb_resolved_write_line	m_keyboard_data_func;

	UINT8 m_clock_signal;
	UINT8 m_data_signal;
};


// device type definition
extern const device_type AT_KEYBOARD_CONTROLLER;


#endif	/* __AT_KEYBC__ */
