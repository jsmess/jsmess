/**********************************************************************

    Commodore IEC Serial Bus emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __CBM_IEC__
#define __CBM_IEC__

#include "emu.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define CBM_IEC_TAG			"iec_bus"
#define CBM_IEC_STUB_TAG	"iec_stub"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_CBM_IEC_ADD(_daisy) \
    MCFG_DEVICE_ADD(CBM_IEC_STUB_TAG, CBM_IEC_STUB, 0) \
	MCFG_DEVICE_CONFIG(default_cbm_iec_stub_interface) \
	MCFG_DEVICE_ADD(CBM_IEC_TAG, CBM_IEC, 0) \
	MCFG_DEVICE_CONFIG(_daisy)


#define MCFG_CBM_IEC_CONFIG_ADD(_daisy, _config) \
    MCFG_DEVICE_ADD(CBM_IEC_STUB_TAG, CBM_IEC_STUB, 0) \
	MCFG_DEVICE_CONFIG(_config) \
	MCFG_DEVICE_ADD(CBM_IEC_TAG, CBM_IEC, 0) \
	MCFG_DEVICE_CONFIG(_daisy)


#define MCFG_CBM_IEC_REMOVE() \
	MCFG_DEVICE_REMOVE(CBM_IEC_STUB_TAG) \
	MCFG_DEVICE_REMOVE(CBM_IEC_TAG)


#define CBM_IEC_DAISY(_name) \
	const cbm_iec_config (_name)[] =


#define CBM_IEC_INTERFACE(_name) \
	const cbm_iec_stub_interface (_name) =



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> cbm_iec_config

struct cbm_iec_config
{
	const char *m_tag;
};


// ======================> device_config_cbm_iec_interface

// device_config_cbm_iec_interface represents configuration information for a cbm_iec device
class device_config_cbm_iec_interface : public device_config_interface
{
public:
	// construction/destruction
	device_config_cbm_iec_interface(const machine_config &mconfig, device_config &devconfig);
	virtual ~device_config_cbm_iec_interface();
};


// ======================> device_cbm_iec_interface

class device_cbm_iec_interface : public device_interface
{
public:
	// construction/destruction
	device_cbm_iec_interface(running_machine &machine, const device_config &config, device_t &device);
	virtual ~device_cbm_iec_interface();

	// optional operation overrides
	virtual void cbm_iec_atn(int state) { };
	virtual void cbm_iec_clk(int state) { };
	virtual void cbm_iec_data(int state) { };
	virtual void cbm_iec_srq(int state) { };
	virtual void cbm_iec_reset(int state) { };

protected:
	const device_config_cbm_iec_interface &m_cbm_iec_config;
};


// ======================> cbm_iec_device_config

class cbm_iec_device_config :   public device_config,
							    public cbm_iec_config
{
    friend class cbm_iec_device;

    // construction/destruction
    cbm_iec_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);

public:
    // allocators
    static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
    virtual device_t *alloc_device(running_machine &machine) const;

protected:
    // device_config overrides
    virtual void device_config_complete();

private:
	const cbm_iec_config *m_daisy;
};


// ======================> cbm_iec_device

class cbm_iec_stub_device;

class cbm_iec_device :  public device_t
{
    friend class cbm_iec_device_config;

    // construction/destruction
    cbm_iec_device(running_machine &_machine, const cbm_iec_device_config &_config);

public:
	// reads for both host and peripherals
	DECLARE_READ_LINE_MEMBER( srq_r );
	DECLARE_READ_LINE_MEMBER( atn_r );
	DECLARE_READ_LINE_MEMBER( clk_r );
	DECLARE_READ_LINE_MEMBER( data_r );
	DECLARE_READ_LINE_MEMBER( reset_r );

	// writes for host (driver_device)
	DECLARE_WRITE_LINE_MEMBER( srq_w );
	DECLARE_WRITE_LINE_MEMBER( atn_w );
	DECLARE_WRITE_LINE_MEMBER( clk_w );
	DECLARE_WRITE_LINE_MEMBER( data_w );
	DECLARE_WRITE_LINE_MEMBER( reset_w );

	// writes for peripherals (device_t)
	void srq_w(device_t *device, int state);
	void atn_w(device_t *device, int state);
	void clk_w(device_t *device, int state);
	void data_w(device_t *device, int state);
	void reset_w(device_t *device, int state);

protected:
	enum
	{
		SRQ = 0,
		ATN,
		CLK,
		DATA,
		RESET,
		SIGNAL_COUNT
	};

	// device-level overrides
    virtual void device_start();

	class daisy_entry
	{
	public:
		daisy_entry(device_t *device);

		daisy_entry *				m_next;			// next device
		device_t *					m_device;		// associated device
		device_cbm_iec_interface *	m_interface;	// associated device's daisy interface

		int m_line[SIGNAL_COUNT];
	};

	daisy_entry *			m_daisy_list;	// head of the daisy chain

private:
	inline void set_signal(device_t *device, int signal, int state);
	inline int get_signal(int signal);

	required_device<cbm_iec_stub_device> m_stub;

    const cbm_iec_device_config &m_config;
};


// ======================> cbm_iec_stub_interface

struct cbm_iec_stub_interface
{
	devcb_write_line	m_out_srq_func;
	devcb_write_line	m_out_atn_func;
	devcb_write_line	m_out_clk_func;
	devcb_write_line	m_out_data_func;
	devcb_write_line	m_out_reset_func;
};

const cbm_iec_stub_interface default_cbm_iec_stub_interface = { DEVCB_NULL, };


// ======================> cbm_iec_stub_device_config

class cbm_iec_stub_device_config :   public device_config,
									 public cbm_iec_stub_interface,
									 public device_config_cbm_iec_interface
{
    friend class cbm_iec_stub_device;

    // construction/destruction
    cbm_iec_stub_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);

public:
    // allocators
    static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
    virtual device_t *alloc_device(running_machine &machine) const;

protected:
    // device_config overrides
    virtual void device_config_complete();
};


// ======================> cbm_iec_stub_device

class cbm_iec_stub_device :  public device_t,
							 public device_cbm_iec_interface
{
    friend class cbm_iec_stub_device_config;

    // construction/destruction
    cbm_iec_stub_device(running_machine &_machine, const cbm_iec_stub_device_config &_config);

protected:
    // device-level overrides
    virtual void device_start();

	// device_cbm_iec_interface overrides
	void cbm_iec_atn(int state);
	void cbm_iec_clk(int state);
	void cbm_iec_data(int state);
	void cbm_iec_srq(int state);
	void cbm_iec_reset(int state);

private:
	devcb_resolved_write_line	m_out_atn_func;
	devcb_resolved_write_line	m_out_clk_func;
	devcb_resolved_write_line	m_out_data_func;
	devcb_resolved_write_line	m_out_srq_func;
	devcb_resolved_write_line	m_out_reset_func;

    const cbm_iec_stub_device_config &m_config;
};


// device type definition
extern const device_type CBM_IEC;
extern const device_type CBM_IEC_STUB;


#endif
