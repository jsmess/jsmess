/**********************************************************************

    Zilog Z8536 Counter/Timer and Parallel I/O emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "emu.h"
#include "z8536.h"
#include "machine/devhelpr.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type Z8536 = z8536_device_config::static_alloc_device_config;



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

GENERIC_DEVICE_CONFIG_SETUP(z8536, "Zilog Z8536")


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void z8536_device_config::device_config_complete()
{
	// inherit a copy of the static data
	const z8536_interface *intf = reinterpret_cast<const z8536_interface *>(static_config());
	if (intf != NULL)
		*static_cast<z8536_interface *>(this) = *intf;

	// or initialize to defaults if none provided
	else
	{
		memset(&m_out_int_func, 0, sizeof(m_out_int_func));
		memset(&m_in_pa_func, 0, sizeof(m_in_pa_func));
		memset(&m_out_pa_func, 0, sizeof(m_out_pa_func));
		memset(&m_in_pb_func, 0, sizeof(m_in_pb_func));
		memset(&m_out_pb_func, 0, sizeof(m_out_pb_func));
		memset(&m_in_pc_func, 0, sizeof(m_in_pc_func));
		memset(&m_out_pc_func, 0, sizeof(m_out_pc_func));
	}
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  z8536_device - constructor
//-------------------------------------------------

z8536_device::z8536_device(running_machine &_machine, const z8536_device_config &config)
    : device_t(_machine, config),
      m_config(config)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void z8536_device::device_start()
{
	// resolve callbacks
	devcb_resolve_write_line(&m_out_int_func, &m_config.m_out_int_func, this);
	devcb_resolve_read8(&m_in_pa_func, &m_config.m_in_pa_func, this);
	devcb_resolve_write8(&m_out_pa_func, &m_config.m_out_pa_func, this);
	devcb_resolve_read8(&m_in_pb_func, &m_config.m_in_pb_func, this);
	devcb_resolve_write8(&m_out_pb_func, &m_config.m_out_pb_func, this);
	devcb_resolve_read8(&m_in_pc_func, &m_config.m_in_pc_func, this);
	devcb_resolve_write8(&m_out_pc_func, &m_config.m_out_pc_func, this);
}


//-------------------------------------------------
//  device_timer - handler timer events
//-------------------------------------------------

void z8536_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
}


//-------------------------------------------------
//  read - register read
//-------------------------------------------------

READ8_MEMBER( z8536_device::read )
{
	return 0;
}


//-------------------------------------------------
//  write - register write
//-------------------------------------------------

WRITE8_MEMBER( z8536_device::write )
{
}


//-------------------------------------------------
//  intack_w - interrupt acknowledge
//-------------------------------------------------

WRITE_LINE_MEMBER( z8536_device::intack_w )
{
}
