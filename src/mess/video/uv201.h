/**********************************************************************

    VideoBrain UV201/UV202 video chip emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
 
**********************************************************************/

#pragma once

#ifndef __UV201__
#define __UV201__

#include "emu.h"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_UV201_ADD(_tag, _screen_tag, _clock, _config) \
	MCFG_DEVICE_ADD(_tag, UV201, _clock) \
	MCFG_DEVICE_CONFIG(_config) \
    MCFG_SCREEN_ADD(_screen_tag, RASTER) \
	MCFG_SCREEN_UPDATE_DEVICE(_tag, uv201_device, screen_update) \
	MCFG_SCREEN_RAW_PARAMS(_clock, 455, 0, 190, 525, 0, 243) \
    MCFG_PALETTE_LENGTH(32)


#define UV201_INTERFACE(name) \
	const uv201_interface (name) =



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> uv201_interface

struct uv201_interface
{
	const char *m_screen_tag;
	
	devcb_write_line		m_out_ext_int_cb;
	devcb_read8				m_in_db_cb;
};


// ======================> uv201_device

class uv201_device :	public device_t,
                        public uv201_interface
{
public:
    // construction/destruction
    uv201_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

    DECLARE_READ8_MEMBER( read );
    DECLARE_WRITE8_MEMBER( write );

    DECLARE_WRITE_LINE_MEMBER( ext_int_w );
    DECLARE_READ_LINE_MEMBER( kbd_r );

	UINT32 screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

protected:
    // device-level overrides
	virtual void device_config_complete();
    virtual void device_start();
    virtual void device_reset();
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);

private:
	enum
	{
		TIMER_Y_ODD,
		TIMER_Y_EVEN
	};

	void initialize_palette();
	int get_field_vpos();
	int get_field();
	void set_y_interrupt();
	void do_partial_update();

	devcb_resolved_write_line	m_out_ext_int_func;
	devcb_resolved_read8		m_in_db_func;

	screen_device *m_screen;

	UINT8 m_ram[0x90];
	UINT8 m_y_int;
	UINT8 m_fmod;
	UINT8 m_bg;
	UINT8 m_cmd;
	UINT8 m_freeze_x;
	UINT16 m_freeze_y;
	int m_field;

	// timers
	emu_timer *m_timer_y_odd;
	emu_timer *m_timer_y_even;
};


// device type definition
extern const device_type UV201;



#endif
