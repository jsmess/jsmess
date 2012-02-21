#include "emu.h"
#include "cpu/sc61860/sc61860.h"

#include "includes/pocketc.h"
#include "includes/pc1350.h"
#include "machine/ram.h"



void pc1350_outa(device_t *device, int data)
{
	pc1350_state *state = device->machine().driver_data<pc1350_state>();
	state->m_outa=data;
}

void pc1350_outb(device_t *device, int data)
{
	pc1350_state *state = device->machine().driver_data<pc1350_state>();
	state->m_outb=data;
}

void pc1350_outc(device_t *device, int data)
{

}

int pc1350_ina(device_t *device)
{
	pc1350_state *state = device->machine().driver_data<pc1350_state>();
	running_machine &machine = device->machine();
	int data = state->m_outa;
	int t = pc1350_keyboard_line_r(machine);

	if (t & 0x01)
		data |= input_port_read(machine, "KEY0");

	if (t & 0x02)
		data |= input_port_read(machine, "KEY1");

	if (t & 0x04)
		data |= input_port_read(machine, "KEY2");

	if (t & 0x08)
		data |= input_port_read(machine, "KEY3");

	if (t & 0x10)
		data |= input_port_read(machine, "KEY4");

	if (t & 0x20)
		data |= input_port_read(machine, "KEY5");

	if (state->m_outa & 0x01)
		data |= input_port_read(machine, "KEY6");

	if (state->m_outa & 0x02)
		data |= input_port_read(machine, "KEY7");

	if (state->m_outa & 0x04)
	{
		data |= input_port_read(machine, "KEY8");

		/* At Power Up we fake a 'CLS' pressure */
		if (state->m_power)
			data |= 0x08;
	}

	if (state->m_outa & 0x08)
		data |= input_port_read(machine, "KEY9");

	if (state->m_outa & 0x10)
		data |= input_port_read(machine, "KEY10");

	if (state->m_outa & 0xc0)
		data |= input_port_read(machine, "KEY11");

	// missing lshift

	return data;
}

int pc1350_inb(device_t *device)
{
	pc1350_state *state = device->machine().driver_data<pc1350_state>();
	int data=state->m_outb;
	return data;
}

int pc1350_brk(device_t *device)
{
	return (input_port_read(device->machine(), "EXTRA") & 0x01);
}

static TIMER_CALLBACK(pc1350_power_up)
{
	pc1350_state *state = machine.driver_data<pc1350_state>();
	state->m_power=0;
}

MACHINE_START( pc1350 )
{
	pc1350_state *state = machine.driver_data<pc1350_state>();
	address_space *space = machine.device("maincpu")->memory().space(AS_PROGRAM);

	state->m_power = 1;
	machine.scheduler().timer_set(attotime::from_seconds(1), FUNC(pc1350_power_up));

	space->install_readwrite_bank(0x6000, 0x6fff, "bank1");
	memory_set_bankptr(machine, "bank1", &machine.device<ram_device>(RAM_TAG)->pointer()[0x0000]);

	if (machine.device<ram_device>(RAM_TAG)->size() >= 0x3000)
	{
		space->install_readwrite_bank(0x4000, 0x5fff, "bank2");
		memory_set_bankptr(machine, "bank2", &machine.device<ram_device>(RAM_TAG)->pointer()[0x1000]);
	}
	else
	{
		space->nop_readwrite(0x4000, 0x5fff);
	}

	if (machine.device<ram_device>(RAM_TAG)->size() >= 0x5000)
	{
		space->install_readwrite_bank(0x2000, 0x3fff, "bank3");
		memory_set_bankptr(machine, "bank3", &machine.device<ram_device>(RAM_TAG)->pointer()[0x3000]);
	}
	else
	{
		space->nop_readwrite(0x2000, 0x3fff);
	}

	device_t *main_cpu = machine.device("maincpu");
	UINT8 *ram = machine.region("maincpu")->base() + 0x2000;
	UINT8 *cpu = sc61860_internal_ram(main_cpu);

	machine.device<nvram_device>("cpu_nvram")->set_base(cpu, 96);
	machine.device<nvram_device>("ram_nvram")->set_base(ram, 0x5000);
}
