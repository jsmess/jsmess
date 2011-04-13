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


static TIMER_CALLBACK(ti85_timer_callback)
{
	ti85_state *state = machine.driver_data<ti85_state>();
	if (input_port_read(machine, "ON") & 0x01)
	{
		if (state->m_ON_interrupt_mask && !state->m_ON_pressed)
		{
			device_set_input_line(state->m_maincpu, 0, HOLD_LINE);
			state->m_ON_interrupt_status = 1;
			if (!state->m_timer_interrupt_mask) state->m_timer_interrupt_mask = 1;
		}
		state->m_ON_pressed = 1;
		return;
	}
	else
		state->m_ON_pressed = 0;
	if (state->m_timer_interrupt_mask)
	{
		device_set_input_line(state->m_maincpu, 0, HOLD_LINE);
		state->m_timer_interrupt_status = 1;
	}
}

static void update_ti85_memory (running_machine &machine)
{
	ti85_state *state = machine.driver_data<ti85_state>();
	memory_set_bankptr(machine, "bank2",machine.region("maincpu")->base() + 0x010000 + 0x004000*state->m_ti8x_memory_page_1);
}

static void update_ti83p_memory (running_machine &machine)
{
	ti85_state *state = machine.driver_data<ti85_state>();
	address_space *space = state->m_maincpu->memory().space(AS_PROGRAM);

	if (state->m_ti8x_memory_page_1 & 0x40)
	{
		if (state->m_ti83p_port4 & 1)
		{

			memory_set_bankptr(machine, "bank3", state->m_ti8x_ram + 0x004000*(state->m_ti8x_memory_page_1&0x01));
			space->install_write_bank(0x8000, 0xbfff, "bank3");
		}
		else
		{
			memory_set_bankptr(machine, "bank2", state->m_ti8x_ram + 0x004000*(state->m_ti8x_memory_page_1&0x01));
			space->install_write_bank(0x4000, 0x7fff, "bank2");
		}
	}
	else
	{
		if (state->m_ti83p_port4 & 1)
		{
			memory_set_bankptr(machine, "bank3", machine.region("maincpu")->base() + 0x010000 + 0x004000*(state->m_ti8x_memory_page_1&0x1f));
			space->unmap_write(0x8000, 0xbfff);
		}
		else
		{
			memory_set_bankptr(machine, "bank2", machine.region("maincpu")->base() + 0x010000 + 0x004000*(state->m_ti8x_memory_page_1&0x1f));
			space->unmap_write(0x4000, 0x7fff);
		}
	}

	if (state->m_ti8x_memory_page_2 & 0x40)
	{
		if (state->m_ti83p_port4 & 1)
		{
			memory_set_bankptr(machine, "bank4", state->m_ti8x_ram + 0x004000*(state->m_ti8x_memory_page_2&0x01));
			space->install_write_bank(0xc000, 0xffff, "bank4");
		}
		else
		{
			memory_set_bankptr(machine, "bank3", state->m_ti8x_ram + 0x004000*(state->m_ti8x_memory_page_2&0x01));
			space->install_write_bank(0x8000, 0xbfff, "bank3");
		}

	}
	else
	{
		if (state->m_ti83p_port4 & 1)
		{
			memory_set_bankptr(machine, "bank4", machine.region("maincpu")->base() + 0x010000 + 0x004000*(state->m_ti8x_memory_page_2&0x1f));
			space->unmap_write(0xc000, 0xffff);
		}
		else
		{
			memory_set_bankptr(machine, "bank3", machine.region("maincpu")->base() + 0x010000 + 0x004000*(state->m_ti8x_memory_page_2&0x1f));
			space->unmap_write(0x8000, 0xbfff);
		}
	}
}

static void update_ti86_memory (running_machine &machine)
{
	ti85_state *state = machine.driver_data<ti85_state>();
	address_space *space = state->m_maincpu->memory().space(AS_PROGRAM);

	if (state->m_ti8x_memory_page_1 & 0x40)
	{
		memory_set_bankptr(machine, "bank2", state->m_ti8x_ram + 0x004000*(state->m_ti8x_memory_page_1&0x07));
		space->install_write_bank(0x4000, 0x7fff, "bank2");
	}
	else
	{
		memory_set_bankptr(machine, "bank2", machine.region("maincpu")->base() + 0x010000 + 0x004000*(state->m_ti8x_memory_page_1&0x0f));
		space->unmap_write(0x4000, 0x7fff);
	}

	if (state->m_ti8x_memory_page_2 & 0x40)
	{
		memory_set_bankptr(machine, "bank3", state->m_ti8x_ram + 0x004000*(state->m_ti8x_memory_page_2&0x07));
		space->install_write_bank(0x8000, 0xbfff, "bank3");
	}
	else
	{
		memory_set_bankptr(machine, "bank3", machine.region("maincpu")->base() + 0x010000 + 0x004000*(state->m_ti8x_memory_page_2&0x0f));
		space->unmap_write(0x8000, 0xbfff);
	}

}

/***************************************************************************
  Machine Initialization
***************************************************************************/

MACHINE_START( ti81 )
{
	ti85_state *state = machine.driver_data<ti85_state>();
	address_space *space = state->m_maincpu->memory().space(AS_PROGRAM);
	UINT8 *mem = machine.region("maincpu")->base();

	state->m_timer_interrupt_mask = 0;
	state->m_timer_interrupt_status = 0;
	state->m_ON_interrupt_mask = 0;
	state->m_ON_interrupt_status = 0;
	state->m_ON_pressed = 0;
	state->m_power_mode = 0;
	state->m_keypad_mask = 0;
	state->m_ti8x_memory_page_1 = 0;
	state->m_LCD_memory_base = 0;
	state->m_LCD_status = 0;
	state->m_LCD_mask = 0;
	state->m_video_buffer_width = 0;
	state->m_interrupt_speed = 0;
	state->m_port4_bit0 = 0;
	state->m_ti81_port_7_data = 0;

	machine.scheduler().timer_pulse(attotime::from_hz(200), FUNC(ti85_timer_callback));

	space->unmap_write(0x0000, 0x3fff);
	space->unmap_write(0x4000, 0x7fff);
	memory_set_bankptr(machine, "bank1", mem + 0x010000);
	memory_set_bankptr(machine, "bank2", mem + 0x014000);
}

MACHINE_RESET( ti85 )
{
	ti85_state *state = machine.driver_data<ti85_state>();
	state->m_red_out = 0x00;
	state->m_white_out = 0x00;
	state->m_PCR = 0xc0;
}


MACHINE_START( ti83p )
{
	ti85_state *state = machine.driver_data<ti85_state>();
	address_space *space = state->m_maincpu->memory().space(AS_PROGRAM);
	UINT8 *mem = machine.region("maincpu")->base();

	state->m_timer_interrupt_mask = 0;
	state->m_timer_interrupt_status = 0;
	state->m_ON_interrupt_mask = 0;
	state->m_ON_interrupt_status = 0;
	state->m_ON_pressed = 0;
	state->m_ti8x_memory_page_1 = 0;
	state->m_ti8x_memory_page_2 = 0;
	state->m_LCD_memory_base = 0;
	state->m_LCD_status = 0;
	state->m_LCD_mask = 0;
	state->m_power_mode = 0;
	state->m_keypad_mask = 0;
	state->m_video_buffer_width = 0;
	state->m_interrupt_speed = 0;
	state->m_port4_bit0 = 0;

	state->m_ti8x_ram = auto_alloc_array(machine, UINT8, 32*1024);
	memset(state->m_ti8x_ram, 0, sizeof(UINT8)*32*1024);

	space->unmap_write(0x0000, 0x3fff);
	space->unmap_write(0x4000, 0x7fff);
	space->unmap_write(0x8000, 0xbfff);

	memory_set_bankptr(machine, "bank1", mem + 0x010000);
	memory_set_bankptr(machine, "bank2", mem + 0x010000);
	memory_set_bankptr(machine, "bank3", mem + 0x010000);
	memory_set_bankptr(machine, "bank4", state->m_ti8x_ram);

	machine.scheduler().timer_pulse(attotime::from_hz(200), FUNC(ti85_timer_callback));

}


MACHINE_START( ti86 )
{
	ti85_state *state = machine.driver_data<ti85_state>();
	address_space *space = state->m_maincpu->memory().space(AS_PROGRAM);
	UINT8 *mem = machine.region("maincpu")->base();

	state->m_timer_interrupt_mask = 0;
	state->m_timer_interrupt_status = 0;
	state->m_ON_interrupt_mask = 0;
	state->m_ON_interrupt_status = 0;
	state->m_ON_pressed = 0;
	state->m_ti8x_memory_page_1 = 0;
	state->m_ti8x_memory_page_2 = 0;
	state->m_LCD_memory_base = 0;
	state->m_LCD_status = 0;
	state->m_LCD_mask = 0;
	state->m_power_mode = 0;
	state->m_keypad_mask = 0;
	state->m_video_buffer_width = 0;
	state->m_interrupt_speed = 0;
	state->m_port4_bit0 = 0;

	state->m_ti8x_ram = auto_alloc_array(machine, UINT8, 128*1024);
	memset(state->m_ti8x_ram, 0, sizeof(UINT8)*128*1024);

	space->unmap_write(0x0000, 0x3fff);

	memory_set_bankptr(machine, "bank1", mem + 0x010000);
	memory_set_bankptr(machine, "bank2", mem + 0x014000);

	memory_set_bankptr(machine, "bank4", state->m_ti8x_ram);

	machine.scheduler().timer_pulse(attotime::from_hz(200), FUNC(ti85_timer_callback));
}


/* I/O ports handlers */

READ8_HANDLER ( ti85_port_0000_r )
{
	return 0xff;
}

READ8_HANDLER ( ti8x_keypad_r )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();
	int data = 0xff;
	int port;
	int bit;
	static const char *const bitnames[] = { "BIT0", "BIT1", "BIT2", "BIT3", "BIT4", "BIT5", "BIT6", "BIT7" };

	if (state->m_keypad_mask == 0x7f) return data;

	for (bit = 0; bit < 7; bit++)
	{
		if (~state->m_keypad_mask&(0x01<<bit))
		{
			for (port = 0; port < 8; port++)
			{
				data ^= input_port_read(space->machine(), bitnames[port]) & (0x01<<bit) ? 0x01<<port : 0x00;
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
	ti85_state *state = space->machine().driver_data<ti85_state>();
	int data = 0;

	if (state->m_LCD_status)
		data |= state->m_LCD_mask;
	if (state->m_ON_interrupt_status)
		data |= 0x01;
	if (state->m_timer_interrupt_status)
		data |= 0x04;
	if (!state->m_ON_pressed)
		data |= 0x08;
	state->m_ON_interrupt_status = 0;
	state->m_timer_interrupt_status = 0;
	return data;
}

 READ8_HANDLER ( ti85_port_0004_r )
{
	return 0xff;
}

 READ8_HANDLER ( ti85_port_0005_r )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();
	return state->m_ti8x_memory_page_1;
}

READ8_HANDLER ( ti85_port_0006_r )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();
	return state->m_power_mode;
}

READ8_HANDLER ( ti8x_serial_r )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();

	ti85_update_serial(state->m_serial);
	return (state->m_white_out<<3)
		| (state->m_red_out<<2)
		| ((ti85serial_white_in(state->m_serial,0)&(1-state->m_white_out))<<1)
		| (ti85serial_red_in(state->m_serial,0)&(1-state->m_red_out))
		| state->m_PCR;
}

 READ8_HANDLER ( ti82_port_0002_r )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();
	return state->m_ti8x_port2;
}

READ8_HANDLER ( ti86_port_0005_r )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();
	return state->m_ti8x_memory_page_1;
}

 READ8_HANDLER ( ti86_port_0006_r )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();
	return state->m_ti8x_memory_page_2;
}

READ8_HANDLER ( ti83_port_0000_r )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();
	return ((state->m_ti8x_memory_page_1 & 0x08) << 1) | 0x0C;
}

 READ8_HANDLER ( ti83_port_0002_r )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();
	return state->m_ti8x_port2;
}

 READ8_HANDLER ( ti83_port_0003_r )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();
	int data = 0;

	data |= state->m_LCD_mask;

	if (state->m_ON_interrupt_status)
		data |= 0x01;
	if (!state->m_ON_pressed)
		data |= 0x08;
	state->m_ON_interrupt_status = 0;
	state->m_timer_interrupt_status = 0;
	return data;
}

READ8_HANDLER ( ti8x_plus_serial_r )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();

	ti85_update_serial(state->m_serial);
	return (state->m_white_out<<3)
		| (state->m_red_out<<2)
		| ((ti85serial_white_in(state->m_serial,0)&(1-state->m_white_out))<<1)
		| (ti85serial_red_in(state->m_serial,0)&(1-state->m_red_out))
		| state->m_PCR;
}

 READ8_HANDLER ( ti83p_port_0002_r )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();
	return state->m_ti8x_port2|3;
}

WRITE8_HANDLER ( ti81_port_0007_w )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();
	state->m_ti81_port_7_data = data;
}

WRITE8_HANDLER ( ti85_port_0000_w )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();
	state->m_LCD_memory_base = data;
}

WRITE8_HANDLER ( ti8x_keypad_w )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();
	state->m_keypad_mask = data&0x7f;
}

WRITE8_HANDLER ( ti85_port_0002_w )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();
	state->m_LCD_contrast = data&0x1f;
}

WRITE8_HANDLER ( ti85_port_0003_w )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();
	if (state->m_LCD_status && !(data&0x08))	state->m_timer_interrupt_mask = 0;
	state->m_ON_interrupt_mask = data&0x01;
//  state->m_timer_interrupt_mask = data&0x04;
	state->m_LCD_mask = data&0x02;
	state->m_LCD_status = data&0x08;
}

WRITE8_HANDLER ( ti85_port_0004_w )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();
	state->m_video_buffer_width = (data>>3)&0x03;
	state->m_interrupt_speed = (data>>1)&0x03;
	state->m_port4_bit0 = data&0x01;
}

WRITE8_HANDLER ( ti85_port_0005_w )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();
	state->m_ti8x_memory_page_1 = data;
	update_ti85_memory(space->machine());
}

WRITE8_HANDLER ( ti85_port_0006_w )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();
	state->m_power_mode = data;
}

WRITE8_HANDLER ( ti8x_serial_w )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();

	speaker_level_w(state->m_speaker, ( (data>>2)|(data>>3) ) & 0x01);
	state->m_red_out=(data>>2)&0x01;
	state->m_white_out=(data>>3)&0x01;
	ti85serial_red_out( state->m_serial, 0, state->m_red_out );
	ti85serial_white_out( state->m_serial, 0, state->m_white_out );
	ti85_update_serial(state->m_serial);
	state->m_PCR = data&0xf0;
}

WRITE8_HANDLER ( ti86_port_0005_w )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();
	state->m_ti8x_memory_page_1 = data&((data&0x40)?0x47:0x4f);
	update_ti86_memory(space->machine());
}

WRITE8_HANDLER ( ti86_port_0006_w )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();
	state->m_ti8x_memory_page_2 = data&((data&0x40)?0x47:0x4f);
	update_ti86_memory(space->machine());
}

WRITE8_HANDLER ( ti82_port_0002_w )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();
	state->m_ti8x_memory_page_1 = (data & 0x07);
	update_ti85_memory(space->machine());
	state->m_ti8x_port2 = data;
}

WRITE8_HANDLER ( ti83_port_0000_w )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();
	state->m_ti8x_memory_page_1 = (state->m_ti8x_memory_page_1 & 7) | ((data & 16) >> 1);
	update_ti85_memory(space->machine());
}

WRITE8_HANDLER ( ti83_port_0002_w )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();
	state->m_ti8x_memory_page_1 = (state->m_ti8x_memory_page_1 & 8) | (data & 7);
	update_ti85_memory(space->machine());
	state->m_ti8x_port2 = data;
}

WRITE8_HANDLER ( ti83_port_0003_w )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();
		if (state->m_LCD_status && !(data&0x08))	state->m_timer_interrupt_mask = 0;
		state->m_ON_interrupt_mask = data&0x01;
		//state->m_timer_interrupt_mask = data&0x04;
		state->m_LCD_mask = data&0x02;
		state->m_LCD_status = data&0x08;
}

WRITE8_HANDLER ( ti8x_plus_serial_w )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();

	speaker_level_w(state->m_speaker,( (data>>0)|(data>>1) )&0x01 );
	state->m_red_out=(data>>0)&0x01;
	state->m_white_out=(data>>1)&0x01;
	ti85serial_red_out( state->m_serial, 0, state->m_red_out );
	ti85serial_white_out( state->m_serial, 0, state->m_white_out );
	ti85_update_serial(state->m_serial);
	state->m_PCR = data&0xf0;
}

WRITE8_HANDLER ( ti83p_port_0002_w )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();
	state->m_ti8x_port2 = data;
}

WRITE8_HANDLER ( ti83p_port_0003_w )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();
	state->m_LCD_mask = (data&0x08) >> 2;
	state->m_ON_interrupt_mask = data & 0x01;
}

WRITE8_HANDLER ( ti83p_port_0004_w )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();
	if ((data & 1) && !(state->m_ti83p_port4 & 1))
	{
		state->m_ti8x_memory_page_1 = 0x1f;
		state->m_ti8x_memory_page_2 = 0x1f;
	}
	else if (!(data & 1) && (state->m_ti83p_port4 & 1))
	{
		state->m_ti8x_memory_page_1 = 0x1f;
		state->m_ti8x_memory_page_2 = 0x40;
	}
	update_ti83p_memory(space->machine());
	state->m_ti83p_port4 = data;
}

WRITE8_HANDLER ( ti83p_port_0006_w )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();
	state->m_ti8x_memory_page_1 = data & ((data&0x40) ? 0x41 : 0x5f);
	update_ti83p_memory(space->machine());
}

WRITE8_HANDLER ( ti83p_port_0007_w )
{
	ti85_state *state = space->machine().driver_data<ti85_state>();
	state->m_ti8x_memory_page_2 = data & ((data&0x40) ? 0x41 : 0x5f);
	update_ti83p_memory(space->machine());
}

/* NVRAM functions */
NVRAM_HANDLER( ti83p )
{
	ti85_state *state = machine.driver_data<ti85_state>();
	if (read_or_write)
	{
		file->write(state->m_ti8x_ram, sizeof(unsigned char)*32*1024);
	}
	else
	{
			if (file)
			{
				file->read(state->m_ti8x_ram, sizeof(unsigned char)*32*1024);
				cpu_set_reg(state->m_maincpu, Z80_PC,0x0c59);
			}
			else
				memset(state->m_ti8x_ram, 0, sizeof(unsigned char)*32*1024);
	}
}

NVRAM_HANDLER( ti86 )
{
	ti85_state *state = machine.driver_data<ti85_state>();
	if (read_or_write)
	{
		file->write(state->m_ti8x_ram, sizeof(unsigned char)*128*1024);
	}
	else
	{
			if (file)
			{
				file->read(state->m_ti8x_ram, sizeof(unsigned char)*128*1024);
				cpu_set_reg(state->m_maincpu, Z80_PC,0x0c59);
			}
			else
				memset(state->m_ti8x_ram, 0, sizeof(unsigned char)*128*1024);
	}
}

/***************************************************************************
  TI calculators snapshot files (SAV)
***************************************************************************/

static void ti8x_snapshot_setup_registers (running_machine &machine, UINT8 * data)
{
	ti85_state *state = machine.driver_data<ti85_state>();
	unsigned char lo,hi;
	unsigned char * reg = data + 0x40;

	/* Set registers */
	lo = reg[0x00] & 0x0ff;
	hi = reg[0x01] & 0x0ff;
	cpu_set_reg(state->m_maincpu, Z80_AF, (hi << 8) | lo);
	lo = reg[0x04] & 0x0ff;
	hi = reg[0x05] & 0x0ff;
	cpu_set_reg(state->m_maincpu, Z80_BC, (hi << 8) | lo);
	lo = reg[0x08] & 0x0ff;
	hi = reg[0x09] & 0x0ff;
	cpu_set_reg(state->m_maincpu, Z80_DE, (hi << 8) | lo);
	lo = reg[0x0c] & 0x0ff;
	hi = reg[0x0d] & 0x0ff;
	cpu_set_reg(state->m_maincpu, Z80_HL, (hi << 8) | lo);
	lo = reg[0x10] & 0x0ff;
	hi = reg[0x11] & 0x0ff;
	cpu_set_reg(state->m_maincpu, Z80_IX, (hi << 8) | lo);
	lo = reg[0x14] & 0x0ff;
	hi = reg[0x15] & 0x0ff;
	cpu_set_reg(state->m_maincpu, Z80_IY, (hi << 8) | lo);
	lo = reg[0x18] & 0x0ff;
	hi = reg[0x19] & 0x0ff;
	cpu_set_reg(state->m_maincpu, Z80_PC, (hi << 8) | lo);
	lo = reg[0x1c] & 0x0ff;
	hi = reg[0x1d] & 0x0ff;
	cpu_set_reg(state->m_maincpu, Z80_SP, (hi << 8) | lo);
	lo = reg[0x20] & 0x0ff;
	hi = reg[0x21] & 0x0ff;
	cpu_set_reg(state->m_maincpu, Z80_AF2, (hi << 8) | lo);
	lo = reg[0x24] & 0x0ff;
	hi = reg[0x25] & 0x0ff;
	cpu_set_reg(state->m_maincpu, Z80_BC2, (hi << 8) | lo);
	lo = reg[0x28] & 0x0ff;
	hi = reg[0x29] & 0x0ff;
	cpu_set_reg(state->m_maincpu, Z80_DE2, (hi << 8) | lo);
	lo = reg[0x2c] & 0x0ff;
	hi = reg[0x2d] & 0x0ff;
	cpu_set_reg(state->m_maincpu, Z80_HL2, (hi << 8) | lo);
	cpu_set_reg(state->m_maincpu, Z80_IFF1, reg[0x30]&0x0ff);
	cpu_set_reg(state->m_maincpu, Z80_IFF2, reg[0x34]&0x0ff);
	cpu_set_reg(state->m_maincpu, Z80_HALT, reg[0x38]&0x0ff);
	cpu_set_reg(state->m_maincpu, Z80_IM, reg[0x3c]&0x0ff);
	cpu_set_reg(state->m_maincpu, Z80_I, reg[0x40]&0x0ff);

	cpu_set_reg(state->m_maincpu, Z80_R, (reg[0x44]&0x7f) | (reg[0x48]&0x80));

	device_set_input_line(state->m_maincpu, 0, 0);
	device_set_input_line(state->m_maincpu, INPUT_LINE_NMI, 0);
	device_set_input_line(state->m_maincpu, INPUT_LINE_HALT, 0);
}

static void ti85_setup_snapshot (running_machine &machine, UINT8 * data)
{
	ti85_state *state = machine.driver_data<ti85_state>();
	address_space *space = state->m_maincpu->memory().space(AS_PROGRAM);
	int i;
	unsigned char lo,hi;
	unsigned char * hdw = data + 0x8000 + 0x94;

	ti8x_snapshot_setup_registers (machine, data);

	/* Memory dump */
	for (i = 0; i < 0x8000; i++)
	   space->write_byte(i + 0x8000, data[i+0x94]);

	state->m_keypad_mask = hdw[0x00]&0x7f;

	state->m_ti8x_memory_page_1 = hdw[0x08]&0xff;
	update_ti85_memory(machine);

	state->m_power_mode = hdw[0x14]&0xff;

	lo = hdw[0x28] & 0x0ff;
	hi = hdw[0x29] & 0x0ff;
	state->m_LCD_memory_base = (((hi << 8) | lo)-0xc000)>>8;

	state->m_LCD_status = hdw[0x2c]&0xff ? 0 : 0x08;
	if (state->m_LCD_status) state->m_LCD_mask = 0x02;

	state->m_LCD_contrast = hdw[0x30]&0xff;

	state->m_timer_interrupt_mask = !hdw[0x38];
	state->m_timer_interrupt_status = 0;

	state->m_ON_interrupt_mask = !state->m_LCD_status && state->m_timer_interrupt_status;
	state->m_ON_interrupt_status = 0;
	state->m_ON_pressed = 0;

	state->m_video_buffer_width = 0x02;
	state->m_interrupt_speed = 0x03;
}

static void ti86_setup_snapshot (running_machine &machine, UINT8 * data)
{
	ti85_state *state = machine.driver_data<ti85_state>();
	unsigned char lo,hi;
	unsigned char * hdw = data + 0x20000 + 0x94;

	ti8x_snapshot_setup_registers (machine, data);

	/* Memory dump */
	memcpy(state->m_ti8x_ram, data+0x94, 0x20000);

	state->m_keypad_mask = hdw[0x00]&0x7f;

	state->m_ti8x_memory_page_1 = hdw[0x04]&0xff ? 0x40 : 0x00;
	state->m_ti8x_memory_page_1 |= hdw[0x08]&0x0f;

	state->m_ti8x_memory_page_2 = hdw[0x0c]&0xff ? 0x40 : 0x00;
	state->m_ti8x_memory_page_2 |= hdw[0x10]&0x0f;

	update_ti86_memory(machine);

	lo = hdw[0x2c] & 0x0ff;
	hi = hdw[0x2d] & 0x0ff;
	state->m_LCD_memory_base = (((hi << 8) | lo)-0xc000)>>8;

	state->m_LCD_status = hdw[0x30]&0xff ? 0 : 0x08;
	if (state->m_LCD_status) state->m_LCD_mask = 0x02;

	state->m_LCD_contrast = hdw[0x34]&0xff;

	state->m_timer_interrupt_mask = !hdw[0x3c];
	state->m_timer_interrupt_status = 0;

	state->m_ON_interrupt_mask = !state->m_LCD_status && state->m_timer_interrupt_status;
	state->m_ON_interrupt_status = 0;
	state->m_ON_pressed = 0;

	state->m_video_buffer_width = 0x02;
	state->m_interrupt_speed = 0x03;
}

SNAPSHOT_LOAD( ti8x )
{
	int expected_snapshot_size = 0;
	UINT8 *ti8x_snapshot_data;

	if (!strncmp(image.device().machine().system().name, "ti85", 4))
		expected_snapshot_size = TI85_SNAPSHOT_SIZE;
	else if (!strncmp(image.device().machine().system().name, "ti86", 4))
		expected_snapshot_size = TI86_SNAPSHOT_SIZE;

	logerror("Snapshot loading\n");

	if (snapshot_size != expected_snapshot_size)
	{
		logerror ("Incomplete snapshot file\n");
		return IMAGE_INIT_FAIL;
	}

	if (!(ti8x_snapshot_data = (UINT8*)malloc(snapshot_size)))
		return IMAGE_INIT_FAIL;

	image.fread( ti8x_snapshot_data, snapshot_size);

	if (!strncmp(image.device().machine().system().name, "ti85", 4))
		ti85_setup_snapshot(image.device().machine(), ti8x_snapshot_data);
	else if (!strncmp(image.device().machine().system().name, "ti86", 4))
		ti86_setup_snapshot(image.device().machine(), ti8x_snapshot_data);

	free(ti8x_snapshot_data);
	return IMAGE_INIT_PASS;
}

