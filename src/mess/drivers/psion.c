/***************************************************************************

        Psion Organiser II series

		Driver by Sandro Ronco

		TODO:
		- implement Datapack read/write interface
		- add save state and NVRAM
		- dump CGROM of the HD44780
		- move HD44780 implementation in a device

		Note:
		- 4 lines dysplay has an custom LCD controller derived from an HD66780

        16/07/2010 Skeleton driver.

		Memory map info :
			http://archive.psion2.org/org2/techref/memory.htm

****************************************************************************/

#include "emu.h"
#include "cpu/m6800/m6800.h"
#include "sound/beep.h"

static struct _psion
{
	UINT8 kb_counter;
	UINT8 enable_nmi;
	UINT8 *sys_register;
	UINT8 tcsr_value;

	/* RAM/ROM banks */
	UINT8 *ram;
	UINT8 rom_bank;
	UINT8 ram_bank;
	UINT8 ram_bank_count;
	UINT8 rom_bank_count;
} psion;

/*************************************************

    Hitachi HD44780 LCD controller implemantation

*************************************************/

static struct _hd44780
{
	UINT8 busy_flag;

	//HD44780 has only 80 bytes of DDRAM, customized HD66780 has 128 bytes
	UINT8 ddram[128];	//internal display data RAM,
	UINT8 cgram[64];	//internal chargen RAM

	UINT8 ac;			//address counter
	UINT8 ddram_a;		//DDRAM address
	UINT8 cgram_a;		//CGRAM address
	UINT8 ac_mode;		//0=DDRAM 1=CGRAM

	UINT8 cursor_pos;	//cursor position
	UINT8 display_on;	//display on/off
	UINT8 cursor_on;	//cursor on/off
	UINT8 blink_on;		//blink on/off

	UINT8 direction;	//auto increment/decrement
	UINT8 data_len;		//interface data length 4 or 8 bit
	UINT8 n_line;		//number of lines
	UINT8 char_size;	//char size 5x8 or 5x10

	UINT8 disp_shift;	//display shift

	UINT8 blink;
} hd44780;

static TIMER_CALLBACK( bf_clear )
{
	hd44780.busy_flag = 0;
}

void set_busy_flag(running_machine *machine, UINT16 usec)
{
	hd44780.busy_flag = 1;
	timer_set(machine, ATTOTIME_IN_USEC(usec), NULL, 0, bf_clear);
}

void hd44780_reset(running_machine *machine)
{
	memset(&hd44780, 0x00, sizeof(_hd44780));
	hd44780.direction = 1;
	hd44780.data_len = 1;
	set_busy_flag(machine, 1520);
}

void hd44780_control_w(running_machine *machine, UINT8 data)
{
	if (BIT(data, 7))
	{
		hd44780.ac_mode = 0;
		hd44780.ac = data & 0x7f;
		hd44780.ddram_a = data & 0x7f;
		if (data != 0x81)
			hd44780.cursor_pos = hd44780.ddram_a;
		set_busy_flag(machine, 37);
	}
	else if (BIT(data, 6))
	{
		hd44780.ac_mode = 1;
		hd44780.ac = data & 0x3f;
		hd44780.cgram_a = data & 0x3f;
		set_busy_flag(machine, 37);
	}
	else if (BIT(data, 5))
	{
		hd44780.data_len = BIT(data, 4);
		hd44780.n_line = BIT(data, 3);
		hd44780.char_size = BIT(data, 2);
		set_busy_flag(machine, 37);
	}
	else if (BIT(data, 4))
	{
		UINT8 direction = (BIT(data, 2)) ? +1 : -1;

		if (BIT(data, 3))
			hd44780.disp_shift += direction;
		else
		{
			hd44780.ac += direction;

			if (hd44780.ac_mode == 0)
			{
				hd44780.ddram_a += hd44780.direction;
				hd44780.cursor_pos += hd44780.direction;
			}
			else
				hd44780.cgram_a += hd44780.direction;
		}
		set_busy_flag(machine, 37);
	}
	else if (BIT(data, 3))
	{
		hd44780.display_on = BIT(data, 2);
		hd44780.cursor_on = BIT(data, 1);
		hd44780.blink_on = BIT(data, 0);

		set_busy_flag(machine, 37);
	}
	else if (BIT(data, 2))
	{
		hd44780.direction = (BIT(data, 1)) ? +1 : -1;
		if (BIT(data, 0))
			hd44780.disp_shift += hd44780.direction;

		set_busy_flag(machine, 37);
	}
	else if (BIT(data, 1))
	{
		hd44780.ac = 0;
		hd44780.ddram_a = 0;
		hd44780.cgram_a = 0;
		hd44780.cursor_pos = 0;
		hd44780.ac_mode = 0;
		hd44780.direction = 1;
		set_busy_flag(machine, 1520);
	}
	else if (BIT(data, 0))
	{
		hd44780.ac = 0;
		hd44780.ddram_a = 0;
		hd44780.cgram_a = 0;
		hd44780.cursor_pos = 0;
		hd44780.ac_mode = 0;
		hd44780.direction = 1;
		memset(hd44780.ddram, 0x20, ARRAY_LENGTH(hd44780.ddram));
		memset(hd44780.cgram, 0x20, ARRAY_LENGTH(hd44780.cgram));
		set_busy_flag(machine, 1520);
	}
}

UINT8 hd44780_control_r(running_machine *machine)
{
	return hd44780.busy_flag<<7 || hd44780.ac&0x7f;
}

void hd44780_data_w(running_machine *machine, UINT8 data)
{
	if (hd44780.ac_mode == 0)
	{
		hd44780.ddram[hd44780.ac] = data;
		hd44780.ddram_a += hd44780.direction;
		hd44780.cursor_pos += hd44780.direction;
	}
	else
	{
		hd44780.cgram[hd44780.ac] = data;
		hd44780.cgram_a += hd44780.direction;
	}

	hd44780.ac += hd44780.direction;
}

UINT8 hd44780_data_r(running_machine *machine)
{
	UINT8 data;

	if (hd44780.ac_mode == 0)
	{
		data = hd44780.ddram[hd44780.ac];
		hd44780.ddram_a += hd44780.direction;
		hd44780.cursor_pos += hd44780.direction;
	}
	else
	{
		data = hd44780.cgram[hd44780.ac];
		hd44780.cgram_a += hd44780.direction;
	}

	hd44780.ac += hd44780.direction;

	return data;
}

/***********************************************

    basic machine

***********************************************/

static TIMER_CALLBACK( nmi_timer )
{
	if (psion.enable_nmi)
		cputag_set_input_line(machine, "maincpu", INPUT_LINE_NMI, PULSE_LINE);
}

static TIMER_CALLBACK( blink_timer )
{
	hd44780.blink = ~hd44780.blink;
}

UINT8 kb_read(running_machine *machine)
{
	static const char *const bitnames[] = {"K1", "K2", "K3", "K4", "K5", "K6", "K7"};
	UINT8 line, data = 0x7c;

	if (psion.kb_counter)
	{
		for (line = 0; line < 7; line++)
			if (psion.kb_counter == (0x7f & ~(1 << line)))
				data = input_port_read(machine, bitnames[line]);
	}
	else
	{
		for (line = 0; line < 7; line++)
			data &= input_port_read(machine, bitnames[line]);
	}

	return data & 0x7c;
}

void update_bank(running_machine *machine)
{
	if (psion.ram_bank < psion.ram_bank_count && psion.ram_bank_count)
		memory_set_bankptr(machine,"rambank", psion.ram + psion.ram_bank*0x4000);

	if (psion.rom_bank_count)
	{
		if (psion.rom_bank==0)
			memory_set_bankptr(machine,"rombank", memory_region(machine, "maincpu") + 0x8000);
		else if (psion.rom_bank < psion.rom_bank_count)
			memory_set_bankptr(machine,"rombank", memory_region(machine, "maincpu") + psion.rom_bank*0x4000 + 0xc000);
	}
}

WRITE8_HANDLER( hd63701_int_reg_w )
{
	switch (offset)
	{
	case 0x08:
		psion.tcsr_value = data;
		break;
	}

	hd63701_internal_registers_w(space, offset, data);
}

READ8_HANDLER( hd63701_int_reg_r )
{
	switch (offset)
	{
	case 0x15:
		/*
		x--- ---- ON key active high
		-xxx xx-- keys matrix active low
		---- --x- ??
		---- ---x battery status
		*/
		return kb_read(space->machine) | input_port_read(space->machine, "BATTERY") | input_port_read(space->machine, "ON");
	case 0x08:
		hd63701_internal_registers_w(space, offset, psion.tcsr_value);
	default:
		return hd63701_internal_registers_r(space, offset);
	}
}

/* Read/Write common */
void io_rw(const address_space* space, UINT16 offset)
{
	switch (offset & 0xffc0)
	{
	case 0xc0:
		//Switch off
		break;
	case 0x100:
		//Pulse enable
		break;
	case 0x140:
		//Pulse disable
		break;
	case 0x200:
		psion.kb_counter = 0;
		break;
	case 0x180:
		beep_set_state(space->machine->device("beep"), 1);
		break;
	case 0x1c0:
		beep_set_state(space->machine->device("beep"), 0);
		break;
	case 0x240:
		if (offset == 0x260 && (psion.rom_bank_count || psion.ram_bank_count))
		{
			psion.ram_bank=0;
			psion.rom_bank=0;
			update_bank(space->machine);
		}
		else
			psion.kb_counter++;
		break;
	case 0x280:
		if (offset == 0x2a0 && psion.ram_bank_count)
		{
			psion.ram_bank++;
			update_bank(space->machine);
		}
		else
			psion.enable_nmi = 1;
		break;
	case 0x2c0:
		if (offset == 0x2e0 && psion.rom_bank_count)
		{
			psion.rom_bank++;
			update_bank(space->machine);
		}
		else
			psion.enable_nmi = 0;
		break;
	}
}

WRITE8_HANDLER( io_w )
{
	switch (offset & 0x0ffc0)
	{
	case 0x80:
		if (offset & 1)
			hd44780_data_w(space->machine, data);
		else
			hd44780_control_w(space->machine, data);
		break;
	default:
		io_rw(space, offset);
	}
}

READ8_HANDLER( io_r )
{
	switch (offset & 0xffc0)
	{
	case 0x80:
		if (offset& 1)
			return hd44780_data_r(space->machine);
		else
			return hd44780_control_r(space->machine);
	default:
		io_rw(space, offset);
	}

	return 0;
}

static ADDRESS_MAP_START(psioncm_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_LOW
	AM_RANGE(0x0000, 0x001f) AM_READWRITE(hd63701_int_reg_r, hd63701_int_reg_w)
	AM_RANGE(0x0040, 0x00ff) AM_RAM AM_BASE(&psion.sys_register)
	AM_RANGE(0x0100, 0x03ff) AM_READWRITE(io_r, io_w)
	AM_RANGE(0x2000, 0x3fff) AM_RAM
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(psionla_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_LOW
	AM_RANGE(0x0000, 0x001f) AM_READWRITE(hd63701_int_reg_r, hd63701_int_reg_w)
	AM_RANGE(0x0040, 0x00ff) AM_RAM AM_BASE(&psion.sys_register)
	AM_RANGE(0x0100, 0x03ff) AM_READWRITE(io_r, io_w)
	AM_RANGE(0x0400, 0x5fff) AM_RAM
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(psionp350_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_LOW
	AM_RANGE(0x0000, 0x001f) AM_READWRITE(hd63701_int_reg_r, hd63701_int_reg_w)
	AM_RANGE(0x0040, 0x00ff) AM_RAM AM_BASE(&psion.sys_register)
	AM_RANGE(0x0100, 0x03ff) AM_READWRITE(io_r, io_w)
	AM_RANGE(0x0400, 0x3fff) AM_RAM
	AM_RANGE(0x4000, 0x7fff) AM_RAMBANK("rambank")
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(psionlam_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_LOW
	AM_RANGE(0x0000, 0x001f) AM_READWRITE(hd63701_int_reg_r, hd63701_int_reg_w)
	AM_RANGE(0x0040, 0x00ff) AM_RAM AM_BASE(&psion.sys_register)
	AM_RANGE(0x0100, 0x03ff) AM_READWRITE(io_r, io_w)
	AM_RANGE(0x0400, 0x7fff) AM_RAM
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK("rombank")
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(psionlz_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_LOW
	AM_RANGE(0x0000, 0x001f) AM_READWRITE(hd63701_int_reg_r, hd63701_int_reg_w)
	AM_RANGE(0x0040, 0x00ff) AM_RAM AM_BASE(&psion.sys_register)
	AM_RANGE(0x0100, 0x03ff) AM_READWRITE(io_r, io_w)
	AM_RANGE(0x0400, 0x3fff) AM_RAM
	AM_RANGE(0x4000, 0x7fff) AM_RAMBANK("rambank")
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK("rombank")
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( psion )
	PORT_START("BATTERY")
		PORT_CONFNAME( 0x01, 0x00, "Battery Status" )
		PORT_CONFSETTING( 0x00, DEF_STR( Normal ) )
		PORT_CONFSETTING( 0x01, "Low Battery" )

	PORT_START("ON")
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ON/CLEAR") PORT_CODE(KEYCODE_F10)

	PORT_START("K1")
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("MODE") PORT_CODE(KEYCODE_EQUALS)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Up [CAP]") PORT_CODE(KEYCODE_UP)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Down [NUM]") PORT_CODE(KEYCODE_DOWN)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RIGHT") PORT_CODE(KEYCODE_RIGHT)

	PORT_START("K2")
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S [;]") PORT_CODE(KEYCODE_S)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M [,]") PORT_CODE(KEYCODE_M)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G [=]") PORT_CODE(KEYCODE_G)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A [<]") PORT_CODE(KEYCODE_A)

	PORT_START("K3")
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T [:]") PORT_CODE(KEYCODE_T)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N [$]") PORT_CODE(KEYCODE_N)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H [\"]") PORT_CODE(KEYCODE_H)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B [>]") PORT_CODE(KEYCODE_B)

	PORT_START("K4")
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y [0]") PORT_CODE(KEYCODE_Y)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U [1]") PORT_CODE(KEYCODE_U)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O [4]") PORT_CODE(KEYCODE_O)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I [7]") PORT_CODE(KEYCODE_I)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C [(]") PORT_CODE(KEYCODE_C)

	PORT_START("K5")
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W [3]") PORT_CODE(KEYCODE_W)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q [6]") PORT_CODE(KEYCODE_Q)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K [9]") PORT_CODE(KEYCODE_K)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E [%]") PORT_CODE(KEYCODE_E)

	PORT_START("K6")
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("EXE") PORT_CODE(KEYCODE_ENTER)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X [+]") PORT_CODE(KEYCODE_X)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R [-]") PORT_CODE(KEYCODE_R)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L [*]") PORT_CODE(KEYCODE_L)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F [/]") PORT_CODE(KEYCODE_F)

	PORT_START("K7")
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z [.]") PORT_CODE(KEYCODE_Z)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V [2]") PORT_CODE(KEYCODE_V)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P [5]") PORT_CODE(KEYCODE_P)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J [8]") PORT_CODE(KEYCODE_J)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D [)]") PORT_CODE(KEYCODE_D)
INPUT_PORTS_END

static MACHINE_START(psion)
{
	if (!strcmp(machine->gamedrv->name, "psionlam"))
	{
		psion.rom_bank_count = 3;
		psion.ram_bank_count = 0;
	}
	else if (!strcmp(machine->gamedrv->name, "psionp350"))
	{
		psion.rom_bank_count = 0;
		psion.ram_bank_count = 5;
	}
	else if (!strncmp(machine->gamedrv->name, "psionlz", 7))
	{
		psion.rom_bank_count = 3;
		psion.ram_bank_count = 3;
	}
	else if (!strcmp(machine->gamedrv->name, "psionp464"))
	{
		psion.rom_bank_count = 3;
		psion.ram_bank_count = 9;
	}
	else
	{
		psion.rom_bank_count = 0;
		psion.ram_bank_count = 0;
	}

	if (psion.ram_bank_count)
		psion.ram = auto_alloc_array(machine, UINT8, psion.ram_bank_count * 0x4000);

	timer_pulse(machine, ATTOTIME_IN_SEC(1), NULL, 0, nmi_timer);
	timer_pulse(machine, ATTOTIME_IN_MSEC(500), NULL, 0, blink_timer);

	state_save_register_global_array(machine, hd44780.ddram);
	state_save_register_global_array(machine, hd44780.cgram);
}

static MACHINE_RESET(psion)
{
	psion.enable_nmi=0;
	psion.kb_counter=0;
	psion.ram_bank=0;
	psion.rom_bank=0;

	hd44780_reset(machine);

	if (psion.rom_bank_count || psion.ram_bank_count)
		update_bank(machine);
}

static VIDEO_START( psion )
{
}

static VIDEO_UPDATE( psion_2lines )
{
	char lcd_map[2][16];
	UINT8 cur_x = 0;
	UINT8 cur_y = 0;
	UINT8 display_layout[2] = {0x00, 0x40};

	memset(lcd_map, 0, 2*16);

	for (int i=0; i<2; i++)
	{
		if (hd44780.cursor_pos >= display_layout[i] && hd44780.cursor_pos < display_layout[i]+16)
		{
			cur_y = i+1;
			cur_x = hd44780.cursor_pos - display_layout[i];
		}

		memcpy(&lcd_map[i], hd44780.ddram + display_layout[i], 16);
	}

	bitmap_fill(bitmap, NULL, 0);

	if (hd44780.display_on)
	{
		for (int l=0; l<2; l++)
			for (int i=0; i<16; i++)
				for (int y=0; y<8; y++)
					for (int x=0; x<5; x++)
						if (lcd_map[l][i] <= 0x10)
						{
							//draw CGRAM characters
							*BITMAP_ADDR16(bitmap, l*9 + y, i*6 + x) = BIT(hd44780.cgram[(lcd_map[l][i]&0x07)*8+y], 4-x);
						}
						else
						{
							//draw CGROM characters
							UINT8 * gc_base = memory_region(screen->machine, "chargen");
							*BITMAP_ADDR16(bitmap, l*9 + y, i*6 + x) = BIT(gc_base[(lcd_map[l][i]-0x20)*8+y], 4-x);
						}

		//draw the cursor
		if (hd44780.cursor_on)
			for (int i=0; i<5; i++)
				*BITMAP_ADDR16(bitmap, cur_y * 9-2, cur_x*6 + i) = 1;

		if (!hd44780.blink && hd44780.blink_on)
			for (int l=0; l<7; l++)
				for (int i=0; i<5; i++)
					*BITMAP_ADDR16(bitmap, (cur_y-1) * 9 + l, cur_x*6 + i) = 1;
	}

#if(1)
	popmessage("cur: %u, pos %u, shift %u", hd44780.cursor_on, hd44780.ddram_a, hd44780.disp_shift);
#endif
    return 0;
}

static VIDEO_UPDATE( psion_4lines )
{
	UINT8 lcd_map[4][20];
	UINT8 cur_x = 0;
	UINT8 cur_y = 0;
	UINT8 line_pos = 0;
	UINT8 display_layout[4][3]=
	{
		{0x00, 0x08, 0x18},
		{0x40, 0x48, 0x58},
		{0x04, 0x10, 0x20},
		{0x44, 0x50, 0x60}
	};

	memset(lcd_map, 0, 4*20);

	for (int i=0; i<4; i++)
	{
		line_pos = 0;

		for (int j=0; j<3; j++)
		{
			if (hd44780.cursor_pos >= display_layout[i][j] && hd44780.cursor_pos < display_layout[i][j] + ((j==0)?4:8))
			{
				cur_y = i+1;
				cur_x = line_pos + hd44780.cursor_pos - display_layout[i][j];
			}

			memcpy(&lcd_map[i][line_pos], hd44780.ddram + display_layout[i][j], ((j==0)?4:8));

			line_pos += (j==0)?4:8;
		}
	}

	bitmap_fill(bitmap, NULL, 0);

	if (hd44780.display_on)
	{
		for (int l=0; l<4; l++)
			for (int i=0; i<20; i++)
				for (int y=0; y<8; y++)
					for (int x=0; x<5; x++)
						if (lcd_map[l][i] <= 0x10)
						{
							//draw CGRAM characters
							*BITMAP_ADDR16(bitmap, l*9 + y, i*6 + x) = BIT(hd44780.cgram[(lcd_map[l][i]&0x07)*8+y], 4-x);
						}
						else
						{
							//draw CGROM characters
							UINT8 * gc_base = memory_region(screen->machine, "chargen");
							*BITMAP_ADDR16(bitmap, l*9 + y, i*6 + x) = BIT(gc_base[(lcd_map[l][i]-0x20)*8+y], 4-x);
						}

		//draw the cursor
		if (hd44780.cursor_on)
			for (int i=0; i<5; i++)
				*BITMAP_ADDR16(bitmap, cur_y * 9-2, cur_x*6 + i) = 1;

		if (!hd44780.blink && hd44780.blink_on)
			for (int l=0; l<7; l++)
				for (int i=0; i<5; i++)
					*BITMAP_ADDR16(bitmap, (cur_y-1) * 9 + l, cur_x*6 + i) = 1;
	}

#if(1)
	popmessage("cur: %u, pos %u, shift %u", hd44780.cursor_on, hd44780.cursor_pos, hd44780.disp_shift);
#endif
    return 0;
}

static PALETTE_INIT( psion )
{
	palette_set_color(machine, 0, MAKE_RGB(138, 146, 148));
	palette_set_color(machine, 1, MAKE_RGB(92, 83, 88));
}

static const gfx_layout psion_charlayout =
{
	5, 8,					/* 5 x 8 characters */
	224,					/* 224 characters */
	1,						/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	{ 3, 4, 5, 6, 7},
	{ 0, 8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8},
	8*8						/* 8 bytes */
};

static GFXDECODE_START( psion )
	GFXDECODE_ENTRY( "chargen", 0x0000, psion_charlayout, 0, 1 )
GFXDECODE_END

/* basic configuration for 2 lines display */
static MACHINE_DRIVER_START( psion_2lines )
	/* basic machine hardware */
    MDRV_CPU_ADD("maincpu",HD63701, 980000) // should be HD6303 at 0.98MHz

	MDRV_MACHINE_START(psion)
    MDRV_MACHINE_RESET(psion)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(6*16, 9*2)
	MDRV_SCREEN_VISIBLE_AREA(0, 6*16-1, 0, 9*2-1)
	MDRV_DEFAULT_LAYOUT(layout_lcd)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(psion)
	MDRV_GFXDECODE(psion)

    MDRV_VIDEO_START(psion)
    MDRV_VIDEO_UPDATE(psion_2lines)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO( "mono" )
	MDRV_SOUND_ADD( "beep", BEEP, 0 )
	MDRV_SOUND_ROUTE( ALL_OUTPUTS, "mono", 1.00 )

MACHINE_DRIVER_END

/* basic configuration for 4 lines display */
static MACHINE_DRIVER_START( psion_4lines )
	/* basic machine hardware */
    MDRV_CPU_ADD("maincpu",HD63701, 980000) // should be HD6303 at 0.98MHz

	MDRV_MACHINE_START(psion)
    MDRV_MACHINE_RESET(psion)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(6*20, 9*4)
	MDRV_SCREEN_VISIBLE_AREA(0, 6*20-1, 0, 9*4-1)
	MDRV_DEFAULT_LAYOUT(layout_lcd)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(psion)
	MDRV_GFXDECODE(psion)

    MDRV_VIDEO_START(psion)
    MDRV_VIDEO_UPDATE(psion_4lines)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO( "mono" )
	MDRV_SOUND_ADD( "beep", BEEP, 0 )
	MDRV_SOUND_ROUTE( ALL_OUTPUTS, "mono", 1.00 )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( psioncm )
	MDRV_IMPORT_FROM( psion_2lines )

	MDRV_CPU_MODIFY("maincpu")
	MDRV_CPU_PROGRAM_MAP(psioncm_mem)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( psionla )
	MDRV_IMPORT_FROM( psion_2lines )

	MDRV_CPU_MODIFY("maincpu")
	MDRV_CPU_PROGRAM_MAP(psionla_mem)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( psionlam )
	MDRV_IMPORT_FROM( psion_2lines )

	MDRV_CPU_MODIFY("maincpu")
	MDRV_CPU_PROGRAM_MAP(psionlam_mem)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( psionp350 )
	MDRV_IMPORT_FROM( psion_2lines )

	MDRV_CPU_MODIFY("maincpu")
	MDRV_CPU_PROGRAM_MAP(psionp350_mem)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( psionlz )
	MDRV_IMPORT_FROM( psion_4lines )

	MDRV_CPU_MODIFY("maincpu")
	MDRV_CPU_PROGRAM_MAP(psionlz_mem)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( psioncm )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "24-cm.dat",    0x8000, 0x8000,  CRC(f6798394) SHA1(736997f0db9a9ee50d6785636bdc3f8ff1c33c66))

    ROM_REGION( 0x00700, "chargen", ROMREGION_ERASE )
	ROM_LOAD( "chargen.bin",    0x0000, 0x0700,  BAD_DUMP CRC(4c64fc7d) SHA1(123a87b2dc7efd7afcd8e27eb9bc74f54a8f1bc9))
ROM_END

ROM_START( psionla )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "33-la.dat",    0x8000, 0x8000,  CRC(02668ed4) SHA1(e5d4ee6b1cde310a2970ffcc6f29a0ce09b08c46))

	ROM_REGION( 0x00700, "chargen", ROMREGION_ERASE )
	ROM_LOAD( "chargen.bin",    0x0000, 0x0700,  BAD_DUMP CRC(4c64fc7d) SHA1(123a87b2dc7efd7afcd8e27eb9bc74f54a8f1bc9))
ROM_END

ROM_START( psionp350 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "36-p350.dat",  0x8000, 0x8000,  CRC(3a371a74) SHA1(9167210b2c0c3bd196afc08ca44ab23e4e62635e))
	ROM_LOAD( "38-p350.dat",  0x8000, 0x8000,  CRC(1b8b082f) SHA1(a3e875a59860e344f304a831148a7980f28eaa4a))

	ROM_REGION( 0x00700, "chargen", ROMREGION_ERASE )
	ROM_LOAD( "chargen.bin",    0x0000, 0x0700,  BAD_DUMP CRC(4c64fc7d) SHA1(123a87b2dc7efd7afcd8e27eb9bc74f54a8f1bc9))
ROM_END

ROM_START( psionlam )
    ROM_REGION( 0x18000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "37-lam.dat",   0x8000, 0x10000, CRC(7ee3a1bc) SHA1(c7fbd6c8e47c9b7d5f636e9f56e911b363d6796b))

	ROM_REGION( 0x00700, "chargen", ROMREGION_ERASE )
	ROM_LOAD( "chargen.bin",    0x0000, 0x0700,  BAD_DUMP CRC(4c64fc7d) SHA1(123a87b2dc7efd7afcd8e27eb9bc74f54a8f1bc9))
ROM_END

ROM_START( psionlz64 )
    ROM_REGION( 0x18000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "44-lz64.dat",  0x8000, 0x10000, CRC(aa487913) SHA1(5a44390f63fc8c1bc94299ab2eb291bc3a5b989a))

	ROM_REGION( 0x00700, "chargen", ROMREGION_ERASE )
	ROM_LOAD( "chargen.bin",    0x0000, 0x0700,  BAD_DUMP CRC(4c64fc7d) SHA1(123a87b2dc7efd7afcd8e27eb9bc74f54a8f1bc9))
ROM_END

ROM_START( psionlz64s )
    ROM_REGION( 0x18000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "46-lz64s.dat", 0x8000, 0x10000, CRC(328d9772) SHA1(7f9e2d591d59ecfb0822d7067c2fe59542ea16dd))

	ROM_REGION( 0x00700, "chargen", ROMREGION_ERASE )
	ROM_LOAD( "chargen.bin",    0x0000, 0x0700,  BAD_DUMP CRC(4c64fc7d) SHA1(123a87b2dc7efd7afcd8e27eb9bc74f54a8f1bc9))
ROM_END

ROM_START( psionlz )
    ROM_REGION( 0x18000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "46-lz.dat",    0x8000, 0x10000, CRC(22715f48) SHA1(cf460c81cadb53eddb7afd8dadecbe8c38ea3fc2))

	ROM_REGION( 0x00700, "chargen", ROMREGION_ERASE )
	ROM_LOAD( "chargen.bin",    0x0000, 0x0700,  BAD_DUMP CRC(4c64fc7d) SHA1(123a87b2dc7efd7afcd8e27eb9bc74f54a8f1bc9))
ROM_END

ROM_START( psionp464 )
    ROM_REGION( 0x18000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "46-p464.dat",  0x8000, 0x10000, CRC(672a0945) SHA1(d2a6e3fe1019d1bd7ae4725e33a0b9973f8cd7d8))

	ROM_REGION( 0x00700, "chargen", ROMREGION_ERASE )
	ROM_LOAD( "chargen.bin",    0x0000, 0x0700,  BAD_DUMP CRC(4c64fc7d) SHA1(123a87b2dc7efd7afcd8e27eb9bc74f54a8f1bc9))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT COMPANY   FULLNAME       FLAGS */
COMP( 1986, psioncm,	0,       0, 	psioncm, 		psion, 	 0,   "Psion",   "Organiser II CM",		GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 198?, psionla,	psioncm, 0, 	psionla, 	    psion, 	 0,   "Psion",   "Organiser II LA",		GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 198?, psionp350,	psioncm, 0, 	psionp350, 	    psion, 	 0,   "Psion",   "Organiser II P350",	GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 198?, psionlam,	psioncm, 0, 	psionlam, 	    psion, 	 0,   "Psion",   "Organiser II LAM",	GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 1989, psionlz64,  psioncm, 0, 	psionlz, 	    psion, 	 0,   "Psion",   "Organiser II LZ64",	GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 1989, psionlz64s,	psioncm, 0, 	psionlz, 	    psion, 	 0,   "Psion",   "Organiser II LZ64S",	GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 1989, psionlz,	psioncm, 0, 	psionlz, 	    psion, 	 0,   "Psion",   "Organiser II LZ",		GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 198?, psionp464,	psioncm, 0, 	psionlz, 	    psion, 	 0,   "Psion",   "Organiser II P464",	GAME_NOT_WORKING | GAME_NO_SOUND)
