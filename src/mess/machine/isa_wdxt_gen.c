/**********************************************************************

    Western Digital WDXT-GEN ISA XT MFM Hard Disk Controller

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "machine/isa_wdxt_gen.h"


//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type WDXT_GEN = &device_creator<wdxt_gen_device>;


//-------------------------------------------------
//  ROM( wdxt_gen )
//-------------------------------------------------

ROM_START( wdxt_gen )
	ROM_REGION( 0x2000, "hdc", 0 )
	ROM_LOAD( "3.u13", 0x0000, 0x2000, CRC(fbcb5f91) SHA1(8c22bd664177eb6126f3011eda8c5655fffe0ef2) ) // Toshiba TMM2464AP
ROM_END


//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *wdxt_gen_device::device_rom_region() const
{
	return ROM_NAME( wdxt_gen );
}


//-------------------------------------------------
//  MACHINE_DRIVER( wdxt_gen )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( wdxt_gen )
	MCFG_HARDDISK_ADD("harddisk0")
	MCFG_HARDDISK_ADD("harddisk1")
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor wdxt_gen_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( wdxt_gen );
}


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  wdxt_gen_device - constructor
//-------------------------------------------------

wdxt_gen_device::wdxt_gen_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, WDXT_GEN, "Western Digital WDXT-GEN (Amstrad PC1512/1640)", tag, owner, clock),
	  device_isa8_card_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void wdxt_gen_device::device_start()
{
	set_isa_device();
	m_isa->install_rom(this, 0xc8000, 0xc9fff, 0, 0, "hdc", "hdc");
	//m_isa->install_device(this, 0x0320, 0x0323, 0, 0, FUNC(pc_HDC_r), FUNC(pc_HDC_w) );
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void wdxt_gen_device::device_reset()
{
}
