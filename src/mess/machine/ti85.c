/***************************************************************************
  TI-85 driver by Krzysztof Strzecha

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include <stdarg.h>
#include "emu.h"
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

static UINT8 ti85_PCR = 0xc0;

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
	memory_set_bankptr(machine, "bank2",memory_region(machine, "maincpu") + 0x010000 + 0x004000*ti8x_memory_page_1);
}

static void update_ti83p_memory (running_machine *machine)
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	if (ti8x_memory_page_1 & 0x40)
	{
		if (ti83p_port4 & 1)
		{

			memory_set_bankptr(machine, "bank3", ti8x_ram + 0x004000*(ti8x_memory_page_1&0x01));
			memory_install_write_bank(space, 0x8000, 0xbfff, 0, 0, "bank3");
		}
		else
		{
			memory_set_bankptr(machine, "bank2", ti8x_ram + 0x004000*(ti8x_memory_page_1&0x01));
			memory_install_write_bank(space, 0x4000, 0x7fff, 0, 0, "bank2");
		}
	}
	else
	{
		if (ti83p_port4 & 1)
		{
			memory_set_bankptr(machine, "bank3", memory_region(machine, "maincpu") + 0x010000 + 0x004000*(ti8x_memory_page_1&0x1f));
			memory_unmap_write(space, 0x8000, 0xbfff, 0, 0);
		}
		else
		{
			memory_set_bankptr(machine, "bank2", memory_region(machine, "maincpu") + 0x010000 + 0x004000*(ti8x_memory_page_1&0x1f));
			memory_unmap_write(space, 0x4000, 0x7fff, 0, 0);
		}
	}

	if (ti8x_memory_page_2 & 0x40)
	{
		if (ti83p_port4 & 1)
		{
			memory_set_bankptr(machine, "bank4", ti8x_ram + 0x004000*(ti8x_memory_page_2&0x01));
			memory_install_write_bank(space, 0xc000, 0xffff, 0, 0, "bank4");
		}
		else
		{
			memory_set_bankptr(machine, "bank3", ti8x_ram + 0x004000*(ti8x_memory_page_2&0x01));
			memory_install_write_bank(space, 0x8000, 0xbfff, 0, 0, "bank3");
		}

	}
	else
	{
		if (ti83p_port4 & 1)
		{
			memory_set_bankptr(machine, "bank4", memory_region(machine, "maincpu") + 0x010000 + 0x004000*(ti8x_memory_page_2&0x1f));
			memory_unmap_write(space, 0xc000, 0xffff, 0, 0);
		}
		else
		{
			memory_set_bankptr(machine, "bank3", memory_region(machine, "maincpu") + 0x010000 + 0x004000*(ti8x_memory_page_2&0x1f));
			memory_unmap_write(space, 0x8000, 0xbfff, 0, 0);
		}
	}
}

static void update_ti86_memory (running_machine *machine)
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	if (ti8x_memory_page_1 & 0x40)
	{
		memory_set_bankptr(machine, "bank2", ti8x_ram + 0x004000*(ti8x_memory_page_1&0x07));
		memory_install_write_bank(space, 0x4000, 0x7fff, 0, 0, "bank2");
	}
	else
	{
		memory_set_bankptr(machine, "bank2", memory_region(machine, "maincpu") + 0x010000 + 0x004000*(ti8x_memory_page_1&0x0f));
		memory_unmap_write(space, 0x4000, 0x7fff, 0, 0);
	}

	if (ti8x_memory_page_2 & 0x40)
	{
		memory_set_bankptr(machine, "bank3", ti8x_ram + 0x004000*(ti8x_memory_page_2&0x07));
		memory_install_write_bank(space, 0x8000, 0xbfff, 0, 0, "bank3");
	}
	else
	{
		memory_set_bankptr(machine, "bank3", memory_region(machine, "maincpu") + 0x010000 + 0x004000*(ti8x_memory_page_2&0x0f));
		memory_unmap_write(space, 0x8000, 0xbfff, 0, 0);
	}

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

	memory_unmap_write(space, 0x0000, 0x3fff, 0, 0);
	memory_unmap_write(space, 0x4000, 0x7fff, 0, 0);
	memory_set_bankptr(machine, "bank1", mem + 0x010000);
	memory_set_bankptr(machine, "bank2", mem + 0x014000);
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

	memory_unmap_write(space, 0x0000, 0x3fff, 0, 0);
	memory_unmap_write(space, 0x4000, 0x7fff, 0, 0);
	memory_set_bankptr(machine, "bank1", mem + 0x010000);
	memory_set_bankptr(machine, "bank2", mem + 0x014000);
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

	memory_unmap_write(space, 0x0000, 0x3fff, 0, 0);
	memory_unmap_write(space, 0x4000, 0x7fff, 0, 0);
	memory_set_bankptr(machine, "bank1", mem + 0x010000);
	memory_set_bankptr(machine, "bank2", mem + 0x014000);
}


MACHINE_RESET( ti85 )
{
	ti85_red_out = 0x00;
	ti85_white_out = 0x00;
	ti85_PCR = 0xc0;
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
		memory_unmap_write(space, 0x0000, 0x3fff, 0, 0);
		memory_unmap_write(space, 0x4000, 0x7fff, 0, 0);
		memory_unmap_write(space, 0x8000, 0xbfff, 0, 0);

		memory_set_bankptr(machine, "bank1", mem + 0x010000);
		memory_set_bankptr(machine, "bank2", mem + 0x010000);
		memory_set_bankptr(machine, "bank3", mem + 0x010000);
		memory_set_bankptr(machine, "bank4", ti8x_ram);

		if (ti_calculator_model == TI_83p)
			memset(ti8x_ram, 0, sizeof(unsigned char)*32*1024);

//      memset(mem + 0x83000, 0xff, 0x1000);

		ti_calculator_model = TI_83p;

		timer_pulse(machine, ATTOTIME_IN_HZ(200), NULL, 0, ti85_timer_callback);
	}
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
		memory_unmap_write(space, 0x0000, 0x3fff, 0, 0);

		memory_set_bankptr(machine, "bank1", mem + 0x010000);
		memory_set_bankptr(machine, "bank2", mem + 0x014000);

		memory_set_bankptr(machine, "bank4", ti8x_ram);

		if (ti_calculator_model == TI_86)
			memset(ti8x_ram, 0, sizeof(unsigned char)*128*1024);

		ti_calculator_model = TI_86;

		timer_pulse(machine, ATTOTIME_IN_HZ(200), NULL, 0, ti85_timer_callback);
	}
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
	running_device *ti85serial = space->machine->device("ti85serial");

	ti85_update_serial(ti85serial);
	return (ti85_white_out<<3)
		| (ti85_red_out<<2)
		| ((ti85serial_white_in(ti85serial,0)&(1-ti85_white_out))<<1)
		| (ti85serial_red_in(ti85serial,0)&(1-ti85_red_out))
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
	running_device *speaker = space->machine->device("speaker");
	running_device *ti85serial = space->machine->device("ti85serial");

	speaker_level_w(speaker,( (data>>2)|(data>>3) )&0x01 );
	ti85_red_out=(data>>2)&0x01;
	ti85_white_out=(data>>3)&0x01;
	ti85serial_red_out( ti85serial, 0, ti85_red_out );
	ti85serial_white_out( ti85serial, 0, ti85_white_out );
	ti85_update_serial(ti85serial);
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
				cpu_set_reg(machine->device("maincpu"), Z80_PC,0x0c59);
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
				cpu_set_reg(machine->device("maincpu"), Z80_PC,0x0c59);
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
	cpu_set_reg(machine->device("maincpu"), Z80_AF, (hi << 8) | lo);
	lo = reg[0x04] & 0x0ff;
	hi = reg[0x05] & 0x0ff;
	cpu_set_reg(machine->device("maincpu"), Z80_BC, (hi << 8) | lo);
	lo = reg[0x08] & 0x0ff;
	hi = reg[0x09] & 0x0ff;
	cpu_set_reg(machine->device("maincpu"), Z80_DE, (hi << 8) | lo);
	lo = reg[0x0c] & 0x0ff;
	hi = reg[0x0d] & 0x0ff;
	cpu_set_reg(machine->device("maincpu"), Z80_HL, (hi << 8) | lo);
	lo = reg[0x10] & 0x0ff;
	hi = reg[0x11] & 0x0ff;
	cpu_set_reg(machine->device("maincpu"), Z80_IX, (hi << 8) | lo);
	lo = reg[0x14] & 0x0ff;
	hi = reg[0x15] & 0x0ff;
	cpu_set_reg(machine->device("maincpu"), Z80_IY, (hi << 8) | lo);
	lo = reg[0x18] & 0x0ff;
	hi = reg[0x19] & 0x0ff;
	cpu_set_reg(machine->device("maincpu"), Z80_PC, (hi << 8) | lo);
	lo = reg[0x1c] & 0x0ff;
	hi = reg[0x1d] & 0x0ff;
	cpu_set_reg(machine->device("maincpu"), Z80_SP, (hi << 8) | lo);
	lo = reg[0x20] & 0x0ff;
	hi = reg[0x21] & 0x0ff;
	cpu_set_reg(machine->device("maincpu"), Z80_AF2, (hi << 8) | lo);
	lo = reg[0x24] & 0x0ff;
	hi = reg[0x25] & 0x0ff;
	cpu_set_reg(machine->device("maincpu"), Z80_BC2, (hi << 8) | lo);
	lo = reg[0x28] & 0x0ff;
	hi = reg[0x29] & 0x0ff;
	cpu_set_reg(machine->device("maincpu"), Z80_DE2, (hi << 8) | lo);
	lo = reg[0x2c] & 0x0ff;
	hi = reg[0x2d] & 0x0ff;
	cpu_set_reg(machine->device("maincpu"), Z80_HL2, (hi << 8) | lo);
	cpu_set_reg(machine->device("maincpu"), Z80_IFF1, reg[0x30]&0x0ff);
	cpu_set_reg(machine->device("maincpu"), Z80_IFF2, reg[0x34]&0x0ff);
	cpu_set_reg(machine->device("maincpu"), Z80_HALT, reg[0x38]&0x0ff);
	cpu_set_reg(machine->device("maincpu"), Z80_IM, reg[0x3c]&0x0ff);
	cpu_set_reg(machine->device("maincpu"), Z80_I, reg[0x40]&0x0ff);

	cpu_set_reg(machine->device("maincpu"), Z80_R, (reg[0x44]&0x7f) | (reg[0x48]&0x80));

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
		return IMAGE_INIT_FAIL;
	}

	if (!(ti8x_snapshot_data = (UINT8*)malloc(snapshot_size)))
		return IMAGE_INIT_FAIL;

	image.fread( ti8x_snapshot_data, snapshot_size);

	switch (ti_calculator_model)
	{
		case TI_85: ti85_setup_snapshot(image.device().machine, ti8x_snapshot_data); break;
		case TI_86: ti86_setup_snapshot(image.device().machine, ti8x_snapshot_data); break;
	}
	free(ti8x_snapshot_data);
	return IMAGE_INIT_PASS;
}


