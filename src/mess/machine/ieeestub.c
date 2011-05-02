#include "ieee488.h"

const device_type IEEE488_STUB = &device_creator<ieee488_stub_device>;

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void ieee488_stub_device::device_config_complete()
{
	// inherit a copy of the static data
	const ieee488_stub_interface *intf = reinterpret_cast<const ieee488_stub_interface *>(static_config());
	if (intf != NULL)
		*static_cast<ieee488_stub_interface *>(this) = *intf;

	// or initialize to defaults if none provided
	else
	{
		memset(&m_out_eoi_cb, 0, sizeof(m_out_eoi_cb));
		memset(&m_out_dav_cb, 0, sizeof(m_out_dav_cb));
		memset(&m_out_nrfd_cb, 0, sizeof(m_out_nrfd_cb));
		memset(&m_out_ndac_cb, 0, sizeof(m_out_ndac_cb));
		memset(&m_out_ifc_cb, 0, sizeof(m_out_ifc_cb));
		memset(&m_out_srq_cb, 0, sizeof(m_out_srq_cb));
		memset(&m_out_atn_cb, 0, sizeof(m_out_atn_cb));
		memset(&m_out_ren_cb, 0, sizeof(m_out_ren_cb));
	}
}


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  ieee488_stub_device - constructor
//-------------------------------------------------

ieee488_stub_device::ieee488_stub_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, IEEE488_STUB, "IEEE488 driver stub", tag, owner, clock),
	  device_ieee488_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void ieee488_stub_device::device_start()
{
	// resolve callbacks
    m_out_eoi_func.resolve(m_out_eoi_cb, *this);
    m_out_dav_func.resolve(m_out_dav_cb, *this);
    m_out_nrfd_func.resolve(m_out_nrfd_cb, *this);
    m_out_ndac_func.resolve(m_out_ndac_cb, *this);
    m_out_ifc_func.resolve(m_out_ifc_cb, *this);
    m_out_srq_func.resolve(m_out_srq_cb, *this);
    m_out_atn_func.resolve(m_out_atn_cb, *this);
    m_out_ren_func.resolve(m_out_ren_cb, *this);
}


//-------------------------------------------------
//  ieee488_eoi -
//-------------------------------------------------

void ieee488_stub_device::ieee488_eoi(int state)
{
	m_out_eoi_func(state);
}


//-------------------------------------------------
//  ieee488_dav -
//-------------------------------------------------

void ieee488_stub_device::ieee488_dav(int state)
{
	m_out_dav_func(state);
}


//-------------------------------------------------
//  ieee488_nrfd -
//-------------------------------------------------

void ieee488_stub_device::ieee488_nrfd(int state)
{
	m_out_nrfd_func(state);
}


//-------------------------------------------------
//  ieee488_ndac -
//-------------------------------------------------

void ieee488_stub_device::ieee488_ndac(int state)
{
	m_out_ndac_func(state);
}


//-------------------------------------------------
//  ieee488_ifc -
//-------------------------------------------------

void ieee488_stub_device::ieee488_ifc(int state)
{
	m_out_ifc_func(state);
}


//-------------------------------------------------
//  ieee488_srq -
//-------------------------------------------------

void ieee488_stub_device::ieee488_srq(int state)
{
	m_out_srq_func(state);
}


//-------------------------------------------------
//  ieee488_atn -
//-------------------------------------------------

void ieee488_stub_device::ieee488_atn(int state)
{
	m_out_atn_func(state);
}


//-------------------------------------------------
//  ieee488_ren -
//-------------------------------------------------

void ieee488_stub_device::ieee488_ren(int state)
{
	m_out_ren_func(state);
}
