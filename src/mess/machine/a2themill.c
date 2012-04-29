/*********************************************************************

    a2themill.c

    Implementation of the Stellation Two The Mill 6809 card
 
    OS9 requires this add-on be implemented:
    http://mirrors.apple2.org.za/Apple%20II%20Documentation%20Project/Interface%20Cards/CPU/Stellation%20Two%20The%20Mill%20-%206809/Photos/Stellation%20Two%20The%20Mill%20OS9%20ROM%20Interface%20Rev.%20A%20-%20Front.jpg
 
    but the chip numbers are unclear, and it may also require a ROM.
 
    Excel Flex requires a ROM in the on-board socket which is not dumped :(

*********************************************************************/

#include "a2themill.h"
#include "includes/apple2.h"
#include "cpu/m6809/m6809.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define MILL_VERBOSE (0)

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type A2BUS_THEMILL = &device_creator<a2bus_themill_device>;

#define M6809_TAG         "m6809"

static ADDRESS_MAP_START( m6809_mem, AS_PROGRAM, 8, a2bus_themill_device )
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(dma_r, dma_w)
ADDRESS_MAP_END

MACHINE_CONFIG_FRAGMENT( a2themill )
	MCFG_CPU_ADD(M6809_TAG, M6809, 1021800)   // M6809 runs at ~1 MHz as per Stellation Two's print ads
	MCFG_CPU_PROGRAM_MAP(m6809_mem)
MACHINE_CONFIG_END

/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor a2bus_themill_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( a2themill );
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

a2bus_themill_device::a2bus_themill_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock) :
    device_t(mconfig, type, name, tag, owner, clock),
    device_a2bus_card_interface(mconfig, *this),
    m_6809(*this, M6809_TAG),
    m_6502space(NULL)
{
	m_shortname = "a2themill";
}

a2bus_themill_device::a2bus_themill_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
    device_t(mconfig, A2BUS_THEMILL, "Microsoft SoftCard", tag, owner, clock),
    device_a2bus_card_interface(mconfig, *this),
    m_6809(*this, M6809_TAG),
    m_6502space(NULL)
{
	m_shortname = "a2themill";
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void a2bus_themill_device::device_start()
{
	// set_a2bus_device makes m_slot valid
	set_a2bus_device();

	save_item(NAME(m_bEnabled));
	save_item(NAME(m_FirstBoot));
	save_item(NAME(m_flipAddrSpace));
	save_item(NAME(m_status));
}

void a2bus_themill_device::device_reset()
{
    m_bEnabled = false;
    m_6502space = NULL;
    m_FirstBoot = true;
    m_flipAddrSpace = false;
    m_status = 0xc0;    // OS9 loader relies on this
    device_set_input_line(m_6809, INPUT_LINE_HALT, ASSERT_LINE);
    device_set_input_line(m_6809, INPUT_LINE_RESET, ASSERT_LINE);
}

UINT8 a2bus_themill_device::read_c0nx(address_space &space, UINT8 offset)
{
    return m_status;
}

void a2bus_themill_device::write_c0nx(address_space &space, UINT8 offset, UINT8 data)
{
    switch (offset)
    {
        case 0: // 6502 IRQ
            if (data & 0x80)
            {
                m_status |= 0x01;
                lower_slot_irq();
            }
            else
            {
                m_status &= ~0x01;
                raise_slot_irq();
            }
            break;

        case 2: // 6809 reset
            if (data & 0x80)
            {
                // steal the 6502's address space
                m_6502space = &space;
                m_6809->reset();

                device_set_input_line(m_6809, INPUT_LINE_HALT, CLEAR_LINE);
                device_set_input_line(m_6809, INPUT_LINE_RESET, CLEAR_LINE);

                m_bEnabled = true;
                m_status &= ~0x04;
            }
            else
            {
                device_set_input_line(m_6809, INPUT_LINE_HALT, ASSERT_LINE);
                m_bEnabled = false;
                m_status |= 0x04;
            }
            break;

        case 1: // 6809 halt
            if (data & 0x80)    // release reset
            {
                m_status |= 0x02;
            }
            else
            {
                m_6809->reset();
                m_status &= ~0x02;
            }
            break;
        
        case 3: // 6809 NMI
            if (data & 0x80)
            {
                device_set_input_line(m_6809, INPUT_LINE_NMI, CLEAR_LINE);
                m_status |= 0x08;
            }
            else
            {
                device_set_input_line(m_6809, INPUT_LINE_NMI, ASSERT_LINE);
                m_status &= ~0x08;
            }
            break;

        case 4: // 6809 FIRQ
            if (data & 0x80)
            {
                device_set_input_line(m_6809, M6809_FIRQ_LINE, CLEAR_LINE);
                m_status |= 0x10;
            }
            else
            {
                device_set_input_line(m_6809, M6809_FIRQ_LINE, ASSERT_LINE);
                m_status &= ~0x10;
            }
            break;

        case 5: // 6809 IRQ
            if (data & 0x80)
            {
                device_set_input_line(m_6809, M6809_IRQ_LINE, CLEAR_LINE);
                m_status |= 0x20;
            }
            else
            {
                device_set_input_line(m_6809, M6809_IRQ_LINE, ASSERT_LINE);
                m_status &= ~0x20;
            }
            break;

        case 6:
            if (data & 0x80)    // enable ROM socket
            {
                m_status |= 0x40;
                printf("The Mill: on-board ROM socket enabled; because none of these ROMs are dumped, the 6809 will not run!\n");
            }
            else
            {
                m_status &= ~0x40;
            }
            break;

        case 7: // 6809 mapping
            if (data & 0x80)
            {
                m_status |= 0x80;
                m_flipAddrSpace = false;
            }
            else
            {
                m_status &= ~0x80;
                m_flipAddrSpace = true;
            }
            break;

        case 0xa:   // addresses >= 0x8 are direct status writes?  "Excel Flex 9" disc seems to indicate so.
            m_status = data;
            break;

        default:
            printf("The Mill: %02x to unhandled c0n%x\n", data, offset);
            break;
    }
}

READ8_MEMBER( a2bus_themill_device::dma_r )
{
    if (m_6502space)
    {
        if (m_flipAddrSpace)
        {
            return m_6502space->read_byte(offset^0x8000);
        }
        else
        {
            return m_6502space->read_byte(offset);
        }
    }

    return 0xff;
}


//-------------------------------------------------
//  dma_w -
//-------------------------------------------------

WRITE8_MEMBER( a2bus_themill_device::dma_w )
{
    if (m_6502space)
    {
        if (m_flipAddrSpace)
        {
            m_6502space->write_byte(offset^0x8000, data);
        }
        else
        {
            m_6502space->write_byte(offset, data);
        }
    }
}

bool a2bus_themill_device::take_c800()
{
    return false;
}

