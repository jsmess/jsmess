/***************************************************************************
  TI-85 driver by Krzysztof Strzecha

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/ti85.h"
#include "formats/ti85_ser.h"
#include "sound/speaker.h"

#define TI85_SNAPSHOT_SIZE	 32976
#define TI86_SNAPSHOT_SIZE	131284

enum
{
	TI_NOT_INITIALIZED,
	TI_81,
	TI_82,
	TI_83,
	TI_83p,
	TI_85,
	TI_86
};

static UINT8 ti_calculator_model = TI_NOT_INITIALIZED;

UINT8 ti85_timer_interrupt_mask;
static UINT8 ti85_timer_interrupt_status;

static UINT8 ti85_ON_interrupt_mask;
static UINT8 ti85_ON_interrupt_status;
static UINT8 ti85_ON_pressed;

static UINT8 ti8x_memory_page_1;
static UINT8 ti8x_memory_page_2;

UINT8 ti85_LCD_memory_base;
UINT8 ti85_LCD_contrast;
UINT8 ti85_LCD_status;
static UINT8 ti85_LCD_mask;

static UINT8 ti85_power_mode;
static UINT8 ti85_keypad_mask;

static UINT8 ti85_video_buffer_width;
static UINT8 ti85_interrupt_speed;
static UINT8 ti85_port4_bit0;

static UINT8 ti81_port_7_data;

static UINT8 *ti8x_ram = NULL;

static ti85_serial_data ti85_serial_stream;

static UINT8 ti85_PCR = 0xc0;

static UINT8 ti85_red_in = 0x01;
static UINT8 ti85_white_in = 0x01;

static UINT8 ti85_red_out = 0x00;
static UINT8 ti85_white_out = 0x00;

static UINT8 ti82_video_mode;
static UINT8 ti82_video_x;
static UINT8 ti82_video_y;
static UINT8 ti82_video_dir;
static UINT8 ti82_video_scroll;
static UINT8 ti82_video_bit;
static UINT8 ti82_video_col;
static UINT8 ti8x_port2;
static UINT8 ti83p_port4;
UINT8 ti82_video_buffer[0x300];

enum
{
	TI85_SEND_STOP,
	TI85_SEND_HEADER,
	TI85_RECEIVE_OK_1,
	TI85_RECEIVE_ANSWER_1,
	TI85_RECEIVE_ANSWER_2,
	TI85_RECEIVE_ANSWER_3,
	TI85_SEND_OK,
	TI85_SEND_DATA,
	TI85_RECEIVE_OK_2,
	TI85_SEND_END,
	TI85_RECEIVE_OK_3,
	TI85_RECEIVE_HEADER_1,
	TI85_PREPARE_VARIABLE_DATA,
	TI85_RECEIVE_HEADER_2,
	TI85_SEND_OK_1,
	TI85_SEND_CONTINUE,
	TI85_RECEIVE_OK,
	TI85_RECEIVE_DATA,
	TI85_SEND_OK_2,
	TI85_RECEIVE_END_OR_HEADER_1,
	TI85_SEND_OK_3,
	TI85_PREPARE_SCREEN_REQUEST,
	TI85_SEND_SCREEN_REQUEST,
	TI85_SEND_RESET
};

static UINT8 ti85_serial_status = TI85_SEND_STOP;

static UINT8 * ti85_receive_buffer;
static UINT8 * ti85_receive_data;

static void ti85_free_serial_data_memory (running_machine *machine);

static int ti85_receive_serial (UINT8*, UINT32);
static int ti85_send_serial (UINT8*, UINT32);

static void ti85_reset_serial (running_machine *machine);
static void ti85_update_serial (running_machine *machine);
static void ti85_send_variables (running_machine *machine);
static void ti85_send_backup (running_machine *machine);
static void ti85_receive_variables (running_machine *machine);
static void ti85_receive_backup (running_machine *machine);
static void ti85_receive_screen (running_machine *machine);

static TIMER_CALLBACK(ti85_timer_callback)
{
	if (input_port_read(machine, "ON") & 0x01)
	{
		if (ti85_ON_interrupt_mask && !ti85_ON_pressed)
		{
			cputag_set_input_line(machine, "maincpu", 0, HOLD_LINE);
			ti85_ON_interrupt_status = 1;
			if (!ti85_timer_interrupt_mask) ti85_timer_interrupt_mask = 1;
		}
		ti85_ON_pressed = 1;
		return;
	}
	else
		ti85_ON_pressed = 0;
	if (ti85_timer_interrupt_mask)
	{
		cputag_set_input_line(machine, "maincpu", 0, HOLD_LINE);
		ti85_timer_interrupt_status = 1;
	}
}

static void update_ti85_memory (running_machine *machine)
{
	memory_set_bankptr(machine, 2,memory_region(machine, "maincpu") + 0x010000 + 0x004000*ti8x_memory_page_1);
}

static void update_ti83p_memory (running_machine *machine)
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	if (ti8x_memory_page_1 & 0x40)
	{
		if (ti83p_port4 & 1)
		{

			memory_set_bankptr(machine, 3, ti8x_ram + 0x004000*(ti8x_memory_page_1&0x01));
			memory_install_write8_handler(space, 0x8000, 0xbfff, 0, 0, SMH_BANK(3));
		}
		else
		{
			memory_set_bankptr(machine, 2, ti8x_ram + 0x004000*(ti8x_memory_page_1&0x01));
			memory_install_write8_handler(space, 0x4000, 0x7fff, 0, 0, SMH_BANK(2));
		}
	}
	else
	{
		if (ti83p_port4 & 1)
		{
			memory_set_bankptr(machine, 3, memory_region(machine, "maincpu") + 0x010000 + 0x004000*(ti8x_memory_page_1&0x1f));
			memory_install_write8_handler(space, 0x8000, 0xbfff, 0, 0, SMH_UNMAP);
		}
		else
		{
			memory_set_bankptr(machine, 2, memory_region(machine, "maincpu") + 0x010000 + 0x004000*(ti8x_memory_page_1&0x1f));
			memory_install_write8_handler(space, 0x4000, 0x7fff, 0, 0, SMH_UNMAP);
		}
	}

	if (ti8x_memory_page_2 & 0x40)
	{
		if (ti83p_port4 & 1)
		{
			memory_set_bankptr(machine, 4, ti8x_ram + 0x004000*(ti8x_memory_page_2&0x01));
			memory_install_write8_handler(space, 0xc000, 0xffff, 0, 0, SMH_BANK(4));
		}
		else
		{
			memory_set_bankptr(machine, 3, ti8x_ram + 0x004000*(ti8x_memory_page_2&0x01));
			memory_install_write8_handler(space, 0x8000, 0xbfff, 0, 0, SMH_BANK(3));
		}

	}
	else
	{
		if (ti83p_port4 & 1)
		{
			memory_set_bankptr(machine, 4, memory_region(machine, "maincpu") + 0x010000 + 0x004000*(ti8x_memory_page_2&0x1f));
			memory_install_write8_handler(space, 0xc000, 0xffff, 0, 0, SMH_UNMAP);
		}
		else
		{
			memory_set_bankptr(machine, 3, memory_region(machine, "maincpu") + 0x010000 + 0x004000*(ti8x_memory_page_2&0x1f));
			memory_install_write8_handler(space, 0x8000, 0xbfff, 0, 0, SMH_UNMAP);
		}
	}
}

static void update_ti86_memory (running_machine *machine)
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	write8_space_func wh;

	if (ti8x_memory_page_1 & 0x40)
	{
		memory_set_bankptr(machine, 2, ti8x_ram + 0x004000*(ti8x_memory_page_1&0x07));
		wh = SMH_BANK(2);
	}
	else
	{
		memory_set_bankptr(machine, 2, memory_region(machine, "maincpu") + 0x010000 + 0x004000*(ti8x_memory_page_1&0x0f));
		wh = SMH_UNMAP;
	}
	memory_install_write8_handler(space, 0x4000, 0x7fff, 0, 0, wh);

	if (ti8x_memory_page_2 & 0x40)
	{
		memory_set_bankptr(machine, 3, ti8x_ram + 0x004000*(ti8x_memory_page_2&0x07));
		wh = SMH_BANK(3);
	}
	else
	{
		memory_set_bankptr(machine, 3, memory_region(machine, "maincpu") + 0x010000 + 0x004000*(ti8x_memory_page_2&0x0f));
		wh = SMH_UNMAP;
	}
	memory_install_write8_handler(space, 0x8000, 0xbfff, 0, 0, wh);
}

/***************************************************************************
  Machine Initialization
***************************************************************************/

MACHINE_START( ti81 )
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT8 *mem = memory_region(machine, "maincpu");

	ti85_timer_interrupt_mask = 0;
	ti85_timer_interrupt_status = 0;
	ti85_ON_interrupt_mask = 0;
	ti85_ON_interrupt_status = 0;
	ti85_ON_pressed = 0;
	ti85_power_mode = 0;
	ti85_keypad_mask = 0;
	ti8x_memory_page_1 = 0;
	ti85_LCD_memory_base = 0;
	ti85_LCD_status = 0;
	ti85_LCD_mask = 0;
	ti85_video_buffer_width = 0;
	ti85_interrupt_speed = 0;
	ti85_port4_bit0 = 0;
	ti81_port_7_data = 0;

	if (ti_calculator_model == TI_81)
		memset(mem+0x8000, 0, sizeof(unsigned char)*0x8000);

	ti_calculator_model = TI_81;

	timer_pulse(machine, ATTOTIME_IN_HZ(200), NULL, 0, ti85_timer_callback);

	memory_install_write8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_UNMAP);
	memory_install_write8_handler(space, 0x4000, 0x7fff, 0, 0, SMH_UNMAP);
	memory_set_bankptr(machine, 1, mem + 0x010000);
	memory_set_bankptr(machine, 2, mem + 0x014000);
}

MACHINE_START( ti82 )
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT8 *mem = memory_region(machine, "maincpu");

	ti85_timer_interrupt_mask = 0;
	ti85_timer_interrupt_status = 0;
	ti85_ON_interrupt_mask = 0;
	ti85_ON_interrupt_status = 0;
	ti85_ON_pressed = 0;
	ti8x_memory_page_1 = 0;
	ti85_LCD_status = 0;
	ti85_LCD_mask = 0;
	ti85_power_mode = 0;
	ti85_keypad_mask = 0;
	ti85_video_buffer_width = 0;
	ti85_interrupt_speed = 0;
	ti85_port4_bit0 = 0;
	ti82_video_mode = 0;
	ti82_video_x = 0;
	ti82_video_y = 0;
	ti82_video_dir = 0;
	ti82_video_scroll = 0;
	ti82_video_bit = 0;
	ti82_video_col = 0;
	memset(ti82_video_buffer, 0x00, 0x300);

	if (ti_calculator_model == TI_82)
		memset(mem+0x8000, 0, sizeof(unsigned char)*0x8000);

	ti_calculator_model = TI_82;

	timer_pulse(machine, ATTOTIME_IN_HZ(200), NULL, 0, ti85_timer_callback);

	memory_install_write8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_UNMAP);
	memory_install_write8_handler(space, 0x4000, 0x7fff, 0, 0, SMH_UNMAP);
	memory_set_bankptr(machine, 1, mem + 0x010000);
	memory_set_bankptr(machine, 2, mem + 0x014000);

	add_reset_callback(machine, ti85_reset_serial);
	add_exit_callback(machine, ti85_free_serial_data_memory);
}

MACHINE_START( ti85 )
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT8 *mem = memory_region(machine, "maincpu");

	ti85_timer_interrupt_mask = 0;
	ti85_timer_interrupt_status = 0;
	ti85_ON_interrupt_mask = 0;
	ti85_ON_interrupt_status = 0;
	ti85_ON_pressed = 0;
	ti8x_memory_page_1 = 0;
	ti85_LCD_memory_base = 0;
	ti85_LCD_status = 0;
	ti85_LCD_mask = 0;
	ti85_power_mode = 0;
	ti85_keypad_mask = 0;
	ti85_video_buffer_width = 0;
	ti85_interrupt_speed = 0;
	ti85_port4_bit0 = 0;

	if (ti_calculator_model == TI_85)
		memset(mem+0x8000, 0, sizeof(unsigned char)*0x8000);

	ti_calculator_model = TI_85;

	timer_pulse(machine, ATTOTIME_IN_HZ(200), NULL, 0, ti85_timer_callback);

	memory_install_write8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_UNMAP);
	memory_install_write8_handler(space, 0x4000, 0x7fff, 0, 0, SMH_UNMAP);
	memory_set_bankptr(machine, 1, mem + 0x010000);
	memory_set_bankptr(machine, 2, mem + 0x014000);

	add_reset_callback(machine, ti85_reset_serial);
	add_exit_callback(machine, ti85_free_serial_data_memory);
}

MACHINE_START( ti83p )
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT8 *mem = memory_region(machine, "maincpu");

	ti85_timer_interrupt_mask = 0;
	ti85_timer_interrupt_status = 0;
	ti85_ON_interrupt_mask = 0;
	ti85_ON_interrupt_status = 0;
	ti85_ON_pressed = 0;
	ti8x_memory_page_1 = 0;
	ti8x_memory_page_2 = 0;
	ti85_LCD_memory_base = 0;
	ti85_LCD_status = 0;
	ti85_LCD_mask = 0;
	ti85_power_mode = 0;
	ti85_keypad_mask = 0;
	ti85_video_buffer_width = 0;
	ti85_interrupt_speed = 0;
	ti85_port4_bit0 = 0;
	ti82_video_mode = 0;
	ti82_video_x = 0;
	ti82_video_y = 0;
	ti82_video_dir = 0;
	ti82_video_scroll = 0;
	ti82_video_bit = 0;
	ti82_video_col = 0;
	memset(ti82_video_buffer, 0x00, 0x300);

	ti8x_ram = auto_alloc_array(machine, UINT8, 32*1024);
	{
		memory_install_write8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_UNMAP);
		memory_install_write8_handler(space, 0x4000, 0x7fff, 0, 0, SMH_UNMAP);
		memory_install_write8_handler(space, 0x8000, 0xbfff, 0, 0, SMH_UNMAP);

		memory_set_bankptr(machine, 1, mem + 0x010000);
		memory_set_bankptr(machine, 2, mem + 0x010000);
		memory_set_bankptr(machine, 3, mem + 0x010000);
		memory_set_bankptr(machine, 4, ti8x_ram);

		if (ti_calculator_model == TI_83p)
			memset(ti8x_ram, 0, sizeof(unsigned char)*32*1024);

//      memset(mem + 0x83000, 0xff, 0x1000);

		ti_calculator_model = TI_83p;

		timer_pulse(machine, ATTOTIME_IN_HZ(200), NULL, 0, ti85_timer_callback);
	}

	add_reset_callback(machine, ti85_reset_serial);
	add_exit_callback(machine, ti85_free_serial_data_memory);
}


MACHINE_START( ti86 )
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT8 *mem = memory_region(machine, "maincpu");

	ti85_timer_interrupt_mask = 0;
	ti85_timer_interrupt_status = 0;
	ti85_ON_interrupt_mask = 0;
	ti85_ON_interrupt_status = 0;
	ti85_ON_pressed = 0;
	ti8x_memory_page_1 = 0;
	ti8x_memory_page_2 = 0;
	ti85_LCD_memory_base = 0;
	ti85_LCD_status = 0;
	ti85_LCD_mask = 0;
	ti85_power_mode = 0;
	ti85_keypad_mask = 0;
	ti85_video_buffer_width = 0;
	ti85_interrupt_speed = 0;
	ti85_port4_bit0 = 0;

	ti8x_ram = auto_alloc_array(machine, UINT8, 128*1024);
	{
		memory_install_write8_handler(space, 0x0000, 0x3fff, 0, 0, SMH_UNMAP);

		memory_set_bankptr(machine, 1, mem + 0x010000);
		memory_set_bankptr(machine, 2, mem + 0x014000);

		memory_set_bankptr(machine, 4, ti8x_ram);

		if (ti_calculator_model == TI_86)
			memset(ti8x_ram, 0, sizeof(unsigned char)*128*1024);

		ti_calculator_model = TI_86;

		timer_pulse(machine, ATTOTIME_IN_HZ(200), NULL, 0, ti85_timer_callback);
	}

	add_reset_callback(machine, ti85_reset_serial);
	add_exit_callback(machine, ti85_free_serial_data_memory);
}

/* I/O ports handlers */

READ8_HANDLER ( ti85_port_0000_r )
{
	return 0xff;
}

READ8_HANDLER ( ti8x_keypad_r )
{
	int data = 0xff;
	int port;
	int bit;
	static const char *const bitnames[] = { "BIT0", "BIT1", "BIT2", "BIT3", "BIT4", "BIT5", "BIT6", "BIT7" };

	if (ti85_keypad_mask == 0x7f) return data;

	for (bit = 0; bit < 7; bit++)
	{
		if (~ti85_keypad_mask&(0x01<<bit))
		{
			for (port = 0; port < 8; port++)
			{
				data ^= input_port_read(space->machine, bitnames[port]) & (0x01<<bit) ? 0x01<<port : 0x00;
			}
		}
	}
	return data;
}

 READ8_HANDLER ( ti85_port_0002_r )
{
	return 0xff;
}

 READ8_HANDLER ( ti85_port_0003_r )
{
	int data = 0;

	if (ti85_LCD_status)
		data |= ti85_LCD_mask;
	if (ti85_ON_interrupt_status)
		data |= 0x01;
	if (ti85_timer_interrupt_status)
		data |= 0x04;
	if (!ti85_ON_pressed)
		data |= 0x08;
	ti85_ON_interrupt_status = 0;
	ti85_timer_interrupt_status = 0;
	return data;
}

 READ8_HANDLER ( ti85_port_0004_r )
{
	return 0xff;
}

 READ8_HANDLER ( ti85_port_0005_r )
{
	return ti8x_memory_page_1;
}

READ8_HANDLER ( ti85_port_0006_r )
{
	return ti85_power_mode;
}

READ8_HANDLER ( ti85_port_0007_r )
{
	ti85_update_serial(space->machine);
	return (ti85_white_out<<3)
		| (ti85_red_out<<2)
		| ((ti85_white_in&(1-ti85_white_out))<<1)
		| (ti85_red_in&(1-ti85_red_out))
		| ti85_PCR;
}

READ8_HANDLER ( ti82_port_0000_r )
{
	return 0xff; // TODO
}

 READ8_HANDLER ( ti82_port_0002_r )
{
	return ti8x_port2;
}

 READ8_HANDLER ( ti82_port_0010_r )
{
	return 0x00;
}

 READ8_HANDLER ( ti82_port_0011_r )
{
    UINT8* ti82_video;
	UINT8 ti82_port11;

	if(ti82_video_dir == 4)
		++ti82_video_y;
	if(ti82_video_dir == 5)
		--ti82_video_y;
	if(ti82_video_dir == 6)
		++ti82_video_x;
	if(ti82_video_dir == 7)
		--ti82_video_x;

	if(ti82_video_mode)
		ti82_port11 = ti82_video_buffer[(12*ti82_video_y)+ti82_video_x];
	else
	{
		ti82_video_col = ti82_video_x * 6;
		ti82_video_bit = ti82_video_col & 7;
		ti82_video = &ti82_video_buffer[(ti82_video_y*12)+(ti82_video_col>>3)];
		ti82_port11 = ((((*ti82_video)<<8)+ti82_video[1])>>(10-ti82_video_bit));
	}

	if(ti82_video_dir == 4)
		ti82_video_y-=2;
	if(ti82_video_dir == 5)
		ti82_video_y+=2;
	if(ti82_video_dir == 6)
		ti82_video_x-=2;
	if(ti82_video_dir == 7)
		ti82_video_x+=2;

	ti82_video_x &= 15;
	ti82_video_y &= 63;

	return ti82_port11;
}

READ8_HANDLER ( ti86_port_0005_r )
{
	return ti8x_memory_page_1;
}

 READ8_HANDLER ( ti86_port_0006_r )
{
	return ti8x_memory_page_2;
}

READ8_HANDLER ( ti83_port_0000_r )
{
	return ((ti8x_memory_page_1 & 0x08) << 1) | 0x0C;
}

 READ8_HANDLER ( ti83_port_0002_r )
{
	return ti8x_port2;
}

 READ8_HANDLER ( ti83_port_0003_r )
{
	int data = 0;

	if (ti85_LCD_status)
		data |= ti85_LCD_mask;
	if (ti85_ON_interrupt_status)
		data |= 0x01;
	if (!ti85_ON_pressed)
		data |= 0x08;
	ti85_ON_interrupt_status = 0;
	ti85_timer_interrupt_status = 0;
	return data;
}

 READ8_HANDLER ( ti83p_port_0002_r )
{
	return ti8x_port2|3;
}

WRITE8_HANDLER ( ti81_port_0007_w )
{
	ti81_port_7_data = data;
}

WRITE8_HANDLER ( ti85_port_0000_w )
{
	ti85_LCD_memory_base = data;
}

WRITE8_HANDLER ( ti8x_keypad_w )
{
	ti85_keypad_mask = data&0x7f;
}

WRITE8_HANDLER ( ti85_port_0002_w )
{
	ti85_LCD_contrast = data&0x1f;
}

WRITE8_HANDLER ( ti85_port_0003_w )
{
	if (ti85_LCD_status && !(data&0x08))	ti85_timer_interrupt_mask = 0;
	ti85_ON_interrupt_mask = data&0x01;
//  ti85_timer_interrupt_mask = data&0x04;
	ti85_LCD_mask = data&0x02;
	ti85_LCD_status = data&0x08;
}

WRITE8_HANDLER ( ti85_port_0004_w )
{
	ti85_video_buffer_width = (data>>3)&0x03;
	ti85_interrupt_speed = (data>>1)&0x03;
	ti85_port4_bit0 = data&0x01;
}

WRITE8_HANDLER ( ti85_port_0005_w )
{
	ti8x_memory_page_1 = data;
	update_ti85_memory(space->machine);
}

WRITE8_HANDLER ( ti85_port_0006_w )
{
	ti85_power_mode = data;
}

WRITE8_HANDLER ( ti85_port_0007_w )
{
	const device_config *speaker = devtag_get_device(space->machine, "speaker");
	speaker_level_w(speaker,( (data>>2)|(data>>3) )&0x01 );
        ti85_red_out=(data>>2)&0x01;
        ti85_white_out=(data>>3)&0x01;
	ti85_update_serial(space->machine);
	ti85_PCR = data&0xf0;
}

WRITE8_HANDLER ( ti86_port_0005_w )
{
	ti8x_memory_page_1 = data&((data&0x40)?0x47:0x4f);
	update_ti86_memory(space->machine);
}

WRITE8_HANDLER ( ti86_port_0006_w )
{
	ti8x_memory_page_2 = data&((data&0x40)?0x47:0x4f);
	update_ti86_memory(space->machine);
}

WRITE8_HANDLER ( ti82_port_0000_w )
{
	// TODO
}

WRITE8_HANDLER ( ti82_port_0002_w )
{
	ti8x_memory_page_1 = (data & 0x07);
	update_ti85_memory(space->machine);
	ti8x_port2 = data;
}

WRITE8_HANDLER ( ti82_port_0010_w)
 {
	if (data == 0x00 || data == 0x01)
		ti82_video_mode = data;
	if (data >= 0x04 && data <= 0x07)
		ti82_video_dir = data;
	if (data >= 0x20 && data <= 0x30)
		ti82_video_x = data - 0x20;
	if (data >= 0x40 && data <= 0x7F)
		ti82_video_scroll = data - 0x40;
	if (data >= 0x80 && data <= 0xBF)
		ti82_video_y = data - 0x80;
	if (data >= 0xD8)
		ti85_LCD_contrast = data - 0xD8;
}

WRITE8_HANDLER ( ti82_port_0011_w )
{
	UINT8* ti82_video;

	if(ti82_video_mode)
		ti82_video_buffer[(12*ti82_video_y)+ti82_video_x] = data;
	else
	{
		data = data<<0x02;
		ti82_video_col = ti82_video_x * 6;
		ti82_video_bit = ti82_video_col & 0x07;
		ti82_video = &ti82_video_buffer[(ti82_video_y*12)+(ti82_video_col>>3)];
		*ti82_video = (*ti82_video & ~(0xFC>>ti82_video_bit)) | (data>>ti82_video_bit);
		if(ti82_video_bit>0x02)
			ti82_video[1] = (ti82_video[1] & ~(0xFC<<(8-ti82_video_bit))) | (data<<(8-ti82_video_bit));
	}

	if(ti82_video_dir == 0x04)
		--ti82_video_y;
	if(ti82_video_dir == 0x05)
		++ti82_video_y;
	if(ti82_video_dir == 0x06)
		--ti82_video_x;
	if(ti82_video_dir == 0x07)
		++ti82_video_x;

	ti82_video_x &= 15;
	ti82_video_y &= 63;
}

WRITE8_HANDLER ( ti83_port_0000_w )
{
	ti8x_memory_page_1 = (ti8x_memory_page_1 & 7) | ((data & 16) >> 1);
	update_ti85_memory(space->machine);
}

WRITE8_HANDLER ( ti83_port_0002_w )
{
	ti8x_memory_page_1 = (ti8x_memory_page_1 & 8) | (data & 7);
	update_ti85_memory(space->machine);
	ti8x_port2 = data;
}

WRITE8_HANDLER ( ti83_port_0003_w )
{
		if (ti85_LCD_status && !(data&0x08))	ti85_timer_interrupt_mask = 0;
		ti85_ON_interrupt_mask = data&0x01;
		//ti85_timer_interrupt_mask = data&0x04;
		ti85_LCD_mask = data&0x02;
		ti85_LCD_status = data&0x08;
}

WRITE8_HANDLER ( ti83p_port_0002_w )
{
	ti8x_port2 = data;
}

WRITE8_HANDLER ( ti83p_port_0003_w )
{
	ti85_LCD_mask = (data&0x08) >> 2;
	ti85_ON_interrupt_mask = data & 0x01;
}

WRITE8_HANDLER ( ti83p_port_0004_w )
{
	if ((data & 1) && !(ti83p_port4 & 1))
	{
		ti8x_memory_page_1 = 0x1f;
		ti8x_memory_page_2 = 0x1f;
	}
	else if (!(data & 1) && (ti83p_port4 & 1))
	{
		ti8x_memory_page_1 = 0x1f;
		ti8x_memory_page_2 = 0x40;
	}
	update_ti83p_memory(space->machine);
	ti83p_port4 = data;
}

WRITE8_HANDLER ( ti83p_port_0006_w )
{
	ti8x_memory_page_1 = data & ((data&0x40) ? 0x41 : 0x5f);
	update_ti83p_memory(space->machine);
}

WRITE8_HANDLER ( ti83p_port_0007_w )
{
	ti8x_memory_page_2 = data & ((data&0x40) ? 0x41 : 0x5f);
	update_ti83p_memory(space->machine);
}

WRITE8_HANDLER ( ti83p_port_0010_w)
 {
	if (data == 0x00 || data == 0x01)
		ti82_video_mode = data;
	if (data == 0x02 || data == 0x03)
		ti85_LCD_status = data - 0x02;
	if (data >= 0x04 && data <= 0x07)
		ti82_video_dir = data;
	if (data >= 0x20 && data <= 0x30)
		ti82_video_x = data - 0x20;
	if (data >= 0x40 && data <= 0x7F)
		ti82_video_scroll = data - 0x40;
	if (data >= 0x80 && data <= 0xBF)
		ti82_video_y = data - 0x80;
	if (data >= 0xc8)
		ti85_LCD_contrast = data - 0xc8;

}

/* NVRAM functions */
NVRAM_HANDLER( ti83p )
{
	if (read_or_write)
	{
		mame_fwrite(file, ti8x_ram, sizeof(unsigned char)*32*1024);
	}
	else
	{
			if (file)
			{
				mame_fread(file, ti8x_ram, sizeof(unsigned char)*32*1024);
				cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_PC,0x0c59);
			}
			else
				memset(ti8x_ram, 0, sizeof(unsigned char)*32*1024);
	}
}

NVRAM_HANDLER( ti86 )
{
	if (read_or_write)
	{
		mame_fwrite(file, ti8x_ram, sizeof(unsigned char)*128*1024);
	}
	else
	{
			if (file)
			{
				mame_fread(file, ti8x_ram, sizeof(unsigned char)*128*1024);
				cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_PC,0x0c59);
			}
			else
				memset(ti8x_ram, 0, sizeof(unsigned char)*128*1024);
	}
}

/***************************************************************************
  TI calculators snapshot files (SAV)
***************************************************************************/

static void ti8x_snapshot_setup_registers (running_machine *machine, UINT8 * data)
{
	unsigned char lo,hi;
	unsigned char * reg = data + 0x40;

	/* Set registers */
	lo = reg[0x00] & 0x0ff;
	hi = reg[0x01] & 0x0ff;
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_AF, (hi << 8) | lo);
	lo = reg[0x04] & 0x0ff;
	hi = reg[0x05] & 0x0ff;
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_BC, (hi << 8) | lo);
	lo = reg[0x08] & 0x0ff;
	hi = reg[0x09] & 0x0ff;
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_DE, (hi << 8) | lo);
	lo = reg[0x0c] & 0x0ff;
	hi = reg[0x0d] & 0x0ff;
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_HL, (hi << 8) | lo);
	lo = reg[0x10] & 0x0ff;
	hi = reg[0x11] & 0x0ff;
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_IX, (hi << 8) | lo);
	lo = reg[0x14] & 0x0ff;
	hi = reg[0x15] & 0x0ff;
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_IY, (hi << 8) | lo);
	lo = reg[0x18] & 0x0ff;
	hi = reg[0x19] & 0x0ff;
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_PC, (hi << 8) | lo);
	lo = reg[0x1c] & 0x0ff;
	hi = reg[0x1d] & 0x0ff;
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_SP, (hi << 8) | lo);
	lo = reg[0x20] & 0x0ff;
	hi = reg[0x21] & 0x0ff;
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_AF2, (hi << 8) | lo);
	lo = reg[0x24] & 0x0ff;
	hi = reg[0x25] & 0x0ff;
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_BC2, (hi << 8) | lo);
	lo = reg[0x28] & 0x0ff;
	hi = reg[0x29] & 0x0ff;
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_DE2, (hi << 8) | lo);
	lo = reg[0x2c] & 0x0ff;
	hi = reg[0x2d] & 0x0ff;
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_HL2, (hi << 8) | lo);
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_IFF1, reg[0x30]&0x0ff);
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_IFF2, reg[0x34]&0x0ff);
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_HALT, reg[0x38]&0x0ff);
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_IM, reg[0x3c]&0x0ff);
	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_I, reg[0x40]&0x0ff);

	cpu_set_reg(cputag_get_cpu(machine, "maincpu"), Z80_R, (reg[0x44]&0x7f) | (reg[0x48]&0x80));

	cputag_set_input_line(machine, "maincpu", 0, 0);
	cputag_set_input_line(machine, "maincpu", INPUT_LINE_NMI, 0);
	cputag_set_input_line(machine, "maincpu", INPUT_LINE_HALT, 0);
}

static void ti85_setup_snapshot (running_machine *machine, UINT8 * data)
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	int i;
	unsigned char lo,hi;
	unsigned char * hdw = data + 0x8000 + 0x94;

	ti8x_snapshot_setup_registers (machine, data);

	/* Memory dump */
	for (i = 0; i < 0x8000; i++)
	   memory_write_byte(space, i + 0x8000, data[i+0x94]);

	ti85_keypad_mask = hdw[0x00]&0x7f;

	ti8x_memory_page_1 = hdw[0x08]&0xff;
	update_ti85_memory(machine);

	ti85_power_mode = hdw[0x14]&0xff;

	lo = hdw[0x28] & 0x0ff;
	hi = hdw[0x29] & 0x0ff;
	ti85_LCD_memory_base = (((hi << 8) | lo)-0xc000)>>8;

	ti85_LCD_status = hdw[0x2c]&0xff ? 0 : 0x08;
	if (ti85_LCD_status) ti85_LCD_mask = 0x02;

	ti85_LCD_contrast = hdw[0x30]&0xff;

	ti85_timer_interrupt_mask = !hdw[0x38];
	ti85_timer_interrupt_status = 0;

	ti85_ON_interrupt_mask = !ti85_LCD_status && ti85_timer_interrupt_status;
	ti85_ON_interrupt_status = 0;
	ti85_ON_pressed = 0;

	ti85_video_buffer_width = 0x02;
	ti85_interrupt_speed = 0x03;
}

static void ti86_setup_snapshot (running_machine *machine, UINT8 * data)
{
	unsigned char lo,hi;
	unsigned char * hdw = data + 0x20000 + 0x94;

	ti8x_snapshot_setup_registers (machine, data);

	/* Memory dump */
	memcpy(ti8x_ram, data+0x94, 0x20000);

	ti85_keypad_mask = hdw[0x00]&0x7f;

	ti8x_memory_page_1 = hdw[0x04]&0xff ? 0x40 : 0x00;
	ti8x_memory_page_1 |= hdw[0x08]&0x0f;

	ti8x_memory_page_2 = hdw[0x0c]&0xff ? 0x40 : 0x00;
	ti8x_memory_page_2 |= hdw[0x10]&0x0f;

	update_ti86_memory(machine);

	lo = hdw[0x2c] & 0x0ff;
	hi = hdw[0x2d] & 0x0ff;
	ti85_LCD_memory_base = (((hi << 8) | lo)-0xc000)>>8;

	ti85_LCD_status = hdw[0x30]&0xff ? 0 : 0x08;
	if (ti85_LCD_status) ti85_LCD_mask = 0x02;

	ti85_LCD_contrast = hdw[0x34]&0xff;

	ti85_timer_interrupt_mask = !hdw[0x3c];
	ti85_timer_interrupt_status = 0;

	ti85_ON_interrupt_mask = !ti85_LCD_status && ti85_timer_interrupt_status;
	ti85_ON_interrupt_status = 0;
	ti85_ON_pressed = 0;

	ti85_video_buffer_width = 0x02;
	ti85_interrupt_speed = 0x03;
}

SNAPSHOT_LOAD( ti8x )
{
	int expected_snapshot_size = 0;
	UINT8 *ti8x_snapshot_data;

	switch (ti_calculator_model)
	{
		case TI_85: expected_snapshot_size = TI85_SNAPSHOT_SIZE; break;
		case TI_86: expected_snapshot_size = TI86_SNAPSHOT_SIZE; break;
	}

	logerror("Snapshot loading\n");

	if (snapshot_size != expected_snapshot_size)
	{
		logerror ("Incomplete snapshot file\n");
		return INIT_FAIL;
	}

	if (!(ti8x_snapshot_data = malloc(snapshot_size)))
		return INIT_FAIL;

	image_fread(image, ti8x_snapshot_data, snapshot_size);

	switch (ti_calculator_model)
	{
		case TI_85: ti85_setup_snapshot(image->machine, ti8x_snapshot_data); break;
		case TI_86: ti86_setup_snapshot(image->machine, ti8x_snapshot_data); break;
	}
	free(ti8x_snapshot_data);
	return INIT_PASS;
}

/***************************************************************************
  TI calculators serial link transmission
***************************************************************************/

DEVICE_START( ti85_serial )
{
	ti85_free_serial_data_memory(device->machine);
	ti85_receive_serial (NULL,0);
}

DEVICE_IMAGE_LOAD( ti85_serial )
{
	UINT8* file_data;
	UINT16 file_size;

	if (ti85_serial_status != TI85_SEND_STOP) return INIT_FAIL;

	file_size = image_length(image);

	if (file_size != 0)
	{
		file_data = auto_alloc_array(image->machine, UINT8, file_size);
		image_fread(image, file_data, file_size);

		if(!ti85_convert_file_data_to_serial_stream(file_data, file_size, &ti85_serial_stream, (char*)image->machine->gamedrv->name))
		{
			ti85_free_serial_stream (&ti85_serial_stream);
			return INIT_FAIL;
		}

		ti85_serial_status = TI85_SEND_HEADER;
	}
	else
	{
		return INIT_FAIL;
	}
	return INIT_PASS;
}

DEVICE_IMAGE_UNLOAD( ti85_serial )
{
	ti85_free_serial_data_memory(image->machine);
	ti85_serial_status = TI85_SEND_STOP;
	ti85_free_serial_stream (&ti85_serial_stream);
}

static int ti85_alloc_serial_data_memory (UINT32 size)
{
	if (!ti85_receive_buffer)
	{
		ti85_receive_buffer = (UINT8*) malloc (8*size*sizeof(UINT8));
		if (!ti85_receive_buffer)
			return 0;
	}

	if (!ti85_receive_data)
	{
		ti85_receive_data = (UINT8*) malloc (size * sizeof(UINT8));
		if (!ti85_receive_data)
		{
			free (ti85_receive_buffer);
			ti85_receive_buffer = NULL;
			return 0;
	        }
	}
	return 1;
}

static void ti85_free_serial_data_memory (running_machine *machine)
{
	if (ti85_receive_buffer)
	{
		free (ti85_receive_buffer);
		ti85_receive_buffer = NULL;
	}
	if (ti85_receive_data)
	{
		free (ti85_receive_data);
		ti85_receive_data = NULL;
	}
}

int ti85_receive_serial (UINT8* received_data, UINT32 received_data_size)
{
	static UINT32 data_counter = 0;

	if (data_counter >= received_data_size)
	{
		if (!ti85_red_out && !ti85_white_out)
		{
			ti85_red_in = ti85_white_in = 1;
			data_counter = 0;
			return 0;
		}
		return 1;
	}

	if (ti85_red_in && ti85_white_in && (ti85_red_out!=ti85_white_out))
	{
		ti85_white_in = ti85_white_out;
		ti85_red_in = ti85_red_out;
		received_data[data_counter] = ti85_white_out;
		return 1;
	}

	if (ti85_red_in!=ti85_white_in && !ti85_red_out && !ti85_white_out)
	{
		ti85_red_in = ti85_white_in = 1;
		data_counter ++;
		return 1;
	}
	return 1;
}

static int ti85_send_serial(UINT8* serial_data, UINT32 serial_data_size)
	{
	static UINT32 data_counter = 0;

	if (data_counter>=serial_data_size)
	{
		if (!ti85_red_out && !ti85_white_out)
		{
			ti85_red_in = ti85_white_in = 1;
			data_counter = 0;
			return 0;
		}
		ti85_red_in = ti85_white_in = 1;
		return 1;
	}

	if (ti85_red_in && ti85_white_in && !ti85_red_out && !ti85_white_out)
	{
		ti85_red_in =  serial_data[data_counter] ? 1 : 0;
		ti85_white_in = serial_data[data_counter] ? 0 : 1;
		return 1;
	}

	if ((ti85_red_in == ti85_red_out) && (ti85_white_in == ti85_white_out))
	{
		ti85_red_in = ti85_white_in = 1;
		data_counter++;
		return 1;
	}
	return 1;
}

void ti85_update_serial (running_machine *machine)
{
	if (ti85_serial_status == TI85_SEND_STOP)
	{
		if (input_port_read(machine, "SERIAL") & 0x01)
		{
			if(!ti85_alloc_serial_data_memory(15)) return;
			if(!ti85_receive_serial (ti85_receive_buffer, 7*8))
			{
				ti85_convert_stream_to_data (ti85_receive_buffer, 7*8, ti85_receive_data);
				if (ti85_receive_data[0]!=0x85 || ti85_receive_data[1]!=0x06)
				{
					ti85_receive_serial (NULL, 0);
					return;
				}
				ti85_serial_transfer_type = (ti85_receive_data[6]==0x1d) ? TI85_RECEIVE_BACKUP : TI85_RECEIVE_VARIABLES;
				ti85_serial_status = (ti85_receive_data[6]==0x1d) ? TI85_RECEIVE_HEADER_2 : TI85_PREPARE_VARIABLE_DATA;
			}
		}
		else
		{
			ti85_receive_serial(NULL, 0);
			ti85_free_serial_data_memory(machine);
			if (input_port_read(machine, "DUMP") & 0x01)
			{
				ti85_serial_status = TI85_PREPARE_SCREEN_REQUEST;
				ti85_serial_transfer_type = TI85_RECEIVE_SCREEN;
			}
			return;
		}
	}

	switch (ti85_serial_transfer_type)
	{
		case TI85_SEND_VARIABLES:
			ti85_send_variables(machine);
			break;
		case TI85_SEND_BACKUP:
			ti85_send_backup(machine);
			break;
		case TI85_RECEIVE_BACKUP:
			ti85_receive_backup(machine);
			break;
		case TI85_RECEIVE_VARIABLES:
			ti85_receive_variables(machine);
			break;
		case TI85_RECEIVE_SCREEN:
			ti85_receive_screen(machine);
			break;
	}
}

static void ti85_reset_serial (running_machine *machine)
{
	ti85_receive_serial(NULL, 0);
	ti85_send_serial(NULL, 0);
	ti85_free_serial_data_memory(machine);
	ti85_red_in = 0x01;
	ti85_white_in = 0x01;
	ti85_red_out = 0x00;
	ti85_white_out = 0x00;
	ti85_PCR = 0xc0;
	ti85_serial_status = TI85_SEND_RESET;
	ti85_send_variables(machine);
	ti85_send_backup(machine);
	ti85_receive_variables(machine);
	ti85_receive_backup(machine);
	ti85_serial_status = TI85_SEND_STOP;
}
static void ti85_send_variables (running_machine *machine)
{
	static UINT16 variable_number = 0;

	if (!ti85_alloc_serial_data_memory(7))
		ti85_serial_status = TI85_SEND_STOP;

	switch (ti85_serial_status)
	{
		case TI85_SEND_HEADER:
			if(!ti85_send_serial(ti85_serial_stream.variables[variable_number].header,ti85_serial_stream.variables[variable_number].header_size))
			{
				ti85_serial_status = TI85_RECEIVE_OK_1;
				memset (ti85_receive_data, 0, 7);
				logerror ("Header sended\n");
			}
			break;
		case TI85_RECEIVE_OK_1:
			if(!ti85_receive_serial (ti85_receive_buffer, 4*8))
			{
				ti85_convert_stream_to_data (ti85_receive_buffer, 4*8, ti85_receive_data);
				ti85_serial_status = TI85_RECEIVE_ANSWER_1;
				logerror ("OK received\n");
			}
			break;
		case TI85_RECEIVE_ANSWER_1:
			if(!ti85_receive_serial (ti85_receive_buffer, 4*8))
			{
				ti85_convert_stream_to_data (ti85_receive_buffer, 4*8, ti85_receive_data);
				switch (ti85_receive_data[1])
				{
					case 0x09:	//continue
						ti85_serial_status = TI85_SEND_OK;
						logerror ("Continue received\n");
						break;
					case 0x36:      //out of memory, skip or exit
						ti85_serial_status = TI85_RECEIVE_ANSWER_2;
						break;
				}
			}
			break;
		case TI85_RECEIVE_ANSWER_2:
			if(!ti85_receive_serial (ti85_receive_buffer+4*8, 1*8))
			{
				ti85_convert_stream_to_data (ti85_receive_buffer, 5*8, ti85_receive_data);
				switch (ti85_receive_data[4])
				{
					case 0x01:	//exit
					case 0x02:	//skip
						ti85_serial_status = TI85_RECEIVE_ANSWER_3;
						break;
					case 0x03:	//out of memory
						variable_number = 0;
						ti85_free_serial_data_memory(machine);
						ti85_serial_status = TI85_SEND_STOP;
						break;
				}
			}
			break;
		case TI85_RECEIVE_ANSWER_3:
			if(!ti85_receive_serial (ti85_receive_buffer+5*8, 2*8))
			{
				ti85_convert_stream_to_data (ti85_receive_buffer, 7*8, ti85_receive_data);
				switch (ti85_receive_data[4])
				{
					case 0x01:	//exit
						variable_number = 0;
						ti85_free_serial_data_memory(machine);
						ti85_serial_status = TI85_SEND_STOP;
						break;
					case 0x02:	//skip
						variable_number++;
						ti85_serial_status = TI85_SEND_OK;
						break;
				}
			}
			break;
		case TI85_SEND_OK:
			if(!ti85_send_serial(ti85_serial_stream.variables[variable_number].ok,ti85_serial_stream.variables[variable_number].ok_size))
			{
				ti85_serial_status = (ti85_receive_data[4]==0x02) ?  ((variable_number < ti85_serial_stream.number_of_variables) ? TI85_SEND_HEADER : TI85_SEND_END) : TI85_SEND_DATA;
				memset(ti85_receive_data, 0, 7);
			}
			break;
		case TI85_SEND_DATA:
			if(!ti85_send_serial(ti85_serial_stream.variables[variable_number].data,ti85_serial_stream.variables[variable_number].data_size))
				ti85_serial_status = TI85_RECEIVE_OK_2;
			break;
		case TI85_RECEIVE_OK_2:
			if(!ti85_receive_serial (ti85_receive_buffer, 4*8))
			{
				variable_number++;
				ti85_convert_stream_to_data (ti85_receive_buffer, 4*8, ti85_receive_data);
				ti85_serial_status = (variable_number < ti85_serial_stream.number_of_variables) ? TI85_SEND_HEADER : TI85_SEND_END;
			}
			break;
		case TI85_SEND_END:
			if(!ti85_send_serial(ti85_serial_stream.end,ti85_serial_stream.end_size))
			{
				logerror ("End sended\n");
				variable_number = 0;
				ti85_serial_status = TI85_RECEIVE_OK_3;
			}
			break;
		case TI85_RECEIVE_OK_3:
			if(!ti85_receive_serial (ti85_receive_buffer, 4*8))
			{
				logerror ("OK received\n");
				ti85_convert_stream_to_data (ti85_receive_buffer, 4*8, ti85_receive_data);
				ti85_free_serial_data_memory(machine);
				ti85_serial_status = TI85_SEND_STOP;
			}
			break;
		case TI85_SEND_RESET:
			variable_number = 0;
			break;
	}
}

static void ti85_send_backup (running_machine *machine)
{
	static UINT16 variable_number = 0;

	if (!ti85_alloc_serial_data_memory(7))
		ti85_serial_status = TI85_SEND_STOP;

	switch (ti85_serial_status)
	{
		case TI85_SEND_HEADER:
			if(!ti85_send_serial(ti85_serial_stream.variables[variable_number].header,ti85_serial_stream.variables[variable_number].header_size))
				ti85_serial_status = TI85_RECEIVE_OK_1;
			break;
		case TI85_RECEIVE_OK_1:
			if(!ti85_receive_serial (ti85_receive_buffer, 4*8))
			{
				ti85_convert_stream_to_data (ti85_receive_buffer, 4*8, ti85_receive_data);
				ti85_serial_status = TI85_RECEIVE_ANSWER_1;
			}
			break;
		case TI85_RECEIVE_ANSWER_1:
			if(!ti85_receive_serial (ti85_receive_buffer, 4*8))
			{
				ti85_convert_stream_to_data (ti85_receive_buffer, 4*8, ti85_receive_data);
				switch (ti85_receive_data[1])
				{
					case 0x09:	//continue
						ti85_serial_status = TI85_SEND_OK;
						break;
					case 0x36:      //out of memory, skip or exit
						ti85_serial_status = TI85_RECEIVE_ANSWER_2;
						break;
				}
			}
			break;
		case TI85_RECEIVE_ANSWER_2:
			if(!ti85_receive_serial (ti85_receive_buffer+4*8, 3*8))
			{
				ti85_convert_stream_to_data (ti85_receive_buffer, 5*8, ti85_receive_data);
				variable_number = 0;
				ti85_free_serial_data_memory(machine);
				ti85_serial_status = TI85_SEND_STOP;
			}
			break;
		case TI85_SEND_OK:
			if(!ti85_send_serial(ti85_serial_stream.variables[variable_number].ok,ti85_serial_stream.variables[variable_number].ok_size))
				ti85_serial_status = TI85_SEND_DATA;
			break;
		case TI85_SEND_DATA:
			if(!ti85_send_serial(ti85_serial_stream.variables[variable_number].data,ti85_serial_stream.variables[variable_number].data_size))
				ti85_serial_status = TI85_RECEIVE_OK_2;
			break;
		case TI85_RECEIVE_OK_2:
			if(!ti85_receive_serial (ti85_receive_buffer, 4*8))
			{
				variable_number++;
				ti85_convert_stream_to_data (ti85_receive_buffer, 4*8, ti85_receive_data);
				ti85_serial_status = (variable_number < ti85_serial_stream.number_of_variables) ? TI85_SEND_DATA : TI85_SEND_END;
			}
			break;
		case TI85_SEND_END:
			if(!ti85_send_serial(ti85_serial_stream.end,ti85_serial_stream.end_size))
			{
				variable_number = 0;
				ti85_free_serial_data_memory(machine);
				ti85_serial_status = TI85_SEND_STOP;
			}
			break;
		case TI85_SEND_RESET:
			variable_number = 0;
			break;
	}
}

static void ti85_receive_variables (running_machine *machine)
{
	static UINT8* var_data = NULL;
	static UINT32 var_file_number = 0;
	static char var_file_name[] = "00000000.85g";
	static int variable_number = 0;
	mame_file * var_file;
	static UINT8* var_file_data = NULL;
	UINT8* temp;
	static int var_file_size = 0;
	file_error filerr;

	switch (ti85_serial_status)
	{
		case TI85_RECEIVE_HEADER_1:
			if(!ti85_receive_serial(ti85_receive_buffer+4*8, 3*8))
			{
				ti85_convert_stream_to_data (ti85_receive_buffer+4*8, 3*8, ti85_receive_data+4);
				ti85_serial_status = TI85_PREPARE_VARIABLE_DATA;
			}
			break;
		case TI85_PREPARE_VARIABLE_DATA:
			var_data = (UINT8*) malloc (ti85_receive_data[2]+2+ti85_receive_data[4]+ti85_receive_data[5]*256+2);
			if(!var_data)
				ti85_serial_status = TI85_SEND_STOP;
			memcpy (var_data, ti85_receive_data+2, 5);
			ti85_free_serial_data_memory(machine);
			if (!ti85_alloc_serial_data_memory(var_data[0]-1))
			{
				free(var_data); var_data = NULL;
				free(var_file_data); var_file_data = NULL;
				ti85_serial_status = TI85_SEND_STOP;
				return;
			}
			ti85_serial_status = TI85_RECEIVE_HEADER_2;
		case TI85_RECEIVE_HEADER_2:
			if(!ti85_receive_serial(ti85_receive_buffer, (var_data[0]-1)*8))
			{
				ti85_convert_stream_to_data (ti85_receive_buffer, (var_data[0]-1)*8, ti85_receive_data);
				memcpy (var_data+5, ti85_receive_data, var_data[0]-3);
				ti85_free_serial_data_memory(machine);
				if(!ti85_alloc_serial_data_memory (8))
				{
					free(var_data); var_data = NULL;
					free(var_file_data); var_file_data = NULL;
					ti85_serial_status = TI85_SEND_STOP;
					return;
				}
				ti85_convert_data_to_stream (ti85_pc_ok_packet, 4, ti85_receive_buffer);
				ti85_convert_data_to_stream (ti85_pc_continue_packet, 4, ti85_receive_buffer+4*8);
				ti85_serial_status = TI85_SEND_OK_1;
			}
			break;
		case TI85_SEND_OK_1:
			if(!ti85_send_serial(ti85_receive_buffer, 4*8))
				ti85_serial_status = TI85_SEND_CONTINUE;
			break;
		case TI85_SEND_CONTINUE:
			if(!ti85_send_serial(ti85_receive_buffer+4*8, 4*8))
			{
				ti85_free_serial_data_memory(machine);
				if(!ti85_alloc_serial_data_memory(4))
				{
					free(var_data); var_data = NULL;
					free(var_file_data); var_file_data = NULL;
					ti85_serial_status = TI85_SEND_STOP;
					return;
				}
				ti85_serial_status = TI85_RECEIVE_OK;
			}
			break;
		case TI85_RECEIVE_OK:
			if(!ti85_receive_serial (ti85_receive_buffer, 4*8))
			{
				ti85_convert_stream_to_data (ti85_receive_buffer, 4*8, ti85_receive_data);
				ti85_free_serial_data_memory(machine);
				if(!ti85_alloc_serial_data_memory(var_data[2]+var_data[3]*256+6))
				{
					free(var_data); var_data = NULL;
					free(var_file_data); var_file_data = NULL;
					ti85_serial_status = TI85_SEND_STOP;
					return;
				}
				ti85_serial_status = TI85_RECEIVE_DATA;
			}
			break;
		case TI85_RECEIVE_DATA:
			if(!ti85_receive_serial (ti85_receive_buffer, (var_data[2]+var_data[3]*256+6)*8))
			{
				ti85_convert_stream_to_data (ti85_receive_buffer, (var_data[2]+var_data[3]*256+6)*8, ti85_receive_data);
				memcpy (var_data+var_data[0]+2, ti85_receive_data+2, var_data[2]+var_data[3]*256+2);
				ti85_free_serial_data_memory(machine);
				if(!ti85_alloc_serial_data_memory (4))
				{
					free(var_data); var_data = NULL;
					free(var_file_data); var_file_data = NULL;
					ti85_serial_status = TI85_SEND_STOP;
					return;
				}
				ti85_convert_data_to_stream (ti85_pc_ok_packet, 4, ti85_receive_buffer);
				ti85_serial_status = TI85_SEND_OK_2;
			}
			break;
		case TI85_SEND_OK_2:
			if(!ti85_send_serial(ti85_receive_buffer, 4*8))
			{
				ti85_free_serial_data_memory(machine);
				if(!ti85_alloc_serial_data_memory(7))
				{
					free(var_data); var_data = NULL;
					free(var_file_data); var_file_data = NULL;
					ti85_serial_status = TI85_SEND_STOP;
					return;
				}
				ti85_serial_status =  TI85_RECEIVE_END_OR_HEADER_1;
			}
			break;
		case TI85_RECEIVE_END_OR_HEADER_1:
			if(!ti85_receive_serial (ti85_receive_buffer, 4*8))
			{
				if (variable_number == 0)
				{
					var_file_data = (UINT8*) malloc (0x39);
					if (!var_file_data)
					{
						free (var_data); var_data = NULL;
						free (var_file_data); var_file_data = NULL;
						ti85_serial_status = TI85_SEND_STOP;
					}
					else
					{
						memcpy(var_file_data, ti85_file_signature, 0x0b);
						memcpy(var_file_data+0x0b, "File created by MESS", 0x14);
						memset(var_file_data+0x1f, 0, 0x1a);
						var_file_size = 0x39;
					}
				}
				temp = malloc (var_file_size+var_data[0]+2+var_data[2]+var_data[3]*256+2);
				if (temp)
				{
					memcpy (temp, var_file_data, var_file_size);
					free (var_file_data);
					var_file_data = temp;
					memcpy (var_file_data + var_file_size -2, var_data, var_data[0]+2+var_data[2]+var_data[3]*256+2);
					var_file_size += var_data[0]+2+var_data[2]+var_data[3]*256+2;
					var_file_data[0x35] = (var_file_size-0x39)&0x00ff;
					var_file_data[0x36] = ((var_file_size-0x39)&0xff00)>>8;
					var_file_data[var_file_size-2] = ti85_calculate_checksum(var_file_data+0x37, var_file_size-0x39)&0x00ff;
					var_file_data[var_file_size-1] = (ti85_calculate_checksum(var_file_data+0x37, var_file_size-0x39)&0xff00)>>8;
				}
				free (var_data);
				var_data = NULL;
				variable_number ++;

				ti85_convert_stream_to_data (ti85_receive_buffer, 4*8, ti85_receive_data);
				if (ti85_receive_data[1] == 0x06)
					ti85_serial_status = TI85_RECEIVE_HEADER_1;
				else
				{
					ti85_free_serial_data_memory(machine);
					if(!ti85_alloc_serial_data_memory (4))
					{
						free(var_data); var_data = NULL;
						free(var_file_data); var_file_data = NULL;
						ti85_serial_status = TI85_SEND_STOP;
						return;
					}
					ti85_convert_data_to_stream (ti85_pc_ok_packet, 4, ti85_receive_buffer);
					ti85_serial_status = TI85_SEND_OK_3;
				}
			}
			break;
		case TI85_SEND_OK_3:
			if(!ti85_send_serial(ti85_receive_buffer, 4*8))
			{
				ti85_free_serial_data_memory(machine);
				variable_number = 0;
				ti85_serial_status =  TI85_SEND_STOP;
				sprintf (var_file_name, "%08d.85g", var_file_number);
				filerr = mame_fopen(SEARCHPATH_IMAGE, var_file_name, OPEN_FLAG_READ | OPEN_FLAG_WRITE | OPEN_FLAG_CREATE, &var_file);
				if (filerr == FILERR_NONE)
				{
					mame_fwrite(var_file, var_file_data, var_file_size);
					mame_fclose(var_file);
					free (var_file_data);
					var_file_data = NULL;
					var_file_size = 0;
					var_file_number++;
				}
			}
			break;
		case TI85_SEND_RESET:
		        if (var_data)
			{
				free(var_data); var_data = NULL;
			}
		        if (var_file_data)
			{
				free(var_file_data); var_file_data = NULL;
			}
			var_file_size = 0;
			variable_number = 0;
			break;
	}
}

static void ti85_receive_backup (running_machine *machine)
{
	static int backup_variable_number = 0;
	static int backup_data_size[3];

	file_error filerr;
	static UINT8* backup_file_data = NULL;
	static UINT32 backup_file_number = 0;
	char backup_file_name[] = "00000000.85b";
	mame_file * backup_file;

	switch (ti85_serial_status)
	{
		case TI85_RECEIVE_HEADER_2:
			if(!ti85_receive_serial(ti85_receive_buffer+7*8, 8*8))
			{
				ti85_convert_stream_to_data (ti85_receive_buffer+7*8, 8*8, ti85_receive_data+7);
				backup_data_size[0] = ti85_receive_data[4] + ti85_receive_data[5]*256;
				backup_data_size[1] = ti85_receive_data[7] + ti85_receive_data[8]*256;
				backup_data_size[2] = ti85_receive_data[9] + ti85_receive_data[10]*256;
				backup_file_data = malloc (0x42+0x06+backup_data_size[0]+backup_data_size[1]+backup_data_size[2]+0x02);
				if(!backup_file_data)
				{
					backup_variable_number = 0;
					ti85_serial_status = TI85_SEND_STOP;
					return;
				}
				memcpy(backup_file_data, ti85_file_signature, 0x0b);
				memcpy(backup_file_data+0x0b, "File created by MESS", 0x14);
				memset(backup_file_data+0x1f, 0, 0x16);
				backup_file_data[0x35] = (backup_data_size[0]+backup_data_size[1]+backup_data_size[2]+0x11)&0x00ff;
				backup_file_data[0x36] = ((backup_data_size[0]+backup_data_size[1]+backup_data_size[2]+0x11)&0xff00)>>8;
				memcpy(backup_file_data+0x37, ti85_receive_data+0x02, 0x0b);
				ti85_free_serial_data_memory(machine);
				if(!ti85_alloc_serial_data_memory (8))
				{
					free(backup_file_data); backup_file_data = NULL;
					backup_variable_number = 0;
					ti85_serial_status = TI85_SEND_STOP;
					return;
				}
				ti85_convert_data_to_stream (ti85_pc_ok_packet, 4, ti85_receive_buffer);
				ti85_convert_data_to_stream (ti85_pc_continue_packet, 4, ti85_receive_buffer+4*8);
				ti85_serial_status = TI85_SEND_OK_1;
			}
			break;
		case TI85_SEND_OK_1:
			if(!ti85_send_serial(ti85_receive_buffer, 4*8))
				ti85_serial_status = TI85_SEND_CONTINUE;
			break;
		case TI85_SEND_CONTINUE:
			if(!ti85_send_serial(ti85_receive_buffer+4*8, 4*8))
			{
				ti85_free_serial_data_memory(machine);
				if(!ti85_alloc_serial_data_memory(4))
				{
					free(backup_file_data); backup_file_data = NULL;
					backup_variable_number = 0;
					ti85_serial_status = TI85_SEND_STOP;
					return;
				}
				ti85_serial_status = TI85_RECEIVE_OK;
			}
			break;
		case TI85_RECEIVE_OK:
			if(!ti85_receive_serial (ti85_receive_buffer, 4*8))
			{
				ti85_convert_stream_to_data (ti85_receive_buffer, 4*8, ti85_receive_data);
				ti85_free_serial_data_memory(machine);
				if(!ti85_alloc_serial_data_memory(backup_data_size[backup_variable_number]+6))
				{
					free(backup_file_data); backup_file_data = NULL;
					backup_variable_number = 0;
					ti85_serial_status = TI85_SEND_STOP;
					return;
				}
				ti85_serial_status = TI85_RECEIVE_DATA;
			}
			break;
		case TI85_RECEIVE_DATA:
			if(!ti85_receive_serial (ti85_receive_buffer, (backup_data_size[backup_variable_number]+6)*8))
			{
				ti85_convert_stream_to_data (ti85_receive_buffer, (backup_data_size[backup_variable_number]+6)*8, ti85_receive_data);
				switch (backup_variable_number)
				{
					case 0:
						memcpy(backup_file_data+0x42, ti85_receive_data+0x02, backup_data_size[0]+0x02);
						break;
					case 1:
						memcpy(backup_file_data+0x42+backup_data_size[0]+0x02, ti85_receive_data+0x02, backup_data_size[1]+0x02);
						break;
					case 2:
						memcpy(backup_file_data+0x42+backup_data_size[0]+backup_data_size[1]+0x04, ti85_receive_data+0x02, backup_data_size[2]+0x02);
						break;
				}
				ti85_free_serial_data_memory(machine);
				if(!ti85_alloc_serial_data_memory (4))
				{
					free(backup_file_data); backup_file_data = NULL;
					backup_variable_number = 0;
					ti85_serial_status = TI85_SEND_STOP;
					return;
				}
				ti85_convert_data_to_stream (ti85_pc_ok_packet, 4, ti85_receive_buffer);
				ti85_serial_status = TI85_SEND_OK_2;
			}
			break;
		case TI85_SEND_OK_2:
			if(!ti85_send_serial(ti85_receive_buffer, 4*8))
			{
				ti85_free_serial_data_memory(machine);
				backup_variable_number++;
				if (backup_variable_number<3)
				{
					if(!ti85_alloc_serial_data_memory(backup_data_size[backup_variable_number]+6))
					{
						free(backup_file_data); backup_file_data = NULL;
						backup_variable_number = 0;
						ti85_serial_status = TI85_SEND_STOP;
						return;
					}
					ti85_serial_status =  TI85_RECEIVE_DATA;
				}
				else
				{
					backup_variable_number = 0;
					backup_file_data[0x42+0x06+backup_data_size[0]+backup_data_size[1]+backup_data_size[2]] = ti85_calculate_checksum(backup_file_data+0x37, 0x42+backup_data_size[0]+backup_data_size[1]+backup_data_size[2]+0x06-0x37)&0x00ff;
					backup_file_data[0x42+0x06+backup_data_size[0]+backup_data_size[1]+backup_data_size[2]+0x01] = (ti85_calculate_checksum(backup_file_data+0x37, 0x42+backup_data_size[0]+backup_data_size[1]+backup_data_size[2]+0x06-0x37)&0xff00)>>8;
					sprintf (backup_file_name, "%08d.85b", backup_file_number);
					filerr = mame_fopen(SEARCHPATH_IMAGE, backup_file_name, OPEN_FLAG_READ | OPEN_FLAG_WRITE | OPEN_FLAG_CREATE, &backup_file);
					if (filerr == FILERR_NONE)
					{
						mame_fwrite(backup_file, backup_file_data, 0x42+0x06+backup_data_size[0]+backup_data_size[1]+backup_data_size[2]+0x02);
						mame_fclose(backup_file);
						backup_file_number++;
					}
					free(backup_file_data); backup_file_data = NULL;
					ti85_serial_status =  TI85_SEND_STOP;
				}
			}
			break;
		case TI85_SEND_RESET:
			if (backup_file_data)
			{
				free(backup_file_data); backup_file_data = NULL;
			}
			backup_variable_number = 0;
			break;
	}
}

static void ti85_receive_screen (running_machine *machine)
{
	static UINT32 image_file_number = 0;
	char image_file_name[] = "00000000.85i";
	file_error filerr;
	mame_file * image_file;
	UINT8 * image_file_data;

	switch (ti85_serial_status)
	{
		case TI85_PREPARE_SCREEN_REQUEST:
			if(!ti85_alloc_serial_data_memory (4))
			{
				ti85_serial_status = TI85_SEND_STOP;
				return;
			}
			ti85_convert_data_to_stream (ti85_pc_screen_request_packet, 4, ti85_receive_buffer);
			ti85_serial_status = TI85_SEND_SCREEN_REQUEST;
			break;
		case TI85_SEND_SCREEN_REQUEST:
			if(!ti85_send_serial(ti85_receive_buffer, 4*8))
			{
				ti85_free_serial_data_memory(machine);
				if(!ti85_alloc_serial_data_memory (4))
				{
					ti85_serial_status = TI85_SEND_STOP;
					return;
				}
				ti85_serial_status = TI85_RECEIVE_OK;
			}
			break;
		case TI85_RECEIVE_OK:
			if(!ti85_receive_serial (ti85_receive_buffer, 4*8))
			{
				ti85_convert_stream_to_data (ti85_receive_buffer, 4*8, ti85_receive_data);
				ti85_free_serial_data_memory(machine);
				if(!ti85_alloc_serial_data_memory (1030))
				{
					ti85_serial_status = TI85_SEND_STOP;
					return;
				}
				ti85_serial_status = TI85_RECEIVE_DATA;
			}
			break;
		case TI85_RECEIVE_DATA:
			if(!ti85_receive_serial (ti85_receive_buffer, 1030*8))
			{
				ti85_convert_stream_to_data (ti85_receive_buffer, 1030*8, ti85_receive_data);
				sprintf (image_file_name, "%08d.85i", image_file_number);
				filerr = mame_fopen(SEARCHPATH_IMAGE, image_file_name, OPEN_FLAG_READ | OPEN_FLAG_WRITE | OPEN_FLAG_CREATE, &image_file);
				if (filerr == FILERR_NONE)
				{
					image_file_data = malloc (0x49+1008);
					if(!image_file_data)
					{
						ti85_free_serial_data_memory(machine);
						ti85_serial_status = TI85_SEND_OK;
						return;
					}
					memcpy(image_file_data, ti85_file_signature, 0x0b);
					memcpy(image_file_data+0x0b, "File created by MESS", 0x14);
					memset(image_file_data+0x1f, 0, 0x16);
					image_file_data[0x35] = 0x00;
					image_file_data[0x36] = 0x04;
					image_file_data[0x37] = 0x0a; //header size
					image_file_data[0x38] = 0x00;
					image_file_data[0x39] = 0xf2; //data size
					image_file_data[0x3a] = 0x03;
					image_file_data[0x3b] = 0x11; //data type
					image_file_data[0x3c] = 0x06; //name size
					memcpy(image_file_data+0x3d, "Screen", 0x06);
					image_file_data[0x43] = 0xf2;
					image_file_data[0x44] = 0x03;
					image_file_data[0x45] = 0xf0;
					image_file_data[0x46] = 0x03;
					memcpy(image_file_data+0x47, ti85_receive_data+4, 1008);
					image_file_data[1008+0x49-2] = ti85_calculate_checksum(image_file_data+0x37, 1008+0x10)&0x00ff;
					image_file_data[1008+0x49-1] = (ti85_calculate_checksum(image_file_data+0x37, 1008+0x10)&0xff00)>>8;
					mame_fwrite(image_file, image_file_data, 1008+0x49);
					mame_fclose(image_file);
					free(image_file_data);
					image_file_number++;
				}
				ti85_free_serial_data_memory(machine);
				if(!ti85_alloc_serial_data_memory (4))
				{
					ti85_serial_status = TI85_SEND_STOP;
					return;
				}
				ti85_convert_data_to_stream (ti85_pc_ok_packet, 4, ti85_receive_buffer);
				ti85_serial_status = TI85_SEND_OK;
			}
			break;
		case TI85_SEND_OK:
			if(!ti85_send_serial(ti85_receive_buffer, 4*8))
			{
				ti85_free_serial_data_memory(machine);
				ti85_serial_status = TI85_SEND_STOP;
			}
			break;
	}
}
