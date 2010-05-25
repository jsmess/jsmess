/***************************************************************************

    Casio X-07

	Driver by Sandro Ronco based on X-07 emu by J.Brigaud

	TODO:
	-External video
	-Sound
	-NVRAM and Save State
	-Kana and graphic char
	-Serial port


	Memory Map

	0x0000 - 0x1fff   Internal RAM
	0x2000 - 0x3fff   External RAM Card
	0x4000 - 0x5fff   Extension ROM/RAM
	0x6000 - 0x7fff   ROM Card
	0x8000 - 0x97ff   Video RAM
	0x9800 - 0x9fff   ?
	0xA000 - 0xafff   TV ROM
	0xb000 - 0xffff   ROM

	CPU was actually a NSC800 (Z80 compatible)
	More info: http://www.silicium.org/oldskool/calc/x07/

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "includes/x07.h"

static void t6834_cmd (running_machine *machine, UINT8 cmd);
static void receive_from_t6834 (running_machine *machine);
static void ack_from_t6834 (running_machine *machine);
static void send_to_t6834 (running_machine *machine);
static void send_to_printer (running_machine *machine);
static void keyboard_scan(running_machine *machine);
static void irq_exec(running_machine *machine);
static void draw_char(running_machine *machine, UINT8 x, UINT8 y, UINT8 char_pos);
static void draw_point(running_machine *machine, UINT8 x, UINT8 y, UINT8 color);
static void draw_udk(running_machine *machine);

class t6834_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, t6834_state(machine)); }

	t6834_state(running_machine &machine) { }

	/* General */
	UINT8 t6834_ram[800];
	UINT8 regs_r[8];
	UINT8 regs_w[8];
	UINT8 udc[256 * 8];
	UINT8 alarm[8];
	UINT8 enable_k7;
	UINT8 udk;
	UINT8 z80_irq;
	UINT8 in_data[256];
	UINT8 in_size;
	UINT8 in_pos;
	UINT8 out_data[256];
	UINT8 out_size;
	UINT8 out_pos;

	/* LCD */
	UINT8 lcd_on;
	UINT8 lcd_map[32][120];
	UINT8 scroll_min;
	UINT8 scroll_max;
	UINT8 cur_on;
	UINT8 cur_x;
	UINT8 cur_y;
	UINT8 loc_on ;
	UINT8 loc_x;
	UINT8 loc_y;
	UINT8 font_code;

	/* Keyboard */
	UINT8 kb_on;
	UINT8 ctrl;
	UINT8 shift;
	UINT8 kana;
	UINT8 graph;
	UINT8 stick;
	UINT8 strig;
	UINT8 strig1;
	UINT8 kb_brk;
	UINT8 kb_wait;
	UINT8 kb_data[20];
	UINT8 kb_size;
};


/***************************************************************************
    T6834 IMPLEMENTATION
***************************************************************************/

static void t6834_cmd (running_machine *machine, UINT8 cmd)
{
	UINT16 address;
	UINT8 p1, p2, p3, p4, i;
	t6834_state *state = (t6834_state *)machine->driver_data;

	switch (cmd)
	{
	case 0x00:
		break;

	case 0x01:	//DATA$ TIME$ read
		{
		mame_system_time systime;
		mame_get_current_datetime(machine, &systime);
		state->out_data[state->out_size++] = (systime.local_time.year>>8) & 0xFF;
		state->out_data[state->out_size++] = systime.local_time.year & 0xFF;
		state->out_data[state->out_size++] = systime.local_time.month + 1;
		state->out_data[state->out_size++] = systime.local_time.mday;
		state->out_data[state->out_size++] = ~(((0x01 << (7 - systime.local_time.weekday)) - 1) & 0xff);
		state->out_data[state->out_size++] = systime.local_time.hour;
		state->out_data[state->out_size++] = systime.local_time.minute;
		state->out_data[state->out_size++] = systime.local_time.second;
		}
		break;

	case 0x02:	//STICK read
		state->out_data[state->out_size++] = state->stick;

		break;

	case 0x03:	//strig
		state->out_data[state->out_size++] = state->strig;
		break;

	case 0x04:	//strig1
		state->out_data[state->out_size++] = state->strig1;
		break;

	case 0x05:	//T6834 Ram Read
   		address = state->in_data[state->in_pos++];
		address |= state->in_data[state->in_pos++] << 8;
		if(address == 0xc00e)
			p1 = 0x0a;
		else if(address == 0xd000)
			p1 = input_port_read(machine, "BATTERY");
		else if(0x200 <= address && address < 0x300)
			p1 = state->udc[0x400 + (address - 0x200)];
		else if(0x300 <= address && address < 0x400)
			p1 = state->udc[0x700 + (address - 0x300)];
		else
			p1 = state->t6834_ram[address & 0x7ff];
		state->out_data[state->out_size++] = p1;
		break;

	case 0x06:	//T6834 Ram Write
	   	p1 = state->in_data[state->in_pos++];
   		address = state->in_data[state->in_pos++];
		address |= state->in_data[state->in_pos++] << 8;

		if(0x200 <= address && address < 0x300)
			state->udc[0x400 + (address - 0x200)] = p1;
		else if(0x300 <= address && address < 0x400)
			state->udc[0x700 + (address - 0x300)] = p1;
		else
			state->t6834_ram[address & 0x7ff] = p1;
		break;

	case 0x07:	//Set line to scroll
		state->scroll_min = state->in_data[state->in_pos++];
		state->scroll_max = state->in_data[state->in_pos++];
		break;

	case 0x08:	//Scroll LCD
		if(state->scroll_min <= state->scroll_max && state->scroll_max < 4)
		{
			for(i = state->scroll_min * 8; i < state->scroll_max * 8; i++)
				memcpy(&state->lcd_map[i][0], &state->lcd_map[i + 8][0], 120);

			for(i = state->scroll_max * 8; i < (state->scroll_max + 1) * 8; i++)
				memset(&state->lcd_map[i][0], 0, 120);
		}
		break;

	case 0x09:	//Clear LCD Line
		p1 = state->in_data[state->in_pos++];
		if(p1 < 4)
			for(UINT8 l = p1 * 8; l < (p1 + 1) * 8; l++)
				memset(&state->lcd_map[l][0], 0, 120);
		break;

	case 0x0a:	//DATA$ TIME$ write
		break;
	case 0x0b:	//Calendar
	case 0x0c:	//ALM$ write
		for(i = 0; i < 8; i++)
			state->alarm[i] = state->in_data[state->in_pos++];
		break;

	case 0x0d:	//BeepOn
	case 0x0e:	//BeepOff
		break;

	case 0x0f:	//Read LCD Line
		p2 = state->in_data[state->in_pos++];
		for(i = 0; i < 120; i++)
			if(p2 < 32)
				state->out_data[state->out_size++] = (state->lcd_map[p2][i]);
			else
				state->out_data[state->out_size++] = 0;
		break;

	case 0x10:	//Read LCD Point
		p1 = state->in_data[state->in_pos++];
		p2 = state->in_data[state->in_pos++];
		if(p1 < 120 && p2 < 32)
			state->out_data[state->out_size++] = state->lcd_map[p2][p1];
		else
			state->out_data[state->out_size++] = 0;
		break;

	case 0x11:	//PSET
		p1 = state->in_data[state->in_pos++];
		p2 = state->in_data[state->in_pos++];
		draw_point(machine, p1, p2, 0x01);
		break;

	case 0x12:	//PRESET
		p1 = state->in_data[state->in_pos++];
		p2 = state->in_data[state->in_pos++];
		draw_point(machine, p1, p2, 0x00);
		break;

	case 0x13:	//Invert LCD Point color
		p1 = state->in_data[state->in_pos++];
		p2 = state->in_data[state->in_pos++];
		if(p1 < 120 && p2 < 32)
			state->lcd_map[p2][p1] = ~state->lcd_map[p2][p1];
		break;

	case 0x14:	//Line
	{
		UINT8 delta_x, delta_y, step_x, step_y, next_x, next_y;
		INT16 frac;
		next_x = p1 = state->in_data[state->in_pos++];
		next_y = p2 = state->in_data[state->in_pos++];
		p3 = state->in_data[state->in_pos++];
		p4 = state->in_data[state->in_pos++];
		delta_x = abs(p3 - p1) * 2;
		delta_y = abs(p4 - p2) * 2;
		step_x = (p3 < p1) ? -1 : 1;
		step_y = (p4 < p2) ? -1 : 1;

		if(delta_x > delta_y)
		{
			frac = delta_y - delta_x / 2;
			while(next_x != p3)
			{
				if(frac >= 0)
				{
					next_y += step_y;
					frac -= delta_x;
				}
				next_x += step_x;
				frac += delta_y;
				draw_point(machine, next_x, next_y, 0x01);
			}
		}
		else {
			frac = delta_x - delta_y / 2;
			while(next_y != p4)
			{
				if(frac >= 0)
				{
					next_x += step_x;
					frac -= delta_y;
				}
				next_y += step_y;
				frac += delta_x;
				draw_point(machine, next_x, next_y, 0x01);
			}
		}
		draw_point(machine, p1, p2, 0x01);
		draw_point(machine, p3, p4, 0x01);
		break;
	}
	case 0x15:	//Circle
		p1 = state->in_data[state->in_pos++];
		p2 = state->in_data[state->in_pos++];
		p3 = state->in_data[state->in_pos++];

		for(int x = 0, y = p3; x <= sqrt((p3 * p3) / 2) ; x++)
		{
			UINT32 d1 = (x * x + y * y) - p3 * p3;
			UINT32 d2 = (x * x + (y - 1) * (y - 1)) - p3 * p3;
			if(abs(d1) > abs(d2))
				y--;
			draw_point(machine, x + p1, y + p2, 0x01);
			draw_point(machine, x + p1, -y + p2, 0x01);
			draw_point(machine, -x + p1, y + p2, 0x01);
			draw_point(machine, -x + p1, -y + p2, 0x01);
			draw_point(machine, y + p1, x + p2, 0x01);
			draw_point(machine, y + p1, -x + p2, 0x01);
			draw_point(machine, -y + p1, x + p2, 0x01);
			draw_point(machine, -y + p1, -x + p2, 0x01);
		}
		break;

	case 0x16:	//udk write
		p1 = state->in_data[state->in_pos++];
		for(i = 0; i < ((p1 != 5 && p1 != 11) ? 0x2a : 0x2e); i++)
			state->t6834_ram[udk_ptr[p1] + i] = state->in_data[state->in_pos++];
		break;

	case 0x17:	//udk read
		p1 = state->in_data[state->in_pos++];
		for(i = 0; i < ((p1 != 5 && p1 != 11) ? 0x2a : 0x2e); i++)
		{
			UINT8 code = state->t6834_ram[udk_ptr[p1] + i];
			state->out_data[state->out_size++] = code;
			if(!code)	break;
		}
		break;

	case 0x18:	//udk Oon
	case 0x19:	//udk off
		break;

	case 0x1a:	//udc write
		address = state->in_data[state->in_pos++] << 3;
		for(i = 0; i < 8; i++)
			state->udc[address + i] = state->in_data[state->in_pos++];

		break;

	case 0x1b:	//udc wead
		address = state->in_data[state->in_pos++] << 3;
		for(i = 0; i < 8; i++)
			state->out_data[state->out_size++] = state->udc[address + i];
		break;

	case 0x1c:	//udc Init
		memcpy(&state->udc, &font, sizeof(state->udc));
		break;

	case 0x1d:	//Pgm Write
	case 0x1e:	//SP Write
	case 0x1f:	//SP On
	case 0x20:	//SP Off
		break;

	case 0x21:	//Pgm Read
		for(i = 0; i < 128; i++)
			state->out_data[state->out_size++] = 0;
		break;

	case 0x22: //Start
		state->out_data[state->out_size++] = 0x04;
		break;

	case 0x23:	//OFF
		state->lcd_on=0;
		break;

	case 0x24:	//Locate
	   	p1 = state->in_data[state->in_pos++];
		p2 = state->in_data[state->in_pos++];
		p3 = state->in_data[state->in_pos++];
		state->loc_on = (state->loc_x != p1 || state->loc_y != p2);
		state->loc_x = state->cur_x = p1;
		state->loc_y = state->cur_y = p2;
		if(p3)
			draw_char(machine, p1, p2, p3);
		break;

	case 0x25:	//Cursor On
		state->cur_on = 1;
		break;

	case 0x26:	//Cursor Off
		state->cur_on = 0;
		break;

	case 0x27:	//TestKey
	case 0x28:	//TestChr
		state->out_data[state->out_size++] = 0;
		break;

	case 0x29:	//Init Sec
	case 0x2a:	//Init Date
		break;

	case 0x2b:	//LCD off
		state->lcd_on = 0;
		break;

	case 0x2c:	//LCD on
		state->lcd_on = 1;
		break;

	case 0x2d:	//KeyBuffer clear
		memset(&state->kb_data, 0, sizeof(state->kb_data));
		break;

	case 0x2e:	//CLS
		memset(&state->lcd_map, 0, sizeof(state->lcd_map));
		break;

	case 0x2f:	//Home
		state->cur_x = state->cur_y = 0;
		break;

	case 0x30:	//udk On
		state->udk = 1;
		draw_udk(machine);
		break;

	case 0x31: 	//udk Off
		state->udk = 0;
		p1 = state->in_data[state->in_pos++];
		for(UINT8 l = 3 * 8; l < (3 + 1) * 8; l++)
			memset(&state->lcd_map[l][0], 0, 120);
		break;

	case 0x32:	//RepateKey on
	case 0x33:	//RepateKey off
	case 0x34:	//KANA
	case 0x35:	//udk ContWrite
		break;

	case 0x36:	//Alarm Read
		for(i = 0; i < 8; i++)
			state->out_data[state->out_size++] = state->alarm[i];
		break;

	case 0x37: // BuzzZero
		state->out_data[state->out_size++] = 0;
		break;

	case 0x38:	//Click off
	case 0x39:	//Click on
	case 0x3a:	//Locate Close
		break;

	case 0x3b: // keyboard on
		state->kb_on = 1;
		break;

	case 0x3c: // keyboard off
		state->kb_on = 0;
		break;

	case 0x3d:	//Start Pgm
	case 0x3e:	//Unexec Start Pgm
		break;

	case 0x3f: //Sleep
		state->lcd_on = 0;
		break;

	case 0x40:	//udk init
		memset(state->t6834_ram, 0, 0x200);
		for(i = 0; i < 12; i++)
			strcpy((char*)state->t6834_ram + udk_ptr[i], udk_ini[i]);
		break;

	case 0x41:	//Out udc
		break;

	case 0x42: // ReadCar
		for(i = 0; i < 8; i++)
			state->out_data[state->out_size++] = 0;
		break;

	case 0x43:	//ScanR
	case 0x44:	//ScanL
		state->out_data[state->out_size++] = 0;
		state->out_data[state->out_size++] = 0;
		break;

	case 0x45:	//TimeChk
	case 0x46:	//AlmChk
		state->out_data[state->out_size++] = 0;
	default:
		printf ("T6834 unknown cmd: 0x%02x\n", cmd);
	}
}


static void receive_from_t6834 (running_machine *machine)
{
	t6834_state *state = (t6834_state *)machine->driver_data;

	state->regs_r[0]  = 0x40;
	state->regs_r[1]  = state->out_data[state->out_pos];
	state->regs_r[2] |= 0x01;
}


static void ack_from_t6834 (running_machine *machine)
{
	t6834_state *state = (t6834_state *)machine->driver_data;

	state->out_pos++;
	state->regs_r[2] &= 0xfe;
	if(state->out_size > state->out_pos)
	{
		state->z80_irq |= 1;
		irq_exec(machine);
	}
}


static void send_to_t6834 (running_machine *machine)
{
	t6834_state *state = (t6834_state *)machine->driver_data;

	if (!state->in_size)
	{
		if (state->loc_on && ((state->regs_w[1] & 0x7F) != 0x24) && ((state->regs_w[1]) >= 0x20) && ((state->regs_w[1]) <  0x80))
		{
			state->cur_x ++;
			draw_char(machine, state->cur_x, state->cur_x, state->regs_w[1]);
		}
		else
		{
			state->loc_on = 0;

			if ((state->regs_w[1] & 0x7f) < 0x47)
			{
				state->in_data[state->in_size++] = state->regs_w[1] & 0x7f;
			}
		}
	}
	else
	{
		state->in_data[state->in_size++] = state->regs_w[1];

		if (state->in_size == 2)
		{
			if (state->in_data[state->in_pos] == 0x0c && state->regs_w [1] == 0xb0)
			{
				memset(state->lcd_map, 0, sizeof(state->lcd_map));
				state->in_size = 0;
				state->in_pos = 0;
				state->in_data[state->in_size++] = state->regs_w[1] & 0x7f;
			}

			if (state->in_data[state->in_pos] == 0x07 && state->regs_w [1] > 4)
			{
				state->in_size = 0;
				state->in_pos = 0;
				state->in_data[state->in_size++] = state->regs_w[1] & 0x7f;
			}
		}
	}

	if (state->in_size)
	{
		UINT8 cmd_len = t6834_cmd_len[state->in_data[state->in_pos]];
		if(cmd_len & 0x80)
		{
			if((cmd_len & 0x7f) < state->in_size && !state->regs_w[1])
				cmd_len = state->in_size;
		}

		if(state->in_size == cmd_len)
		{
			state->out_size = 0;
			state->out_pos = 0;
			t6834_cmd(machine, state->in_data[state->in_pos++]);
			state->in_size = 0;
			state->in_pos = 0;
			if(state->out_size)
			{
				state->z80_irq |= 1;
				irq_exec(machine);
			}
		}
	}
}


static void send_to_printer (running_machine *machine)
{
	t6834_state *state = (t6834_state *)machine->driver_data;
	static UINT8 send_bit = 0;
	static UINT8 char_code = 0;

	if (state->regs_r[4] & 0x20)
		char_code |= 1;

	send_bit++;

	if (send_bit == 8)
	{
		if (char_code)
			popmessage("Print 0x%02x\n", char_code);
		send_bit = 0;
		char_code = 0;
		state->regs_r[2] |= 0x80;
	}
	else
		char_code <<= 1;
}


static void keyboard_scan(running_machine *machine)
{
	t6834_state *state = (t6834_state *)machine->driver_data;
	static const char *const keynames[] = { "ROW0", "ROW1", "ROW2", "ROW3", "ROW4", "ROW5", "ROW6", "ROW7", "ROW8"};
	UINT8 row, col, val, kb_table = 0;
	UINT16 f_ptr = 0;
	UINT8 row6 = input_port_read(machine, keynames[6]);
	UINT8 row7 = input_port_read(machine, keynames[7]);
	UINT8 row8 = input_port_read(machine, keynames[8]);

	if (state->kb_wait || !state->kb_on)
		return;

	state->shift = (row7 & 0x01) ? 1 : 0;
	state->ctrl = (row7 & 0x02) ? 1 : 0;
	state->graph = (row7 & 0x04) ? 1 : 0;

	if(state->udk)
		draw_udk(machine);

	/* Function keys */
	if (row8)
	{
		if (row8 & 0x01)
			f_ptr = udk_ptr[state->shift ? 6 : 0];
		else if (row8 & 0x02)
			f_ptr = udk_ptr[state->shift ? 7 : 1];
		else if (row8 & 0x04)
			f_ptr = udk_ptr[state->shift ? 8 : 2];
		else if (row8 & 0x08)
			f_ptr = udk_ptr[state->shift ? 9 : 3];
		else if (row8 & 0x10)
			f_ptr = udk_ptr[state->shift ? 10 : 4];

		/* First 3 chars are used for description */
		f_ptr += 3;

		do
		{
			val = state->t6834_ram[f_ptr++];

			if (state->kb_size < sizeof(state->kb_data) && val != 0)
				state->kb_data[state->kb_size++] = val;
		} while(val != 0);
	}
	else if (row6 & 0x01)		//left
		state->stick = 0x37;
	else if (row6 & 0x02)		//right
		state->stick = 0x32;
	else if (row6 & 0x04)		//up
		state->stick = 0x31;
	else if (row6 & 0x08)		//down
		state->stick = 0x36;

	/* Check for Break key */
	if (row7 & 0x80)
	{
		if (!state->lcd_on)
		{
			state->lcd_on = 1;
			cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_PC, 0xc3c3);
		}
		else
			state->kb_brk = 1;
	}
	else
	{
		if (state->shift && !state->ctrl)
			kb_table = 1;
		else if (!state->shift && state->ctrl)
			kb_table = 2;

		for (row = 0; row < 7; row++)
		{
			UINT8 data = input_port_read(machine, keynames[row]);

			for (col = 0; col < 8; col++)
			{
				if (BIT(data, col))
				{
					UINT8 keydata = x07_keycodes[row + (kb_table * 7)][col];

					if (keydata && state->kb_size < sizeof(state->kb_data))
						state->kb_data[state->kb_size++] = keydata;
				}
			}
		}
	}

	if (state->kb_size || state->kb_brk)
	{
		state->kb_wait = 1;
		irq_exec(machine);
		timer_set(machine, ATTOTIME_IN_MSEC(150), NULL, 0, keyboard_clear);
	}
}


static void irq_exec(running_machine *machine)
{
	t6834_state *state = (t6834_state *)machine->driver_data;

	if (state->kb_size)
	{
		state->regs_r[0] = 0;
		state->regs_r[1] = state->kb_data[0];
		memcpy(state->kb_data, &state->kb_data[1], sizeof(state->kb_data - 1));
		state->kb_size--;
		state->regs_r[2] |= 0x01;
		cputag_set_input_line(machine, "maincpu", NSC800_RSTA, ASSERT_LINE);
		timer_set(machine, cputag_clocks_to_attotime(machine, "maincpu", 500), NULL, 0, irq_clear);
		return;
	}
	else if (state->kb_brk)
	{
		state->regs_r[0]  = 0x80;
		state->regs_r[1]  = 0x05;
		state->regs_r[2] |= 0x01;
		state->kb_brk = 0;
		cputag_set_input_line(machine, "maincpu", NSC800_RSTA, ASSERT_LINE );
		timer_set(machine, cputag_clocks_to_attotime(machine, "maincpu", 500), NULL, 0, irq_clear);
		return;
	}
	else if ( state->z80_irq&1 )
	{
		receive_from_t6834 (machine);
		state->z80_irq &= ~1;
		cputag_set_input_line(machine, "maincpu", NSC800_RSTA, ASSERT_LINE);
		timer_set(machine, cputag_clocks_to_attotime(machine, "maincpu", 500), NULL, 0, irq_clear);
		return;
	}
}


/***************************************************************************
    Video
***************************************************************************/

static void draw_char(running_machine *machine, UINT8 x, UINT8 y, UINT8 char_pos)
{
	t6834_state *state = (t6834_state *)machine->driver_data;
	UINT8 cl;

	if(x < 20 && y < 4)
		for(cl = 0; cl < 8; cl++)
		{
			state->lcd_map[y * 8 + cl][x * 6 + 0] = (state->udc[((char_pos << 3) + cl) & 0x7ff] & 0x80) ? 1 : 0;
			state->lcd_map[y * 8 + cl][x * 6 + 1] = (state->udc[((char_pos << 3) + cl) & 0x7ff] & 0x40) ? 1 : 0;
			state->lcd_map[y * 8 + cl][x * 6 + 2] = (state->udc[((char_pos << 3) + cl) & 0x7ff] & 0x20) ? 1 : 0;
			state->lcd_map[y * 8 + cl][x * 6 + 3] = (state->udc[((char_pos << 3) + cl) & 0x7ff] & 0x10) ? 1 : 0;
			state->lcd_map[y * 8 + cl][x * 6 + 4] = (state->udc[((char_pos << 3) + cl) & 0x7ff] & 0x08) ? 1 : 0;
			state->lcd_map[y * 8 + cl][x * 6 + 5] = (state->udc[((char_pos << 3) + cl) & 0x7ff] & 0x04) ? 1 : 0;
		}
}


static void draw_point(running_machine *machine, UINT8 x, UINT8 y, UINT8 color)
{
	t6834_state *state = (t6834_state *)machine->driver_data;
	if((x) < 120 && (y) < 32)
		state->lcd_map[y][x] = color;
}


static void draw_udk(running_machine *machine)
{
	t6834_state *state = (t6834_state *)machine->driver_data;
	UINT8 i, x, j;

	for(i = 0, x = 0; i < 5; i++)
	{
		UINT16 ofs = udk_ptr[i + (state->shift ? 6 : 0)];
		draw_char(machine, x++, 3, 0x83);
		for(j = 0; j < 3; j++)
			draw_char(machine, x++, 3, state->t6834_ram[ofs++]);
	}
}


static PALETTE_INIT( x07 )
{
	machine->colortable = colortable_alloc(machine, 2);
	colortable_palette_set_color(machine->colortable, 1, MAKE_RGB(140, 148, 140));
	colortable_palette_set_color(machine->colortable, 0, MAKE_RGB(48, 56, 16));
}


static VIDEO_UPDATE( x07 )
{
	t6834_state *state = (t6834_state *)screen->machine->driver_data;
	static UINT8 cursor_blink = 0;

	cursor_blink++;

	if (state->lcd_on)
		for(int y = 0; y < 4; y++)
		{
			int py = y * 8;
			for(int x = 0; x < 20; x++)
			{
				int px = x * 6;
				if(state->cur_on && (cursor_blink & 0x20) && state->cur_x == x && state->cur_y == y)
				{
					for(int l = 0; l < 8; l++)
					{
						*BITMAP_ADDR16(bitmap, py + l, px) = (l < 7) ? 1: 0;
						*BITMAP_ADDR16(bitmap, py + l, px+1) = (l < 7) ? 1: 0;
						*BITMAP_ADDR16(bitmap, py + l, px+2) = (l < 7) ? 1: 0;
						*BITMAP_ADDR16(bitmap, py + l, px+3) = (l < 7) ? 1: 0;
						*BITMAP_ADDR16(bitmap, py + l, px+4) = (l < 7) ? 1: 0;
						*BITMAP_ADDR16(bitmap, py + l, px+5) = (l < 7) ? 1: 0;
					}
				}
				else {
					for(int l = 0; l < 8; l++)
					{
						*BITMAP_ADDR16(bitmap, py + l, px+0) = state->lcd_map[py + l][px + 0] ? 0 : 1;
						*BITMAP_ADDR16(bitmap, py + l, px+1) = state->lcd_map[py + l][px + 1] ? 0 : 1;
						*BITMAP_ADDR16(bitmap, py + l, px+2) = state->lcd_map[py + l][px + 2] ? 0 : 1;
						*BITMAP_ADDR16(bitmap, py + l, px+3) = state->lcd_map[py + l][px + 3] ? 0 : 1;
						*BITMAP_ADDR16(bitmap, py + l, px+4) = state->lcd_map[py + l][px + 4] ? 0 : 1;
						*BITMAP_ADDR16(bitmap, py + l, px+5) = state->lcd_map[py + l][px + 5] ? 0 : 1;
					}
				}
			}
		}
	else
		bitmap_fill(bitmap, NULL, 1);

	return 0;
}


/***************************************************************************
    Machine
***************************************************************************/

static READ8_HANDLER( x07_IO_r )
{
	t6834_state *state = (t6834_state *)space->machine->driver_data;
	UINT32 val = 0xff;

	switch(offset)
	{
	case 0x80:
	case 0x81:
	case 0x82:
	case 0x83:
	case 0x84:
	case 0x85:
	case 0x86:
	case 0x87:
	case 0x88:
	case 0x89:
	case 0x8a:
	case 0x8b:
	case 0x8c:
		val = ((offset & 0x0f) < 8) ? state->udc[(state->font_code << 3) | (offset & 7)] : 0;
		break;
	case 0x90:
		val =  0x00;
		break;
	case 0xf0:
	case 0xf1:
	case 0xf3:
	case 0xf4:
	case 0xf5:
	case 0xf7:
		val = state->regs_r[offset & 7];
		break;
	case 0xf2:
		if(state->regs_w[5] & 4)
			state->regs_r[2] |= 2;
		else
			state->regs_r[2] &= 0xfd;
		val = state->regs_r[2] | 2;
		break;
	case 0xf6:
		if (state->enable_k7)
			state->regs_r[6] |= 5;
		val = state->regs_r[6];
		break;
	}
	return val;
}


static WRITE8_HANDLER( x07_IO_w )
{
	t6834_state *state = (t6834_state *)space->machine->driver_data;

	switch(offset)
	{
	case 0x80:
		state->font_code = data;
		break;
	case 0xf0:
	case 0xf1:
	case 0xf2:
	case 0xf3:
	case 0xf6:
	case 0xf7:
		state->regs_w[offset & 7] = data;
		break;
	case 0xf4:

		state->regs_r[4] = state->regs_w[4] = data;
		state->enable_k7=((data & 0x0c) == 8) ? 1 : 0;
		if((data & 0x0e) == 0x0e)
		{
			popmessage("Beep %x", state->regs_w[2] | (state->regs_w[3] << 8));

		}

		break;
	case 0xf5:
		if(data & 0x01)
			ack_from_t6834 (space->machine);
		else if(data & 0x02)
			send_to_t6834 (space->machine);
		else if(data & 0x20)
			send_to_printer (space->machine);
	}
}

static ADDRESS_MAP_START(x07_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x5fff) AM_RAM
	AM_RANGE(0x6000, 0x7fff) AM_READONLY
	AM_RANGE(0x8000, 0x97ff) AM_RAM
	AM_RANGE(0x9800, 0x9fff) AM_NOP
	AM_RANGE(0xa000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( x07_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK (0xff)
	AM_RANGE(0x00, 0xffff) AM_READWRITE(x07_IO_r, x07_IO_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( x07 )

	PORT_START("BATTERY")
		PORT_CONFNAME( 0x40, 0x30, "Battery Status" )
		PORT_CONFSETTING( 0x30, DEF_STR( Normal ) )
		PORT_CONFSETTING( 0x40, "Low Battery" )

	PORT_START("CARDBATTERY")
		PORT_CONFNAME( 0x10, 0x00, "Card Battery Status" )
		PORT_CONFSETTING( 0x00, DEF_STR( Normal ) )
		PORT_CONFSETTING( 0x10, "Low Battery" )

	PORT_START("ROW0")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
		PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
		PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
		PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
		PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
		PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
		PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')

	PORT_START("ROW1")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('|')
		PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=')
		PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('^') PORT_CHAR('`')
		PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
		PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
		PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
		PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')

	PORT_START("ROW2")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
		PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
		PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
		PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')
		PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
		PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR('@') PORT_CHAR('\'')
		PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR('{')

	PORT_START("ROW3")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
		PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
		PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
		PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
		PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
		PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
		PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('k')

	PORT_START("ROW4")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR(';') PORT_CHAR('+')
		PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':') PORT_CHAR('*')
		PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('}')
		PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
		PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')
		PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
		PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')

	PORT_START("ROW5")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
		PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
		PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
		PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
		PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
		PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Yen") PORT_CODE(KEYCODE_PGUP) PORT_CHAR('?')
		PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')

	PORT_START("ROW6")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)
		PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Up") PORT_CODE(KEYCODE_UP)
		PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN)
		PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Del") PORT_CODE(KEYCODE_DEL)
		PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Home") PORT_CODE(KEYCODE_HOME)
		PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER)
		PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Ins") PORT_CODE(KEYCODE_INSERT)

	PORT_START("ROW7")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("shift") PORT_CODE(KEYCODE_LSHIFT)
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("ctrl") PORT_CODE(KEYCODE_LCONTROL)
		PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("GRPH") PORT_CODE(KEYCODE_RCONTROL)
		PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Break") PORT_CODE(KEYCODE_RSHIFT)

	PORT_START("ROW8")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("F1") PORT_CODE(KEYCODE_F1)
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("F2") PORT_CODE(KEYCODE_F2)
		PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("F3") PORT_CODE(KEYCODE_F3)
		PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("F4") PORT_CODE(KEYCODE_F4)
		PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("F5") PORT_CODE(KEYCODE_F5)

INPUT_PORTS_END

static TIMER_CALLBACK( irq_clear )
{
	cputag_set_input_line(machine, "maincpu", NSC800_RSTA, CLEAR_LINE);
}

static TIMER_CALLBACK( keyboard_clear )
{
	t6834_state *state = (t6834_state *)machine->driver_data;

	state->kb_wait = 0;
}

static TIMER_DEVICE_CALLBACK( keyboard_poll )
{
	keyboard_scan (timer->machine);
}

static MACHINE_START( x07 )
{
	t6834_state *state = (t6834_state *)machine->driver_data;

	state_save_register_global_pointer(machine, state->t6834_ram, sizeof(state->t6834_ram));
}

static MACHINE_RESET( x07 )
{
	t6834_state *state = (t6834_state *)machine->driver_data;
	UINT8 i;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_PC, 0xc3c3);

	memset(state, 0, sizeof(t6834_state));

	state->lcd_on		= 0x01;
	state->stick		= 0x30;
	state->strig		= 0xFF;
	state->strig1		= 0xFF;
	state->scroll_max	= 0x03;

	for(i = 0; i < 12; i++)
		strcpy((char*)state->t6834_ram + udk_ptr[i], udk_ini[i]);

	state->regs_r[2] = input_port_read(machine, "CARDBATTERY");

	memcpy(&state->udc, &font, sizeof(font));
}

static MACHINE_DRIVER_START( x07 )

	MDRV_DRIVER_DATA(t6834_state)

	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", NSC800, XTAL_15_36MHz / 4)
	MDRV_CPU_PROGRAM_MAP(x07_mem)
	MDRV_CPU_IO_MAP(x07_io)

	MDRV_MACHINE_START(x07)
	MDRV_MACHINE_RESET(x07)

	/* video hardware */
	MDRV_SCREEN_ADD("lcd", LCD)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(120, 32)
	MDRV_SCREEN_VISIBLE_AREA(0, 120-1, 0, 32-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(x07)
	MDRV_DEFAULT_LAYOUT(layout_lcd)
	MDRV_VIDEO_UPDATE(x07)

	MDRV_TIMER_ADD_PERIODIC("keyboard", keyboard_poll, USEC(2500))

MACHINE_DRIVER_END

/* ROM definition */
ROM_START( x07 )
	/* very strange size... */
	ROM_REGION( 0x11000, "maincpu", 0 )
	ROM_LOAD( "x07.bin", 0xb000, 0x5001, CRC(61a6e3cc) SHA1(c53c22d33085ac7d5e490c5d8f41207729e5f08a) )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME    FLAGS */
COMP( 1983, x07,    0,      0,       x07,       x07,     0,      "Canon",  "X-07",     GAME_NO_SOUND)
