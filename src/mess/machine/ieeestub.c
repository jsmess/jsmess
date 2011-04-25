#include "ieee488.h"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type IEEE488_STUB = ieee488_stub_device_config::static_alloc_device_config;



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  ieee488_stub_device_config - constructor
//-------------------------------------------------

ieee488_stub_device_config::ieee488_stub_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
	: device_config(mconfig, static_alloc_device_config, "IEEE488 driver stub", tag, owner, clock),
	  device_config_ieee488_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  static_alloc_device_config - allocate a new
//  configuration object
//-------------------------------------------------

device_config *ieee488_stub_device_config::static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
{
	return global_alloc(ieee488_stub_device_config(mconfig, tag, owner, clock));
}


//-------------------------------------------------
//  alloc_device - allocate a new device object
//-------------------------------------------------

device_t *ieee488_stub_device_config::alloc_device(running_machine &machine) const
{
	return auto_alloc(machine, ieee488_stub_device(machine, *this));
}


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void ieee488_stub_device_config::device_config_complete()
{
	// inherit a copy of the static data
	const ieee488_stub_interface *intf = reinterpret_cast<const ieee488_stub_interface *>(static_config());
	if (intf != NULL)
		*static_cast<ieee488_stub_interface *>(this) = *intf;

	// or initialize to defaults if none provided
	else
	{
		memset(&m_out_eoi_func, 0, sizeof(m_out_eoi_func));
		memset(&m_out_dav_func, 0, sizeof(m_out_dav_func));
		memset(&m_out_nrfd_func, 0, sizeof(m_out_nrfd_func));
		memset(&m_out_ndac_func, 0, sizeof(m_out_ndac_func));
		memset(&m_out_ifc_func, 0, sizeof(m_out_ifc_func));
		memset(&m_out_srq_func, 0, sizeof(m_out_srq_func));
		memset(&m_out_atn_func, 0, sizeof(m_out_atn_func));
		memset(&m_out_ren_func, 0, sizeof(m_out_ren_func));
	}
}


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  ieee488_stub_device - constructor
//-------------------------------------------------

ieee488_stub_device::ieee488_stub_device(running_machine &_machine, const ieee488_stub_device_config &_config)
    : device_t(_machine, _config),
	  device_ieee488_interface(_machine, _config, *this),
      m_config(_config)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void ieee488_stub_device::device_start()
{
	// resolve callbacks
    devcb_resolve_write_line(&m_out_eoi_func, &m_config.m_out_eoi_func, this);
    devcb_resolve_write_line(&m_out_dav_func, &m_config.m_out_dav_func, this);
    devcb_resolve_write_line(&m_out_nrfd_func, &m_config.m_out_nrfd_func, this);
    devcb_resolve_write_line(&m_out_ndac_func, &m_config.m_out_ndac_func, this);
    devcb_resolve_write_line(&m_out_ifc_func, &m_config.m_out_ifc_func, this);
    devcb_resolve_write_line(&m_out_srq_func, &m_config.m_out_srq_func, this);
    devcb_resolve_write_line(&m_out_atn_func, &m_config.m_out_atn_func, this);
    devcb_resolve_write_line(&m_out_ren_func, &m_config.m_out_ren_func, this);
}


//-------------------------------------------------
//  ieee488_eoi -
//-------------------------------------------------

void ieee488_stub_device::ieee488_eoi(int state)
{
	devcb_call_write_line(&m_out_eoi_func, state);
}


//-------------------------------------------------
//  ieee488_dav -
//-------------------------------------------------

void ieee488_stub_device::ieee488_dav(int state)
{
	devcb_call_write_line(&m_out_dav_func, state);
}


//-------------------------------------------------
//  ieee488_nrfd -
//-------------------------------------------------

void ieee488_stub_device::ieee488_nrfd(int state)
{
	devcb_call_write_line(&m_out_nrfd_func, state);
}


//-------------------------------------------------
//  ieee488_ndac -
//-------------------------------------------------

void ieee488_stub_device::ieee488_ndac(int state)
{
	devcb_call_write_line(&m_out_ndac_func, state);
}


//-------------------------------------------------
//  ieee488_ifc -
//-------------------------------------------------

void ieee488_stub_device::ieee488_ifc(int state)
{
	devcb_call_write_line(&m_out_ifc_func, state);
}


//-------------------------------------------------
//  ieee488_srq -
//-------------------------------------------------

void ieee488_stub_device::ieee488_srq(int state)
{
	devcb_call_write_line(&m_out_srq_func, state);
}


//-------------------------------------------------
//  ieee488_atn -
//-------------------------------------------------

void ieee488_stub_device::ieee488_atn(int state)
{
	devcb_call_write_line(&m_out_atn_func, state);
}


//-------------------------------------------------
//  ieee488_ren -
//-------------------------------------------------

void ieee488_stub_device::ieee488_ren(int state)
{
	devcb_call_write_line(&m_out_ren_func, state);
}
