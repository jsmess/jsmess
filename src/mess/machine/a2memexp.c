/*********************************************************************

    a2memexp.c

    Implementation of the Apple II Memory Expansion Card

*********************************************************************/

#include "a2memexp.h"
#include "includes/apple2.h"


/***************************************************************************
    PARAMETERS
***************************************************************************/

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type A2BUS_MEMEXP = &device_creator<a2bus_memexpapple_device>;

#define MEMEXP_ROM_REGION  "memexp_rom"

MACHINE_CONFIG_FRAGMENT( memexp2 )
MACHINE_CONFIG_END

ROM_START( memexp2 )
	ROM_REGION(0x1000, MEMEXP_ROM_REGION, 0)
    ROM_LOAD( "341-0344a.bin", 0x0000, 0x1000, CRC(1e994e17) SHA1(6e823a1fa40ed37eeddcef23f5df24da2ea1463e) ) 
ROM_END

/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor a2bus_memexp_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( memexp2 );
}

//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *a2bus_memexp_device::device_rom_region() const
{
	return ROM_NAME( memexp2 );
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

a2bus_memexp_device::a2bus_memexp_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock) :
    device_t(mconfig, type, name, tag, owner, clock),
    device_a2bus_card_interface(mconfig, *this)
{
}

a2bus_memexpapple_device::a2bus_memexpapple_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
    a2bus_memexp_device(mconfig, A2BUS_MEMEXP, "Apple II Memory Expansion Card", tag, owner, clock)
{
	m_shortname = "a2memexp";
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void a2bus_memexp_device::device_start()
{
	// set_a2bus_device makes m_slot valid
	set_a2bus_device();

	astring tempstring;
	m_rom = device().machine().region(this->subtag(tempstring, MEMEXP_ROM_REGION))->base();

    memset(m_ram, 0xff, 1024*1024*sizeof(UINT8));
}

void a2bus_memexp_device::device_reset()
{
    memset(m_regs, 0, sizeof(UINT8) * 0x10);
    m_wptr = m_liveptr = 0;
}


/*-------------------------------------------------
    read_c0nx - called for reads from this card's c0nx space
-------------------------------------------------*/

UINT8 a2bus_memexp_device::read_c0nx(address_space &space, UINT8 offset)
{
    UINT8 retval = m_regs[offset];

    if (offset == 3)
    {
        retval = m_ram[m_liveptr&0xfffff];
//        printf("Read RAM[%x] = %02x\n", m_liveptr, retval);
        m_liveptr++;
        m_regs[0] = m_liveptr & 0xff;
        m_regs[1] = (m_liveptr>>8) & 0xff;
        m_regs[2] = ((m_liveptr>>16) & 0xff) | 0xf0;
    }

//    printf("Read c0n%x (PC=%x) = %02x\n", offset, cpu_get_pc(&space.device()), retval);

	return retval;
}


/*-------------------------------------------------
    write_c0nx - called for writes to this card's c0nx space 
-------------------------------------------------*/

void a2bus_memexp_device::write_c0nx(address_space &space, UINT8 offset, UINT8 data)
{
//    printf("Write %02x to c0n%x (PC=%x)\n", data, offset, cpu_get_pc(&space.device()));

    switch (offset)
    {
        case 0:
            m_wptr &= ~0xff;
            m_wptr |= data;
            m_regs[0] = m_wptr & 0xff;
            m_regs[1] = (m_wptr>>8) & 0xff;
            m_regs[2] = ((m_wptr>>16) & 0xff) | 0xf0;
            m_liveptr = m_wptr;
            break;

        case 1:
            m_wptr &= ~0xff00;
            m_wptr |= (data<<8);
            m_regs[0] = m_wptr & 0xff;
            m_regs[1] = (m_wptr>>8) & 0xff;
            m_regs[2] = ((m_wptr>>16) & 0xff) | 0xf0;
            m_liveptr = m_wptr;
            break;

        case 2:
            m_wptr &= ~0xff0000;
            m_wptr |= (data<<16);
            m_regs[0] = m_wptr & 0xff;
            m_regs[1] = (m_wptr>>8) & 0xff;
            m_regs[2] = ((m_wptr>>16) & 0xff) | 0xf0;
            m_liveptr = m_wptr;
            break;

        case 3:
//            printf("Write %02x to RAM[%x]\n", data, m_liveptr);
            m_ram[(m_liveptr&0xfffff)] = data;
            m_liveptr++;
            m_regs[0] = m_liveptr & 0xff;
            m_regs[1] = (m_liveptr>>8) & 0xff;
            m_regs[2] = ((m_liveptr>>16) & 0xff) | 0xf0;
            break;

        default:
            m_regs[offset] = data;
            break;
    }
}

/*-------------------------------------------------
    read_cnxx - called for reads from this card's cnxx space
-------------------------------------------------*/

UINT8 a2bus_memexp_device::read_cnxx(address_space &space, UINT8 offset)
{
    int slotimg = m_slot * 0x100;

    // first 0x400 of ROM contains a CnXX image for each of slots 1-7, last 0x400 is c800 image
    return m_rom[offset+slotimg];
}

/*-------------------------------------------------
    read_c800 - called for reads from this card's c800 space
-------------------------------------------------*/

UINT8 a2bus_memexp_device::read_c800(address_space &space, UINT16 offset)
{
    return m_rom[offset+0x800];
}
