/***************************************************************************

    IBM AT Compatibles

***************************************************************************/

#include "driver.h"

#include "cpu/i386/i386.h"
#include "cpu/mcs48/mcs48.h"

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
#include "sound/speaker.h"
#include "audio/sblaster.h"
#include "machine/i82439tx.h"

#include "machine/pc_fdc.h"
#include "machine/pc_hdc.h"
#include "includes/pc_mouse.h"

#include "machine/kb_keytro.h"

#include "devices/messram.h"

#define LOG_PORT80	0
#define LOG_KBDC	0

static const SOUNDBLASTER_CONFIG soundblaster = { 1,5, {1,0} };


/*************************************************************
 *
 * pic8259 configuration
 *
 *************************************************************/

static PIC8259_SET_INT_LINE( at_pic8259_master_set_int_line ) {
	cputag_set_input_line(device->machine, "maincpu", 0, interrupt ? ASSERT_LINE : CLEAR_LINE);
}


static PIC8259_SET_INT_LINE( at_pic8259_slave_set_int_line ) {
	at_state *st = device->machine->driver_data;
	pic8259_set_irq_line( st->pic8259_master, 2, interrupt);
}


const struct pic8259_interface at_pic8259_master_config = {
	at_pic8259_master_set_int_line
};


const struct pic8259_interface at_pic8259_slave_config = {
	at_pic8259_slave_set_int_line
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


static void at_speaker_set_spkrdata(running_machine *machine, UINT8 data)
{
	const device_config *speaker = devtag_get_device(machine, "speaker");
	at_spkrdata = data ? 1 : 0;
	speaker_level_w( speaker, at_speaker_get_spk() );
}


static void at_speaker_set_input(running_machine *machine, UINT8 data)
{
	const device_config *speaker = devtag_get_device(machine, "speaker");
	at_speaker_input = data ? 1 : 0;
	speaker_level_w( speaker, at_speaker_get_spk() );
}



/*************************************************************
 *
 * pit8254 configuration
 *
 *************************************************************/

static PIT8253_OUTPUT_CHANGED( at_pit8254_out0_changed )
{
	at_state *st = device->machine->driver_data;
	if ( st->pic8259_master ) {
		pic8259_set_irq_line(st->pic8259_master, 0, state);
	}
}


static PIT8253_OUTPUT_CHANGED( at_pit8254_out2_changed )
{
	at_speaker_set_input( device->machine, state ? 1 : 0 );
}


const struct pit8253_config at_pit8254_config =
{
	{
		{
			4772720/4,				/* heartbeat IRQ */
			at_pit8254_out0_changed
		}, {
			4772720/4,				/* dram refresh */
			NULL
		}, {
			4772720/4,				/* pio port c pin 4, and speaker polling enough */
			at_pit8254_out2_changed
		}
	}
};


static void at_set_gate_a20(running_machine *machine, int a20)
{
	/* set the CPU's A20 line */
	cputag_set_input_line(machine, "maincpu", INPUT_LINE_A20, a20);
}


static void at_set_irq_line(running_machine *machine,int irq, int state) {
	at_state *st = machine->driver_data;
	pic8259_set_irq_line(st->pic8259_master, irq, state);
}


static void at_set_keyb_int(running_machine *machine, int state) {
	at_state *st = machine->driver_data;
	pic8259_set_irq_line(st->pic8259_master, 1, state);
}


static void init_at_common(running_machine *machine, const struct kbdc8042_interface *at8042)
{
	const address_space* space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	mess_init_pc_common(machine, PCCOMMON_KEYBOARD_AT, at_set_keyb_int, at_set_irq_line);
	mc146818_init(machine, MC146818_STANDARD);
	soundblaster_config(&soundblaster);
	kbdc8042_init(machine, at8042);

	if (messram_get_size(devtag_get_device(machine, "messram")) > 0x0a0000)
	{
		offs_t ram_limit = 0x100000 + messram_get_size(devtag_get_device(machine, "messram")) - 0x0a0000;
		memory_install_read_bank(space, 0x100000,  ram_limit - 1, 0, 0, "bank1");
		memory_install_write_bank(space, 0x100000,  ram_limit - 1, 0, 0, "bank1");
		memory_set_bankptr(machine, "bank1", messram_get_ptr(devtag_get_device(machine, "messram")) + 0xa0000);
	}
}



static void at_keyboard_interrupt(running_machine *machine, int state)
{
	at_state *st = machine->driver_data;
	pic8259_set_irq_line(st->pic8259_master, 1, state);
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
							(unsigned) cpu_get_reg(cputag_get_cpu(space->machine, "maincpu"), REG_GENPC));
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
	at_state *st = device->machine->driver_data;
	cpu_set_input_line(st->maincpu, INPUT_LINE_HALT, state ? ASSERT_LINE : CLEAR_LINE);

	/* Assert HLDA */
	i8237_hlda_w( device, state );
}

static READ8_HANDLER( pc_dma_read_byte )
{
	UINT8 result;
	offs_t page_offset = (((offs_t) dma_offset[0][dma_channel]) << 16)
		& 0xFF0000;

	result = memory_read_byte(space, page_offset + offset);
	return result;
}


static WRITE8_HANDLER( pc_dma_write_byte )
{
	offs_t page_offset = (((offs_t) dma_offset[0][dma_channel]) << 16)
		& 0xFF0000;

	memory_write_byte(space, page_offset + offset, data);
}


static READ8_DEVICE_HANDLER( at_dma8237_fdc_dack_r ) {
	return pc_fdc_dack_r(device->machine);
}


static READ8_DEVICE_HANDLER( at_dma8237_hdc_dack_r ) {
	return pc_hdc_dack_r( device->machine );
}


static WRITE8_DEVICE_HANDLER( at_dma8237_fdc_dack_w ) {
	pc_fdc_dack_w( device->machine, data );
}


static WRITE8_DEVICE_HANDLER( at_dma8237_hdc_dack_w ) {
	pc_hdc_dack_w( device->machine, data );
}


static WRITE_LINE_DEVICE_HANDLER( at_dma8237_out_eop ) {
	pc_fdc_set_tc_state( device->machine, state );
}

static void set_dma_channel(const device_config *device, int channel, int state)
{
	if (!state) dma_channel = channel;
}

static WRITE_LINE_DEVICE_HANDLER( pc_dack0_w ) { set_dma_channel(device, 0, state); }
static WRITE_LINE_DEVICE_HANDLER( pc_dack1_w ) { set_dma_channel(device, 1, state); }
static WRITE_LINE_DEVICE_HANDLER( pc_dack2_w ) { set_dma_channel(device, 2, state); }
static WRITE_LINE_DEVICE_HANDLER( pc_dack3_w ) { set_dma_channel(device, 3, state); }

I8237_INTERFACE( at_dma8237_1_config )
{
	DEVCB_LINE(pc_dma_hrq_changed),
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
	at_state *st = device->machine->driver_data;
	pic8259_set_irq_line(st->pic8259_master, 4, state);
}

static INS8250_INTERRUPT( at_com_interrupt_2 )
{
	at_state *st = device->machine->driver_data;
	pic8259_set_irq_line(st->pic8259_master, 3, state);
}

/* called when com registers read/written - used to update peripherals that
are connected */
static void at_com_refresh_connected_common(const device_config *device, int n, int data)
{
	/* mouse connected to this port? */
	if (input_port_read(device->machine, "DSW2") & (0x80>>n))
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

static void at_fdc_interrupt(running_machine *machine, int state)
{
	at_state *st = machine->driver_data;
	pic8259_set_irq_line(st->pic8259_master, 6, state);
//if ( messram_get_ptr(devtag_get_device(machine, "messram"))[0x0490] == 0x74 )
//  messram_get_ptr(devtag_get_device(machine, "messram"))[0x0490] = 0x54;
}


static void at_fdc_dma_drq(running_machine *machine, int state, int read_)
{
	at_state *st = machine->driver_data;
	i8237_dreq2_w( st->dma8237_1, state);
}

static const device_config *at_get_device(running_machine *machine)
{
	return devtag_get_device(machine, "upd765");
}

static const struct pc_fdc_interface fdc_interface =
{
	at_fdc_interrupt,
	at_fdc_dma_drq,
	NULL,
	at_get_device
};


static int at_get_out2(running_machine *machine) {
	at_state *st = machine->driver_data;
	return pit8253_get_output(st->pit8254, 2 );
}


/**********************************************************
 *
 * 8042 keyboard controller interface
 *
 * Things connected to the 8042 I/O ports:
 *
 *  AT compatible
 *  =============
 *
 *  Port 1 (Input port)
 *  0 - P10 - Undefined
 *  1 - P11 - Undefined
 *  2 - P12 - Undefined
 *  3 - P13 - Undefined
 *  4 - P14 - External RAM ( 1 = Enable external RAM, 0 = Disable external RAM )
 *  5 - P15 - Manufacturing setting ( 1 = Setting enabled, 0 = Setting disabled )
 *  6 - P16 - Display type switch ( 1 = Color display, 0 = Monochrome display )
 *  7 - P17 - Keyboard inhibit switch ( 1 = Keyboard enabled, 0 = Keyboard inhibited )
 *
 *  Port 2 (Output port)
 *  0 - P20 - System Reset ( 1 = Normal, 0 = Reset comupter )
 *  1 - P21 - Gate A20
 *  2 - P22 - Undefined
 *  3 - P23 - Undefined
 *  4 - P24 - Input Buffer Full
 *  5 - P25 - Output Buffer Empty
 *  6 - P26 - Keyboard Clock ( 1 = Pull Clock low, 0 = High-Z )
 *  7 - P27 - Keyboard Data ( 1 = Pull Data low, 0 = High-Z )
 *
 *  Test Pins (input)
 *  0 - T0 - Keyboard Clock input
 *  1 - T1 - Keyboard Data input
 *
 **********************************************************/

static struct {
	UINT8					speaker;
	UINT8					offset1;
	UINT8					clock_signal;
	UINT8					data_signal;
	write8_space_func		clock_callback;
	write8_space_func		data_callback;
} at_kbdc8042;


static READ8_HANDLER( at_kbdc8042_p1_r )
{
	logerror("%04x: reading P1\n", cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")) );
	return 0xFF;
}


static READ8_HANDLER( at_kbdc8042_p2_r )
{
	return 0xFF;
}


static WRITE8_HANDLER( at_kbdc8042_p2_w )
{
	at_state *st = space->machine->driver_data;

	logerror("%04x: writing $%02x to P2\n", cpu_get_pc(cputag_get_cpu(space->machine, "maincpu")), data );

	at_set_gate_a20( space->machine, ( data & 0x02 ) ? 1 : 0 );

	cputag_set_input_line(space->machine, "maincpu", INPUT_LINE_RESET, ( data & 0x01 ) ? CLEAR_LINE : ASSERT_LINE );

	/* OPT BUF FULL is connected to IR1 on the master 8259 */
	if ( st->pic8259_master )
		pic8259_set_irq_line(st->pic8259_master, 1, ( data & 0x10 ) ? ASSERT_LINE : CLEAR_LINE );

	at_kbdc8042.clock_signal = ( data & 0x40 ) ? 0 : 1;
	at_kbdc8042.data_signal = ( data & 0x80 ) ? 1 : 0;

	at_kbdc8042.data_callback( space, 0, at_kbdc8042.data_signal );
	at_kbdc8042.clock_callback( space, 0, at_kbdc8042.clock_signal );
}


static READ8_HANDLER( at_kbdc8042_t0_r )
{
	return at_kbdc8042.clock_signal;
}


static READ8_HANDLER( at_kbdc8042_t1_r )
{
	return at_kbdc8042.data_signal;
}


static WRITE8_HANDLER( at_kbdc8042_set_clock_signal )
{
	at_kbdc8042.clock_signal = data;
}


static WRITE8_HANDLER( at_kbdc8042_set_data_signal )
{
	at_kbdc8042.data_signal = data;
}


static void at_kbdc8042_set_keyboard_interface( running_machine *machine, write8_space_func clock_cb, write8_space_func data_cb )
{
	at_kbdc8042.offset1 = 0xFF;
	at_kbdc8042.clock_callback = clock_cb;
	at_kbdc8042.data_callback = data_cb;
}


static ADDRESS_MAP_START( kbdc8042_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE( MCS48_PORT_T0, MCS48_PORT_T0 )	AM_READ( at_kbdc8042_t0_r )
	AM_RANGE( MCS48_PORT_T1, MCS48_PORT_T1 )	AM_READ( at_kbdc8042_t1_r )
	AM_RANGE( MCS48_PORT_P1, MCS48_PORT_P1 )	AM_READ( at_kbdc8042_p1_r )
	AM_RANGE( MCS48_PORT_P2, MCS48_PORT_P2 )	AM_READWRITE( at_kbdc8042_p2_r, at_kbdc8042_p2_w )
ADDRESS_MAP_END


MACHINE_DRIVER_START( at_kbdc8042 )
	MDRV_CPU_ADD("kbdc8042", I8042, XTAL_12MHz )
	MDRV_CPU_IO_MAP( kbdc8042_io)
MACHINE_DRIVER_END


READ8_HANDLER(at_kbdc8042_r)
{
	static int poll_delay = 4;
    UINT8 data = 0;
	at_state *st = space->machine->driver_data;

	switch ( offset )
	{
	case 0:		/* A2 is wired to 8042 A0 */
		data = upi41_master_r( cputag_get_cpu(space->machine, "kbdc8042"), 0 );
		break;

	case 1:
		data = at_kbdc8042.speaker;
		data &= ~0xc0; /* AT BIOS don't likes this being set */

		/* This needs fixing/updating not sure what this is meant to fix */
		if ( --poll_delay < 0 )
		{
			poll_delay = 3;
			at_kbdc8042.offset1 ^= 0x10;
		}
		data = (data & ~0x10) | ( at_kbdc8042.offset1 & 0x10 );

		if ( pit8253_get_output(st->pit8254, 2 ) )
			data |= 0x20;
		else
			data &= ~0x20; /* ps2m30 wants this */
		break;

	case 2:
		if (at_get_out2(space->machine))
			data |= 0x20;
		else
			data &= ~0x20;
		break;

	case 4:		/* A2 is wired to 8042 A0 */
		data = upi41_master_r( cputag_get_cpu(space->machine, "kbdc8042"), 1 );
		break;
	}

	if (LOG_KBDC)
		logerror("kbdc8042_8_r(): offset=%d data=0x%02x\n", offset, (unsigned) data);
	return data;
}


WRITE8_HANDLER(at_kbdc8042_w)
{
	at_state *st = space->machine->driver_data;
	if (LOG_KBDC)
		logerror("kbdc8042_8_w(): ofset=%d data=0x%02x\n", offset, data);

	switch (offset) {
	case 0:		/* A2 is wired to 8042 A0 */
		upi41_master_w( cputag_get_cpu(space->machine, "kbdc8042"), 0, data );
		break;

	case 1:
		at_kbdc8042.speaker = data;
		pit8253_gate_w( st->pit8254, 2, data & 1);
		at_speaker_set_spkrdata( space->machine, data & 0x02 );
		break;

	case 4:		/* A2 is wired to 8042 A0 */
//printf("8042 command %02x\n", data );
		upi41_master_w( cputag_get_cpu(space->machine, "kbdc8042"), 1, data );
		break;
    }
}


/**********************************************************
 *
 * Init functions
 *
 **********************************************************/


static READ8_HANDLER( input_port_0_r ) { return input_port_read(space->machine, "IN0"); }

static const struct pc_vga_interface vga_interface =
{
	NULL,
	NULL,

	input_port_0_r,

	ADDRESS_SPACE_IO,
	0x0000
};


DRIVER_INIT( atcga )
{
	static const struct kbdc8042_interface at8042 =
	{
		KBDC8042_STANDARD, at_set_gate_a20, at_keyboard_interrupt, at_get_out2
	};
	init_at_common(machine, &at8042);

	/* Attach keyboard to the keyboard controller */
	at_kbdc8042_set_keyboard_interface( machine, kb_keytronic_set_clock_signal, kb_keytronic_set_data_signal );
	kb_keytronic_set_host_interface( machine, at_kbdc8042_set_clock_signal, at_kbdc8042_set_data_signal );
}


DRIVER_INIT( atega )
{
	static const struct kbdc8042_interface at8042 =
	{
		KBDC8042_STANDARD, at_set_gate_a20, at_keyboard_interrupt, at_get_out2
	};
	UINT8	*dst = memory_region( machine, "maincpu" ) + 0xc0000;
	UINT8	*src = memory_region( machine, "user1" ) + 0x3fff;
	int		i;

	init_at_common(machine, &at8042);

	/* Perform the EGA bios address line swaps */
	for( i = 0; i < 0x4000; i++ )
	{
		*dst++ = *src--;
	}

	/* Attach keyboard to the keyboard controller */
	at_kbdc8042_set_keyboard_interface( machine, kb_keytronic_set_clock_signal, kb_keytronic_set_data_signal );
	kb_keytronic_set_host_interface( machine, at_kbdc8042_set_clock_signal, at_kbdc8042_set_data_signal );
}



DRIVER_INIT( at386 )
{
	static const struct kbdc8042_interface at8042 =
	{
		KBDC8042_STANDARD, at_set_gate_a20, at_keyboard_interrupt, at_get_out2
	};
	init_at_common(machine, &at8042);
	pc_vga_init(machine, &vga_interface, NULL);

	/* Attach keyboard to the keyboard controller */
	at_kbdc8042_set_keyboard_interface( machine, kb_keytronic_set_clock_signal, kb_keytronic_set_data_signal );
	kb_keytronic_set_host_interface( machine, at_kbdc8042_set_clock_signal, at_kbdc8042_set_data_signal );
}



DRIVER_INIT( at586 )
{
	DRIVER_INIT_CALL(at386);
	intel82439tx_init(machine);
}

DRIVER_INIT( at_vga )
{
	static const struct kbdc8042_interface at8042 =
	{
		KBDC8042_STANDARD, at_set_gate_a20, at_keyboard_interrupt, at_get_out2
	};

	init_at_common(machine, &at8042);
	pc_turbo_setup(machine, machine->firstcpu, "DSW2", 0x02, 4.77/12, 1);
	pc_vga_init(machine, &vga_interface, NULL);

	/* Attach keyboard to the keyboard controller */
	at_kbdc8042_set_keyboard_interface( machine, kb_keytronic_set_clock_signal, kb_keytronic_set_data_signal );
	kb_keytronic_set_host_interface( machine, at_kbdc8042_set_clock_signal, at_kbdc8042_set_data_signal );
}



DRIVER_INIT( ps2m30286 )
{
	static const struct kbdc8042_interface at8042 =
	{
		KBDC8042_PS2, at_set_gate_a20, at_keyboard_interrupt, at_get_out2
	};
	init_at_common(machine, &at8042);
	pc_turbo_setup(machine, machine->firstcpu, "DSW2", 0x02, 4.77/12, 1);
	pc_vga_init(machine, &vga_interface, NULL);

	/* Attach keyboard to the keyboard controller */
	at_kbdc8042_set_keyboard_interface( machine, kb_keytronic_set_clock_signal, kb_keytronic_set_data_signal );
	kb_keytronic_set_host_interface( machine, at_kbdc8042_set_clock_signal, at_kbdc8042_set_data_signal );
}



static IRQ_CALLBACK(at_irq_callback)
{
	at_state *st = device->machine->driver_data;
	return pic8259_acknowledge( st->pic8259_master);
}



MACHINE_START( at )
{
	cpu_set_irq_callback(cputag_get_cpu(machine, "maincpu"), at_irq_callback);
	pc_fdc_init( machine, &fdc_interface );
}



MACHINE_RESET( at )
{
	at_state *st = machine->driver_data;
	st->maincpu = devtag_get_device(machine, "maincpu");
	st->pic8259_master = devtag_get_device(machine, "pic8259_master");
	st->pic8259_slave = devtag_get_device(machine, "pic8259_slave");
	st->dma8237_1 = devtag_get_device(machine, "dma8237_1");
	st->dma8237_2 = devtag_get_device(machine, "dma8237_2");
	st->pit8254 = devtag_get_device(machine, "pit8254");
	pc_mouse_set_serial_port( devtag_get_device(machine, "ns16450_0") );
}

