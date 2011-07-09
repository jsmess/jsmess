#pragma once

#ifndef __ISA_IDE_H__
#define __ISA_IDE_H__

#include "emu.h"
#include "machine/isa.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> isa16_ide_device

class isa16_ide_device :
		public device_t,
		public device_isa16_card_interface,
		public device_slot_card_interface
{
public:
		// construction/destruction
        isa16_ide_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

		// optional information overrides
		virtual machine_config_constructor device_mconfig_additions() const;
protected:
        // device-level overrides
        virtual void device_start();
        virtual void device_reset();

private:
        // internal state
};


// device type definition
extern const device_type ISA16_IDE;

#endif  /* __ISA_IDE_H__ */
