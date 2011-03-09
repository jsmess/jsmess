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

enum
{
	PORT_C = 0,
	PORT_B,
	PORT_A,
	CONTROL
};


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
	UINT8 data = 0;
	
	switch (offset & 0x03)
	{
	case PORT_C:
		data = devcb_call_read8(&m_in_pc_func, 0) & 0x0f;
		break;

	case PORT_B:
		data = devcb_call_read8(&m_in_pb_func, 0);
		break;

	case PORT_A:
		data = devcb_call_read8(&m_in_pa_func, 0);
		break;

	case CONTROL:
		break;
	}
	
	return data;
}


//-------------------------------------------------
//  write - register write
//-------------------------------------------------

WRITE8_MEMBER( z8536_device::write )
{
	switch (offset & 0x03)
	{
	case PORT_C:
		devcb_call_write8(&m_out_pc_func, 0, data & 0x0f);
		break;

	case PORT_B:
		devcb_call_write8(&m_out_pb_func, 0, data);
		break;

	case PORT_A:
		devcb_call_write8(&m_out_pa_func, 0, data);
		break;

	case CONTROL:
		break;
	}
}


//-------------------------------------------------
//  intack_w - interrupt acknowledge
//-------------------------------------------------

WRITE_LINE_MEMBER( z8536_device::intack_w )
{
}


//-------------------------------------------------
//  pb0_w - port B bit 0 write
//-------------------------------------------------

WRITE_LINE_MEMBER( z8536_device::pb0_w ) { }
WRITE_LINE_MEMBER( z8536_device::pb1_w ) { }
WRITE_LINE_MEMBER( z8536_device::pb2_w ) { }
WRITE_LINE_MEMBER( z8536_device::pb3_w ) { }
WRITE_LINE_MEMBER( z8536_device::pb4_w ) { }
WRITE_LINE_MEMBER( z8536_device::pb5_w ) { }
WRITE_LINE_MEMBER( z8536_device::pb6_w ) { }
WRITE_LINE_MEMBER( z8536_device::pb7_w ) { }
