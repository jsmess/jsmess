/***************************************************************************

        Toshiba T6A04 LCD controller

***************************************************************************/

#pragma once

#ifndef __T6A04_H__
#define __T6A04_H__

#define MCFG_T6A04_ADD( _tag, _config ) \
	MCFG_DEVICE_ADD( _tag, T6A04, 0 ) \
	MCFG_DEVICE_CONFIG(_config)

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> t6a04_interface

struct t6a04_interface
{
	UINT8 height;			// number of lines
	UINT8 width;			// pixels for line
};


// ======================> t6a04_device_config

class t6a04_device_config :
	public device_config,
	public t6a04_interface
{
	friend class t6a04_device;

	// construction/destruction
	t6a04_device_config( const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock );

public:
	// allocators
	static device_config *static_alloc_device_config( const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock );
	virtual device_t *alloc_device( running_machine &machine ) const;

protected:
	// device_config overrides
	virtual void device_config_complete();
	virtual bool device_validity_check( const game_driver &driver ) const;
};


// ======================> t6a04_device

class t6a04_device :
	public device_t
{
	friend class t6a04_device_config;

	// construction/destruction
	t6a04_device( running_machine &_machine, const t6a04_device_config &config );

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

private:
	// internal state
	const t6a04_device_config &m_config;

	UINT8 m_busy_flag;
	UINT8 m_lcd_ram[960];	//7680 bit (64*120)
	UINT8 m_display_on;
	UINT8 m_contrast;
	UINT8 m_xpos;
	UINT8 m_ypos;
	UINT8 m_zpos;
	INT8  m_direction;
	UINT8 m_active_counter;
	UINT8 m_word_len;
	UINT8 m_opa1;
	UINT8 m_opa2;
	UINT8 m_output_reg;
};

// device type definition
extern const device_type T6A04;

#endif
