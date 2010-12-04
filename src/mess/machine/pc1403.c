#include "emu.h"
#include "cpu/sc61860/sc61860.h"

#include "includes/pocketc.h"
#include "includes/pc1403.h"
#include "devices/messram.h"

/* C-CE while reset, program will not be destroyed! */




/*
   port 2:
     bits 0,1: external rom a14,a15 lines
   port 3:
     bits 0..6 keyboard output select matrix line
*/

WRITE8_HANDLER(pc1403_asic_write)
{
	pc1403_state *state = space->machine->driver_data<pc1403_state>();
    state->asic[offset>>9]=data;
    switch( (offset>>9) ){
    case 0/*0x3800*/:
	// output
	logerror ("asic write %.4x %.2x\n",offset, data);
	break;
    case 1/*0x3a00*/:
	logerror ("asic write %.4x %.2x\n",offset, data);
	break;
    case 2/*0x3c00*/:
	memory_set_bankptr(space->machine, "bank1", memory_region(space->machine, "user1")+((data&7)<<14));
	logerror ("asic write %.4x %.2x\n",offset, data);
	break;
    case 3/*0x3e00*/: break;
    }
}

READ8_HANDLER(pc1403_asic_read)
{
	pc1403_state *state = space->machine->driver_data<pc1403_state>();
    UINT8 data=state->asic[offset>>9];
    switch( (offset>>9) ){
    case 0: case 1: case 2:
	logerror ("asic read %.4x %.2x\n",offset, data);
	break;
    }
    return data;
}

void pc1403_outa(running_device *device, int data)
{
	pc1403_state *state = device->machine->driver_data<pc1403_state>();
    state->outa=data;
}

int pc1403_ina(running_device *device)
{
	pc1403_state *state = device->machine->driver_data<pc1403_state>();
    UINT8 data=state->outa;

    if (state->asic[3] & 0x01)
		data |= input_port_read(device->machine, "KEY0");

    if (state->asic[3] & 0x02)
		data |= input_port_read(device->machine, "KEY1");

    if (state->asic[3] & 0x04)
		data |= input_port_read(device->machine, "KEY2");

    if (state->asic[3] & 0x08)
		data |= input_port_read(device->machine, "KEY3");

    if (state->asic[3] & 0x10)
		data |= input_port_read(device->machine, "KEY4");

    if (state->asic[3] & 0x20)
		data |= input_port_read(device->machine, "KEY5");

    if (state->asic[3] & 0x40)
		data |= input_port_read(device->machine, "KEY6");

    if (state->outa & 0x01)
	{
		data |= input_port_read(device->machine, "KEY7");

		/* At Power Up we fake a 'C-CE' pressure */
		if (state->power)
			data |= 0x02;
	}

    if (state->outa & 0x02)
		data |= input_port_read(device->machine, "KEY8");

    if (state->outa & 0x04)
		data |= input_port_read(device->machine, "KEY9");

    if (state->outa & 0x08)
		data |= input_port_read(device->machine, "KEY10");

    if (state->outa & 0x10)
		data |= input_port_read(device->machine, "KEY11");

    if (state->outa & 0x20)
		data |= input_port_read(device->machine, "KEY12");

    if (state->outa & 0x40)
		data |= input_port_read(device->machine, "KEY13");

    return data;
}

#if 0
int pc1403_inb(void)
{
	pc1403_state *state = machine->driver_data<pc140_state>();
	int data = state->outb;

	if (input_port_read(machine, "KEY13"))
		data |= 1;

	return data;
}
#endif

void pc1403_outc(running_device *device, int data)
{
	pc1403_state *state = device->machine->driver_data<pc1403_state>();
    state->portc = data;
//    logerror("%g pc %.4x outc %.2x\n", attotime_to_double(timer_get_time(device->machine)), cpu_get_pc(device->machine->device("maincpu")), data);
}


int pc1403_brk(running_device *device)
{
	return (input_port_read(device->machine, "EXTRA") & 0x01);
}

int pc1403_reset(running_device *device)
{
	return (input_port_read(device->machine, "EXTRA") & 0x02);
}

/* currently enough to save the external ram */
NVRAM_HANDLER( pc1403 )
{
	running_device *main_cpu = machine->device("maincpu");
	UINT8 *ram = memory_region(machine, "maincpu") + 0x8000;
	UINT8 *cpu = sc61860_internal_ram(main_cpu);

	if (read_or_write)
	{
		mame_fwrite(file, cpu, 96);
		mame_fwrite(file, ram, 0x8000);
	}
	else if (file)
	{
		mame_fread(file, cpu, 96);
		mame_fread(file, ram, 0x8000);
	}
	else
	{
		memset(cpu, 0, 96);
		memset(ram, 0, 0x8000);
	}
}

static TIMER_CALLBACK(pc1403_power_up)
{
	pc1403_state *state = machine->driver_data<pc1403_state>();
	state->power=0;
}

DRIVER_INIT( pc1403 )
{
	pc1403_state *state = machine->driver_data<pc1403_state>();
	int i;
	UINT8 *gfx=memory_region(machine, "gfx1");

	for (i=0; i<128; i++) gfx[i]=i;

	state->power = 1;
	timer_set(machine, ATTOTIME_IN_SEC(1), NULL, 0, pc1403_power_up);

	memory_set_bankptr(machine, "bank1", memory_region(machine, "user1"));
}
