#include "driver.h"
#include "cpu/sc61860/sc61860.h"

#include "includes/pocketc.h"
#include "includes/pc1401.h"

/* C-CE while reset, program will not be destroyed! */

/* error codes
1 syntax error
2 calculation error
3 illegal function argument
4 too large a line number
5 next without for
  return without gosub
6 memory overflow
7 print using error
8 i/o device error
9 other errors*/

static UINT8 outa,outb;
UINT8 pc1401_portc;

static int power = 1; /* simulates pressed cce when mess is started */

void pc1401_outa(const device_config *device, int data)
{
	outa=data;
}

void pc1401_outb(const device_config *device, int data)
{
	outb=data;
}

void pc1401_outc(const device_config *device, int data)
{
	//logerror("%g outc %.2x\n", attotime_to_double(timer_get_time(machine)), data);
	pc1401_portc=data;
}

int pc1401_ina(const device_config *device)
{
	int data = outa;

	if (outb & 0x01)
		data |= input_port_read(device->machine, "KEY0");

	if (outb & 0x02)
		data |= input_port_read(device->machine, "KEY1");

	if (outb & 0x04)
		data |= input_port_read(device->machine, "KEY2");

	if (outb & 0x08)
		data |= input_port_read(device->machine, "KEY3");

	if (outb & 0x10)
		data |= input_port_read(device->machine, "KEY4");

	if (outb & 0x20)
	{
		data |= input_port_read(device->machine, "KEY5");

		/* At Power Up we fake a 'C-CE' pressure */
		if (power)
			data |= 0x01;
	}

	if (outa & 0x01)
		data |= input_port_read(device->machine, "KEY6");

	if (outa & 0x02)
		data |= input_port_read(device->machine, "KEY7");

	if (outa & 0x04)
		data |= input_port_read(device->machine, "KEY8");

	if (outa & 0x08)
		data |= input_port_read(device->machine, "KEY9");

	if (outa & 0x10)
		data |= input_port_read(device->machine, "KEY10");

	if (outa & 0x20)
		data |= input_port_read(device->machine, "KEY11");

	if (outa & 0x40)
		data |= input_port_read(device->machine, "KEY12");

	return data;
}

int pc1401_inb(const device_config *device)
{
	int data=outb;

	if (input_port_read(device->machine, "EXTRA") & 0x04)
		data |= 0x01;

	return data;
}

int pc1401_brk(const device_config *device)
{
	return (input_port_read(device->machine, "EXTRA") & 0x01);
}

int pc1401_reset(const device_config *device)
{
	return (input_port_read(device->machine, "EXTRA") & 0x02);
}

/* currently enough to save the external ram */
NVRAM_HANDLER( pc1401 )
{
	const device_config *main_cpu = cputag_get_cpu(machine, "maincpu");
	UINT8 *ram = memory_region(machine, "maincpu")+0x2000;
	UINT8 *cpu = sc61860_internal_ram(main_cpu);

	if (read_or_write)
	{
		mame_fwrite(file, cpu, 96);
		mame_fwrite(file, ram, 0x2800);
	}
	else if (file)
	{
		mame_fread(file, cpu, 96);
		mame_fread(file, ram, 0x2800);
	}
	else
	{
		memset(cpu, 0, 96);
		memset(ram, 0, 0x2800);
	}
}

static TIMER_CALLBACK(pc1401_power_up)
{
	power=0;
}

DRIVER_INIT( pc1401 )
{
	int i;
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT8 *gfx=memory_region(machine, "gfx1");
#if 0
	char sucker[]={
		/* this routine dump the memory (start 0)
           in an endless loop,
           the pc side must be started before this
           its here to allow verification of the decimal data
           in mame disassembler
        */
#if 1
		18,4,/*lip xl */
		2,0,/*lia 0 startaddress low */
		219,/*exam */
		18,5,/*lip xh */
		2,0,/*lia 0 startaddress high */
		219,/*exam */
/*400f x: */
		/* dump internal rom */
		18,5,/*lip 4 */
		89,/*ldm */
		218,/*exab */
		18,4,/*lip 5 */
		89,/*ldm */
		4,/*ix for increasing x */
		0,0,/*lii,0 */
		18,20,/*lip 20 */
		53, /* */
		18,20,/* lip 20 */
		219,/*exam */
#else
		18,4,/*lip xl */
		2,255,/*lia 0 */
		219,/*exam */
		18,5,/*lip xh */
		2,255,/*lia 0 */
		219,/*exam */
/*400f x: */
		/* dump external memory */
		4, /*ix */
		87,/*                ldd */
#endif
		218,/*exab */



		0,4,/*lii 4 */

		/*a: */
		218,/*                exab */
		90,/*                 sl */
		218,/*                exab */
		18,94,/*            lip 94 */
		96,252,/*                 anma 252 */
		2,2, /*lia 2 */
		196,/*                adcm */
		95,/*                 outf */
		/*b:  */
		204,/*inb */
		102,128,/*tsia 0x80 */
#if 0
		41,4,/*            jnzm b */
#else
		/* input not working reliable! */
		/* so handshake removed, PC side must run with disabled */
		/* interrupt to not lose data */
		78,20, /*wait 20 */
#endif

		218,/*                exab */
		90,/*                 sl */
		218,/*                exab */
		18,94,/*            lip 94 */
		96,252,/*anma 252 */
		2,0,/*                lia 0 */
		196,/*adcm */
		95,/*                 outf */
		/*c:  */
		204,/*inb */
		102,128,/*tsia 0x80 */
#if 0
		57,4,/*            jzm c */
#else
		78,20, /*wait 20 */
#endif

		65,/*deci */
		41,34,/*jnzm a */

		41,41,/*jnzm x: */

		55,/*               rtn */
	};

	for (i=0; i<sizeof(sucker);i++) pc1401_mem[0x4000+i]=sucker[i];
	logerror("%d %d\n",i, 0x4000+i);
#endif

	for (i=0; i<128; i++)
		gfx[i]=i;

	timer_set(machine, ATTOTIME_IN_SEC(1), NULL, 0, pc1401_power_up);

	/* NPW 28-Jun-2006 - Input ports can't be read at init time! Even then, this should use mess_ram */
	if (0 && (input_port_read(machine, "DSW0") & 0xc0) == 0x80)
	{
		memory_install_write8_handler(space, 0x2000, 0x3fff, 0, 0, SMH_RAM);
	}
	else if (0 && (input_port_read(machine, "DSW0") & 0xc0) == 0x40)
	{
		memory_install_write8_handler(space, 0x2000, 0x37ff, 0, 0, SMH_NOP);
		memory_install_write8_handler(space,  0x3800, 0x3fff, 0, 0, SMH_RAM);
	}
	else
	{
		memory_install_write8_handler(space, 0x2000, 0x3fff, 0, 0, SMH_NOP);
	}
}


