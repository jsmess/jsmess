/***************************************************************************

        Psion Organiser II series

        Driver by Sandro Ronco

        TODO:
        - add save state
        - dump CGROM of the HD44780
		- emulate other devices in slot C(Comms Link, printer)

        Note:
        - 4 lines display has an custom LCD controller derived from an HD66780
        - NVRAM works only if the machine is turned off (with OFF menu) before closing MESS

        16/07/2010 Skeleton driver.

        Memory map info :
            http://archive.psion2.org/org2/techref/memory.htm

****************************************************************************/

#include "emu.h"
#include "cpu/m6800/m6800.h"
#include "sound/beep.h"
#include "devices/cartslot.h"
#include "video/hd44780.h"

struct datapack
{
	device_image_interface *image;
	UINT8 id;
	UINT8 len;
	UINT16 counter;
	UINT8 page;
	UINT8 segment;
	UINT8 control_lines;
	UINT8* data;

	bool write_flag;
};

class psion_state : public driver_device
{
public:
	psion_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT16 kb_counter;
	UINT8 enable_nmi;
	UINT8 *sys_register;
	UINT8 tcsr_value;
	UINT8 stby_pwr;
	UINT8 pulse;

	/* RAM/ROM banks */
	UINT8 *ram;
	size_t ram_size;
	UINT8 *paged_ram;
	UINT8 rom_bank;
	UINT8 ram_bank;
	UINT8 ram_bank_count;
	UINT8 rom_bank_count;

	UINT8 port2_ddr;
	UINT8 port2;
	UINT8 port6_ddr;
	UINT8 port6;

	datapack pack1;
	datapack pack2;

	// devices
	hd44780_device *hd44780;
	device_t *beep;
};


/***********************************************

    Datapack

***********************************************/

static UINT8 datapack_update(datapack *pack, UINT8 data)
{
	assert(pack != NULL);

	if (pack->len == 0 || (pack->control_lines & 0x80))
		return 0;

	UINT32 pack_addr = pack->counter + ((pack->id & 0x04) ? 0x100 * pack->page : 0);

	// if the datapack is 128k or more is treated as segmented
	if (pack->len >= 0x10)
		pack_addr += (pack->segment * 0x4000);

	if (pack_addr < pack->len * 0x2000)
	{
		if ((pack->control_lines&0x08) && !(pack->control_lines & 0x02))
		{
			//write latched data
			pack->data[pack_addr] = data;
			pack->write_flag = true;
		}
		else if ((pack->control_lines&0x08) && (pack->control_lines & 0x02))
		{
			//write datapack segment
			if (pack->len <= 0x10)
				pack->segment = data & 0x07;
			else if (pack->len <= 0x20)
				pack->segment = data & 0x0f;
			else if (pack->len <= 0x40)
				pack->segment = data & 0x1f;
			else if (pack->len <= 0x80)
				pack->segment = data & 0x3f;
			else
				pack->segment = data;
		}
		else if (!(pack->control_lines&0x08) && !(pack->control_lines & 0x02))
		{
			//latch data
			data = pack->data[pack_addr];
		}
		else if (!(pack->control_lines&0x08) && (pack->control_lines & 0x02))
		{
			//read datapack ID
			if (pack->id & 0x02)
				data = pack->data[0];
			else
				data = 1;
		}
	}

	return data;
}

static void datapack_control_lines_w(datapack *pack, UINT8 data)
{
	assert(pack != NULL);

	if ((pack->control_lines & 0x01) != (data & 0x01))
	{
		pack->counter++;

		if (pack->counter >= ((pack->id & 0x04) ? 0x100 : (pack->len * 0x2000)))
			pack->counter = 0;
	}

	if ((pack->control_lines & 0x04) && !(data & 0x04))
	{
		pack->page++;

		if (pack->page >= (pack->len * 0x2000) / 0x100)
			pack->page = 0;
	}

	if (data & 0x02)
	{
		pack->counter = 0;
		pack->page = 0;
	}

	pack->control_lines = data;
}

static UINT8 datapack_control_lines_r(datapack *pack)
{
	assert(pack != NULL);

	return pack->control_lines;
}

static int datapack_load(device_image_interface &image, datapack &pack)
{
	running_machine *machine = image.device().machine;
	char opk_head[6];
	UINT8 pack_id, pack_len;

	image.fread(opk_head, 6);
	image.fread(&pack_id, 1);
	image.fread(&pack_len, 1);

	if(strcmp(opk_head, "OPK") || pack_len * 0x2000 < image.length() - 6)
		return IMAGE_INIT_FAIL;

	pack.image = &image;
	pack.write_flag = false;
	pack.id = pack_id;
	pack.len = pack_len;
	image.fseek(6, SEEK_SET);
	pack.data = auto_alloc_array(machine, UINT8, pack.len * 0x2000);
	memset(pack.data, 0xff, pack.len * 0x2000);
	image.fread(pack.data, image.length() - 6);

	/* load battery */
	UINT8 *opk = auto_alloc_array(machine, UINT8, pack.len * 0x2000 + 6);
	image.battery_load(opk, pack.len * 0x2000 + 6, 0xff);
	if(!strncmp((char*)opk, "OPK", 3))
		memcpy(pack.data, opk + 6, pack.len * 0x2000);

	return IMAGE_INIT_PASS;
}

static void datapack_unload(device_image_interface &image, datapack &pack)
{
	running_machine *machine = image.device().machine;

	// save battery only if a write-access occurs
	if (pack.len > 0 && pack.write_flag)
	{
		UINT8 opk_magic[6] = {'O', 'P', 'K', 0x00, 0x00, 0x00};
		UINT8 *opk = auto_alloc_array(machine, UINT8, pack.len * 0x2000 + 6);

		memcpy(opk, opk_magic, 6);
		memcpy(opk + 6, pack.data, pack.len * 0x2000);

		pack.image->battery_save(opk, pack.len * 0x2000 + 6);

		auto_free(machine, opk);
	}

	auto_free(machine, pack.data);
	pack.len = 0;
}

/***********************************************

    basic machine

***********************************************/

static TIMER_CALLBACK( nmi_timer )
{
	psion_state *state = machine->driver_data<psion_state>();

	if (state->enable_nmi)
		cputag_set_input_line(machine, "maincpu", INPUT_LINE_NMI, PULSE_LINE);
}

static UINT8 kb_read(running_machine *machine)
{
	psion_state *state = machine->driver_data<psion_state>();
	static const char *const bitnames[] = {"K1", "K2", "K3", "K4", "K5", "K6", "K7"};
	UINT8 line, data = 0x7c;

	if (state->kb_counter)
	{
		for (line = 0; line < 7; line++)
			if (state->kb_counter == (0x7f & ~(1 << line)))
				data = input_port_read(machine, bitnames[line]);
	}
	else
	{
		for (line = 0; line < 7; line++)
			data &= input_port_read(machine, bitnames[line]);
	}

	return data & 0x7c;
}

static void update_bank(running_machine *machine)
{
	psion_state *state = machine->driver_data<psion_state>();

	if (state->ram_bank < state->ram_bank_count && state->ram_bank_count)
		memory_set_bankptr(machine,"rambank", state->paged_ram + state->ram_bank*0x4000);

	if (state->rom_bank_count)
	{
		if (state->rom_bank==0)
			memory_set_bankptr(machine,"rombank", memory_region(machine, "maincpu") + 0x8000);
		else if (state->rom_bank < state->rom_bank_count)
			memory_set_bankptr(machine,"rombank", memory_region(machine, "maincpu") + state->rom_bank*0x4000 + 0xc000);
	}
}

static struct datapack *get_active_slot(running_machine *machine, UINT8 data)
{
	psion_state *state = machine->driver_data<psion_state>();

	switch(data & 0x70)
	{
		case 0x60:
			return &state->pack1;
		case 0x50:
			return &state->pack2;
	}

	return NULL;
}

static WRITE8_HANDLER( hd63701_int_reg_w )
{
	psion_state *state = space->machine->driver_data<psion_state>();

	switch (offset)
	{
	case 0x01:
		state->port2_ddr = data;
		break;
	case 0x03:
		/* datapack i/o data bus */
		if (state->port2_ddr == 0xff)
		{
			datapack *pack = get_active_slot(space->machine, state->port6);

			if (pack)
				state->port2 = datapack_update(pack, data);
			else
				state->port2 = data;
		}
		break;
	case 0x08:
		state->tcsr_value = data;
		break;
	case 0x15:
		/* read-only */
		break;
	case 0x16:
		state->port6_ddr = data;
		break;
	case 0x17:
		/*
        datapack control lines
        x--- ---- slot on/off
        -x-- ---- slot 3
        --x- ---- slot 2
        ---x ---- slot 1
        ---- x--- output enable
        ---- -x-- program line
        ---- --x- reset line
        ---- ---x clock line
        */
		{
			state->port6 = (data & state->port6_ddr) | (state->port6 & ~state->port6_ddr);
			datapack *pack = get_active_slot(space->machine, state->port6);

			if (pack != NULL)
			{
				datapack_control_lines_w(pack, state->port6);
				datapack_update(pack, state->port2);
			}
		}
		break;
	}

	hd63701_internal_registers_w(space, offset, data);
}

static READ8_HANDLER( hd63701_int_reg_r )
{
	psion_state *state = space->machine->driver_data<psion_state>();

	switch (offset)
	{
	case 0x03:
		/* datapack i/o data bus */
		if (state->port2_ddr == 0)
		{
			datapack *pack = get_active_slot(space->machine, state->port6);

			if (pack)
				state->port2 = datapack_update(pack, state->port2);
			else
				state->port2 = 0;

			return state->port2;
		}
		else
			return 0;
	case 0x14:
		return (hd63701_internal_registers_r(space, offset)&0x7f) | (state->stby_pwr<<7);
	case 0x15:
		/*
        x--- ---- ON key active high
        -xxx xx-- keys matrix active low
        ---- --x- pulse
        ---- ---x battery status
        */
		return kb_read(space->machine) | input_port_read(space->machine, "BATTERY") | input_port_read(space->machine, "ON") | (state->kb_counter == 0x7ff)<<1 | state->pulse<<1;
	case 0x17:
		/* datapack control lines */
		{
			datapack *pack = get_active_slot(space->machine, state->port6);

			if (pack)
				return datapack_control_lines_r(pack);
			else
				return state->port6;
		}
	case 0x08:
		hd63701_internal_registers_w(space, offset, state->tcsr_value);
	default:
		return hd63701_internal_registers_r(space, offset);
	}
}

/* Read/Write common */
static void io_rw(address_space* space, UINT16 offset)
{
	psion_state *state = space->machine->driver_data<psion_state>();

	if (space->debugger_access())
		return;

	switch (offset & 0xffc0)
	{
	case 0xc0:
		/* switch off, CPU goes into standby mode */
		state->enable_nmi = 0;
		state->stby_pwr = 1;
		space->machine->device<cpu_device>("maincpu")->suspend(SUSPEND_REASON_HALT, 1);
		break;
	case 0x100:
		state->pulse = 1;
		break;
	case 0x140:
		state->pulse = 0;
		break;
	case 0x200:
		state->kb_counter = 0;
		break;
	case 0x180:
		beep_set_state(state->beep, 1);
		break;
	case 0x1c0:
		beep_set_state(state->beep, 0);
		break;
	case 0x240:
		if (offset == 0x260 && (state->rom_bank_count || state->ram_bank_count))
		{
			state->ram_bank=0;
			state->rom_bank=0;
			update_bank(space->machine);
		}
		else
			state->kb_counter++;
		break;
	case 0x280:
		if (offset == 0x2a0 && state->ram_bank_count)
		{
			state->ram_bank++;
			update_bank(space->machine);
		}
		else
			state->enable_nmi = 1;
		break;
	case 0x2c0:
		if (offset == 0x2e0 && state->rom_bank_count)
		{
			state->rom_bank++;
			update_bank(space->machine);
		}
		else
			state->enable_nmi = 0;
		break;
	}
}

static WRITE8_HANDLER( io_w )
{
	psion_state *state = space->machine->driver_data<psion_state>();

	switch (offset & 0x0ffc0)
	{
	case 0x80:
		if (offset & 1)
			state->hd44780->data_write(offset, data);
		else
			state->hd44780->control_write(offset, data);
		break;
	default:
		io_rw(space, offset);
	}
}

static READ8_HANDLER( io_r )
{
		psion_state *state = space->machine->driver_data<psion_state>();

	switch (offset & 0xffc0)
	{
	case 0x80:
		if (offset& 1)
			return state->hd44780->data_read(offset);
		else
			return state->hd44780->control_read(offset);
	default:
		io_rw(space, offset);
	}

	return 0;
}

static INPUT_CHANGED( psion_on )
{
	cpu_device *cpu = field->port->machine->device<cpu_device>("maincpu");

	/* reset the CPU for resume from standby */
	if (cpu->suspended(SUSPEND_REASON_HALT))
		cpu->reset();
}

static ADDRESS_MAP_START(psioncm_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_LOW
	AM_RANGE(0x0000, 0x001f) AM_READWRITE(hd63701_int_reg_r, hd63701_int_reg_w)
	AM_RANGE(0x0040, 0x00ff) AM_RAM AM_BASE_MEMBER(psion_state, sys_register)
	AM_RANGE(0x0100, 0x03ff) AM_READWRITE(io_r, io_w)
	AM_RANGE(0x2000, 0x3fff) AM_RAM AM_BASE_SIZE_MEMBER(psion_state, ram, ram_size)
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(psionla_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_LOW
	AM_RANGE(0x0000, 0x001f) AM_READWRITE(hd63701_int_reg_r, hd63701_int_reg_w)
	AM_RANGE(0x0040, 0x00ff) AM_RAM AM_BASE_MEMBER(psion_state, sys_register)
	AM_RANGE(0x0100, 0x03ff) AM_READWRITE(io_r, io_w)
	AM_RANGE(0x0400, 0x5fff) AM_RAM AM_BASE_SIZE_MEMBER(psion_state, ram, ram_size)
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(psionp350_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_LOW
	AM_RANGE(0x0000, 0x001f) AM_READWRITE(hd63701_int_reg_r, hd63701_int_reg_w)
	AM_RANGE(0x0040, 0x00ff) AM_RAM AM_BASE_MEMBER(psion_state, sys_register)
	AM_RANGE(0x0100, 0x03ff) AM_READWRITE(io_r, io_w)
	AM_RANGE(0x0400, 0x3fff) AM_RAM AM_BASE_SIZE_MEMBER(psion_state, ram, ram_size)
	AM_RANGE(0x4000, 0x7fff) AM_RAMBANK("rambank")
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(psionlam_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_LOW
	AM_RANGE(0x0000, 0x001f) AM_READWRITE(hd63701_int_reg_r, hd63701_int_reg_w)
	AM_RANGE(0x0040, 0x00ff) AM_RAM AM_BASE_MEMBER(psion_state, sys_register)
	AM_RANGE(0x0100, 0x03ff) AM_READWRITE(io_r, io_w)
	AM_RANGE(0x0400, 0x7fff) AM_RAM AM_BASE_SIZE_MEMBER(psion_state, ram, ram_size)
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK("rombank")
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(psionlz_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_LOW
	AM_RANGE(0x0000, 0x001f) AM_READWRITE(hd63701_int_reg_r, hd63701_int_reg_w)
	AM_RANGE(0x0040, 0x00ff) AM_RAM AM_BASE_MEMBER(psion_state, sys_register)
	AM_RANGE(0x0100, 0x03ff) AM_READWRITE(io_r, io_w)
	AM_RANGE(0x0400, 0x3fff) AM_RAM AM_BASE_SIZE_MEMBER(psion_state, ram, ram_size)
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
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ON/CLEAR") PORT_CODE(KEYCODE_F10)  PORT_CHANGED(psion_on, 0)

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

static DEVICE_IMAGE_LOAD( psion_pack1 )
{
	psion_state *state = image.device().machine->driver_data<psion_state>();

	return datapack_load(image, state->pack1);
}

static DEVICE_IMAGE_UNLOAD( psion_pack1 )
{
	psion_state *state = image.device().machine->driver_data<psion_state>();

	datapack_unload(image, state->pack1);
}

static DEVICE_IMAGE_LOAD( psion_pack2 )
{
	psion_state *state = image.device().machine->driver_data<psion_state>();

	return datapack_load(image, state->pack2);
}

static DEVICE_IMAGE_UNLOAD( psion_pack2 )
{
	psion_state *state = image.device().machine->driver_data<psion_state>();

	datapack_unload(image, state->pack2);
}

static NVRAM_HANDLER( psion )
{
	psion_state *state = machine->driver_data<psion_state>();

	if (read_or_write)
	{
		mame_fwrite(file, state->sys_register, 0xc0);
		mame_fwrite(file, state->ram, state->ram_size);
		if (state->ram_bank_count)
			mame_fwrite(file, state->paged_ram, state->ram_bank_count * 0x4000);
	}
	else
	{
		if (file)
		{
			mame_fread(file, state->sys_register, 0xc0);
			mame_fread(file, state->ram, state->ram_size);
			if (state->ram_bank_count)
				mame_fread(file, state->paged_ram, state->ram_bank_count * 0x4000);

			state->stby_pwr = 1;
		}
		else
			state->stby_pwr = 0;
	}
}

static MACHINE_START(psion)
{
	psion_state *state = machine->driver_data<psion_state>();

	if (!strcmp(machine->gamedrv->name, "psionlam"))
	{
		state->rom_bank_count = 3;
		state->ram_bank_count = 0;
	}
	else if (!strcmp(machine->gamedrv->name, "psionp350"))
	{
		state->rom_bank_count = 0;
		state->ram_bank_count = 5;
	}
	else if (!strncmp(machine->gamedrv->name, "psionlz", 7))
	{
		state->rom_bank_count = 3;
		state->ram_bank_count = 3;
	}
	else if (!strcmp(machine->gamedrv->name, "psionp464"))
	{
		state->rom_bank_count = 3;
		state->ram_bank_count = 9;
	}
	else
	{
		state->rom_bank_count = 0;
		state->ram_bank_count = 0;
	}

	if (state->ram_bank_count)
		state->paged_ram = auto_alloc_array(machine, UINT8, state->ram_bank_count * 0x4000);

	timer_pulse(machine, ATTOTIME_IN_SEC(1), NULL, 0, nmi_timer);

	state->hd44780 = machine->device<hd44780_device>("hd44780");
	state->beep = machine->device<device_t>("beep");
}

static MACHINE_RESET(psion)
{
	psion_state *state = machine->driver_data<psion_state>();

	state->enable_nmi=0;
	state->kb_counter=0;
	state->ram_bank=0;
	state->rom_bank=0;
	state->pulse=0;

	if (state->rom_bank_count || state->ram_bank_count)
		update_bank(machine);
}

static VIDEO_START( psion )
{
}

static VIDEO_UPDATE( psion_2lines )
{
	psion_state *state = screen->machine->driver_data<psion_state>();

	return state->hd44780->video_update( bitmap, cliprect );
}

static VIDEO_UPDATE( psion_4lines )
{
	psion_state *state = screen->machine->driver_data<psion_state>();

	return state->hd44780->video_update( bitmap, cliprect );
}

static PALETTE_INIT( psion )
{
	palette_set_color(machine, 0, MAKE_RGB(138, 146, 148));
	palette_set_color(machine, 1, MAKE_RGB(92, 83, 88));
}

static const gfx_layout psion_charlayout =
{
	5, 8,					/* 5 x 8 characters */
	256,					/* 256 characters */
	1,						/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	{ 3, 4, 5, 6, 7},
	{ 0, 8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8},
	8*8						/* 8 bytes */
};

static GFXDECODE_START( psion )
	GFXDECODE_ENTRY( "hd44780", 0x0000, psion_charlayout, 0, 1 )
GFXDECODE_END


static MACHINE_CONFIG_FRAGMENT( psion_slot )
	/* Datapack slot 1 */
	MDRV_CARTSLOT_ADD("pack1")
	MDRV_CARTSLOT_EXTENSION_LIST("opk")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_LOAD(psion_pack1)
	MDRV_CARTSLOT_UNLOAD(psion_pack1)

	/* Datapack slot 2 */
	MDRV_CARTSLOT_ADD("pack2")
	MDRV_CARTSLOT_EXTENSION_LIST("opk")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_LOAD(psion_pack2)
	MDRV_CARTSLOT_UNLOAD(psion_pack2)
MACHINE_CONFIG_END

static const hd44780_interface psion_2line_display =
{
	2,					// number of lines
	16,					// chars for line
	NULL				// custom display layout
};

/* basic configuration for 2 lines display */
static MACHINE_CONFIG_START( psion_2lines, psion_state )
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

	MDRV_HD44780_ADD("hd44780", psion_2line_display)

    MDRV_VIDEO_START(psion)
    MDRV_VIDEO_UPDATE(psion_2lines)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO( "mono" )
	MDRV_SOUND_ADD( "beep", BEEP, 0 )
	MDRV_SOUND_ROUTE( ALL_OUTPUTS, "mono", 1.00 )

	MDRV_NVRAM_HANDLER(psion)

	MDRV_FRAGMENT_ADD( psion_slot )
MACHINE_CONFIG_END

UINT8 psion_4line_layout[] =
{
	0x00, 0x01, 0x02, 0x03, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	0x40, 0x41, 0x42, 0x43, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
	0x04, 0x05, 0x06, 0x07, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x44, 0x45, 0x46, 0x47, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67
};

static const hd44780_interface psion_4line_display =
{
	4,					// number of lines
	20,					// chars for line
	psion_4line_layout	// custom display layout
};

/* basic configuration for 4 lines display */
static MACHINE_CONFIG_START( psion_4lines, psion_state )
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

	MDRV_HD44780_ADD("hd44780", psion_4line_display)

    MDRV_VIDEO_START(psion)
    MDRV_VIDEO_UPDATE(psion_4lines)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO( "mono" )
	MDRV_SOUND_ADD( "beep", BEEP, 0 )
	MDRV_SOUND_ROUTE( ALL_OUTPUTS, "mono", 1.00 )

	MDRV_NVRAM_HANDLER(psion)

	MDRV_FRAGMENT_ADD( psion_slot )
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( psioncm, psion_2lines )

	MDRV_CPU_MODIFY("maincpu")
	MDRV_CPU_PROGRAM_MAP(psioncm_mem)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( psionla, psion_2lines )

	MDRV_CPU_MODIFY("maincpu")
	MDRV_CPU_PROGRAM_MAP(psionla_mem)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( psionlam, psion_2lines )

	MDRV_CPU_MODIFY("maincpu")
	MDRV_CPU_PROGRAM_MAP(psionlam_mem)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( psionp350, psion_2lines )

	MDRV_CPU_MODIFY("maincpu")
	MDRV_CPU_PROGRAM_MAP(psionp350_mem)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( psionlz, psion_4lines )

	MDRV_CPU_MODIFY("maincpu")
	MDRV_CPU_PROGRAM_MAP(psionlz_mem)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( psioncm )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "v24", "CM v2.4")
	ROMX_LOAD( "24-cm.dat",    0x8000, 0x8000,  CRC(f6798394) SHA1(736997f0db9a9ee50d6785636bdc3f8ff1c33c66), ROM_BIOS(1))

    ROM_REGION( 0x00800, "hd44780", ROMREGION_ERASE )
	ROM_LOAD( "chargen.bin",    0x0100, 0x0700,  BAD_DUMP CRC(4c64fc7d) SHA1(123a87b2dc7efd7afcd8e27eb9bc74f54a8f1bc9))
ROM_END

ROM_START( psionla )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "v33", "LA v3.3")
	ROMX_LOAD( "33-la.dat",    0x8000, 0x8000,  CRC(02668ed4) SHA1(e5d4ee6b1cde310a2970ffcc6f29a0ce09b08c46), ROM_BIOS(1))

	ROM_REGION( 0x00800, "hd44780", ROMREGION_ERASE )
	ROM_LOAD( "chargen.bin",    0x0100, 0x0700,  BAD_DUMP CRC(4c64fc7d) SHA1(123a87b2dc7efd7afcd8e27eb9bc74f54a8f1bc9))
ROM_END

ROM_START( psionp350 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "v36", "POS350 v3.6")
	ROMX_LOAD( "36-p350.dat",  0x8000, 0x8000,  CRC(3a371a74) SHA1(9167210b2c0c3bd196afc08ca44ab23e4e62635e), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "v38", "POS350 v3.8")
	ROMX_LOAD( "38-p350.dat",  0x8000, 0x8000,  CRC(1b8b082f) SHA1(a3e875a59860e344f304a831148a7980f28eaa4a), ROM_BIOS(2))

	ROM_REGION( 0x00800, "hd44780", ROMREGION_ERASE )
	ROM_LOAD( "chargen.bin",    0x0100, 0x0700,  BAD_DUMP CRC(4c64fc7d) SHA1(123a87b2dc7efd7afcd8e27eb9bc74f54a8f1bc9))
ROM_END

ROM_START( psionlam )
    ROM_REGION( 0x18000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "v37", "LA v3.7")
	ROMX_LOAD( "37-lam.dat",   0x8000, 0x10000, CRC(7ee3a1bc) SHA1(c7fbd6c8e47c9b7d5f636e9f56e911b363d6796b), ROM_BIOS(1))

	ROM_REGION( 0x00800, "hd44780", ROMREGION_ERASE )
	ROM_LOAD( "chargen.bin",    0x0100, 0x0700,  BAD_DUMP CRC(4c64fc7d) SHA1(123a87b2dc7efd7afcd8e27eb9bc74f54a8f1bc9))
ROM_END

ROM_START( psionlz64 )
    ROM_REGION( 0x18000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "v44", "LZ64 v4.4")
	ROMX_LOAD( "44-lz64.dat",  0x8000, 0x10000, CRC(aa487913) SHA1(5a44390f63fc8c1bc94299ab2eb291bc3a5b989a), ROM_BIOS(1))

	ROM_REGION( 0x00800, "hd44780", ROMREGION_ERASE )
	ROM_LOAD( "chargen.bin",    0x0100, 0x0700,  BAD_DUMP CRC(4c64fc7d) SHA1(123a87b2dc7efd7afcd8e27eb9bc74f54a8f1bc9))
ROM_END

ROM_START( psionlz64s )
    ROM_REGION( 0x18000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "v46", "LZ64 v4.6")
	ROMX_LOAD( "46-lz64s.dat", 0x8000, 0x10000, CRC(328d9772) SHA1(7f9e2d591d59ecfb0822d7067c2fe59542ea16dd), ROM_BIOS(1))

	ROM_REGION( 0x00800, "hd44780", ROMREGION_ERASE )
	ROM_LOAD( "chargen.bin",    0x0100, 0x0700,  BAD_DUMP CRC(4c64fc7d) SHA1(123a87b2dc7efd7afcd8e27eb9bc74f54a8f1bc9))
ROM_END

ROM_START( psionlz )
    ROM_REGION( 0x18000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "v46", "LZ v4.6")
	ROMX_LOAD( "46-lz.dat",    0x8000, 0x10000, CRC(22715f48) SHA1(cf460c81cadb53eddb7afd8dadecbe8c38ea3fc2), ROM_BIOS(1))

	ROM_REGION( 0x00800, "hd44780", ROMREGION_ERASE )
	ROM_LOAD( "chargen.bin",    0x0100, 0x0700,  BAD_DUMP CRC(4c64fc7d) SHA1(123a87b2dc7efd7afcd8e27eb9bc74f54a8f1bc9))
ROM_END

ROM_START( psionp464 )
    ROM_REGION( 0x18000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "v46", "POS464 v4.6")
	ROMX_LOAD( "46-p464.dat",  0x8000, 0x10000, CRC(672a0945) SHA1(d2a6e3fe1019d1bd7ae4725e33a0b9973f8cd7d8), ROM_BIOS(1))

	ROM_REGION( 0x00800, "hd44780", ROMREGION_ERASE )
	ROM_LOAD( "chargen.bin",    0x0100, 0x0700,  BAD_DUMP CRC(4c64fc7d) SHA1(123a87b2dc7efd7afcd8e27eb9bc74f54a8f1bc9))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT COMPANY   FULLNAME       FLAGS */
COMP( 1986, psioncm,	0,       0, 	psioncm,		psion,	 0,   "Psion",   "Organiser II CM",		0)
COMP( 198?, psionla,	psioncm, 0, 	psionla,	    psion,	 0,   "Psion",   "Organiser II LA",		0)
COMP( 198?, psionp350,	psioncm, 0, 	psionp350,	    psion,	 0,   "Psion",   "Organiser II P350",	0)
COMP( 198?, psionlam,	psioncm, 0, 	psionlam,	    psion,	 0,   "Psion",   "Organiser II LAM",	0)
COMP( 1989, psionlz64,  psioncm, 0, 	psionlz,	    psion,	 0,   "Psion",   "Organiser II LZ64",	0)
COMP( 1989, psionlz64s,	psioncm, 0, 	psionlz,	    psion,	 0,   "Psion",   "Organiser II LZ64S",	0)
COMP( 1989, psionlz,	psioncm, 0, 	psionlz,	    psion,	 0,   "Psion",   "Organiser II LZ",		0)
COMP( 198?, psionp464,	psioncm, 0, 	psionlz,	    psion,	 0,   "Psion",   "Organiser II P464",	0)
