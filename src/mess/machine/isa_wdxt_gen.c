/**********************************************************************

    Western Digital WDXT-GEN ISA XT MFM Hard Disk Controller

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

PCB Layout
----------

ASSY 61-000222-00

|-------------------------------------------|
|	CN2		CN1								|
|	CN3		7406				TMM2016		|
|			LS38	LS14					|
| MC3486									|
|				WD1015			WD11C00		|
|			33.04MHz						|
| MC3487									|
|				WD2010			LS244		|
|	WD10C20									|
|				LS260	LS13		ROM		|
|											|
|---|									|---|
	|-----------------------------------|

Notes:
    All IC's shown.

    ROM		- Toshiba TMM2464AP 8Kx8 ROM "3"
	TMM2016	- Toshiba TMM2016BP-10 2Kx8 SRAM
	WD1015	- Western Digital WD1015-PL-54-02 Buffer Manager Control Processor
	WD11C00	- Western Digital WD11C00L-JT-17-02 PC/XT Host Interface Logic Device
	WD10C20	- Western Digital WD10C20B-PH-05-05 Self-Adjusting Data Separator
	WD2010	- Western Digital WD2010A-PL-05-02 Winchester Disk Controller
	CN1		- 2x17 pin PCB header, control
	CN2		- 2x10 pin PCB header, drive 0 data
	CN3		- 2x10 pin PCB header, drive 1 data

*/

#include "machine/isa_wdxt_gen.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define WD1015_TAG		"u6"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type WDXT_GEN = &device_creator<wdxt_gen_device>;


//-------------------------------------------------
//  ROM( wdxt_gen )
//-------------------------------------------------

ROM_START( wdxt_gen )
	ROM_REGION( 0x800, WD1015_TAG, 0 )
	ROM_LOAD( "wd1015-pl-54-02.u6", 0x000, 0x800, NO_DUMP )

	ROM_REGION( 0x2000, "hdc", 0 )
	ROM_LOAD( "3.u13", 0x0000, 0x2000, CRC(fbcb5f91) SHA1(8c22bd664177eb6126f3011eda8c5655fffe0ef2) )
ROM_END


//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *wdxt_gen_device::device_rom_region() const
{
	return ROM_NAME( wdxt_gen );
}


//-------------------------------------------------
//  ADDRESS_MAP( wd1015_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( wd1015_mem, AS_PROGRAM, 8, wdxt_gen_device )
	AM_RANGE(0x0000, 0x07ff) AM_ROM AM_REGION(WD1015_TAG, 0)
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( wd1015_io )
//-------------------------------------------------

static ADDRESS_MAP_START( wd1015_io, AS_IO, 8, wdxt_gen_device )
ADDRESS_MAP_END


//-------------------------------------------------
//  MACHINE_DRIVER( wdxt_gen )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( wdxt_gen )
	MCFG_CPU_ADD(WD1015_TAG, I8048, 33040000/10) // ?
	MCFG_CPU_PROGRAM_MAP(wd1015_mem)
	MCFG_CPU_IO_MAP(wd1015_io)

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
