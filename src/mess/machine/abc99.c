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

    CPU0		- STMicro ET8035N-6 8035 CPU
	CPU1		- STMicro ET8035N-6 8035 CPU
	ROM0		- Texas Instruments TMS2516JL-16 2Kx8 ROM "107268-16" 
	ROM1		- STMicro ET2716Q-1 2Kx8 ROM "107268-17" 
	GI			- General Instruments 321239007 keyboard chip "4=7"
    CN1         - DB15 connector, Luxor ABC R8 (3 button mouse)
    CN2         - 12 pin PCB header, keyboard data cable
	SW1			- reset switch?

*/

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "abc99.h"
#include "cpu/mcs48/mcs48.h"
#include "machine/devhelpr.h"



//**************************************************************************
//	MACROS / CONSTANTS
//**************************************************************************

#define I8035_Z2_TAG		"z2"
#define I8035_Z5_TAG		"z5"



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
	ROM_REGION( 0x800, I8035_Z2_TAG, 0 )
	ROM_LOAD( "107268-17.z3", 0x000, 0x800, CRC(2f60cc35) SHA1(ebc6af9cd0a49a0d01698589370e628eebb6221c) )

	ROM_REGION( 0x800, I8035_Z5_TAG, 0 )
	ROM_LOAD( "107268-16.z6", 0x000, 0x800, CRC(785ec0c6) SHA1(0b261beae20dbc06fdfccc50b19ea48b5b6e22eb) )

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
//  ADDRESS_MAP( abc99_z2_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( abc99_z2_mem, ADDRESS_SPACE_PROGRAM, 8, abc99_device )
	AM_RANGE(0x0000, 0x07ff) AM_ROM AM_REGION("abc99:z2", 0)
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( abc99_z2_io )
//-------------------------------------------------

static ADDRESS_MAP_START( abc99_z2_io, ADDRESS_SPACE_IO, 8, abc99_device )
	AM_RANGE(0x30, 0x30) AM_READ_PORT("X0") AM_WRITENOP
	AM_RANGE(0x31, 0x31) AM_READ_PORT("X1") AM_WRITENOP
	AM_RANGE(0x32, 0x32) AM_READ_PORT("X2") AM_WRITENOP
	AM_RANGE(0x33, 0x33) AM_READ_PORT("X3") AM_WRITENOP
	AM_RANGE(0x34, 0x34) AM_READ_PORT("X4") AM_WRITENOP
	AM_RANGE(0x35, 0x35) AM_READ_PORT("X5") AM_WRITENOP
	AM_RANGE(0x36, 0x36) AM_READ_PORT("X6") AM_WRITENOP
	AM_RANGE(0x37, 0x37) AM_READ_PORT("X7") AM_WRITENOP
	AM_RANGE(0x38, 0x38) AM_READ_PORT("X8") AM_WRITENOP
	AM_RANGE(0x39, 0x39) AM_READ_PORT("X9") AM_WRITENOP
	AM_RANGE(0x3a, 0x3a) AM_READ_PORT("X10") AM_WRITENOP
	AM_RANGE(0x3b, 0x3b) AM_READ_PORT("X11") AM_WRITENOP
	AM_RANGE(0x3c, 0x3c) AM_READ_PORT("X12") AM_WRITENOP
	AM_RANGE(0x3d, 0x3d) AM_READ_PORT("X13") AM_WRITENOP
	AM_RANGE(0x3e, 0x3e) AM_READ_PORT("X14") AM_WRITENOP
	AM_RANGE(0x3f, 0x3f) AM_READ_PORT("X15") AM_WRITENOP
	AM_RANGE(MCS48_PORT_P1, MCS48_PORT_P1) AM_DEVWRITE(DEVICE_SELF_OWNER, abc99_device, z2_p1_w)
	AM_RANGE(MCS48_PORT_T1, MCS48_PORT_T1) AM_DEVREAD(DEVICE_SELF_OWNER, abc99_device, z2_t1_r)
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( abc99_z5_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( abc99_z5_mem, ADDRESS_SPACE_PROGRAM, 8, abc99_device )
	AM_RANGE(0x0000, 0x07ff) AM_ROM AM_REGION("abc99:z5", 0)
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( abc99_z5_io )
//-------------------------------------------------

static ADDRESS_MAP_START( abc99_z5_io, ADDRESS_SPACE_IO, 8, abc99_device )
	AM_RANGE(MCS48_PORT_P1, MCS48_PORT_P1) AM_DEVREAD(DEVICE_SELF_OWNER, abc99_device, z5_p1_r)
	AM_RANGE(MCS48_PORT_P2, MCS48_PORT_P2) AM_DEVWRITE(DEVICE_SELF_OWNER, abc99_device, z5_p2_w)
	AM_RANGE(MCS48_PORT_T0, MCS48_PORT_T0) AM_DEVWRITE(DEVICE_SELF_OWNER, abc99_device, z5_t0_w)
	AM_RANGE(MCS48_PORT_T1, MCS48_PORT_T1) AM_DEVREAD(DEVICE_SELF_OWNER, abc99_device, z5_t1_r)
ADDRESS_MAP_END


//-------------------------------------------------
//  MACHINE_DRIVER( abc99 )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( abc99 )
	// keyboard CPU
	MDRV_CPU_ADD(I8035_Z2_TAG, I8035, XTAL_6MHz)
	MDRV_CPU_PROGRAM_MAP(abc99_z2_mem)
	MDRV_CPU_IO_MAP(abc99_z2_io)

	// mouse CPU
	MDRV_CPU_ADD(I8035_Z5_TAG, I8035, XTAL_6MHz)
	MDRV_CPU_PROGRAM_MAP(abc99_z5_mem)
	MDRV_CPU_IO_MAP(abc99_z5_io)
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
	PORT_START("abc99:X0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("abc99:X1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("abc99:X2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("abc99:X3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("abc99:X4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("abc99:X5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("abc99:X6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("abc99:X7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("abc99:X8")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("abc99:X9")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("abc99:X10")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("abc99:X11")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("abc99:X12")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("abc99:X13")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("abc99:X14")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("abc99:X15")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
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


//-------------------------------------------------
//  z2_p1_w - 
//-------------------------------------------------

WRITE8_MEMBER( abc99_device::z2_p1_w )
{
}


//-------------------------------------------------
//  z2_t1_r - 
//-------------------------------------------------

READ8_MEMBER( abc99_device::z2_t1_r )
{
	return 0;
}


//-------------------------------------------------
//  z5_p1_r - 
//-------------------------------------------------

READ8_MEMBER( abc99_device::z5_p1_r )
{
	return 0;
}


//-------------------------------------------------
//  z5_p2_w - 
//-------------------------------------------------

WRITE8_MEMBER( abc99_device::z5_p2_w )
{
}


//-------------------------------------------------
//  z5_t0_w - 
//-------------------------------------------------

WRITE8_MEMBER( abc99_device::z5_t0_w )
{
}


//-------------------------------------------------
//  z5_t1_r - 
//-------------------------------------------------

READ8_MEMBER( abc99_device::z5_t1_r )
{
	return 0;
}
