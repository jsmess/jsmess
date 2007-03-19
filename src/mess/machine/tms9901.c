/*
	TMS9901 Programmable System Interface

	Raphael Nabet, 2000-2004
*/

#include <math.h>
#include "driver.h"

#include "tms9901.h"

#define MAX_9901 2

/*
	TMS9901 emulation.

Overview:
	TMS9901 is a support chip for TMS9900.  It handles interrupts, provides
	several I/O pins, and a timer (a.k.a. clock: it is merely a register which
	decrements regularly and can generate an interrupt when it reaches 0).

	It communicates with the TMS9900 with the CRU bus, and with the rest of the
	world with a number of parallel I/O pins.

	I/O and timer functions should work with any other 990/99xx/99xxx CPU.
	On the other hand, interrupt handling was primarily designed for tms9900
	and 99000 based systems: other CPUs can support interrupts, but not the 16
	distinct interrupt vectors.

Pins:
	Vcc, Vss: power supply
	Phi*: system clock (connected to TMS9900 Phi3* or TMS9980 CLKOUT*)
	RST1*: reset input
	CRUIN, CRUOUT, CRUCLK, CE*, S0-S4: CRU bus (CPU interface)
	INTREQ*, IC0-IC3: interrupt bus (CPU interface)
	INT*1-INT*6: used as interrupt/input pins.
	P0-P6: used as input/output pins.
	INT*7/P15-INT*15/P7: used as either interrupt/input or input/output pins.
	  Note that a pin cannot be used simultaneously as output and as interrupt.
	  (This is mostly obvious, but it implies that you cannot trigger an
	  interrupt by setting the output state of a pin, which is not SO obvious.)

Interrupt handling:
	After each clock cycle, TMS9901 latches the state of INT1*-INT15* (except
	pins which are set as output pins).  If the clock is enabled, it replaces
	INT3* with an internal timer interrupt flag.  Then it inverts the value and
	performs a bit-wise AND with the interrupt mask.

	If there are some unmasked interrupt bits, INTREQ* is asserted and the code
	of the lowest active interrupt is placed on IC0-IC3.  If these pins are
	duly connected to the tms9900 INTREQ* and IC0-IC3 pins, the result is that
	asserting an INTn* on tms9901 will cause a level-n interrupt request on the
	tms9900, provided that this interrupt pin is not masked in tms9901, and
	that no unmasked higher-priority (i.e. lower-level) interrupt pin is set.

	This interrupt request lasts for as long as the interrupt pin and the
	relevant bit in the interrupt mask are set (level-triggered interrupts).
	(The request may be shadowed by a higher-priority interrupt request, but
	it will resume when the higher-priority request ends.)

	TIMER interrupts are kind of an exception, since they are not associated
	with an external interrupt pin.  I think there is an internal timer
	interrupt flag that is set when the decrementer reaches 0, and is cleared
	by a write to the 9901 int*3 enable bit ("SBO 3" in interrupt mode).

TODO:
	* Emulate the RST1* input.  Note that RST1* active (low) makes INTREQ*
	  inactive (high) with IC0-IC3 = 0.
	* the clock read register is updated every time the timer decrements when
	  the TMS9901 is not in clock mode.  This probably implies that if the
	  clock mode is cleared and re-asserted immediately, the tms9901 may fail
	  to update the clock read register: this is not emulated.
	* The clock mode is entered when a 1 is written to the control bit.  It is
	  exited when a 0 is written to the control bit or the a tms9901 select bit
	  greater than 15 is accessed.  According to the data sheet, "when CE* is
	  inactive (HIGH), the PSI is not disabled from seeing the select lines.
	  As the CPU is accessing memory, A10-A14 could very easily have a value of
	  15 or greater" (this is assuming that S0-S4 are connected to A10-A14,
	  which makes sense with most tms9900 family members).  There is no way
	  this "feature" (I would call it a hardware bug) can be emulated
	  efficiently, as we would need to watch every memory access.
*/


/*
	tms9901 state structure
*/

typedef struct tms9901_t
{
	/* interrupt registers */
	int supported_int_mask;	/* mask:  bit #n is set if pin #n is supported as an interrupt pin,
							  i.e. the driver sends a notification whenever the pin state changes */
							/* setting these bits is not required, but it saves you the trouble of
							  saving the state of interrupt pins and feeding it to the port read
							  handlers again */
	int int_state;			/* state of the int1-int15 lines */
	int timer_int_pending;	/* timer int pending (overrides int3 pin if timer enabled) */
	int enabled_ints;		/* interrupt enable mask */
	int int_pending;		/* status of the int* pin (connected to TMS9900) */

	/* PIO registers */
	int pio_direction;		/* direction register for PIO */
	int pio_output;			/* current PIO output (to be masked with pio_direction) */
	/* mirrors used for INT7*-INT15* */
	int pio_direction_mirror;
	int pio_output_mirror;

	/* clock registers */
	void *timer;			/* MESS timer, used to emulate the decrementer register */

	int clockinvl;			/* clock interval, loaded in decrementer when it reaches 0.
							  0 means decrementer off */
	int latchedtimer;		/* when we go into timer mode, the decrementer is copied there to allow to read it reliably */

	int mode9901;			/* TMS9901 current mode
							  0 = so-called interrupt mode (read interrupt
							    state, write interrupt enable mask)
							  1 = clock mode (read/write clock interval) */

	/* driver-dependent read and write handlers */
	int (*read_handlers[4])(int offset);
	void (*write_handlers[16])(int offset, int data);

	/* interrupt callback to notify driver of changes in intreq and ic0-3 pins */
	void (*interrupt_callback)(int intreq, int ic);

	/* tms9901 clock rate (PHI* pin, normally connected to TMS9900 Phi3*) */
	/* decrementer rate equates PHI* rate/64 (e.g. 46875Hz for a 3MHz clock)
	(warning: 3MHz on a tms9900 is equivalent to 12MHz on a tms9995 or tms99000) */
	double clock_rate;
} tms9901_t;

static tms9901_t tms9901[MAX_9901];


/*
	prototypes
*/

static void tms9901_field_interrupts(int which);
static void decrementer_callback(int which);
static void tms9901_timer_reload(int which);



/*
	utilities
*/


/*
	return the number of the first (i.e. least significant) non-zero bit among
	the 16 first bits
*/
static int find_first_bit(int value)
{
	int bit = 0;

	if (! value)
		return -1;

#if 0
	if (! (value & 0x00FF))
	{
		value >> 8;
		bit = 8;
	}
	if (! (value & 0x000F))
	{
		value >> 4;
		bit += 4;
	}
	if (! (value & 0x0003))
	{
		value >> 2;
		bit += 2;
	}
	if (! (value & 0x0001))
		bit++;
#else
	while (! (value & 1))
	{
		value >>= 1;	/* try next bit */
		bit++;
	}
#endif

	return bit;
}


/*
	initialize the tms9901 core
*/
void tms9901_init(int which, const tms9901reset_param *param)
{
	int i;

	tms9901[which].timer = timer_alloc(decrementer_callback);

	tms9901[which].supported_int_mask = param->supported_int_mask;

	for (i=0; i<4; i++)
		tms9901[which].read_handlers[i] = param->read_handlers[i];

	for (i=0; i<16; i++)
		tms9901[which].write_handlers[i] = param->write_handlers[i];

	tms9901[which].interrupt_callback = param->interrupt_callback;

	tms9901[which].clock_rate = param->clock_rate;


	tms9901[which].int_state = 0;


	tms9901_reset(which);
}


void tms9901_cleanup(int which)
{
	if (tms9901[which].timer)
	{
		timer_reset(tms9901[which].timer, TIME_NEVER);	/* FIXME - timers should only be allocated once */
		tms9901[which].timer = NULL;
	}
}

void tms9901_reset(int which)
{
	tms9901[which].timer_int_pending = 0;
	tms9901[which].enabled_ints = 0;

	tms9901[which].pio_direction = 0;
	tms9901[which].pio_direction_mirror = 0;
	tms9901[which].pio_output = tms9901[which].pio_output_mirror = 0;

	tms9901_field_interrupts(which);

	tms9901[which].mode9901 = 0;

	tms9901[which].clockinvl = 0;
	tms9901_timer_reload(which);
}

/*
	should be called after any change to int_state or enabled_ints.
*/
static void tms9901_field_interrupts(int which)
{
	int current_ints;

	/* int_state: state of lines int1-int15 */
	current_ints = tms9901[which].int_state;
	if (tms9901[which].clockinvl != 0)
	{	/* if timer is enabled, INT3 pin is overriden by timer */
		if (tms9901[which].timer_int_pending)
			current_ints |= TMS9901_INT3;
		else
			current_ints &= ~ TMS9901_INT3;
	}

	/* enabled_ints: enabled interrupts */
	/* mask out all int pins currently set as output */
	current_ints &= tms9901[which].enabled_ints & (~ tms9901[which].pio_direction_mirror);

	if (current_ints)
	{
		/* find which interrupt tripped us:
		  we simply look for the first bit set to 1 in current_ints... */
		int level = find_first_bit(current_ints);

		tms9901[which].int_pending = TRUE;

		if (tms9901[which].interrupt_callback)
			(*tms9901[which].interrupt_callback)(1, level);
	}
	else
	{
		tms9901[which].int_pending = FALSE;

		if (tms9901[which].interrupt_callback)
			(*tms9901[which].interrupt_callback)(0, 0xf);
	}
}


/*
	function which should be called by the driver when the state of an INTn*
	pin changes (only required if the pin is set up as an interrupt pin)

	state == 0: INTn* is inactive (high)
	state != 0: INTn* is active (low)

	0<=pin_number<=15
*/
void tms9901_set_single_int(int which, int pin_number, int state)
{
	/* remember new state of INTn* pin state */
	if (state)
		tms9901[which].int_state |= 1 << pin_number;
	else
		tms9901[which].int_state &= ~ (1 << pin_number);

	/* we do not need to always call this function - time for an optimization */
	tms9901_field_interrupts(which);
}


/*
	This call-back is called by the MESS timer system when the decrementer
	reaches 0.
*/
static void decrementer_callback(int which)
{
	tms9901[which].timer_int_pending = TRUE;			/* decrementer interrupt requested */

	tms9901_field_interrupts(which);
}

/*
	load the content of clockinvl into the decrementer
*/
static void tms9901_timer_reload(int which)
{
	if (tms9901[which].clockinvl)
	{	/* reset clock interval */
		timer_adjust(tms9901[which].timer, (double) tms9901[which].clockinvl / (tms9901[which].clock_rate / 64.), which, (double) tms9901[which].clockinvl / (tms9901[which].clock_rate / 64.));
	}
	else
	{	/* clock interval == 0 -> no timer */
		timer_enable(tms9901[which].timer, 0);
	}
}



/*----------------------------------------------------------------
	TMS9901 CRU interface.
----------------------------------------------------------------*/

/*
	Read a 8 bit chunk from tms9901.

	signification:
	bit 0: mode9901
	if (mode9901 == 0)
	 bit 1-15: current status of the INT1*-INT15* pins
	else
	 bit 1-14: current timer value
	 bit 15: value of the INTREQ* (interrupt request to TMS9900) pin.

	bit 16-31: current status of the P0-P15 pins (quits timer mode, too...)
*/
int tms9901_cru_r(int which, int offset)
{
	int answer;

	offset &= 0x003;

	switch (offset)
	{
	case 0:
		if (tms9901[which].mode9901)
		{	/* clock mode */
			answer = ((tms9901[which].latchedtimer & 0x7F) << 1) | 0x01;
		}
		else
		{	/* interrupt mode */
			answer = ((~ tms9901[which].int_state) & tms9901[which].supported_int_mask) & 0xFF;

			if (tms9901[which].read_handlers[0])
				answer |= (* tms9901[which].read_handlers[0])(0);

			answer &= ~ tms9901[which].pio_direction_mirror;
			answer |= (tms9901[which].pio_output_mirror & tms9901[which].pio_direction_mirror) & 0xFF;
		}
		break;
	case 1:
		if (tms9901[which].mode9901)
		{	/* clock mode */
			answer = (tms9901[which].latchedtimer & 0x3F80) >> 7;
			if (! tms9901[which].int_pending)
				answer |= 0x80;
		}
		else
		{	/* interrupt mode */
			answer = ((~ tms9901[which].int_state) & tms9901[which].supported_int_mask) >> 8;

			if (tms9901[which].read_handlers[1])
				answer |= (* tms9901[which].read_handlers[1])(1);

			answer &= ~ (tms9901[which].pio_direction_mirror >> 8);
			answer |= (tms9901[which].pio_output_mirror & tms9901[which].pio_direction_mirror) >> 8;
		}
		break;
	case 2:
		/* exit timer mode */
		tms9901[which].mode9901 = 0;

		answer = (tms9901[which].read_handlers[2]) ? (* tms9901[which].read_handlers[2])(2) : 0;

		answer &= ~ tms9901[which].pio_direction;
		answer |= (tms9901[which].pio_output & tms9901[which].pio_direction) & 0xFF;

		break;
	default:
		/* exit timer mode */
		tms9901[which].mode9901 = 0;

		answer = (tms9901[which].read_handlers[3]) ? (* tms9901[which].read_handlers[3])(3) : 0;

		answer &= ~ (tms9901[which].pio_direction >> 8);
		answer |= (tms9901[which].pio_output & tms9901[which].pio_direction) >> 8;

		break;
	}

	return answer;
}

/*
	Write 1 bit to tms9901.

	signification:
	bit 0: write mode9901
	if (mode9901 == 0)
	 bit 1-15: write interrupt mask register
	else
	 bit 1-14: write timer period
	 bit 15: if written value == 0, soft reset (just resets all I/O pins as input)

	bit 16-31: set output state of P0-P15 (and set them as output pin) (quit timer mode, too...)
*/
void tms9901_cru_w(int which, int offset, int data)
{
	data &= 1;	/* clear extra bits */
	offset &= 0x01F;

	switch (offset)
	{
	case 0x00:	/* write to mode bit */
		if (tms9901[which].mode9901 != data)
		{
			tms9901[which].mode9901 = data;

			if (tms9901[which].mode9901)
			{
				/* we are switching to clock mode: latch the current value of
				the decrementer register */
				if (tms9901[which].clockinvl)
					tms9901[which].latchedtimer = ceil(timer_timeleft(tms9901[which].timer) * (tms9901[which].clock_rate / 64.));
				else
					tms9901[which].latchedtimer = 0;		/* timer inactive... */
			}
			else
			{
				/* we are quitting clock mode */
			}
		}
		break;
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
	case 0x08:
	case 0x09:
	case 0x0A:
	case 0x0B:
	case 0x0C:
	case 0x0D:
	case 0x0E:
		/*
			write one bit to 9901 (bits 1-14)

			mode9901==0 ?  Disable/Enable an interrupt
			            :  Bit in clock interval
		*/
		/* offset is the index of the modified bit of register (-> interrupt number -1) */
		if (tms9901[which].mode9901)
		{	/* modify clock interval */
			int mask = 1 << ((offset & 0x0F) - 1);	/* corresponding mask */

			if (data)
				tms9901[which].clockinvl |= mask;		/* set bit */
			else
				tms9901[which].clockinvl &= ~ mask;		/* clear bit */

			/* reset clock timer */
			tms9901_timer_reload(which);
		}
		else
		{	/* modify interrupt enable mask */
			int mask = 1 << (offset & 0x0F);	/* corresponding mask */

			if (data)
				tms9901[which].enabled_ints |= mask;		/* set bit */
			else
				tms9901[which].enabled_ints &= ~ mask;		/* unset bit */

			if (offset == 3)
				tms9901[which].timer_int_pending = FALSE;	/* SBO 3 clears pending timer interrupt (??) */

			tms9901_field_interrupts(which);		/* changed interrupt state */
		}
		break;
	case 0x0F:
		if (tms9901[which].mode9901)
		{	/* in clock mode, this is the soft reset bit */
			if (! data)
			{	/* TMS9901 soft reset */
				/* all output pins are input pins again */
				tms9901[which].pio_direction = 0;
				tms9901[which].pio_direction_mirror = 0;

				tms9901[which].enabled_ints = 0;	/* right??? */
			}
		}
		else
		{	/* modify interrupt enable mask */
			if (data)
				tms9901[which].enabled_ints |= 0x4000;		/* set bit */
			else
				tms9901[which].enabled_ints &= ~0x4000;		/* unset bit */

			tms9901_field_interrupts(which);		/* changed interrupt state */
		}
		break;
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
	case 0x14:
	case 0x15:
	case 0x16:
	case 0x17:
	case 0x18:
	case 0x19:
	case 0x1A:
	case 0x1B:
	case 0x1C:
	case 0x1D:
	case 0x1E:
	case 0x1F:
		{
			int pin = offset & 0x0F;
			int mask = (1 << pin);

			/* exit timer mode */
			tms9901[which].mode9901 = 0;

			tms9901[which].pio_direction |= mask;			/* set up as output pin */

			if (data)
				tms9901[which].pio_output |= mask;
			else
				tms9901[which].pio_output &= ~ mask;

			if (pin >= 7)
			{	/* pins P7-P15 are mirrored as INT15*-INT7* */
				int pin2 = 22 - pin;
				int mask2 = (1 << pin2);

				tms9901[which].pio_direction_mirror |= mask2;	/* set up as output pin */

				if (data)
					tms9901[which].pio_output_mirror |= mask2;
				else
					tms9901[which].pio_output_mirror &= ~ mask2;
			}

			if (tms9901[which].write_handlers[pin] != NULL)
				(* tms9901[which].write_handlers[pin])(pin, data);
		}

		break;
	}
}

 READ8_HANDLER ( tms9901_0_cru_r )
{
	return tms9901_cru_r(0, offset);
}

WRITE8_HANDLER ( tms9901_0_cru_w )
{
	tms9901_cru_w(0, offset, data);
}

 READ8_HANDLER ( tms9901_1_cru_r )
{
	return tms9901_cru_r(1, offset);
}

WRITE8_HANDLER ( tms9901_1_cru_w )
{
	tms9901_cru_w(1, offset, data);
}
