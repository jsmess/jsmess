/***************************************************************************

        Psion Organiser II series

        Driver by Sandro Ronco

        TODO:
        - move datapack into device
        - dump CGROM of the HD44780
        - emulate other devices in slot C(Comms Link, printer)

        Note:
        - 4 lines display has an custom LCD controller derived from an HD66780
        - NVRAM works only if the machine is turned off (with OFF menu) before closing MESS

        More info:
            http://archive.psion2.org/org2/techref/index.htm

****************************************************************************/

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m6800/m6800.h"
#include "sound/beep.h"
#include "imagedev/cartslot.h"
#include "video/hd44780.h"
#include "includes/psion.h"
#include "rendlay.h"

/***********************************************

    Datapack

***********************************************/

//Datapack control line
#define DP_LINE_CLOCK			0x01
#define DP_LINE_RESET			0x02
#define DP_LINE_PROGRAM			0x04
#define DP_LINE_OUTPUT_ENABLE	0x08
#define DP_LINE_POWER_ON		0x80

//Datapack ID
#define DP_ID_EPROM				0x02
#define DP_ID_PAGED				0x04
#define DP_ID_WRITE				0x08
#define DP_ID_BOOT				0x10
#define DP_ID_COPY				0x20

UINT8 datapack::update(UINT8 data)
{
	if (len == 0 || (control_lines & DP_LINE_POWER_ON))
		return 0;

	UINT32 pack_addr = counter + ((id & DP_ID_PAGED) ? 0x100 * page : 0);

	// if the datapack is 128k or more is treated as segmented
	if (len >= 0x10)
		pack_addr += (segment * 0x4000);

	if (pack_addr < len * 0x2000)
	{
		if ((control_lines & DP_LINE_OUTPUT_ENABLE) && !(control_lines & DP_LINE_RESET))
		{
			//write latched data
			buffer[pack_addr] = data;
			write_flag = true;
		}
		else if ((control_lines & DP_LINE_OUTPUT_ENABLE) && (control_lines & DP_LINE_RESET))
		{
			//write datapack segment
			if (len <= 0x10)
				segment = data & 0x07;
			else if (len <= 0x20)
				segment = data & 0x0f;
			else if (len <= 0x40)
				segment = data & 0x1f;
			else if (len <= 0x80)
				segment = data & 0x3f;
			else
				segment = data;
		}
		else if (!(control_lines & DP_LINE_OUTPUT_ENABLE) && !(control_lines & DP_LINE_RESET))
		{
			//latch data
			data = buffer[pack_addr];
		}
		else if (!(control_lines & DP_LINE_OUTPUT_ENABLE) && (control_lines & DP_LINE_RESET))
		{
			//read datapack ID
			if (id & DP_ID_EPROM)
				data = buffer[0];
			else
				data = 1;
		}
	}

	return data;
}

void datapack::control_lines_w(UINT8 data)
{
	if ((control_lines & DP_LINE_CLOCK) != (data & DP_LINE_CLOCK))
	{
		counter++;

		if (counter >= ((id & DP_ID_PAGED) ? 0x100 : (len * 0x2000)))
			counter = 0;
	}

	if ((control_lines & DP_LINE_PROGRAM) && !(data & DP_LINE_PROGRAM))
	{
		page++;

		if (page >= (len * 0x2000) / 0x100)
			page = 0;
	}

	if (data & DP_LINE_RESET)
	{
		counter = 0;
		page = 0;
	}

	control_lines = data;
}

UINT8 datapack::control_lines_r()
{
	return control_lines;
}

int datapack::load(device_image_interface &image)
{
	running_machine &machine = image.device().machine();
	UINT8 *data;
	UINT32 length;

	if (image.software_entry() == NULL)
	{
		length = image.length();
		data = auto_alloc_array(machine, UINT8, length);
		image.fread(data, image.length());
	}
	else
	{
		length = image.get_software_region_length("rom");
		data = auto_alloc_array(machine, UINT8, length);
		memcpy(data, image.get_software_region("rom"), length);
	}

	//check the OPK head
	if(strncmp((const char*)data, "OPK", 3) || data[7] * 0x2000 < length - 6)
		return IMAGE_INIT_FAIL;

	d_image = &image;
	write_flag = false;
	id = data[6];
	len = data[7];
	buffer = auto_alloc_array(machine, UINT8, len * 0x2000);
	memset(buffer, 0xff, len * 0x2000);
	memcpy(buffer, data + 6, length - 6);

	/* load battery */
	UINT8 *opk = auto_alloc_array(machine, UINT8, len * 0x2000 + 6);
	image.battery_load(opk, len * 0x2000 + 6, 0xff);
	if(!strncmp((char*)opk, "OPK", 3))
		memcpy(buffer, opk + 6, len * 0x2000);

	return IMAGE_INIT_PASS;
}

void datapack::unload(device_image_interface &image)
{
	running_machine &machine = image.device().machine();

	// save battery only if a write-access occurs
	if (len > 0 && write_flag)
	{
		const UINT8 opk_magic[6] = {'O', 'P', 'K', 0x00, 0x00, 0x00};
		UINT8 *opk = auto_alloc_array(machine, UINT8, len * 0x2000 + 6);

		memcpy(opk, opk_magic, 6);
		memcpy(opk + 6, buffer, len * 0x2000);

		d_image->battery_save(opk, len * 0x2000 + 6);

		auto_free(machine, opk);
	}

	auto_free(machine, buffer);
	len = 0;
}

void datapack::reset()
{
	d_image = NULL;
	id = 0;
	len = 0;
	counter = 0;
	page = 0;
	segment = 0;
	control_lines = 0;
	buffer = NULL;
	write_flag = false;
}

/***********************************************

    basic machine

***********************************************/

static TIMER_DEVICE_CALLBACK( nmi_timer )
{
	psion_state *state = timer.machine().driver_data<psion_state>();

	if (state->m_enable_nmi)
		cputag_set_input_line(timer.machine(), "maincpu", INPUT_LINE_NMI, PULSE_LINE);
}

UINT8 psion_state::kb_read(running_machine &machine)
{
	static const char *const bitnames[] = {"K1", "K2", "K3", "K4", "K5", "K6", "K7"};
	UINT8 line, data = 0x7c;

	if (m_kb_counter)
	{
		for (line = 0; line < 7; line++)
			if (m_kb_counter == (0x7f & ~(1 << line)))
				data = input_port_read(machine, bitnames[line]);
	}
	else
	{
		//Read all the input lines
		for (line = 0; line < 7; line++)
			data &= input_port_read(machine, bitnames[line]);
	}

	return data & 0x7c;
}

void psion_state::update_banks(running_machine &machine)
{
	if (m_ram_bank < m_ram_bank_count && m_ram_bank_count)
		memory_set_bank(machine, "rambank", m_ram_bank);

	if (m_rom_bank < m_rom_bank_count && m_rom_bank_count)
		memory_set_bank(machine, "rombank", m_rom_bank);
}

datapack* psion_state::get_active_slot(UINT8 data)
{
	switch(data & 0x70)
	{
		case 0x60:
			return &m_pack1;
		case 0x50:
			return &m_pack2;
		case 0x30:
			//Unemulated slot C
			return NULL;
	}

	return NULL;
}

WRITE8_MEMBER( psion_state::hd63701_int_reg_w )
{
	switch (offset)
	{
	case 0x01:
		m_port2_ddr = data;
		break;
	case 0x03:
		/* datapack i/o data bus */
		if (m_port2_ddr == 0xff)
		{
			datapack *pack = get_active_slot(m_port6);

			if (pack)
				m_port2 = pack->update(data);
			else
				m_port2 = data;
		}
		break;
	case 0x08:
		m_tcsr_value = data;
		break;
	case 0x15:
		/* read-only */
		break;
	case 0x16:
		m_port6_ddr = data;
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
			m_port6 = (data & m_port6_ddr) | (m_port6 & ~m_port6_ddr);
			datapack *pack = get_active_slot(m_port6);

			if (pack != NULL)
			{
				pack->control_lines_w(m_port6);
				pack->update(m_port2);
			}
		}
		break;
	}

	m6801_io_w(&space, offset, data);
}

READ8_MEMBER( psion_state::hd63701_int_reg_r )
{
	switch (offset)
	{
	case 0x03:
		/* datapack i/o data bus */
		if (m_port2_ddr == 0)
		{
			datapack *pack = get_active_slot(m_port6);

			if (pack)
				m_port2 = pack->update(m_port2);
			else
				m_port2 = 0;

			return m_port2;
		}
		else
			return 0;
	case 0x14:
		return (m6801_io_r(&space, offset)&0x7f) | (m_stby_pwr<<7);
	case 0x15:
		/*
        x--- ---- ON key active high
        -xxx xx-- keys matrix active low
        ---- --x- pulse
        ---- ---x battery status
        */
		return kb_read(m_machine) | input_port_read(m_machine, "BATTERY") | input_port_read(m_machine, "ON") | (m_kb_counter == 0x7ff)<<1 | m_pulse<<1;
	case 0x17:
		/* datapack control lines */
		{
			datapack *pack = get_active_slot(m_port6);

			if (pack)
				return pack->control_lines_r();
			else
				return m_port6;
		}
	case 0x08:
		m6801_io_w(&space, offset, m_tcsr_value);
	default:
		return m6801_io_r(&space, offset);
	}
}

/* Read/Write common */
void psion_state::io_rw(address_space &space, UINT16 offset)
{
	if (space.debugger_access())
		return;

	switch (offset & 0xffc0)
	{
	case 0xc0:
		/* switch off, CPU goes into standby mode */
		m_enable_nmi = 0;
		m_stby_pwr = 1;
		space.machine().device<cpu_device>("maincpu")->suspend(SUSPEND_REASON_HALT, 1);
		break;
	case 0x100:
		m_pulse = 1;
		break;
	case 0x140:
		m_pulse = 0;
		break;
	case 0x200:
		m_kb_counter = 0;
		break;
	case 0x180:
		beep_set_state(m_beep, 1);
		break;
	case 0x1c0:
		beep_set_state(m_beep, 0);
		break;
	case 0x240:
		if (offset == 0x260 && (m_rom_bank_count || m_ram_bank_count))
		{
			m_ram_bank=0;
			m_rom_bank=0;
			update_banks(m_machine);
		}
		else
			m_kb_counter++;
		break;
	case 0x280:
		if (offset == 0x2a0 && m_ram_bank_count)
		{
			m_ram_bank++;
			update_banks(m_machine);
		}
		else
			m_enable_nmi = 1;
		break;
	case 0x2c0:
		if (offset == 0x2e0 && m_rom_bank_count)
		{
			m_rom_bank++;
			update_banks(m_machine);
		}
		else
			m_enable_nmi = 0;
		break;
	}
}

WRITE8_MEMBER( psion_state::io_w )
{
	switch (offset & 0x0ffc0)
	{
	case 0x80:
		if (offset & 1)
			m_lcdc->data_write(space, offset, data);
		else
			m_lcdc->control_write(space, offset, data);
		break;
	default:
		io_rw(space, offset);
	}
}

READ8_MEMBER( psion_state::io_r )
{
	switch (offset & 0xffc0)
	{
	case 0x80:
		if (offset & 1)
			return m_lcdc->data_read(space, offset);
		else
			return m_lcdc->control_read(space, offset);
	default:
		io_rw(space, offset);
	}

	return 0;
}

static INPUT_CHANGED( psion_on )
{
	cpu_device *cpu = field->port->machine().device<cpu_device>("maincpu");

	/* reset the CPU for resume from standby */
	if (cpu->suspended(SUSPEND_REASON_HALT))
		cpu->reset();
}

static ADDRESS_MAP_START(psioncm_mem, AS_PROGRAM, 8, psion_state)
	ADDRESS_MAP_UNMAP_LOW
	AM_RANGE(0x0000, 0x001f) AM_READWRITE(hd63701_int_reg_r, hd63701_int_reg_w)
	AM_RANGE(0x0040, 0x00ff) AM_RAM AM_BASE(m_sys_register)
	AM_RANGE(0x0100, 0x03ff) AM_READWRITE(io_r, io_w)
	AM_RANGE(0x2000, 0x3fff) AM_RAM AM_BASE(m_ram) AM_SIZE(psion_state, m_ram_size)
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(psionla_mem, AS_PROGRAM, 8, psion_state)
	ADDRESS_MAP_UNMAP_LOW
	AM_RANGE(0x0000, 0x001f) AM_READWRITE(hd63701_int_reg_r, hd63701_int_reg_w)
	AM_RANGE(0x0040, 0x00ff) AM_RAM AM_BASE(m_sys_register)
	AM_RANGE(0x0100, 0x03ff) AM_READWRITE(io_r, io_w)
	AM_RANGE(0x0400, 0x5fff) AM_RAM AM_BASE(m_ram) AM_SIZE(psion_state, m_ram_size)
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(psionp350_mem, AS_PROGRAM, 8, psion_state)
	ADDRESS_MAP_UNMAP_LOW
	AM_RANGE(0x0000, 0x001f) AM_READWRITE(hd63701_int_reg_r, hd63701_int_reg_w)
	AM_RANGE(0x0040, 0x00ff) AM_RAM AM_BASE(m_sys_register)
	AM_RANGE(0x0100, 0x03ff) AM_READWRITE(io_r, io_w)
	AM_RANGE(0x0400, 0x3fff) AM_RAM AM_BASE(m_ram) AM_SIZE(psion_state, m_ram_size)
	AM_RANGE(0x4000, 0x7fff) AM_RAMBANK("rambank")
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(psionlam_mem, AS_PROGRAM, 8, psion_state)
	ADDRESS_MAP_UNMAP_LOW
	AM_RANGE(0x0000, 0x001f) AM_READWRITE(hd63701_int_reg_r, hd63701_int_reg_w)
	AM_RANGE(0x0040, 0x00ff) AM_RAM AM_BASE(m_sys_register)
	AM_RANGE(0x0100, 0x03ff) AM_READWRITE(io_r, io_w)
	AM_RANGE(0x0400, 0x7fff) AM_RAM AM_BASE(m_ram) AM_SIZE(psion_state, m_ram_size)
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK("rombank")
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(psionlz_mem, AS_PROGRAM, 8, psion_state)
	ADDRESS_MAP_UNMAP_LOW
	AM_RANGE(0x0000, 0x001f) AM_READWRITE(hd63701_int_reg_r, hd63701_int_reg_w)
	AM_RANGE(0x0040, 0x00ff) AM_RAM AM_BASE(m_sys_register)
	AM_RANGE(0x0100, 0x03ff) AM_READWRITE(io_r, io_w)
	AM_RANGE(0x0400, 0x3fff) AM_RAM AM_BASE(m_ram) AM_SIZE(psion_state, m_ram_size)
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
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ON/CLEAR") PORT_CODE(KEYCODE_MINUS)  PORT_CHANGED(psion_on, 0)

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
	psion_state *state = image.device().machine().driver_data<psion_state>();

	return state->m_pack1.load(image);
}

static DEVICE_IMAGE_UNLOAD( psion_pack1 )
{
	psion_state *state = image.device().machine().driver_data<psion_state>();

	state->m_pack1.unload(image);
}

static DEVICE_IMAGE_LOAD( psion_pack2 )
{
	psion_state *state = image.device().machine().driver_data<psion_state>();

	return state->m_pack2.load(image);
}

static DEVICE_IMAGE_UNLOAD( psion_pack2 )
{
	psion_state *state = image.device().machine().driver_data<psion_state>();

	state->m_pack2.unload(image);
}

static NVRAM_HANDLER( psion )
{
	psion_state *state = machine.driver_data<psion_state>();

	if (read_or_write)
	{
		file->write(state->m_sys_register, 0xc0);
		file->write(state->m_ram, state->m_ram_size);
		if (state->m_ram_bank_count)
			file->write(state->m_paged_ram, state->m_ram_bank_count * 0x4000);
	}
	else
	{
		if (file)
		{
			file->read(state->m_sys_register, 0xc0);
			file->read(state->m_ram, state->m_ram_size);
			if (state->m_ram_bank_count)
				file->read(state->m_paged_ram, state->m_ram_bank_count * 0x4000);

			//warm start
			state->m_stby_pwr = 1;
		}
		else
			//cold start
			state->m_stby_pwr = 0;
	}
}

void psion_state::machine_start()
{
	if (!strcmp(m_machine.system().name, "psionlam"))
	{
		m_rom_bank_count = 3;
		m_ram_bank_count = 0;
	}
	else if (!strcmp(m_machine.system().name, "psionp350"))
	{
		m_rom_bank_count = 0;
		m_ram_bank_count = 5;
	}
	else if (!strncmp(m_machine.system().name, "psionlz", 7))
	{
		m_rom_bank_count = 3;
		m_ram_bank_count = 3;
	}
	else if (!strcmp(m_machine.system().name, "psionp464"))
	{
		m_rom_bank_count = 3;
		m_ram_bank_count = 9;
	}
	else
	{
		m_rom_bank_count = 0;
		m_ram_bank_count = 0;
	}

	if (m_rom_bank_count)
	{
		UINT8* rom_base = (UINT8 *)m_machine.region("maincpu")->base();

		memory_configure_bank(m_machine, "rombank", 0, 1, rom_base + 0x8000, 0x4000);
		memory_configure_bank(m_machine, "rombank", 1, m_rom_bank_count-1, rom_base + 0x10000, 0x4000);
		memory_set_bank(m_machine, "rombank", 0);
	}

	if (m_ram_bank_count)
	{
		m_paged_ram = auto_alloc_array(m_machine, UINT8, m_ram_bank_count * 0x4000);
		memory_configure_bank(m_machine, "rambank", 0, m_ram_bank_count, m_paged_ram, 0x4000);
		memory_set_bank(m_machine, "rambank", 0);
	}

	state_save_register_global(m_machine, m_kb_counter);
	state_save_register_global(m_machine, m_enable_nmi);
	state_save_register_global(m_machine, m_tcsr_value);
	state_save_register_global(m_machine, m_stby_pwr);
	state_save_register_global(m_machine, m_pulse);
	state_save_register_global(m_machine, m_rom_bank);
	state_save_register_global(m_machine, m_ram_bank);
	state_save_register_global(m_machine, m_port2_ddr);
	state_save_register_global(m_machine, m_port2);
	state_save_register_global(m_machine, m_port6_ddr);
	state_save_register_global(m_machine, m_port6);
	state_save_register_global_pointer(m_machine, m_paged_ram, m_ram_bank_count * 0x4000);
}

void psion_state::machine_reset()
{
	m_enable_nmi=0;
	m_kb_counter=0;
	m_ram_bank=0;
	m_rom_bank=0;
	m_pulse=0;

	if (m_rom_bank_count || m_ram_bank_count)
		update_banks(m_machine);
}

bool psion_state::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	return m_lcdc->video_update( &bitmap, &cliprect );
}

static DRIVER_INIT( psion )
{
	psion_state *state = machine.driver_data<psion_state>();

	state->m_pack1.reset();
	state->m_pack2.reset();
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
	MCFG_CARTSLOT_ADD("pack1")
	MCFG_CARTSLOT_EXTENSION_LIST("opk")
	MCFG_CARTSLOT_NOT_MANDATORY
	MCFG_CARTSLOT_LOAD(psion_pack1)
	MCFG_CARTSLOT_UNLOAD(psion_pack1)
	MCFG_CARTSLOT_INTERFACE("psion_pack")

	/* Datapack slot 2 */
	MCFG_CARTSLOT_ADD("pack2")
	MCFG_CARTSLOT_EXTENSION_LIST("opk")
	MCFG_CARTSLOT_NOT_MANDATORY
	MCFG_CARTSLOT_LOAD(psion_pack2)
	MCFG_CARTSLOT_UNLOAD(psion_pack2)
	MCFG_CARTSLOT_INTERFACE("psion_pack")

	/* Software lists */
	MCFG_SOFTWARE_LIST_ADD("pack_list", "psion")
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
    MCFG_CPU_ADD("maincpu", HD63701, 980000) // should be HD6303 at 0.98MHz

    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(6*16, 9*2)
	MCFG_SCREEN_VISIBLE_AREA(0, 6*16-1, 0, 9*2-1)
	MCFG_DEFAULT_LAYOUT(layout_lcd)
    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(psion)
	MCFG_GFXDECODE(psion)

	MCFG_HD44780_ADD("hd44780", psion_2line_display)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO( "mono" )
	MCFG_SOUND_ADD( "beep", BEEP, 0 )
	MCFG_SOUND_ROUTE( ALL_OUTPUTS, "mono", 1.00 )

	MCFG_NVRAM_HANDLER(psion)

	MCFG_FRAGMENT_ADD( psion_slot )

	MCFG_TIMER_ADD_PERIODIC("nmi_timer", nmi_timer, attotime::from_seconds(1))
MACHINE_CONFIG_END

static const UINT8 psion_4line_layout[] =
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
    MCFG_CPU_ADD("maincpu",HD63701, 980000) // should be HD6303 at 0.98MHz

    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(6*20, 9*4)
	MCFG_SCREEN_VISIBLE_AREA(0, 6*20-1, 0, 9*4-1)
	MCFG_DEFAULT_LAYOUT(layout_lcd)
    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(psion)
	MCFG_GFXDECODE(psion)

	MCFG_HD44780_ADD("hd44780", psion_4line_display)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO( "mono" )
	MCFG_SOUND_ADD( "beep", BEEP, 0 )
	MCFG_SOUND_ROUTE( ALL_OUTPUTS, "mono", 1.00 )

	MCFG_NVRAM_HANDLER(psion)

	MCFG_FRAGMENT_ADD( psion_slot )

	MCFG_TIMER_ADD_PERIODIC("nmi_timer", nmi_timer, attotime::from_seconds(1))
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( psioncm, psion_2lines )

	MCFG_CPU_MODIFY("maincpu")
	MCFG_CPU_PROGRAM_MAP(psioncm_mem)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( psionla, psion_2lines )

	MCFG_CPU_MODIFY("maincpu")
	MCFG_CPU_PROGRAM_MAP(psionla_mem)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( psionlam, psion_2lines )

	MCFG_CPU_MODIFY("maincpu")
	MCFG_CPU_PROGRAM_MAP(psionlam_mem)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( psionp350, psion_2lines )

	MCFG_CPU_MODIFY("maincpu")
	MCFG_CPU_PROGRAM_MAP(psionp350_mem)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( psionlz, psion_4lines )

	MCFG_CPU_MODIFY("maincpu")
	MCFG_CPU_PROGRAM_MAP(psionlz_mem)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( psioncm )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "v24", "CM v2.4")
	ROMX_LOAD( "24-cm.dat",    0x8000, 0x8000,  CRC(f6798394) SHA1(736997f0db9a9ee50d6785636bdc3f8ff1c33c66), ROM_BIOS(1))

	ROM_REGION( 0x0860, "hd44780", ROMREGION_ERASE )
	ROM_LOAD( "44780a00.bin",    0x0000, 0x0860,  BAD_DUMP CRC(3a89024c) SHA1(5a87b68422a916d1b37b5be1f7ad0b3fb3af5a8d))
ROM_END

ROM_START( psionla )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "v33", "LA v3.3")
	ROMX_LOAD( "33-la.dat",    0x8000, 0x8000,  CRC(02668ed4) SHA1(e5d4ee6b1cde310a2970ffcc6f29a0ce09b08c46), ROM_BIOS(1))

	ROM_REGION( 0x0860, "hd44780", ROMREGION_ERASE )
	ROM_LOAD( "44780a00.bin",    0x0000, 0x0860,  BAD_DUMP CRC(3a89024c) SHA1(5a87b68422a916d1b37b5be1f7ad0b3fb3af5a8d))
ROM_END

ROM_START( psionp350 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "v36", "POS350 v3.6")
	ROMX_LOAD( "36-p350.dat",  0x8000, 0x8000,  CRC(3a371a74) SHA1(9167210b2c0c3bd196afc08ca44ab23e4e62635e), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "v38", "POS350 v3.8")
	ROMX_LOAD( "38-p350.dat",  0x8000, 0x8000,  CRC(1b8b082f) SHA1(a3e875a59860e344f304a831148a7980f28eaa4a), ROM_BIOS(2))

	ROM_REGION( 0x0860, "hd44780", ROMREGION_ERASE )
	ROM_LOAD( "44780a00.bin",    0x0000, 0x0860,  BAD_DUMP CRC(3a89024c) SHA1(5a87b68422a916d1b37b5be1f7ad0b3fb3af5a8d))
ROM_END

ROM_START( psionlam )
    ROM_REGION( 0x18000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "v37", "LA v3.7")
	ROMX_LOAD( "37-lam.dat",   0x8000, 0x10000, CRC(7ee3a1bc) SHA1(c7fbd6c8e47c9b7d5f636e9f56e911b363d6796b), ROM_BIOS(1))

	ROM_REGION( 0x0860, "hd44780", ROMREGION_ERASE )
	ROM_LOAD( "44780a00.bin",    0x0000, 0x0860,  BAD_DUMP CRC(3a89024c) SHA1(5a87b68422a916d1b37b5be1f7ad0b3fb3af5a8d))
ROM_END

ROM_START( psionlz64 )
    ROM_REGION( 0x18000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "v44", "LZ64 v4.4")
	ROMX_LOAD( "44-lz64.dat",  0x8000, 0x10000, CRC(aa487913) SHA1(5a44390f63fc8c1bc94299ab2eb291bc3a5b989a), ROM_BIOS(1))

	ROM_REGION( 0x0860, "hd44780", ROMREGION_ERASE )
	ROM_LOAD( "44780a00.bin",    0x0000, 0x0860,  BAD_DUMP CRC(3a89024c) SHA1(5a87b68422a916d1b37b5be1f7ad0b3fb3af5a8d))
ROM_END

ROM_START( psionlz64s )
    ROM_REGION( 0x18000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "v46", "LZ64 v4.6")
	ROMX_LOAD( "46-lz64s.dat", 0x8000, 0x10000, CRC(328d9772) SHA1(7f9e2d591d59ecfb0822d7067c2fe59542ea16dd), ROM_BIOS(1))

	ROM_REGION( 0x0860, "hd44780", ROMREGION_ERASE )
	ROM_LOAD( "44780a00.bin",    0x0000, 0x0860,  BAD_DUMP CRC(3a89024c) SHA1(5a87b68422a916d1b37b5be1f7ad0b3fb3af5a8d))
ROM_END

ROM_START( psionlz )
    ROM_REGION( 0x18000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "v46", "LZ v4.6")
	ROMX_LOAD( "46-lz.dat",    0x8000, 0x10000, CRC(22715f48) SHA1(cf460c81cadb53eddb7afd8dadecbe8c38ea3fc2), ROM_BIOS(1))

	ROM_REGION( 0x0860, "hd44780", ROMREGION_ERASE )
	ROM_LOAD( "44780a00.bin",    0x0000, 0x0860,  BAD_DUMP CRC(3a89024c) SHA1(5a87b68422a916d1b37b5be1f7ad0b3fb3af5a8d))
ROM_END

ROM_START( psionp464 )
    ROM_REGION( 0x18000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "v46", "POS464 v4.6")
	ROMX_LOAD( "46-p464.dat",  0x8000, 0x10000, CRC(672a0945) SHA1(d2a6e3fe1019d1bd7ae4725e33a0b9973f8cd7d8), ROM_BIOS(1))

	ROM_REGION( 0x0860, "hd44780", ROMREGION_ERASE )
	ROM_LOAD( "44780a00.bin",    0x0000, 0x0860,  BAD_DUMP CRC(3a89024c) SHA1(5a87b68422a916d1b37b5be1f7ad0b3fb3af5a8d))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT COMPANY   FULLNAME       FLAGS */
COMP( 1986, psioncm,	0,       0, 	psioncm,		psion,	 psion,   "Psion",   "Organiser II CM",		GAME_SUPPORTS_SAVE | GAME_IMPERFECT_GRAPHICS)
COMP( 1986, psionla,	psioncm, 0, 	psionla,	    psion,	 psion,   "Psion",   "Organiser II LA",		GAME_SUPPORTS_SAVE | GAME_IMPERFECT_GRAPHICS)
COMP( 1986, psionp350,	psioncm, 0, 	psionp350,	    psion,	 psion,   "Psion",   "Organiser II P350",	GAME_SUPPORTS_SAVE | GAME_IMPERFECT_GRAPHICS)
COMP( 1986, psionlam,	psioncm, 0, 	psionlam,	    psion,	 psion,   "Psion",   "Organiser II LAM",	GAME_SUPPORTS_SAVE | GAME_IMPERFECT_GRAPHICS)
COMP( 1989, psionlz,	0,		 0, 	psionlz,	    psion,	 psion,   "Psion",   "Organiser II LZ",		GAME_SUPPORTS_SAVE | GAME_IMPERFECT_GRAPHICS)
COMP( 1989, psionlz64,  psionlz, 0, 	psionlz,	    psion,	 psion,   "Psion",   "Organiser II LZ64",	GAME_SUPPORTS_SAVE | GAME_IMPERFECT_GRAPHICS)
COMP( 1989, psionlz64s,	psionlz, 0, 	psionlz,	    psion,	 psion,   "Psion",   "Organiser II LZ64S",	GAME_SUPPORTS_SAVE | GAME_IMPERFECT_GRAPHICS)
COMP( 1989, psionp464,	psionlz, 0, 	psionlz,	    psion,	 psion,   "Psion",   "Organiser II P464",	GAME_SUPPORTS_SAVE | GAME_IMPERFECT_GRAPHICS)
