/***************************************************************************

        Hitachi HD44780 LCD controller

        TODO:
        - 4-bit mode
        - 5x10 chars
        - dump internal CGROM

        HACKS:
        - A00 10 bit chars are tacked onto recreated chrrom at $700 (until internal rom is dumped)
        - A00/A02 drawing selected by sizeof romfile, A02 is $800, A00 is $860

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
	device_config(mconfig, static_alloc_device_config, "HD44780", tag, owner, clock)
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
	return auto_alloc(machine, hd44780_device( machine, *this ) );
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
	m_busy_timer = timer_alloc(BUSY_TIMER);
	m_blink_timer = timer_alloc(BLINKING_TIMER);

	m_blink_timer->adjust(attotime::from_msec(409), 0, attotime::from_msec(409));

	save_item( NAME(m_ac));
	save_item( NAME(m_ac_mode));
	save_item( NAME(m_data_bus_flag));
	save_item( NAME(m_cursor_pos));
	save_item( NAME(m_display_on));
	save_item( NAME(m_cursor_on));
	save_item( NAME(m_shift_on));
	save_item( NAME(m_blink_on));
	save_item( NAME(m_direction));
	save_item( NAME(m_data_len));
	save_item( NAME(m_num_line));
	save_item( NAME(m_char_size));
	save_item( NAME(m_disp_shift));
	save_item( NAME(m_blink));
	save_item( NAME(m_ddram));
	save_item( NAME(m_cgram));

}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void hd44780_device::device_reset()
{
	m_busy_flag = 0;

	memset(m_ddram, 0x20, sizeof(m_ddram)); // can't use 0 here as it would show CGRAM instead of blank space on a soft reset
	memset(m_cgram, 0, sizeof(m_cgram));
	m_ac = 0;
	m_ac_mode = 0;
	m_data_bus_flag = 0;
	m_cursor_pos = 0;
	m_display_on = 0;
	m_cursor_on = 0;
	m_shift_on = 0;
	m_blink_on = 0;
	m_direction = 1;
	m_data_len = -1; // must not be 0 or 1 on intial start to pick up first 4/8 bit mode change
	m_num_line = 0;
	m_char_size = 0;
	m_disp_shift = 0;
	m_blink = 0;

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
			m_busy_flag = 0;
			break;

		case BLINKING_TIMER:
			m_blink = !m_blink;
			break;
	}
}

void hd44780_device::set_busy_flag(UINT16 usec)
{
	m_busy_flag = 1;

	m_busy_timer->adjust( attotime::from_usec( usec ) );

}

//**************************************************************************
//  device interface
//**************************************************************************

int hd44780_device::video_update(bitmap_t *bitmap, const rectangle *cliprect)
{
	assert(m_config.height*9 <= bitmap->height && m_config.width*6 <= bitmap->width);

	bitmap_fill(bitmap, cliprect, 0);

	if (m_display_on)
		for (int l=0; l<m_config.height; l++)
			for (int i=0; i<m_config.width; i++)
			{
				UINT8 line_base = l * 0x40;
				UINT8 line_size = (m_num_line) ? 40 : 80;
				INT8 char_pos = line_base + i;

				// if specified uses the custom layout
				if (m_config.custom_layout != NULL)
					char_pos = m_config.custom_layout[l*m_config.width + i];
				else
				{
					char_pos += m_disp_shift;

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
						if (m_ddram[char_pos] <= 0x10)
						{
							//draw CGRAM characters
							*BITMAP_ADDR16(bitmap, l*9 + y, i*6 + x) = BIT(m_cgram[(m_ddram[char_pos]&0x07)*8+y], 4-x);
						}
						else
						{
							//draw CGROM characters
							if (region()->bytes() <= 0x800)
							{
								*BITMAP_ADDR16(bitmap, l*9 + y, i*6 + x) = BIT(region()->u8(m_ddram[char_pos]*8+y), 4-x);
							}
							else
							{
								if(m_ddram[char_pos] < 0xe0)
									*BITMAP_ADDR16(bitmap, l*9 + y, i*6 + x) = BIT(region()->u8(m_ddram[char_pos]*8+y), 4-x);
								else
									*BITMAP_ADDR16(bitmap, l*9 + y, i*6 + x) = BIT(region()->u8(0x700+((m_ddram[char_pos]-0xe0)*11)+y), 4-x);
							}
						}

				// if is the correct position draw cursor and blink
				if (char_pos == m_cursor_pos)
				{
					//draw the cursor
					if (m_cursor_on)
						for (int x=0; x<5; x++)
							*BITMAP_ADDR16(bitmap, l*9 + 7, i * 6 + x) = 1;

					if (!m_blink && m_blink_on)
						for (int y=0; y<7; y++)
							for (int x=0; x<5; x++)
								*BITMAP_ADDR16(bitmap, l*9 + y, i * 6 + x) = 1;
				}
			}

	return 0;

}

WRITE8_MEMBER(hd44780_device::control_write)
{
	if (BIT(data, 7)) // Set DDRAM Address
	{
		m_ac_mode = 0;
		m_ac = data & 0x7f;
		if (data != 0x81) // not in datasheet spec
			m_cursor_pos = m_ac;
		set_busy_flag(37);
	}
	else if (BIT(data, 6)) // Set CGRAM Address
	{
		m_ac_mode = 1;
		m_ac = data & 0x3f;
		set_busy_flag(37);
	}
	else if (BIT(data, 5)) // Function Set
	{
		// datasheet says you can't change char size after first function set without altering 4/8 bit mode
		if (BIT(data, 4) != m_data_len)
			m_char_size = BIT(data, 2);

		m_data_len = BIT(data, 4);
		m_num_line = BIT(data, 3);
		set_busy_flag(37);
	}
	else if (BIT(data, 4)) // Cursor or display shift
	{
		UINT8 direct = (BIT(data, 2)) ? +1 : -1;

		if (BIT(data, 3))
			m_disp_shift += direct;
		else
		{
			m_ac += direct;
			m_cursor_pos += direct;
		}

		set_busy_flag(37);
	}
	else if (BIT(data, 3)) // Display on/off Control
	{
		m_display_on = BIT(data, 2);
		m_cursor_on = BIT(data, 1);
		m_blink_on = BIT(data, 0);

		set_busy_flag(37);
	}
	else if (BIT(data, 2)) // Entry Mode set
	{
		m_direction = (BIT(data, 1)) ? +1 : -1;

		m_shift_on = BIT(data, 0);

		set_busy_flag(37);
	}
	else if (BIT(data, 1)) // return home
	{
		m_ac = 0;
		m_cursor_pos = 0;
		m_ac_mode = 0; // datasheet does not specifically say this but mephisto won't run without it
		m_direction = 1;
		m_disp_shift = 0;
		set_busy_flag(1520);
	}
	else if (BIT(data, 0)) // clear display
	{
		m_ac = 0;
		m_cursor_pos = 0;
		m_ac_mode = 0;
		m_direction = 1;
		m_disp_shift = 0;
		memset(m_ddram, 0x20, sizeof(m_ddram));
		set_busy_flag(1520);
	}
}

READ8_MEMBER(hd44780_device::control_read)
{
	return m_busy_flag<<7 || m_ac&0x7f;
}

void hd44780_device::update_ac(void) // m_data_bus_flag was left as global so old savestates will work
{
	int new_ac = m_ac + m_direction;
	m_ac = (new_ac < 0) ? 0 : ((new_ac > 0x7f) ? 0x7f : new_ac);
	if (m_ac_mode == 0)
	{
		m_cursor_pos = m_ac;
		// display is shifted only after a write
		if (m_shift_on && m_data_bus_flag == 1)	m_disp_shift += m_direction;
	}
	m_data_bus_flag = 0;
}


WRITE8_MEMBER(hd44780_device::data_write)
{
	if (m_busy_flag)
	{
		logerror("HD44780 '%s' Ignoring data write %02x due of busy flag\n", tag(), data);
		return;
	}

	if (m_ac_mode == 0)
		m_ddram[m_ac] = data;
	else
		m_cgram[m_ac] = data;

	m_data_bus_flag = 1;
	update_ac();
	set_busy_flag(41);
}

READ8_MEMBER(hd44780_device::data_read)
{
	UINT8 data;

	if (m_ac_mode == 0)
		data = m_ddram[m_ac];
	else
		data = m_cgram[m_ac];

	m_data_bus_flag = 2;
	update_ac();

	set_busy_flag(41);

	return data;
}

// devices
const device_type HD44780 = hd44780_device_config::static_alloc_device_config;
