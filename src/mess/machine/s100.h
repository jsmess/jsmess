/**********************************************************************

	S-100 (IEEE-696/1983) bus emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __S100__
#define __S100__

#include "emu.h"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_S100_BUS_ADD(_tag, _cpu_tag, _config) \
    MCFG_DEVICE_ADD(_tag, S100, 0) \
    MCFG_DEVICE_CONFIG(_config) \
    s100_device::static_set_cputag(*device, _cpu_tag); \

	
#define S100_INTERFACE(_name) \
	const s100_bus_interface (_name) =
	
	
#define MCFG_S100_SLOT_ADD(_bus_tag, _num, _tag, _slot_intf, _def_slot) \
    MCFG_DEVICE_ADD(_tag, S100_SLOT, 0) \
	MCFG_DEVICE_SLOT_INTERFACE(_slot_intf, _def_slot) \
	s100_slot_device::static_set_s100_slot(*device, _bus_tag, _num); \

	
//**************************************************************************
//  CONSTANTS
//**************************************************************************

#define MAX_S100_SLOTS	16



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

class s100_device;

class s100_slot_device : public device_t,
						 public device_slot_interface
{
public:
	// construction/destruction
	s100_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	
	// device-level overrides
	virtual void device_start();

    // inline configuration
    static void static_set_s100_slot(device_t &device, const char *tag, int num);

private:
	// configuration
	const char *m_bus_tag;
	int m_bus_num;
	s100_device  *m_bus;
};


// device type definition
extern const device_type S100_SLOT;


// ======================> s100_bus_interface

struct s100_bus_interface
{
    devcb_write_line	m_out_int_cb;
    devcb_write_line	m_out_nmi_cb;
    devcb_write_line	m_out_vi0_cb;
    devcb_write_line	m_out_vi1_cb;
    devcb_write_line	m_out_vi2_cb;
    devcb_write_line	m_out_vi3_cb;
    devcb_write_line	m_out_vi4_cb;
    devcb_write_line	m_out_vi5_cb;
    devcb_write_line	m_out_vi6_cb;
    devcb_write_line	m_out_vi7_cb;
};

class device_s100_card_interface;


// ======================> s100_device

class s100_device : public device_t,
                    public s100_bus_interface
{
public:
	// construction/destruction
	s100_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	// inline configuration
	static void static_set_cputag(device_t &device, const char *tag);

	void add_s100_card(device_s100_card_interface *card,int pos);

	DECLARE_WRITE_LINE_MEMBER( int_w );
	DECLARE_WRITE_LINE_MEMBER( nmi_w );
	DECLARE_WRITE_LINE_MEMBER( vi0_w );
	DECLARE_WRITE_LINE_MEMBER( vi1_w );
	DECLARE_WRITE_LINE_MEMBER( vi2_w );
	DECLARE_WRITE_LINE_MEMBER( vi3_w );
	DECLARE_WRITE_LINE_MEMBER( vi4_w );
	DECLARE_WRITE_LINE_MEMBER( vi5_w );
	DECLARE_WRITE_LINE_MEMBER( vi6_w );
	DECLARE_WRITE_LINE_MEMBER( vi7_w );

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();
	virtual void device_config_complete();

private:
	// internal state
	device_t   *m_maincpu;

	devcb_resolved_write_line	m_out_int_func;
	devcb_resolved_write_line	m_out_nmi_func;
	devcb_resolved_write_line	m_out_vi0_func;
	devcb_resolved_write_line	m_out_vi1_func;
	devcb_resolved_write_line	m_out_vi2_func;
	devcb_resolved_write_line	m_out_vi3_func;
	devcb_resolved_write_line	m_out_vi4_func;
	devcb_resolved_write_line	m_out_vi5_func;
	devcb_resolved_write_line	m_out_vi6_func;
	devcb_resolved_write_line	m_out_vi7_func;

	device_s100_card_interface *m_s100_device[MAX_S100_SLOTS];
	const char *m_cputag;
};


// device type definition
extern const device_type S100;


// ======================> device_s100_card_interface

// class representing interface-specific live s100 card
class device_s100_card_interface : public device_interface
{
	friend class s100_device;
	
public:
	// construction/destruction
	device_s100_card_interface(const machine_config &mconfig, device_t &device);
	virtual ~device_s100_card_interface();

	// interrupts
	virtual void int_w(int state) { };
	virtual void nmi_w(int state) { };

	// vectored interrupts
	virtual void vi0_w(int state) { };
	virtual void vi1_w(int state) { };
	virtual void vi2_w(int state) { };
	virtual void vi3_w(int state) { };
	virtual void vi4_w(int state) { };
	virtual void vi5_w(int state) { };
	virtual void vi6_w(int state) { };
	virtual void vi7_w(int state) { };

	// configuration access
	virtual int sixtn_r() { return 1; }

    // inline configuration
    static void static_set_s100_tag(device_t &device, const char *tag);
	
public:
	s100_device  *m_s100;
};

#endif
