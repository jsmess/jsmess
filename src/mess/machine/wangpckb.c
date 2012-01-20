#include "wangpckb.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define I8051_TAG		"i8051"
#define SN76489AN_TAG	"sn76489an"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type WANGPC_KEYBOARD = &device_creator<wangpc_keyboard_device>;



//-------------------------------------------------
//  ROM( wangpc_keyboard )
//-------------------------------------------------

ROM_START( wangpc_keyboard )
	ROM_REGION( 0x1000, I8051_TAG, 0 )
	ROM_LOAD( "keyboard-z5.bin", 0x0000, 0x1000, CRC(82d2999f) SHA1(2bb34a1de2d94b2885d9e8fcd4964296f6276c5c) )
ROM_END


//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *wangpc_keyboard_device::device_rom_region() const
{
	return ROM_NAME( wangpc_keyboard );
}


//-------------------------------------------------
//  ADDRESS_MAP( wangpc_keyboard_io )
//-------------------------------------------------

static ADDRESS_MAP_START( wangpc_keyboard_io, AS_IO, 8, wangpc_keyboard_device )
ADDRESS_MAP_END


//-------------------------------------------------
//  MACHINE_DRIVER( wangpc_keyboard )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( wangpc_keyboard )
	MCFG_CPU_ADD(I8051_TAG, I8051, 4000000) // ???
	MCFG_CPU_IO_MAP(wangpc_keyboard_io)

	// sound hardware
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(SN76489AN_TAG, SN76489A, 2000000) // ???
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor wangpc_keyboard_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( wangpc_keyboard );
}


//-------------------------------------------------
//  INPUT_PORTS( wangpc_keyboard )
//-------------------------------------------------

INPUT_PORTS_START( wangpc_keyboard )
INPUT_PORTS_END


//-------------------------------------------------
//  input_ports - device-specific input ports
//-------------------------------------------------

ioport_constructor wangpc_keyboard_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( wangpc_keyboard );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  wangpc_keyboard_device - constructor
//-------------------------------------------------

wangpc_keyboard_device::wangpc_keyboard_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, WANGPC_KEYBOARD, "Wang PC Keyboard", tag, owner, clock),
	  m_maincpu(*this, I8051_TAG)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void wangpc_keyboard_device::device_start()
{
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void wangpc_keyboard_device::device_reset()
{
}
