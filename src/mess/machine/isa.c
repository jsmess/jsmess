/***************************************************************************
 
        ISA bus device
 
***************************************************************************/
 
#include "emu.h"
#include "machine/isa.h"
#include "machine/pic8259.h"
#include "machine/8237dma.h"

 
 
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
void isa8_device_config::static_set_dmatag(device_config *device, const char *tag)
{
        isa8_device_config *isa = downcast<isa8_device_config *>(device);
        isa->m_dmatag = tag;
}
void isa8_device_config::static_set_pictag(device_config *device, const char *tag)
{
        isa8_device_config *isa = downcast<isa8_device_config *>(device);
        isa->m_pictag = tag;
}
 
//**************************************************************************
//  LIVE DEVICE
//**************************************************************************
 
//-------------------------------------------------
//  isa8_device - constructor
//-------------------------------------------------
 
isa8_device::isa8_device(running_machine &_machine, const isa8_device_config &config) :
        device_t(_machine, config),
        m_config(config),
		m_maincpu(NULL),
		m_pic8259(NULL),
		m_dma8237(NULL)
{
	for(int i=0;i<8;i++) 
		m_isa_device[i] = NULL;
}
 
//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------
 
void isa8_device::device_start()
{        
	m_maincpu = m_machine.device(m_config.m_cputag);
	m_pic8259 = m_machine.device(m_config.m_pictag);
	m_dma8237 = m_machine.device(m_config.m_dmatag);
}
 
//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------
  
void isa8_device::device_reset()
{
}

void isa8_device::add_isa_card(device_isa8_card_interface *card,int pos)
{
	m_isa_device[pos] = card;
}

void isa8_device::install_device(device_t *dev, offs_t start, offs_t end, offs_t mask, offs_t mirror, read8_device_func rhandler, write8_device_func whandler)
{	
	int buswidth = device_memory(m_maincpu)->space_config(AS_PROGRAM)->m_databus_width;
	switch(buswidth)
	{
		case 8:
			memory_install_readwrite8_device_handler(cpu_get_address_space(m_maincpu, ADDRESS_SPACE_IO), dev, start, end, mask, mirror, rhandler, whandler );
			break;

		default:
			fatalerror("ISA8: Bus width %d not supported", buswidth);
			break;
	}
}

void isa8_device::install_bank(offs_t start, offs_t end, offs_t mask, offs_t mirror, const char *tag, UINT8 *data)
{
	address_space *space = cpu_get_address_space(m_maincpu, ADDRESS_SPACE_PROGRAM);
	memory_install_readwrite_bank(space, start, end, mask, mirror, tag );
	memory_set_bankptr(&m_machine, tag, data);
}

void isa8_device::install_rom(device_t *dev, offs_t start, offs_t end, offs_t mask, offs_t mirror, const char *tag, const char *region)
{
	astring tempstring;
	address_space *space = cpu_get_address_space(m_maincpu, ADDRESS_SPACE_PROGRAM);	
	memory_install_read_bank(space, start, end, mask, mirror, tag);
	memory_unmap_write(space, start, end, mask, mirror);		
	memory_set_bankptr(&m_machine, tag, machine->region(dev->subtag(tempstring, region))->base());
}



void isa8_device::set_irq_line(int irq, int state)
{
	switch (irq)
	{
		case 0: pic8259_ir0_w(m_pic8259, state); break;
		case 1: pic8259_ir1_w(m_pic8259, state); break;
		case 2: pic8259_ir2_w(m_pic8259, state); break;
		case 3: pic8259_ir3_w(m_pic8259, state); break;
		case 4: pic8259_ir4_w(m_pic8259, state); break;
		case 5: pic8259_ir5_w(m_pic8259, state); break;
		case 6: pic8259_ir6_w(m_pic8259, state); break;
		case 7: pic8259_ir7_w(m_pic8259, state); break;
	}
}

void isa8_device::set_dreq_line(int line, int state)
{
	switch (line)
	{
		case 0: i8237_dreq0_w(m_dma8237, state); break;
		case 1: i8237_dreq1_w(m_dma8237, state); break;
		case 2: i8237_dreq2_w(m_dma8237, state); break;
		case 3: i8237_dreq3_w(m_dma8237, state); break;
	}
}

UINT8 isa8_device::dack_r(int line) 
{
	UINT8 retVal = 0xff;
	for(int i=0;i<8;i++) {
		if (m_isa_device[i] != NULL && m_isa_device[i]->have_dack(line)) {
			retVal = m_isa_device[i]->dack_r(line);
		}		
	}
	return retVal;
}
void isa8_device::dack_w(int line,UINT8 data)
{
	for(int i=0;i<8;i++) {
		if (m_isa_device[i] != NULL && m_isa_device[i]->have_dack(line)) {
			m_isa_device[i]->dack_w(line,data);
		}		
	}
}
void isa8_device::eop_w(int state)
{
	for(int i=0;i<8;i++) {
		if (m_isa_device[i] != NULL) m_isa_device[i]->eop_w(state);
	}
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
	m_isa = machine.device<isa8_device>(m_isa8_card_config.m_isa_tag);
}


//-------------------------------------------------
//  ~device_isa8_card_interface - destructor
//-------------------------------------------------

device_isa8_card_interface::~device_isa8_card_interface()
{
}

void device_isa8_card_interface::interface_pre_start()
{    
	m_isa->add_isa_card(this, m_isa8_card_config.m_isa_num);
}

UINT8 device_isa8_card_interface::dack_r(int line)
{
	return 0;
}
void device_isa8_card_interface::dack_w(int line,UINT8 data)
{
}
void device_isa8_card_interface::eop_w(int state)
{
}

bool device_isa8_card_interface::have_dack(int line)
{
	return FALSE;
}
