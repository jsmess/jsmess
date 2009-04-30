/***********************************************************************

	machine/z80ne.c

	Functions to emulate general aspects of the machine (RAM, ROM,
	interrupts, I/O ports)

***********************************************************************/

/* Core includes */
#include "driver.h"
#include "devintrf.h"
#include "includes/z80ne.h"

/* Components */
#include "machine/ay31015.h"
#include "machine/kr2376.h"
#include "machine/wd17xx.h"
#include "video/m6847.h"

/* Devices */
#include "devices/basicdsk.h"
#include "devices/cassette.h"

static UINT8 lx383_scan_counter;
static UINT8 lx383_key[LX383_KEYS];
static int   lx383_downsampler;
static int   nmi_delay_counter;
static const device_config *z80ne_ay31015;
static UINT8 lx385_ctrl = 0x1f;
static const device_config *lx388_kr2376;



/* timer to read cassette waveforms */
static emu_timer *cassette_timer;

/* wave duration threshold */
typedef enum
{
	TAPE_300BPS  = 300, /*  300 bps */
	TAPE_600BPS  = 600, /*  600 bps */
	TAPE_1200BPS = 1200 /* 1200 bps */
} z80netape_speed;

static struct {
	struct {
		int length;		/* time cassette level is at input.level */
		int level;		/* cassette level */
		int bit;		/* bit being read */
	} input;
	struct {
		int length;		/* time cassette level is at output.level */
		int level;		/* cassette level */
		int bit;		/* bit to to output */
	} output;
	z80netape_speed speed;			/* 300 - 600 - 1200 */
	int wave_filter;
	int wave_length;
	int wave_short;
	int wave_long;
} cass_data;

static const device_config *cassette_device_image(running_machine *machine)
{
	if (lx385_ctrl & 0x08)
		return devtag_get_device(machine, "cassetteb");
	else
		return devtag_get_device(machine, "cassettea");
}

static TIMER_CALLBACK(z80ne_cassette_tc)
{
	UINT8 cass_ws = 0;
	cass_data.input.length++;

	cass_ws = (cassette_input(cassette_device_image(machine)) > +0.02) ? 1 : 0;

	if ((cass_ws ^ cass_data.input.level) & cass_ws)
	{
		cass_data.input.level = cass_ws;
		cass_data.input.bit = ((cass_data.input.length < cass_data.wave_filter) || (cass_data.input.length > 0x20)) ? 1 : 0;
		cass_data.input.length = 0;
		ay31015_set_input_pin( z80ne_ay31015, AY31015_SI, cass_data.input.bit );
	}
	cass_data.input.level = cass_ws;

	/* saving a tape - convert the serial stream from the uart */

	cass_data.output.length--;

	if (!(cass_data.output.length))
	{
		if (cass_data.output.level)
			cass_data.output.level = 0;
		else
		{
			cass_data.output.level=1;
			cass_ws = ay31015_get_output_pin( z80ne_ay31015, AY31015_SO );
			cass_data.wave_length = cass_ws ? cass_data.wave_short : cass_data.wave_long;
		}
		cassette_output(cassette_device_image(machine), cass_data.output.level ? -1.0 : +1.0);
		cass_data.output.length = cass_data.wave_length;
	}
}


DRIVER_INIT( z80ne )
{
	/* first two entries point to rom on reset */
	UINT8 *RAM = memory_region(machine, "z80ne");
	memory_configure_bank(machine, 1, 0, 2, &RAM[0x0000], 0x8000);
}

DRIVER_INIT( z80net )
{
	DRIVER_INIT_CALL(z80ne);
}

DRIVER_INIT( z80netb )
{
}

/* after the first 3 bytes have been read from ROM, switch the ram back in */
static TIMER_CALLBACK(z80ne_reset_address_decoder)
{
	memory_set_bank(machine, 1, 0);
}

static TIMER_CALLBACK( z80ne_kbd_scan )
{
	/*
	 * NE555 is connected to a 74LS93 binary counter
	 * 74LS93 output:
	 *   QA-QC: column index for LEDs and keyboard
	 *   QD:    keyboard row select
	 *
	 * Port F0 input bit assignment:
	 *   0	QA	bits 0..3 of row counter
	 *   1	QB
	 *   2	QC
	 *   3	QD
	 *   4	Control button pressed, active high
	 *   5  Always low
	 *   6  Always low
	 *   7  Selected button pressed, active low
	 *
	 *
	 */

	static UINT16 key_bits;
	static UINT8 ctrl, rst;
	static UINT8 i;

	// 4-bit counter
	--lx383_scan_counter;
	lx383_scan_counter &= 0x0f;

	if ( --lx383_downsampler == 0 )
	{
		lx383_downsampler = LX383_DOWNSAMPLING;
		key_bits = (input_port_read(machine, "ROW1") << 8) | input_port_read(machine, "ROW0");
		rst = input_port_read(machine, "RST");
		ctrl = input_port_read(machine, "CTRL");

	    for ( i = 0; i<LX383_KEYS; i++)
		{
			lx383_key[i] = ( i | (key_bits & 0x01 ? 0x80 : 0x00) | ~ctrl);
			key_bits >>= 1;
		}
	}
}

/*
 * Handle NMI delay for single step instruction
 */
//static offs_t nmi_delay_count(ATTR_UNUSED const address_space *space, ATTR_UNUSED offs_t address, direct_read_data *direct)
static DIRECT_UPDATE_HANDLER( nmi_delay_count )
{
	nmi_delay_counter--;

	if (!nmi_delay_counter)
	{
		memory_set_direct_update_handler( space, NULL );
		cputag_set_input_line(space->machine, "z80ne", INPUT_LINE_NMI, PULSE_LINE);
	}
	return address;
}

MACHINE_RESET(z80ne_base)
{
	int i;
	const address_space *space = cputag_get_address_space(machine, "z80ne", ADDRESS_SPACE_PROGRAM);

	logerror("In MACHINE_RESET z80ne_base\n");

	for ( i=0; i<LX383_KEYS; i++)
    	lx383_key[i] = 0xf0 | i;
    lx383_scan_counter = 0x0f;
    lx383_downsampler = LX383_DOWNSAMPLING;
    timer_pulse(machine,  ATTOTIME_IN_HZ(1000), NULL, 0, z80ne_kbd_scan );

	/* Initialize cassette interface */
	switch(input_port_read(machine, "LX.385") & 0x07)
	{
	case 0x01:
		cass_data.speed = TAPE_300BPS;
		cass_data.wave_filter = LX385_TAPE_SAMPLE_FREQ / 1600;
		cass_data.wave_short = LX385_TAPE_SAMPLE_FREQ / (2400 * 2);
		cass_data.wave_long = LX385_TAPE_SAMPLE_FREQ / (1200 * 2);
		break;
	case 0x02:
		cass_data.speed = TAPE_600BPS;
		cass_data.wave_filter = LX385_TAPE_SAMPLE_FREQ / 3200;
		cass_data.wave_short = LX385_TAPE_SAMPLE_FREQ / (4800 * 2);
		cass_data.wave_long = LX385_TAPE_SAMPLE_FREQ / (2400 * 2);
		break;
	case 0x04:
		cass_data.speed = TAPE_1200BPS;
		cass_data.wave_filter = LX385_TAPE_SAMPLE_FREQ / 6400;
		cass_data.wave_short = LX385_TAPE_SAMPLE_FREQ / (9600 * 2);
		cass_data.wave_long = LX385_TAPE_SAMPLE_FREQ / (4800 * 2);
	}
//	cass_data.output.length = 0;
	cass_data.wave_length = cass_data.wave_short;
	cass_data.output.length = cass_data.wave_length;
	cass_data.output.level = 1;
	cass_data.input.length = 0;
	cass_data.input.bit = 1;

	z80ne_ay31015 = devtag_get_device(machine, "ay_3_1015");
	ay31015_set_input_pin( z80ne_ay31015, AY31015_CS, 0 );
	ay31015_set_input_pin( z80ne_ay31015, AY31015_NB1, 1 );
	ay31015_set_input_pin( z80ne_ay31015, AY31015_NB2, 1 );
	ay31015_set_input_pin( z80ne_ay31015, AY31015_TSB, 1 );
	ay31015_set_input_pin( z80ne_ay31015, AY31015_EPS, 1 );
	ay31015_set_input_pin( z80ne_ay31015, AY31015_NP, input_port_read(machine, "LX.385") & 0x80 ? 1 : 0 );
	ay31015_set_input_pin( z80ne_ay31015, AY31015_CS, 1 );
	ay31015_set_receiver_clock( z80ne_ay31015, cass_data.speed * 16.0);
	ay31015_set_transmitter_clock( z80ne_ay31015, cass_data.speed * 16.0);

	memory_set_direct_update_handler( space, NULL );
	nmi_delay_counter = 0;
	lx385_ctrl_w(space, 0, 0);

}

MACHINE_RESET(z80ne)
{
//	UINT8 *RAM = memory_region(machine, "z80ne");
//	memory_configure_bank(machine, 1, 0, 2, &RAM[0x0000], 0x8000);

	/* after the first 3 bytes have been read from ROM, switch the ram back in */
	timer_set(machine, ATTOTIME_IN_USEC(6), NULL, 0, z80ne_reset_address_decoder);
    memory_set_bank(machine, 1, 1);

	logerror("In MACHINE_RESET z80ne\n");
	MACHINE_RESET_CALL( z80ne_base );
}

MACHINE_RESET(z80net)
{
	MACHINE_RESET_CALL( z80ne );
	logerror("In MACHINE_RESET z80net\n");

	lx388_kr2376 = devtag_get_device(machine, "lx388_kr2376");
	kr2376_set_input_pin( lx388_kr2376, KR2376_DSII, 0);
	kr2376_set_input_pin( lx388_kr2376, KR2376_PII, 0);
}

MACHINE_RESET(z80netb)
{
	MACHINE_RESET_CALL( z80ne_base );
	logerror("In MACHINE_RESET z80netb\n");

	lx388_kr2376 = devtag_get_device(machine, "lx388_kr2376");
	kr2376_set_input_pin( lx388_kr2376, KR2376_DSII, 0);
	kr2376_set_input_pin( lx388_kr2376, KR2376_PII, 0);
}

INPUT_CHANGED( z80ne_reset )
{
	UINT8 rst;
	rst = input_port_read(field->port->machine, "RST");

	if ( ! BIT(rst, 0))
	{
		running_machine *machine = field->port->machine;
		//MACHINE_RESET_CALL(z80ne);
		//device_reset(cputag_get_cpu(machine, "z80ne"));
		mame_schedule_soft_reset(machine);
	}
}

INPUT_CHANGED( z80ne_nmi )
{
	UINT8 nmi;
	nmi = input_port_read(field->port->machine, "LX388_BRK");

	if ( ! BIT(nmi, 0))
	{
		cputag_set_input_line(field->port->machine, "z80ne", INPUT_LINE_NMI, PULSE_LINE);
	}
}

MACHINE_START( z80ne )
{
	logerror("In MACHINE_START z80ne\n");
	state_save_register_item( machine, "z80ne", NULL, 0, lx383_scan_counter );
	state_save_register_item( machine, "z80ne", NULL, 0, lx383_downsampler );
	state_save_register_item_array( machine, "z80ne", NULL, 0, lx383_key );
	state_save_register_item( machine, "z80ne", NULL, 0, nmi_delay_counter );
	cassette_timer = timer_alloc(machine, z80ne_cassette_tc, NULL);
}

MACHINE_START( z80net )
{
	MACHINE_START_CALL( z80ne );
	logerror("In MACHINE_START z80net\n");
}

MACHINE_START( z80netb )
{
	MACHINE_START_CALL( z80net );
	logerror("In MACHINE_START z80netb\n");
}

/******************************************************************************
 Drivers
******************************************************************************/

/* LX.383 - LX.384 HEX keyboard and display */
READ8_HANDLER( lx383_r )
{
	/*
	 * Keyboard scanning
	 *
	 * IC14 NE555 astable oscillator
	 * IC13 74LS93 binary counter
	 * IC5  74LS240 tri-state buffer
	 *
	 * l'oscillatore NE555 alimenta il clock del contatore 74LS93
	 * 		D0 - Q(A) --\
	 * 		D1 - Q(B)    |-- column
	 * 		D2 - Q(C) --/
	 * 		D3 - Q(D)        row
	 *      D4 - CTRL
	 *      D5 - 0
	 *      D6 - 0
	 *      D7 - ~KEY Pressed
	 */
    return lx383_key[lx383_scan_counter];
}

//#define ATTR_UNUSED __attribute__((__unused__))
WRITE8_HANDLER( lx383_w )
{
	/*
	 * First 8 locations (F0-F7) are mapped to a dual-port 8-byte RAM
	 * The 1KHz NE-555 astable oscillator circuit drive
	 * a 4-bit 74LS93 binary counter.
	 * The 3 least sigificant bits of the counter are connected
	 * both to the read addres of the dual-port ram and to
	 * a 74LS156 3 to 8 binary decoder driving the cathode
	 * of 8 7-segments LEDS.
	 * The data output of the dual-port ram drive the anodes
	 * of the LEDS through 74LS07 buffers.
	 * LED segments - dual-port RAM bit:
	 *   A   0x01
	 *   B   0x02
	 *   C   0x04
	 *   D   0x08
	 *   E   0x10
	 *   F   0x20
	 *   G   0x40
	 *   P   0x80 (represented by DP in original schematics)
	 *
	 *   A write in the range F0-FF starts a 74LS90 counter
	 *   that trigger the NMI line of the CPU afther 2 instruction
	 *   fetch cycles for single step execution.
	 */

	if ( offset < 8 )
    	output_set_digit_value( offset, data ^ 0xff );
    else
    	/* after writing to port 0xF8 and the first ~M1 cycles strike a NMI for single step execution */
    	nmi_delay_counter = 1;
    	memory_set_direct_update_handler(cputag_get_address_space(space->machine, "z80ne", ADDRESS_SPACE_PROGRAM),nmi_delay_count);
}


/* LX.385 Cassette tape interface */
/*
 * NE555 is connected to a 74LS93 binary counter
 * 74LS93 output:
 *   QA-QC: colum index for LEDs and keyboard
 *   QD:    keyboard row select
 *
 * Port EE: UART Data Read/Write
 * Port EF: Status/Control
 *     read, UART status bits read
 *         0 OR   Overrun
 *         1 FE   Framing Error
 *         2 PE   Parity Error
 *         3 TBMT Transmitter Buffer Empty
 *         4 DAV  Data Available
 *         5 EOC  End Of Character
 *         6 1
 *         7 1
 *     write, UART control bits / Tape Unit select / Modulation control
 *         0 bit1=0, bit0=0   UART Reset pulse
 *         1 bit1=0, bit0=1   UART RDAV (Reset Data Available) pulse
 *         2 Tape modulation enable
 *         3 *TAPEA Enable (active low) (at reset: low)
 *         4 *TAPEB Enable (active low) (at reset: low)
 *  Cassette is connected to the uart data input and output via the cassette
 *  interface hardware.
 *
 *  The cassette interface hardware converts square-wave pulses into bits which the uart receives.
 *
 *  1. the cassette format: "frequency shift" is converted
    into the uart data format "non-return to zero"

    2. on cassette a 1 data bit is stored as 8 2400 Hz pulses
    and a 0 data bit as 4 1200 Hz pulses
    - At 1200 baud, a logic 1 is 1 cycle of 1200 Hz and a logic 0 is 1/2 cycle of 600 Hz.
    - At 300 baud, a logic 1 is 8 cycles of 2400 Hz and a logic 0 is 4 cycles of 1200 Hz.

    Attenuation is applied to the signal and the square wave edges are rounded.

    A manchester encoder is used. A flip-flop synchronises input
    data on the positive-edge of the clock pulse.
 *
 */
READ8_HANDLER(lx385_data_r)
{
	return ay31015_get_received_data( z80ne_ay31015 );
}

READ8_HANDLER(lx385_ctrl_r)
{
	/* set unused bits high */
	UINT8 data = 0xc0;
//	UINT8 olddata = 0xc0;

	ay31015_set_input_pin( z80ne_ay31015, AY31015_SWE, 0 );
	data |= (ay31015_get_output_pin( z80ne_ay31015, AY31015_OR   ) ? 0x01 : 0);
	data |= (ay31015_get_output_pin( z80ne_ay31015, AY31015_FE   ) ? 0x02 : 0);
	data |= (ay31015_get_output_pin( z80ne_ay31015, AY31015_PE   ) ? 0x04 : 0);
	data |= (ay31015_get_output_pin( z80ne_ay31015, AY31015_TBMT ) ? 0x08 : 0);
	data |= (ay31015_get_output_pin( z80ne_ay31015, AY31015_DAV  ) ? 0x10 : 0);
	data |= (ay31015_get_output_pin( z80ne_ay31015, AY31015_EOC  ) ? 0x20 : 0);
	ay31015_set_input_pin( z80ne_ay31015, AY31015_SWE, 1 );

	return data;
}
WRITE8_HANDLER(lx385_data_w)
{
	ay31015_set_transmit_data( z80ne_ay31015, data );
}

#define LX385_CASSETTE_MOTOR_MASK ((1<<3)|(1<<4))

WRITE8_HANDLER(lx385_ctrl_w)
{
	/* Translate data to control signals
	 *     0 bit1=0, bit0=0   UART Reset pulse
	 *     1 bit1=0, bit0=1   UART RDAV (Reset Data Available) pulse
	 *     2 UART Tx Clock Enable (active high)
	 *     3 *TAPEA Enable (active low) (at reset: low)
	 *     4 *TAPEB Enable (active low) (at reset: low)
	 */
	UINT8 uart_reset, uart_rdav, uart_tx_clock;
	UINT8 motor_a, motor_b;
	UINT8 changed_bits = (lx385_ctrl ^ data) & 0x1C;
	lx385_ctrl = data;

	uart_reset = ((data & 0x03) == 0x00);
	uart_rdav  = ((data & 0x03) == 0x01);
	uart_tx_clock = ((data & 0x04) == 0x04);
	motor_a = ((data & 0x08) == 0x00);
	motor_b = ((data & 0x10) == 0x00);

	/* UART Reset and RDAV */
	if(uart_reset)
	{
		ay31015_set_input_pin( z80ne_ay31015, AY31015_XR, 1 );
		ay31015_set_input_pin( z80ne_ay31015, AY31015_XR, 0 );
	}

	if(uart_rdav)
	{
		ay31015_set_input_pin( z80ne_ay31015, AY31015_RDAV, 1 );
		ay31015_set_input_pin( z80ne_ay31015, AY31015_RDAV, 0 );
	}

	if (!changed_bits) return;

	/* UART Tx Clock enable/disable */
	if(changed_bits & 0x04)
		ay31015_set_transmitter_clock( z80ne_ay31015, uart_tx_clock ? cass_data.speed * 16.0 : 0.0);

	/* motors */
	if(changed_bits & 0x18)
	{
		cassette_change_state(devtag_get_device(space->machine, "cassettea"),
			(motor_a) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);

		cassette_change_state(devtag_get_device(space->machine, "cassetteb"),
			(motor_b) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);

		if (motor_a || motor_b)
			timer_adjust_periodic(cassette_timer, attotime_zero, 0, ATTOTIME_IN_HZ(LX385_TAPE_SAMPLE_FREQ));
		else
			timer_adjust_oneshot(cassette_timer, attotime_zero, 0);
	}
}

READ8_HANDLER(lx388_data_r)
{
	UINT8 data;

	data = kr2376_data_r(lx388_kr2376, 0) & 0x7f;
	data |= kr2376_get_output_pin(lx388_kr2376, KR2376_SO) << 7;
	return data;
}

READ8_HANDLER(lx388_read_field_sync)
{
	UINT8 data;

	data = m6847_get_field_sync(space->machine) ? 0x00 : 0x80;
	return data;
}
