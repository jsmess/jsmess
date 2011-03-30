#pragma once
 
#ifndef __ISA_MDA_H__
#define __ISA_MDA_H__
 
#include "emu.h"
#include "machine/isa.h"
#include "video/mc6845.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************
 
// ======================> isa8_mda_device_config
 
class isa8_mda_device_config : 
		public device_config,
		public device_config_isa8_card_interface
{
        friend class isa8_device;
		friend class isa8_mda_device;
		friend class isa8_hercules_device_config;
 
        // construction/destruction
        isa8_mda_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
 
public:
        // allocators
        static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
        virtual device_t *alloc_device(running_machine &machine) const;
		
		// optional information overrides
		virtual machine_config_constructor device_mconfig_additions() const;
		virtual const rom_entry *device_rom_region() const;		
};
 
 
// ======================> isa8_mda_device
 
class isa8_mda_device : 
		public device_t,
		public device_isa8_card_interface
{
        friend class isa8_mda_device_config;
		friend class isa8_hercules_device_config;		
		friend class isa8_hercules_device;
 
        // construction/destruction
        isa8_mda_device(running_machine &_machine, const isa8_mda_device_config &config);

protected:
        // device-level overrides
        virtual void device_start();
        virtual void device_reset();

private:
        // internal state
        const isa8_mda_device_config &m_config;
public:		
		required_device<isa8_device> m_isa;
		int  framecnt;

		UINT8 mode_control;
		
		mc6845_update_row_func  update_row;
		UINT8   *chr_gen;
		UINT8   vsync;
		UINT8   hsync;
		UINT8  *videoram;
};
 
 
// device type definition
extern const device_type ISA8_MDA;

// ======================> isa8_hercules_device_config
 
class isa8_hercules_device_config : 
		public isa8_mda_device_config
{
        friend class isa8_hercules_device;
		friend class isa8_mda_device;
 
        // construction/destruction
        isa8_hercules_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
 
public:
        // allocators
        static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
        virtual device_t *alloc_device(running_machine &machine) const;
		
		// optional information overrides
		virtual machine_config_constructor device_mconfig_additions() const;
		virtual const rom_entry *device_rom_region() const;		
};
 
 
// ======================> isa8_hercules_device
 
class isa8_hercules_device : 
		public isa8_mda_device
{
        friend class isa8_hercules_device_config;
		friend class isa8_mda_device_config;
 
        // construction/destruction
        isa8_hercules_device(running_machine &_machine, const isa8_hercules_device_config &config);

protected:
        // device-level overrides
        virtual void device_start();
        virtual void device_reset();
 
private:
        // internal state
        const isa8_hercules_device_config &m_config;

public:
		UINT8 configuration_switch; //hercules
};
 
 
// device type definition
extern const device_type ISA8_HERCULES;

#endif  /* __ISA_MDA_H__ */
