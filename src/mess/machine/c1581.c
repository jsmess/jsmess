/**********************************************************************

    Commodore 1581/1563 Single Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "c1581.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define M6502_TAG		"u1"
#define M8520_TAG		"u5"
#define WD1770_TAG		"u4"


enum
{
	LED_POWER = 0,
	LED_ACT
};



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type C1581 = c1581_device_config::static_alloc_device_config;
const device_type C1563 = c1581_device_config::static_alloc_device_config;



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************


//-------------------------------------------------
//  c1581_device_config - constructor
//-------------------------------------------------

c1581_device_config::c1581_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
	: device_config(mconfig, static_alloc_device_config, "C1581", tag, owner, clock),
	  device_config_cbm_iec_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  static_alloc_device_config - allocate a new
//  configuration object
//-------------------------------------------------

device_config *c1581_device_config::static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
{
	return global_alloc(c1581_device_config(mconfig, tag, owner, clock));
}


//-------------------------------------------------
//  alloc_device - allocate a new device object
//-------------------------------------------------

device_t *c1581_device_config::alloc_device(running_machine &machine) const
{
	return auto_alloc(machine, c1581_device(machine, *this));
}


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void c1581_device_config::device_config_complete()
{
	m_shortname = "c1581";
}


//-------------------------------------------------
//  static_set_config - configuration helper
//-------------------------------------------------

void c1581_device_config::static_set_config(device_config *device, int address, int variant)
{
	c1581_device_config *c1581 = downcast<c1581_device_config *>(device);

	assert((address > 7) && (address < 12));

	c1581->m_address = address - 8;
	c1581->m_variant = variant;
}


//-------------------------------------------------
//  ROM( c1581 )
//-------------------------------------------------

ROM_START( c1581 )
	ROM_REGION( 0x8000, M6502_TAG, 0 )
	ROM_LOAD_OPTIONAL( "jiffydos 1581.u2", 0x0000, 0x8000, CRC(98873d0f) SHA1(65bbf2be7bcd5bdcbff609d6c66471ffb9d04bfe) )
	ROM_LOAD_OPTIONAL( "beta.u2",	       0x0000, 0x8000, CRC(ecc223cd) SHA1(a331d0d46ead1f0275b4ca594f87c6694d9d9594) )
	ROM_LOAD_OPTIONAL( "318045-01.u2",	   0x0000, 0x8000, CRC(113af078) SHA1(3fc088349ab83e8f5948b7670c866a3c954e6164) )
	ROM_LOAD(		   "318045-02.u2",	   0x0000, 0x8000, CRC(a9011b84) SHA1(01228eae6f066bd9b7b2b6a7fa3f667e41dad393) )
ROM_END


//-------------------------------------------------
//  ROM( c1563 )
//-------------------------------------------------

ROM_START( c1563 )
	ROM_REGION( 0x8000, M6502_TAG, 0 )
	ROM_LOAD( "1563-rom.bin",     0x0000, 0x8000, CRC(1d184687) SHA1(2c5111a9c15be7b7955f6c8775fea25ec10c0ca0) )
ROM_END


//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *c1581_device_config::device_rom_region() const
{
	switch (m_variant)
	{
	default:
	case TYPE_1581:
		return ROM_NAME( c1581 );

	case TYPE_1563:
		return ROM_NAME( c1563 );
	}
}


//-------------------------------------------------
//  ADDRESS_MAP( c1581_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( c1581_mem, AS_PROGRAM, 8, c1581_device )
	AM_RANGE(0x0000, 0x1fff) AM_MIRROR(0x2000) AM_RAM
	AM_RANGE(0x4000, 0x400f) AM_MIRROR(0x1ff0) AM_DEVREADWRITE_LEGACY(M8520_TAG, mos6526_r, mos6526_w)
	AM_RANGE(0x6000, 0x6003) AM_MIRROR(0x1ffc) AM_DEVREADWRITE_LEGACY(WD1770_TAG, wd17xx_r, wd17xx_w)
	AM_RANGE(0x8000, 0xffff) // AM_ROM
ADDRESS_MAP_END


//-------------------------------------------------
//  MOS8520_INTERFACE( cia_intf )
//-------------------------------------------------

WRITE_LINE_MEMBER( c1581_device::cnt_w )
{
	// fast serial clock out
	m_cnt_out = state;

	set_iec_srq();
}


WRITE_LINE_MEMBER( c1581_device::sp_w )
{
	// fast serial data out
	m_sp_out = state;

	set_iec_data();
}


READ8_MEMBER( c1581_device::cia_pa_r )
{
	/*

        bit     description

        PA0
        PA1     /RDY
        PA2
        PA3     DEV# SEL (SW1)
        PA4     DEV# SEL (SW1)
        PA5
        PA6
        PA7     /DISK CHNG

    */

	UINT8 data = 0;

	// ready
	data |= !(floppy_drive_get_flag_state(m_image, FLOPPY_DRIVE_READY) == FLOPPY_DRIVE_READY) << 1;

	// device number
	data |= m_config.m_address << 3;

	// disk change
	data |= floppy_dskchg_r(m_image) << 7;

	return data;
}


WRITE8_MEMBER( c1581_device::cia_pa_w )
{
	/*

        bit     description

        PA0     SIDE0
        PA1
        PA2     /MOTOR
        PA3
        PA4
        PA5     POWER LED
        PA6     ACT LED
        PA7

    */

	// side 0
	wd17xx_set_side(m_fdc, !BIT(data, 0));

	// motor
	int motor = BIT(data, 2);
	floppy_mon_w(m_image, motor);
	floppy_drive_set_ready_state(m_image, !motor, 1);

	// power led
	output_set_led_value(LED_POWER, BIT(data, 5));

	// activity led
	output_set_led_value(LED_ACT, BIT(data, 6));
}


READ8_MEMBER( c1581_device::cia_pb_r )
{
	/*

        bit     description

        PB0     DATA IN
        PB1
        PB2     CLK IN
        PB3
        PB4
        PB5
        PB6     /WPRT
        PB7     ATN IN

    */

	UINT8 data = 0;

	// data in
	data = !m_bus->data_r();

	// clock in
	data |= !m_bus->clk_r() << 2;

	// write protect
	data |= !floppy_wpt_r(m_image) << 6;

	// attention in
	data |= !m_bus->atn_r() << 7;

	return data;
}


WRITE8_MEMBER( c1581_device::cia_pb_w )
{
	/*

        bit     description

        PB0
        PB1     DATA OUT
        PB2
        PB3     CLK OUT
        PB4     ATN ACK
        PB5     FAST SER DIR
        PB6
        PB7

    */

	// data out
	m_data_out = BIT(data, 1);

	// clock out
	m_bus->clk_w(this, !BIT(data, 3));

	// attention acknowledge
	m_atn_ack = BIT(data, 4);

	// fast serial direction
	m_fast_ser_dir = BIT(data, 5);

	set_iec_data();
	set_iec_srq();
}


static MOS8520_INTERFACE( cia_intf )
{
	XTAL_16MHz/8,
	DEVCB_CPU_INPUT_LINE(M6502_TAG, INPUT_LINE_IRQ0),
	DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, c1581_device, cnt_w),
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, c1581_device, sp_w),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, c1581_device, cia_pa_r),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, c1581_device, cia_pa_w),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, c1581_device, cia_pb_r),
	DEVCB_DEVICE_MEMBER(DEVICE_SELF_OWNER, c1581_device, cia_pb_w)
};


//-------------------------------------------------
//  wd17xx_interface fdc_intf
//-------------------------------------------------

static const wd17xx_interface fdc_intf =
{
	DEVCB_LINE_GND,
	DEVCB_NULL,
	DEVCB_NULL,
	{ FLOPPY_0, NULL, NULL, NULL }
};


//-------------------------------------------------
//  FLOPPY_OPTIONS( c1581 )
//-------------------------------------------------

static FLOPPY_OPTIONS_START( c1581 )
	FLOPPY_OPTION( c1581, "d81", "Commodore 1581 Disk Image", d81_dsk_identify, d81_dsk_construct, NULL )
FLOPPY_OPTIONS_END


//-------------------------------------------------
//  floppy_config c1581_floppy_config
//-------------------------------------------------

static const floppy_config c1581_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_3_5_DSDD,
	FLOPPY_OPTIONS_NAME(c1581),
	NULL
};


//-------------------------------------------------
//  MACHINE_DRIVER( c1581 )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( c1581 )
	MCFG_CPU_ADD(M6502_TAG, M6502, XTAL_16MHz/8)
	MCFG_CPU_PROGRAM_MAP(c1581_mem)

	MCFG_MOS8520_ADD(M8520_TAG, XTAL_16MHz/8, cia_intf)
	MCFG_WD1770_ADD(WD1770_TAG, /*XTAL_16MHz/2,*/ fdc_intf)

	MCFG_FLOPPY_DRIVE_ADD(FLOPPY_0, c1581_floppy_config)
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor c1581_device_config::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( c1581 );
}



//**************************************************************************
//  INLINE HELPERS
//**************************************************************************

//-------------------------------------------------
//  c1581_device - constructor
//-------------------------------------------------

inline void c1581_device::set_iec_data()
{
	int atn = m_bus->atn_r();
	int data = !m_data_out & !(m_atn_ack & !atn);

	// fast serial data
	if (m_fast_ser_dir) data &= m_sp_out;

	m_bus->data_w(this, data);
}


//-------------------------------------------------
//  c1581_device - constructor
//-------------------------------------------------

inline void c1581_device::set_iec_srq()
{
	int srq = 1;

	// fast serial clock
	if (m_fast_ser_dir) srq &= m_cnt_out;

	m_bus->srq_w(this, srq);
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  c1581_device - constructor
//-------------------------------------------------

c1581_device::c1581_device(running_machine &_machine, const c1581_device_config &_config)
    : device_t(_machine, _config),
	  device_cbm_iec_interface(_machine, _config, *this),
	  m_maincpu(*this, M6502_TAG),
	  m_cia(*this, M8520_TAG),
	  m_fdc(*this, WD1770_TAG),
	  m_image(*this, FLOPPY_0),
	  m_bus(*this->owner(), CBM_IEC_TAG),
      m_config(_config)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void c1581_device::device_start()
{
	// map ROM
	address_space *program = m_maincpu->memory().space(AS_PROGRAM);
	program->install_rom(0x8000, 0xbfff, subregion(M6502_TAG)->base());

	// state saving
	save_item(NAME(m_data_out));
	save_item(NAME(m_atn_ack));
	save_item(NAME(m_fast_ser_dir));
	save_item(NAME(m_sp_out));
	save_item(NAME(m_cnt_out));
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void c1581_device::device_reset()
{
	m_sp_out = 1;
	m_cnt_out = 1;
}


//-------------------------------------------------
//  cbm_iec_srq -
//-------------------------------------------------

void c1581_device::cbm_iec_srq(int state)
{
	if (!m_fast_ser_dir)
	{
		m_cia->cnt_w(state);
	}
}


//-------------------------------------------------
//  cbm_iec_atn -
//-------------------------------------------------

void c1581_device::cbm_iec_atn(int state)
{
	m_cia->flag_w(state);

	set_iec_data();
}


//-------------------------------------------------
//  cbm_iec_data -
//-------------------------------------------------

void c1581_device::cbm_iec_data(int state)
{
	if (!m_fast_ser_dir)
	{
		m_cia->sp_w(state);
	}
}


//-------------------------------------------------
//  cbm_iec_reset -
//-------------------------------------------------

void c1581_device::cbm_iec_reset(int state)
{
	if (!state)
	{
		device_reset();
	}
}
