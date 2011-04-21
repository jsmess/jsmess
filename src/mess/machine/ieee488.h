/**********************************************************************

    IEEE-488.1 General Purpose Interface Bus emulation
    (aka HP-IB, GPIB, CBM IEEE)

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __IEEE488__
#define __IEEE488__

#include "emu.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define IEEE488_TAG			"ieee_bus"
#define IEEE488_STUB_TAG	"ieee_stub"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_IEEE488_ADD(_daisy) \
    MCFG_DEVICE_ADD(IEEE488_STUB_TAG, IEEE488_STUB, 0) \
	MCFG_DEVICE_CONFIG(default_ieee488_stub_interface) \
	MCFG_DEVICE_ADD(IEEE488_TAG, IEEE488, 0) \
	MCFG_DEVICE_CONFIG(_daisy)


#define MCFG_IEEE488_CONFIG_ADD(_daisy, _config) \
    MCFG_DEVICE_ADD(IEEE488_STUB_TAG, IEEE488_STUB, 0) \
	MCFG_DEVICE_CONFIG(_config) \
	MCFG_DEVICE_ADD(IEEE488_TAG, IEEE488, 0) \
	MCFG_DEVICE_CONFIG(_daisy)


#define MCFG_IEEE488_REMOVE() \
	MCFG_DEVICE_REMOVE(IEEE488_STUB_TAG) \
	MCFG_DEVICE_REMOVE(IEEE488_TAG)


#define IEEE488_DAISY(_name) \
	const ieee488_config (_name)[] =


#define IEEE488_INTERFACE(_name) \
	const ieee488_stub_interface (_name) =



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> ieee488_config

struct ieee488_config
{
	const char *m_tag;
};


// ======================> device_config_ieee488_interface

// device_config_ieee488_interface represents configuration information for a ieee488 device
class device_config_ieee488_interface : public device_config_interface
{
public:
	// construction/destruction
	device_config_ieee488_interface(const machine_config &mconfig, device_config &devconfig);
	virtual ~device_config_ieee488_interface();
};


// ======================> device_ieee488_interface

class device_ieee488_interface : public device_interface
{
public:
	// construction/destruction
	device_ieee488_interface(running_machine &machine, const device_config &config, device_t &device);
	virtual ~device_ieee488_interface();

	// optional operation overrides
	virtual void ieee488_eoi(int state) { };
	virtual void ieee488_dav(int state) { };
	virtual void ieee488_nrfd(int state) { };
	virtual void ieee488_ndac(int state) { };
	virtual void ieee488_ifc(int state) { };
	virtual void ieee488_srq(int state) { };
	virtual void ieee488_atn(int state) { };
	virtual void ieee488_ren(int state) { };

protected:
	const device_config_ieee488_interface &m_ieee488_config;
};


// ======================> ieee488_device_config

class ieee488_device_config :   public device_config,
							    public ieee488_config
{
    friend class ieee488_device;

    // construction/destruction
    ieee488_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);

public:
    // allocators
    static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
    virtual device_t *alloc_device(running_machine &machine) const;

protected:
    // device_config overrides
    virtual void device_config_complete();

private:
	const ieee488_config *m_daisy;
};


// ======================> ieee488_device

class ieee488_stub_device;

class ieee488_device :  public device_t
{
    friend class ieee488_device_config;

    // construction/destruction
    ieee488_device(running_machine &_machine, const ieee488_device_config &_config);

public:
	// reads for both host and peripherals
	UINT8 dio_r();
	READ8_MEMBER( dio_r );
	DECLARE_READ_LINE_MEMBER( eoi_r );
	DECLARE_READ_LINE_MEMBER( dav_r );
	DECLARE_READ_LINE_MEMBER( nrfd_r );
	DECLARE_READ_LINE_MEMBER( ndac_r );
	DECLARE_READ_LINE_MEMBER( ifc_r );
	DECLARE_READ_LINE_MEMBER( srq_r );
	DECLARE_READ_LINE_MEMBER( atn_r );
	DECLARE_READ_LINE_MEMBER( ren_r );

	// writes for host (driver_device)
	void dio_w(UINT8 data);
	WRITE8_MEMBER( dio_w );
	DECLARE_WRITE_LINE_MEMBER( eoi_w );
	DECLARE_WRITE_LINE_MEMBER( dav_w );
	DECLARE_WRITE_LINE_MEMBER( nrfd_w );
	DECLARE_WRITE_LINE_MEMBER( ndac_w );
	DECLARE_WRITE_LINE_MEMBER( ifc_w );
	DECLARE_WRITE_LINE_MEMBER( srq_w );
	DECLARE_WRITE_LINE_MEMBER( atn_w );
	DECLARE_WRITE_LINE_MEMBER( ren_w );

	// writes for peripherals (device_t)
	void dio_w(device_t *device, UINT8 data);
	void eoi_w(device_t *device, int state);
	void dav_w(device_t *device, int state);
	void nrfd_w(device_t *device, int state);
	void ndac_w(device_t *device, int state);
	void ifc_w(device_t *device, int state);
	void srq_w(device_t *device, int state);
	void atn_w(device_t *device, int state);
	void ren_w(device_t *device, int state);

protected:
	enum
	{
		EOI = 0,
		DAV,
		NRFD,
		NDAC,
		IFC,
		SRQ,
		ATN,
		REN,
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
		device_ieee488_interface *	m_interface;	// associated device's daisy interface

		int m_line[SIGNAL_COUNT];
		UINT8 m_dio;
	};

	daisy_entry *			m_daisy_list;	// head of the daisy chain

private:
	inline void set_signal(device_t *device, int signal, int state);
	inline int get_signal(int signal);
	inline void set_data(device_t *device, UINT8 data);
	inline UINT8 get_data();

	required_device<ieee488_stub_device> m_stub;

    const ieee488_device_config &m_config;
};


// ======================> ieee488_stub_interface

struct ieee488_stub_interface
{
	devcb_write_line	m_out_eoi_func;
	devcb_write_line	m_out_dav_func;
	devcb_write_line	m_out_nrfd_func;
	devcb_write_line	m_out_ndac_func;
	devcb_write_line	m_out_ifc_func;
	devcb_write_line	m_out_srq_func;
	devcb_write_line	m_out_atn_func;
	devcb_write_line	m_out_ren_func;
};

const ieee488_stub_interface default_ieee488_stub_interface = { DEVCB_NULL, };


// ======================> ieee488_stub_device_config

class ieee488_stub_device_config :   public device_config,
									 public ieee488_stub_interface,
									 public device_config_ieee488_interface
{
    friend class ieee488_stub_device;

    // construction/destruction
    ieee488_stub_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);

public:
    // allocators
    static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
    virtual device_t *alloc_device(running_machine &machine) const;

protected:
    // device_config overrides
    virtual void device_config_complete();
};


// ======================> ieee488_stub_device

class ieee488_stub_device :  public device_t,
							 public device_ieee488_interface
{
    friend class ieee488_stub_device_config;

    // construction/destruction
    ieee488_stub_device(running_machine &_machine, const ieee488_stub_device_config &_config);

protected:
    // device-level overrides
    virtual void device_start();

	// device_ieee488_interface overrides
	void ieee488_eoi(int state);
	void ieee488_dav(int state);
	void ieee488_nrfd(int state);
	void ieee488_ndac(int state);
	void ieee488_ifc(int state);
	void ieee488_srq(int state);
	void ieee488_atn(int state);
	void ieee488_ren(int state);

private:
	devcb_resolved_write_line	m_out_eoi_func;
	devcb_resolved_write_line	m_out_dav_func;
	devcb_resolved_write_line	m_out_nrfd_func;
	devcb_resolved_write_line	m_out_ndac_func;
	devcb_resolved_write_line	m_out_ifc_func;
	devcb_resolved_write_line	m_out_srq_func;
	devcb_resolved_write_line	m_out_atn_func;
	devcb_resolved_write_line	m_out_ren_func;

    const ieee488_stub_device_config &m_config;
};


// device type definition
extern const device_type IEEE488;
extern const device_type IEEE488_STUB;


#endif
