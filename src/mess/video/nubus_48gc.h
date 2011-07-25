#pragma once

#ifndef __NUBUS_48GC_H__
#define __NUBUS_48GC_H__

#include "emu.h"
#include "machine/nubus.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> nubus_mda_device

class nubus_48gc_device :
		public device_t,
		public device_nubus_card_interface,
		public device_slot_card_interface
{
public:
        // construction/destruction
        nubus_48gc_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
		nubus_48gc_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock);
		
		// optional information overrides
		virtual machine_config_constructor device_mconfig_additions() const;
		virtual const rom_entry *device_rom_region() const;

protected:
        // device-level overrides
        virtual void device_start();
        virtual void device_reset();
public:

private:
        UINT8  *m_vram;
};


// device type definition
extern const device_type NUBUS_48GC;

#endif  /* __NUBUS_48GC_H__ */
