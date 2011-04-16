/**********************************************************************

    General Instruments AY-5-3600 Keyboard Encoder emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

/*

    TODO:

	- scan timer clock frequency
    - more accurate emulation of real chip

*/

#include "kb3600.h"
#include "machine/devhelpr.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define LOG 0



//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

// devices
const device_type AY3600 = ay3600_device_config::static_alloc_device_config;



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

GENERIC_DEVICE_CONFIG_SETUP(ay3600, "AY-5-3600")


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void ay3600_device_config::device_config_complete()
{
	// inherit a copy of the static data
	const ay3600_interface *intf = reinterpret_cast<const ay3600_interface *>(static_config());
	if (intf != NULL)
		*static_cast<ay3600_interface *>(this) = *intf;

	// or initialize to defaults if none provided
	else
	{
		memset(&m_in_shift_func, 0, sizeof(m_in_shift_func));
		memset(&m_in_control_func, 0, sizeof(m_in_control_func));
		memset(&m_out_data_ready_func, 0, sizeof(m_out_data_ready_func));
		memset(&m_out_ako_func, 0, sizeof(m_out_ako_func));
	}
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  ay3600_device - constructor
//-------------------------------------------------

ay3600_device::ay3600_device(running_machine &_machine, const ay3600_device_config &config)
    : device_t(_machine, config),
      m_config(config)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void ay3600_device::device_start()
{
	// resolve callbacks
	devcb_resolve_read16(&m_in_x_func[0], &m_config.m_in_x0_func, this);
	devcb_resolve_read16(&m_in_x_func[1], &m_config.m_in_x1_func, this);
	devcb_resolve_read16(&m_in_x_func[2], &m_config.m_in_x2_func, this);
	devcb_resolve_read16(&m_in_x_func[3], &m_config.m_in_x3_func, this);
	devcb_resolve_read16(&m_in_x_func[4], &m_config.m_in_x4_func, this);
	devcb_resolve_read16(&m_in_x_func[5], &m_config.m_in_x5_func, this);
	devcb_resolve_read16(&m_in_x_func[6], &m_config.m_in_x6_func, this);
	devcb_resolve_read16(&m_in_x_func[7], &m_config.m_in_x7_func, this);
	devcb_resolve_read16(&m_in_x_func[8], &m_config.m_in_x8_func, this);
	devcb_resolve_read_line(&m_in_shift_func, &m_config.m_in_shift_func, this);
	devcb_resolve_read_line(&m_in_control_func, &m_config.m_in_control_func, this);
	devcb_resolve_write_line(&m_out_data_ready_func, &m_config.m_out_data_ready_func, this);
	devcb_resolve_write_line(&m_out_ako_func, &m_config.m_out_ako_func, this);

	// allocate timers
	m_scan_timer = timer_alloc();
	m_scan_timer->adjust(attotime::from_hz(60), 0, attotime::from_hz(60));

	// state saving
	save_item(NAME(m_b));
	save_item(NAME(m_ako));
}


//-------------------------------------------------
//  device_timer - handler timer events
//-------------------------------------------------

void ay3600_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	int ako = 0;

	for (int x = 0; x < 9; x++)
	{
		UINT16 data = devcb_call_read16(&m_in_x_func[x], 0);

		for (int y = 0; y < 10; y++)
		{
			if (BIT(data, y))
			{
				int b = (x * 10) + y;

				ako = 1;

				if (b > 63)
				{
					b -= 64;
					b = 0x100 | b;
				}

				b |= (devcb_call_read_line(&m_in_shift_func) << 6);
				b |= (devcb_call_read_line(&m_in_control_func) << 7);

				if (m_b != b)
				{
					m_b = b;

					devcb_call_write_line(&m_out_data_ready_func, 1);
					return;
				}
			}
		}
	}

	if (!ako)
	{
		m_b = -1;
	}

	if (ako != m_ako)
	{
		devcb_call_write_line(&m_out_ako_func, ako);
		m_ako = ako;
	}
}


//-------------------------------------------------
//  b_r - 
//-------------------------------------------------

UINT16 ay3600_device::b_r()
{
	UINT16 data = m_b;

	devcb_call_write_line(&m_out_data_ready_func, 0);

	return data;
}
