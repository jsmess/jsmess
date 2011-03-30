/**********************************************************************

    ISA 8 bit Floppy Disk Controller

**********************************************************************/
#pragma once
 
#ifndef ISA_FDC_H
#define ISA_FDC_H

#include "emu.h"
#include "machine/isa.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************
 
// ======================> isa8_fdc_device_config
 
class isa8_fdc_device_config : 
		public device_config,
		public device_config_isa8_card_interface
{
        friend class isa8_device;
		friend class isa8_fdc_device;
 
        // construction/destruction
        isa8_fdc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
 
public:
        // allocators
        static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
        virtual device_t *alloc_device(running_machine &machine) const;
		
		// optional information overrides
		virtual machine_config_constructor device_mconfig_additions() const;
};
 
 
// ======================> isa8_fdc_device
 
class isa8_fdc_device : 
		public device_t,
		public device_isa8_card_interface
{
        friend class isa8_fdc_device_config;
 
        // construction/destruction
        isa8_fdc_device(running_machine &_machine, const isa8_fdc_device_config &config);

protected:
        // device-level overrides
        virtual void device_start();
        virtual void device_reset();

private:
        // internal state
        const isa8_fdc_device_config &m_config;
public:
		virtual UINT8 dack_r(int line);
		virtual void dack_w(int line,UINT8 data);
		virtual void eop_w(int state);
		virtual bool have_dack(int line);
		
		int status_register_a;
		int status_register_b;
		int digital_output_register;
		int tape_drive_register;
		int data_rate_register;
		int digital_input_register;
		int configuration_control_register;

		/* stored tc state - state present at pins */
		int tc_state;
		/* stored dma drq state */
		int dma_state;
		/* stored int state */
		int int_state;
		
		required_device<device_t> m_upd765;
public:		
		required_device<isa8_device> m_isa;
};
 
 
// device type definition
extern const device_type ISA8_FDC;

#endif  /* ISA_FDC_H */
