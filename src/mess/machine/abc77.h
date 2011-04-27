#pragma once

#ifndef __ABC77__
#define __ABC77__

#include "emu.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define ABC77_TAG	"abc77"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_ABC77_ADD(_config) \
    MCFG_DEVICE_ADD(ABC77_TAG, ABC77, 0) \
	MCFG_DEVICE_CONFIG(_config)


#define ABC77_INTERFACE(_name) \
	const abc77_interface (_name) =



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> abc77_interface

struct abc77_interface
{
	devcb_write_line	m_out_txd_cb;
	devcb_write_line	m_out_clock_cb;
	devcb_write_line	m_out_keydown_cb;
};

// ======================> abc77_device

class abc77_device :  public device_t,
                      public abc77_interface
{
public:
    // construction/destruction
    abc77_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual const rom_entry *device_rom_region() const;
	virtual machine_config_constructor device_mconfig_additions() const;
	virtual const input_port_token *device_input_ports() const;

	DECLARE_READ8_MEMBER( p1_r );
	DECLARE_WRITE8_MEMBER( p2_w );
	DECLARE_READ8_MEMBER( t1_r );

	DECLARE_WRITE_LINE_MEMBER( rxd_w );
	DECLARE_READ_LINE_MEMBER( txd_r );
	DECLARE_WRITE_LINE_MEMBER( reset_w );

protected:
    // device-level overrides
    virtual void device_start();
	virtual void device_reset();
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);
	virtual void device_config_complete();
	
private:
	static const device_timer_id TIMER_SERIAL = 0;
	static const device_timer_id TIMER_RESET = 1;

	inline void serial_output(int state);
	inline void serial_clock();
	inline void key_down(int state);

	devcb_resolved_write_line	m_out_txd_func;
	devcb_resolved_write_line	m_out_clock_func;
	devcb_resolved_write_line	m_out_keydown_func;

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_discrete;

	int m_txd;						// transmit data
	int m_keylatch;					// keyboard row latch
	int m_keydown;					// key down
	int m_clock;					// transmit clock
	int m_hys;						// hysteresis
	int m_reset;					// reset

	// timers
	emu_timer *m_serial_timer;
	emu_timer *m_reset_timer;
};


// device type definition
extern const device_type ABC77;



#endif
