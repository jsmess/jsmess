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
	devcb_write_line	m_out_dma0_cb;
	devcb_write_line	m_out_dma1_cb;
	devcb_write_line	m_out_dma2_cb;
	devcb_write_line	m_out_dma3_cb;
	devcb_write_line	m_out_rdy_cb;
	devcb_write_line	m_out_hold_cb;
	devcb_write_line	m_out_error_cb;
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

	void add_s100_card(device_s100_card_interface *card, int pos);

	DECLARE_READ8_MEMBER( smemr_r );
	DECLARE_WRITE8_MEMBER( mwrt_w );
	
	DECLARE_READ8_MEMBER( sinp_r );
	DECLARE_WRITE8_MEMBER( sout_w );

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
	DECLARE_WRITE_LINE_MEMBER( dma0_w );
	DECLARE_WRITE_LINE_MEMBER( dma1_w );
	DECLARE_WRITE_LINE_MEMBER( dma2_w );
	DECLARE_WRITE_LINE_MEMBER( dma3_w );
	DECLARE_WRITE_LINE_MEMBER( rdy_w );
	DECLARE_WRITE_LINE_MEMBER( hold_w );
	DECLARE_WRITE_LINE_MEMBER( error_w );

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
	devcb_resolved_write_line	m_out_dma0_func;
	devcb_resolved_write_line	m_out_dma1_func;
	devcb_resolved_write_line	m_out_dma2_func;
	devcb_resolved_write_line	m_out_dma3_func;
	devcb_resolved_write_line	m_out_rdy_func;
	devcb_resolved_write_line	m_out_hold_func;
	devcb_resolved_write_line	m_out_error_func;

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
	virtual void s100_int_w(int state) { };
	virtual void s100_nmi_w(int state) { };
	virtual UINT8 s100_sinta_r(offs_t offset) { return 0; };

	// vectored interrupts
	virtual void s100_vi0_w(int state) { };
	virtual void s100_vi1_w(int state) { };
	virtual void s100_vi2_w(int state) { };
	virtual void s100_vi3_w(int state) { };
	virtual void s100_vi4_w(int state) { };
	virtual void s100_vi5_w(int state) { };
	virtual void s100_vi6_w(int state) { };
	virtual void s100_vi7_w(int state) { };
	
	// memory access
	virtual UINT8 s100_smemr_r(offs_t offset) { return 0; };
	virtual void s100_mwrt_w(offs_t offset, UINT8 data) { };

	// I/O access
	virtual UINT8 s100_sinp_r(offs_t offset) { return 0; };
	virtual void s100_sout_w(offs_t offset, UINT8 data) { };
	
	// configuration access
	virtual void s100_phlda_w(int state) { }
	virtual void s100_shalta_w(int state) { }
	virtual void s100_phantom_w(int state) { }
	virtual void s100_sxtrq_w(int state) { }
	virtual int s100_sixtn_r() { return 1; }
	
	// reset
	virtual void s100_poc_w(int state) { }
	virtual void s100_reset_w(int state) { }
	virtual void s100_slave_clr_w(int state) { }

    // inline configuration
    static void static_set_s100_tag(device_t &device, const char *tag);
	
public:
	s100_device  *m_s100;
};

#endif
