#pragma once

#ifndef __ISA_SOUND_BLASTER_H__
#define __ISA_SOUND_BLASTER_H__

#include "emu.h"
#include "machine/isa.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> isa8_sblaster_device

class isa8_sblaster1_0_device :
		public device_t,
		public device_isa8_card_interface
{
public:
        // construction/destruction
        isa8_sblaster1_0_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

		// optional information overrides
		virtual machine_config_constructor device_mconfig_additions() const;
protected:
        // device-level overrides
        virtual void device_start();
        virtual void device_reset();

private:
        // internal state
};

class isa8_sblaster1_5_device :
		public device_t,
		public device_isa8_card_interface
{
public:
        // construction/destruction
        isa8_sblaster1_5_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

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
extern const device_type ISA8_SOUND_BLASTER_1_0;
extern const device_type ISA8_SOUND_BLASTER_1_5;

#endif  /* __ISA_SOUND_BLASTER_H__ */
