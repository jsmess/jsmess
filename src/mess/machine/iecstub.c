#include "cbmiec.h"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type CBM_IEC_STUB = &device_creator<cbm_iec_stub_device>;

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void cbm_iec_stub_device::device_config_complete()
{
	// inherit a copy of the static data
	const cbm_iec_stub_interface *intf = reinterpret_cast<const cbm_iec_stub_interface *>(static_config());
	if (intf != NULL)
		*static_cast<cbm_iec_stub_interface *>(this) = *intf;

	// or initialize to defaults if none provided
	else
	{
		memset(&m_out_srq_cb, 0, sizeof(m_out_srq_cb));
		memset(&m_out_atn_cb, 0, sizeof(m_out_atn_cb));
		memset(&m_out_clk_cb, 0, sizeof(m_out_clk_cb));
		memset(&m_out_data_cb, 0, sizeof(m_out_data_cb));
		memset(&m_out_reset_cb, 0, sizeof(m_out_reset_cb));
	}
}


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  cbm_iec_stub_device - constructor
//-------------------------------------------------

cbm_iec_stub_device::cbm_iec_stub_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, CBM_IEC_STUB, "CBM IEC driver stub", tag, owner, clock),
	  device_cbm_iec_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void cbm_iec_stub_device::device_start()
{
	// resolve callbacks
    devcb_resolve_write_line(&m_out_srq_func, &m_out_srq_cb, this);
    devcb_resolve_write_line(&m_out_atn_func, &m_out_atn_cb, this);
    devcb_resolve_write_line(&m_out_clk_func, &m_out_clk_cb, this);
    devcb_resolve_write_line(&m_out_data_func, &m_out_data_cb, this);
    devcb_resolve_write_line(&m_out_reset_func, &m_out_reset_cb, this);
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
