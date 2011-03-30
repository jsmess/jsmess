/*
    TMS9901 Programmable System Interface

    Raphael Nabet, 2000-2004
*/

#include <math.h>
#include "emu.h"

#include "tms9901.h"

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


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _tms9901_t tms9901_t;
struct _tms9901_t
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
	emu_timer *timer;			/* MESS timer, used to emulate the decrementer register */

	int clockinvl;			/* clock interval, loaded in decrementer when it reaches 0.
                              0 means decrementer off */
	int latchedtimer;		/* when we go into timer mode, the decrementer is copied there to allow to read it reliably */

	int mode9901;			/* TMS9901 current mode
                              0 = so-called interrupt mode (read interrupt
                                state, write interrupt enable mask)
                              1 = clock mode (read/write clock interval) */

	/* tms9901 clock rate (PHI* pin, normally connected to TMS9900 Phi3*) */
	/* decrementer rate equates PHI* rate/64 (e.g. 46875Hz for a 3MHz clock)
    (warning: 3MHz on a tms9900 is equivalent to 12MHz on a tms9995 or tms99000) */
	double clock_rate;

	/* Pointer to interface */
	const tms9901_interface *intf;
};


/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE tms9901_t *get_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == TMS9901);
	return (tms9901_t *) downcast<legacy_device_base *>(device)->token();
}


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
    should be called after any change to int_state or enabled_ints.
*/
static void tms9901_field_interrupts(device_t *device)
{
	int current_ints;
	tms9901_t *tms = get_token(device);

	/* int_state: state of lines int1-int15 */
	current_ints = tms->int_state;
	if (tms->clockinvl != 0)
	{	/* if timer is enabled, INT3 pin is overriden by timer */
		if (tms->timer_int_pending)
			current_ints |= TMS9901_INT3;
		else
			current_ints &= ~ TMS9901_INT3;
	}

	/* enabled_ints: enabled interrupts */
	/* mask out all int pins currently set as output */
	current_ints &= tms->enabled_ints & (~ tms->pio_direction_mirror);

	if (current_ints)
	{
		/* find which interrupt tripped us:
          we simply look for the first bit set to 1 in current_ints... */
		int level = find_first_bit(current_ints);

		tms->int_pending = TRUE;

		if (tms->intf->interrupt_callback)
			(*tms->intf->interrupt_callback)(device, 1, level);
	}
	else
	{
		tms->int_pending = FALSE;

		if (tms->intf->interrupt_callback)
			(*tms->intf->interrupt_callback)(device, 0, 0xf);
	}
}


/*
    function which should be called by the driver when the state of an INTn*
    pin changes (only required if the pin is set up as an interrupt pin)

    state == 0: INTn* is inactive (high)
    state != 0: INTn* is active (low)

    0<=pin_number<=15
*/
void tms9901_set_single_int(device_t *device, int pin_number, int state)
{
	tms9901_t *tms = get_token(device);
	/* remember new state of INTn* pin state */
	if (state)
		tms->int_state |= 1 << pin_number;
	else
		tms->int_state &= ~ (1 << pin_number);

	/* we do not need to always call this function - time for an optimization */
	tms9901_field_interrupts(device);
}


/*
    This call-back is called by the MESS timer system when the decrementer
    reaches 0.
*/
static TIMER_CALLBACK(decrementer_callback)
{
	device_t *device = (device_t *) ptr;
	tms9901_t *tms = get_token(device);

	tms->timer_int_pending = TRUE;			/* decrementer interrupt requested */

	tms9901_field_interrupts(device);
}

/*
    load the content of clockinvl into the decrementer
*/
static void tms9901_timer_reload(device_t *device)
{
	tms9901_t *tms = get_token(device);
	if (tms->clockinvl)
	{	/* reset clock interval */
		tms->timer->adjust(attotime::from_double((double) tms->clockinvl / (tms->clock_rate / 64.)), 0, attotime::from_double((double) tms->clockinvl / (tms->clock_rate / 64.)));
	}
	else
	{	/* clock interval == 0 -> no timer */
		tms->timer->enable(0);
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
READ8_DEVICE_HANDLER ( tms9901_cru_r )
{
	int answer;
	tms9901_t *tms = get_token(device);

	offset &= 0x003;

	switch (offset)
	{
	case 0:
		if (tms->mode9901)
		{	/* clock mode */
			answer = ((tms->latchedtimer & 0x7F) << 1) | 0x01;
		}
		else
		{	/* interrupt mode */
			answer = ((~ tms->int_state) & tms->supported_int_mask) & 0xFF;

			if (tms->intf->read_handlers[0])
				answer |= (* tms->intf->read_handlers[0])(device, 0);

			answer &= ~ tms->pio_direction_mirror;
			answer |= (tms->pio_output_mirror & tms->pio_direction_mirror) & 0xFF;
		}
		break;
	case 1:
		if (tms->mode9901)
		{	/* clock mode */
			answer = (tms->latchedtimer & 0x3F80) >> 7;
			if (! tms->int_pending)
				answer |= 0x80;
		}
		else
		{	/* interrupt mode */
			answer = ((~ tms->int_state) & tms->supported_int_mask) >> 8;

			if (tms->intf->read_handlers[1])
				answer |= (* tms->intf->read_handlers[1])(device, 1);

			answer &= ~ (tms->pio_direction_mirror >> 8);
			answer |= (tms->pio_output_mirror & tms->pio_direction_mirror) >> 8;
		}
		break;
	case 2:
		/* exit timer mode */
		tms->mode9901 = 0;

		answer = (tms->intf->read_handlers[2]) ? (* tms->intf->read_handlers[2])(device, 2) : 0;

		answer &= ~ tms->pio_direction;
		answer |= (tms->pio_output & tms->pio_direction) & 0xFF;

		break;
	default:
		/* exit timer mode */
		tms->mode9901 = 0;

		answer = (tms->intf->read_handlers[3]) ? (* tms->intf->read_handlers[3])(device, 3) : 0;

		answer &= ~ (tms->pio_direction >> 8);
		answer |= (tms->pio_output & tms->pio_direction) >> 8;

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
WRITE8_DEVICE_HANDLER ( tms9901_cru_w )
{
	tms9901_t *tms = get_token(device);

	data &= 1;	/* clear extra bits */
	offset &= 0x01F;

	switch (offset)
	{
	case 0x00:	/* write to mode bit */
		if (tms->mode9901 != data)
		{
			tms->mode9901 = data;

			if (tms->mode9901)
			{
				/* we are switching to clock mode: latch the current value of
                the decrementer register */
				if (tms->clockinvl)
					tms->latchedtimer = ceil(tms->timer->remaining().as_double() * (tms->clock_rate / 64.));
				else
					tms->latchedtimer = 0;		/* timer inactive... */
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
		if (tms->mode9901)
		{	/* modify clock interval */
			int mask = 1 << ((offset & 0x0F) - 1);	/* corresponding mask */

			if (data)
				tms->clockinvl |= mask;		/* set bit */
			else
				tms->clockinvl &= ~ mask;		/* clear bit */

			/* reset clock timer */
			tms9901_timer_reload(device);
		}
		else
		{	/* modify interrupt enable mask */
			int mask = 1 << (offset & 0x0F);	/* corresponding mask */

			if (data)
				tms->enabled_ints |= mask;		/* set bit */
			else
				tms->enabled_ints &= ~ mask;		/* unset bit */

			if (offset == 3)
				tms->timer_int_pending = FALSE;	/* SBO 3 clears pending timer interrupt (??) */

			tms9901_field_interrupts(device);		/* changed interrupt state */
		}
		break;
	case 0x0F:
		if (tms->mode9901)
		{	/* in clock mode, this is the soft reset bit */
			if (! data)
			{	/* TMS9901 soft reset */
				/* all output pins are input pins again */
				tms->pio_direction = 0;
				tms->pio_direction_mirror = 0;

				tms->enabled_ints = 0;	/* right??? */
			}
		}
		else
		{	/* modify interrupt enable mask */
			if (data)
				tms->enabled_ints |= 0x4000;		/* set bit */
			else
				tms->enabled_ints &= ~0x4000;		/* unset bit */

			tms9901_field_interrupts(device);		/* changed interrupt state */
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
			tms->mode9901 = 0;

			tms->pio_direction |= mask;			/* set up as output pin */

			if (data)
				tms->pio_output |= mask;
			else
				tms->pio_output &= ~ mask;

			if (pin >= 7)
			{	/* pins P7-P15 are mirrored as INT15*-INT7* */
				int pin2 = 22 - pin;
				int mask2 = (1 << pin2);

				tms->pio_direction_mirror |= mask2;	/* set up as output pin */

				if (data)
					tms->pio_output_mirror |= mask2;
				else
					tms->pio_output_mirror &= ~ mask2;
			}

			if (tms->intf->write_handlers[pin] != NULL)
				(* tms->intf->write_handlers[pin])(device, pin, data);
		}

		break;
	}
}

/*-------------------------------------------------
    DEVICE_STOP( tms9901 )
-------------------------------------------------*/

static DEVICE_STOP( tms9901 )
{
	tms9901_t *tms = get_token(device);

	if (tms->timer)
	{
		tms->timer->reset();	/* FIXME - timers should only be allocated once */
		tms->timer = NULL;
	}
}

/*-------------------------------------------------
    DEVICE_RESET( tms9901 )
-------------------------------------------------*/

static DEVICE_RESET( tms9901 )
{
	tms9901_t *tms = get_token(device);

	tms->timer_int_pending = 0;
	tms->enabled_ints = 0;

	tms->pio_direction = 0;
	tms->pio_direction_mirror = 0;
	tms->pio_output = tms->pio_output_mirror = 0;

	tms9901_field_interrupts(device);

	tms->mode9901 = 0;

	tms->clockinvl = 0;
	tms9901_timer_reload(device);

}


/*-------------------------------------------------
    DEVICE_START( tms9901 )
-------------------------------------------------*/

static DEVICE_START( tms9901 )
{
	tms9901_t *tms = get_token(device);

	assert(device != NULL);
	assert(device->tag() != NULL);
	assert(device->baseconfig().static_config() != NULL);

	tms->intf = (const tms9901_interface*)device->baseconfig().static_config();

	tms->timer = device->machine().scheduler().timer_alloc(FUNC(decrementer_callback), (void *) device);

	tms->supported_int_mask = tms->intf->supported_int_mask;

	tms->clock_rate = tms->intf->clock_rate;

	tms->int_state = 0;
}



/*-------------------------------------------------
    DEVICE_GET_INFO( tms9901 )
-------------------------------------------------*/

DEVICE_GET_INFO( tms9901 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(tms9901_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(tms9901);	break;
		case DEVINFO_FCT_STOP:							info->stop  = DEVICE_STOP_NAME (tms9901);	break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(tms9901);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "TMS9901");					break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "TMS9901");					break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						/* Nothing */								break;
	}
}

DEFINE_LEGACY_DEVICE(TMS9901, tms9901);
