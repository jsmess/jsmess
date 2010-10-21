/***************************************************************************

        Hitachi HD44780 LCD controller

***************************************************************************/

#pragma once

#ifndef __HD44780_H__
#define __HD44780_H__


#define MDRV_HD44780_ADD( _tag , _config) \
	MDRV_DEVICE_ADD( _tag, HD44780, 0 ) \
	MDRV_DEVICE_CONFIG(_config)

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> hd44780_interface

struct hd44780_interface
{
	UINT8 height;			// number of lines
	UINT8 width;			// chars for line
	UINT8 *custom_layout;	// custom display layout (NULL for default)
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
	void control_write(offs_t offset, UINT8 data);
	UINT8 control_read(offs_t offset);
	void data_write(offs_t offset, UINT8 data);
	UINT8 data_read(offs_t offset);

	int video_update(bitmap_t *bitmap, const rectangle *cliprect);

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();

	// internal helper
	static TIMER_CALLBACK( bf_clear );
	static TIMER_CALLBACK( blink_timer );

	void set_busy_flag(UINT16 usec);

	// internal state
	const hd44780_device_config &m_config;

	emu_timer *busy_timer;
	UINT8 *chargen;
	UINT8 busy_flag;

	UINT8 ddram[0x80];	//internal display data RAM
	UINT8 cgram[0x40];	//internal chargen RAM

	INT8 ac;			//address counter
	UINT8 ac_mode;		//0=DDRAM 1=CGRAM
	UINT8 data_bus_flag;//0=none 1=write 2=read

	INT8 cursor_pos;	//cursor position
	UINT8 display_on;	//display on/off
	UINT8 cursor_on;	//cursor on/off
	UINT8 blink_on;		//blink on/off
	UINT8 shift_on;		//shift  on/off
	INT8 disp_shift;	//display shift

	INT8 direction;		//auto increment/decrement
	UINT8 data_len;		//interface data length 4 or 8 bit
	UINT8 n_line;		//number of lines
	UINT8 char_size;	//char size 5x8 or 5x10

	UINT8 blink;
};

// device type definition
extern const device_type HD44780;


//**************************************************************************
//  READ/WRITE HANDLERS
//**************************************************************************

WRITE8_DEVICE_HANDLER( hd44780_control_w );
WRITE8_DEVICE_HANDLER( hd44780_data_w );
READ8_DEVICE_HANDLER( hd44780_control_r );
READ8_DEVICE_HANDLER( hd44780_data_r );

#endif
