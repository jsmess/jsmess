/*
	TMS9902 Asynchronous Communication Controller

	Raphael Nabet, 2003
*/

#include <math.h>
#include "driver.h"

#include "tms9902.h"

#define MAX_9902 2

/*
	TMS9902 emulation.

Overview:
	TMS9902 is an asynchronous serial controller for use with the TI990 and
	TMS9900 family.  It provides serial I/O, three extra I/O pins (namely RTS,
	DSR and CTS), and a timer.  It communicates with the CPU through the CRU
	I/O bus, and one interrupt pin.

Pins:
	(...)

*/

typedef struct tms9902_t
{
	/* driver-specific configuration */
	/* tms9902 clock rate (PHI* pin, normally connected to TMS9900 Phi3*) */
	/* Official range is 2MHz-3.3MHz.  Some tms9902s were sold as "MP9214", and
	were tested for speeds up to 4MHz, provided the clk4m control bit is set.
	(warning: 3MHz on a tms9900 is equivalent to 12MHz on a tms9995 or tms99000) */
	double clock_rate;

	void (*int_callback)(int which, int INT);	/* called when interrupt pin state changes */
	void (*rts_callback)(int which, int RTS);	/* called when Request To Send pin state changes */
	void (*brk_callback)(int which, int BRK);	/* called when BReaK state changes */
	void (*xmit_callback)(int which, int data);	/* called when a character is transmitted */


	/* CRU interface */
	int register_select;	/* bits which select which register is being written to */

	/* inputs pins */
	unsigned int CTS : 1;
	unsigned int DSR : 1;

	/* output pin */
	unsigned int RTSON : 1;
	unsigned int RTS : 1;

	/* transmitter registers */
	unsigned int BRKON : 1;
	unsigned int BRK : 1;

	unsigned int XBR : 8;		/* transmit buffer register */
	unsigned int XSR : 8;		/* transmit shift register */

	unsigned int XBRE : 1;
	unsigned int XSRE : 1;

	/* receiver registers */

	unsigned int RBR : 8;		/* receive buffer register */
	unsigned int RBRL : 1;
	unsigned int ROVER : 1;

	/* various status flags */

	unsigned int DSCH : 1;
	unsigned int TIMELP : 1;

	unsigned int TIMERR : 1;

	unsigned int DSCENB : 1;
	unsigned int RIENB : 1;
	unsigned int XBIENB : 1;
	unsigned int TIMENB : 1;

	unsigned int INT : 1;

	unsigned int CLK4M : 1;
	unsigned int RCL : 2;

	unsigned int RDV8 : 1;
	unsigned int RDR : 10;
	unsigned int XDV8 : 1;
	unsigned int XDR : 10;

	unsigned int TMR : 8;		/* interval timer */

	/* clock registers */
	void *timer;			/* MESS timer, used to emulate the decrementer register */
} tms9902_t;

/* bits in register_select */
enum
{
	register_select_LXDR = 0x1,
	register_select_LRDR = 0x2,
	register_select_LDIR = 0x4,
	register_select_LDCTRL = 0x8
};

static tms9902_t tms9902[MAX_9902];


/*
	prototypes
*/

static void reset(int which);
static void field_interrupts(int which);
static void decrementer_callback(int which);
static void set_rts(int which, int state);
static void set_brk(int which, int state);
static void initiate_transmit(int which);


/*
	utilities
*/


/*
	initialize the tms9902 core
*/
void tms9902_init(int which, const tms9902reset_param *param)
{
	tms9902[which].clock_rate = param->clock_rate;

	tms9902[which].int_callback = param->int_callback;
	tms9902[which].rts_callback = param->rts_callback;
	tms9902[which].brk_callback = param->brk_callback;
	tms9902[which].xmit_callback = param->xmit_callback;

	tms9902[which].timer = timer_alloc(decrementer_callback);

	reset(which);
}


void tms9902_cleanup(int which)
{
	if (tms9902[which].timer)
	{
		timer_reset(tms9902[which].timer, TIME_NEVER);	/* FIXME - timers should only be allocated once */
		tms9902[which].timer = NULL;
	}
}

static void reset(int which)
{
	/*  disable all interrupts */
	tms9902[which].DSCENB = 0;
	tms9902[which].TIMENB = 0;
	tms9902[which].XBIENB = 0;
	tms9902[which].RIENB = 0;
	if (tms9902[which].int_callback)
		(*tms9902[which].int_callback)(which, 0);
	/* initialize transmitter */
	tms9902[which].XBRE = 1;
	tms9902[which].XSRE = 1;
	/* initialize receiver */
	tms9902[which].RBRL = 0;
	/* we probably need to clear receive error conditions, too... */
	/* clear RTS */
	tms9902[which].RTSON = 0;
	tms9902[which].RTS = 0;
	if (tms9902[which].rts_callback)
		(*tms9902[which].rts_callback)(which, 0);
	/* set all resister load flags to 1 */
	tms9902[which].register_select = 0xf;
	/* clear break condition */
	tms9902[which].BRKON = 0;
	tms9902[which].BRK = 0;
	if (tms9902[which].brk_callback)
		(*tms9902[which].brk_callback)(which, 0);
	field_interrupts(which);
}

/*
	should be called after any change to int_state or enabled_ints.
*/
static void field_interrupts(int which)
{
	int new_int = (tms9902[which].DSCH && tms9902[which].DSCENB)
							|| (tms9902[which].RBRL && tms9902[which].RIENB)
							|| (tms9902[which].XBRE && tms9902[which].XBIENB)
							|| (tms9902[which].TIMELP && tms9902[which].TIMENB);

	if (new_int != tms9902[which].INT)
	{
		tms9902[which].INT = new_int;

		if (tms9902[which].int_callback)
			(*tms9902[which].int_callback)(which, new_int);
	}
}


/*
	function which should be called by the driver when the state of CTS changes

	state == 0: CTS* is inactive (high)
	state != 0: CTS* is active (low)
*/
void tms9902_set_cts(int which, int state)
{
	state = state != 0;

	if (state != tms9902[which].CTS)
	{
		tms9902[which].CTS = state;
		tms9902[which].DSCH = 1;
		field_interrupts(which);

		if (state && tms9902[which].RTS && tms9902[which].XSRE)
		{
			if ((! tms9902[which].XBRE) && ! tms9902[which].BRK)
				initiate_transmit(which);
			else if (tms9902[which].BRKON)
				set_brk(which, 1);
		}
	}
}


/*
	function which should be called by the driver when the state of DSR changes

	state == 0: DSR* is inactive (high)
	state != 0: DSR* is active (low)
*/
void tms9902_set_dsr(int which, int state)
{
	state = state != 0;

	if (state != tms9902[which].DSR)
	{
		tms9902[which].DSR = state;
		tms9902[which].DSCH = 1;
		field_interrupts(which);
	}
}


void tms9902_push_data(int which, int data)
{
	tms9902[which].RBR = data;

	if (! tms9902[which].RBRL)
	{
		tms9902[which].RBRL = 1;
		field_interrupts(which);
	}
	else
		tms9902[which].ROVER = 1;
}


/*
	This call-back is called by the MESS timer system when the decrementer
	reaches 0.
*/
static void decrementer_callback(int which)
{
	if (tms9902[which].TIMELP)
		tms9902[which].TIMERR = 1;
	else
		tms9902[which].TIMELP = 1;
}

/*
	load the content of clockinvl into the decrementer
*/
static void reload_interval_timer(int which)
{
	if (tms9902[which].TMR)
	{	/* reset clock interval */
		timer_adjust(tms9902[which].timer,
						(double) tms9902[which].TMR / (tms9902[which].clock_rate / ((tms9902[which].CLK4M) ? 4. : 3.) / 64.),
						which,
						(double) tms9902[which].TMR / (tms9902[which].clock_rate / ((tms9902[which].CLK4M) ? 4. : 3.) / 64.));
	}
	else
	{	/* clock interval == 0 -> no timer */
		timer_enable(tms9902[which].timer, 0);
	}
}


static void set_rts(int which, int state)
{
	/*state = state != 0;*/

	if (state != tms9902[which].RTS)
	{
		tms9902[which].RTS = state;

		if (tms9902[which].rts_callback)
			(*tms9902[which].rts_callback)(which, state);
	}
}


static void set_brk(int which, int state)
{
	/*state = state != 0;*/

	if (state != tms9902[which].BRK)
	{
		tms9902[which].BRK = state;

		if (tms9902[which].brk_callback)
			(*tms9902[which].brk_callback)(which, state);
	}
}


static void initiate_transmit(int which)
{
	/* Load transmit register */
	tms9902[which].XSR = tms9902[which].XBR;
	tms9902[which].XSRE = 0;
	tms9902[which].XBRE = 1;
	field_interrupts(which);

	/* Do transmit at once (I will add a timer delay some day, maybe) */

	if (tms9902[which].xmit_callback)
		(*tms9902[which].xmit_callback)(which, tms9902[which].XSR & (0xff >> (3-tms9902[which].RCL)));

	tms9902[which].XSRE = 1;

	if ((! tms9902[which].XBRE) /*&& tms9902[which].RTS*/ && tms9902[which].CTS /*&& ! tms9902[which].BRK*/)
		/* load next byte */
		initiate_transmit(which);
	else if (tms9902[which].BRKON /*&& tms9902[which].RTS*/ && tms9902[which].CTS)
		/* enter break mode */
		set_brk(which, 1);
	else if ((! tms9902[which].RTSON) && ((! tms9902[which].CTS) || (tms9902[which].XBRE && ! tms9902[which].BRK)))
		/* clear RTS output */
		set_rts(which, 0);
}



/*----------------------------------------------------------------
	TMS9902 CRU interface.
----------------------------------------------------------------*/

/*
	Read a 8 bit chunk from tms9902.

	signification:
	bit 0-7: RBR0-7 Receive Buffer register
	bit 8: not used (always 0)
	bit 9: RCVERR Receive Error (RFER | ROVER | RPER)
	bit 10: RPER Receive Parity Error
	bit 11: ROVER Receive Overrun Error
	bit 12: RFER Receive Framing Error
	bit 13-15: not emulated, normally used for diagnostics
	bit 16: RBINT (RBRL&RIENB)
*/
int tms9902_cru_r(int which, int offset)
{
	int answer = 0;

	offset &= 0x003;

	switch (offset)
	{
	case 0:
		answer = tms9902[which].RBR;
		break;

	case 1:
		answer = (tms9902[which].ROVER << 3)
					| (tms9902[which].ROVER << 1);
		break;

	case 2:
		answer = (tms9902[which].XSRE << 7)
					| (tms9902[which].XBRE << 6)
					| (tms9902[which].RBRL << 5)
					| ((tms9902[which].DSCH & tms9902[which].DSCENB) << 4)
					| ((tms9902[which].TIMELP & tms9902[which].TIMENB) << 3)
					| ((tms9902[which].XBRE & tms9902[which].XBIENB) << 1)
					| (tms9902[which].RBRL & tms9902[which].RIENB);
		break;

	case 3:
		answer = (tms9902[which].INT << 7)
					| ((tms9902[which].register_select != 0) << 6)
					| (tms9902[which].DSCH << 5)
					| (tms9902[which].CTS << 4)
					| (tms9902[which].DSR << 3)
					| (tms9902[which].RTS << 2)
					| (tms9902[which].TIMELP << 1)
					| (tms9902[which].TIMERR);
		break;
	}

	return answer;
}

/*
	Write 1 bit to tms9902.

	signification:
	...
*/
void tms9902_cru_w(int which, int offset, int data)
{
	data &= 1;	/* clear extra bits */
	offset &= 0x01F;

	switch (offset)
	{
	case 0x00:
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
		/* write to a register */
		{
			int mask = 1 << offset;

			if (tms9902[which].register_select & register_select_LDCTRL)
			{	/* Control Register */
				if (offset <= 7)
				{
					switch (offset)
					{
					case 0:
					case 1:
						if (data)
							tms9902[which].RCL |= mask;
						else
							tms9902[which].RCL &= ~mask;
						break;

					case 3:
						tms9902[which].CLK4M = data;
						break;

					/* ... */
					}

					if (offset == 7)
						tms9902[which].register_select &= ~register_select_LDCTRL;
				}
			}
			else if (tms9902[which].register_select & register_select_LDIR)
			{	/* Interval Register */
				if (offset <= 7)
				{
					if (data)
						tms9902[which].TMR |= mask;
					else
						tms9902[which].TMR &= ~mask;

					if (offset == 7)
					{
						reload_interval_timer(which);
						tms9902[which].register_select &= ~register_select_LDIR;
					}
				}
			}
			else if (tms9902[which].register_select & (register_select_LRDR | register_select_LXDR))
			{	/* Receive&Transmit Data Rate Register */
				if (offset < 10)
				{
					if (tms9902[which].register_select & register_select_LRDR)
					{
						if (data)
							tms9902[which].RDR |= mask;
						else
							tms9902[which].RDR &= ~mask;
					}
					if (tms9902[which].register_select & register_select_LXDR)
					{
						if (data)
							tms9902[which].XDR |= mask;
						else
							tms9902[which].XDR &= ~mask;
					}
				}
				else if (offset == 10)
				{
					if (tms9902[which].register_select & register_select_LRDR)
						tms9902[which].RDV8 = data;
					if (tms9902[which].register_select & register_select_LXDR)
						tms9902[which].XDV8 = data;

					/* set data rate */
					/* ... */

					tms9902[which].register_select &= ~ (register_select_LRDR | register_select_LXDR);
				}
			}
			else
			{	/* Transmit Buffer Register */
				if (offset <= 7)
				{
					if (data)
						tms9902[which].XBR |= mask;
					else
						tms9902[which].XBR &= ~mask;

					if (offset == 7)
					{	/* transmit */
						tms9902[which].XBRE = 0;
						if (tms9902[which].XSRE && tms9902[which].RTS && tms9902[which].CTS && ! tms9902[which].BRK)
						{
							initiate_transmit(which);
						}
					}
				}
			}
		}
		break;

	case 0x0B:	/* LXDR */
	case 0x0C:	/* LRDR */
	case 0x0D:	/* LDIR */
	case 0x0E:	/* LDCTRL */
		{
			int mask = 1 << (offset - 0x0B);

			if ((offset == 0x0D) && (! data) && (tms9902[which].register_select & register_select_LDIR))
				/* clearing LDIR reloads the interval timer (right???) */
				reload_interval_timer(which);

			if (data)
				tms9902[which].register_select |= mask;
			else
				tms9902[which].register_select &= ~mask;
		}
		break;

	case 0x0F:	/* TSTMD */
		/* ... */
		break;

	case 0x10:	/* RTSON */
		tms9902[which].RTSON = data;
		if (data || (tms9902[which].XBRE && tms9902[which].XSRE && ! tms9902[which].BRK))
		{
			set_rts(which, data);
			if (data && tms9902[which].CTS)
			{
				if (tms9902[which].XSRE && (! tms9902[which].XBRE) && ! tms9902[which].BRK)
					initiate_transmit(which);
				else if (tms9902[which].BRKON)
					set_brk(which, 1);
			}
		}
		break;

	case 0x11:	/* BRKON */
		tms9902[which].BRKON = data;
		if (tms9902[which].BRK && ! data)
		{
			tms9902[which].BRK = 0;
			if ((! tms9902[which].XBRE) && tms9902[which].CTS)
				/* transmit next byte */
				initiate_transmit(which);
			else if (! tms9902[which].RTSON)
				/* clear RTS */
				set_rts(which, 0);
		}
		else if (tms9902[which].XBRE && tms9902[which].XSRE && tms9902[which].RTS && tms9902[which].CTS)
		{
			set_brk(which, data);
		}
		break;

	case 0x12:	/* RIENB */
		tms9902[which].RIENB = data;
		tms9902[which].RBRL = 0;
		field_interrupts(which);
		break;

	case 0x13:	/* XBIENB */
		tms9902[which].XBIENB = data;
		field_interrupts(which);
		break;

	case 0x14:	/* TIMENB */
		tms9902[which].TIMENB = data;
		tms9902[which].TIMELP = 0;
		tms9902[which].TIMERR = 0;
		field_interrupts(which);
		break;

	case 0x15:	/* DSCENB */
		tms9902[which].DSCENB = data;
		tms9902[which].DSCH = 0;
		field_interrupts(which);
		break;

	case 0x1F:	/* RESET */
		reset(which);
		break;
	}
}


 READ8_HANDLER ( tms9902_0_cru_r )
{
	return tms9902_cru_r(0, offset);
}

WRITE8_HANDLER ( tms9902_0_cru_w )
{
	tms9902_cru_w(0, offset, data);
}
