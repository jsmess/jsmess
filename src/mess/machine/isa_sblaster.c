/***************************************************************************

  ISA 8 bit Creative Labs Sound Blaster Sound Card

***************************************************************************/

#include "emu.h"
#include "isa_sblaster.h"
#include "sound/speaker.h"
#include "sound/3812intf.h"
#include "sound/saa1099.h"

/*
  adlib (YM3812/OPL2 chip), part of many many soundcards (soundblaster)
  soundblaster: YM3812 also accessible at 0x228/9 (address jumperable)
  soundblaster pro version 1: 2 YM3812 chips
   at 0x388 both accessed,
   at 0x220/1 left?, 0x222/3 right? (jumperable)
  soundblaster pro version 2: 1 OPL3 chip

  pro audio spectrum +: 2 OPL2
  pro audio spectrum 16: 1 OPL3

  2 x saa1099 chips
    also on sound blaster 1.0
    option on sound blaster 1.5

  jumperable? normally 0x220
*/
#define ym3812_StdClock 3579545

static const ym3812_interface pc_ym3812_interface =
{
	NULL
};

static MACHINE_CONFIG_FRAGMENT( sblaster_config )
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("ym3812", YM3812, ym3812_StdClock)
	MCFG_SOUND_CONFIG(pc_ym3812_interface)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MCFG_SOUND_ADD("saa1099.1", SAA1099, 4772720)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MCFG_SOUND_ADD("saa1099.2", SAA1099, 4772720)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
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

static READ8_DEVICE_HANDLER( saa1099_16_r )
{
	return 0xff;
}

static WRITE8_DEVICE_HANDLER( saa1099_16_w )
{
	switch(offset)
	{
		case 0 : saa1099_control_w( device, offset, data ); break;
		case 1 : saa1099_data_w( device, offset, data ); break;
	}
}

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type ISA8_SOUND_BLASTER_1_0 = &device_creator<isa8_sblaster_device>;

//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor isa8_sblaster_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( sblaster_config );
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  isa8_sblaster_device - constructor
//-------------------------------------------------

isa8_sblaster_device::isa8_sblaster_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, ISA8_SOUND_BLASTER_1_0, "ISA8_SOUND_BLASTER_1_0", tag, owner, clock),
		device_isa8_card_interface(mconfig, *this),
		device_slot_card_interface(mconfig, *this)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void isa8_sblaster_device::device_start()
{
    set_isa_device();
	m_isa->install_device(subdevice("ym3812"), 0x0388, 0x0389, 0, 0, FUNC(ym3812_16_r), FUNC(ym3812_16_w) );
	m_isa->install_device(subdevice("saa1099.1"), 0x0220, 0x0221, 0, 0, FUNC(saa1099_16_r), FUNC(saa1099_16_w) );
	m_isa->install_device(subdevice("saa1099.2"), 0x0222, 0x0223, 0, 0, FUNC(saa1099_16_r), FUNC(saa1099_16_w) );
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void isa8_sblaster_device::device_reset()
{
}
