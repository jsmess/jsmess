/***************************************************************************

	machine/pc.c

	Functions to emulate general aspects of the machine
	(RAM, ROM, interrupts, I/O ports)

	The information herein is heavily based on
	'Ralph Browns Interrupt List'
	Release 52, Last Change 20oct96

***************************************************************************/

#include <assert.h>
#include "driver.h"
#include "deprecat.h"
#include "includes/pc.h"

#include "machine/8255ppi.h"
#include "machine/uart8250.h"
#include "machine/mc146818.h"
#include "machine/pic8259.h"
#include "machine/pc_turbo.h"

#include "video/pc_vga.h"
#include "video/pc_cga.h"
#include "video/pc_aga.h"
#include "video/pc_mda.h"
#include "video/pc_t1t.h"

#include "machine/pit8253.h"

#include "includes/pc_mouse.h"
#include "machine/pckeybrd.h"

#include "includes/pclpt.h"
#include "machine/centroni.h"

#include "machine/pc_fdc.h"
#include "machine/pc_hdc.h"
#include "machine/nec765.h"
#include "includes/amstr_pc.h"
#include "includes/europc.h"
#include "includes/ibmpc.h"
#include "machine/pcshare.h"
#include "audio/pc.h"

#include "machine/8237dma.h"


static struct {
	device_config	*pic8259_master;
	device_config	*pic8259_slave;
	device_config	*dma8237_1;
	device_config	*dma8237_2;
} pc_devices;

/*************************************************************************
 *
 *      PC DMA stuff
 *
 *************************************************************************/

static UINT8 dma_offset[2][4];


READ8_HANDLER(pc_page_r)
{
	return 0xFF;
}


WRITE8_HANDLER(pc_page_w)
{
	switch(offset % 4) {
	case 1:
		dma_offset[0][2] = data;
		break;
	case 2:
		dma_offset[0][3] = data;
		break;
	case 3:
		dma_offset[0][0] = dma_offset[0][1] = data;
		break;
	}
}


static DMA8237_MEM_READ( pc_dma_read_byte )
{
	UINT8 result;
	offs_t page_offset = (((offs_t) dma_offset[0][channel]) << 16)
		& 0x0F0000;

	cpuintrf_push_context(0);
	result = program_read_byte(page_offset + offset);
	cpuintrf_pop_context();

	return result;
}


static DMA8237_MEM_WRITE( pc_dma_write_byte )
{
	offs_t page_offset = (((offs_t) dma_offset[0][channel]) << 16)
		& 0x0F0000;

	cpuintrf_push_context(0);
	program_write_byte(page_offset + offset, data);
	cpuintrf_pop_context();
}


static DMA8237_CHANNEL_READ( pc_dma8237_fdc_dack_r ) {
	return pc_fdc_dack_r();
}


static DMA8237_CHANNEL_READ( pc_dma8237_hdc_dack_r ) {
	return pc_hdc_dack_r();
}


static DMA8237_CHANNEL_WRITE( pc_dma8237_fdc_dack_w ) {
	pc_fdc_dack_w( data );
}


static DMA8237_CHANNEL_WRITE( pc_dma8237_hdc_dack_w ) {
	pc_hdc_dack_w( data );
}


static DMA8237_OUT_EOP( pc_dma8237_out_eop ) {
	pc_fdc_set_tc_state( state );
}


const struct dma8237_interface pc_dma8237_config =
{
	0,
	1.0e-6, // 1us

	pc_dma_read_byte,
	pc_dma_write_byte,

	{ 0, 0, pc_dma8237_fdc_dack_r, pc_dma8237_hdc_dack_r },
	{ 0, 0, pc_dma8237_fdc_dack_w, pc_dma8237_hdc_dack_w },
	pc_dma8237_out_eop
};


/*************************************************************
 *
 * pic8259 configuration
 *
 *************************************************************/

static PIC8259_SET_INT_LINE( pc_pic8259_master_set_int_line ) {
	cpunum_set_input_line(device->machine, 0, 0, interrupt ? HOLD_LINE : CLEAR_LINE);
}


static PIC8259_SET_INT_LINE( pc_pic8259_slave_set_int_line ) {
	pic8259_set_irq_line( pc_devices.pic8259_master, 2, interrupt);
}


const struct pic8259_interface pc_pic8259_master_config = {
	pc_pic8259_master_set_int_line
};


const struct pic8259_interface pc_pic8259_slave_config = {
	pc_pic8259_slave_set_int_line
};


/*************************************************************
 *
 * pit8253 configuration
 *
 *************************************************************/

static PIT8253_OUTPUT_CHANGED( pc_timer0_w )
{
	pic8259_set_irq_line(pc_devices.pic8259_master, 0, state);
}
 

const struct pit8253_config pc_pit8253_config =
{
	{
		{
			4772720/4,				/* heartbeat IRQ */
			pc_timer0_w,
			NULL
		}, {
			4772720/4,				/* dram refresh */
			NULL,
			NULL
		}, {
			4772720/4,				/* pio port c pin 4, and speaker polling enough */
			NULL,
			pc_sh_speaker_change_clock
		}
	}
};


/**********************************************************
 *
 * COM hardware
 *
 **********************************************************/

/* called when a interrupt is set/cleared from com hardware */
static void pc_com_interrupt(int nr, int state)
{
	static const int irq[4] = {4, 3, 4, 3};

	/* issue COM1/3 IRQ4, COM2/4 IRQ3 */
	pic8259_set_irq_line(pc_devices.pic8259_master, irq[nr], state);
}

/* called when com registers read/written - used to update peripherals that
are connected */
static void pc_com_refresh_connected(int n, int data)
{
	/* mouse connected to this port? */
	if (input_port_read_indexed(Machine, 3) & (0x80>>n))
		pc_mouse_handshake_in(n,data);
}

/* PC interface to PC-com hardware. Done this way because PCW16 also
uses PC-com hardware and doesn't have the same setup! */
static const uart8250_interface com_interface[4]=
{
	{
		TYPE8250,
		1843200,
		pc_com_interrupt,
		NULL,
		pc_com_refresh_connected
	},
	{
		TYPE8250,
		1843200,
		pc_com_interrupt,
		NULL,
		pc_com_refresh_connected
	},
	{
		TYPE8250,
		1843200,
		pc_com_interrupt,
		NULL,
		pc_com_refresh_connected
	},
	{
		TYPE8250,
		1843200,
		pc_com_interrupt,
		NULL,
		pc_com_refresh_connected
	}
};


/**********************************************************
 *
 * LPT interface
 *
 **********************************************************/

static const PC_LPT_CONFIG lpt_config[3]={
	{
		1,
		LPT_UNIDIRECTIONAL,
		NULL
	},
	{
		1,
		LPT_UNIDIRECTIONAL,
		NULL
	},
	{
		1,
		LPT_UNIDIRECTIONAL,
		NULL
	}
};

static const CENTRONICS_CONFIG cent_config[3]={
	{
		PRINTER_IBM,
		pc_lpt_handshake_in
	},
	{
		PRINTER_IBM,
		pc_lpt_handshake_in
	},
	{
		PRINTER_IBM,
		pc_lpt_handshake_in
	}
};



/**********************************************************
 *
 * NEC uPD765 floppy interface
 *
 **********************************************************/

#define FDC_DMA 2

static void pc_fdc_interrupt(int state)
{
	if ( pc_devices.pic8259_master ) {
		pic8259_set_irq_line(pc_devices.pic8259_master, 6, state);
	}
}

static void pc_fdc_dma_drq(int state, int read_)
{
	dma8237_drq_write( (device_config*)device_list_find_by_tag( Machine->config->devicelist, DMA8237, "dma8237" ), FDC_DMA, state);
}


static const struct pc_fdc_interface fdc_interface_nc =
{
	NEC765A,
	NEC765_RDY_PIN_NOT_CONNECTED,
	pc_fdc_interrupt,
	pc_fdc_dma_drq,
};

static void pc_set_irq_line(int irq, int state) {
	pic8259_set_irq_line(pc_devices.pic8259_master, irq, state);
}

static void pc_set_keyb_int(int state) {
	pc_set_irq_line( 1, state );
}

void mess_init_pc_common(UINT32 flags, void (*set_keyb_int_func)(int), void (*set_hdc_int_func)(int,int)) {
	init_pc_common(flags, set_keyb_int_func);

	/* MESS managed RAM */
	if ( mess_ram )
		memory_set_bankptr( 10, mess_ram );

	/* FDC/HDC hardware */
	pc_hdc_setup(set_hdc_int_func);

	/* com hardware */
	uart8250_init(0, com_interface);
	uart8250_reset(0);
	uart8250_init(1, com_interface+1);
	uart8250_reset(1);
	uart8250_init(2, com_interface+2);
	uart8250_reset(2);
	uart8250_init(3, com_interface+3);
	uart8250_reset(3);

	pc_lpt_config(0, lpt_config);
	centronics_config(0, cent_config);
	pc_lpt_set_device(0, &CENTRONICS_PRINTER_DEVICE);
	pc_lpt_config(1, lpt_config+1);
	centronics_config(1, cent_config+1);
	pc_lpt_set_device(1, &CENTRONICS_PRINTER_DEVICE);
	pc_lpt_config(2, lpt_config+2);
	centronics_config(2, cent_config+2);
	pc_lpt_set_device(2, &CENTRONICS_PRINTER_DEVICE);

	/* serial mouse */
	pc_mouse_set_serial_port(0);
	pc_mouse_initialise();
}

DRIVER_INIT( pccga )
{
	mess_init_pc_common(PCCOMMON_KEYBOARD_PC, pc_set_keyb_int, pc_set_irq_line);
	pc_rtc_init();
	pc_turbo_setup(0, 3, 0x02, 4.77/12, 1);
}

DRIVER_INIT( bondwell )
{
	mess_init_pc_common(PCCOMMON_KEYBOARD_PC, pc_set_keyb_int, pc_set_irq_line);
	pc_turbo_setup(0, 3, 0x02, 4.77/12, 1);
}

DRIVER_INIT( pcmda )
{
	mess_init_pc_common(PCCOMMON_KEYBOARD_PC, pc_set_keyb_int, pc_set_irq_line);
	pc_turbo_setup(0, 3, 0x02, 4.77/12, 1);
}

DRIVER_INIT( europc )
{
	UINT8 *gfx = &memory_region(REGION_GFX1)[0x8000];
	UINT8 *rom = &memory_region(REGION_CPU1)[0];
	int i;

    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;

	/*
	  fix century rom bios bug !
	  if year <79 month (and not CENTURY) is loaded with 0x20
	*/
	if (rom[0xff93e]==0xb6){ // mov dh,
		UINT8 a;
		rom[0xff93e]=0xb5; // mov ch,
		for (i=0xf8000, a=0; i<0xfffff; i++ ) a+=rom[i];
		rom[0xfffff]=256-a;
	}

	mess_init_pc_common(PCCOMMON_KEYBOARD_PC, pc_set_keyb_int, pc_set_irq_line);

	europc_rtc_init();
//	europc_rtc_set_time(machine);
}

DRIVER_INIT( t1000hx )
{
	UINT8 *gfx = &memory_region(REGION_GFX1)[0x1000];
	int i;
    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;
	mess_init_pc_common(PCCOMMON_KEYBOARD_PC, pc_set_keyb_int, pc_set_irq_line);
	pc_turbo_setup(0, 3, 0x02, 4.77/12, 1);
}

DRIVER_INIT( pc200 )
{
	UINT8 *gfx = &memory_region(REGION_GFX1)[0x8000];
	int i;

    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;

	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xb0000, 0xbffff, 0, 0, pc200_videoram16le_r );
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xb0000, 0xbffff, 0, 0, pc200_videoram16le_w );
	videoram_size=0x10000;
	videoram=memory_region(REGION_CPU1)+0xb0000;
	memory_install_read16_handler(0, ADDRESS_SPACE_IO, 0x278, 0x27b, 0, 0, pc200_16le_port378_r );

	mess_init_pc_common(PCCOMMON_KEYBOARD_PC, pc_set_keyb_int, pc_set_irq_line);
}

DRIVER_INIT( pc1512 )
{
	UINT8 *gfx = &memory_region(REGION_GFX1)[0x8000];
	int i;

    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;

	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xb8000, 0xbbfff, 0, 0x0C000, SMH_BANK1 );
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xb8000, 0xbbfff, 0, 0x0C000, pc1512_videoram16le_w );

	memory_install_read16_handler(0, ADDRESS_SPACE_IO, 0x3d0, 0x3df, 0, 0, pc1512_16le_r );
	memory_install_write16_handler(0, ADDRESS_SPACE_IO, 0x3d0, 0x3df, 0, 0, pc1512_16le_w );

	memory_install_read16_handler(0, ADDRESS_SPACE_IO, 0x278, 0x27b, 0, 0, pc16le_parallelport2_r );


	mess_init_pc_common(PCCOMMON_KEYBOARD_PC, pc_set_keyb_int, pc_set_irq_line);
	mc146818_init(MC146818_IGNORE_CENTURY);
}



static void pc_map_vga_memory(offs_t begin, offs_t end, read8_machine_func rh, write8_machine_func wh)
{
	int buswidth;
	buswidth = cputype_databus_width(Machine->config->cpu[0].type, ADDRESS_SPACE_PROGRAM);
	switch(buswidth)
	{
		case 8:
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xA0000, 0xBFFFF, 0, 0, SMH_NOP);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xA0000, 0xBFFFF, 0, 0, SMH_NOP);

			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, begin, end, 0, 0, rh);
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, begin, end, 0, 0, wh);
			break;

		default:
			fatalerror("VGA:  Bus width %d not supported\n", buswidth);
			break;
	}
}



static const struct pc_vga_interface vga_interface =
{
	1,
	pc_map_vga_memory,

	input_port_0_r,

	ADDRESS_SPACE_IO,
	0x0000
};



DRIVER_INIT( pc1640 )
{
	pc_vga_init(machine, &vga_interface, NULL);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xa0000, 0xaffff, 0, 0, SMH_BANK1 );
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xb0000, 0xb7fff, 0, 0, SMH_BANK2 );
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xb8000, 0xbffff, 0, 0, SMH_BANK3 );

	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xa0000, 0xaffff, 0, 0, SMH_BANK1 );
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xb0000, 0xb7fff, 0, 0, SMH_BANK2 );
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xb8000, 0xbffff, 0, 0, SMH_BANK3 );

	memory_install_read16_handler(0, ADDRESS_SPACE_IO, 0x3b0, 0x3bf, 0, 0, vga_port16le_03b0_r );
	memory_install_read16_handler(0, ADDRESS_SPACE_IO, 0x3c0, 0x3cf, 0, 0, paradise_ega16le_03c0_r );
	memory_install_read16_handler(0, ADDRESS_SPACE_IO, 0x3d0, 0x3df, 0, 0, pc1640_16le_port3d0_r );

	memory_install_write16_handler(0, ADDRESS_SPACE_IO, 0x3b0, 0x3bf, 0, 0, vga_port16le_03b0_w );
	memory_install_write16_handler(0, ADDRESS_SPACE_IO, 0x3c0, 0x3cf, 0, 0, vga_port16le_03c0_w );
	memory_install_write16_handler(0, ADDRESS_SPACE_IO, 0x3d0, 0x3df, 0, 0, vga_port16le_03d0_w );

	memory_install_read16_handler(0, ADDRESS_SPACE_IO, 0x278, 0x27b, 0, 0, pc1640_16le_port278_r );
	memory_install_read16_handler(0, ADDRESS_SPACE_IO, 0x4278, 0x427b, 0, 0, pc1640_16le_port4278_r );

	mess_init_pc_common(PCCOMMON_KEYBOARD_PC, pc_set_keyb_int, pc_set_irq_line);

	mc146818_init(MC146818_IGNORE_CENTURY);
}

DRIVER_INIT( pc_vga )
{
	mess_init_pc_common(PCCOMMON_KEYBOARD_PC, pc_set_keyb_int, pc_set_irq_line);

	pc_vga_init(machine, &vga_interface, NULL);
}

static IRQ_CALLBACK(pc_irq_callback)
{
	return pic8259_acknowledge( pc_devices.pic8259_master );
}


MACHINE_START( pc ) {
	pc_fdc_init( &fdc_interface_nc );
}


MACHINE_RESET( pc )
{
	cpunum_set_irq_callback(0, pc_irq_callback);

	pc_devices.pic8259_master = (device_config*)device_list_find_by_tag( machine->config->devicelist, PIC8259, "pic8259_master" );
	pc_devices.pic8259_slave = (device_config*)device_list_find_by_tag( machine->config->devicelist, PIC8259, "pic8259_slave" );
	pc_devices.dma8237_1 = (device_config*)device_list_find_by_tag( machine->config->devicelist, DMA8237, "dma8237_1" );
	pc_devices.dma8237_2 = (device_config*)device_list_find_by_tag( machine->config->devicelist, DMA8237, "dma8237_2" );
}


/**************************************************************************
 *
 *      Interrupt handlers.
 *
 **************************************************************************/
static void pc_generic_frame_interrupt(void (*pc_timer)(void))
{
	if (pc_timer)
		pc_timer();

	pc_keyboard();
}

INTERRUPT_GEN( pc_mda_frame_interrupt )
{
	pc_generic_frame_interrupt(NULL);
}

INTERRUPT_GEN( pc_cga_frame_interrupt )
{
	pc_generic_frame_interrupt(NULL);
}

INTERRUPT_GEN( pc_pc1512_frame_interrupt )
{
	pc_generic_frame_interrupt(NULL);
}

INTERRUPT_GEN( tandy1000_frame_interrupt )
{
	pc_generic_frame_interrupt(NULL);
}

INTERRUPT_GEN( pc_aga_frame_interrupt )
{
	pc_generic_frame_interrupt(NULL);
}

INTERRUPT_GEN( pc_vga_frame_interrupt )
{
	pc_generic_frame_interrupt(NULL /* vga_timer */);
}

