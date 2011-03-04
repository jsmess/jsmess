/***************************************************************************
 
        ISA bus device
 
***************************************************************************/
 
#include "emu.h"
#include "machine/isa.h"
 
 
//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************
 
const device_type ISA8 = isa8_device_config::static_alloc_device_config;
 
//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************
 
//-------------------------------------------------
//  isa8_device_config - constructor
//-------------------------------------------------
 
isa8_device_config::isa8_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
        : device_config(mconfig, static_alloc_device_config, "ISA8", tag, owner, clock)
{
}
 
//-------------------------------------------------
//  static_alloc_device_config - allocate a new
//  configuration object
//-------------------------------------------------
 
device_config *isa8_device_config::static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
{
        return global_alloc(isa8_device_config(mconfig, tag, owner, clock));
}
 
//-------------------------------------------------
//  alloc_device - allocate a new device object
//-------------------------------------------------
 
device_t *isa8_device_config::alloc_device(running_machine &machine) const
{
        return auto_alloc(&machine, isa8_device(machine, *this));
}
 
void isa8_device_config::static_set_cputag(device_config *device, const char *tag)
{
        isa8_device_config *isa = downcast<isa8_device_config *>(device);
        isa->m_cputag = tag;
}
 
//**************************************************************************
//  LIVE DEVICE
//**************************************************************************
 
//-------------------------------------------------
//  isa8_device - constructor
//-------------------------------------------------
 
isa8_device::isa8_device(running_machine &_machine, const isa8_device_config &config) :
        device_t(_machine, config),
        m_config(config)
{
}
 
//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------
 
void isa8_device::device_start()
{        
        device_t *cpu = m_machine.device(m_config.m_cputag);
        m_space = cpu_get_address_space(cpu, ADDRESS_SPACE_PROGRAM);
}
 
//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------
 
void isa8_device::device_reset()
{
}


//**************************************************************************
//  DEVICE CONFIG ISA8 CARD INTERFACE
//**************************************************************************

//-------------------------------------------------
//  device_config_isa8_card_interface - constructor
//-------------------------------------------------

device_config_isa8_card_interface::device_config_isa8_card_interface(const machine_config &mconfig, device_config &devconfig)
	: device_config_interface(mconfig, devconfig)	
{
}


//-------------------------------------------------
//  ~device_config_isa8_card_interface - destructor
//-------------------------------------------------

device_config_isa8_card_interface::~device_config_isa8_card_interface()
{
}

void device_config_isa8_card_interface::static_set_isa8_tag(device_config *device, const char *tag) 
{
	device_config_isa8_card_interface *isa_card = dynamic_cast<device_config_isa8_card_interface *>(device);
	isa_card->m_isa_tag = tag;
}

void device_config_isa8_card_interface::static_set_isa8_num(device_config *device, int num)
{
	device_config_isa8_card_interface *isa_card = dynamic_cast<device_config_isa8_card_interface *>(device);
	isa_card->m_isa_num = num;
}


//**************************************************************************
//  DEVICE ISA8 CARD INTERFACE
//**************************************************************************

//-------------------------------------------------
//  device_isa8_card_interface - constructor
//-------------------------------------------------

device_isa8_card_interface::device_isa8_card_interface(running_machine &machine, const device_config &config, device_t &device)
	: device_interface(machine, config, device),
	  m_isa8_card_config(dynamic_cast<const device_config_isa8_card_interface &>(config))
{
}


//-------------------------------------------------
//  ~device_isa8_card_interface - destructor
//-------------------------------------------------

device_isa8_card_interface::~device_isa8_card_interface()
{
}
