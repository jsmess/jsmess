/***************************************************************************

	commodore c128 home computer

	peter.trauner@jk.uni-linz.ac.at
    documentation:
 	 iDOC (http://www.softwolves.pp.se/idoc)
           Christian Janoff  mepk@c64.org

***************************************************************************/

#include "driver.h"

#include "cpu/m6502/m6502.h"
#include "sound/sid6581.h"
#include "machine/6526cia.h"

#include "includes/cbm.h"
#include "includes/cbmserb.h"
#include "includes/cbmdrive.h"
#include "includes/vc1541.h"
#include "video/vic6567.h"
#include "video/vdc8563.h"


#include "includes/c128.h"
#include "includes/c64.h"

#include "devices/cassette.h"


#define VERBOSE_LEVEL 0
#define DBG_LOG( MACHINE, N, M, A ) \
	do { \
		if(VERBOSE_LEVEL >= N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s", attotime_to_double(timer_get_time(MACHINE)), (char*) M ); \
			logerror A; \
		} \
	} while (0)

/*
 two cpus share 1 memory system, only 1 cpu can run
 controller is the mmu

 mame has different memory subsystems for each cpu
*/


/* m8502 port
 in c128 mode
 bit 0 color
 bit 1 lores
 bit 2 hires
 3 textmode
 5 graphics (turned on ram at 0x1000 for video chip
*/

UINT8 *c128_basic;
UINT8 *c128_kernal;
UINT8 *c128_chargen;
UINT8 *c128_z80;
UINT8 *c128_editor;
UINT8 *c128_internal_function;
UINT8 *c128_external_function;
UINT8 *c128_vdcram;

static int c64mode = 0, c128_write_io;

static int c128_ram_bottom, c128_ram_top;
static UINT8 *c128_ram;

static UINT8 c64_port_data;

static UINT8 c128_keyline[3] = {0xff, 0xff, 0xff};

static int c128_va1617;
static int c128_cia1_on = 1;
static UINT8 serial_clock, serial_data, serial_atn;
static UINT8 vicirq = 0;
static int emulated_drive = 0;

static void c128_nmi(running_machine *machine)
{
	static int nmilevel = 0;
	const device_config *cia_1 = devtag_get_device(machine, "cia_1");
	int cia1irq = cia_get_irq(cia_1);

	if (nmilevel != (input_port_read(machine, "SPECIAL") & 0x80) || cia1irq)	/* KEY_RESTORE */
	{
		if (1) // this was never valid, there is no active CPU during a timer firing!  cpu_getactivecpu() == 0)
		{
			/* z80 */
			cputag_set_input_line(machine, "maincpu", INPUT_LINE_NMI, (input_port_read(machine, "SPECIAL") & 0x80) || cia1irq);
		}
		else
		{
			cputag_set_input_line(machine, "m8502", INPUT_LINE_NMI, (input_port_read(machine, "SPECIAL") & 0x80) || cia1irq);
		}

		nmilevel = (input_port_read(machine, "SPECIAL") & 0x80) || cia1irq;
	}
}

/***********************************************

	CIA Interfaces

***********************************************/

/*
 *	CIA 0 - Port A keyboard line select
 *	CIA 0 - Port B keyboard line read
 *
 *	flag cassette read input, serial request in
 *	irq to irq connected
 *
 *	see machine/cbm.c
 */

static READ8_DEVICE_HANDLER( c128_cia0_port_a_r )
{
	UINT8 cia0portb = cia_get_output_b(devtag_get_device(device->machine, "cia_0"));

	return common_cia0_port_a_r(device, cia0portb);
}

static READ8_DEVICE_HANDLER( c128_cia0_port_b_r )
{
    UINT8 value = 0xff;
	UINT8 cia0porta = cia_get_output_a(devtag_get_device(device->machine, "cia_0"));

	value &= common_cia0_port_b_r(device, cia0porta);

	if (!vic2e_k0_r())
		value &= c128_keyline[0];
	if (!vic2e_k1_r())
		value &= c128_keyline[1];
	if (!vic2e_k2_r())
		value &= c128_keyline[2];

    return value;
}

static WRITE8_DEVICE_HANDLER( c128_cia0_port_b_w )
{
    vic2_lightpen_write(data & 0x10);
}

static void c128_irq (running_machine *machine, int level)
{
	static int old_level = 0;

	if (level != old_level)
	{
		DBG_LOG(machine, 3, "mos6510", ("irq %s\n", level ? "start" : "end"));

		if (0) // && (cpu_getactivecpu() == 0))
		{
			cputag_set_input_line(machine, "maincpu", 0, level);
		}
		else
		{
			cputag_set_input_line(machine, "m8502", M6510_IRQ_LINE, level);
		}

		old_level = level;
	}
}

static void c128_cia0_interrupt (const device_config *device, int level)
{
	c128_irq (device->machine, level || vicirq);
}

static void c128_vic_interrupt (running_machine *machine, int level)
{
	const device_config *cia_0 = devtag_get_device(machine, "cia_0");
#if 1
	if (level != vicirq)
	{
		c128_irq (machine, level || cia_get_irq(cia_0));
		vicirq = level;
	}
#endif
}

const cia6526_interface c128_ntsc_cia0 =
{
	DEVCB_LINE(c128_cia0_interrupt),
	DEVCB_NULL,	/* pc_func */
	60,

	{
		{ DEVCB_HANDLER(c128_cia0_port_a_r), DEVCB_NULL },
		{ DEVCB_HANDLER(c128_cia0_port_b_r), DEVCB_HANDLER(c128_cia0_port_b_w) }
	}
};

const cia6526_interface c128_pal_cia0 =
{
	DEVCB_LINE(c128_cia0_interrupt),
	DEVCB_NULL,	/* pc_func */
	50,

	{
		{ DEVCB_HANDLER(c128_cia0_port_a_r), DEVCB_NULL },
		{ DEVCB_HANDLER(c128_cia0_port_b_r), DEVCB_HANDLER(c128_cia0_port_b_w) }
	}
};


/*
 * CIA 1 - Port A
 * bit 7 serial bus data input
 * bit 6 serial bus clock input
 * bit 5 serial bus data output
 * bit 4 serial bus clock output
 * bit 3 serial bus atn output
 * bit 2 rs232 data output
 * bits 1-0 vic-chip system memory bank select
 *
 * CIA 1 - Port B
 * bit 7 user rs232 data set ready
 * bit 6 user rs232 clear to send
 * bit 5 user
 * bit 4 user rs232 carrier detect
 * bit 3 user rs232 ring indicator
 * bit 2 user rs232 data terminal ready
 * bit 1 user rs232 request to send
 * bit 0 user rs232 received data
 *
 * flag restore key or rs232 received data input
 * irq to nmi connected ?
 */
static READ8_DEVICE_HANDLER( c128_cia1_port_a_r )
{
	UINT8 value = 0xff;
	const device_config *serbus = devtag_get_device(device->machine, "serial_bus");

	if (!serial_clock || !cbm_serial_clock_read(serbus, 0))
		value &= ~0x40;

	if (!serial_data || !cbm_serial_data_read(serbus, 0))
		value &= ~0x80;

	return value;
}

static WRITE8_DEVICE_HANDLER( c128_cia1_port_a_w )
{
	static const int helper[4] = {0xc000, 0x8000, 0x4000, 0x0000};
	const device_config *serbus = devtag_get_device(device->machine, "serial_bus");

	cbm_serial_clock_write(serbus, 0, serial_clock = !(data & 0x10));
	cbm_serial_data_write(serbus, 0, serial_data = !(data & 0x20));
	cbm_serial_atn_write(serbus, 0, serial_atn = !(data & 0x08));
	c64_vicaddr = c64_memory + helper[data & 0x03];
	c128_vicaddr = c64_memory + helper[data & 0x03] + c128_va1617;
}

static void c128_cia1_interrupt (const device_config *device, int level)
{
	c128_nmi(device->machine);
}

const cia6526_interface c128_ntsc_cia1 =
{
	DEVCB_LINE(c128_cia1_interrupt),
	DEVCB_NULL,	/* pc_func */
	60,

	{
		{ DEVCB_HANDLER(c128_cia1_port_a_r), DEVCB_HANDLER(c128_cia1_port_a_w) },
		{ DEVCB_NULL, DEVCB_NULL }
	}
};

const cia6526_interface c128_pal_cia1 =
{
	DEVCB_LINE(c128_cia1_interrupt),
	DEVCB_NULL,	/* pc_func */
	50,

	{
		{ DEVCB_HANDLER(c128_cia1_port_a_r), DEVCB_HANDLER(c128_cia1_port_a_w) },
		{ DEVCB_NULL, DEVCB_NULL }
	}
};

/***********************************************

	Memory Handlers

***********************************************/

static void c128_set_m8502_read_handler(running_machine *machine, UINT16 start, UINT16 end, read8_space_func rh)
{
	const device_config *cpu;
	cpu = cputag_get_cpu(machine, "m8502");
	memory_install_read8_handler(cpu_get_address_space(cpu, ADDRESS_SPACE_PROGRAM), start, end, 0, 0, rh);
}

static WRITE8_HANDLER(c128_dma8726_port_w)
{
	DBG_LOG(space->machine, 1, "dma write", ("%.3x %.2x\n",offset,data));
}

static READ8_HANDLER(c128_dma8726_port_r)
{
	DBG_LOG(space->machine, 1, "dma read", ("%.3x\n",offset));
	return 0xff;
}

WRITE8_HANDLER( c128_write_d000 )
{
	const device_config *cia_0 = devtag_get_device(space->machine, "cia_0");
	const device_config *cia_1 = devtag_get_device(space->machine, "cia_1");
	const device_config *sid = devtag_get_device(space->machine, "sid6581");

	UINT8 c64_port6510 = (UINT8) devtag_get_info_int(space->machine, "maincpu", CPUINFO_INT_M6510_PORT);

	if (!c128_write_io)
	{
		if (offset + 0xd000 >= c128_ram_top)
			c64_memory[0xd000 + offset] = data;
		else
			c128_ram[0xd000 + offset] = data;
	}
	else
	{
		switch ((offset&0xf00)>>8)
		{
		case 0:case 1: case 2: case 3:
			vic2_port_w(space, offset & 0x3ff, data);
			break;
		case 4:
			sid6581_w(sid, offset & 0x3f, data);
			break;
		case 5:
			c128_mmu8722_port_w(space, offset & 0xff, data);
			break;
		case 6:case 7:
			vdc8563_port_w(space, offset & 0xff, data);
			break;
		case 8: case 9: case 0xa: case 0xb:
		    if (c64mode)
				c64_colorram[(offset & 0x3ff)] = data | 0xf0;
		    else
				c64_colorram[(offset & 0x3ff)|((c64_port6510&3)<<10)] = data | 0xf0; // maybe all 8 bit connected!
		    break;
		case 0xc:
			cia_w(cia_0, offset, data);
			break;
		case 0xd:
			cia_w(cia_1, offset, data);
			break;
		case 0xf:
			c128_dma8726_port_w(space, offset&0xff,data);
			break;
		case 0xe:
			DBG_LOG(space->machine, 1, "io write", ("%.3x %.2x\n", offset, data));
			break;
		}
	}
}



static READ8_HANDLER( c128_read_io )
{
	const device_config *cia_0 = devtag_get_device(space->machine, "cia_0");
	const device_config *cia_1 = devtag_get_device(space->machine, "cia_1");
	const device_config *sid = devtag_get_device(space->machine, "sid6581");

	if (offset < 0x400)
		return vic2_port_r(space, offset & 0x3ff);
	else if (offset < 0x500)
		return sid6581_r(sid, offset & 0xff);
	else if (offset < 0x600)
		return c128_mmu8722_port_r(space, offset & 0xff);
	else if (offset < 0x800)
		return vdc8563_port_r(space, offset & 0xff);
	else if (offset < 0xc00)
		return c64_colorram[offset & 0x3ff];
	else if (offset == 0xc00)
		{
			cia_set_port_mask_value(cia_0, 0, input_port_read(space->machine, "CTRLSEL") & 0x80 ? c64_keyline[8] : c64_keyline[9] );
			return cia_r(cia_0, offset);
		}
	else if (offset == 0xc01)
		{
			cia_set_port_mask_value(cia_0, 1, input_port_read(space->machine, "CTRLSEL") & 0x80 ? c64_keyline[9] : c64_keyline[8] );
			return cia_r(cia_0, offset);
		}
	else if (offset < 0xd00)
		return cia_r(cia_0, offset);
	else if (offset < 0xe00)
		return cia_r(cia_1, offset);
	else if ((offset >= 0xf00) & (offset <= 0xfff))
		return c128_dma8726_port_r(space, offset&0xff);
	DBG_LOG(space->machine, 1, "io read", ("%.3x\n", offset));
	return 0xff;
}

void c128_bankswitch_64(running_machine *machine, int reset)
{
	static int old, exrom, game;
	int data, loram, hiram, charen;
	read8_space_func rh;

	if (!c64mode)
		return;

	data = (UINT8) devtag_get_info_int(machine, "maincpu", CPUINFO_INT_M6510_PORT) & 0x07;
	if ((data == old) && (exrom == c64_exrom) && (game == c64_game) && !reset)
		return;

	DBG_LOG(machine, 1, "bankswitch", ("%d\n", data & 7));
	loram = (data & 1) ? 1 : 0;
	hiram = (data & 2) ? 1 : 0;
	charen = (data & 4) ? 1 : 0;

	if ((!c64_game && c64_exrom)
		|| (loram && hiram && !c64_exrom))
	{
		memory_set_bankptr (machine, 8, c64_roml);
	}
	else
	{
		memory_set_bankptr (machine, 8, c64_memory + 0x8000);
	}

	if ((!c64_game && c64_exrom && hiram)
		|| (!c64_exrom))
	{
		memory_set_bankptr (machine, 9, c64_romh);
	}
	else if (loram && hiram)
	{
		memory_set_bankptr (machine, 9, c64_basic);
	}
	else
	{
		memory_set_bankptr (machine, 9, c64_memory + 0xa000);
	}

	if ((!c64_game && c64_exrom)
		|| (charen && (loram || hiram)))
	{
		rh = c128_read_io;
		c128_write_io = 1;
	}
	else
	{
		rh = SMH_BANK(5);
		c128_write_io = 0;
		if ((!charen && (loram || hiram)))
			memory_set_bankptr (machine, 13, c64_chargen);
		else
			memory_set_bankptr (machine, 13, c64_memory + 0xd000);
	}
	c128_set_m8502_read_handler(machine, 0xd000, 0xdfff, rh);

	if (!c64_game && c64_exrom)
	{
		memory_set_bankptr (machine, 14, c64_romh);
		memory_set_bankptr (machine, 15, c64_romh+0x1f00);
		memory_set_bankptr (machine, 16, c64_romh+0x1f05);
	}
	else
	{
		if (hiram)
		{
			memory_set_bankptr (machine, 14, c64_kernal);
			memory_set_bankptr (machine, 15, c64_kernal+0x1f00);
			memory_set_bankptr (machine, 16, c64_kernal+0x1f05);
		}
		else
		{
			memory_set_bankptr (machine, 14, c64_memory + 0xe000);
			memory_set_bankptr (machine, 15, c64_memory + 0xff00);
			memory_set_bankptr (machine, 16, c64_memory + 0xff05);
		}
	}
	old = data;
	exrom= c64_exrom;
	game=c64_game;
}

static UINT8 c128_mmu[0x0b];
static const int c128_mmu_helper[4] =
{0x400, 0x1000, 0x2000, 0x4000};
static int mmu_cpu=0;
static int mmu_page0, mmu_page1;

#define MMU_PAGE1 ((((c128_mmu[10]&0xf)<<8)|c128_mmu[9])<<8)
#define MMU_PAGE0 ((((c128_mmu[8]&0xf)<<8)|c128_mmu[7])<<8)
#define MMU_VIC_ADDR ((c128_mmu[6]&0xc0)<<10)
#define MMU_RAM_RCR_ADDR ((c128_mmu[6]&0x30)<<14)
#define MMU_SIZE (c128_mmu_helper[c128_mmu[6]&3])
#define MMU_BOTTOM (c128_mmu[6]&4)
#define MMU_TOP (c128_mmu[6]&8)
#define MMU_CPU8502 (c128_mmu[5]&1)	   /* else z80 */
/* fastio output (c128_mmu[5]&8) else input */
#define MMU_GAME_IN (c128_mmu[5]&0x10)
#define MMU_EXROM_IN (c128_mmu[5]&0x20)
#define MMU_64MODE (c128_mmu[5]&0x40)
#define MMU_40_IN (c128_mmu[5]&0x80)

#define MMU_RAM_CR_ADDR ((c128_mmu[0]&0xc0)<<10)
#define MMU_RAM_LO (c128_mmu[0]&2)	   /* else rom at 0x4000 */
#define MMU_RAM_MID ((c128_mmu[0]&0xc)==0xc)	/* 0x8000 - 0xbfff */
#define MMU_ROM_MID ((c128_mmu[0]&0xc)==0)
#define MMU_EXTERNAL_ROM_MID ((c128_mmu[0]&0xc)==8)
#define MMU_INTERNAL_ROM_MID ((c128_mmu[0]&0xc)==4)

#define MMU_IO_ON (!(c128_mmu[0]&1))   /* io window at 0xd000 */
#define MMU_ROM_HI ((c128_mmu[0]&0x30)==0)	/* rom at 0xc000 */
#define MMU_EXTERNAL_ROM_HI ((c128_mmu[0]&0x30)==0x20)
#define MMU_INTERNAL_ROM_HI ((c128_mmu[0]&0x30)==0x10)
#define MMU_RAM_HI ((c128_mmu[0]&0x30)==0x30)

#define MMU_RAM_ADDR (MMU_RAM_RCR_ADDR|MMU_RAM_CR_ADDR)

/* typical z80 configuration
   0x3f 0x3f 0x7f 0x3e 0x7e 0xb0 0x0b 0x00 0x00 0x01 0x00 */
static void c128_bankswitch_z80(running_machine *machine)
{
	 c128_ram = c64_memory + MMU_RAM_ADDR;
	 c128_va1617 = MMU_VIC_ADDR;
#if 1
	 memory_set_bankptr (machine, 10, c128_z80);
	 memory_set_bankptr (machine, 11, c128_ram + 0x1000);
	 if ( (( (input_port_read(machine, "SPECIAL") & 0x06) == 0x02 ) && (MMU_RAM_ADDR >= 0x40000))
		  || (( (input_port_read(machine, "SPECIAL") & 0x06) == 0x00) && (MMU_RAM_ADDR >= 0x20000)) )
		 c128_ram = NULL;
#else
	 if (MMU_BOTTOM)
		 c128_ram_bottom = MMU_SIZE;
	 else
		 c128_ram_bottom = 0;

	 if (MMU_RAM_ADDR==0) { /* this is used in z80 mode for rom on/off switching !*/
		 memory_set_bankptr (machine, 10, c128_z80);
		 memory_set_bankptr (machine, 11, c128_z80 + 0x400);
	 } 
	 else 
	 {
		 memory_set_bankptr (machine, 10, (c128_ram_bottom > 0 ? c64_memory : c128_ram));
		 memory_set_bankptr (machine, 11, (c128_ram_bottom > 0x400 ? c64_memory : c128_ram) + 0x400);
	 }
	 
	 memory_set_bankptr (machine, 1, (c128_ram_bottom > 0 ? c64_memory : c128_ram));
	 memory_set_bankptr (machine, 2, (c128_ram_bottom > 0x400 ? c64_memory : c128_ram) + 0x400);

	 memory_set_bankptr (machine, 3, (c128_ram_bottom > 0x1000 ? c64_memory : c128_ram) + 0x1000);
	 memory_set_bankptr (machine, 4, (c128_ram_bottom > 0x2000 ? c64_memory : c128_ram) + 0x2000);
	 memory_set_bankptr (machine, 5, c128_ram + 0x4000);

	 if (MMU_TOP)
		 c128_ram_top = 0x10000 - MMU_SIZE;
	 else
		 c128_ram_top = 0x10000;

	 if (c128_ram_top > 0xc000) 
	 { 
		memory_set_bankptr (machine, 6, c128_ram + 0xc000); 
	 }
	 else 
	 { 
		memory_set_bankptr (machine, 6, c64_memory + 0xc000); 
	 }

	 if (c128_ram_top > 0xe000) 
	 { 
		memory_set_bankptr (machine, 7, c128_ram + 0xe000); 
	 }
	 else 
	 { 
		memory_set_bankptr (machine, 7, c64_memory + 0xd000); 
	 }

	 if (c128_ram_top > 0xf000) 
	 { 
		memory_set_bankptr (machine, 8, c128_ram + 0xf000); 
	 }
	 else 
	 { 
		memory_set_bankptr (machine, 8, c64_memory + 0xe000); 
	 }

	 if (c128_ram_top > 0xff05) 
	 { 
		memory_set_bankptr (machine, 9, c128_ram + 0xff05); 
	 }
	 else 
	 { 
		memory_set_bankptr (machine, 9, c64_memory + 0xff05); 
	 }

	 if ( (( (input_port_read(machine, "SPECIAL") & 0x06) == 0x02 ) && (MMU_RAM_ADDR >= 0x40000))
		  || (( (input_port_read(machine, "SPECIAL") & 0x06) == 0x00) && (MMU_RAM_ADDR >= 0x20000)) )
		 c128_ram = NULL;
#endif
}

static void c128_bankswitch_128 (running_machine *machine, int reset)
{
	read8_space_func rh;

	c64mode = MMU_64MODE;
	if (c64mode)
	{
		/* mmu works also in c64 mode, but this can wait */
		c128_ram = c64_memory;
		c128_va1617 = 0;
		c128_ram_bottom = 0;
		c128_ram_top = 0x10000;

		memory_set_bankptr (machine, 1, c64_memory);
		memory_set_bankptr (machine, 2, c64_memory + 0x100);

		memory_set_bankptr (machine, 3, c64_memory + 0x200);
		memory_set_bankptr (machine, 4, c64_memory + 0x400);
		memory_set_bankptr (machine, 5, c64_memory + 0x1000);
		memory_set_bankptr (machine, 6, c64_memory + 0x2000);

		memory_set_bankptr (machine, 7, c64_memory + 0x4000);

		memory_set_bankptr (machine, 12, c64_memory + 0xc000);

		c128_bankswitch_64 (machine, reset);
	}
	else
	{
		c128_ram = c64_memory + MMU_RAM_ADDR;
		c128_va1617 = MMU_VIC_ADDR;
		memory_set_bankptr (machine, 1, c64_memory + mmu_page0);
		memory_set_bankptr (machine, 2, c64_memory + mmu_page1);
		if (MMU_BOTTOM)
			{
				c128_ram_bottom = MMU_SIZE;
			}
		else
			c128_ram_bottom = 0;
		memory_set_bankptr (machine, 3, (c128_ram_bottom > 0x200 ? c64_memory : c128_ram) + 0x200);
		memory_set_bankptr (machine, 4, (c128_ram_bottom > 0x400 ? c64_memory : c128_ram) + 0x400);
		memory_set_bankptr (machine, 5, (c128_ram_bottom > 0x1000 ? c64_memory : c128_ram) + 0x1000);
		memory_set_bankptr (machine, 6, (c128_ram_bottom > 0x2000 ? c64_memory : c128_ram) + 0x2000);

		if (MMU_RAM_LO)
		{
			memory_set_bankptr (machine, 7, c128_ram + 0x4000);
		}
		else
		{
			memory_set_bankptr (machine, 7, c128_basic);
		}

		if (MMU_RAM_MID)
		{
			memory_set_bankptr (machine, 8, c128_ram + 0x8000);
			memory_set_bankptr (machine, 9, c128_ram + 0xa000);
		}
		else if (MMU_ROM_MID)
		{
			memory_set_bankptr (machine, 8, c128_basic + 0x4000);
			memory_set_bankptr (machine, 9, c128_basic + 0x6000);
		}
		else if (MMU_INTERNAL_ROM_MID)
		{
			memory_set_bankptr (machine, 8, c128_internal_function);
			memory_set_bankptr (machine, 9, c128_internal_function + 0x2000);
		}
		else
		{
			memory_set_bankptr (machine, 8, c128_external_function);
			memory_set_bankptr (machine, 9, c128_external_function + 0x2000);
		}

		if (MMU_TOP)
		{
			c128_ram_top = 0x10000 - MMU_SIZE;
		}
		else
			c128_ram_top = 0x10000;

		c128_set_m8502_read_handler(machine, 0xff00, 0xff04, c128_mmu8722_ff00_r);

		if (MMU_IO_ON)
		{
			rh = c128_read_io;
			c128_write_io = 1;
		}
		else
		{
			c128_write_io = 0;
			rh = SMH_BANK(13);
		}
		c128_set_m8502_read_handler(machine, 0xd000, 0xdfff, rh);

		if (MMU_RAM_HI)
		{
			if (c128_ram_top > 0xc000)
			{
				memory_set_bankptr (machine, 12, c128_ram + 0xc000);
			}
			else
			{
				memory_set_bankptr (machine, 12, c64_memory + 0xc000);
			}
			if (!MMU_IO_ON)
			{
				if (c128_ram_top > 0xd000)
				{
					memory_set_bankptr (machine, 13, c128_ram + 0xd000);
				}
				else
				{
					memory_set_bankptr (machine, 13, c64_memory + 0xd000);
				}
			}
			if (c128_ram_top > 0xe000)
			{
				memory_set_bankptr (machine, 14, c128_ram + 0xe000);
			}
			else
			{
				memory_set_bankptr (machine, 14, c64_memory + 0xe000);
			}
			if (c128_ram_top > 0xff05)
			{
				memory_set_bankptr (machine, 16, c128_ram + 0xff05);
			}
			else
			{
				memory_set_bankptr (machine, 16, c64_memory + 0xff05);
			}
		}
		else if (MMU_ROM_HI)
		{
			memory_set_bankptr (machine, 12, c128_editor);
			if (!MMU_IO_ON) {
				memory_set_bankptr (machine, 13, c128_chargen);
			}
			memory_set_bankptr (machine, 14, c128_kernal);
			memory_set_bankptr (machine, 16, c128_kernal + 0x1f05);
		}
		else if (MMU_INTERNAL_ROM_HI)
		{
			memory_set_bankptr (machine, 12, c128_internal_function);
			if (!MMU_IO_ON) {
				memory_set_bankptr (machine, 13, c128_internal_function + 0x1000);
			}
			memory_set_bankptr (machine, 14, c128_internal_function + 0x2000);
			memory_set_bankptr (machine, 16, c128_internal_function + 0x3f05);
		}
		else					   /*if (MMU_EXTERNAL_ROM_HI) */
		{
			memory_set_bankptr (machine, 12, c128_external_function);
			if (!MMU_IO_ON) {
				memory_set_bankptr (machine, 13, c128_external_function + 0x1000);
			}
			memory_set_bankptr (machine, 14, c128_external_function + 0x2000);
			memory_set_bankptr (machine, 16, c128_external_function + 0x3f05);
		}

		if ( (( (input_port_read(machine, "SPECIAL") & 0x06) == 0x02 ) && (MMU_RAM_ADDR >= 0x40000))
				|| (( (input_port_read(machine, "SPECIAL") & 0x06) == 0x00) && (MMU_RAM_ADDR >= 0x20000)) )
			c128_ram = NULL;
	}
}

// 128u4
// FIX-ME: are the bankswitch functions working in the expected way without the memory_set_context?
static void c128_bankswitch (running_machine *machine, int reset)
{
	if (mmu_cpu != MMU_CPU8502)
	{
		if (!MMU_CPU8502)
		{
//			DBG_LOG(machine, 1, "switching to z80", ("active %d\n",cpu_getactivecpu()) );
//			memory_set_context(machine, 0);
			c128_bankswitch_z80(machine);
//			memory_set_context(machine, 1);
			cputag_set_input_line(machine, "maincpu", INPUT_LINE_HALT, CLEAR_LINE);
			cputag_set_input_line(machine, "m8502", INPUT_LINE_HALT, ASSERT_LINE);
		}
		else
		{
//			DBG_LOG(machine, 1, "switching to m6502", ("active %d\n",cpu_getactivecpu()) );
//			memory_set_context(machine, 1);
			c128_bankswitch_128(machine, reset);
//			memory_set_context(machine, 0);
			cputag_set_input_line(machine, "maincpu", INPUT_LINE_HALT, ASSERT_LINE);
			cputag_set_input_line(machine, "m8502", INPUT_LINE_HALT, CLEAR_LINE);

			/* NPW 19-Nov-2005 - In the C128, CPU #0 starts out and hands over
			 * control to CPU #1.  CPU #1 seems to execute garbage from 0x0000
			 * up to 0x1100, at which point it finally hits some code
			 * (presumably set up by CPU #1.) This always worked, but when I
			 * changed the m8502 CPU core to use an internal memory map, it
			 * started BRK-ing forever when trying to execute 0x0000.
			 *
			 * I am not sure whether the C128 actually executes this invalid
			 * memory or if this is a bug in the C128 driver.  In any case, the
			 * driver used to work with this behavior, so I am doing this hack
			 * where I set CPU #1's PC to 0x1100 on reset.
			 */
			if (cpu_get_reg(cputag_get_cpu(machine, "m8502"), REG_GENPC) == 0x0000)
				cpu_set_reg(cputag_get_cpu(machine, "m8502"), REG_GENPC, 0x1100);
		}
		mmu_cpu = MMU_CPU8502;
		return;
	}
	if (!MMU_CPU8502)
		c128_bankswitch_z80(machine);
	else
		c128_bankswitch_128(machine, reset);
}

static void c128_mmu8722_reset (running_machine *machine)
{
	memset(c128_mmu, 0, sizeof (c128_mmu));
	c128_mmu[5] |= 0x38;
	c128_mmu[10] = 1;
	mmu_cpu = 0;
    mmu_page0 = 0;
    mmu_page1 = 0x0100;
	c128_bankswitch (machine, 1);
}

WRITE8_HANDLER( c128_mmu8722_port_w )
{
	offset &= 0xf;
	switch (offset)
	{
	case 1:
	case 2:
	case 3:
	case 4:
	case 8:
	case 10:
		c128_mmu[offset] = data;
		break;
	case 0:
	case 5:
	case 6:
		c128_mmu[offset] = data;
		c128_bankswitch (space->machine, 0);
		break;
	case 7:
		c128_mmu[offset] = data;
		mmu_page0=MMU_PAGE0;
		break;
	case 9:
		c128_mmu[offset] = data;
		mmu_page1=MMU_PAGE1;
		c128_bankswitch (space->machine, 0);
		break;
	case 0xb:
		break;
	case 0xc:
	case 0xd:
	case 0xe:
	case 0xf:
		break;
	}
}

READ8_HANDLER( c128_mmu8722_port_r )
{
	int data;

	offset &= 0x0f;
	switch (offset)
	{
	case 5:
		data = c128_mmu[offset] | 6;
		if ( /*disk enable signal */ 0)
			data &= ~8;
		if (!c64_game)
			data &= ~0x10;
		if (!c64_exrom)
			data &= ~0x20;
		if (input_port_read(space->machine, "SPECIAL") & 0x10)
			data &= ~0x80;
		break;
	case 0xb:
		/* hinybble number of 64 kb memory blocks */
		if ((input_port_read(space->machine, "SPECIAL") & 0x06) == 0x02)		// 256KB RAM
			data = 0x4f;
		else if ((input_port_read(space->machine, "SPECIAL") & 0x06) == 0x04)	//	1MB
			data = 0xf;
		else 
			data = 0x2f;
		break;
	case 0xc:
	case 0xd:
	case 0xe:
	case 0xf:
		data=0xff;
		break;
	default:
		data=c128_mmu[offset];
	}
	return data;
}

WRITE8_HANDLER( c128_mmu8722_ff00_w )
{
	switch (offset)
	{
	case 0:
		c128_mmu[offset] = data;
		c128_bankswitch (space->machine, 0);
		break;
	case 1:
	case 2:
	case 3:
	case 4:
#if 1
		c128_mmu[0]= c128_mmu[offset];
#else
		c128_mmu[0]|= c128_mmu[offset];
#endif
		c128_bankswitch (space->machine, 0);
		break;
	}
}

 READ8_HANDLER( c128_mmu8722_ff00_r )
{
	return c128_mmu[offset];
}

WRITE8_HANDLER( c128_write_0000 )
{
	if (c128_ram!=NULL)
		c128_ram[0x0000 + offset] = data;
}

WRITE8_HANDLER( c128_write_1000 )
{
	if (c128_ram!=NULL)
		c128_ram[0x1000 + offset] = data;
}

WRITE8_HANDLER( c128_write_4000 )
{
	if (c128_ram!=NULL)
		c128_ram[0x4000 + offset] = data;
}

WRITE8_HANDLER( c128_write_8000 )
{
	if (c128_ram!=NULL)
		c128_ram[0x8000 + offset] = data;
}

WRITE8_HANDLER( c128_write_a000 )
{
	if (c128_ram!=NULL)
		c128_ram[0xa000 + offset] = data;
}

WRITE8_HANDLER( c128_write_c000 )
{
	if (c128_ram!=NULL)
		c128_ram[0xc000 + offset] = data;
}

WRITE8_HANDLER( c128_write_e000 )
{
	if (offset + 0xe000 >= c128_ram_top)
		c64_memory[0xe000 + offset] = data;
	else if (c128_ram!=NULL)
		c128_ram[0xe000 + offset] = data;
}

WRITE8_HANDLER( c128_write_ff00 )
{
	if (!c64mode)
		c128_mmu8722_ff00_w (space, offset, data);
	else if (c128_ram!=NULL)
		c64_memory[0xff00 + offset] = data;
}

WRITE8_HANDLER( c128_write_ff05 )
{
	if (offset + 0xff05 >= c128_ram_top)
		c64_memory[0xff05 + offset] = data;
	else if (c128_ram!=NULL)
		c128_ram[0xff05 + offset] = data;
}

/*
 * only 14 address lines
 * a15 and a14 portlines
 * 0x1000-0x1fff, 0x9000-0x9fff char rom
 */
static int c128_dma_read(running_machine *machine, int offset)
{
	UINT8 c64_port6510 = (UINT8) devtag_get_info_int(machine, "maincpu", CPUINFO_INT_M6510_PORT);

	/* main memory configuration to include */
	if (c64mode)
	{
		if (!c64_game && c64_exrom)
		{
			if (offset < 0x3000)
				return c64_memory[offset];
			return c64_romh[offset & 0x1fff];
		}
		if (((c64_vicaddr - c64_memory + offset) & 0x7000) == 0x1000)
			return c64_chargen[offset & 0xfff];
		return c64_vicaddr[offset];
	}
	if ( !(c64_port6510&4)
		 && (((c128_vicaddr - c64_memory + offset) & 0x7000) == 0x1000) )
		return c128_chargen[offset & 0xfff];
	return c128_vicaddr[offset];
}

static int c128_dma_read_color(running_machine *machine, int offset)
{
	UINT8 c64_port6510 = (UINT8) devtag_get_info_int(machine, "maincpu", CPUINFO_INT_M6510_PORT);

	if (c64mode)
		return c64_colorram[offset & 0x3ff] & 0xf;
	else
		return c64_colorram[(offset & 0x3ff)|((c64_port6510&0x3)<<10)] & 0xf;
}

/* 2008-09-01
	We need here the m6510 port handlers from c64, otherwise c128_common_driver_init
	seems unable to use correctly the timer.
	This will be probably fixed in a future clean up.
*/

static emu_timer *datasette_timer;

void c128_m6510_port_write(const device_config *device, UINT8 direction, UINT8 data)
{
	/* if line is marked as input then keep current value */
	data = (c64_port_data & ~direction) | (data & direction);

	/* resistors make P0,P1,P2 go high when respective line is changed to input */
	if (!(direction & 0x04)) 
		data |= 0x04;

	if (!(direction & 0x02))
		data |= 0x02;

	if (!(direction & 0x01))
		data |= 0x01;

	c64_port_data = data;

	if (c64_tape_on)
	{
		if (direction & 0x08) 
		{
			cassette_output(devtag_get_device(device->machine, "cassette"), (data & 0x08) ? -(0x5a9e >> 1) : +(0x5a9e >> 1));
		}

		if (direction & 0x20)
		{
			if(!(data & 0x20))
			{
				cassette_change_state(devtag_get_device(device->machine, "cassette"),CASSETTE_MOTOR_ENABLED,CASSETTE_MASK_MOTOR);
				timer_adjust_periodic(datasette_timer, attotime_zero, 0, ATTOTIME_IN_HZ(44100));
			}
			else
			{
				cassette_change_state(devtag_get_device(device->machine, "cassette"),CASSETTE_MOTOR_DISABLED ,CASSETTE_MASK_MOTOR);
				timer_reset(datasette_timer, attotime_never);
			}
		}
	}

	c128_bankswitch_64(device->machine, 0);

	c64_memory[0x000] = memory_read_byte( cpu_get_address_space(device, ADDRESS_SPACE_PROGRAM), 0 );
	c64_memory[0x001] = memory_read_byte( cpu_get_address_space(device, ADDRESS_SPACE_PROGRAM), 1 );
}

UINT8 c128_m6510_port_read(const device_config *device, UINT8 direction)
{
	UINT8 data = c64_port_data;

	if ((cassette_get_state(devtag_get_device(device->machine, "cassette")) & CASSETTE_MASK_UISTATE) != CASSETTE_STOPPED)
		data &= ~0x10;
	else
		data |=  0x10;

	if (input_port_read(device->machine, "SPECIAL") & 0x20)		/* Check Caps Lock */
		data &= ~0x40;
	else
		data |=  0x40;

	return data;
}

static void c128_common_driver_init(running_machine *machine)
{
	UINT8 *gfx=memory_region(machine, "gfx1");
	UINT8 *ram = memory_region(machine, "maincpu");
	int i;

	c64_memory = ram;

	c128_basic = ram+0x100000;
	c64_basic = ram+0x108000;
	c64_kernal = ram+0x10a000;
	c128_editor = ram+0x10c000;
	c128_z80 = ram+0x10d000;
	c128_kernal = ram+0x10e000;
	c128_internal_function = ram+0x110000;
	c128_external_function = ram+0x118000;
	c64_chargen = ram+0x120000;
	c128_chargen = ram+0x121000;
	c64_colorram = ram+0x122000;
	c128_vdcram = ram+0x122800;

	for (i=0; i<0x100; i++)
		gfx[i]=i;

	if (c64_tape_on)
		datasette_timer = timer_alloc(machine, c64_tape_timer, NULL);
}

DRIVER_INIT( c128 )
{
	c64_tape_on = 1;
	c128_common_driver_init(machine);
	vic6567_init(1, c64_pal, c128_dma_read, c128_dma_read_color, c128_vic_interrupt);
	vic2_set_rastering(0);
	vdc8563_init(0);
	vdc8563_set_rastering(1);
}

DRIVER_INIT( c128pal )
{
	c64_tape_on = 1;
	c64_pal = 1;
	c128_common_driver_init(machine);
	vic6567_init(1, c64_pal, c128_dma_read, c128_dma_read_color, c128_vic_interrupt);
	vic2_set_rastering(1);
	vdc8563_init(0);
	vdc8563_set_rastering(0);
}

DRIVER_INIT( c128d )
{
	DRIVER_INIT_CALL( c128 );
	emulated_drive = 1;
	drive_config(machine, type_1571, 0, 0, "cpu_vc1571", 8);
}

DRIVER_INIT( c128dpal )
{
	DRIVER_INIT_CALL( c128pal );
	emulated_drive = 1;
	drive_config(machine, type_1571, 0, 0, "cpu_vc1571", 8);
}

DRIVER_INIT( c128dcr )
{
	DRIVER_INIT_CALL( c128 );
	emulated_drive = 1;
	drive_config(machine, type_1571cr, 0, 0, "cpu_vc1571", 8);
}

DRIVER_INIT( c128dcrp )
{
	DRIVER_INIT_CALL( c128pal );
	emulated_drive = 1;
	drive_config(machine, type_1571cr, 0, 0, "cpu_vc1571", 8);
}

DRIVER_INIT( c128d81 )
{
	DRIVER_INIT_CALL( c128 );
	emulated_drive = 1;
	drive_config(machine, type_1581, 0, 0, "cpu_vc1571", 8);
}

MACHINE_START( c128 )
{
	if (emulated_drive)
	{
		drive_reset();
	}
	else if (c128_cia1_on)
	{
		cbm_drive_0_config(SERIAL, 8);
		cbm_drive_1_config(SERIAL, 9);
		serial_clock = serial_data = serial_atn = 1;
	}

// This was in MACHINE_START( c64 ), but never called
// TO DO: find its correct use, when fixing c64 mode
	if (c64mode)
		c128_bankswitch_64(machine, 1);
}

MACHINE_RESET( c128 )
{
	c128_vicaddr = c64_vicaddr = c64_memory;
	c64mode = 0;
	c128_mmu8722_reset (machine);
	cputag_set_input_line(machine, "maincpu", INPUT_LINE_HALT, CLEAR_LINE);
	cputag_set_input_line(machine, "m8502", INPUT_LINE_HALT, ASSERT_LINE);
}


INTERRUPT_GEN( c128_frame_interrupt )
{
	static int monitor = -1;
	static const char *const c128ports[] = { "KP0", "KP1", "KP2" };
	int i, value;

	c128_nmi(device->machine);

	if ((input_port_read(device->machine, "SPECIAL") & 0x08) != monitor)
	{
		if (input_port_read(device->machine, "SPECIAL") & 0x08)
		{
			vic2_set_rastering(0);
			vdc8563_set_rastering(1);
			video_screen_set_visarea(device->machine->primary_screen, 0, 655, 0, 215);
		}
		else
		{
			vic2_set_rastering(1);
			vdc8563_set_rastering(0);
			if (c64_pal)
				video_screen_set_visarea(device->machine->primary_screen, 0, VIC6569_VISIBLECOLUMNS - 1, 0, VIC6569_VISIBLELINES - 1);
			else
				video_screen_set_visarea(device->machine->primary_screen, 0, VIC6567_VISIBLECOLUMNS - 1, 0, VIC6567_VISIBLELINES - 1);
		}
		monitor = input_port_read(device->machine, "SPECIAL") & 0x08;
	}


	/* common keys input ports */
	cbm_common_interrupt(device);

	/* Fix Me! Currently, neither left Shift nor Shift Lock work in c128, but reading the correspondent input produces a bug!
	Hence, we overwrite the actual reading as it never happens */
	if ((input_port_read(device->machine, "SPECIAL") & 0x40))	// 
		c64_keyline[1] |= 0x80;

	/* c128 specific: keypad input ports */
	for (i = 0; i < 3; i++)
	{
		value = 0xff;
		value &= ~input_port_read(device->machine, c128ports[i]);
		c128_keyline[i] = value;
	}
}


VIDEO_START( c128 )
{
	VIDEO_START_CALL(vdc8563);
	VIDEO_START_CALL(vic2);
}

VIDEO_UPDATE( c128 )
{
	VIDEO_UPDATE_CALL(vdc8563);
	VIDEO_UPDATE_CALL(vic2);
	return 0;
}
