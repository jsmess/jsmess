/***************************************************************************

	machine/bebox.c

	BeBox

	Memory map:

	00000000 - 3FFFFFFF   Physical RAM
	40000000 - 7FFFFFFF   Motherboard glue registers
	80000000 - 807FFFFF   ISA I/O
	81000000 - BF7FFFFF   PCI I/O
	BFFFFFF0 - BFFFFFFF   PCI/ISA interrupt acknowledge
	FF000000 - FFFFFFFF   ROM/flash

	In ISA space, peripherals are generally in similar places as they are on
	standard PC hardware (e.g. - the keyboard is 80000060 and 80000064).  The
	following table shows more:

	Keyboard/Mouse      (Intel 8242)   80000060, 80000064
	Real Time Clock     (BQ3285)       80000070, 80000074
	IDE ATA Interface                  800001F0-F7, 800003F6-7
	COM2                               800002F8-F
	GeekPort A/D Control               80000360-1
	GeekPort A/D Data                  80000362-3
	GeekPort A/D Rate                  80000364
	GeekPort OE                        80000366
	Infrared Interface                 80000368-A
	COM3                               80000380-7
	COM4                               80000388-F
	GeekPort D/A                       80000390-3
	GeekPort GPA/GPB                   80000394
	Joystick Buttons                   80000397
	SuperIO Config (PReP standard)     80000398-9
	MIDI Port 1                        800003A0-7
	MIDI Port 2                        800003A8-F
	Parallel                           800003BC-E
	Floppy                             800003F0-7
	COM1                               800003F8-F
	AD1848                             80000830-4


  Interrupt bit masks:

	bit 31	- N/A (used to set/clear masks)
	bit 30	- SMI interrupt to CPU 0 (CPU 0 only)
	bit 29	- SMI interrupt to CPU 1 (CPU 1 only)
	bit 28	- Unused
	bit 27	- COM1 (PC IRQ #4)
	bit 26	- COM2 (PC IRQ #3)
	bit 25	- COM3
	bit 24	- COM4
	bit 23	- MIDI1
	bit 22	- MIDI2
	bit 21	- SCSI
	bit 20	- PCI Slot #1
	bit 19	- PCI Slot #2
	bit 18	- PCI Slot #3
	bit 17	- Sound
	bit 16	- Keyboard (PC IRQ #1)
	bit 15	- Real Time Clock (PC IRQ #8)
	bit 14	- PC IRQ #5
	bit 13	- Floppy Disk (PC IRQ #6)
	bit 12	- Parallel Port (PC IRQ #7)
	bit 11	- PC IRQ #9
	bit 10	- PC IRQ #10
	bit  9	- PC IRQ #11
	bit  8	- Mouse (PC IRQ #12)
	bit  7	- IDE (PC IRQ #14)
	bit  6	- PC IRQ #15
	bit  5	- PIC8259
	bit  4  - Infrared Controller
	bit  3  - Analog To Digital
	bit  2  - GeekPort
	bit  1  - Unused
	bit  0  - Unused

	Be documentation uses PowerPC bit numbering conventions (i.e. - bit #0 is
	the most significant bit)

	PCI Devices:
		#0		Motorola MPC105
		#11		Intel 82378 PCI/ISA bridge
		#12		NCR 53C810 SCSI

	More hardware information at http://www.netbsd.org/Ports/bebox/hardware.html

***************************************************************************/

#include "includes/bebox.h"
#include "video/pc_vga.h"
#include "video/cirrus.h"
#include "cpu/powerpc/ppc.h"
#include "machine/uart8250.h"
#include "machine/pc_fdc.h"
#include "machine/mpc105.h"
#include "machine/mc146818.h"
#include "machine/pic8259.h"
#include "machine/pit8253.h"
#include "machine/8237dma.h"
#include "machine/idectrl.h"
#include "machine/pci.h"
#include "machine/intelfsh.h"
#include "machine/8042kbdc.h"
#include "machine/53c810.h"
#include "memconv.h"

#define LOG_CPUIMASK	1
#define LOG_UART		1
#define LOG_INTERRUPTS	1

static UINT32 bebox_cpu_imask[2];
static UINT32 bebox_interrupts;
static UINT32 bebox_crossproc_interrupts;


/*************************************
 *
 *	Interrupts and Motherboard Registers
 *
 *************************************/

static void bebox_update_interrupts(void);

static void bebox_mbreg32_w(UINT32 *target, UINT64 data, UINT64 mem_mask)
{
	int i;

	for (i = 1; i < 32; i++)
	{
		if ((data >> (63 - i)) & 1)
		{
			if ((data >> 63) & 1)
				*target |= 0x80000000 >> i;
			else
				*target &= ~(0x80000000 >> i);
		}
	}
}


READ64_HANDLER( bebox_cpu0_imask_r )		{ return ((UINT64) bebox_cpu_imask[0]) << 32; }
READ64_HANDLER( bebox_cpu1_imask_r )		{ return ((UINT64) bebox_cpu_imask[1]) << 32; }
READ64_HANDLER( bebox_interrupt_sources_r )	{ return ((UINT64) bebox_interrupts) << 32; }

WRITE64_HANDLER( bebox_cpu0_imask_w )
{
	UINT32 old_imask = bebox_cpu_imask[0];

	bebox_mbreg32_w(&bebox_cpu_imask[0], data, mem_mask);

	if (old_imask != bebox_cpu_imask[0])
	{
		if (LOG_CPUIMASK)
		{
			logerror("BeBox CPU #0 pc=0x%08X imask=0x%08x\n",
				(unsigned) activecpu_get_reg(REG_PC), bebox_cpu_imask[0]);
		}
		bebox_update_interrupts();
	}
}

WRITE64_HANDLER( bebox_cpu1_imask_w )
{
	UINT32 old_imask = bebox_cpu_imask[1];

	bebox_mbreg32_w(&bebox_cpu_imask[1], data, mem_mask);

	if (old_imask != bebox_cpu_imask[1])
	{
		if (LOG_CPUIMASK)
		{
			logerror("BeBox CPU #1 pc=0x%08X imask=0x%08x\n",
				(unsigned) activecpu_get_reg(REG_PC), bebox_cpu_imask[1]);
		}
		bebox_update_interrupts();
	}
}

READ64_HANDLER( bebox_crossproc_interrupts_r )
{
	UINT32 result;
	result = bebox_crossproc_interrupts;
	if (cpu_getactivecpu())
		result |= 0x02000000;
	else
		result &= ~0x02000000;
	return ((UINT64) result) << 32;
}

WRITE64_HANDLER( bebox_crossproc_interrupts_w )
{
	static const struct
	{
		UINT32 mask;
		int cpunum;
		int active_high;
		int inputline;
	} crossproc_map[] =
	{
		{ 0x40000000, 0, 1, PPC_INPUT_LINE_SMI },
		{ 0x20000000, 1, 1, PPC_INPUT_LINE_SMI },
		{ 0x08000000, 0, 0, PPC_INPUT_LINE_TLBISYNC },
		{ 0x04000000, 1, 0, PPC_INPUT_LINE_TLBISYNC }
	};
	int i, line;
	UINT32 old_crossproc_interrupts = bebox_crossproc_interrupts;

	bebox_mbreg32_w(&bebox_crossproc_interrupts, data, mem_mask);

	for (i = 0; i < sizeof(crossproc_map) / sizeof(crossproc_map[0]); i++)
	{
		if ((old_crossproc_interrupts ^ bebox_crossproc_interrupts) & crossproc_map[i].mask)
		{
			if (bebox_crossproc_interrupts & crossproc_map[i].mask)
				line = crossproc_map[i].active_high ? ASSERT_LINE : CLEAR_LINE;
			else
				line = crossproc_map[i].active_high ? CLEAR_LINE : ASSERT_LINE;

			if (LOG_INTERRUPTS)
			{
				logerror("bebox_crossproc_interrupts_w(): CPU #%d %s %s\n",
					crossproc_map[i].cpunum, line ? "Asserting" : "Clearing",
					(crossproc_map[i].inputline == PPC_INPUT_LINE_SMI) ? "SMI" : "TLBISYNC");
			}

			cpunum_set_input_line(crossproc_map[i].cpunum, crossproc_map[i].inputline, line);
		}
	}
}

WRITE64_HANDLER( bebox_processor_resets_w )
{
	UINT8 b = (UINT8) (data >> 56);

	if (b & 0x20)
	{
		cpunum_set_input_line(1, INPUT_LINE_RESET, (b & 0x80) ? CLEAR_LINE : ASSERT_LINE);
	}
}


static void bebox_update_interrupts(void)
{
	int cpunum;
	UINT32 interrupt;

	for (cpunum = 0; cpunum < 2; cpunum++)
	{
		interrupt = bebox_interrupts & bebox_cpu_imask[cpunum];
		
		if (LOG_INTERRUPTS)
		{
			logerror("\tbebox_update_interrupts(): CPU #%d [%08X|%08X] IRQ %s\n", cpunum,
				bebox_interrupts, bebox_cpu_imask[cpunum], interrupt ? "on" : "off");
		}

		cpunum_set_input_line(cpunum, INPUT_LINE_IRQ0, interrupt ? ASSERT_LINE : CLEAR_LINE);
	}
}


static void bebox_set_irq_bit(unsigned int interrupt_bit, int val)
{
	static const char *interrupt_names[32] =
	{
		NULL,
		NULL,
		"GEEKPORT",
		"ADC",
		"IR",
		"PIC8259",
		"PCIRQ 15",
		"IDE",
		"MOUSE",
		"PCIRQ 11",
		"PCIRQ 10",
		"PCIRQ 9",
		"PARALLEL",
		"FLOPPY",
		"PCIRQ 5",
		"RTC",
		"KEYBOARD",
		"SOUND",
		"PCI3",
		"PCI2",
		"PCI1",
		"SCSI",
		"MIDI2",
		"MIDI1",
		"COM4",
		"COM3",
		"COM2",
		"COM1",
		NULL,
		"SMI1",
		"SMI0",
		NULL
	};
	UINT32 old_interrupts;

	if (LOG_INTERRUPTS)
	{
		/* make sure that we don't shoot ourself in the foot */
		if ((interrupt_bit > sizeof(interrupt_names) / sizeof(interrupt_names[0])) && !interrupt_names[interrupt_bit])
			fatalerror("Raising invalid interrupt %u", interrupt_bit);

		logerror("bebox_set_irq_bit(): pc[0]=0x%08x pc[1]=0x%08x %s interrupt #%u (%s)\n",
			(unsigned) cpunum_get_reg(0, REG_PC),
			(unsigned) cpunum_get_reg(1, REG_PC),
			val ? "Asserting" : "Clearing",
			interrupt_bit, interrupt_names[interrupt_bit]);
	}

	old_interrupts = bebox_interrupts;
	if (val)
		bebox_interrupts |= 1 << interrupt_bit;
	else
		bebox_interrupts &= ~(1 << interrupt_bit);

	/* if interrupt values have changed, update the lines */
	if (bebox_interrupts != old_interrupts)
		bebox_update_interrupts();
}


/*************************************
 *
 *	COM ports
 *
 *************************************/

static void bebox_uart_transmit(int id, int data)
{
	if (LOG_UART)
		logerror("bebox_uart_transmit(): id=%d data=0x%02X\n", id, data);
}


static void bebox_uart_handshake(int id, int data)
{
	if (LOG_UART)
		logerror("bebox_uart_handshake(): id=%d data=0x%02X\n", id, data);
}


static const uart8250_interface bebox_uart_inteface =
{
	TYPE16550,
	0,
	NULL,
	bebox_uart_transmit,
	bebox_uart_handshake
};


/*************************************
 *
 *	Floppy Disk Controller
 *
 *************************************/

#define FDC_DMA 2

static void bebox_fdc_interrupt(int state)
{
	bebox_set_irq_bit(13, state);
	pic8259_set_irq_line(0, 6, state);
}


static void bebox_fdc_dma_drq(int state, int read_)
{
	dma8237_drq_write(0, FDC_DMA, state);
}


static mess_image *bebox_fdc_get_image(int floppy_index)
{
	/* the BeBox boot ROM seems to query for floppy #1 when it should be
	 * querying for floppy #0 */
	return image_from_devtype_and_index(IO_FLOPPY, 0);
}


static const struct pc_fdc_interface bebox_fdc_interface =
{
	SMC37C78,
	bebox_fdc_interrupt,
	bebox_fdc_dma_drq,
	bebox_fdc_get_image
};


/*************************************
 *
 *	8259 PIC
 *
 *************************************/

READ64_HANDLER( bebox_interrupt_ack_r )
{
	int result;
	result = pic8259_acknowledge(0);
	if (result == 2)
		result = pic8259_acknowledge(1);
	bebox_set_irq_bit(5, 0);	/* HACK */
	return ((UINT64) result) << 56;
}



static void bebox_pic_set_int_line(int which, int interrupt)
{
	switch(which)
	{
		case 0:
			/* Master */
			bebox_set_irq_bit(5, interrupt);
			break;

		case 1:
			/* Slave */
			pic8259_set_irq_line(0, 2, interrupt);
			break;
	}
}



/*************************************
 *
 *	Floppy/IDE/ATA
 *
 *************************************/

static READ8_HANDLER( bebox_800001F0_8_r ) { return ide_controller_0_r(offset + 0x1F0); }
static WRITE8_HANDLER( bebox_800001F0_8_w ) { ide_controller_0_w(offset + 0x1F0, data); }

READ64_HANDLER( bebox_800001F0_r ) { return read64be_with_read8_handler(bebox_800001F0_8_r, offset, mem_mask); }
WRITE64_HANDLER( bebox_800001F0_w ) { write64be_with_write8_handler(bebox_800001F0_8_w, offset, data, mem_mask); }


READ64_HANDLER( bebox_800003F0_r )
{
	UINT64 result = pc64be_fdc_r(offset, mem_mask | 0xFFFF);

	if (((mem_mask >> 8) & 0xFF) == 0)
	{
		result &= ~(0xFF << 8);
		result |= ide_controller_0_r(0x3F6) << 8;
	}
	
	if (((mem_mask >> 0) & 0xFF) == 0)
	{
		result &= ~(0xFF << 0);
		result |= ide_controller_0_r(0x3F7) << 0;
	}
	return result;
}


WRITE64_HANDLER( bebox_800003F0_w )
{
	pc64be_fdc_w(offset, data, mem_mask | 0xFFFF);

	if (((mem_mask >> 8) & 0xFF) == 0)
		ide_controller_0_w(0x3F6, (data >> 8) & 0xFF);
	
	if (((mem_mask >> 0) & 0xFF) == 0)
		ide_controller_0_w(0x3F7, (data >> 0) & 0xFF);
}


static void bebox_ide_interrupt(int state)
{
	bebox_set_irq_bit(7, state);
	pic8259_set_irq_line(1, 6, state);
}


static struct ide_interface bebox_ide_interface =
{
	bebox_ide_interrupt
};


/*************************************
 *
 *	Video card (Cirrus Logic CL-GD5430)
 *
 *************************************/

static read8_handler bebox_vga_memory_rh;
static write8_handler bebox_vga_memory_wh;

static READ64_HANDLER( bebox_video_r )
{
	const UINT64 *mem = (const UINT64 *) pc_vga_memory();
	return BIG_ENDIANIZE_INT64(mem[offset]);
}


static WRITE64_HANDLER( bebox_video_w )
{
	UINT64 *mem = (UINT64 *) pc_vga_memory();
	data = BIG_ENDIANIZE_INT64(data);
	mem_mask = BIG_ENDIANIZE_INT64(mem_mask);
	COMBINE_DATA(&mem[offset]);
}


static READ64_HANDLER( bebox_vga_memory_r )
{
	return read64be_with_read8_handler(bebox_vga_memory_rh, offset, mem_mask);
}


static WRITE64_HANDLER( bebox_vga_memory_w )
{
	write64be_with_write8_handler(bebox_vga_memory_wh, offset, data, mem_mask);
}


static void bebox_map_vga_memory(offs_t begin, offs_t end, read8_handler rh, write8_handler wh)
{
	read64_handler rh64 = (rh == MRA8_BANK4) ? MRA64_BANK4 : bebox_vga_memory_r;
	write64_handler wh64 = (wh == MWA8_BANK4) ? MWA64_BANK4 : bebox_vga_memory_w;

	bebox_vga_memory_rh = rh;
	bebox_vga_memory_wh = wh;

	memory_install_read64_handler(0, ADDRESS_SPACE_PROGRAM, 0xC00A0000, 0xC00BFFFF, 0, 0, MRA64_NOP);
	memory_install_write64_handler(0, ADDRESS_SPACE_PROGRAM, 0xC00A0000, 0xC00BFFFF, 0, 0, MWA64_NOP);

	memory_install_read64_handler(0, ADDRESS_SPACE_PROGRAM, 0xC0000000 + begin, 0xC0000000 + end, 0, 0, rh64);
	memory_install_write64_handler(0, ADDRESS_SPACE_PROGRAM, 0xC0000000 + begin, 0xC0000000 + end, 0, 0, wh64);
}


static const struct pc_vga_interface bebox_vga_interface =
{
	4,
	bebox_map_vga_memory,

	NULL,

	ADDRESS_SPACE_PROGRAM,
	0x80000000
};


/*************************************
 *
 *	8237 DMA
 *
 *************************************/

static UINT16 dma_offset[2][4];
static UINT8 at_pages[0x10];

static READ8_HANDLER(at_page8_r)
{
	UINT8 data = at_pages[offset % 0x10];

	switch(offset % 8)
	{
		case 1:
			data = dma_offset[(offset / 8) & 1][2];
			break;
		case 2:
			data = dma_offset[(offset / 8) & 1][3];
			break;
		case 3:
			data = dma_offset[(offset / 8) & 1][1];
			break;
		case 7:
			data = dma_offset[(offset / 8) & 1][0];
			break;
	}
	return data;
}


static WRITE8_HANDLER(at_page8_w)
{
	at_pages[offset % 0x10] = data;

	switch(offset % 8)
	{
		case 1:
			dma_offset[(offset / 8) & 1][2] &= 0xFF00;
			dma_offset[(offset / 8) & 1][2] |= ((UINT16 ) data) << 0;
			break;
		case 2:
			dma_offset[(offset / 8) & 1][3] &= 0xFF00;
			dma_offset[(offset / 8) & 1][3] |= ((UINT16 ) data) << 0;
			break;
		case 3:
			dma_offset[(offset / 8) & 1][1] &= 0xFF00;
			dma_offset[(offset / 8) & 1][1] |= ((UINT16 ) data) << 0;
			break;
		case 7:
			dma_offset[(offset / 8) & 1][0] &= 0xFF00;
			dma_offset[(offset / 8) & 1][0] |= ((UINT16 ) data) << 0;
			break;
	}
}


READ64_HANDLER(bebox_page_r)
{
	return read64be_with_read8_handler(at_page8_r, offset, mem_mask);
}


WRITE64_HANDLER(bebox_page_w)
{
	write64be_with_write8_handler(at_page8_w, offset, data, mem_mask);
}


static WRITE8_HANDLER(at_hipage8_w)
{
	switch(offset % 8)
	{
		case 1:
			dma_offset[(offset / 8) & 1][2] &= 0x00FF;
			dma_offset[(offset / 8) & 1][2] |= ((UINT16 ) data) << 8;
			break;
		case 2:
			dma_offset[(offset / 8) & 1][3] &= 0x00FF;
			dma_offset[(offset / 8) & 1][3] |= ((UINT16 ) data) << 8;
			break;
		case 3:
			dma_offset[(offset / 8) & 1][1] &= 0x00FF;
			dma_offset[(offset / 8) & 1][1] |= ((UINT16 ) data) << 8;
			break;
		case 7:
			dma_offset[(offset / 8) & 1][0] &= 0x00FF;
			dma_offset[(offset / 8) & 1][0] |= ((UINT16 ) data) << 8;
			break;
	}
}


READ64_HANDLER(bebox_80000480_r)
{
	fatalerror("NYI");
}


WRITE64_HANDLER(bebox_80000480_w)
{
	write64be_with_write8_handler(at_hipage8_w, offset, data, mem_mask);
}


static UINT8 bebox_dma_read_byte(int channel, offs_t offset)
{
	offs_t page_offset = (((offs_t) dma_offset[0][channel]) << 16)
		& 0x7FFF0000;
	return program_read_byte(page_offset + offset);
}


static void bebox_dma_write_byte(int channel, offs_t offset, UINT8 data)
{
	offs_t page_offset = (((offs_t) dma_offset[0][channel]) << 16)
		& 0x7FFF0000;
	program_write_byte(page_offset + offset, data);
}


static const struct dma8237_interface bebox_dma =
{
	0,
	TIME_IN_USEC(1),

	bebox_dma_read_byte,
	bebox_dma_write_byte,

	{ 0, 0, pc_fdc_dack_r, 0 },
	{ 0, 0, pc_fdc_dack_w, 0 },
	pc_fdc_set_tc_state
};


/*************************************
 *
 *	8254 PIT
 *
 *************************************/

static void bebox_timer0_w(int state)
{
	pic8259_set_irq_line(0, 0, state);
}


static const struct pit8253_config bebox_pit8254_config =
{
	TYPE8254,
	{
		{
			4772720/4,				/* heartbeat IRQ */
			bebox_timer0_w,
			NULL
		},
		{
			4772720/4,				/* dram refresh */
			NULL,
			NULL
		},
		{
			4772720/4,				/* pio port c pin 4, and speaker polling enough */
			NULL,
			NULL
		}
	}
};


/*************************************
 *
 *	Flash ROM
 *
 *************************************/

static READ8_HANDLER( bebox_flash8_r )
{
	offset = (offset & ~7) | (7 - (offset & 7));
	return intelflash_read(0, offset);
}


static WRITE8_HANDLER( bebox_flash8_w )
{
	offset = (offset & ~7) | (7 - (offset & 7));
	intelflash_write(0, offset, data);
}


READ64_HANDLER( bebox_flash_r )
{
	return read64be_with_read8_handler(bebox_flash8_r, offset, mem_mask);
}


WRITE64_HANDLER( bebox_flash_w )
{
	write64be_with_write8_handler(bebox_flash8_w, offset, data, mem_mask);
}


/*************************************
 *
 *	Keyboard
 *
 *************************************/

static void bebox_keyboard_interrupt(int state)
{
	bebox_set_irq_bit(16, state);
	pic8259_set_irq_line(0, 1, state);
}


static const struct kbdc8042_interface bebox_8042_interface =
{
	KBDC8042_STANDARD,
	NULL,
	bebox_keyboard_interrupt
};


/*************************************
 *
 *	SCSI
 *
 *************************************/

static UINT32 scsi53c810_data[0x100 / 4];

static READ64_HANDLER( scsi53c810_r )
{
	int reg = offset*8;
	UINT64 r = 0;
	if (!(mem_mask & U64(0xff00000000000000))) {
		r |= (UINT64)lsi53c810_reg_r(reg+0) << 56;
	}
	if (!(mem_mask & U64(0x00ff000000000000))) {
		r |= (UINT64)lsi53c810_reg_r(reg+1) << 48;
	}
	if (!(mem_mask & U64(0x0000ff0000000000))) {
		r |= (UINT64)lsi53c810_reg_r(reg+2) << 40;
	}
	if (!(mem_mask & U64(0x000000ff00000000))) {
		r |= (UINT64)lsi53c810_reg_r(reg+3) << 32;
	}
	if (!(mem_mask & U64(0x00000000ff000000))) {
		r |= (UINT64)lsi53c810_reg_r(reg+4) << 24;
	}
	if (!(mem_mask & U64(0x0000000000ff0000))) {
		r |= (UINT64)lsi53c810_reg_r(reg+5) << 16;
	}
	if (!(mem_mask & U64(0x000000000000ff00))) {
		r |= (UINT64)lsi53c810_reg_r(reg+6) << 8;
	}
	if (!(mem_mask & U64(0x00000000000000ff))) {
		r |= (UINT64)lsi53c810_reg_r(reg+7) << 0;
	}

	return r;
}


static WRITE64_HANDLER( scsi53c810_w )
{
	int reg = offset*8;
	if (!(mem_mask & U64(0xff00000000000000))) {
		lsi53c810_reg_w(reg+0, data >> 56);
	}
	if (!(mem_mask & U64(0x00ff000000000000))) {
		lsi53c810_reg_w(reg+1, data >> 48);
	}
	if (!(mem_mask & U64(0x0000ff0000000000))) {
		lsi53c810_reg_w(reg+2, data >> 40);
	}
	if (!(mem_mask & U64(0x000000ff00000000))) {
		lsi53c810_reg_w(reg+3, data >> 32);
	}
	if (!(mem_mask & U64(0x00000000ff000000))) {
		lsi53c810_reg_w(reg+4, data >> 24);
	}
	if (!(mem_mask & U64(0x0000000000ff0000))) {
		lsi53c810_reg_w(reg+5, data >> 16);
	}
	if (!(mem_mask & U64(0x000000000000ff00))) {
		lsi53c810_reg_w(reg+6, data >> 8);
	}
	if (!(mem_mask & U64(0x00000000000000ff))) {
		lsi53c810_reg_w(reg+7, data >> 0);
	}
}


#define BYTE_REVERSE32(x)		(((x >> 24) & 0xff) | \
								((x >> 8) & 0xff00) | \
								((x << 8) & 0xff0000) | \
								((x << 24) & 0xff000000))

static UINT32 scsi53c810_fetch(UINT32 dsp)
{
	UINT32 result;
	result = program_read_dword_64be(dsp & 0x7FFFFFFF);
	return BYTE_REVERSE32(result);
}


static void scsi53c810_irq_callback(void)
{
	bebox_set_irq_bit(21, 1);
	bebox_set_irq_bit(21, 0);
}


static void scsi53c810_dma_callback(UINT32 src, UINT32 dst, int length, int byteswap)
{
}


static UINT32 scsi53c810_pci_read(int function, int offset, UINT32 mem_mask)
{
	UINT32 result = 0;

	if (function == 0)
	{
		switch(offset)
		{
			case 0x00:	/* vendor/device ID */
				result = 0x00011000;
				break;

			case 0x08:
				result = 0x01000000;
				break;

			default:
				result = scsi53c810_data[offset / 4];
				break;
		}
	}
	return result;
}


static void scsi53c810_pci_write(int function, int offset, UINT32 data, UINT32 mem_mask)
{
	offs_t addr;

	if (function == 0)
	{
		scsi53c810_data[offset / 4] = data;

		switch(offset)
		{
			case 0x04:
				/* command
				 *
				 * bit 8:	SERR/ Enable
				 * bit 6:	Enable Parity Response
				 * bit 4:	Write and Invalidate Mode
				 * bit 2:	Enable Bus Mastering
				 * bit 1:	Enable Memory Space
				 * bit 0:	Enable IO Space
				 */
				if (data & 0x02)
				{
					/* brutal ugly hack; at some point the PCI code should be handling this stuff */
					if (scsi53c810_data[5] != 0xFFFFFFF0)
					{
						addr = (scsi53c810_data[5] | 0xC0000000) & ~0xFF;
						memory_install_read64_handler(0, ADDRESS_SPACE_PROGRAM, addr, addr + 0xFF, 0, 0, scsi53c810_r);
						memory_install_write64_handler(0, ADDRESS_SPACE_PROGRAM, addr, addr + 0xFF, 0, 0, scsi53c810_w);
					}
				}
				break;
		}
	}
}


static const struct pci_device_info scsi53c810_callbacks =
{
	scsi53c810_pci_read,
	scsi53c810_pci_write
};

static SCSIConfigTable dev_table =
{
	2, /* 2 SCSI devices */
	{
		{ SCSI_ID_0, 0, SCSI_DEVICE_HARDDISK },	/* SCSI ID 0, using HD 0, HD */
		{ SCSI_ID_3, 0, SCSI_DEVICE_CDROM }	/* SCSI ID 3, using CHD 0, CD-ROM */
	}
};

static struct LSI53C810interface scsi53c810_intf =
{
	&dev_table,		/* SCSI device table */
	&scsi53c810_irq_callback,
	&scsi53c810_dma_callback,
	&scsi53c810_fetch,
};


/*************************************
 *
 *	Driver main
 *
 *************************************/

NVRAM_HANDLER( bebox )
{
	nvram_handler_intelflash(machine, 0, file, read_or_write);
}

MACHINE_RESET( bebox )
{
	cpunum_set_input_line(0, INPUT_LINE_RESET, CLEAR_LINE);
	cpunum_set_input_line(1, INPUT_LINE_RESET, ASSERT_LINE);
	ide_controller_reset(0);
}

DRIVER_INIT( bebox )
{
	int cpu;
	offs_t vram_begin;
	offs_t vram_end;

	mpc105_init(0);
	pci_add_device(0, 1, &cirrus5430_callbacks);
	if (0)
		pci_add_device(0, 12, &scsi53c810_callbacks);

	/* set up boot and flash ROM */
	memory_set_bankptr(2, memory_region(REGION_USER2));
	intelflash_init(0, FLASH_FUJITSU_29F016A, memory_region(REGION_USER1));

	/* install MESS managed RAM */
	for (cpu = 0; cpu < 2; cpu++)
	{
		memory_install_read64_handler(cpu, ADDRESS_SPACE_PROGRAM, 0, mess_ram_size - 1, 0, 0x02000000, MRA64_BANK3);
		memory_install_write64_handler(cpu, ADDRESS_SPACE_PROGRAM, 0, mess_ram_size - 1, 0, 0x02000000, MWA64_BANK3);
	}
	memory_set_bankptr(3, mess_ram);

	/* set up UARTs */
	uart8250_init(0, NULL);
	uart8250_init(1, NULL);
	uart8250_init(2, NULL);
	uart8250_init(3, &bebox_uart_inteface);

	pc_fdc_init(&bebox_fdc_interface);
	mc146818_init(MC146818_STANDARD);
	pic8259_init(2, bebox_pic_set_int_line);
	ide_controller_init_custom(0, &bebox_ide_interface, NULL);
	pc_vga_init(&bebox_vga_interface, &cirrus_svga_interface);
	pit8253_init(1, &bebox_pit8254_config);
	kbdc8042_init(&bebox_8042_interface);

	dma8237_init(2);
	dma8237_config(0, &bebox_dma);

	/* install VGA memory */
	vram_begin = 0xC1000000;
	vram_end = vram_begin + pc_vga_memory_size() - 1;
	for (cpu = 0; cpu < 2; cpu++)
	{
		memory_install_read64_handler(cpu, ADDRESS_SPACE_PROGRAM, vram_begin, vram_end, 0, 0, bebox_video_r);
		memory_install_write64_handler(cpu, ADDRESS_SPACE_PROGRAM, vram_begin, vram_end, 0, 0, bebox_video_w);
	}

	/* SCSI */
	lsi53c810_init(&scsi53c810_intf);

	/* The following is a verrrry ugly hack put in to support NetBSD for
	 * NetBSD.  When NetBSD/bebox it does most of its work on CPU #0 and then
	 * lets CPU #1 go.  However, it seems that CPU #1 jumps into never-never
	 * land, crashes, and then goes into NetBSD's crash handler which catches
	 * it.  The current PowerPC core cannot catch this trip into never-never
	 * land properly, and MESS crashes.  In the interim, this "mitten" catches
	 * the crash
	 */
	{
		static UINT64 ops[2] =
		{
			/* li r0, 0x0700 */
			/* mtspr ctr, r0 */
			U64(0x380007007C0903A6),

			/* bcctr 0x14, 0 */
			U64(0x4E80042000000000)
		};
		memory_install_read64_handler(1, ADDRESS_SPACE_PROGRAM, 0x9421FFF0, 0x9421FFFF, 0, 0, MRA64_BANK1);
		memory_set_bankptr(1, ops);
	}
}
