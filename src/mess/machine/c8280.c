/**********************************************************************

    Commodore 8280 Dual 8" Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "c8280.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define M6502_DOS_TAG	"5c"
#define M6502_FDC_TAG	"9e"
#define M6532_0_TAG		"9f"
#define M6532_1_TAG		"9g"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type C8280 = &device_creator<c8280_device>;

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void c8280_device::device_config_complete()
{
	m_shortname = "c8280";
}


//-------------------------------------------------
//  static_set_config - configuration helper
//-------------------------------------------------

void c8280_device::static_set_config(device_t &device, int address)
{
	c8280_device &c8280 = downcast<c8280_device &>(device);

	assert((address > 7) && (address < 12));

	c8280.m_address = address - 8;
}


//-------------------------------------------------
//  ROM( c8280 )
//-------------------------------------------------

ROM_START( c8280 )
	ROM_REGION( 0x4000, M6502_DOS_TAG, 0 )
	ROM_LOAD( "300542-001.10c", 0x0000, 0x2000, CRC(3c6eee1e) SHA1(0726f6ab4de4fc9c18707fe87780ffd9f5ed72ab) )
	ROM_LOAD( "300543-001.10d", 0x2000, 0x2000, CRC(f58e665e) SHA1(9e58b47c686c91efc6ef1a27f72dbb5e26c485ec) )

	ROM_REGION( 0x800, M6502_FDC_TAG, 0 )
	ROM_LOAD( "300541-001.3c",  0x000, 0x800, CRC(cb07b2db) SHA1(a1f9c5a7bd3798f5a97dc0b465c3bf5e3513e148) )
ROM_END


//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *c8280_device::device_rom_region() const
{
	return ROM_NAME( c8280 );
}


//-------------------------------------------------
//  ADDRESS_MAP( c8280_main_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( c8280_main_mem, AS_PROGRAM, 8, c8280_device )
	AM_RANGE(0xc000, 0xffff) // AM_ROM
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( c8280_fdc_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( c8280_fdc_mem, AS_PROGRAM, 8, c8280_device )
	ADDRESS_MAP_GLOBAL_MASK(0x1fff)
	AM_RANGE(0x1c00, 0x1fff) // AM_ROM 6530
ADDRESS_MAP_END


//-------------------------------------------------
//  riot6532_interface riot0_intf
//-------------------------------------------------

static const riot6532_interface riot0_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  riot6532_interface riot1_intf
//-------------------------------------------------

static const riot6532_interface riot1_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  FLOPPY_OPTIONS( c8280 )
//-------------------------------------------------

static FLOPPY_OPTIONS_START( c8280 )
FLOPPY_OPTIONS_END


//-------------------------------------------------
//  floppy_interface c8280_floppy_interface
//-------------------------------------------------

static const floppy_interface c8280_floppy_interface =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_8_DSDD,
	FLOPPY_OPTIONS_NAME(c8280),
	"floppy_8",
	NULL
};


//-------------------------------------------------
//  MACHINE_CONFIG_FRAGMENT( c8280 )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( c8280 )
	MCFG_CPU_ADD(M6502_DOS_TAG, M6502, 1000000)
	MCFG_CPU_PROGRAM_MAP(c8280_main_mem)

	MCFG_RIOT6532_ADD(M6532_0_TAG, 1000000, riot0_intf)
	MCFG_RIOT6532_ADD(M6532_1_TAG, 1000000, riot1_intf)

	MCFG_CPU_ADD(M6502_FDC_TAG, M6502, 1000000)
	MCFG_CPU_PROGRAM_MAP(c8280_fdc_mem)

	MCFG_LEGACY_FLOPPY_2_DRIVES_ADD(c8280_floppy_interface)
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor c8280_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( c8280 );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  c8280_device - constructor
//-------------------------------------------------

c8280_device::c8280_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, C8280, "C8280", tag, owner, clock),
	  device_ieee488_interface(mconfig, *this),
	  m_maincpu(*this, M6502_DOS_TAG),
	  m_fdccpu(*this, M6502_FDC_TAG),
	  m_riot0(*this, M6532_0_TAG),
	  m_riot1(*this, M6532_1_TAG),
	  m_image0(*this, FLOPPY_0),
	  m_image1(*this, FLOPPY_1),
	  m_bus(*this->owner(), IEEE488_TAG)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void c8280_device::device_start()
{
	address_space *main = m_maincpu->memory().space(AS_PROGRAM);
	address_space *fdc = m_fdccpu->memory().space(AS_PROGRAM);

	main->install_rom(0xc000, 0xffff, subregion(M6502_DOS_TAG)->base());
	fdc->install_rom(0x1800, 0x1fff, subregion(M6502_FDC_TAG)->base());
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void c8280_device::device_reset()
{
}


//-------------------------------------------------
//  device_timer - handler timer events
//-------------------------------------------------

void c8280_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
}


//-------------------------------------------------
//  ieee488_atn -
//-------------------------------------------------

void c8280_device::ieee488_atn(int state)
{
}


//-------------------------------------------------
//  ieee488_ifc -
//-------------------------------------------------

void c8280_device::ieee488_ifc(int state)
{
	if (!state)
	{
		device_reset();
	}
}
