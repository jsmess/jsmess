/**********************************************************************

    Commodore 1551 Single Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

    TODO:

    - byte latching does not match hardware behavior
      (CPU skips data bytes if implemented per schematics)
    - activity LED

*/

#include "c1551.h"
#include "machine/devhelpr.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define M6510T_TAG		"u2"
#define M6523_0_TAG		"u3"
#define M6523_1_TAG		"ci_u2"
#define C64H156_TAG		"u6"

enum
{
	LED_POWER = 0,
	LED_ACT
};



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type C1551 = c1551_device_config::static_alloc_device_config;



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

GENERIC_DEVICE_CONFIG_SETUP(c1551, "C1551");


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void c1551_device_config::device_config_complete()
{
	m_shortname = "c1551";
}


//-------------------------------------------------
//  static_set_config - configuration helper
//-------------------------------------------------

void c1551_device_config::static_set_config(device_config *device, int address)
{
	c1551_device_config *c1551 = downcast<c1551_device_config *>(device);

	assert((address > 7) && (address < 10));

	c1551->m_address = address - 8;
}


//-------------------------------------------------
//  ROM( c1551 )
//-------------------------------------------------

ROM_START( c1551 ) // schematic 251860
	ROM_REGION( 0x4000, "c1551", 0 )
	ROM_LOAD( "318001-01.u4", 0x0000, 0x4000, CRC(6d16d024) SHA1(fae3c788ad9a6cc2dbdfbcf6c0264b2ca921d55e) )
ROM_END


//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *c1551_device_config::device_rom_region() const
{
	return ROM_NAME( c1551 );
}


//-------------------------------------------------
//  m6502_interface m6510t_intf
//-------------------------------------------------

READ8_MEMBER( c1551_device::port_r )
{
	/*

        bit     description

        P0
        P1
        P2
        P3
        P4      WPS
        P5
        P6
        P7      BYTE LTCHED

    */

	UINT8 data = 0;

	// write protect sense
	data |= (!floppy_wpt_r(m_image)) << 4;

	// byte latched
	data |= m_ga->atn_r() << 7;

	return data;
}


WRITE8_MEMBER( c1551_device::port_w )
{
	/*

        bit     description

        P0      STP0A
        P1      STP0B
        P2      MTR0
        P3      ACT0
        P4
        P5      DS0
        P6      DS1
        P7

    */

	// spindle motor
	m_ga->mtr_w(BIT(data, 2));

	// stepper motor
	m_ga->stp_w(data & 0x03);

	// activity LED
	output_set_led_value(LED_ACT, BIT(data, 3));

	// density select
	m_ga->ds_w((data >> 5) & 0x03);
}


static const m6502_interface m6510t_intf =
{
	NULL,			// read_indexed_func
	NULL,			// write_indexed_func
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, c1551_device, port_r),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, c1551_device, port_w)
};


//-------------------------------------------------
//  tpi6525_interface tpi0_intf
//-------------------------------------------------

READ8_MEMBER( c1551_device::tcbm_data_r )
{
	/*

        bit     description

        PA0     TCBM PA0
        PA1     TCBM PA1
        PA2     TCBM PA2
        PA3     TCBM PA3
        PA4     TCBM PA4
        PA5     TCBM PA5
        PA6     TCBM PA6
        PA7     TCBM PA7

    */

	return m_tcbm_data;
}


WRITE8_MEMBER( c1551_device::tcbm_data_w )
{
	/*

        bit     description

        PA0     TCBM PA0
        PA1     TCBM PA1
        PA2     TCBM PA2
        PA3     TCBM PA3
        PA4     TCBM PA4
        PA5     TCBM PA5
        PA6     TCBM PA6
        PA7     TCBM PA7

    */

	m_tcbm_data = data;
}


READ8_MEMBER( c1551_device::tpi0_pc_r )
{
	/*

        bit     description

        PC0
        PC1
        PC2
        PC3
        PC4
        PC5     JP1
        PC6     _SYNC
        PC7     TCBM DAV

    */

	UINT8 data = 0;

	// JP1
	data |= m_config.m_address << 5;

	// SYNC detect line
	data |= m_ga->sync_r() << 6;

	// TCBM data valid
	data |= m_dav << 7;

	return data;
}


WRITE8_MEMBER( c1551_device::tpi0_pc_w )
{
	/*

        bit     description

        PC0     TCBM STATUS0
        PC1     TCBM STATUS1
        PC2     TCBM DEV
        PC3     TCBM ACK
        PC4     MODE
        PC5
        PC6
        PC7

    */

	// TCBM status
	m_status = data & 0x03;

	// TODO TCBM device number

	// TCBM acknowledge
	m_ack = BIT(data, 3);

	// read/write mode
	m_ga->oe_w(BIT(data, 4));
}


static const tpi6525_interface tpi0_intf =
{
	DEVCB_NULL,
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, c1551_device, tcbm_data_r),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, c1551_device, tcbm_data_w),
	DEVCB_DEVICE_MEMBER(C64H156_TAG, c64h156_device, yb_r),
	DEVCB_DEVICE_MEMBER(C64H156_TAG, c64h156_device, yb_w),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, c1551_device, tpi0_pc_r),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, c1551_device, tpi0_pc_w),
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  tpi6525_interface tpi1_intf
//-------------------------------------------------

READ8_MEMBER( c1551_device::tpi1_pb_r )
{
	/*

        bit     description

        PB0     STATUS0
        PB1     STATUS1
        PB2
        PB3
        PB4
        PB5
        PB6
        PB7

    */

	return m_status & 0x03;
}


READ8_MEMBER( c1551_device::tpi1_pc_r )
{
	/*

        bit     description

        PC0
        PC1
        PC2
        PC3
        PC4
        PC5
        PC6
        PC7     TCBM ACK

    */

	UINT8 data = 0;

	// TCBM acknowledge
	data |= m_ack << 7;

	return data;
}


WRITE8_MEMBER( c1551_device::tpi1_pc_w )
{
	/*

        bit     description

        PC0
        PC1
        PC2
        PC3
        PC4
        PC5
        PC6     TCBM DAV
        PC7

    */

	// TCBM data valid
	m_dav = BIT(data, 6);
}


static const tpi6525_interface tpi1_intf =
{
	DEVCB_NULL,
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, c1551_device, tcbm_data_r),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, c1551_device, tcbm_data_w),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, c1551_device, tpi1_pb_r),
	DEVCB_NULL,
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, c1551_device, tpi1_pc_r),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, c1551_device, tpi1_pc_w),
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  ADDRESS_MAP( c1551_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( c1551_mem, AS_PROGRAM, 8, c1551_device )
	AM_RANGE(0x0000, 0x07ff) AM_MIRROR(0x0800) AM_RAM
	AM_RANGE(0x4000, 0x4007) AM_MIRROR(0x3ff8) AM_DEVREADWRITE_LEGACY(M6523_0_TAG, tpi6525_r, tpi6525_w)
	AM_RANGE(0xc000, 0xffff) AM_ROM AM_REGION("c1551:c1551", 0)
ADDRESS_MAP_END


//-------------------------------------------------
//  FLOPPY_OPTIONS( c1551 )
//-------------------------------------------------

static FLOPPY_OPTIONS_START( c1551 )
	FLOPPY_OPTION( c1551, "g64", "Commodore 1551 GCR Disk Image", g64_dsk_identify, g64_dsk_construct, NULL )
	FLOPPY_OPTION( c1551, "d64", "Commodore 1551 Disk Image", d64_dsk_identify, d64_dsk_construct, NULL )
FLOPPY_OPTIONS_END


//-------------------------------------------------
//  floppy_config c1551_floppy_config
//-------------------------------------------------

static const floppy_config c1551_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_SSDD,
	FLOPPY_OPTIONS_NAME(c1551),
	NULL
};


//-------------------------------------------------
//  C64H156_INTERFACE( ga_intf )
//-------------------------------------------------

static C64H156_INTERFACE( ga_intf )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE_MEMBER(c64h156_device, atni_w)
};


//-------------------------------------------------
//  MACHINE_DRIVER( c1551 )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( c1551 )
	MCFG_CPU_ADD(M6510T_TAG, M6510T, XTAL_16MHz/8)
	MCFG_CPU_PROGRAM_MAP(c1551_mem)
	MCFG_CPU_CONFIG(m6510t_intf)

	MCFG_TPI6525_ADD(M6523_0_TAG, tpi0_intf)
	MCFG_TPI6525_ADD(M6523_1_TAG, tpi1_intf)

	MCFG_FLOPPY_DRIVE_ADD(FLOPPY_0, c1551_floppy_config)
	MCFG_64H156_ADD(C64H156_TAG, XTAL_16MHz, ga_intf)
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor c1551_device_config::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( c1551 );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  c1551_device - constructor
//-------------------------------------------------

c1551_device::c1551_device(running_machine &_machine, const c1551_device_config &_config)
    : device_t(_machine, _config),
	  m_maincpu(*this, M6510T_TAG),
	  m_tpi0(*this, M6523_0_TAG),
	  m_tpi1(*this, M6523_1_TAG),
	  m_ga(*this, C64H156_TAG),
	  m_image(*this, FLOPPY_0),
	  m_tcbm_data(0),
	  m_status(0),
	  m_dav(0),
	  m_ack(0),
      m_config(_config)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void c1551_device::device_start()
{
	// allocate timers
	m_irq_timer = timer_alloc();
	m_irq_timer->adjust(attotime::from_hz(120), 0, attotime::from_hz(120));

	// map TPI1 to host CPU memory space
	address_space *program = machine().firstcpu->memory().space(AS_PROGRAM);
	UINT32 start_address = m_config.m_address ? 0xfec0 : 0xfef0;

	program->install_legacy_readwrite_handler(*m_tpi1, start_address, start_address + 7, FUNC(tpi6525_r), FUNC(tpi6525_w));

	// install image callbacks
	floppy_install_unload_proc(m_image, c1551_device::on_disk_change);
	floppy_install_load_proc(m_image, c1551_device::on_disk_change);

	// register for state saving
	save_item(NAME(m_tcbm_data));
	save_item(NAME(m_status));
	save_item(NAME(m_dav));
	save_item(NAME(m_ack));
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void c1551_device::device_reset()
{
	// initialize gate array
	m_ga->test_w(1);
	m_ga->soe_w(1);
	m_ga->accl_w(1);
	m_ga->atna_w(1);
}


//-------------------------------------------------
//  device_timer - handler timer events
//-------------------------------------------------

void c1551_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	m_maincpu->set_input_line(M6502_IRQ_LINE, ASSERT_LINE);
	m_maincpu->set_input_line(M6502_IRQ_LINE, CLEAR_LINE);
}


//-------------------------------------------------
//  on_disk_change -
//-------------------------------------------------

void c1551_device::on_disk_change(device_image_interface &image)
{
    c1551_device *c1551 = static_cast<c1551_device *>(image.device().owner());

	c1551->m_ga->on_disk_changed();
}
