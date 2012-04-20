/*********************************************************************

    a2mockingboard.c

    Implementation of the Sweet Micro Systems Mockingboard card

*********************************************************************/

#include "emu.h"
#include "machine/a2mockingboard.h"


/***************************************************************************
    PARAMETERS
***************************************************************************/

#define VIA1_TAG "mockbd_via1"
#define VIA2_TAG "mockbd_via2"
#define AY1_TAG "mockbd_ay1"
#define AY2_TAG "mockbd_ay2"

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type A2BUS_MOCKINGBOARD = &device_creator<a2bus_mockingboard_device>;

static const ay8910_interface mockingboard_ay8910_interface =
{
   AY8910_LEGACY_OUTPUT,
   AY8910_DEFAULT_LOADS,
   DEVCB_NULL
};

const via6522_interface mockingboard_via1_intf =
{
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, a2bus_mockingboard_device, via1_in_a), DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, a2bus_mockingboard_device, via1_in_b),
	DEVCB_NULL, DEVCB_NULL,
	DEVCB_NULL, DEVCB_NULL,
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, a2bus_mockingboard_device, via1_out_a), DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, a2bus_mockingboard_device, via1_out_b),
	DEVCB_NULL, DEVCB_NULL,
	DEVCB_NULL, DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, a2bus_mockingboard_device, via1_irq_w)
};

const via6522_interface mockingboard_via2_intf =
{
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, a2bus_mockingboard_device, via2_in_a), DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, a2bus_mockingboard_device, via2_in_b),
	DEVCB_NULL, DEVCB_NULL,
	DEVCB_NULL, DEVCB_NULL,
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, a2bus_mockingboard_device, via2_out_a), DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, a2bus_mockingboard_device, via2_out_b),
	DEVCB_NULL, DEVCB_NULL,
	DEVCB_NULL, DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, a2bus_mockingboard_device, via2_irq_w)
};

MACHINE_CONFIG_FRAGMENT( mockingboard )
	MCFG_VIA6522_ADD(VIA1_TAG, 1022727, mockingboard_via1_intf)
	MCFG_VIA6522_ADD(VIA2_TAG, 1022727, mockingboard_via2_intf)

    MCFG_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")
    MCFG_SOUND_ADD(AY1_TAG, AY8913, 1022727)
    MCFG_SOUND_CONFIG(mockingboard_ay8910_interface)
    MCFG_SOUND_ROUTE(ALL_OUTPUTS, "lspeaker", 1.0)
    MCFG_SOUND_ADD(AY2_TAG, AY8913, 1022727)
    MCFG_SOUND_CONFIG(mockingboard_ay8910_interface)
    MCFG_SOUND_ROUTE(ALL_OUTPUTS, "rspeaker", 1.0)
MACHINE_CONFIG_END

//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor a2bus_mockingboard_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( mockingboard );
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

a2bus_mockingboard_device::a2bus_mockingboard_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
    device_t(mconfig, A2BUS_MOCKINGBOARD, "Sweet Micro Systems Mockingboard", tag, owner, clock),
    device_a2bus_card_interface(mconfig, *this),
    m_via1(*this, VIA1_TAG),
    m_via2(*this, VIA2_TAG),
    m_ay1(*this, AY1_TAG),
    m_ay2(*this, AY2_TAG)
{
	m_shortname = "a2mockbd";
}

a2bus_mockingboard_device::a2bus_mockingboard_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock) :
    device_t(mconfig, type, name, tag, owner, clock),
    device_a2bus_card_interface(mconfig, *this),
    m_via1(*this, VIA1_TAG),
    m_via2(*this, VIA2_TAG),
    m_ay1(*this, AY1_TAG),
    m_ay2(*this, AY2_TAG)
{
	m_shortname = "a2mockbd";
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void a2bus_mockingboard_device::device_start()
{
	// set_a2bus_device makes m_slot valid
	set_a2bus_device();

	save_item(NAME(m_porta1));
	save_item(NAME(m_porta2));
}

void a2bus_mockingboard_device::device_reset()
{
    m_porta1 = m_porta2 = 0;
}

/*-------------------------------------------------
    read_cnxx - called for reads from this card's c0nx space
-------------------------------------------------*/

UINT8 a2bus_mockingboard_device::read_cnxx(address_space &space, UINT8 offset)
{
    if (offset <= 0x10)
    {
        return m_via1->read(space, offset);
    }
    if (offset >= 0x80 && offset <= 0x90)
    {
        return m_via2->read(space, offset & 0xf);
    }

    return 0;
}

/*-------------------------------------------------
    write_cnxx - called for writes to this card's c0nx space
-------------------------------------------------*/

void a2bus_mockingboard_device::write_cnxx(address_space &space, UINT8 offset, UINT8 data)
{
    if (offset <= 0x10)
    {
        m_via1->write(space, offset, data);
    }
    else if (offset >= 0x80 && offset <= 0x90)
    {
        m_via2->write(space, offset & 0xf, data);
    }
    else
    {
        printf("Mockingboard(%d): unk write %02x to Cn%02X\n", m_slot, data, offset);
    }
}


WRITE_LINE_MEMBER( a2bus_mockingboard_device::via1_irq_w )
{
    if (state)
    {
        raise_slot_irq();
    }
    else
    {
        lower_slot_irq();
    }
}

WRITE_LINE_MEMBER( a2bus_mockingboard_device::via2_irq_w )
{
    if (state)
    {
        raise_slot_nmi();
    }
    else
    {
        lower_slot_nmi();
    }
}

READ8_MEMBER( a2bus_mockingboard_device::via1_in_a )
{
	return 0;
}

WRITE8_MEMBER( a2bus_mockingboard_device::via1_out_a )
{
    m_porta1 = data;
}

READ8_MEMBER( a2bus_mockingboard_device::via1_in_b )
{
	return 0;
}

WRITE8_MEMBER( a2bus_mockingboard_device::via1_out_b )
{
    if (!(data & 8))
    {
        ay8910_reset_w(m_ay1, 0, 0);
    }
    else
    {
        switch (data & 3)
        {
            case 0: // BDIR=0, BC1=0 (inactive)
                break;

            case 1: // BDIR=0, BC1=1 (read PSG)
                m_porta1 = ay8910_r(m_ay1, 0);
                break;

            case 2: // BDIR=1, BC1=0 (write PSG)
                ay8910_data_w(m_ay1, 0, m_porta1);
                break;

            case 3: // BDIR=1, BC1=1 (latch)
                ay8910_address_w(m_ay1, 0, m_porta1);
                break;
        }
    }
}

READ8_MEMBER( a2bus_mockingboard_device::via2_in_a )
{
	return 0;
}

WRITE8_MEMBER( a2bus_mockingboard_device::via2_out_a )
{
    m_porta2 = data;
}

READ8_MEMBER( a2bus_mockingboard_device::via2_in_b )
{
	return 0;
}

WRITE8_MEMBER( a2bus_mockingboard_device::via2_out_b )
{
    if (!(data & 8))
    {
        ay8910_reset_w(m_ay2, 0, 0);
    }
    else
    {
        switch (data & 3)
        {
            case 0: // BDIR=0, BC1=0 (inactive)
                break;

            case 1: // BDIR=0, BC1=1 (read PSG)
                m_porta2 = ay8910_r(m_ay2, 0);
                break;

            case 2: // BDIR=1, BC1=0 (write PSG)
                ay8910_data_w(m_ay2, 0, m_porta2);
                break;

            case 3: // BDIR=1, BC1=1 (latch)
                ay8910_address_w(m_ay2, 0, m_porta2);
                break;
        }
    }
}
