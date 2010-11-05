/***************************************************************************

        Hitachi HD44780 LCD controller

        TODO:
        - 4-bit mode
        - 5x10 chars
        - dump internal CGROM

***************************************************************************/

#include "emu.h"
#include "video/hd44780.h"


//**************************************************************************
//  device configuration
//**************************************************************************

//-------------------------------------------------
//  hd44780_device_config - constructor
//-------------------------------------------------

hd44780_device_config::hd44780_device_config( const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock ):
	device_config( mconfig, static_alloc_device_config, "HD44780", tag, owner, clock)
{
}


//-------------------------------------------------
//  static_alloc_device_config - allocate a new
//  configuration object
//-------------------------------------------------

device_config *hd44780_device_config::static_alloc_device_config( const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock )
{
	return global_alloc( hd44780_device_config( mconfig, tag, owner, clock ) );
}


//-------------------------------------------------
//  alloc_device - allocate a new device object
//-------------------------------------------------

device_t *hd44780_device_config::alloc_device( running_machine &machine ) const
{
	return auto_alloc( &machine, hd44780_device( machine, *this ) );
}

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void hd44780_device_config::device_config_complete()
{
	// inherit a copy of the static data
	const hd44780_interface *intf = reinterpret_cast<const hd44780_interface *>(static_config());

	if (intf != NULL)
		*static_cast<hd44780_interface *>(this) = *intf;

	// or initialize to defaults if none provided
	else
	{
		height = width = 0;
		custom_layout = NULL;
	}
}

//-------------------------------------------------
//  device_validity_check - perform validity checks
//  on this device
//-------------------------------------------------

bool hd44780_device_config::device_validity_check( const game_driver &driver ) const
{
	bool error = false;

	// display with more than 2 lines requires a layout
	if ((custom_layout == NULL && height > 2) || height == 0 || width == 0)
	{
		mame_printf_error("%s: %s device '%s' has invalid parameter\n", driver.source_file, driver.name, tag());
		error = true;
	}

	return error;
}

//**************************************************************************
//  live device
//**************************************************************************

//-------------------------------------------------
//  hd44780_device - constructor
//-------------------------------------------------

hd44780_device::hd44780_device( running_machine &_machine, const hd44780_device_config &config ) :
	device_t( _machine, config ),
	m_config( config )
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void hd44780_device::device_start()
{
	m_busy_timer = device_timer_alloc(*this, BUSY_TIMER);
	m_blink_timer = device_timer_alloc(*this, BLINKING_TIMER);

	timer_adjust_periodic(m_blink_timer, ATTOTIME_IN_MSEC(500), 0, ATTOTIME_IN_MSEC(500));

	state_save_register_device_item( this, 0, ac);
	state_save_register_device_item( this, 0, ac_mode);
	state_save_register_device_item( this, 0, data_bus_flag);
	state_save_register_device_item( this, 0, cursor_pos);
	state_save_register_device_item( this, 0, display_on);
	state_save_register_device_item( this, 0, cursor_on);
	state_save_register_device_item( this, 0, shift_on);
	state_save_register_device_item( this, 0, blink_on);
	state_save_register_device_item( this, 0, direction);
	state_save_register_device_item( this, 0, data_len);
	state_save_register_device_item( this, 0, n_line);
	state_save_register_device_item( this, 0, char_size);
	state_save_register_device_item( this, 0, disp_shift);
	state_save_register_device_item( this, 0, blink);
	state_save_register_device_item_array( this, 0, ddram);
	state_save_register_device_item_array( this, 0, cgram);
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void hd44780_device::device_reset()
{
	busy_flag = 0;

	memset(ddram, 0, ARRAY_LENGTH(ddram));
	memset(cgram, 0, ARRAY_LENGTH(cgram));
	ac = 0;
	ac_mode = 0;
	data_bus_flag = 0;
	cursor_pos = 0;
	display_on = 0;
	cursor_on = 0;
	shift_on = 0;
	blink_on = 0;
	direction = 1;
	data_len = 1;
	n_line = 0;
	char_size = 0;
	disp_shift = 0;
	blink = 0;

	set_busy_flag(1520);
}


//-------------------------------------------------
//  device_timer - handler timer events
//-------------------------------------------------
void hd44780_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	switch(id)
	{
		case BUSY_TIMER:
			if (data_bus_flag)
			{
				int new_ac = ac + direction;

				ac = (new_ac < 0) ? 0 : ((new_ac > 0x7f) ? 0x7f : new_ac);

				if (ac_mode == 0)
				{
					cursor_pos = ac;

					// display is shifted only after a write
					if (shift_on && data_bus_flag == 1)
						disp_shift += direction;
				}

				data_bus_flag = 0;
			}

			busy_flag = 0;
			break;

		case BLINKING_TIMER:
			blink = !blink;
			break;
	}
}

void hd44780_device::set_busy_flag(UINT16 usec)
{
	busy_flag = 1;

	timer_adjust_oneshot( m_busy_timer, ATTOTIME_IN_USEC( usec ), 0 );
}

//**************************************************************************
//  device interface
//**************************************************************************

int hd44780_device::video_update(bitmap_t *bitmap, const rectangle *cliprect)
{
	assert(m_config.height*9 <= bitmap->height && m_config.width*6 <= bitmap->width);

	bitmap_fill(bitmap, cliprect, 0);

	if (display_on)
		for (int l=0; l<m_config.height; l++)
			for (int i=0; i<m_config.width; i++)
			{
				UINT8 line_base = l * 0x40;
				UINT8 line_size = (n_line) ? 40 : 80;
				INT8 char_pos = line_base + i;

				// if specified uses the custom layout
				if (m_config.custom_layout != NULL)
					char_pos = m_config.custom_layout[l*m_config.width + i];
				else
				{
					char_pos += disp_shift;

					while (char_pos < 0 || (char_pos - line_base) >= line_size)
					{
						if (char_pos < 0)
							char_pos += line_size;
						else if (char_pos - line_base >= line_size)
							char_pos -= line_size;
					}
				}

				for (int y=0; y<8; y++)
					for (int x=0; x<5; x++)
						if (ddram[char_pos] <= 0x10)
						{
							//draw CGRAM characters
							*BITMAP_ADDR16(bitmap, l*9 + y, i*6 + x) = BIT(cgram[(ddram[char_pos]&0x07)*8+y], 4-x);
						}
						else
						{
							//draw CGROM characters
							*BITMAP_ADDR16(bitmap, l*9 + y, i*6 + x) = BIT(region()->u8(ddram[char_pos]*8+y), 4-x);
						}

				// if is the correct position draw cursor and blink
				if (char_pos == cursor_pos)
				{
					//draw the cursor
					if (cursor_on)
						for (int x=0; x<5; x++)
							*BITMAP_ADDR16(bitmap, l*9 + 7, i * 6 + x) = 1;

					if (!blink && blink_on)
						for (int y=0; y<7; y++)
							for (int x=0; x<5; x++)
								*BITMAP_ADDR16(bitmap, l*9 + y, i * 6 + x) = 1;
				}
			}

	return 0;

}

void hd44780_device::control_write(offs_t offset, UINT8 data)
{
	if (BIT(data, 7))
	{
		ac_mode = 0;
		ac = data & 0x7f;
		if (data != 0x81)
			cursor_pos = ac;
		set_busy_flag(37);
	}
	else if (BIT(data, 6))
	{
		ac_mode = 1;
		ac = data & 0x3f;
		set_busy_flag(37);
	}
	else if (BIT(data, 5))
	{
		data_len = BIT(data, 4);
		n_line = BIT(data, 3);
		char_size = BIT(data, 2);
		set_busy_flag(37);
	}
	else if (BIT(data, 4))
	{
		UINT8 direct = (BIT(data, 2)) ? +1 : -1;

		if (BIT(data, 3))
			disp_shift += direct;
		else
		{
			ac += direct;
			cursor_pos += direct;
		}

		set_busy_flag(37);
	}
	else if (BIT(data, 3))
	{
		display_on = BIT(data, 2);
		cursor_on = BIT(data, 1);
		blink_on = BIT(data, 0);

		set_busy_flag(37);
	}
	else if (BIT(data, 2))
	{
		direction = (BIT(data, 1)) ? +1 : -1;

		shift_on = BIT(data, 0);

		set_busy_flag(37);
	}
	else if (BIT(data, 1))
	{
		ac = 0;
		cursor_pos = 0;
		ac_mode = 0;
		direction = 1;
		disp_shift = 0;
		set_busy_flag(1520);
	}
	else if (BIT(data, 0))
	{
		ac = 0;
		cursor_pos = 0;
		ac_mode = 0;
		direction = 1;
		disp_shift = 0;
		memset(ddram, 0x20, ARRAY_LENGTH(ddram));
		memset(cgram, 0x20, ARRAY_LENGTH(cgram));
		set_busy_flag(1520);
	}
}

UINT8 hd44780_device::control_read(offs_t offset)
{
	return busy_flag<<7 || ac&0x7f;
}

void hd44780_device::data_write(offs_t offset, UINT8 data)
{
	if (ac_mode == 0)
		ddram[ac] = data;
	else
		cgram[ac] = data;

	data_bus_flag = 1;

	set_busy_flag(41);
}

UINT8 hd44780_device::data_read(offs_t offset)
{
	UINT8 data;

	if (ac_mode == 0)
		data = ddram[ac];
	else
		data = cgram[ac];

	data_bus_flag = 2;

	set_busy_flag(41);

	return data;
}

// devices
const device_type HD44780 = hd44780_device_config::static_alloc_device_config;
