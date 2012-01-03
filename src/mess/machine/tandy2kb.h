#pragma once

#ifndef __TANDY2K_KEYBOARD__
#define __TANDY2K_KEYBOARD__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/mcs48/mcs48.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define TANDY2K_KEYBOARD_TAG	"tandy2kb"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_TANDY2K_KEYBOARD_ADD() \
    MCFG_DEVICE_ADD(TANDY2K_KEYBOARD_TAG, TANDY2K_KEYBOARD, 0)



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> tandy2k_keyboard_device

class tandy2k_keyboard_device :  public device_t
{
public:
    // construction/destruction
    tandy2k_keyboard_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual const rom_entry *device_rom_region() const;
	virtual machine_config_constructor device_mconfig_additions() const;
	virtual ioport_constructor device_input_ports() const;
	
	// not really public
	DECLARE_READ8_MEMBER( kb_x0_r );
	DECLARE_WRITE8_MEMBER( kb_y0_w );
	DECLARE_WRITE8_MEMBER( kb_y8_w );

protected:
    // device-level overrides
    virtual void device_start();
	virtual void device_reset();
	virtual void device_config_complete() { m_shortname = "tandy2kb"; }
	
private:
	required_device<cpu_device> m_maincpu;

	UINT16 m_keylatch;
};


// device type definition
extern const device_type TANDY2K_KEYBOARD;



#endif
