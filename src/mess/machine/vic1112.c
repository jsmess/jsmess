#include "vic1112.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define M6522_0_TAG		"u4"
#define M6522_1_TAG		"u5"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type VIC1112 = vic1112_device_config::static_alloc_device_config;



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

GENERIC_DEVICE_CONFIG_SETUP(vic1112, "VIC-1112")


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void vic1112_device_config::device_config_complete()
{
	// inherit a copy of the static data
	const vic1112_interface *intf = reinterpret_cast<const vic1112_interface *>(static_config());
	if (intf != NULL)
		*static_cast<vic1112_interface *>(this) = *intf;

	// or initialize to defaults if none provided
	else
	{
		fatalerror("Interface not provided!");
	}

	m_bus_tag = intf->m_bus_tag;

	m_shortname = "vic1112";
}


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

const rom_entry *vic1112_device_config::device_rom_region() const
{
	return ROM_NAME( vic1112 );
}


//-------------------------------------------------
//  via6522_interface via0_intf
//-------------------------------------------------

WRITE_LINE_MEMBER( vic1112_device::via0_irq_w )
{
	m_via0_irq = state;

	device_set_input_line(m_machine.firstcpu, M6502_IRQ_LINE, (m_via0_irq | m_via1_irq) ? ASSERT_LINE : CLEAR_LINE);
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
	data |= ieee488_eoi_r(m_bus) << 3;

	/* data valid in */
	data |= ieee488_dav_r(m_bus) << 4;

	/* not ready for data in */
	data |= ieee488_nrfd_r(m_bus) << 5;

	/* not data accepted in */
	data |= ieee488_ndac_r(m_bus) << 6;

	/* attention in */
	data |= ieee488_atn_r(m_bus) << 7;

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
	ieee488_dav_w(m_bus, this, BIT(data, 0));

	/* not ready for data out */
	ieee488_nrfd_w(m_bus, this, BIT(data, 1));

	/* not data accepted out */
	ieee488_ndac_w(m_bus, this, BIT(data, 2));
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

	device_set_input_line(m_machine.firstcpu, M6502_IRQ_LINE, (m_via0_irq | m_via1_irq) ? ASSERT_LINE : CLEAR_LINE);
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

	return ieee488_dio_r(m_bus, 0);
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

	ieee488_dio_w(m_bus, this, data);
}


WRITE_LINE_MEMBER( vic1112_device::atn_w )
{
	/* attention out */
	ieee488_atn_w(m_bus, this, state);
}


READ_LINE_MEMBER( vic1112_device::srq_r )
{
	/* service request in */
	return ieee488_srq_r(m_bus);
}


WRITE_LINE_MEMBER( vic1112_device::eoi_w )
{
	/* end or identify out */
	ieee488_eoi_w(m_bus, this, state);
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
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor vic1112_device_config::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( vic1112 );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  vic1112_device - constructor
//-------------------------------------------------

vic1112_device::vic1112_device(running_machine &_machine, const vic1112_device_config &_config)
    : device_t(_machine, _config),
	  m_via0(*this, M6522_0_TAG),
	  m_via1(*this, M6522_1_TAG),
	  m_bus(m_machine.device(_config.m_bus_tag)),
      m_config(_config)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void vic1112_device::device_start()
{
	address_space *program = m_machine.firstcpu->memory().space(AS_PROGRAM);

	// map VIAs to VIC-20 address space
	program->install_readwrite_handler(0x9800, 0x980f, 0, 0, read8_delegate_create(via6522_device, read, *m_via0), write8_delegate_create(via6522_device, write, *m_via0));
	program->install_readwrite_handler(0x9810, 0x981f, 0, 0, read8_delegate_create(via6522_device, read, *m_via1), write8_delegate_create(via6522_device, write, *m_via1));

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
	ieee488_ifc_w(m_bus, this, 0);
	ieee488_ifc_w(m_bus, this, 1);
}


//-------------------------------------------------
//  srq_w - 
//-------------------------------------------------

WRITE_LINE_MEMBER( vic1112_device::srq_w )
{
	m_via1->write_cb1(state);
}
