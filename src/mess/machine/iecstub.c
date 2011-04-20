#include "cbmiec.h"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type CBM_IEC_STUB = cbm_iec_stub_device_config::static_alloc_device_config;



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  cbm_iec_stub_device_config - constructor
//-------------------------------------------------

cbm_iec_stub_device_config::cbm_iec_stub_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
	: device_config(mconfig, static_alloc_device_config, "CBM IEC driver stub", tag, owner, clock),
	  device_config_cbm_iec_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  static_alloc_device_config - allocate a new
//  configuration object
//-------------------------------------------------

device_config *cbm_iec_stub_device_config::static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
{
	return global_alloc(cbm_iec_stub_device_config(mconfig, tag, owner, clock));
}


//-------------------------------------------------
//  alloc_device - allocate a new device object
//-------------------------------------------------

device_t *cbm_iec_stub_device_config::alloc_device(running_machine &machine) const
{
	return auto_alloc(machine, cbm_iec_stub_device(machine, *this));
}


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void cbm_iec_stub_device_config::device_config_complete()
{
	// inherit a copy of the static data
	const cbm_iec_stub_interface *intf = reinterpret_cast<const cbm_iec_stub_interface *>(static_config());
	if (intf != NULL)
		*static_cast<cbm_iec_stub_interface *>(this) = *intf;

	// or initialize to defaults if none provided
	else
	{
		memset(&m_out_srq_func, 0, sizeof(m_out_srq_func));
		memset(&m_out_atn_func, 0, sizeof(m_out_atn_func));
		memset(&m_out_clk_func, 0, sizeof(m_out_clk_func));
		memset(&m_out_data_func, 0, sizeof(m_out_data_func));
		memset(&m_out_reset_func, 0, sizeof(m_out_reset_func));
	}
}


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  cbm_iec_stub_device - constructor
//-------------------------------------------------

cbm_iec_stub_device::cbm_iec_stub_device(running_machine &_machine, const cbm_iec_stub_device_config &_config)
    : device_t(_machine, _config),
	  device_cbm_iec_interface(_machine, _config, *this),
      m_config(_config)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void cbm_iec_stub_device::device_start()
{
	// resolve callbacks
    devcb_resolve_write_line(&m_out_srq_func, &m_config.m_out_srq_func, this);
    devcb_resolve_write_line(&m_out_atn_func, &m_config.m_out_atn_func, this);
    devcb_resolve_write_line(&m_out_clk_func, &m_config.m_out_clk_func, this);
    devcb_resolve_write_line(&m_out_data_func, &m_config.m_out_data_func, this);
    devcb_resolve_write_line(&m_out_reset_func, &m_config.m_out_reset_func, this);
}


//-------------------------------------------------
//  cbm_iec_srq - 
//-------------------------------------------------

void cbm_iec_stub_device::cbm_iec_srq(int state)
{
	devcb_call_write_line(&m_out_srq_func, state);
}


//-------------------------------------------------
//  cbm_iec_atn - 
//-------------------------------------------------

void cbm_iec_stub_device::cbm_iec_atn(int state)
{
	devcb_call_write_line(&m_out_atn_func, state);
}


//-------------------------------------------------
//  cbm_iec_clk - 
//-------------------------------------------------

void cbm_iec_stub_device::cbm_iec_clk(int state)
{
	devcb_call_write_line(&m_out_clk_func, state);
}


//-------------------------------------------------
//  cbm_iec_data - 
//-------------------------------------------------

void cbm_iec_stub_device::cbm_iec_data(int state)
{
	devcb_call_write_line(&m_out_data_func, state);
}


//-------------------------------------------------
//  cbm_iec_reset - 
//-------------------------------------------------

void cbm_iec_stub_device::cbm_iec_reset(int state)
{
	devcb_call_write_line(&m_out_reset_func, state);
}
