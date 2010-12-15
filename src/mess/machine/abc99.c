/*

Luxor ABC99

Keyboard PCB Layout
-------------------
    
|-----------------------------------------------------------------------|
|	CN1			CN2				CPU1		ROM1					SW1 |
|                                                                       |
|   6MHz	CPU0		ROM0		GI                                  |
|                                                                       |
|                                                                       |
|                                                                       |
|                                                                       |
|                                                                       |
|                                                                       |
|                                                                       |
|                                                                       |
|-----------------------------------------------------------------------|

Notes:
    Relevant IC's shown.

    CPU0		- STMicro ET8035N-6
	CPU1		- STMicro ET8035N-6
	ROM0		- Texas Instruments TMS2516JL-16 "107268-16"
	ROM1		- STMicro ET2716Q-1 "107268-17"
	GI			- General Insruments 321239007 "4=7"
    CN1         - DB15 mouse connector
    CN1         - 12 pin PCB header

*/

#include "emu.h"
#include "abc99.h"
#include "cpu/mcs48/mcs48.h"
#include "machine/devhelpr.h"



//**************************************************************************
//	MACROS / CONSTANTS
//**************************************************************************

#define I8035_Z5_TAG		"z5"
#define I8035_Z2_TAG		"z2"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type ABC99 = abc99_device_config::static_alloc_device_config;



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

GENERIC_DEVICE_CONFIG_SETUP(abc99, "Luxor ABC 99")


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void abc99_device_config::device_config_complete()
{
}


//-------------------------------------------------
//  ROM( abc99 )
//-------------------------------------------------

ROM_START( abc99 )
	ROM_REGION( 0x800, I8035_Z5_TAG, 0 )
	ROM_LOAD( "107268-16.z6", 0x000, 0x800, CRC(785ec0c6) SHA1(0b261beae20dbc06fdfccc50b19ea48b5b6e22eb) )

	ROM_REGION( 0x800, I8035_Z2_TAG, 0 )
	ROM_LOAD( "107268-17.z3", 0x000, 0x800, CRC(2f60cc35) SHA1(ebc6af9cd0a49a0d01698589370e628eebb6221c) )

	ROM_REGION( 0x1800, "investigate", 0)
	ROM_LOAD( "106819-09.bin", 0x0000, 0x1000, CRC(ffe32a71) SHA1(fa2ce8e0216a433f9bbad0bdd6e3dc0b540f03b7) )
	ROM_LOAD( "107268-64.bin", 0x1000, 0x0800, CRC(e33683ae) SHA1(0c1d9e320f82df05f4804992ef6f6f6cd20623f3) )
ROM_END


//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *abc99_device_config::rom_region() const
{
	return ROM_NAME( abc99 );
}


//-------------------------------------------------
//  ADDRESS_MAP( abc99_z5_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( abc99_z5_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_ROM AM_REGION("abc99:z5", 0)
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( abc99_z3_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( abc99_z2_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_ROM AM_REGION("abc99:z2", 0)
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( abc99_z5_io )
//-------------------------------------------------

static ADDRESS_MAP_START( abc99_z5_io, ADDRESS_SPACE_IO, 8 )
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( abc99_z3_io )
//-------------------------------------------------

static ADDRESS_MAP_START( abc99_z2_io, ADDRESS_SPACE_IO, 8 )
ADDRESS_MAP_END


//-------------------------------------------------
//  MACHINE_DRIVER( abc99 )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( abc99 )
	MDRV_CPU_ADD(I8035_Z5_TAG, I8035, XTAL_6MHz)
	MDRV_CPU_PROGRAM_MAP(abc99_z5_mem)
	MDRV_CPU_IO_MAP(abc99_z5_io)

	MDRV_CPU_ADD(I8035_Z2_TAG, I8035, XTAL_6MHz)
	MDRV_CPU_PROGRAM_MAP(abc99_z2_mem)
	MDRV_CPU_IO_MAP(abc99_z2_io)
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//	machine configurations
//-------------------------------------------------

machine_config_constructor abc99_device_config::machine_config_additions() const
{
	return MACHINE_CONFIG_NAME( abc99 );
}


//-------------------------------------------------
//  INPUT_PORTS( abc99 )
//-------------------------------------------------

INPUT_PORTS_START( abc99 )
INPUT_PORTS_END



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  abc99_device - constructor
//-------------------------------------------------

abc99_device::abc99_device(running_machine &_machine, const abc99_device_config &config)
    : device_t(_machine, config),
      m_config(config)
{

}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void abc99_device::device_start()
{
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void abc99_device::device_reset()
{
}
