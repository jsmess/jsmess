#include "emu.h"
#include "cpu/sc61860/sc61860.h"

#include "includes/pocketc.h"
#include "includes/pc1251.h"

/* C-CE while reset, program will not be destroyed! */



void pc1251_outa(device_t *device, int data)
{
	pc1251_state *state = device->machine().driver_data<pc1251_state>();
	state->m_outa = data;
}

void pc1251_outb(device_t *device, int data)
{
	pc1251_state *state = device->machine().driver_data<pc1251_state>();
	state->m_outb = data;
}

void pc1251_outc(device_t *device, int data)
{
}

int pc1251_ina(device_t *device)
{
	pc1251_state *state = device->machine().driver_data<pc1251_state>();
	int data = state->m_outa;
	running_machine &machine = device->machine();

	if (state->m_outb & 0x01)
	{
		data |= input_port_read(machine, "KEY0");

		/* At Power Up we fake a 'CL' pressure */
		if (state->m_power)
			data |= 0x02;		// problem with the deg lcd
	}

	if (state->m_outb & 0x02)
		data |= input_port_read(machine, "KEY1");

	if (state->m_outb & 0x04)
		data |= input_port_read(machine, "KEY2");

	if (state->m_outa & 0x01)
		data |= input_port_read(machine, "KEY3");

	if (state->m_outa & 0x02)
		data |= input_port_read(machine, "KEY4");

	if (state->m_outa & 0x04)
		data |= input_port_read(machine, "KEY5");

	if (state->m_outa & 0x08)
		data |= input_port_read(machine, "KEY6");

	if (state->m_outa & 0x10)
		data |= input_port_read(machine, "KEY7");

	if (state->m_outa & 0x20)
		data |= input_port_read(machine, "KEY8");

	if (state->m_outa & 0x40)
		data |= input_port_read(machine, "KEY9");

	return data;
}

int pc1251_inb(device_t *device)
{
	pc1251_state *state = device->machine().driver_data<pc1251_state>();
	int data = state->m_outb;

	if (state->m_outb & 0x08)
		data |= (input_port_read(device->machine(), "MODE") & 0x07);

	return data;
}

int pc1251_brk(device_t *device)
{
	return (input_port_read(device->machine(), "EXTRA") & 0x01);
}

int pc1251_reset(device_t *device)
{
	return (input_port_read(device->machine(), "EXTRA") & 0x02);
}

/* currently enough to save the external ram */
NVRAM_HANDLER( pc1251 )
{
	device_t *main_cpu = machine.device("maincpu");
	UINT8 *ram = machine.region("maincpu")->base() + 0x8000;
	UINT8 *cpu = sc61860_internal_ram(main_cpu);

	if (read_or_write)
	{
		file->write(cpu, 96);
		file->write(ram, 0x4800);
	}
	else if (file)
	{
		file->read(cpu, 96);
		file->read(ram, 0x4800);
	}
	else
	{
		memset(cpu, 0, 96);
		memset(ram, 0, 0x4800);
	}
}

static TIMER_CALLBACK(pc1251_power_up)
{
	pc1251_state *state = machine.driver_data<pc1251_state>();
	state->m_power = 0;
}

DRIVER_INIT( pc1251 )
{
	pc1251_state *state = machine.driver_data<pc1251_state>();
	int i;
	UINT8 *gfx = machine.region("gfx1")->base();
	for (i=0; i<128; i++) gfx[i]=i;

	state->m_power = 1;
	machine.scheduler().timer_set(attotime::from_seconds(1), FUNC(pc1251_power_up));
}

