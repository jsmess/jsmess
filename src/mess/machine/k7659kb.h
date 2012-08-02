/**********************************************************************

    K7659 keyboard emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

#pragma once

#ifndef __K7659_KEYBOARD__
#define __K7659_KEYBOARD__


#include "emu.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define K7659_KEYBOARD_TAG	"k7659kb"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_K7659_KEYBOARD_ADD() \
	MCFG_DEVICE_ADD(K7659_KEYBOARD_TAG, K7659_KEYBOARD, 0)



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> k7659_keyboard_device

class k7659_keyboard_device :  public device_t
{
public:
	// construction/destruction
	k7659_keyboard_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual const rom_entry *device_rom_region() const;
	virtual machine_config_constructor device_mconfig_additions() const;
	virtual ioport_constructor device_input_ports() const;

	DECLARE_READ8_MEMBER(read);

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();
	virtual void device_config_complete() { m_shortname = "k7659kb"; }

private:
	UINT8 key_pos(UINT8 val);
};


// device type definition
extern const device_type K7659_KEYBOARD;



#endif
