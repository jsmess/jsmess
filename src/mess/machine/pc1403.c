#include "emu.h"
#include "cpu/sc61860/sc61860.h"

#include "includes/pocketc.h"
#include "includes/pc1403.h"
#include "machine/ram.h"

/* C-CE while reset, program will not be destroyed! */




/*
   port 2:
     bits 0,1: external rom a14,a15 lines
   port 3:
     bits 0..6 keyboard output select matrix line
*/

WRITE8_HANDLER(pc1403_asic_write)
{
	pc1403_state *state = space->machine().driver_data<pc1403_state>();
    state->m_asic[offset>>9]=data;
    switch( (offset>>9) ){
    case 0/*0x3800*/:
	// output
	logerror ("asic write %.4x %.2x\n",offset, data);
	break;
    case 1/*0x3a00*/:
	logerror ("asic write %.4x %.2x\n",offset, data);
	break;
    case 2/*0x3c00*/:
	memory_set_bankptr(space->machine(), "bank1", space->machine().region("user1")->base()+((data&7)<<14));
	logerror ("asic write %.4x %.2x\n",offset, data);
	break;
    case 3/*0x3e00*/: break;
    }
}

READ8_HANDLER(pc1403_asic_read)
{
	pc1403_state *state = space->machine().driver_data<pc1403_state>();
    UINT8 data=state->m_asic[offset>>9];
    switch( (offset>>9) ){
    case 0: case 1: case 2:
	logerror ("asic read %.4x %.2x\n",offset, data);
	break;
    }
    return data;
}

void pc1403_outa(device_t *device, int data)
{
	pc1403_state *state = device->machine().driver_data<pc1403_state>();
    state->m_outa=data;
}

int pc1403_ina(device_t *device)
{
	pc1403_state *state = device->machine().driver_data<pc1403_state>();
    UINT8 data=state->m_outa;

    if (state->m_asic[3] & 0x01)
		data |= input_port_read(device->machine(), "KEY0");

    if (state->m_asic[3] & 0x02)
		data |= input_port_read(device->machine(), "KEY1");

    if (state->m_asic[3] & 0x04)
		data |= input_port_read(device->machine(), "KEY2");

    if (state->m_asic[3] & 0x08)
		data |= input_port_read(device->machine(), "KEY3");

    if (state->m_asic[3] & 0x10)
		data |= input_port_read(device->machine(), "KEY4");

    if (state->m_asic[3] & 0x20)
		data |= input_port_read(device->machine(), "KEY5");

    if (state->m_asic[3] & 0x40)
		data |= input_port_read(device->machine(), "KEY6");

    if (state->m_outa & 0x01)
	{
		data |= input_port_read(device->machine(), "KEY7");

		/* At Power Up we fake a 'C-CE' pressure */
		if (state->m_power)
			data |= 0x02;
	}

    if (state->m_outa & 0x02)
		data |= input_port_read(device->machine(), "KEY8");

    if (state->m_outa & 0x04)
		data |= input_port_read(device->machine(), "KEY9");

    if (state->m_outa & 0x08)
		data |= input_port_read(device->machine(), "KEY10");

    if (state->m_outa & 0x10)
		data |= input_port_read(device->machine(), "KEY11");

    if (state->m_outa & 0x20)
		data |= input_port_read(device->machine(), "KEY12");

    if (state->m_outa & 0x40)
		data |= input_port_read(device->machine(), "KEY13");

    return data;
}

#if 0
int pc1403_inb(void)
{
	pc1403_state *state = machine.driver_data<pc140_state>();
	int data = state->m_outb;

	if (input_port_read(machine, "KEY13"))
		data |= 1;

	return data;
}
#endif

void pc1403_outc(device_t *device, int data)
{
	pc1403_state *state = device->machine().driver_data<pc1403_state>();
    state->m_portc = data;
//    logerror("%g pc %.4x outc %.2x\n", device->machine().time().as_double(), cpu_get_pc(device->machine().device("maincpu")), data);
}


int pc1403_brk(device_t *device)
{
	return (input_port_read(device->machine(), "EXTRA") & 0x01);
}

int pc1403_reset(device_t *device)
{
	return (input_port_read(device->machine(), "EXTRA") & 0x02);
}

/* currently enough to save the external ram */
NVRAM_HANDLER( pc1403 )
{
	device_t *main_cpu = machine.device("maincpu");
	UINT8 *ram = machine.region("maincpu")->base() + 0x8000;
	UINT8 *cpu = sc61860_internal_ram(main_cpu);

	if (read_or_write)
	{
		file->write(cpu, 96);
		file->write(ram, 0x8000);
	}
	else if (file)
	{
		file->read(cpu, 96);
		file->read(ram, 0x8000);
	}
	else
	{
		memset(cpu, 0, 96);
		memset(ram, 0, 0x8000);
	}
}

static TIMER_CALLBACK(pc1403_power_up)
{
	pc1403_state *state = machine.driver_data<pc1403_state>();
	state->m_power=0;
}

DRIVER_INIT( pc1403 )
{
	pc1403_state *state = machine.driver_data<pc1403_state>();
	int i;
	UINT8 *gfx=machine.region("gfx1")->base();

	for (i=0; i<128; i++) gfx[i]=i;

	state->m_power = 1;
	machine.scheduler().timer_set(attotime::from_seconds(1), FUNC(pc1403_power_up));

	memory_set_bankptr(machine, "bank1", machine.region("user1")->base());
}
