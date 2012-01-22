#pragma once

#ifndef __WANGPC_KEYBOARD__
#define __WANGPC_KEYBOARD__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/mcs51/mcs51.h"
#include "sound/sn76496.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define WANGPC_KEYBOARD_TAG	"wangpckb"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_WANGPC_KEYBOARD_ADD(_config) \
    MCFG_DEVICE_ADD(WANGPC_KEYBOARD_TAG, WANGPC_KEYBOARD, 0) \
	MCFG_DEVICE_CONFIG(_config)


#define WANGPC_KEYBOARD_INTERFACE(_name) \
	const wangpc_keyboard_interface (_name) =



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> wangpc_keyboard_interface

struct wangpc_keyboard_interface
{
	devcb_write_line	m_out_data_cb;
};


// ======================> wangpc_keyboard_device

class wangpc_keyboard_device :  public device_t,
								public wangpc_keyboard_interface
{
public:
    // construction/destruction
    wangpc_keyboard_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual const rom_entry *device_rom_region() const;
	virtual machine_config_constructor device_mconfig_additions() const;
	virtual ioport_constructor device_input_ports() const;
	
	// not really public
	DECLARE_READ8_MEMBER( kb_p1_r );
	DECLARE_WRITE8_MEMBER( kb_p2_w );
	DECLARE_WRITE8_MEMBER( kb_p3_w );
	
protected:
    // device-level overrides
    virtual void device_start();
	virtual void device_reset();
	virtual void device_config_complete() { m_shortname = "wangpckb"; }

private:
	required_device<cpu_device> m_maincpu;
	
	UINT8 m_y;
};


// device type definition
extern const device_type WANGPC_KEYBOARD;



#endif
