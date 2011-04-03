/***************************************************************************

        Hitachi HD44780 LCD controller

***************************************************************************/

#pragma once

#ifndef __HD44780_H__
#define __HD44780_H__


#define MCFG_HD44780_ADD( _tag , _config) \
	MCFG_DEVICE_ADD( _tag, HD44780, 0 ) \
	MCFG_DEVICE_CONFIG(_config)

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> hd44780_interface

struct hd44780_interface
{
	UINT8 height;			// number of lines
	UINT8 width;			// chars for line
	const UINT8 *custom_layout;	// custom display layout (NULL for default)
};


// ======================> hd44780_device_config

class hd44780_device_config :
	public device_config,
	public hd44780_interface
{
	friend class hd44780_device;

	// construction/destruction
	hd44780_device_config( const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock );

public:
	// allocators
	static device_config *static_alloc_device_config( const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock );
	virtual device_t *alloc_device( running_machine &machine ) const;

protected:
	// device_config overrides
	virtual void device_config_complete();
	virtual bool device_validity_check( const game_driver &driver ) const;
};


// ======================> hd44780_device

class hd44780_device :
	public device_t
{
	friend class hd44780_device_config;

	// construction/destruction
	hd44780_device( running_machine &_machine, const hd44780_device_config &config );

public:
	// device interface
	DECLARE_WRITE8_MEMBER(control_write);
	DECLARE_READ8_MEMBER(control_read);
	DECLARE_WRITE8_MEMBER(data_write);
	DECLARE_READ8_MEMBER(data_read);

	int video_update(bitmap_t *bitmap, const rectangle *cliprect);

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);

private:
	// internal helper
	void set_busy_flag(UINT16 usec);
	void update_ac(void);
	// internal state
	const hd44780_device_config &m_config;
	static const device_timer_id BUSY_TIMER = 0;
	static const device_timer_id BLINKING_TIMER = 1;

	emu_timer *m_blink_timer;
	emu_timer *m_busy_timer;

	UINT8 m_busy_flag;

	UINT8 m_ddram[0x80];	//internal display data RAM
	UINT8 m_cgram[0x40];	//internal chargen RAM

	INT8 m_ac;				//address counter
	UINT8 m_ac_mode;		//0=DDRAM 1=CGRAM
	UINT8 m_data_bus_flag;	//0=none 1=write 2=read

	INT8 m_cursor_pos;		//cursor position
	UINT8 m_display_on;		//display on/off
	UINT8 m_cursor_on;		//cursor on/off
	UINT8 m_blink_on;		//blink on/off
	UINT8 m_shift_on;		//shift  on/off
	INT8 m_disp_shift;		//display shift

	INT8 m_direction;		//auto increment/decrement
	UINT8 m_data_len;		//interface data length 4 or 8 bit
	UINT8 m_num_line;		//number of lines
	UINT8 m_char_size;		//char size 5x8 or 5x10

	UINT8 m_blink;
};

// device type definition
extern const device_type HD44780;

#endif
