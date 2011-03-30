#pragma once
 
#ifndef __ISA_COM_H__
#define __ISA_COM_H__
 
#include "emu.h"
#include "machine/isa.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************
 
// ======================> isa8_com_device_config
 
class isa8_com_device_config : 
		public device_config,
		public device_config_isa8_card_interface
{
        friend class isa8_device;
		friend class isa8_com_device;
 
        // construction/destruction
        isa8_com_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
 
public:
        // allocators
        static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
        virtual device_t *alloc_device(running_machine &machine) const;
		
		// optional information overrides
		virtual machine_config_constructor device_mconfig_additions() const;
};
 
 
// ======================> isa8_com_device
 
class isa8_com_device : 
		public device_t,
		public device_isa8_card_interface
{
        friend class isa8_com_device_config;
 
        // construction/destruction
        isa8_com_device(running_machine &_machine, const isa8_com_device_config &config);

protected:
        // device-level overrides
        virtual void device_start();
        virtual void device_reset();

private:
        // internal state
        const isa8_com_device_config &m_config;
public:		
		required_device<isa8_device> m_isa;		
};
 
 
// device type definition
extern const device_type ISA8_COM;

#endif  /* __ISA_COM_H__ */
