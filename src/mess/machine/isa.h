/***************************************************************************
 
        ISA bus device
 
***************************************************************************/
 
#pragma once
 
#ifndef __ISA_H__
#define __ISA_H__
 
#include "emu.h"
 
 
//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************
 
#define MCFG_ISA8_BUS_ADD(_tag, _cputag) \
    MCFG_DEVICE_ADD(_tag, ISA8, 0) \
    isa8_device_config::static_set_cputag(device, _cputag); \
 
#define MCFG_ISA8_BUS_DEVICE(_isatag, _num, _tag, _dev_type) \
    MCFG_DEVICE_ADD(_tag, _dev_type, 0) \
	device_config_isa8_card_interface::static_set_isa8_tag(device, _isatag); \
	device_config_isa8_card_interface::static_set_isa8_num(device, _num); \

	
//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************
 
// ======================> isa8_device_config
 
class isa8_device_config : public device_config
{
        friend class isa8_device;
 
        // construction/destruction
        isa8_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
 
public:
        // allocators
        static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
        virtual device_t *alloc_device(running_machine &machine) const;
 
        // inline configuration
        static void static_set_cputag(device_config *device, const char *tag);
 
        const char *m_cputag;
};
 
 
// ======================> isa8_device
 
class isa8_device : public device_t
{
        friend class isa8_device_config;
 
        // construction/destruction
        isa8_device(running_machine &_machine, const isa8_device_config &config);

protected:
        // device-level overrides
        virtual void device_start();
        virtual void device_reset();
 
private:
        // internal state
        const isa8_device_config &m_config;
 
        address_space *m_space; 
};
 
 
// device type definition
extern const device_type ISA8;
 

// ======================> device_config_isa8_card_interface

// class representing interface-specific configuration isa8 card
class device_config_isa8_card_interface : public device_config_interface
{
public:
	// construction/destruction
	device_config_isa8_card_interface(const machine_config &mconfig, device_config &device);
	virtual ~device_config_isa8_card_interface();
    // inline configuration
    static void static_set_isa8_tag(device_config *device, const char *tag);
    static void static_set_isa8_num(device_config *device, int num);	
protected:
	const char *m_isa_tag;
	int m_isa_num;
};


// ======================> device_isa8_card_interface

// class representing interface-specific live isa8 card
class device_isa8_card_interface : public device_interface
{
public:
	// construction/destruction
	device_isa8_card_interface(running_machine &machine, const device_config &config, device_t &device);
	virtual ~device_isa8_card_interface();

	// configuration access
	const device_config_isa8_card_interface &isa8_card_config() const { return m_isa8_card_config; }

protected:

	// configuration
	const device_config_isa8_card_interface &m_isa8_card_config;	// reference to our device_config_execute_interface
};
 
#endif  /* __ISA_H__ */
