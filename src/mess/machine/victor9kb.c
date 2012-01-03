#include "victor9kb.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define I8021_TAG		"z3"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type VICTOR9K_KEYBOARD = &device_creator<victor9k_keyboard_device>;



//-------------------------------------------------
//  ROM( victor9k_keyboard )
//-------------------------------------------------

ROM_START( victor9k_keyboard )
	ROM_REGION( 0x400, I8021_TAG, 0)
	ROM_LOAD( "20-8021-139.z3", 0x000, 0x400, CRC(0fe9d53d) SHA1(61d92ba90f98f8978bbd9303c1ac3134cde8cdcb) )
ROM_END


//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *victor9k_keyboard_device::device_rom_region() const
{
	return ROM_NAME( victor9k_keyboard );
}


//-------------------------------------------------
//  ADDRESS_MAP( kb_io )
//-------------------------------------------------

static ADDRESS_MAP_START( victor9k_keyboard_io, AS_IO, 8, victor9k_keyboard_device )
ADDRESS_MAP_END


//-------------------------------------------------
//  MACHINE_DRIVER( victor9k_keyboard )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( victor9k_keyboard )
	MCFG_CPU_ADD(I8021_TAG, I8021, 100000) // XTAL 48-300-010, 30 clocks/cycle
	MCFG_CPU_IO_MAP(victor9k_keyboard_io)
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor victor9k_keyboard_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( victor9k_keyboard );
}


//-------------------------------------------------
//  INPUT_PORTS( victor9k_keyboard )
//-------------------------------------------------

INPUT_PORTS_START( victor9k_keyboard )
INPUT_PORTS_END


//-------------------------------------------------
//  input_ports - device-specific input ports
//-------------------------------------------------

ioport_constructor victor9k_keyboard_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( victor9k_keyboard );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  victor9k_keyboard_device - constructor
//-------------------------------------------------

victor9k_keyboard_device::victor9k_keyboard_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, VICTOR9K_KEYBOARD, "Victor 9000 Keyboard", tag, owner, clock),
	  m_maincpu(*this, I8021_TAG),
	  m_kbdata(0)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void victor9k_keyboard_device::device_start()
{
	// resolve callbacks
    m_out_kbrdy_func.resolve(m_out_kbrdy_cb, *this);

	// state saving
	save_item(NAME(m_kbdata));
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void victor9k_keyboard_device::device_reset()
{
}


//-------------------------------------------------
//  kback_w - 
//-------------------------------------------------

WRITE_LINE_MEMBER( victor9k_keyboard_device::kback_w )
{
}


//-------------------------------------------------
//  kbdata_r - 
//-------------------------------------------------

READ_LINE_MEMBER( victor9k_keyboard_device::kbdata_r )
{
	return m_kbdata;
}
