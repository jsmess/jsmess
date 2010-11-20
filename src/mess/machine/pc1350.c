#include "emu.h"
#include "cpu/sc61860/sc61860.h"

#include "includes/pocketc.h"
#include "includes/pc1350.h"
#include "devices/messram.h"

static UINT8 outa,outb;

static int power; /* simulates pressed cce when mess is started */

void pc1350_outa(running_device *device, int data)
{
	outa=data;
}

void pc1350_outb(running_device *device, int data)
{
	outb=data;
}

void pc1350_outc(running_device *device, int data)
{

}

int pc1350_ina(running_device *device)
{
	running_machine *machine = device->machine;
	int data = outa;
	int t = pc1350_keyboard_line_r();

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

	if (outa & 0x01)
		data |= input_port_read(machine, "KEY6");

	if (outa & 0x02)
		data |= input_port_read(machine, "KEY7");

	if (outa & 0x04)
	{
		data |= input_port_read(machine, "KEY8");

		/* At Power Up we fake a 'CLS' pressure */
		if (power)
			data |= 0x08;
	}

	if (outa & 0x08)
		data |= input_port_read(machine, "KEY9");

	if (outa & 0x10)
		data |= input_port_read(machine, "KEY10");

	if (outa & 0xc0)
		data |= input_port_read(machine, "KEY11");

	// missing lshift

	return data;
}

int pc1350_inb(running_device *device)
{
	int data=outb;
	return data;
}

int pc1350_brk(running_device *device)
{
	return (input_port_read(device->machine, "EXTRA") & 0x01);
}

/* currently enough to save the external ram */
NVRAM_HANDLER( pc1350 )
{
	running_device *main_cpu = machine->device("maincpu");
	UINT8 *ram = memory_region(machine, "maincpu") + 0x2000;
	UINT8 *cpu = sc61860_internal_ram(main_cpu);

	if (read_or_write)
	{
		mame_fwrite(file, cpu, 96);
		mame_fwrite(file, ram, 0x5000);
	}
	else if (file)
	{
		mame_fread(file, cpu, 96);
		mame_fread(file, ram, 0x5000);
	}
	else
	{
		memset(cpu, 0, 96);
		memset(ram, 0, 0x5000);
	}
}

static TIMER_CALLBACK(pc1350_power_up)
{
	power=0;
}

MACHINE_START( pc1350 )
{
	address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	power = 1;
	timer_set(machine, ATTOTIME_IN_SEC(1), NULL, 0, pc1350_power_up);

	memory_install_readwrite_bank(space, 0x6000, 0x6fff, 0, 0, "bank1");
	memory_set_bankptr(machine, "bank1", &messram_get_ptr(machine->device("messram"))[0x0000]);

	if (messram_get_size(machine->device("messram")) >= 0x3000)
	{
		memory_install_readwrite_bank(space, 0x4000, 0x5fff, 0, 0, "bank2");
		memory_set_bankptr(machine, "bank2", &messram_get_ptr(machine->device("messram"))[0x1000]);
	}
	else
	{
		memory_nop_readwrite(space, 0x4000, 0x5fff, 0, 0);
	}

	if (messram_get_size(machine->device("messram")) >= 0x5000)
	{
		memory_install_readwrite_bank(space, 0x2000, 0x3fff, 0, 0, "bank3");
		memory_set_bankptr(machine, "bank3", &messram_get_ptr(machine->device("messram"))[0x3000]);
	}
	else
	{
		memory_nop_readwrite(space, 0x2000, 0x3fff, 0, 0);
	}
}
