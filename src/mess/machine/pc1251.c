#include "emu.h"
#include "cpu/sc61860/sc61860.h"

#include "includes/pocketc.h"
#include "includes/pc1251.h"

/* C-CE while reset, program will not be destroyed! */

static UINT8 outa,outb;

static int power=1; /* simulates pressed cce when mess is started */

void pc1251_outa(running_device *device, int data)
{
	outa=data;
}

void pc1251_outb(running_device *device, int data)
{
	outb=data;
}

void pc1251_outc(running_device *device, int data)
{

}

int pc1251_ina(running_device *device)
{
	int data = outa;
	running_machine *machine = device->machine;

	if (outb & 0x01)
	{
		data |= input_port_read(machine, "KEY0");

		/* At Power Up we fake a 'CL' pressure */
		if (power)
			data |= 0x02;		// problem with the deg lcd
	}

	if (outb & 0x02)
		data |= input_port_read(machine, "KEY1");

	if (outb & 0x04)
		data |= input_port_read(machine, "KEY2");

	if (outa & 0x01)
		data |= input_port_read(machine, "KEY3");

	if (outa & 0x02)
		data |= input_port_read(machine, "KEY4");

	if (outa & 0x04)
		data |= input_port_read(machine, "KEY5");

	if (outa & 0x08)
		data |= input_port_read(machine, "KEY6");

	if (outa & 0x10)
		data |= input_port_read(machine, "KEY7");

	if (outa & 0x20)
		data |= input_port_read(machine, "KEY8");

	if (outa & 0x40)
		data |= input_port_read(machine, "KEY9");

	return data;
}

int pc1251_inb(running_device *device)
{
	int data = outb;

	if (outb & 0x08)
		data |= (input_port_read(device->machine, "MODE") & 0x07);

	return data;
}

int pc1251_brk(running_device *device)
{
	return (input_port_read(device->machine, "EXTRA") & 0x01);
}

int pc1251_reset(running_device *device)
{
	return (input_port_read(device->machine, "EXTRA") & 0x02);
}

/* currently enough to save the external ram */
NVRAM_HANDLER( pc1251 )
{
	running_device *main_cpu = devtag_get_device(machine, "maincpu");
	UINT8 *ram = memory_region(machine, "maincpu") + 0x8000;
	UINT8 *cpu = sc61860_internal_ram(main_cpu);

	if (read_or_write)
	{
		mame_fwrite(file, cpu, 96);
		mame_fwrite(file, ram, 0x4800);
	}
	else if (file)
	{
		mame_fread(file, cpu, 96);
		mame_fread(file, ram, 0x4800);
	}
	else
	{
		memset(cpu, 0, 96);
		memset(ram, 0, 0x4800);
	}
}

static TIMER_CALLBACK(pc1251_power_up)
{
	power = 0;
}

DRIVER_INIT( pc1251 )
{
	int i;
	UINT8 *gfx = memory_region(machine, "gfx1");
	for (i=0; i<128; i++) gfx[i]=i;

	timer_set(machine, ATTOTIME_IN_SEC(1), NULL, 0, pc1251_power_up);
}

