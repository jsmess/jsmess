/**********************************************************************

    Morrow Designs Disk Jockey 2D/B floppy controller board emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "s100_dj2db.h"



//**************************************************************************
//  MACROS/CONSTANTS
//**************************************************************************

#define MB8866_TAG		"14b"
#define S1602_TAG		"14d"
#define BR1941_TAG		"13d"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type S100_DJ2DB = &device_creator<s100_dj2db_device>;


//-------------------------------------------------
//  ROM( dj2db )
//-------------------------------------------------

ROM_START( dj2db )
	ROM_REGION( 0x400, "dj2db", 0 )
	ROM_LOAD( "bv-2 f8.11d", 0x000, 0x400, CRC(b6218d0b) SHA1(e4b2ae886c0dd7717e2e02ae2e202115d8ec2def) )

	ROM_REGION( 0x220, "proms", 0 )
	ROM_LOAD( "8c-b f8.8c", 0x000, 0x200, NO_DUMP ) // 6301
	ROM_LOAD( "3d.3d", 0x200, 0x20, NO_DUMP ) // 6331
ROM_END


//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *s100_dj2db_device::device_rom_region() const
{
	return ROM_NAME( dj2db );
}


//-------------------------------------------------
//  COM8116_INTERFACE( brg_intf )
//-------------------------------------------------

static COM8116_INTERFACE( brg_intf )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	{ 6336, 4224, 2880, 2355, 2112, 1056, 528, 264, 176, 158, 132, 88, 66, 44, 33, 16 }, // from WD1943-00 datasheet
	{ 6336, 4224, 2880, 2355, 2112, 1056, 528, 264, 176, 158, 132, 88, 66, 44, 33, 16 },
};


//-------------------------------------------------
//  wd17xx_interface fdc_intf
//-------------------------------------------------

static const floppy_interface dj2db_floppy_interface =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_8_DSDD,
	FLOPPY_OPTIONS_NAME(default),
	"floppy_8",
	NULL
};

static const wd17xx_interface fdc_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};


//-------------------------------------------------
//  MACHINE_CONFIG_FRAGMENT( s100_dj2db )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( s100_dj2db )
	MCFG_COM8116_ADD(BR1941_TAG, XTAL_5_0688MHz, brg_intf)
	MCFG_WD179X_ADD(MB8866_TAG, fdc_intf) // FD1791
	MCFG_FLOPPY_2_DRIVES_ADD(dj2db_floppy_interface)
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor s100_dj2db_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( s100_dj2db );
}


//-------------------------------------------------
//  INPUT_PORTS( mm65k16s )
//-------------------------------------------------

static INPUT_PORTS_START( dj2db )
	PORT_START("13C")

	PORT_START("5D")
INPUT_PORTS_END


//-------------------------------------------------
//  input_ports - device-specific input ports
//-------------------------------------------------

ioport_constructor s100_dj2db_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( dj2db );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  s100_dj2db_device - constructor
//-------------------------------------------------

s100_dj2db_device::s100_dj2db_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, S100_DJ2DB, "DJ2DB", tag, owner, clock),
	device_s100_card_interface(mconfig, *this),
	device_slot_card_interface(mconfig, *this),
	m_fdc(*this, MB8866_TAG),
	m_dbrg(*this, BR1941_TAG)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void s100_dj2db_device::device_start()
{
	m_s100 = machine().device<s100_device>("s100");
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void s100_dj2db_device::device_reset()
{
}


//-------------------------------------------------
//  s100_smemr_r - memory read
//-------------------------------------------------

UINT8 s100_dj2db_device::s100_smemr_r(offs_t offset)
{
	return 0;
}


//-------------------------------------------------
//  s100_mwrt_w - memory write
//-------------------------------------------------

void s100_dj2db_device::s100_mwrt_w(offs_t offset, UINT8 data)
{
}


//-------------------------------------------------
//  s100_sinp_r - I/O read
//-------------------------------------------------

UINT8 s100_dj2db_device::s100_sinp_r(offs_t offset)
{
	return 0;
}


//-------------------------------------------------
//  s100_sout_w - I/O write
//-------------------------------------------------

void s100_dj2db_device::s100_sout_w(offs_t offset, UINT8 data)
{
}


//-------------------------------------------------
//  s100_phantom_w - phantom
//-------------------------------------------------

void s100_dj2db_device::s100_phantom_w(int state)
{
}


//-------------------------------------------------
//  s100_terminal_w - terminal write
//-------------------------------------------------

void s100_dj2db_device::s100_terminal_w(UINT8 data)
{
}
