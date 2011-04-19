/***************************************************************************

  ISA 8 bit Adlib Sound Card

***************************************************************************/

#include "emu.h"
#include "isa_adlib.h"
#include "sound/speaker.h"
#include "sound/3812intf.h"

#define ym3812_StdClock 3579545

static const ym3812_interface pc_ym3812_interface =
{
	NULL
};

static MACHINE_CONFIG_FRAGMENT( adlib_config )
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("ym3812", YM3812, ym3812_StdClock)
	MCFG_SOUND_CONFIG(pc_ym3812_interface)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "adlib:mono", 1.00)
MACHINE_CONFIG_END

static READ8_DEVICE_HANDLER( ym3812_16_r )
{
	UINT8 retVal = 0xff;
	switch(offset)
	{
		case 0 : retVal = ym3812_status_port_r( device, offset ); break;
	}
	return retVal;
}

static WRITE8_DEVICE_HANDLER( ym3812_16_w )
{
	switch(offset)
	{
		case 0 : ym3812_control_port_w( device, offset, data ); break;
		case 1 : ym3812_write_port_w( device, offset, data ); break;
	}
}

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type ISA8_ADLIB = isa8_adlib_device_config::static_alloc_device_config;

//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  isa8_adlib_device_config - constructor
//-------------------------------------------------

isa8_adlib_device_config::isa8_adlib_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
        : device_config(mconfig, static_alloc_device_config, "ISA8_ADLIB", tag, owner, clock),
			device_config_isa8_card_interface(mconfig, *this)
{
}

//-------------------------------------------------
//  static_alloc_device_config - allocate a new
//  configuration object
//-------------------------------------------------

device_config *isa8_adlib_device_config::static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
{
        return global_alloc(isa8_adlib_device_config(mconfig, tag, owner, clock));
}

//-------------------------------------------------
//  alloc_device - allocate a new device object
//-------------------------------------------------

device_t *isa8_adlib_device_config::alloc_device(running_machine &machine) const
{
        return auto_alloc(machine, isa8_adlib_device(machine, *this));
}

//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor isa8_adlib_device_config::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( adlib_config );
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  isa8_adlib_device - constructor
//-------------------------------------------------

isa8_adlib_device::isa8_adlib_device(running_machine &_machine, const isa8_adlib_device_config &config) :
        device_t(_machine, config),
		device_isa8_card_interface( _machine, config, *this ),
        m_config(config),
		m_isa(*owner(),config.m_isa_tag)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void isa8_adlib_device::device_start()
{
	m_isa->add_isa_card(this, m_config.m_isa_num);
	m_isa->install_device(subdevice("ym3812"), 0x0388, 0x0389, 0, 0, FUNC(ym3812_16_r), FUNC(ym3812_16_w) );
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void isa8_adlib_device::device_reset()
{
}
