#include "driver.h"
#include "cpu/sc61860/sc61860.h"

#include "includes/pocketc.h"
#include "includes/pc1401.h"
#include "includes/pc1403.h"

/* C-CE while reset, program will not be destroyed! */

static UINT8 outa;
UINT8 pc1403_portc;

static int power=1; /* simulates pressed cce when mess is started */


/*
   port 2:
     bits 0,1: external rom a14,a15 lines
   port 3:
     bits 0..6 keyboard output select matrix line
*/
static UINT8 asic[4];

WRITE8_HANDLER(pc1403_asic_write)
{
    asic[offset>>9]=data;
    switch( (offset>>9) ){
    case 0/*0x3800*/:
	// output
	logerror ("asic write %.4x %.2x\n",offset, data);
	break;
    case 1/*0x3a00*/:
	logerror ("asic write %.4x %.2x\n",offset, data);
	break;
    case 2/*0x3c00*/:
	memory_set_bankptr(space->machine, 1, memory_region(space->machine, "user1")+((data&7)<<14));
	logerror ("asic write %.4x %.2x\n",offset, data);
	break;
    case 3/*0x3e00*/: break;
    }
}

READ8_HANDLER(pc1403_asic_read)
{
    UINT8 data=asic[offset>>9];
    switch( (offset>>9) ){
    case 0: case 1: case 2:
	logerror ("asic read %.4x %.2x\n",offset, data);
	break;
    }
    return data;
}

void pc1403_outa(const device_config *device, int data)
{
    outa=data;
}

int pc1403_ina(const device_config *device)
{
    UINT8 data=outa;

    if (asic[3] & 0x01)
		data |= input_port_read(device->machine, "KEY0");

    if (asic[3] & 0x02)
		data |= input_port_read(device->machine, "KEY1");

    if (asic[3] & 0x04)
		data |= input_port_read(device->machine, "KEY2");

    if (asic[3] & 0x08)
		data |= input_port_read(device->machine, "KEY3");

    if (asic[3] & 0x10)
		data |= input_port_read(device->machine, "KEY4");

    if (asic[3] & 0x20)
		data |= input_port_read(device->machine, "KEY5");

    if (asic[3] & 0x40)
		data |= input_port_read(device->machine, "KEY6");

    if (outa & 0x01)
	{
		data |= input_port_read(device->machine, "KEY7");

		/* At Power Up we fake a 'C-CE' pressure */
		if (power)
			data |= 0x02;
	}

    if (outa & 0x02)
		data |= input_port_read(device->machine, "KEY8");

    if (outa & 0x04)
		data |= input_port_read(device->machine, "KEY9");

    if (outa & 0x08)
		data |= input_port_read(device->machine, "KEY10");

    if (outa & 0x10)
		data |= input_port_read(device->machine, "KEY11");

    if (outa & 0x20)
		data |= input_port_read(device->machine, "KEY12");

    if (outa & 0x40)
		data |= input_port_read(device->machine, "KEY13");

    return data;
}

#if 0
int pc1403_inb(void)
{
	int data = outb;

	if (input_port_read(machine, "KEY13"))
		data |= 1;

	return data;
}
#endif

void pc1403_outc(const device_config *device, int data)
{
    pc1403_portc = data;
//    logerror("%g pc %.4x outc %.2x\n", attotime_to_double(timer_get_time(device->machine)), cpu_get_pc(cputag_get_cpu(device->machine, "maincpu")), data);
}


int pc1403_brk(const device_config *device)
{
	return (input_port_read(device->machine, "EXTRA") & 0x01);
}

int pc1403_reset(const device_config *device)
{
	return (input_port_read(device->machine, "EXTRA") & 0x02);
}

/* currently enough to save the external ram */
NVRAM_HANDLER( pc1403 )
{
	const device_config *main_cpu = cputag_get_cpu(machine, "maincpu");
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
	power=0;
}

DRIVER_INIT( pc1403 )
{
	int i;
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT8 *gfx=memory_region(machine, "gfx1");

	for (i=0; i<128; i++) gfx[i]=i;

	timer_set(machine, ATTOTIME_IN_SEC(1), NULL, 0, pc1403_power_up);

	memory_set_bankptr(machine, 1, memory_region(machine, "user1"));
	/* NPW 28-Jun-2006 - Input ports can't be read at init time! Even then, this should use mess_ram */
	if (0 && (input_port_read(machine, "DSW0") & 0x80) == 0x80)
	{
		memory_install_read8_handler(space, 0x8000, 0xdfff, 0, 0, SMH_RAM);
		memory_install_write8_handler(space, 0x8000, 0xdfff, 0, 0, SMH_RAM);
	}
	else
	{
		memory_install_read8_handler(space, 0x8000, 0xdfff, 0, 0, SMH_NOP);
		memory_install_write8_handler(space, 0x8000, 0xdfff, 0, 0, SMH_NOP);
	}
}
