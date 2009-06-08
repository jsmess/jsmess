/* Super80.c written by Robbbert, 2005-2009. See the MESS wiki for documentation. */

#include "driver.h"
//#include "cpu/z80/z80.h"
#include "sound/wave.h"
#include "devices/cassette.h"
#include "sound/speaker.h"
#include "machine/ctronics.h"
#include "super80.h"

/* Bits in shared variable:
	d5 cassette LED
	d4 super80v rom or pcg bankswitch (1=pcg ram, 0=char gen rom)
	d2 super80v video or colour bankswitch (1=video ram, 0=colour ram)
	d2 super80 screen off (=2mhz) or on (bursts of 2mhz at 50hz = 1mhz) */

UINT8 super80_shared=0xff;

static const device_config *super80_z80pio;
static const device_config *super80_speaker;
static const device_config *super80_cassette;
static const device_config *super80_printer;

/**************************** PIO ******************************************************************************/

static UINT8 keylatch;

/* This activates when Control + C + 4 pressed */
static void super80_pio_interrupt(const device_config *device, int state)
{
	cputag_set_input_line(device->machine, "maincpu", 0, state );
}

static WRITE8_DEVICE_HANDLER( pio_port_a_w )
{
	keylatch = data;
};

static READ8_DEVICE_HANDLER( pio_port_b_r )
{
	static const char *const keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7" };

	int bit;
	UINT8 data = 0xff;

	for (bit = 0; bit < 8; bit++)
	{
		if (!BIT(keylatch, bit)) data &= input_port_read(device->machine, keynames[bit]);
	}

	return data;
};

const z80pio_interface super80_pio_intf =
{
	DEVCB_LINE(super80_pio_interrupt),		/* callback when change interrupt status */
	DEVCB_NULL,
	DEVCB_HANDLER(pio_port_b_r),
	DEVCB_HANDLER(pio_port_a_w),
	DEVCB_NULL,
	DEVCB_NULL,			/* portA ready active callback (not used in super80) */
	DEVCB_NULL			/* portB ready active callback (not used in super80) */
};


/**************************** CASSETTE ROUTINES *****************************************************************/

static void super80_cassette_motor( running_machine *machine, UINT8 data )
{
	if (data)
		cassette_change_state(super80_cassette, CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);
	else
		cassette_change_state(super80_cassette, CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);

	/* does user want to hear the sound? */
	if (input_port_read(machine, "CONFIG") & 8)
		cassette_change_state(super80_cassette, CASSETTE_SPEAKER_ENABLED, CASSETTE_MASK_SPEAKER);
	else
		cassette_change_state(super80_cassette, CASSETTE_SPEAKER_MUTED, CASSETTE_MASK_SPEAKER);
}

/********************************************* TIMER ************************************************/

static UINT8 cass_data[]={ 0, 0, 0, 0 };

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
	UINT8 cass_ws=0;

	cass_data[1]++;
	cass_ws = (cassette_input(super80_cassette) > +0.03) ? 4 : 0;

	if (cass_ws != cass_data[0])
	{
		if (cass_ws) cass_data[3] ^= 2;						// the MDS flipflop
		cass_data[0] = cass_ws;
		cass_data[2] = ((cass_data[1] < 0x40) ? 1 : 0) | cass_ws | cass_data[3];
		cass_data[1] = 0;
	}

	z80pio_p_w(super80_z80pio,1,pio_port_b_r(super80_z80pio,0));
}

/* after the first 4 bytes have been read from ROM, switch the ram back in */
static TIMER_CALLBACK( super80_reset )
{
	memory_set_bank(machine, 1, 0);
}

static TIMER_CALLBACK( super80_halfspeed )
{
	static UINT8 int_sw=0;
	UINT8 go_fast = 0;
	if (!(super80_shared & 4) || (!(input_port_read(machine, "CONFIG") & 2)))	/* bit 2 of port F0 is low, OR user turned on config switch */
		go_fast++;

	/* code to slow down computer to 1 MHz by halting cpu on every second frame */
	if (!go_fast)
	{
		if (!int_sw)
			cputag_set_input_line(machine, "maincpu", INPUT_LINE_HALT, ASSERT_LINE);	// if going, stop it

		int_sw++;
		if (int_sw > 1)
		{
			cputag_set_input_line(machine, "maincpu", INPUT_LINE_HALT, CLEAR_LINE);		// if stopped, start it
			int_sw = 0;
		}
	}
	else
	{
		if (int_sw < 8)								// @2MHz, reset just once
		{
			cputag_set_input_line(machine, "maincpu", INPUT_LINE_HALT, CLEAR_LINE);
			int_sw = 8;							// ...not every time
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
	All the home-brew roms use this, except for super80e which uses BC-BE (which we don't support yet). */

/**************************** I/O PORTS *****************************************************************/

READ8_HANDLER( super80_dc_r )
{
	UINT8 data=0x7f;

	/* bit 7 = printer busy
	0 = printer is not busy */

	data |= centronics_busy_r(super80_printer) << 7;

	return data;
}

READ8_HANDLER( super80_f2_r )
{
	UINT8 data = input_port_read(space->machine, "DSW") & 0xf0;	// dip switches on pcb
	data |= cass_data[2];			// bit 0 = output of U1, bit 1 = MDS cass state, bit 2 = current wave_state
	data |= 0x08;				// bit 3 - not used
	return data;
}

WRITE8_HANDLER( super80_dc_w )
{
	/* hardware strobe driven from port select, bit 7..0 = data */
	centronics_strobe_w(super80_printer, 1);
	centronics_data_w(super80_printer, 0, data);
	centronics_strobe_w(super80_printer, 0);
}

static UINT8 last_data;

WRITE8_HANDLER( super80_f0_w )
{
	UINT8 bits = data ^ last_data;
	super80_shared = data;
	speaker_level_w(super80_speaker, (data & 8) ? 0 : 1);				/* bit 3 - speaker */
	if (bits & 2) super80_cassette_motor(space->machine, data & 2 ? 1 : 0);	/* bit 1 - cassette motor */
	cassette_output(super80_cassette, (data & 1) ? -1.0 : +1.0);	/* bit 0 - cass out */

	last_data = data;
}

WRITE8_HANDLER( super80r_f0_w )
{
	super80_f0_w(space, 0, data);
	super80_shared |= 0x14;
}

READ8_DEVICE_HANDLER( super80_pio_r )
{
	if (!offset)
		return z80pio_d_r(device, 0);
	else
	if (offset == 1)
		return z80pio_c_r(device, 0);
	else
	if (offset == 2)
		return z80pio_d_r(device, 1);
	else
		return z80pio_c_r(device, 1);
}

WRITE8_DEVICE_HANDLER( super80_pio_w )
{
	if (!offset)
		z80pio_d_w(device, 0, data);
	else
	if (offset == 1)
		z80pio_c_w(device, 0, data);
	else
	if (offset == 2)
		z80pio_d_w(device, 1, data);
	else
		z80pio_c_w(device, 1, data);
}

/**************************** BASIC MACHINE CONSTRUCTION ***********************************************************/

MACHINE_RESET( super80 )
{
	super80_shared=0xff;
	timer_set(machine, ATTOTIME_IN_USEC(10), NULL, 0, super80_reset);
	memory_set_bank(machine, 1, 1);
	super80_z80pio = devtag_get_device(machine, "z80pio");
	super80_speaker = devtag_get_device(machine, "speaker");
	super80_cassette = devtag_get_device(machine, "cassette");
	super80_printer = devtag_get_device(machine, "centronics");
}

static void driver_init_common( running_machine *machine )
{
	UINT8 *RAM = memory_region(machine, "maincpu");
	memory_configure_bank(machine, 1, 0, 2, &RAM[0x0000], 0xc000);
	timer_pulse(machine, ATTOTIME_IN_HZ(200000),NULL,0,super80_timer);	/* timer for keyboard and cassette */
}

DRIVER_INIT( super80 )
{
	timer_pulse(machine, ATTOTIME_IN_HZ(100),NULL,0,super80_halfspeed);	/* timer for 1MHz slowdown */
	driver_init_common(machine);
}

DRIVER_INIT( super80v )
{
	pcgram = memory_region(machine, "maincpu")+0xf000;
	videoram = memory_region(machine, "maincpu")+0x18000;
	colorram = memory_region(machine, "maincpu")+0x1C000;
	driver_init_common(machine);
}

