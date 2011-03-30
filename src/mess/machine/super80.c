/* Super80.c written by Robbbert, 2005-2009. See driver source for documentation. */

#include "emu.h"
#include "includes/super80.h"


/**************************** PIO ******************************************************************************/


WRITE8_MEMBER( super80_state::pio_port_a_w )
{
	m_keylatch = data;
};

static READ8_DEVICE_HANDLER( pio_port_b_r ) // cannot be modernised yet as super80 hangs at start
{
	super80_state *state = device->machine().driver_data<super80_state>();
	static const char *const keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7" };

	int bit;
	UINT8 data = 0xff;

	for (bit = 0; bit < 8; bit++)
		if (!BIT(state->m_keylatch, bit)) data &= input_port_read(device->machine(), keynames[bit]);

	return data;
};

Z80PIO_INTERFACE( super80_pio_intf )
{
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0),
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(super80_state, pio_port_a_w),
	DEVCB_NULL,			/* portA ready active callback (not used in super80) */
	DEVCB_HANDLER(pio_port_b_r),
	DEVCB_NULL,
	DEVCB_NULL			/* portB ready active callback (not used in super80) */
};


/**************************** CASSETTE ROUTINES *****************************************************************/

static void super80_cassette_motor( running_machine &machine, UINT8 data )
{
	super80_state *state = machine.driver_data<super80_state>();
	if (data)
		cassette_change_state(state->m_cass, CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);
	else
		cassette_change_state(state->m_cass, CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);

	/* does user want to hear the sound? */
	if (input_port_read(machine, "CONFIG") & 8)
		cassette_change_state(state->m_cass, CASSETTE_SPEAKER_ENABLED, CASSETTE_MASK_SPEAKER);
	else
		cassette_change_state(state->m_cass, CASSETTE_SPEAKER_MUTED, CASSETTE_MASK_SPEAKER);
}

/********************************************* TIMER ************************************************/


	/* this timer runs at 200khz and does 2 jobs:
    1. Scan the keyboard and present the results to the pio
    2. Emulate the 2 chips in the cassette input circuit

    Reasons why it is necessary:
    1. The real z80pio is driven by the cpu clock and is capable of independent actions.
    MAME does not support this at all. If the interrupt key sequence is entered, the
    computer can be reset out of a hung state by the operator.
    2. This "emulates" U79 CD4046BCN PLL chip and U1 LM311P op-amp. U79 converts a frequency to a voltage,
    and U1 amplifies that voltage to digital levels. U1 has a trimpot connected, to set the midpoint.

    The MDS homebrew input circuit consists of 2 op-amps followed by a D-flipflop.
    My "read-any-system" cassette circuit was a CA3140 op-amp, the smarts being done in software.

    bit 0 = original system (U79 and U1)
    bit 1 = MDS fast system
    bit 2 = CA3140 */

static TIMER_CALLBACK( super80_timer )
{
	super80_state *state = machine.driver_data<super80_state>();
	UINT8 cass_ws=0;

	state->m_cass_data[1]++;
	cass_ws = (cassette_input(state->m_cass) > +0.03) ? 4 : 0;

	if (cass_ws != state->m_cass_data[0])
	{
		if (cass_ws) state->m_cass_data[3] ^= 2;		// the MDS flipflop
		state->m_cass_data[0] = cass_ws;
		state->m_cass_data[2] = ((state->m_cass_data[1] < 0x40) ? 1 : 0) | cass_ws | state->m_cass_data[3];
		state->m_cass_data[1] = 0;
	}

	z80pio_pb_w(state->m_pio,1,pio_port_b_r(state->m_pio,0));
}

/* after the first 4 bytes have been read from ROM, switch the ram back in */
static TIMER_CALLBACK( super80_reset )
{
	memory_set_bank(machine, "boot", 0);
}

static TIMER_CALLBACK( super80_halfspeed )
{
	UINT8 go_fast = 0;
	super80_state *state = machine.driver_data<super80_state>();
	if (!(state->m_shared & 4) || (!(input_port_read(machine, "CONFIG") & 2)))	/* bit 2 of port F0 is low, OR user turned on config switch */
		go_fast++;

	/* code to slow down computer to 1 MHz by halting cpu on every second frame */
	if (!go_fast)
	{
		if (!state->m_int_sw)
			cputag_set_input_line(machine, "maincpu", INPUT_LINE_HALT, ASSERT_LINE);	// if going, stop it

		state->m_int_sw++;
		if (state->m_int_sw > 1)
		{
			cputag_set_input_line(machine, "maincpu", INPUT_LINE_HALT, CLEAR_LINE);		// if stopped, start it
			state->m_int_sw = 0;
		}
	}
	else
	{
		if (state->m_int_sw < 8)								// @2MHz, reset just once
		{
			cputag_set_input_line(machine, "maincpu", INPUT_LINE_HALT, CLEAR_LINE);
			state->m_int_sw = 8;							// ...not every time
		}
	}
}

/*************************************** PRINTER ********************************************************/

/* The Super80 had an optional I/O card that plugged into the S-100 slot. The card had facility for running
    an 8-bit Centronics printer, and a serial device at 300, 600, or 1200 baud. The I/O address range
    was selectable via 4 dipswitches. The serial parameters (baud rate, parity, stop bits, etc) was
    chosen with more dipswitches. Regretably, no parameters could be set by software. Currently, the
    Centronics printer is emulated; the serial side of things may be done later, as will the dipswitches.

    The most commonly used I/O range is DC-DE (DC = centronics, DD = serial data, DE = serial control
    All the home-brew roms use this, except for super80e which uses BC-BE. */

/**************************** I/O PORTS *****************************************************************/

READ8_MEMBER( super80_state::super80_dc_r )
{
	UINT8 data=0x7f;

	/* bit 7 = printer busy    0 = printer is not busy */

	data |= centronics_busy_r(m_printer) << 7;

	return data;
}

READ8_MEMBER( super80_state::super80_f2_r )
{
	UINT8 data = input_port_read(m_machine, "DSW") & 0xf0;	// dip switches on pcb
	data |= m_cass_data[2];			// bit 0 = output of U1, bit 1 = MDS cass state, bit 2 = current wave_state
	data |= 0x08;				// bit 3 - not used
	return data;
}

WRITE8_MEMBER( super80_state::super80_dc_w )
{
	/* hardware strobe driven from port select, bit 7..0 = data */
	centronics_strobe_w(m_printer, 1);
	centronics_data_w(m_printer, 0, data);
	centronics_strobe_w(m_printer, 0);
}


WRITE8_MEMBER( super80_state::super80_f0_w )
{
	UINT8 bits = data ^ m_last_data;
	m_shared = data;
	speaker_level_w(m_speaker, (data & 8) ? 0 : 1);				/* bit 3 - speaker */
	if (bits & 2) super80_cassette_motor(m_machine, data & 2 ? 1 : 0);	/* bit 1 - cassette motor */
	cassette_output(m_cass, (data & 1) ? -1.0 : +1.0);	/* bit 0 - cass out */

	m_last_data = data;
}

WRITE8_MEMBER( super80_state::super80r_f0_w )
{
	UINT8 bits = data ^ m_last_data;
	m_shared = data | 0x14;
	speaker_level_w(m_speaker, (data & 8) ? 0 : 1);				/* bit 3 - speaker */
	if (bits & 2) super80_cassette_motor(m_machine, data & 2 ? 1 : 0);	/* bit 1 - cassette motor */
	cassette_output(m_cass, (data & 1) ? -1.0 : +1.0);	/* bit 0 - cass out */

	m_last_data = data;
}

/**************************** BASIC MACHINE CONSTRUCTION ***********************************************************/

void super80_state::machine_reset()
{
	m_shared=0xff;
	m_machine.scheduler().timer_set(attotime::from_usec(10), FUNC(super80_reset));
	memory_set_bank(m_machine, "boot", 1);
}

static void driver_init_common( running_machine &machine )
{
	UINT8 *RAM = machine.region("maincpu")->base();
	memory_configure_bank(machine, "boot", 0, 2, &RAM[0x0000], 0xc000);
	machine.scheduler().timer_pulse(attotime::from_hz(200000), FUNC(super80_timer));	/* timer for keyboard and cassette */
}

DRIVER_INIT( super80 )
{
	machine.scheduler().timer_pulse(attotime::from_hz(100), FUNC(super80_halfspeed));	/* timer for 1MHz slowdown */
	driver_init_common(machine);
}

DRIVER_INIT( super80v )
{
	driver_init_common(machine);
}

