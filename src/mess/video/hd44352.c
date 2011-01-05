/***************************************************************************

        Hitachi HD44352 LCD controller

***************************************************************************/

#include "emu.h"
#include "video/hd44352.h"

#define		LCD_BYTE_INPUT			0x01
#define 	LCD_BYTE_OUTPUT			0x02
#define 	LCD_CHAR_OUTPUT			0x03
#define 	LCD_ON_OFF				0x04
#define 	LCD_CURSOR_GRAPHIC		0x06
#define 	LCD_CURSOR_CHAR			0x07
#define 	LCD_SCROLL_CHAR_WIDTH	0x08
#define 	LCD_CURSOR_STATUS		0x09
#define 	LCD_USER_CHARACTER		0x0b
#define 	LCD_CONTRAST			0x0c
#define 	LCD_IRQ_FREQUENCY		0x0d
#define 	LCD_CURSOR_POSITION		0x0e

//**************************************************************************
//  device configuration
//**************************************************************************

//-------------------------------------------------
//  hd44352_device_config - constructor
//-------------------------------------------------

hd44352_device_config::hd44352_device_config( const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock ):
	device_config( mconfig, static_alloc_device_config, "hd44352", tag, owner, clock)
{
}

//-------------------------------------------------
//  static_alloc_device_config - allocate a new
//  configuration object
//-------------------------------------------------

device_config *hd44352_device_config::static_alloc_device_config( const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock )
{
	return global_alloc( hd44352_device_config( mconfig, tag, owner, clock ) );
}


//-------------------------------------------------
//  alloc_device - allocate a new device object
//-------------------------------------------------

device_t *hd44352_device_config::alloc_device( running_machine &machine ) const
{
	return auto_alloc( &machine, hd44352_device( machine, *this ) );
}

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void hd44352_device_config::device_config_complete()
{
	// inherit a copy of the static data
	const hd44352_interface *intf = reinterpret_cast<const hd44352_interface *>(static_config());
	if (intf != NULL)
		*static_cast<hd44352_interface *>(this) = *intf;

	// or initialize to defaults if none provided
	else
	{
		memset(&m_on, 0, sizeof(m_on));
	}
}

//-------------------------------------------------
//  device_validity_check - perform validity checks
//  on this device
//-------------------------------------------------

bool hd44352_device_config::device_validity_check( const game_driver &driver ) const
{
	bool error = false;

	return error;
}


//**************************************************************************
//  live device
//**************************************************************************

//-------------------------------------------------
//  hd44352_device - constructor
//-------------------------------------------------

hd44352_device::hd44352_device( running_machine &_machine, const hd44352_device_config &_config ) :
	device_t( _machine, _config ),
	m_config( _config )
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void hd44352_device::device_start()
{
	devcb_resolve_write_line(&m_on, &m_config.m_on, this);

	m_on_timer = device_timer_alloc(*this, ON_TIMER);
	timer_adjust_periodic(m_on_timer, ATTOTIME_IN_HZ(m_clock/16384), 0, ATTOTIME_IN_HZ(m_clock/16384));

	state_save_register_device_item( this, 0, control_lines);
	state_save_register_device_item( this, 0, data_bus);
	state_save_register_device_item( this, 0, state);
	state_save_register_device_item( this, 0, offset);
	state_save_register_device_item( this, 0, char_width);
	state_save_register_device_item( this, 0, bank);
	state_save_register_device_item( this, 0, lcd_on);
	state_save_register_device_item( this, 0, scroll);
	state_save_register_device_item( this, 0, contrast);
	state_save_register_device_item( this, 0, byte_count);
	state_save_register_device_item( this, 0, cursor_status);
	state_save_register_device_item( this, 0, cursor_x);
	state_save_register_device_item( this, 0, cursor_y);
	state_save_register_device_item( this, 0, cursor_lcd);
	state_save_register_device_item_array( this, 0, video_ram[0]);
	state_save_register_device_item_array( this, 0, video_ram[1]);
	state_save_register_device_item_array( this, 0, par);
	state_save_register_device_item_array( this, 0, cursor);
	state_save_register_device_item_array( this, 0, custom_char[0]);
	state_save_register_device_item_array( this, 0, custom_char[1]);
	state_save_register_device_item_array( this, 0, custom_char[2]);
	state_save_register_device_item_array( this, 0, custom_char[3]);
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void hd44352_device::device_reset()
{
	memset(video_ram, 0x00, 0x300);
	memset(par, 0x00, 3);
	memset(custom_char, 0x00, 4*8);
	memset(cursor, 0x00, 8);
	control_lines = 0;
	data_bus = 0xff;
	state = 0;
	bank = 0;
	offset = 0;
	char_width = 6;
	lcd_on = 0;
	scroll = 0;
	byte_count = 0;
	cursor_status = 0;
	cursor_x = 0;
	cursor_y = 0;
	cursor_lcd = 0;
	contrast = 0;
}


//-------------------------------------------------
//  device_timer - handler timer events
//-------------------------------------------------
void hd44352_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	switch(id)
	{
		case ON_TIMER:
			if (control_lines & 0x40)
			{
				devcb_call_write_line(&m_on, ASSERT_LINE);
				devcb_call_write_line(&m_on, CLEAR_LINE);
			}
			break;
	}
}

//**************************************************************************
//  device interface
//**************************************************************************

int hd44352_device::video_update(bitmap_t &bitmap, const rectangle &cliprect)
{
	UINT8 cw = char_width;

	bitmap_fill(&bitmap, &cliprect, 0);

	if (control_lines&0x80 && lcd_on)
	{
		for (int a=0; a<2; a++)
			for (int py=0; py<4; py++)
				for (int px=0; px<16; px++)
					if (BIT(cursor_status, 4) && px == cursor_x && py == cursor_y && a == cursor_lcd)
					{
						//draw the cursor
						for (int c=0; c<cw; c++)
						{
							UINT8 d = compute_newval((cursor_status>>5) & 0x07, video_ram[a][py*16*cw + px*cw + c + scroll * 48], cursor[c]);
							for (int b=0; b<8; b++)
							{
								*BITMAP_ADDR16(&bitmap, py*8 + b, a*cw*16 + px*cw + c) = BIT(d, 7-b);
							}
						}
					}
					else
					{
						for (int c=0; c<cw; c++)
						{
							UINT8 d = video_ram[a][py*16*cw + px*cw + c + scroll * 48];
							for (int b=0; b<8; b++)
							{
								*BITMAP_ADDR16(&bitmap, py*8 + b, a*cw*16 + px*cw + c) = BIT(d, 7-b);
							}
						}
					}
	}

	return 0;
}


void hd44352_device::control_write(UINT8 data)
{
	if(control_lines == data)
		state = 0;

	control_lines = data;
}

UINT8 hd44352_device::compute_newval(UINT8 type, UINT8 oldval, UINT8 newval)
{
	switch(type & 0x07)
	{
		case 0x00:
			return (~oldval) & newval;
		case 0x01:
			return oldval ^ newval;
		case 0x03:
			return oldval & (~newval);
		case 0x04:
			return newval;
		case 0x05:
			return oldval | newval;
		case 0x07:
			return oldval;
		case 0x02:
		case 0x06:
		default:
			return 0;
	}
}

UINT8 hd44352_device::get_char(UINT16 pos)
{
	switch ((UINT8)pos/8)
	{
		case 0xcf:
			return custom_char[0][pos%8];
		case 0xdf:
			return custom_char[1][pos%8];
		case 0xef:
			return custom_char[2][pos%8];
		case 0xff:
			return custom_char[3][pos%8];
		default:
			return region()->u8(pos);
	}
}

void hd44352_device::data_write(UINT8 data)
{
	// verify that controller is active
	if (!(control_lines&0x80))
		return;

	if (control_lines & 0x01)
	{
		if (!(control_lines&0x02) && !(control_lines&0x04))
			return;

		switch (state)
		{
			case 0:		//parameter 0
				par[state++] = data;
				break;
			case 1:		//parameter 1
				par[state++] = data;
				break;
			case 2:		//parameter 2
				par[state++] = data;
				break;
		}

		switch (par[0] & 0x0f)
		{
			case LCD_BYTE_INPUT:
			case LCD_CHAR_OUTPUT:
				{
					if (state == 1)
						bank = BIT(data, 4);
					else if (state == 2)
						offset = ((data>>1)&0x3f) % 48 + (BIT(data,7) * 48);
					else if (state == 3)
						offset += ((data & 0x03) * 96);
				}
				break;
			case LCD_BYTE_OUTPUT:
				{
					if (state == 1)
						bank = BIT(data, 4);
					else if (state == 2)
						offset = ((data>>1)&0x3f) % 48 + (BIT(data,7) * 48);
					else if (state == 3)
						offset += ((data & 0x03) * 96);
				}
				break;
			case LCD_ON_OFF:
				{
					if (state == 1)
						lcd_on = BIT(data, 4);
					data_bus = 0xff;
					state = 0;
				}
				break;
			case LCD_SCROLL_CHAR_WIDTH:
				{
					if (state == 1)
					{
						char_width = 8-((data>>4)&3);
						scroll = ((data>>6)&3);
					}

					data_bus = 0xff;
					state = 0;
				}
				break;
			case LCD_CURSOR_STATUS:
				{
					if (state == 1)
						cursor_status = data;
					data_bus = 0xff;
					state = 0;
				}
				break;
			case LCD_CONTRAST:
				{
					if (state == 1)
						contrast = (contrast & 0x00ffff) | (data<<16);
					else if (state == 2)
						contrast = (contrast & 0xff00ff) | (data<<8);
					else if (state == 3)
					{
						contrast = (contrast & 0xffff00) | (data<<0);
						state = 0;
					}

					data_bus = 0xff;
				}
				break;
			case LCD_IRQ_FREQUENCY:
				{
					if (state == 1)
					{
						UINT32 on_timer_rate;

						switch((data>>4) & 0x0f)
						{
							case 0x00:		on_timer_rate = 16384;		break;
							case 0x01:		on_timer_rate = 8;			break;
							case 0x02:		on_timer_rate = 16;			break;
							case 0x03:		on_timer_rate = 32;			break;
							case 0x04:		on_timer_rate = 64;			break;
							case 0x05:		on_timer_rate = 128;		break;
							case 0x06:		on_timer_rate = 256;		break;
							case 0x07:		on_timer_rate = 512;		break;
							case 0x08:		on_timer_rate = 1024;		break;
							case 0x09:		on_timer_rate = 2048;		break;
							case 0x0a:		on_timer_rate = 4096;		break;
							case 0x0b:		on_timer_rate = 4096;		break;
							default:		on_timer_rate = 8192;		break;
						}

						timer_adjust_periodic(m_on_timer, ATTOTIME_IN_HZ(m_clock/on_timer_rate), 0, ATTOTIME_IN_HZ(m_clock/on_timer_rate));
					}
					data_bus = 0xff;
					state = 0;
				}
				break;
			case LCD_CURSOR_POSITION:
				{
					if (state == 1)
						cursor_lcd = BIT(data, 4);	//0:left lcd 1:right lcd;
					else if (state == 2)
						cursor_x = ((data>>1)&0x3f) % 48 + (BIT(data,7) * 48);
					else if (state == 3)
					{
						cursor_y = data & 0x03;
						state = 0;
					}

					data_bus = 0xff;
				}
				break;
		}

		byte_count = 0;
		data_bus = 0xff;
	}
	else
	{
		switch (par[0] & 0x0f)
		{
			case LCD_BYTE_INPUT:
				{
					if (((par[0]>>5) & 0x07) != 0x03)
						break;

					offset %= 0x180;
					data_bus = ((video_ram[bank][offset]<<4)&0xf0) | ((video_ram[bank][offset]>>4)&0x0f);
					offset++; byte_count++;
				}
				break;
			case LCD_BYTE_OUTPUT:
				{
					offset %= 0x180;
					video_ram[bank][offset] = compute_newval((par[0]>>5) & 0x07, video_ram[bank][offset], data);
					offset++; byte_count++;

					data_bus = 0xff;
				}
				break;
			case LCD_CHAR_OUTPUT:
				{
					int char_pos = data*8;

					for (int i=0; i<char_width; i++)
					{
						offset %= 0x180;
						video_ram[bank][offset] = compute_newval((par[0]>>5) & 0x07, video_ram[bank][offset], get_char(char_pos));
						offset++; char_pos++;
					}

					byte_count++;
					data_bus = 0xff;
				}
				break;
			case LCD_CURSOR_GRAPHIC:
				if (byte_count<8)
				{
					cursor[byte_count] = data;
					byte_count++;
					data_bus = 0xff;
				}
				break;
			case LCD_CURSOR_CHAR:
				if (byte_count<1)
				{
					UINT8 char_code = ((data<<4)&0xf0) | ((data>>4)&0x0f);

					for (int i=0; i<8; i++)
						cursor[i] = get_char(char_code*8 + i);

					byte_count++;
					data_bus = 0xff;
				}
				break;
			case LCD_USER_CHARACTER:
				if (byte_count<8)
				{
					custom_char[(par[0]&0x03)][byte_count] = data;
					byte_count++;
					data_bus = 0xff;
				}
				break;
			default:
				data_bus = 0xff;
		}

		state=0;
	}
}

UINT8 hd44352_device::data_read()
{
	return data_bus;
}

// devices
const device_type HD44352 = hd44352_device_config::static_alloc_device_config;

