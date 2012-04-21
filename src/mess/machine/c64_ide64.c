/**********************************************************************

    IDE64 v4.1 cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "c64_ide64.h"



//**************************************************************************
//  MACROS/CONSTANTS
//**************************************************************************

#define AT29C010_TAG		"u3"
#define DS1302_TAG			"u4"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type C64_IDE64 = &device_creator<c64_ide64_cartridge_device>;


//-------------------------------------------------
//  MACHINE_CONFIG_FRAGMENT( c64_ide64 )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( c64_ide64 )
	MCFG_ATMEL_29C010_ADD(AT29C010_TAG)
	MCFG_DS1302_ADD(DS1302_TAG)
	// TODO CompactFlash
	
	MCFG_IDE_CONTROLLER_ADD("ide", NULL, ide_image_devices, "hdd", "hdd")
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor c64_ide64_cartridge_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( c64_ide64 );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  c64_ide64_cartridge_device - constructor
//-------------------------------------------------

c64_ide64_cartridge_device::c64_ide64_cartridge_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, C64_IDE64, "C64 IDE64 cartridge", tag, owner, clock),
	device_c64_expansion_card_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void c64_ide64_cartridge_device::device_start()
{
	// allocate memory
	c64_ram_pointer(machine(), 0x8000);
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void c64_ide64_cartridge_device::device_reset()
{
}


//-------------------------------------------------
//  c64_cd_r - cartridge data read
//-------------------------------------------------

UINT8 c64_ide64_cartridge_device::c64_cd_r(address_space &space, offs_t offset, int roml, int romh, int io1, int io2)
{
	UINT8 data = 0;

	// TODO

	return data;
}


//-------------------------------------------------
//  c64_cd_w - cartridge data write
//-------------------------------------------------

void c64_ide64_cartridge_device::c64_cd_w(address_space &space, offs_t offset, UINT8 data, int roml, int romh, int io1, int io2)
{
	// TODO
}
