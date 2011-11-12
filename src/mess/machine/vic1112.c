/**********************************************************************

    Commodore VIC-1112 IEEE-488 Interface Cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "vic1112.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define M6522_0_TAG		"u4"
#define M6522_1_TAG		"u5"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type VIC1112 = &device_creator<vic1112_device>;


//-------------------------------------------------
//  IEEE488_INTERFACE( ieee488_intf )
//-------------------------------------------------

static SLOT_INTERFACE_START( cbm_ieee488_devices )
	SLOT_INTERFACE("c2040", C2040)
	SLOT_INTERFACE("c3040", C3040)
	SLOT_INTERFACE("c4040", C4040)
	SLOT_INTERFACE("c8050", C8050)
	SLOT_INTERFACE("c8250", C8250)
	SLOT_INTERFACE("sfd1001", SFD1001)
	SLOT_INTERFACE("c2031", C2031)
	SLOT_INTERFACE("c8280", C8280)
	SLOT_INTERFACE("d9060", D9060)
	SLOT_INTERFACE("d9090", D9090)
SLOT_INTERFACE_END

IEEE488_INTERFACE( ieee488_intf )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(M6522_1_TAG, via6522_device, write_cb1),
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  ROM( vic1112 )
//-------------------------------------------------

ROM_START( vic1112 )
	ROM_REGION( 0x800, VIC1112_TAG, 0 )
	ROM_LOAD( "325329-03.u2", 0x000, 0x800, CRC(d37b6335) SHA1(828c965829d21c60e8c2d083caee045c639a270f) )
ROM_END


//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *vic1112_device::device_rom_region() const
{
	return ROM_NAME( vic1112 );
}


//-------------------------------------------------
//  via6522_interface via0_intf
//-------------------------------------------------

WRITE_LINE_MEMBER( vic1112_device::via0_irq_w )
{
	m_via0_irq = state;

	device_set_input_line(machine().firstcpu, M6502_IRQ_LINE, (m_via0_irq | m_via1_irq) ? ASSERT_LINE : CLEAR_LINE);
}


READ8_MEMBER( vic1112_device::via0_pb_r )
{
	/*

        bit     description

        PB0     _DAV OUT
        PB1     _NRFD OUT
        PB2     _NDAC OUT
        PB3     _EOI
        PB4     _DAV IN
        PB5     _NRFD IN
        PB6     _NDAC IN
        PB7     _ATN IN

    */

	UINT8 data = 0;

	/* end or identify */
	data |= m_bus->eoi_r() << 3;

	/* data valid in */
	data |= m_bus->dav_r() << 4;

	/* not ready for data in */
	data |= m_bus->nrfd_r() << 5;

	/* not data accepted in */
	data |= m_bus->ndac_r() << 6;

	/* attention in */
	data |= m_bus->atn_r() << 7;

	return data;
}


WRITE8_MEMBER( vic1112_device::via0_pb_w )
{
	/*

        bit     description

        PB0     _DAV OUT
        PB1     _NRFD OUT
        PB2     _NDAC OUT
        PB3     _EOI IN
        PB4     _DAV IN
        PB5     _NRFD IN
        PB6     _NDAC IN
        PB7     _ATN IN

    */

	/* data valid out */
	m_bus->dav_w(BIT(data, 0));

	/* not ready for data out */
	m_bus->nrfd_w(BIT(data, 1));

	/* not data accepted out */
	m_bus->ndac_w(BIT(data, 2));
}


static const via6522_interface via0_intf =
{
	DEVCB_NULL,
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, vic1112_device, via0_pb_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_NULL,
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, vic1112_device, via0_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, vic1112_device, via0_irq_w)
};


//-------------------------------------------------
//  via6522_interface via1_intf
//-------------------------------------------------

WRITE_LINE_MEMBER( vic1112_device::via1_irq_w )
{
	m_via1_irq = state;

	device_set_input_line(machine().firstcpu, M6502_IRQ_LINE, (m_via0_irq | m_via1_irq) ? ASSERT_LINE : CLEAR_LINE);
}

READ8_MEMBER( vic1112_device::dio_r )
{
	/*

        bit     description

        PB0     DI1
        PB1     DI2
        PB2     DI3
        PB3     DI4
        PB4     DI5
        PB5     DI6
        PB6     DI7
        PB7     DI8

    */

	return m_bus->dio_r();
}


WRITE8_MEMBER( vic1112_device::dio_w )
{
	/*

        bit     description

        PA0     DO1
        PA1     DO2
        PA2     DO3
        PA3     DO4
        PA4     DO5
        PA5     DO6
        PA6     DO7
        PA7     DO8

    */

	m_bus->dio_w(data);
}


WRITE_LINE_MEMBER( vic1112_device::atn_w )
{
	/* attention out */
	m_bus->atn_w(state);
}


READ_LINE_MEMBER( vic1112_device::srq_r )
{
	/* service request in */
	return m_bus->srq_r();
}


WRITE_LINE_MEMBER( vic1112_device::eoi_w )
{
	/* end or identify out */
	m_bus->eoi_w(state);
}


static const via6522_interface via1_intf =
{
	DEVCB_NULL,
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, vic1112_device, dio_r),
	DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, vic1112_device, srq_r),
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, vic1112_device, dio_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, vic1112_device, atn_w),
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, vic1112_device, eoi_w),

	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, vic1112_device, via1_irq_w)
};


//-------------------------------------------------
//  MACHINE_DRIVER( vic1112 )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( vic1112 )
	MCFG_VIA6522_ADD(M6522_0_TAG, 0, via0_intf)
	MCFG_VIA6522_ADD(M6522_1_TAG, 0, via1_intf)

	MCFG_IEEE488_BUS_ADD(ieee488_intf)
	MCFG_IEEE488_SLOT_ADD("ieee8", 8, cbm_ieee488_devices, NULL, NULL)
	MCFG_IEEE488_SLOT_ADD("ieee9", 9, cbm_ieee488_devices, NULL, NULL)
	MCFG_IEEE488_SLOT_ADD("ieee10", 10, cbm_ieee488_devices, NULL, NULL)
	MCFG_IEEE488_SLOT_ADD("ieee11", 11, cbm_ieee488_devices, NULL, NULL)
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor vic1112_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( vic1112 );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  vic1112_device - constructor
//-------------------------------------------------

vic1112_device::vic1112_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, VIC1112, "VIC1112", tag, owner, clock),
	  m_via0(*this, M6522_0_TAG),
	  m_via1(*this, M6522_1_TAG),
	  m_bus(*this, IEEE488_TAG)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void vic1112_device::device_start()
{
	address_space *program = machine().firstcpu->memory().space(AS_PROGRAM);

	// map VIAs to VIC-20 address space
	program->install_readwrite_handler(0x9800, 0x980f, 0, 0, read8_delegate(FUNC(via6522_device::read), (via6522_device*)m_via0), write8_delegate(FUNC(via6522_device::write), (via6522_device*)m_via0));
	program->install_readwrite_handler(0x9810, 0x981f, 0, 0, read8_delegate(FUNC(via6522_device::read), (via6522_device*)m_via1), write8_delegate(FUNC(via6522_device::write), (via6522_device*)m_via1));

	// map ROM to VIC-20 address space
	program->install_rom(0xb000, 0xb7ff, subregion(VIC1112_TAG)->base());

	// state saving
	save_item(NAME(m_via0_irq));
	save_item(NAME(m_via1_irq));
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void vic1112_device::device_reset()
{
	m_bus->ifc_w(0);
	m_bus->ifc_w(1);
}
