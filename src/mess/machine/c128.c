/***************************************************************************

	commodore c128 home computer

	peter.trauner@jk.uni-linz.ac.at
    documentation:
 	 iDOC (http://www.softwolves.pp.se/idoc)
           Christian Janoff  mepk@c64.org

***************************************************************************/
#include <ctype.h>
#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "sound/sid6581.h"
#include "mscommon.h"
#include "machine/6526cia.h"

#define VERBOSE_DBG 1
#include "includes/cbm.h"
#include "includes/cbmserb.h"
#include "includes/vc1541.h"
#include "includes/vc20tape.h"
#include "includes/vic6567.h"
#include "includes/vdc8563.h"

#include "includes/c128.h"


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

static void c128_set_m8502_read_handler(UINT16 start, UINT16 end, read8_handler rh)
{
	int cpunum;
	cpunum = mame_find_cpu_index(Machine, "m8502");
	memory_install_read8_handler(cpunum, ADDRESS_SPACE_PROGRAM, start, end, 0, 0, rh);
}



int c128_capslock_r (void)
{
	return !KEY_DIN;
}

static WRITE8_HANDLER(c128_dma8726_port_w)
{
	DBG_LOG(1,"dma write",("%.3x %.2x\n",offset,data));
}

static  READ8_HANDLER(c128_dma8726_port_r)
{
	DBG_LOG(1,"dma read",("%.3x\n",offset));
	return 0xff;
}

WRITE8_HANDLER( c128_mmu8722_port_w );
READ8_HANDLER( c128_mmu8722_port_r );

WRITE8_HANDLER( c128_write_d000 )
{
	UINT8 c64_port6510 = (UINT8) cpunum_get_info_int(0, CPUINFO_INT_M6510_PORT);

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
			vic2_port_w (offset & 0x3ff, data);
			break;
		case 4:
			sid6581_0_port_w (offset & 0x3f, data);
			break;
		case 5:
			c128_mmu8722_port_w (offset & 0xff, data);
			break;
		case 6:case 7:
			vdc8563_port_w (offset & 0xff, data);
			break;
		case 8: case 9: case 0xa: case 0xb:
		    if (c64mode)
				c64_colorram[(offset & 0x3ff)] = data | 0xf0;
		    else
				c64_colorram[(offset & 0x3ff)|((c64_port6510&3)<<10)] = data | 0xf0; // maybe all 8 bit connected!
		    break;
		case 0xc:
			cia_0_w(offset, data);
			break;
		case 0xd:
			cia_1_w(offset, data);
			break;
		case 0xf:
			c128_dma8726_port_w(offset&0xff,data);
			break;
		case 0xe:
			DBG_LOG (1, "io write", ("%.3x %.2x\n", offset, data));
			break;
		}
	}
}



static  READ8_HANDLER( c128_read_io )
{
	switch ((offset&0xf00)>>8) {
	case 0:case 1: case 2: case 3:
		return vic2_port_r (offset & 0x3ff);
	case 4:
		return sid6581_0_port_r (offset & 0xff);
	case 5:
		return c128_mmu8722_port_r (offset & 0xff);
	case 6:case 7:
		return vdc8563_port_r (offset & 0xff);
	case 8: case 9: case 0xa: case 0xb:
		return c64_colorram[offset & 0x3ff];
	case 0xc:
		return cia_0_r(offset);
	case 0xd:
		return cia_1_r(offset);
	case 0xf:
		return c128_dma8726_port_r(offset&0xff);
	case 0xe:default:
		DBG_LOG (1, "io read", ("%.3x\n", offset));
		return 0xff;
	}
}

void c128_bankswitch_64 (int reset)
{
	static int old, exrom, game;
	int data, loram, hiram, charen;
	read8_handler rh;

	if (!c64mode)
		return;

	data = (UINT8) cpunum_get_info_int(0, CPUINFO_INT_M6510_PORT) & 7;
	if ((data == old)&&(exrom==c64_exrom)&&(game==c64_game)&&!reset)
		return;

	DBG_LOG (1, "bankswitch", ("%d\n", data & 7));
	loram = (data & 1) ? 1 : 0;
	hiram = (data & 2) ? 1 : 0;
	charen = (data & 4) ? 1 : 0;

	if ((!c64_game && c64_exrom)
		|| (loram && hiram && !c64_exrom))
	{
		memory_set_bankptr(8, c64_roml);
	}
	else
	{
		memory_set_bankptr(8, c64_memory + 0x8000);
	}

	if ((!c64_game && c64_exrom && hiram)
		|| (!c64_exrom))
	{
		memory_set_bankptr(9, c64_romh);
	}
	else if (loram && hiram)
	{
		memory_set_bankptr(9, c64_basic);
	}
	else
	{
		memory_set_bankptr(9, c64_memory + 0xa000);
	}

	if ((!c64_game && c64_exrom)
		|| (charen && (loram || hiram)))
	{
		rh = c128_read_io;
		c128_write_io = 1;
	}
	else
	{
		rh = MRA8_BANK5;
		c128_write_io = 0;
		if ((!charen && (loram || hiram)))
			memory_set_bankptr(13, c64_chargen);
		else
			memory_set_bankptr(13, c64_memory + 0xd000);
	}
	c128_set_m8502_read_handler(0xd000, 0xdfff, rh);

	if (!c64_game && c64_exrom)
	{
		memory_set_bankptr(14, c64_romh);
		memory_set_bankptr(15, c64_romh+0x1f00);
		memory_set_bankptr(16, c64_romh+0x1f05);
	}
	else
	{
		if (hiram)
		{
			memory_set_bankptr(14, c64_kernal);
			memory_set_bankptr(15, c64_kernal+0x1f00);
			memory_set_bankptr(16, c64_kernal+0x1f05);
		}
		else
		{
			memory_set_bankptr(14, c64_memory + 0xe000);
			memory_set_bankptr(15, c64_memory + 0xff00);
			memory_set_bankptr(16, c64_memory + 0xff05);
		}
	}
	old = data;
	exrom= c64_exrom;
	game=c64_game;
}

UINT8 c128_mmu[0x0b];
static int c128_mmu_helper[4] =
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
 static void c128_bankswitch_z80 (void)
 {
	 c128_ram = c64_memory + MMU_RAM_ADDR;
	 c128_va1617 = MMU_VIC_ADDR;
#if 1
	 memory_set_bankptr(10, c128_z80);
	 memory_set_bankptr(11, c128_ram+0x1000);
	 if ( ((C128_MAIN_MEMORY==RAM256KB)&&(MMU_RAM_ADDR>=0x40000))
		  ||((C128_MAIN_MEMORY==RAM128KB)&&(MMU_RAM_ADDR>=0x20000)) )
		 c128_ram=NULL;
#else
	 if (MMU_BOTTOM)
		 c128_ram_bottom = MMU_SIZE;
	 else
		 c128_ram_bottom = 0;

	 if (MMU_RAM_ADDR==0) { /* this is used in z80 mode for rom on/off switching !*/
		 memory_set_bankptr(10, c128_z80);
		 memory_set_bankptr(11, c128_z80+0x400);
	 } else {
		 memory_set_bankptr(10, (c128_ram_bottom > 0 ? c64_memory : c128_ram));
		 memory_set_bankptr(11, (c128_ram_bottom > 0x400 ? c64_memory : c128_ram) + 0x400);
	 }
	 memory_set_bankptr(1, (c128_ram_bottom > 0 ? c64_memory : c128_ram));
	 memory_set_bankptr(2, (c128_ram_bottom > 0x400 ? c64_memory : c128_ram) + 0x400);

	 memory_set_bankptr(3, (c128_ram_bottom > 0x1000 ? c64_memory : c128_ram) + 0x1000);
	 memory_set_bankptr(4, (c128_ram_bottom > 0x2000 ? c64_memory : c128_ram) + 0x2000);
	 memory_set_bankptr(5, c128_ram + 0x4000);

	 if (MMU_TOP)
		 c128_ram_top = 0x10000 - MMU_SIZE;
	 else
		 c128_ram_top = 0x10000;

	 if (c128_ram_top > 0xc000) { memory_set_bankptr(6, c128_ram + 0xc000); }
	 else { memory_set_bankptr(6, c64_memory + 0xc000); }
	 if (c128_ram_top > 0xe000) { memory_set_bankptr(7, c128_ram + 0xe000); }
	 else { memory_set_bankptr(7, c64_memory + 0xd000); }
	 if (c128_ram_top > 0xf000) { memory_set_bankptr(8, c128_ram + 0xf000); }
	 else { memory_set_bankptr(8, c64_memory + 0xe000); }
	 if (c128_ram_top > 0xff05) { memory_set_bankptr(9, c128_ram + 0xff05); }
	 else { memory_set_bankptr(9, c64_memory + 0xff05); }

	 if ( ((C128_MAIN_MEMORY==RAM256KB)&&(MMU_RAM_ADDR>=0x40000))
		  ||((C128_MAIN_MEMORY==RAM128KB)&&(MMU_RAM_ADDR>=0x20000)) )
		 c128_ram=NULL;
#endif
 }

static void c128_bankswitch_128 (int reset)
{
	read8_handler rh;

	c64mode = MMU_64MODE;
	if (c64mode)
	{
		/* mmu works also in c64 mode, but this can wait */
		c128_ram = c64_memory;
		c128_va1617 = 0;
		c128_ram_bottom = 0;
		c128_ram_top = 0x10000;

		memory_set_bankptr(1, c64_memory);
		memory_set_bankptr(2, c64_memory + 0x100);

		memory_set_bankptr(3, c64_memory + 0x200);
		memory_set_bankptr(4, c64_memory + 0x400);
		memory_set_bankptr(5, c64_memory + 0x1000);
		memory_set_bankptr(6, c64_memory + 0x2000);

		memory_set_bankptr(7, c64_memory + 0x4000);

		memory_set_bankptr(12, c64_memory + 0xc000);

		c128_bankswitch_64 (reset);
	}
	else
	{
		c128_ram = c64_memory + MMU_RAM_ADDR;
		c128_va1617 = MMU_VIC_ADDR;
		memory_set_bankptr(1, c64_memory + mmu_page0);
		memory_set_bankptr(2, c64_memory + mmu_page1);
		if (MMU_BOTTOM)
			{
				c128_ram_bottom = MMU_SIZE;
			}
		else
			c128_ram_bottom = 0;
		memory_set_bankptr(3, (c128_ram_bottom > 0x200 ? c64_memory : c128_ram) + 0x200);
		memory_set_bankptr(4, (c128_ram_bottom > 0x400 ? c64_memory : c128_ram) + 0x400);
		memory_set_bankptr(5, (c128_ram_bottom > 0x1000 ? c64_memory : c128_ram) + 0x1000);
		memory_set_bankptr(6, (c128_ram_bottom > 0x2000 ? c64_memory : c128_ram) + 0x2000);

		if (MMU_RAM_LO)
		{
			memory_set_bankptr(7, c128_ram + 0x4000);
		}
		else
		{
			memory_set_bankptr(7, c128_basic);
		}

		if (MMU_RAM_MID)
		{
			memory_set_bankptr(8, c128_ram + 0x8000);
			memory_set_bankptr(9, c128_ram + 0xa000);
		}
		else if (MMU_ROM_MID)
		{
			memory_set_bankptr(8, c128_basic + 0x4000);
			memory_set_bankptr(9, c128_basic + 0x6000);
		}
		else if (MMU_INTERNAL_ROM_MID)
		{
			memory_set_bankptr(8, c128_internal_function);
			memory_set_bankptr(9, c128_internal_function + 0x2000);
		}
		else
		{
			memory_set_bankptr(8, c128_external_function);
			memory_set_bankptr(9, c128_external_function + 0x2000);
		}

		if (MMU_TOP)
		{
			c128_ram_top = 0x10000 - MMU_SIZE;
		}
		else
			c128_ram_top = 0x10000;

		c128_set_m8502_read_handler(0xff00, 0xff04, c128_mmu8722_ff00_r);

		if (MMU_IO_ON)
		{
			rh = c128_read_io;
			c128_write_io = 1;
		}
		else
		{
			c128_write_io = 0;
			rh = MRA8_BANK13;
		}
		c128_set_m8502_read_handler(0xd000, 0xdfff, rh);

		if (MMU_RAM_HI)
		{
			if (c128_ram_top > 0xc000)
			{
				memory_set_bankptr(12, c128_ram + 0xc000);
			}
			else
			{
				memory_set_bankptr(12, c64_memory + 0xc000);
			}
			if (!MMU_IO_ON)
			{
				if (c128_ram_top > 0xd000)
				{
					memory_set_bankptr(13, c128_ram + 0xd000);
				}
				else
				{
					memory_set_bankptr(13, c64_memory + 0xd000);
				}
			}
			if (c128_ram_top > 0xe000)
			{
				memory_set_bankptr(14, c128_ram + 0xe000);
			}
			else
			{
				memory_set_bankptr(14, c64_memory + 0xe000);
			}
			if (c128_ram_top > 0xff05)
			{
				memory_set_bankptr(16, c128_ram + 0xff05);
			}
			else
			{
				memory_set_bankptr(16, c64_memory + 0xff05);
			}
		}
		else if (MMU_ROM_HI)
		{
			memory_set_bankptr(12, c128_editor);
			if (!MMU_IO_ON) {
				memory_set_bankptr(13, c128_chargen);
			}
			memory_set_bankptr(14, c128_kernal);
			memory_set_bankptr(16, c128_kernal + 0x1f05);
		}
		else if (MMU_INTERNAL_ROM_HI)
		{
			memory_set_bankptr(12, c128_internal_function);
			if (!MMU_IO_ON) {
				memory_set_bankptr(13, c128_internal_function + 0x1000);
			}
			memory_set_bankptr(14, c128_internal_function + 0x2000);
			memory_set_bankptr(16, c128_internal_function + 0x3f05);
		}
		else					   /*if (MMU_EXTERNAL_ROM_HI) */
		{
			memory_set_bankptr(12, c128_external_function);
			if (!MMU_IO_ON) {
				memory_set_bankptr(13, c128_external_function + 0x1000);
			}
			memory_set_bankptr(14, c128_external_function + 0x2000);
			memory_set_bankptr(16, c128_external_function + 0x3f05);
		}
		if ( ((C128_MAIN_MEMORY==RAM256KB)&&(MMU_RAM_ADDR>=0x40000))
			 ||((C128_MAIN_MEMORY==RAM128KB)&&(MMU_RAM_ADDR>=0x20000)) ) c128_ram=NULL;
	}
}


static void c128_bankswitch (int reset)
{
	if (mmu_cpu != MMU_CPU8502)
	{
		if (!MMU_CPU8502)
		{
			DBG_LOG (1, "switching to z80",
						("active %d\n",cpu_getactivecpu()) );
			memory_set_context(0);
			c128_bankswitch_z80();
			memory_set_context(1);
			cpunum_set_input_line(0, INPUT_LINE_HALT, CLEAR_LINE);
			cpunum_set_input_line(1, INPUT_LINE_HALT, ASSERT_LINE);
		}
		else
		{
			DBG_LOG (1, "switching to m6502",
						("active %d\n",cpu_getactivecpu()) );
			memory_set_context(1);
			c128_bankswitch_128(reset);
			memory_set_context(0);
			cpunum_set_input_line(0, INPUT_LINE_HALT, ASSERT_LINE);
			cpunum_set_input_line(1, INPUT_LINE_HALT, CLEAR_LINE);

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
			if (cpunum_get_reg(1, REG_PC) == 0x0000)
				cpunum_set_reg(1, REG_PC, 0x1100);
		}
		mmu_cpu = MMU_CPU8502;
		return;
	}
	if (!MMU_CPU8502)
		c128_bankswitch_z80();
	else
		c128_bankswitch_128(reset);
}

static void c128_mmu8722_reset (void)
{
	memset(c128_mmu, 0, sizeof (c128_mmu));
	c128_mmu[5] |= 0x38;
	c128_mmu[10] = 1;
	mmu_cpu = 0;
    mmu_page0 = 0;
    mmu_page1 = 0x0100;
	c128_bankswitch (1);
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
		c128_bankswitch (0);
		break;
	case 7:
		c128_mmu[offset] = data;
		mmu_page0=MMU_PAGE0;
		break;
	case 9:
		c128_mmu[offset] = data;
		mmu_page1=MMU_PAGE1;
		c128_bankswitch (0);
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
		if (KEY_4080)
			data &= ~0x80;
		break;
	case 0xb:
		/* hinybble number of 64 kb memory blocks */
		if (C128_MAIN_MEMORY==RAM256KB) data=0x4f;
		else if (C128_MAIN_MEMORY==RAM1MB) data=0xf;
		else data=0x2f;
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
		c128_bankswitch (0);
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
		c128_bankswitch (0);
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
		c128_mmu8722_ff00_w (offset, data);
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
static int c128_dma_read(int offset)
{
	UINT8 c64_port6510 = (UINT8) cpunum_get_info_int(0, CPUINFO_INT_M6510_PORT);

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

static int c128_dma_read_color (int offset)
{
	UINT8 c64_port6510 = (UINT8) cpunum_get_info_int(0, CPUINFO_INT_M6510_PORT);

	if (c64mode)
		return c64_colorram[offset & 0x3ff] & 0xf;
	else
		return c64_colorram[(offset & 0x3ff)|((c64_port6510&0x3)<<10)] & 0xf;
}

static void c128_common_driver_init (void)
{
	UINT8 *gfx=memory_region(REGION_GFX1);
	int i;

	/* configure the M6510 port */
	cpunum_set_info_fct(1, CPUINFO_PTR_M6510_PORTREAD, (genf *) c64_m6510_port_read);
	cpunum_set_info_fct(1, CPUINFO_PTR_M6510_PORTWRITE, (genf *) c64_m6510_port_write);

	c64_memory = memory_region(REGION_CPU1);

	c128_basic = memory_region(REGION_CPU1)+0x100000;
	c64_basic = memory_region(REGION_CPU1)+0x108000;
	c64_kernal = memory_region(REGION_CPU1)+0x10a000;
	c128_editor = memory_region(REGION_CPU1)+0x10c000;
	c128_z80 = memory_region(REGION_CPU1)+0x10d000;
	c128_kernal = memory_region(REGION_CPU1)+0x10e000;
	c128_internal_function = memory_region(REGION_CPU1)+0x110000;
	c128_external_function = memory_region(REGION_CPU1)+0x118000;
	c64_chargen = memory_region(REGION_CPU1)+0x120000;
	c128_chargen = memory_region(REGION_CPU1)+0x121000;
	c64_colorram = memory_region(REGION_CPU1)+0x122000;
	c128_vdcram = memory_region(REGION_CPU1)+0x122800;

	for (i=0; i<0x100; i++)
		gfx[i]=i;

	vc20_tape_open (c64_tape_read);

	{
		cia6526_interface cia_intf[2];
		cia_intf[0] = c64_cia0;
		cia_intf[1] = c64_cia1;
		cia_intf[0].tod_clock = c64_pal ? 50 : 60;
		cia_intf[1].tod_clock = c64_pal ? 50 : 60;

		cia_config(0, &cia_intf[0]);
		cia_config(1, &cia_intf[1]);
	}
}

DRIVER_INIT( c128 )
{
	c128_common_driver_init ();
	vic6567_init (1, c64_pal,
				  c128_dma_read, c128_dma_read_color, c64_vic_interrupt);
	vic2_set_rastering(0);
	vdc8563_init(0);
	vdc8563_set_rastering(1);
}

DRIVER_INIT( c128pal )
{
	c64_pal = 1;
	c128_common_driver_init ();
	vic6567_init (1, c64_pal,
				  c128_dma_read, c128_dma_read_color, c64_vic_interrupt);
	vic2_set_rastering(1);
	vdc8563_init(0);
	vdc8563_set_rastering(0);
}

MACHINE_RESET( c128 )
{
	c64_common_init_machine ();
	c128_vicaddr = c64_vicaddr = c64_memory;

	sndti_reset(SOUND_SID6581, 0);

	c64_rom_recognition ();
	c64_rom_load();

	c64mode = 0;
	c128_mmu8722_reset ();
	cpunum_set_input_line(0, INPUT_LINE_HALT, CLEAR_LINE);
	cpunum_set_input_line(1, INPUT_LINE_HALT, ASSERT_LINE);
}

VIDEO_START( c128 )
{
	return video_start_vdc8563(machine) || video_start_vic2(machine);
}

VIDEO_UPDATE( c128 )
{
	video_update_vdc8563(machine, screen, bitmap, cliprect);
	video_update_vic2(machine, screen, bitmap, cliprect);
	return 0;
}
