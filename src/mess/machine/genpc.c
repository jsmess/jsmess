/***************************************************************************

    machine/genpc.c


***************************************************************************/

#include "emu.h"
#include "includes/genpc.h"

#include "machine/i8255a.h"
#include "machine/pic8259.h"
#include "machine/pit8253.h"
#include "machine/8237dma.h"

#include "machine/pit8253.h"
#include "machine/pckeybrd.h"
#include "sound/speaker.h"

#include "machine/kb_keytro.h"
#include "machine/ram.h"

#include "video/pc_vga_mess.h"

#include "memconv.h"

#define VERBOSE_PIO 0	/* PIO (keyboard controller) */

#define PIO_LOG(N,M,A) \
	do { \
		if(VERBOSE_PIO>=N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s",machine->time().as_double(),(char*)M ); \
			logerror A; \
		} \
	} while (0)

/*************************************************************************
 *
 *      PC DMA stuff
 *
 *************************************************************************/

READ8_HANDLER(genpc_page_r)
{
	return 0xFF;
}


WRITE8_HANDLER(genpc_page_w)
{
	genpc_state *st = space->machine->driver_data<genpc_state>();
	switch(offset % 4)
	{
	case 1:
		st->dma_offset[0][2] = data;
		break;
	case 2:
		st->dma_offset[0][3] = data;
		break;
	case 3:
		st->dma_offset[0][0] = st->dma_offset[0][1] = data;
		break;
	}
}


static WRITE_LINE_DEVICE_HANDLER( pc_dma_hrq_changed )
{
	genpc_state *st = device->machine->driver_data<genpc_state>();
	cpu_set_input_line(st->maincpu, INPUT_LINE_HALT, state ? ASSERT_LINE : CLEAR_LINE);

	/* Assert HLDA */
	i8237_hlda_w( device, state );
}


static READ8_HANDLER( pc_dma_read_byte )
{
	genpc_state *st = space->machine->driver_data<genpc_state>();
	offs_t page_offset = (((offs_t) st->dma_offset[0][st->dma_channel]) << 16) & 0x0F0000;
	return space->read_byte( page_offset + offset);
}


static WRITE8_HANDLER( pc_dma_write_byte )
{
	genpc_state *st = space->machine->driver_data<genpc_state>();
	offs_t page_offset = (((offs_t) st->dma_offset[0][st->dma_channel]) << 16) & 0x0F0000;

	space->write_byte( page_offset + offset, data);
}


static READ8_DEVICE_HANDLER( pc_dma8237_1_dack_r )
{
	return 0;
}

static READ8_DEVICE_HANDLER( pc_dma8237_2_dack_r )
{
	return 0;//return pc_fdc_dack_r(device->machine);
}


static READ8_DEVICE_HANDLER( pc_dma8237_3_dack_r )
{
	return 0;//return pc_hdc_dack_r( device->machine );
}


static WRITE8_DEVICE_HANDLER( pc_dma8237_1_dack_w )
{
}

static WRITE8_DEVICE_HANDLER( pc_dma8237_2_dack_w )
{
	//pc_fdc_dack_w( device->machine, data );
}


static WRITE8_DEVICE_HANDLER( pc_dma8237_3_dack_w )
{
	//pc_hdc_dack_w( device->machine, data );
}


static WRITE8_DEVICE_HANDLER( pc_dma8237_0_dack_w )
{
	genpc_state *st = device->machine->driver_data<genpc_state>();
	st->u73_q2 = 0;
	i8237_dreq0_w( st->dma8237, st->u73_q2 );
}


static WRITE_LINE_DEVICE_HANDLER( pc_dma8237_out_eop )
{
	//pc_fdc_set_tc_state( device->machine, state == ASSERT_LINE ? 0 : 1 );
}

static void set_dma_channel(device_t *device, int channel, int state)
{
	genpc_state *st = device->machine->driver_data<genpc_state>();

	if (!state) st->dma_channel = channel;
}

static WRITE_LINE_DEVICE_HANDLER( pc_dack0_w ) { set_dma_channel(device, 0, state); }
static WRITE_LINE_DEVICE_HANDLER( pc_dack1_w ) { set_dma_channel(device, 1, state); }
static WRITE_LINE_DEVICE_HANDLER( pc_dack2_w ) { set_dma_channel(device, 2, state); }
static WRITE_LINE_DEVICE_HANDLER( pc_dack3_w ) { set_dma_channel(device, 3, state); }

I8237_INTERFACE( genpc_dma8237_config )
{
	DEVCB_LINE(pc_dma_hrq_changed),
	DEVCB_LINE(pc_dma8237_out_eop),
	DEVCB_MEMORY_HANDLER("maincpu", PROGRAM, pc_dma_read_byte),
	DEVCB_MEMORY_HANDLER("maincpu", PROGRAM, pc_dma_write_byte),
	{ DEVCB_NULL, 						  DEVCB_HANDLER(pc_dma8237_1_dack_r), DEVCB_HANDLER(pc_dma8237_2_dack_r), DEVCB_HANDLER(pc_dma8237_3_dack_r) },
	{ DEVCB_HANDLER(pc_dma8237_0_dack_w), DEVCB_HANDLER(pc_dma8237_1_dack_w), DEVCB_HANDLER(pc_dma8237_2_dack_w), DEVCB_HANDLER(pc_dma8237_3_dack_w) },
	{ DEVCB_LINE(pc_dack0_w), DEVCB_LINE(pc_dack1_w), DEVCB_LINE(pc_dack2_w), DEVCB_LINE(pc_dack3_w) }
};


/*************************************************************
 *
 * pic8259 configuration
 *
 *************************************************************/

const struct pic8259_interface genpc_pic8259_config =
{
	DEVCB_CPU_INPUT_LINE("maincpu", 0)
};


/*************************************************************************
 *
 *      PC Speaker related
 *
 *************************************************************************/
static UINT8 pc_speaker_get_spk(running_machine *machine)
{
	genpc_state *st = machine->driver_data<genpc_state>();
	return st->pc_spkrdata & st->pc_input;
}


static void pc_speaker_set_spkrdata(running_machine *machine, UINT8 data)
{
	device_t *speaker = machine->device("speaker");
	genpc_state *st = machine->driver_data<genpc_state>();
	st->pc_spkrdata = data ? 1 : 0;
	speaker_level_w( speaker, pc_speaker_get_spk(machine) );
}


static void pc_speaker_set_input(running_machine *machine, UINT8 data)
{
	device_t *speaker = machine->device("speaker");
	genpc_state *st = machine->driver_data<genpc_state>();
	st->pc_input = data ? 1 : 0;
	speaker_level_w( speaker, pc_speaker_get_spk(machine) );
}


/*************************************************************
 *
 * pit8253 configuration
 *
 *************************************************************/

static WRITE_LINE_DEVICE_HANDLER( genpc_pit8253_out1_changed )
{
	genpc_state *st = device->machine->driver_data<genpc_state>();
	/* Trigger DMA channel #0 */
	if ( st->out1 == 0 && state == 1 && st->u73_q2 == 0 )
	{
		st->u73_q2 = 1;
		i8237_dreq0_w( st->dma8237, st->u73_q2 );
	}
	st->out1 = state;
}


static WRITE_LINE_DEVICE_HANDLER( genpc_pit8253_out2_changed )
{
	pc_speaker_set_input( device->machine, state );
}


const struct pit8253_config genpc_pit8253_config =
{
	{
		{
			XTAL_14_31818MHz/12,				/* heartbeat IRQ */
			DEVCB_NULL,
			DEVCB_DEVICE_LINE("pic8259", pic8259_ir0_w)
		}, {
			XTAL_14_31818MHz/12,				/* dram refresh */
			DEVCB_NULL,
			DEVCB_LINE(genpc_pit8253_out1_changed)
		}, {
			XTAL_14_31818MHz/12,				/* pio port c pin 4, and speaker polling enough */
			DEVCB_NULL,
			DEVCB_LINE(genpc_pit8253_out2_changed)
		}
	}
};

/**********************************************************
 *
 * PPI8255 interface
 *
 *
 * PORT A (input)
 *
 * Directly attached to shift register which stores data
 * received from the keyboard.
 *
 * PORT B (output)
 * 0 - PB0 - TIM2GATESPK - Enable/disable counting on timer 2 of the 8253
 * 1 - PB1 - SPKRDATA    - Speaker data
 * 2 - PB2 -             - Enable receiving data from the keyboard when keyboard is not locked.
 * 3 - PB3 -             - Dipsswitch set selector
 * 4 - PB4 - ENBRAMPCK   - Enable ram parity check
 * 5 - PB5 - ENABLEI/OCK - Enable expansion I/O check
 * 6 - PB6 -             - Connected to keyboard clock signal
 *                         0 = ignore keyboard signals
 *                         1 = accept keyboard signals
 * 7 - PB7 -             - Clear/disable shift register and IRQ1 line
 *                         0 = normal operation
 *                         1 = clear and disable shift register and clear IRQ1 flip flop
 *
 * PORT C
 * 0 - PC0 -         - Dipswitch 0/4 SW1
 * 1 - PC1 -         - Dipswitch 1/5 SW1
 * 2 - PC2 -         - Dipswitch 2/6 SW1
 * 3 - PC3 -         - Dipswitch 3/7 SW1
 * 4 - PC4 - SPK     - Speaker/cassette data
 * 5 - PC5 - I/OCHCK - Expansion I/O check result
 * 6 - PC6 - T/C2OUT - Output of 8253 timer 2
 * 7 - PC7 - PCK     - Parity check result
 *
 * IBM5150 SW1:
 * 0   - OFF - One or more floppy drives
 *       ON  - Diskless operation
 * 1   - OFF - 8087 present
 *       ON  - No 8087 present
 * 2+3 - Used to determine on board memory configuration
 *       OFF OFF - 64KB
 *       ON  OFF - 48KB
 *       OFF ON  - 32KB
 *       ON  ON  - 16KB
 * 4+5 - Used to select display
 *       OFF OFF - Monochrome
 *       ON  OFF - CGA, 80 column
 *       OFF ON  - CGA, 40 column
 *       ON  ON  - EGA/VGA display
 * 6+7 - Used to select number of disk drives
 *       OFF OFF - four disk drives
 *       ON  OFF - three disk drives
 *       OFF ON  - two disk drives
 *       ON  ON  - one disk drive
 *
 **********************************************************/

static READ8_DEVICE_HANDLER (genpc_ppi_portb_r )
{
	int data;
	running_machine *machine = device->machine;

	data = 0xff;
	PIO_LOG(1,"PIO_B_r",("$%02x\n", data));
	return data;
}


static WRITE8_DEVICE_HANDLER ( genpc_ppi_porta_w )
{
	running_machine *machine = device->machine;

	/* KB controller port A */
	PIO_LOG(1,"PIO_A_w",("$%02x\n", data));
}



static WRITE8_DEVICE_HANDLER ( genpc_ppi_portc_w )
{
	running_machine *machine = device->machine;

	/* KB controller port C */
	PIO_LOG(1,"PIO_C_w",("$%02x\n", data));
}


WRITE8_HANDLER( genpc_kb_set_clock_signal )
{
	genpc_state *st = space->machine->driver_data<genpc_state>();
	device_t *keyboard = space->machine->device("keyboard");

	if ( st->ppi_clock_signal != data )
	{
		if ( st->ppi_keyb_clock && st->ppi_shift_enable )
		{
			st->ppi_clock_signal = data;
			if ( ! st->ppi_keyboard_clear )
			{
				/* Data is clocked in on a high->low transition */
				if ( ! data )
				{
					UINT8	trigger_irq = st->ppi_shift_register & 0x01;

					st->ppi_shift_register = ( st->ppi_shift_register >> 1 ) | ( st->ppi_data_signal << 7 );
					if ( trigger_irq )
					{
						pic8259_ir1_w(st->pic8259, 1);
						st->ppi_shift_enable = 0;
						st->ppi_clock_signal = 0;
						kb_keytronic_clock_w(keyboard, st->ppi_clock_signal);
					}
				}
			}
		}
	}

	kb_keytronic_clock_w(keyboard, st->ppi_clock_signal);
}


WRITE8_HANDLER( genpc_kb_set_data_signal )
{
	genpc_state *st = space->machine->driver_data<genpc_state>();
	device_t *keyboard = space->machine->device("keyboard");

	st->ppi_data_signal = data;

	kb_keytronic_data_w(keyboard, st->ppi_data_signal);
}

static READ8_DEVICE_HANDLER (genpc_ppi_porta_r)
{
	int data = 0xFF;
	running_machine *machine = device->machine;
	genpc_state *st = device->machine->driver_data<genpc_state>();

	/* KB port A */
	if (st->ppi_keyboard_clear)
	{
		/*   0  0 - no floppy drives
         *   1  Not used
         * 2-3  The number of memory banks on the system board
         * 4-5  Display mode
         *      11 = monochrome
         *      10 - color 80x25
         *      01 - color 40x25
         * 6-7  The number of floppy disk drives
         */
		data = input_port_read(device->machine, "DSW0");
	}
	else
	{
		data = st->ppi_shift_register;
	}
    PIO_LOG(1,"PIO_A_r",("$%02x\n", data));
    return data;
}


static READ8_DEVICE_HANDLER ( genpc_ppi_portc_r )
{
	genpc_state *st = device->machine->driver_data<genpc_state>();
	int timer2_output = pit8253_get_output( st->pit8253, 2 );
	int data=0xff;
	running_machine *machine = device->machine;

	data&=~0x80; // no parity error
	data&=~0x40; // no error on expansion board
	/* KB port C: equipment flags */
//  if (pc_port[0x61] & 0x08)
	if (st->ppi_portc_switch_high)
	{
		/* read hi nibble of S2 */
		data = (data & 0xf0) | ((input_port_read(device->machine, "DSW0") >> 4) & 0x0f);
		PIO_LOG(1,"PIO_C_r (hi)",("$%02x\n", data));
	}
	else
	{
		/* read lo nibble of S2 */
		data = (data & 0xf0) | (input_port_read(device->machine, "DSW0") & 0x0f);
		PIO_LOG(1,"PIO_C_r (lo)",("$%02x\n", data));
	}

	if ( st->ppi_portb & 0x01 )
	{
		data = ( data & ~0x10 ) | ( timer2_output ? 0x10 : 0x00 );
	}
	data = ( data & ~0x20 ) | ( timer2_output ? 0x20 : 0x00 );

	return data;
}


static WRITE8_DEVICE_HANDLER( genpc_ppi_portb_w )
{
	genpc_state *st = device->machine->driver_data<genpc_state>();
	device_t *keyboard = device->machine->device("keyboard");

	/* PPI controller port B*/
	st->ppi_portb = data;
	st->ppi_portc_switch_high = data & 0x08;
	st->ppi_keyboard_clear = data & 0x80;
	st->ppi_keyb_clock = data & 0x40;
	pit8253_gate2_w(st->pit8253, BIT(data, 0));
	pc_speaker_set_spkrdata( device->machine, data & 0x02 );

	st->ppi_clock_signal = ( st->ppi_keyb_clock ) ? 1 : 0;
	kb_keytronic_clock_w(keyboard, st->ppi_clock_signal);

	/* If PB7 is set clear the shift register and reset the IRQ line */
	if ( st->ppi_keyboard_clear )
	{
		pic8259_ir1_w(st->pic8259, 0);
		st->ppi_shift_register = 0;
		st->ppi_shift_enable = 1;
	}
}


I8255A_INTERFACE( genpc_ppi8255_interface )
{
	DEVCB_HANDLER(genpc_ppi_porta_r),
	DEVCB_HANDLER(genpc_ppi_portb_r),
	DEVCB_HANDLER(genpc_ppi_portc_r),
	DEVCB_HANDLER(genpc_ppi_porta_w),
	DEVCB_HANDLER(genpc_ppi_portb_w),
	DEVCB_HANDLER(genpc_ppi_portc_w)
};


/**********************************************************
 *
 * Initialization code
 *
 **********************************************************/

DRIVER_INIT( genpc )
{
	/* MESS managed RAM */
	if ( ram_get_ptr(machine->device(RAM_TAG)) )
		memory_set_bankptr( machine, "bank10", ram_get_ptr(machine->device(RAM_TAG)) );
}



static IRQ_CALLBACK(pc_irq_callback)
{
	genpc_state *st = device->machine->driver_data<genpc_state>();
	return pic8259_acknowledge( st->pic8259 );
}


MACHINE_START( genpc )
{
	genpc_state *st = machine->driver_data<genpc_state>();

	st->pic8259 = machine->device("pic8259");
	st->dma8237 = machine->device("dma8237");
	st->pit8253 = machine->device("pit8253");
	st->isabus = machine->device<isa8_device>("isa");
}


MACHINE_RESET( genpc )
{
	device_t *speaker = machine->device("speaker");
	genpc_state *st = machine->driver_data<genpc_state>();
	st->maincpu = machine->device("maincpu" );
	cpu_set_irq_callback(st->maincpu, pc_irq_callback);

	st->u73_q2 = 0;
	st->out1 = 0;
	st->pc_spkrdata = 0;
	st->pc_input = 0;
	st->dma_channel = 0;
	memset(st->dma_offset,0,sizeof(st->dma_offset));
	st->ppi_portc_switch_high = 0;
	st->ppi_speaker = 0;
	st->ppi_keyboard_clear = 0;
	st->ppi_keyb_clock = 0;
	st->ppi_portb = 0;
	st->ppi_clock_signal = 0;
	st->ppi_data_signal = 0;
	st->ppi_shift_register = 0;
	st->ppi_shift_enable = 0;

	//pc_hdc_set_dma8237_device( st->dma8237 );
	speaker_level_w( speaker, 0 );
}

static READ8_HANDLER( input_port_0_r ) { return 0x08; }

static const struct pc_vga_interface vga_interface =
{
	NULL,
	NULL,

	input_port_0_r,

	ADDRESS_SPACE_IO,
	0x0000
};

DRIVER_INIT( genpcvga )
{
	DRIVER_INIT_CALL(genpc);
	pc_vga_init(machine, &vga_interface, NULL);
}