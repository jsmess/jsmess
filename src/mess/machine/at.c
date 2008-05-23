/***************************************************************************

	IBM AT Compatibles

***************************************************************************/

#include "driver.h"
#include "deprecat.h"

#include "cpu/i386/i386.h"

#include "machine/pic8259.h"
#include "machine/8237dma.h"
#include "machine/mc146818.h"
#include "machine/pc_turbo.h"

#include "video/pc_vga.h"
#include "video/pc_cga.h"

#include "machine/pit8253.h"
#include "machine/pcshare.h"
#include "machine/8042kbdc.h"
#include "includes/pc.h"
#include "includes/at.h"
#include "machine/pckeybrd.h"
#include "audio/pc.h"
#include "audio/sblaster.h"
#include "machine/i82439tx.h"

#include "machine/pc_fdc.h"
#include "machine/pc_hdc.h"
#include "includes/pc_mouse.h"

#define LOG_PORT80 1

static struct {
	const device_config	*pic8259_master;
	const device_config	*pic8259_slave;
	const device_config	*dma8237_1;
	const device_config	*dma8237_2;
} at_devices;

static const SOUNDBLASTER_CONFIG soundblaster = { 1,5, {1,0} };


/*************************************************************
 *
 * pic8259 configuration
 *
 *************************************************************/

static PIC8259_SET_INT_LINE( at_pic8259_master_set_int_line ) {
	cpunum_set_input_line(device->machine, 0, 0, interrupt ? HOLD_LINE : CLEAR_LINE);
}


static PIC8259_SET_INT_LINE( at_pic8259_slave_set_int_line ) {
	pic8259_set_irq_line( at_devices.pic8259_master, 2, interrupt);
}


const struct pic8259_interface at_pic8259_master_config = {
	at_pic8259_master_set_int_line
};


const struct pic8259_interface at_pic8259_slave_config = {
	at_pic8259_slave_set_int_line
};



/*************************************************************
 *
 * pit8254 configuration
 *
 *************************************************************/

static PIT8253_OUTPUT_CHANGED( pc_timer0_w )
{
	if ( at_devices.pic8259_master ) {
		pic8259_set_irq_line(at_devices.pic8259_master, 0, state);
	}
}


const struct pit8253_config at_pit8254_config =
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


static void at_set_gate_a20(int a20)
{
	/* set the CPU's A20 line */
	cpunum_set_input_line(Machine, 0, INPUT_LINE_A20, a20);
}


static void at_set_irq_line(int irq, int state) {
	pic8259_set_irq_line(at_devices.pic8259_master, irq, state);
}


static void at_set_keyb_int(int state) {
	pic8259_set_irq_line(at_devices.pic8259_master, 1, state);
}


static void init_at_common(const struct kbdc8042_interface *at8042)
{
	mess_init_pc_common(PCCOMMON_KEYBOARD_AT, at_set_keyb_int, at_set_irq_line);
	mc146818_init(MC146818_STANDARD);
	soundblaster_config(&soundblaster);
	kbdc8042_init(at8042);

	if (mess_ram_size > 0x0a0000)
	{
		offs_t ram_limit = 0x100000 + mess_ram_size - 0x0a0000;
		memory_install_read_handler(Machine, 0,  ADDRESS_SPACE_PROGRAM, 0x100000,  ram_limit - 1, 0, 0, 1);
		memory_install_write_handler(Machine, 0, ADDRESS_SPACE_PROGRAM, 0x100000,  ram_limit - 1, 0, 0, 1);
		memory_set_bankptr(1, mess_ram + 0xa0000);
	}
}



static void at_keyboard_interrupt(int state)
{
	pic8259_set_irq_line(at_devices.pic8259_master, 1, state);
}



/*************************************************************************
 *
 *      PC DMA stuff
 *
 *************************************************************************/

static UINT8 dma_offset[2][4];
static UINT8 at_pages[0x10];


READ8_HANDLER(at_page8_r)
{
	UINT8 data = at_pages[offset % 0x10];

	switch(offset % 8) {
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


WRITE8_HANDLER(at_page8_w)
{
	at_pages[offset % 0x10] = data;

	if (LOG_PORT80 && (offset == 0))
		logerror(" at_page8_w(): Port 80h <== 0x%02x (PC=0x%08x)\n", data, (unsigned) activecpu_get_reg(REG_PC));

	switch(offset % 8) {
	case 1:
		dma_offset[(offset / 8) & 1][2] = data;
		break;
	case 2:
		dma_offset[(offset / 8) & 1][3] = data;
		break;
	case 3:
		dma_offset[(offset / 8) & 1][1] = data;
		break;
	case 7:
		dma_offset[(offset / 8) & 1][0] = data;
		break;
	}
}


static DMA8237_MEM_READ( pc_dma_read_byte )
{
	UINT8 result;
	offs_t page_offset = (((offs_t) dma_offset[0][channel]) << 16)
		& 0xFF0000;

	cpuintrf_push_context(0);
	result = program_read_byte(page_offset + offset);
	cpuintrf_pop_context();

	return result;
}


static DMA8237_MEM_WRITE( pc_dma_write_byte )
{
	offs_t page_offset = (((offs_t) dma_offset[0][channel]) << 16)
		& 0xFF0000;

	cpuintrf_push_context(0);
	program_write_byte(page_offset + offset, data);
	cpuintrf_pop_context();
}


static DMA8237_CHANNEL_READ( at_dma8237_fdc_dack_r ) {
	return pc_fdc_dack_r();
}


static DMA8237_CHANNEL_READ( at_dma8237_hdc_dack_r ) {
	return pc_hdc_dack_r();
}


static DMA8237_CHANNEL_WRITE( at_dma8237_fdc_dack_w ) {
	pc_fdc_dack_w( data );
}


static DMA8237_CHANNEL_WRITE( at_dma8237_hdc_dack_w ) {
	pc_hdc_dack_w( data );
}


static DMA8237_OUT_EOP( at_dma8237_out_eop ) {
	pc_fdc_set_tc_state( state );
}


const struct dma8237_interface at_dma8237_1_config =
{
	0,
	1.0e-6, // 1us

	pc_dma_read_byte,
	pc_dma_write_byte,

	{ 0, 0, at_dma8237_fdc_dack_r, at_dma8237_hdc_dack_r },
	{ 0, 0, at_dma8237_fdc_dack_w, at_dma8237_hdc_dack_w },
	at_dma8237_out_eop
};


/* TODO: How is still hooked up in the actual machine? */
const struct dma8237_interface at_dma8237_2_config =
{
	0, 
	1.0e-6, // 1us 

	NULL,
	NULL,

	{ NULL, NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL },
	NULL
};


/**********************************************************
 *
 * COM hardware
 *
 **********************************************************/

/* called when a interrupt is set/cleared from com hardware */
static INS8250_INTERRUPT( at_com_interrupt_1 )
{
	pic8259_set_irq_line(at_devices.pic8259_master, 4, state);
}

static INS8250_INTERRUPT( at_com_interrupt_2 )
{
	pic8259_set_irq_line(at_devices.pic8259_master, 3, state);
}

/* called when com registers read/written - used to update peripherals that
are connected */
static void at_com_refresh_connected_common(const device_config *device, int n, int data)
{
	/* mouse connected to this port? */
	if (input_port_read_indexed(device->machine, 3) & (0x80>>n))
		pc_mouse_handshake_in(device,data);
}

static INS8250_HANDSHAKE_OUT( at_com_handshake_out_0 ) { at_com_refresh_connected_common( device, 0, data ); }
static INS8250_HANDSHAKE_OUT( at_com_handshake_out_1 ) { at_com_refresh_connected_common( device, 1, data ); }
static INS8250_HANDSHAKE_OUT( at_com_handshake_out_2 ) { at_com_refresh_connected_common( device, 2, data ); }
static INS8250_HANDSHAKE_OUT( at_com_handshake_out_3 ) { at_com_refresh_connected_common( device, 3, data ); }

/* PC interface to PC-com hardware. Done this way because PCW16 also
uses PC-com hardware and doesn't have the same setup! */
const ins8250_interface ibm5170_com_interface[4]=
{
	{
		1843200,
		at_com_interrupt_1,
		NULL,
		at_com_handshake_out_0,
		NULL
	},
	{
		1843200,
		at_com_interrupt_2,
		NULL,
		at_com_handshake_out_1,
		NULL
	},
	{
		1843200,
		at_com_interrupt_1,
		NULL,
		at_com_handshake_out_2,
		NULL
	},
	{
		1843200,
		at_com_interrupt_2,
		NULL,
		at_com_handshake_out_3,
		NULL
	}
};


/**********************************************************
 *
 * NEC uPD765 floppy interface
 *
 **********************************************************/

#define FDC_DMA 2

static void at_fdc_interrupt(int state)
{
	pic8259_set_irq_line(at_devices.pic8259_master, 6, state);
}


static void at_fdc_dma_drq(int state, int read_)
{
	dma8237_drq_write((device_config*)device_list_find_by_tag( Machine->config->devicelist, DMA8237, "dma8237_1" ), FDC_DMA, state);
}

static const struct pc_fdc_interface fdc_interface =
{
	NEC765A,
	NEC765_RDY_PIN_CONNECTED,
	at_fdc_interrupt,
	at_fdc_dma_drq,
};


static int at_get_out2(running_machine *machine) {
	return pit8253_get_output((device_config*)device_list_find_by_tag( machine->config->devicelist, PIT8254, "at_pit8254" ), 2 );
}


DRIVER_INIT( atcga )
{
	static const struct kbdc8042_interface at8042 =
	{
		KBDC8042_STANDARD, at_set_gate_a20, at_keyboard_interrupt, at_get_out2
	};
	init_at_common(&at8042);
}


DRIVER_INIT( atega )
{
	static const struct kbdc8042_interface at8042 =
	{
		KBDC8042_STANDARD, at_set_gate_a20, at_keyboard_interrupt, at_get_out2
	};
	UINT8	*dst = memory_region( REGION_CPU1 ) + 0xc0000;
	UINT8	*src = memory_region( REGION_USER1 ) + 0x3fff;
	int		i;

	init_at_common(&at8042);

	/* Perform the EGA bios address line swaps */
	for( i = 0; i < 0x4000; i++ )
	{
		*dst++ = *src--;
	}
}



DRIVER_INIT( at386 )
{
	static const struct kbdc8042_interface at8042 =
	{
		KBDC8042_AT386, at_set_gate_a20, at_keyboard_interrupt, at_get_out2
	};
	init_at_common(&at8042);
}



DRIVER_INIT( at586 )
{
	DRIVER_INIT_CALL(at386);
	intel82439tx_init();
}



static void at_map_vga_memory(offs_t begin, offs_t end, read8_machine_func rh, write8_machine_func wh)
{
	int buswidth;
	buswidth = cputype_databus_width(Machine->config->cpu[0].type, ADDRESS_SPACE_PROGRAM);
	switch(buswidth)
	{
		case 8:
			memory_install_read8_handler(Machine, 0, ADDRESS_SPACE_PROGRAM, 0xA0000, 0xBFFFF, 0, 0, SMH_NOP);
			memory_install_write8_handler(Machine, 0, ADDRESS_SPACE_PROGRAM, 0xA0000, 0xBFFFF, 0, 0, SMH_NOP);

			memory_install_read8_handler(Machine, 0, ADDRESS_SPACE_PROGRAM, begin, end, 0, 0, rh);
			memory_install_write8_handler(Machine, 0, ADDRESS_SPACE_PROGRAM, begin, end, 0, 0, wh);
			break;
	}
}



static const struct pc_vga_interface vga_interface =
{
	1,
	at_map_vga_memory,

	input_port_0_r,

	ADDRESS_SPACE_IO,
	0x0000
};



DRIVER_INIT( at_vga )
{
	static const struct kbdc8042_interface at8042 =
	{
		KBDC8042_STANDARD, at_set_gate_a20, at_keyboard_interrupt, at_get_out2
	};

	init_at_common(&at8042);
	pc_turbo_setup(0, 3, 0x02, 4.77/12, 1);
	pc_vga_init(machine, &vga_interface, NULL);
}



DRIVER_INIT( ps2m30286 )
{
	static const struct kbdc8042_interface at8042 =
	{
		KBDC8042_PS2, at_set_gate_a20, at_keyboard_interrupt, at_get_out2
	};
	init_at_common(&at8042);
	pc_turbo_setup(0, 3, 0x02, 4.77/12, 1);
	pc_vga_init(machine, &vga_interface, NULL);
}



static IRQ_CALLBACK(at_irq_callback)
{
	return pic8259_acknowledge( at_devices.pic8259_master);
}



MACHINE_START( at )
{
	cpunum_set_irq_callback(0, at_irq_callback);
	pc_fdc_init( &fdc_interface );
}



MACHINE_RESET( at )
{
	at_devices.pic8259_master = (device_config*)device_list_find_by_tag( machine->config->devicelist, PIC8259, "pic8259_master" );
	at_devices.pic8259_slave = (device_config*)device_list_find_by_tag( machine->config->devicelist, PIC8259, "pic8259_slave" );
	at_devices.dma8237_1 = (device_config*)device_list_find_by_tag( machine->config->devicelist, DMA8237, "dma8237_1" );
	at_devices.dma8237_2 = (device_config*)device_list_find_by_tag( machine->config->devicelist, DMA8237, "dma8237_2" );
	pc_mouse_set_serial_port( device_list_find_by_tag( machine->config->devicelist, NS16450, "ns16450_0" ) );
}

