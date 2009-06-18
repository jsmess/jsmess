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

/* Core includes */
#include "driver.h"
#include "memconv.h"
#include "includes/bebox.h"

/* Components */
#include "video/pc_vga.h"
#include "video/cirrus.h"
#include "cpu/powerpc/ppc.h"
#include "machine/ins8250.h"
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


#define LOG_CPUIMASK	1
#define LOG_UART		1
#define LOG_INTERRUPTS	1

static UINT32 bebox_cpu_imask[2];
static UINT32 bebox_interrupts;
static UINT32 bebox_crossproc_interrupts;

static struct {
	device_config	*pic8259_master;
	device_config	*pic8259_slave;
	device_config	*dma8237_1;
	device_config	*dma8237_2;
} bebox_devices;


/*************************************
 *
 *	Interrupts and Motherboard Registers
 *
 *************************************/

static void bebox_update_interrupts(running_machine *machine);

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
				(unsigned) cpu_get_pc( space->cpu), bebox_cpu_imask[0]);
		}
		bebox_update_interrupts(space->machine);
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
				(unsigned) cpu_get_pc( space->cpu ), bebox_cpu_imask[1]);
		}
		bebox_update_interrupts(space->machine);
	}
}

READ64_HANDLER( bebox_crossproc_interrupts_r )
{
	UINT32 result;
	result = bebox_crossproc_interrupts;

	/* return a different result depending on which CPU is accessing this handler */
	if (space != cputag_get_address_space(space->machine, "ppc1", ADDRESS_SPACE_PROGRAM))
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
		{ 0x40000000, 0, 1, 0/*PPC_INPUT_LINE_SMI*/ },
		{ 0x20000000, 1, 1, 0/*PPC_INPUT_LINE_SMI*/ },
		{ 0x08000000, 0, 0, 0/*PPC_INPUT_LINE_TLBISYNC*/ },
		{ 0x04000000, 1, 0, 0/*PPC_INPUT_LINE_TLBISYNC*/ }
	};
	int i, line;
	UINT32 old_crossproc_interrupts = bebox_crossproc_interrupts;
	static const char *const cputags[] = { "ppc1", "ppc2" };

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
/*
				logerror("bebox_crossproc_interrupts_w(): CPU #%d %s %s\n",
					crossproc_map[i].cpunum, line ? "Asserting" : "Clearing",
					(crossproc_map[i].inputline == PPC_INPUT_LINE_SMI) ? "SMI" : "TLBISYNC");
					*/
			}

			cputag_set_input_line(space->machine, cputags[crossproc_map[i].cpunum], crossproc_map[i].inputline, line);
		}
	}
}

WRITE64_HANDLER( bebox_processor_resets_w )
{
	UINT8 b = (UINT8) (data >> 56);

	if (b & 0x20)
	{
		cputag_set_input_line(space->machine, "ppc2", INPUT_LINE_RESET, (b & 0x80) ? CLEAR_LINE : ASSERT_LINE);
	}
}


static void bebox_update_interrupts(running_machine *machine)
{
	int cpunum;
	UINT32 interrupt;
	static const char *const cputags[] = { "ppc1", "ppc2" };

	for (cpunum = 0; cpunum < 2; cpunum++)
	{
		interrupt = bebox_interrupts & bebox_cpu_imask[cpunum];

		if (LOG_INTERRUPTS)
		{
			logerror("\tbebox_update_interrupts(): CPU #%d [%08X|%08X] IRQ %s\n", cpunum,
				bebox_interrupts, bebox_cpu_imask[cpunum], interrupt ? "on" : "off");
		}

		cputag_set_input_line(machine, cputags[cpunum], INPUT_LINE_IRQ0, interrupt ? ASSERT_LINE : CLEAR_LINE);
	}
}


static void bebox_set_irq_bit(running_machine *machine, unsigned int interrupt_bit, int val)
{
	static const char *const interrupt_names[32] =
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
		assert_always((interrupt_bit < ARRAY_LENGTH(interrupt_names)) && (interrupt_names[interrupt_bit] != NULL), "Raising invalid interrupt");

		logerror("bebox_set_irq_bit(): pc[0]=0x%08x pc[1]=0x%08x %s interrupt #%u (%s)\n",
			(unsigned) cpu_get_reg(cputag_get_cpu(machine, "ppc1"), REG_GENPC),
			(unsigned) cpu_get_reg(cputag_get_cpu(machine, "ppc2"), REG_GENPC),
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
		bebox_update_interrupts(machine);
}


/*************************************
 *
 *	COM ports
 *
 *************************************/

static INS8250_TRANSMIT( bebox_uart_transmit )
{
	if (LOG_UART)
		logerror("bebox_uart_transmit(): data=0x%02X\n", data);
}


static INS8250_HANDSHAKE_OUT( bebox_uart_handshake )
{
	if (LOG_UART)
		logerror("bebox_uart_handshake(): data=0x%02X\n", data);
}


const ins8250_interface bebox_uart_inteface_0 =
{
	0,
	NULL,
	NULL,
	NULL,
	NULL
};

const ins8250_interface bebox_uart_inteface_1 =
{
	0,
	NULL,
	NULL,
	NULL,
	NULL
};

const ins8250_interface bebox_uart_inteface_2 =
{
	0,
	NULL,
	NULL,
	NULL,
	NULL
};

const ins8250_interface bebox_uart_inteface_3 =
{
	0,
	NULL,
	bebox_uart_transmit,
	bebox_uart_handshake,
	NULL
};


/*************************************
 *
 *	Floppy Disk Controller
 *
 *************************************/

#define FDC_DMA 2

static void bebox_fdc_interrupt(running_machine *machine, int state)
{
	bebox_set_irq_bit(machine, 13, state);
	if ( bebox_devices.pic8259_master ) {
		pic8259_set_irq_line(bebox_devices.pic8259_master, 6, state);
	}
}


static void bebox_fdc_dma_drq(running_machine *machine, int state, int read_)
{
	if ( bebox_devices.dma8237_1 ) {
		dma8237_drq_write(bebox_devices.dma8237_1, FDC_DMA, state);
	}
}


static const device_config *bebox_fdc_get_image(running_machine *machine, int floppy_index)
{
	/* the BeBox boot ROM seems to query for floppy #1 when it should be
	 * querying for floppy #0 */
	return image_from_devtype_and_index(machine, IO_FLOPPY, 0);
}

static device_config * bebox_get_device(running_machine *machine )
{
	return (device_config*)devtag_get_device(machine, "smc37c78");
}


static const struct pc_fdc_interface bebox_fdc_interface =
{
	bebox_fdc_interrupt,
	bebox_fdc_dma_drq,
	bebox_fdc_get_image,
	bebox_get_device
};


/*************************************
 *
 *	8259 PIC
 *
 *************************************/

READ64_HANDLER( bebox_interrupt_ack_r )
{
	int result;
	result = pic8259_acknowledge( bebox_devices.pic8259_master );
	if (result == 2)
		result = pic8259_acknowledge( bebox_devices.pic8259_slave );
	bebox_set_irq_bit(space->machine, 5, 0);	/* HACK */
	return ((UINT64) result) << 56;
}


/*************************************************************
 *
 * pic8259 configuration
 *
 *************************************************************/
 
static PIC8259_SET_INT_LINE( bebox_pic8259_master_set_int_line ) {
	bebox_set_irq_bit(device->machine, 5, interrupt);
}


static PIC8259_SET_INT_LINE( bebox_pic8259_slave_set_int_line ) {
	if ( bebox_devices.pic8259_master ) {
		pic8259_set_irq_line( bebox_devices.pic8259_master, 2, interrupt );
	}
}


const struct pic8259_interface bebox_pic8259_master_config = {
	bebox_pic8259_master_set_int_line
};


const struct pic8259_interface bebox_pic8259_slave_config = {
	bebox_pic8259_slave_set_int_line
};


/*************************************
 *
 *	Floppy/IDE/ATA
 *
 *************************************/

static const device_config *ide_device(running_machine *machine)
{
	return devtag_get_device(machine, "ide");
}

static READ8_HANDLER( bebox_800001F0_8_r ) { return ide_controller_r(ide_device(space->machine), offset + 0x1F0, 1); }
static WRITE8_HANDLER( bebox_800001F0_8_w ) { ide_controller_w(ide_device(space->machine), offset + 0x1F0, 1, data); }

READ64_HANDLER( bebox_800001F0_r ) { return read64be_with_read8_handler(bebox_800001F0_8_r, space, offset, mem_mask); }
WRITE64_HANDLER( bebox_800001F0_w ) { write64be_with_write8_handler(bebox_800001F0_8_w, space, offset, data, mem_mask); }


READ64_HANDLER( bebox_800003F0_r )
{
	UINT64 result = read64be_with_read8_handler(pc_fdc_r, space, offset, mem_mask | 0xFFFF);

	if (((mem_mask >> 8) & 0xFF) == 0)
	{
		result &= ~(0xFF << 8);
		result |= ide_controller_r(ide_device(space->machine), 0x3F6, 1) << 8;
	}

	if (((mem_mask >> 0) & 0xFF) == 0)
	{
		result &= ~(0xFF << 0);
		result |= ide_controller_r(ide_device(space->machine), 0x3F7, 1) << 0;
	}
	return result;
}


WRITE64_HANDLER( bebox_800003F0_w )
{
	write64be_with_write8_handler(pc_fdc_w, space, offset, data, mem_mask | 0xFFFF);

	if (((mem_mask >> 8) & 0xFF) == 0)
		ide_controller_w(ide_device(space->machine), 0x3F6, 1, (data >> 8) & 0xFF);

	if (((mem_mask >> 0) & 0xFF) == 0)
		ide_controller_w(ide_device(space->machine), 0x3F7, 1, (data >> 0) & 0xFF);
}


void bebox_ide_interrupt(const device_config *device, int state)
{
	bebox_set_irq_bit(device->machine, 7, state);
	if ( bebox_devices.pic8259_master ) {
		pic8259_set_irq_line( bebox_devices.pic8259_master, 6, state);
	}
}


/*************************************
 *
 *	Video card (Cirrus Logic CL-GD5430)
 *
 *************************************/

static read8_space_func bebox_vga_memory_rh;
static write8_space_func bebox_vga_memory_wh;

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
	return read64be_with_read8_handler(bebox_vga_memory_rh, space, offset, mem_mask);
}


static WRITE64_HANDLER( bebox_vga_memory_w )
{
	write64be_with_write8_handler(bebox_vga_memory_wh, space, offset, data, mem_mask);
}


static void bebox_map_vga_memory(running_machine *machine, offs_t begin, offs_t end, read8_space_func rh, write8_space_func wh)
{
	const address_space *space = cputag_get_address_space(machine, "ppc1", ADDRESS_SPACE_PROGRAM);

	read64_space_func rh64 = (rh == SMH_BANK(4)) ? SMH_BANK(4) : bebox_vga_memory_r;
	write64_space_func wh64 = (wh == SMH_BANK(4)) ? SMH_BANK(4) : bebox_vga_memory_w;

	bebox_vga_memory_rh = rh;
	bebox_vga_memory_wh = wh;

	memory_install_read64_handler(space, 0xC00A0000, 0xC00BFFFF, 0, 0, SMH_NOP);
	memory_install_write64_handler(space, 0xC00A0000, 0xC00BFFFF, 0, 0, SMH_NOP);

	memory_install_read64_handler(space, 0xC0000000 + begin, 0xC0000000 + end, 0, 0, rh64);
	memory_install_write64_handler(space, 0xC0000000 + begin, 0xC0000000 + end, 0, 0, wh64);
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
	return read64be_with_read8_handler(at_page8_r, space, offset, mem_mask);
}


WRITE64_HANDLER(bebox_page_w)
{
	write64be_with_write8_handler(at_page8_w, space, offset, data, mem_mask);
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
	write64be_with_write8_handler(at_hipage8_w, space, offset, data, mem_mask);
}


static DMA8237_HRQ_CHANGED( bebox_dma_hrq_changed )
{
	cputag_set_input_line(device->machine, "ppc1", INPUT_LINE_HALT, state ? ASSERT_LINE : CLEAR_LINE);

	/* Assert HLDA */
	dma8237_set_hlda( device, state );
}


static DMA8237_MEM_READ( bebox_dma_read_byte )
{
	const address_space *space = cputag_get_address_space(device->machine, "ppc1", ADDRESS_SPACE_PROGRAM);
	offs_t page_offset = (((offs_t) dma_offset[0][channel]) << 16)
		& 0x7FFF0000;
	return memory_read_byte(space, page_offset + offset);
}


static DMA8237_MEM_WRITE( bebox_dma_write_byte )
{
	const address_space *space = cputag_get_address_space(device->machine, "ppc1", ADDRESS_SPACE_PROGRAM);
	offs_t page_offset = (((offs_t) dma_offset[0][channel]) << 16)
		& 0x7FFF0000;
	memory_write_byte(space, page_offset + offset, data);
}


static DMA8237_CHANNEL_READ( bebox_dma8237_fdc_dack_r ) {
	return pc_fdc_dack_r(device->machine);
}


static DMA8237_CHANNEL_WRITE( bebox_dma8237_fdc_dack_w ) {
	pc_fdc_dack_w( device->machine, data );
}


static DMA8237_OUT_EOP( bebox_dma8237_out_eop ) {
	pc_fdc_set_tc_state( device->machine, state );
}


const struct dma8237_interface bebox_dma8237_1_config =
{
	XTAL_14_31818MHz/3, /* this needs to be verified */

	bebox_dma_hrq_changed,
	bebox_dma_read_byte,
	bebox_dma_write_byte,

	{ 0, 0, bebox_dma8237_fdc_dack_r, 0 },
	{ 0, 0, bebox_dma8237_fdc_dack_w, 0 },
	bebox_dma8237_out_eop
};


const struct dma8237_interface bebox_dma8237_2_config =
{
	XTAL_14_31818MHz/3, /* this needs to be verified */

	NULL,
	NULL,
	NULL,

	{ NULL, NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL },
	NULL
};


/*************************************
 *
 *	8254 PIT
 *
 *************************************/

static void bebox_timer0_w(const device_config *device, int state)
{
	if ( bebox_devices.pic8259_master ) {
		pic8259_set_irq_line(bebox_devices.pic8259_master, 0, state);
	}
}


const struct pit8253_config bebox_pit8254_config =
{
	{
		{
			4772720/4,				/* heartbeat IRQ */
			bebox_timer0_w
		},
		{
			4772720/4,				/* dram refresh */
			NULL
		},
		{
			4772720/4,				/* pio port c pin 4, and speaker polling enough */
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
	return read64be_with_read8_handler(bebox_flash8_r, space, offset, mem_mask);
}


WRITE64_HANDLER( bebox_flash_w )
{
	write64be_with_write8_handler(bebox_flash8_w, space, offset, data, mem_mask);
}


/*************************************
 *
 *	Keyboard
 *
 *************************************/

static void bebox_keyboard_interrupt(running_machine *machine,int state)
{
	bebox_set_irq_bit(machine, 16, state);
	if ( bebox_devices.pic8259_master ) {
		pic8259_set_irq_line( bebox_devices.pic8259_master, 1, state);
	}
}

static int bebox_get_out2(running_machine *machine) {
	return pit8253_get_output((device_config*)devtag_get_device(machine, "pit8254"), 2 );
}

static const struct kbdc8042_interface bebox_8042_interface =
{
	KBDC8042_STANDARD,
	NULL,
	bebox_keyboard_interrupt,
	bebox_get_out2
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
		r |= (UINT64)lsi53c810_reg_r(space, reg+0) << 56;
	}
	if (!(mem_mask & U64(0x00ff000000000000))) {
		r |= (UINT64)lsi53c810_reg_r(space, reg+1) << 48;
	}
	if (!(mem_mask & U64(0x0000ff0000000000))) {
		r |= (UINT64)lsi53c810_reg_r(space, reg+2) << 40;
	}
	if (!(mem_mask & U64(0x000000ff00000000))) {
		r |= (UINT64)lsi53c810_reg_r(space, reg+3) << 32;
	}
	if (!(mem_mask & U64(0x00000000ff000000))) {
		r |= (UINT64)lsi53c810_reg_r(space, reg+4) << 24;
	}
	if (!(mem_mask & U64(0x0000000000ff0000))) {
		r |= (UINT64)lsi53c810_reg_r(space, reg+5) << 16;
	}
	if (!(mem_mask & U64(0x000000000000ff00))) {
		r |= (UINT64)lsi53c810_reg_r(space, reg+6) << 8;
	}
	if (!(mem_mask & U64(0x00000000000000ff))) {
		r |= (UINT64)lsi53c810_reg_r(space, reg+7) << 0;
	}

	return r;
}


static WRITE64_HANDLER( scsi53c810_w )
{
	int reg = offset*8;
	if (!(mem_mask & U64(0xff00000000000000))) {
		lsi53c810_reg_w(space, reg+0, data >> 56);
	}
	if (!(mem_mask & U64(0x00ff000000000000))) {
		lsi53c810_reg_w(space, reg+1, data >> 48);
	}
	if (!(mem_mask & U64(0x0000ff0000000000))) {
		lsi53c810_reg_w(space, reg+2, data >> 40);
	}
	if (!(mem_mask & U64(0x000000ff00000000))) {
		lsi53c810_reg_w(space, reg+3, data >> 32);
	}
	if (!(mem_mask & U64(0x00000000ff000000))) {
		lsi53c810_reg_w(space, reg+4, data >> 24);
	}
	if (!(mem_mask & U64(0x0000000000ff0000))) {
		lsi53c810_reg_w(space, reg+5, data >> 16);
	}
	if (!(mem_mask & U64(0x000000000000ff00))) {
		lsi53c810_reg_w(space, reg+6, data >> 8);
	}
	if (!(mem_mask & U64(0x00000000000000ff))) {
		lsi53c810_reg_w(space, reg+7, data >> 0);
	}
}


#define BYTE_REVERSE32(x)		(((x >> 24) & 0xff) | \
								((x >> 8) & 0xff00) | \
								((x << 8) & 0xff0000) | \
								((x << 24) & 0xff000000))

static UINT32 scsi53c810_fetch(running_machine *machine, UINT32 dsp)
{
	UINT32 result;
	result = memory_read_dword_64be(cputag_get_address_space(machine, "ppc1", ADDRESS_SPACE_PROGRAM), dsp & 0x7FFFFFFF);
	return BYTE_REVERSE32(result);
}


static void scsi53c810_irq_callback(running_machine *machine, int value)
{
	bebox_set_irq_bit(machine, 21, value);
}


static void scsi53c810_dma_callback(running_machine *machine, UINT32 src, UINT32 dst, int length, int byteswap)
{
}


UINT32 scsi53c810_pci_read(const device_config *busdevice, const device_config *device, int function, int offset, UINT32 mem_mask)
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


void scsi53c810_pci_write(const device_config *busdevice, const device_config *device, int function, int offset, UINT32 data, UINT32 mem_mask)
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
						const address_space *space = cputag_get_address_space(device->machine, "ppc1", ADDRESS_SPACE_PROGRAM);

						addr = (scsi53c810_data[5] | 0xC0000000) & ~0xFF;
						memory_install_read64_handler(space, addr, addr + 0xFF, 0, 0, scsi53c810_r);
						memory_install_write64_handler(space, addr, addr + 0xFF, 0, 0, scsi53c810_w);
					}
				}
				break;
		}
	}
}


static const SCSIConfigTable dev_table =
{
	2, /* 2 SCSI devices */
	{
		{ SCSI_ID_0, "harddisk1", SCSI_DEVICE_HARDDISK },	/* SCSI ID 0, using HD 0, HD */
		{ SCSI_ID_3, "cdrom", SCSI_DEVICE_CDROM }	/* SCSI ID 3, using CHD 0, CD-ROM */
	}
};

static const struct LSI53C810interface scsi53c810_intf =
{
	&dev_table,		/* SCSI device table */
	&scsi53c810_irq_callback,
	&scsi53c810_dma_callback,
	&scsi53c810_fetch,
};


static TIMER_CALLBACK( bebox_get_devices ) {
	bebox_devices.pic8259_master = (device_config*)devtag_get_device(machine, "pic8259_master");
	bebox_devices.pic8259_slave = (device_config*)devtag_get_device(machine, "pic8259_slave");
	bebox_devices.dma8237_1 = (device_config*)devtag_get_device(machine, "dma8237_1");
	bebox_devices.dma8237_2 = (device_config*)devtag_get_device(machine, "dma8237_2");
}


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
	bebox_devices.pic8259_master = NULL;
	bebox_devices.pic8259_slave = NULL;
	bebox_devices.dma8237_1 = NULL;
	bebox_devices.dma8237_2 = NULL;

	timer_set(machine,  attotime_zero, NULL, 0, bebox_get_devices );

	cputag_set_input_line(machine, "ppc1", INPUT_LINE_RESET, CLEAR_LINE);
	cputag_set_input_line(machine, "ppc2", INPUT_LINE_RESET, ASSERT_LINE);
}

static void bebox_exit(running_machine *machine)
{
	lsi53c810_exit(&scsi53c810_intf);
}

MACHINE_START( bebox )
{
	pc_fdc_init(machine, &bebox_fdc_interface);
	/* SCSI */
	lsi53c810_init(machine, &scsi53c810_intf);
	add_exit_callback(machine, bebox_exit);
}

DRIVER_INIT( bebox )
{
	const address_space *space_0 = cputag_get_address_space(machine, "ppc1", ADDRESS_SPACE_PROGRAM);
	const address_space *space_1 = cputag_get_address_space(machine, "ppc2", ADDRESS_SPACE_PROGRAM);
	offs_t vram_begin;
	offs_t vram_end;

	mpc105_init(machine, 0);

	/* set up boot and flash ROM */
	memory_set_bankptr(machine, 2, memory_region(machine, "user2"));
	intelflash_init(machine, 0, FLASH_FUJITSU_29F016A, memory_region(machine, "user1"));

	/* install MESS managed RAM */
	memory_install_read64_handler(space_0, 0, mess_ram_size - 1, 0, 0x02000000, SMH_BANK(3));
	memory_install_write64_handler(space_0, 0, mess_ram_size - 1, 0, 0x02000000, SMH_BANK(3));
	memory_install_read64_handler(space_1, 0, mess_ram_size - 1, 0, 0x02000000, SMH_BANK(3));
	memory_install_write64_handler(space_1, 0, mess_ram_size - 1, 0, 0x02000000, SMH_BANK(3));
	memory_set_bankptr(machine, 3, mess_ram);

	mc146818_init(machine, MC146818_STANDARD);
	pc_vga_init(machine, &bebox_vga_interface, &cirrus_svga_interface);
	kbdc8042_init(machine, &bebox_8042_interface);

	/* install VGA memory */
	vram_begin = 0xC1000000;
	vram_end = vram_begin + pc_vga_memory_size() - 1;
	memory_install_read64_handler(space_0, vram_begin, vram_end, 0, 0, bebox_video_r);
	memory_install_write64_handler(space_0, vram_begin, vram_end, 0, 0, bebox_video_w);
	memory_install_read64_handler(space_1, vram_begin, vram_end, 0, 0, bebox_video_r);
	memory_install_write64_handler(space_1, vram_begin, vram_end, 0, 0, bebox_video_w);

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
		memory_install_read64_handler(space_1, 0x9421FFF0, 0x9421FFFF, 0, 0, SMH_BANK(1));
		memory_set_bankptr(machine, 1, ops);
	}
}
