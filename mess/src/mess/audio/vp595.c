/**********************************************************************

    RCA VP595 - VIP Simple Sound System emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

#include "vp595.h"
#include "machine/devhelpr.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define CDP1863_TAG		"u1"
#define CDP1863_XTAL	440560



//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

// devices
const device_type VP595 = &device_creator<vp595_device>;

//-------------------------------------------------
//  MACHINE_CONFIG_FRAGMENT( vp595 )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( vp595 )
	MCFG_SPEAKER_STANDARD_MONO("mono")

	MCFG_CDP1863_ADD(CDP1863_TAG, 0, CDP1863_XTAL)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor vp595_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( vp595 );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  vp595_device - constructor
//-------------------------------------------------

vp595_device::vp595_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, VP595, "VP595", tag, owner, clock),
	  m_pfg(*this, CDP1863_TAG)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void vp595_device::device_start()
{
}


//-------------------------------------------------
//  latch_w -
//-------------------------------------------------

WRITE8_MEMBER( vp595_device::latch_w )
{
	if (!data) data = 0x80;

	m_pfg->str_w(data);
}


//-------------------------------------------------
//  q_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( vp595_device::q_w )
{
	m_pfg->oe_w(state);
}


//-------------------------------------------------
//  install_write_handlers -
//-------------------------------------------------

void vp595_device::install_write_handlers(address_space *space, bool enabled)
{
	if (enabled)
	{
		space->install_write_handler(0x03, 0x03, write8_delegate(FUNC(cdp1863_device::str_w), (cdp1863_device*)m_pfg));
	}
	else
	{
		space->unmap_write(0x03, 0x03);
	}
}
