/***************************************************************************

    IBM AT Compatibles

***************************************************************************/

#include "emu.h"

#include "cpu/i386/i386.h"

#include "machine/pic8259.h"
#include "machine/8237dma.h"
#include "machine/ins8250.h"
#include "machine/mc146818.h"
#include "machine/pc_turbo.h"

#include "video/pc_vga.h"
#include "video/pc_cga.h"

#include "machine/pit8253.h"
#include "includes/at.h"
#include "sound/speaker.h"
#include "audio/sblaster.h"
#include "machine/i82439tx.h"

#include "machine/pc_fdc.h"
#include "machine/pc_hdc.h"
#include "includes/pc_mouse.h"

#include "machine/ram.h"

#define LOG_PORT80	0


static const SOUNDBLASTER_CONFIG soundblaster = { 1,5, {1,0} };
static int poll_delay;


/*************************************************************
 *
 * pic8259 configuration
 *
 *************************************************************/

const struct pic8259_interface at_pic8259_master_config =
{
	DEVCB_CPU_INPUT_LINE("maincpu", 0)
};

const struct pic8259_interface at_pic8259_slave_config =
{
	DEVCB_DEVICE_LINE("pic8259_master", pic8259_ir2_w)
};



/*************************************************************************
 *
 *      PC Speaker related
 *
 *************************************************************************/

static UINT8 at_spkrdata = 0;
static UINT8 at_speaker_input = 0;

static UINT8 at_speaker_get_spk(void)
{
	return at_spkrdata & at_speaker_input;
}


static void at_speaker_set_spkrdata(running_machine &machine, UINT8 data)
{
	device_t *speaker = machine.device("speaker");
	at_spkrdata = data ? 1 : 0;
	speaker_level_w( speaker, at_speaker_get_spk() );
}


static void at_speaker_set_input(running_machine &machine, UINT8 data)
{
	device_t *speaker = machine.device("speaker");
	at_speaker_input = data ? 1 : 0;
	speaker_level_w( speaker, at_speaker_get_spk() );
}



/*************************************************************
 *
 * pit8254 configuration
 *
 *************************************************************/

static WRITE_LINE_DEVICE_HANDLER( at_pit8254_out0_changed )
{
	at_state *st = device->machine().driver_data<at_state>();
	if (st->m_pic8259_master)
	{
		pic8259_ir0_w(st->m_pic8259_master, state);
	}
}


static WRITE_LINE_DEVICE_HANDLER( at_pit8254_out2_changed )
{
	at_speaker_set_input( device->machine(), state ? 1 : 0 );
}


const struct pit8253_config at_pit8254_config =
{
	{
		{
			4772720/4,				/* heartbeat IRQ */
			DEVCB_NULL,
			DEVCB_LINE(at_pit8254_out0_changed)
		}, {
			4772720/4,				/* dram refresh */
			DEVCB_NULL,
			DEVCB_NULL
		}, {
			4772720/4,				/* pio port c pin 4, and speaker polling enough */
			DEVCB_NULL,
			DEVCB_LINE(at_pit8254_out2_changed)
		}
	}
};


static void at_set_irq_line(running_machine &machine,int irq, int state)
{
	at_state *st = machine.driver_data<at_state>();

	switch (irq)
	{
	case 0: pic8259_ir0_w(st->m_pic8259_master, state); break;
	case 1: pic8259_ir1_w(st->m_pic8259_master, state); break;
	case 2: pic8259_ir2_w(st->m_pic8259_master, state); break;
	case 3: pic8259_ir3_w(st->m_pic8259_master, state); break;
	case 4: pic8259_ir4_w(st->m_pic8259_master, state); break;
	case 5: pic8259_ir5_w(st->m_pic8259_master, state); break;
	case 6: pic8259_ir6_w(st->m_pic8259_master, state); break;
	case 7: pic8259_ir7_w(st->m_pic8259_master, state); break;
	}
}



/*************************************************************************
 *
 *      PC DMA stuff
 *
 *************************************************************************/

static int dma_channel;
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
	{
		logerror(" at_page8_w(): Port 80h <== 0x%02x (PC=0x%08x)\n", data,
							(unsigned) cpu_get_reg(space->machine().device("maincpu"), STATE_GENPC));
	}

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


static WRITE_LINE_DEVICE_HANDLER( pc_dma_hrq_changed )
{
	at_state *st = device->machine().driver_data<at_state>();
	device_set_input_line(st->m_maincpu, INPUT_LINE_HALT, state ? ASSERT_LINE : CLEAR_LINE);

	/* Assert HLDA */
	i8237_hlda_w( device, state );
}

static READ8_HANDLER( pc_dma_read_byte )
{
	UINT8 result;
	offs_t page_offset = (((offs_t) dma_offset[0][dma_channel]) << 16)
		& 0xFF0000;

	result = space->read_byte(page_offset + offset);
	return result;
}


static WRITE8_HANDLER( pc_dma_write_byte )
{
	offs_t page_offset = (((offs_t) dma_offset[0][dma_channel]) << 16)
		& 0xFF0000;

	space->write_byte(page_offset + offset, data);
}


static READ8_DEVICE_HANDLER( at_dma8237_fdc_dack_r ) {
	return pc_fdc_dack_r(device->machine());
}


static READ8_DEVICE_HANDLER( at_dma8237_hdc_dack_r ) {
	return pc_hdc_dack_r(device->machine());
}


static WRITE8_DEVICE_HANDLER( at_dma8237_fdc_dack_w ) {
	pc_fdc_dack_w( device->machine(), data );
}


static WRITE8_DEVICE_HANDLER( at_dma8237_hdc_dack_w ) {
	pc_hdc_dack_w( device->machine(), data );
}


static WRITE_LINE_DEVICE_HANDLER( at_dma8237_out_eop ) {
	pc_fdc_set_tc_state( device->machine(), state );
}

static void set_dma_channel(device_t *device, int channel, int state)
{
	if (!state) dma_channel = channel;
}

static WRITE_LINE_DEVICE_HANDLER( pc_dack0_w ) { set_dma_channel(device, 0, state); }
static WRITE_LINE_DEVICE_HANDLER( pc_dack1_w ) { set_dma_channel(device, 1, state); }
static WRITE_LINE_DEVICE_HANDLER( pc_dack2_w ) { set_dma_channel(device, 2, state); }
static WRITE_LINE_DEVICE_HANDLER( pc_dack3_w ) { set_dma_channel(device, 3, state); }

I8237_INTERFACE( at_dma8237_1_config )
{
	DEVCB_DEVICE_LINE("dma8237_2",i8237_dreq0_w),
	DEVCB_LINE(at_dma8237_out_eop),
	DEVCB_MEMORY_HANDLER("maincpu", PROGRAM, pc_dma_read_byte),
	DEVCB_MEMORY_HANDLER("maincpu", PROGRAM, pc_dma_write_byte),
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_HANDLER(at_dma8237_fdc_dack_r), DEVCB_HANDLER(at_dma8237_hdc_dack_r) },
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_HANDLER(at_dma8237_fdc_dack_w), DEVCB_HANDLER(at_dma8237_hdc_dack_w) },
	{ DEVCB_LINE(pc_dack0_w), DEVCB_LINE(pc_dack1_w), DEVCB_LINE(pc_dack2_w), DEVCB_LINE(pc_dack3_w) }
};


/* TODO: How is this hooked up in the actual machine? */
I8237_INTERFACE( at_dma8237_2_config )
{
	DEVCB_LINE(pc_dma_hrq_changed),
	DEVCB_NULL,
	DEVCB_MEMORY_HANDLER("maincpu", PROGRAM, pc_dma_read_byte),
	DEVCB_MEMORY_HANDLER("maincpu", PROGRAM, pc_dma_write_byte),
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL },
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL },
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL }
};


/**********************************************************
 *
 * COM hardware
 *
 **********************************************************/

/* called when a interrupt is set/cleared from com hardware */
static INS8250_INTERRUPT( at_com_interrupt_1 )
{
	at_state *st = device->machine().driver_data<at_state>();
	pic8259_ir4_w(st->m_pic8259_master, state);
}

static INS8250_INTERRUPT( at_com_interrupt_2 )
{
	at_state *st = device->machine().driver_data<at_state>();
	pic8259_ir3_w(st->m_pic8259_master, state);
}

/* called when com registers read/written - used to update peripherals that
are connected */
static void at_com_refresh_connected_common(device_t *device, int n, int data)
{
	/* mouse connected to this port? */
	if (input_port_read(device->machine(), "DSW2") & (0x80>>n))
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

static void at_fdc_interrupt(running_machine &machine, int state)
{
	at_state *st = machine.driver_data<at_state>();
	pic8259_ir6_w(st->m_pic8259_master, state);
//if ( ram_get_ptr(machine.device(RAM_TAG))[0x0490] == 0x74 )
//  ram_get_ptr(machine.device(RAM_TAG))[0x0490] = 0x54;
}


static void at_fdc_dma_drq(running_machine &machine, int state)
{
	at_state *st = machine.driver_data<at_state>();
	i8237_dreq2_w( st->m_dma8237_1, state);
}

static device_t *at_get_device(running_machine &machine)
{
	return machine.device("upd765");
}

static const struct pc_fdc_interface fdc_interface =
{
	at_fdc_interrupt,
	at_fdc_dma_drq,
	NULL,
	at_get_device
};





static UINT8 at_speaker;
static UINT8 at_offset1;

READ8_HANDLER( at_portb_r )
{
	at_state *st = space->machine().driver_data<at_state>();

	UINT8 data = at_speaker;
	data &= ~0xc0; /* AT BIOS don't likes this being set */

	/* This needs fixing/updating not sure what this is meant to fix */
	if ( --poll_delay < 0 )
	{
		poll_delay = 3;
		at_offset1 ^= 0x10;
	}
	data = (data & ~0x10) | ( at_offset1 & 0x10 );

	if ( pit8253_get_output(st->m_pit8254, 2 ) )
		data |= 0x20;
	else
		data &= ~0x20; /* ps2m30 wants this */

	return data;
}

WRITE8_HANDLER( at_portb_w )
{
	at_state *st = space->machine().driver_data<at_state>();
	at_speaker = data;
	pit8253_gate2_w(st->m_pit8254, BIT(data, 0));
	at_speaker_set_spkrdata( space->machine(), data & 0x02 );
}


/**********************************************************
 *
 * Init functions
 *
 **********************************************************/

static void init_at_common(running_machine &machine)
{
	address_space* space = machine.device("maincpu")->memory().space(AS_PROGRAM);
	soundblaster_config(&soundblaster);

	// The CS4031 chipset does this itself
	if (machine.device("cs4031") == NULL)
	{
		/* MESS managed RAM */
		memory_set_bankptr(machine, "bank10", ram_get_ptr(machine.device(RAM_TAG)));

		if (ram_get_size(machine.device(RAM_TAG)) > 0x0a0000)
		{
			offs_t ram_limit = 0x100000 + ram_get_size(machine.device(RAM_TAG)) - 0x0a0000;
			space->install_read_bank(0x100000,  ram_limit - 1, "bank1");
			space->install_write_bank(0x100000,  ram_limit - 1, "bank1");
			memory_set_bankptr(machine, "bank1", ram_get_ptr(machine.device(RAM_TAG)) + 0xa0000);
		}
	}

	/* FDC/HDC hardware */
	pc_hdc_setup(machine, at_set_irq_line);

	/* serial mouse */
	pc_mouse_initialise(machine);

	at_offset1 = 0xff;
}


static READ8_HANDLER( input_port_0_r ) { return input_port_read(space->machine(), "IN0"); }

static const struct pc_vga_interface vga_interface =
{
	NULL,
	NULL,
	input_port_0_r,
	AS_IO,
	0x0000
};


DRIVER_INIT( atcga )
{
	init_at_common(machine);
}


DRIVER_INIT( atega )
{
	UINT8	*dst = machine.region( "maincpu" )->base() + 0xc0000;
	UINT8	*src = machine.region( "user1" )->base() + 0x3fff;
	int		i;

	init_at_common(machine);

	/* Perform the EGA bios address line swaps */
	for( i = 0; i < 0x4000; i++ )
	{
		*dst++ = *src--;
	}
}



DRIVER_INIT( at386 )
{
	init_at_common(machine);
	pc_vga_init(machine, &vga_interface, NULL);
}

DRIVER_INIT( at_vga )
{
	init_at_common(machine);
	pc_turbo_setup(machine, machine.firstcpu, "DSW2", 0x02, 4.77/12, 1);
	pc_vga_init(machine, &vga_interface, NULL);
}



DRIVER_INIT( ps2m30286 )
{
	init_at_common(machine);
	pc_turbo_setup(machine, machine.firstcpu, "DSW2", 0x02, 4.77/12, 1);
	pc_vga_init(machine, &vga_interface, NULL);
}



static IRQ_CALLBACK(at_irq_callback)
{
	at_state *st = device->machine().driver_data<at_state>();
	return pic8259_acknowledge(st->m_pic8259_master);
}

static void pc_set_irq_line(running_machine &machine,int irq, int state)
{
	at_state *st = machine.driver_data<at_state>();;

	switch (irq)
	{
	case 0: pic8259_ir0_w(st->m_pic8259_master, state); break;
	case 1: pic8259_ir1_w(st->m_pic8259_master, state); break;
	case 2: pic8259_ir2_w(st->m_pic8259_master, state); break;
	case 3: pic8259_ir3_w(st->m_pic8259_master, state); break;
	case 4: pic8259_ir4_w(st->m_pic8259_master, state); break;
	case 5: pic8259_ir5_w(st->m_pic8259_master, state); break;
	case 6: pic8259_ir6_w(st->m_pic8259_master, state); break;
	case 7: pic8259_ir7_w(st->m_pic8259_master, state); break;
	}
}

MACHINE_START( at )
{
	device_set_irq_callback(machine.device("maincpu"), at_irq_callback);
	/* FDC/HDC hardware */
	pc_fdc_init( machine, &fdc_interface );
	pc_hdc_setup(machine, pc_set_irq_line);
}



MACHINE_RESET( at )
{
	at_state *st = machine.driver_data<at_state>();
	st->m_maincpu = machine.device("maincpu");
	st->m_pic8259_master = machine.device("pic8259_master");
	st->m_pic8259_slave = machine.device("pic8259_slave");
	st->m_dma8237_1 = machine.device("dma8237_1");
	st->m_dma8237_2 = machine.device("dma8237_2");
	st->m_pit8254 = machine.device("pit8254");
	pc_mouse_set_serial_port( machine.device("ns16450_0") );
	pc_hdc_set_dma8237_device( st->m_dma8237_1 );
	poll_delay = 4;
}
