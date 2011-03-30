/*
    TMS9902 Asynchronous Communication Controller

    Raphael Nabet, 2003
*/

#include <math.h>
#include "emu.h"

#include "tms9902.h"

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


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _tms9902_t tms9902_t;
struct _tms9902_t
{
	/* driver-specific configuration */
	/* tms9902 clock rate (PHI* pin, normally connected to TMS9900 Phi3*) */
	/* Official range is 2MHz-3.3MHz.  Some tms9902s were sold as "MP9214", and
    were tested for speeds up to 4MHz, provided the clk4m control bit is set.
    (warning: 3MHz on a tms9900 is equivalent to 12MHz on a tms9995 or tms99000) */
	double clock_rate;

	/* CRU interface */
	int register_select;	/* bits device select device register is being written to */

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
	emu_timer *timer;			/* MESS timer, used to emulate the decrementer register */

	/* Pointer to interface */
	const tms9902_interface *intf;
};

/* bits in register_select */
enum
{
	register_select_LXDR = 0x1,
	register_select_LRDR = 0x2,
	register_select_LDIR = 0x4,
	register_select_LDCTRL = 0x8
};

static void initiate_transmit(device_t *device);
static void set_brk(device_t *device, int state);
static DEVICE_RESET( tms9902 );

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE tms9902_t *get_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == TMS9902);
	return (tms9902_t *) downcast<legacy_device_base *>(device)->token();
}


/*
    should be called after any change to int_state or enabled_ints.
*/
static void field_interrupts(device_t *device)
{
	tms9902_t *tms9902 = get_token(device);
	int new_int = (tms9902->DSCH && tms9902->DSCENB)
							|| (tms9902->RBRL && tms9902->RIENB)
							|| (tms9902->XBRE && tms9902->XBIENB)
							|| (tms9902->TIMELP && tms9902->TIMENB);

	if (new_int != tms9902->INT)
	{
		tms9902->INT = new_int;

		if (tms9902->intf->int_callback)
			(*tms9902->intf->int_callback)(device, new_int);
	}
}


/*
    function device should be called by the driver when the state of CTS changes

    state == 0: CTS* is inactive (high)
    state != 0: CTS* is active (low)
*/
void tms9902_set_cts(device_t *device, int state)
{
	tms9902_t *tms9902 = get_token(device);
	state = state != 0;

	if (state != tms9902->CTS)
	{
		tms9902->CTS = state;
		tms9902->DSCH = 1;
		field_interrupts(device);

		if (state && tms9902->RTS && tms9902->XSRE)
		{
			if ((! tms9902->XBRE) && ! tms9902->BRK)
				initiate_transmit(device);
			else if (tms9902->BRKON)
				set_brk(device, 1);
		}
	}
}


/*
    function device should be called by the driver when the state of DSR changes

    state == 0: DSR* is inactive (high)
    state != 0: DSR* is active (low)
*/
void tms9902_set_dsr(device_t *device, int state)
{
	tms9902_t *tms9902 = get_token(device);
	state = state != 0;

	if (state != tms9902->DSR)
	{
		tms9902->DSR = state;
		tms9902->DSCH = 1;
		field_interrupts(device);
	}
}


void tms9902_push_data(device_t *device, int data)
{
	tms9902_t *tms9902 = get_token(device);
	tms9902->RBR = data;

	if (! tms9902->RBRL)
	{
		tms9902->RBRL = 1;
		field_interrupts(device);
	}
	else
		tms9902->ROVER = 1;
}


/*
    This call-back is called by the MESS timer system when the decrementer
    reaches 0.
*/
static TIMER_CALLBACK(decrementer_callback)
{
	device_t *device = (device_t *) ptr;
	tms9902_t *tms9902 = get_token(device);

	if (tms9902->TIMELP)
		tms9902->TIMERR = 1;
	else
		tms9902->TIMELP = 1;
}

/*
    load the content of clockinvl into the decrementer
*/
static void reload_interval_timer(device_t *device)
{
	tms9902_t *tms9902 = get_token(device);
	if (tms9902->TMR)
	{	/* reset clock interval */
		tms9902->timer->adjust(
						attotime::from_double((double) tms9902->TMR / (tms9902->clock_rate / ((tms9902->CLK4M) ? 4. : 3.) / 64.)),
						0,
						attotime::from_double((double) tms9902->TMR / (tms9902->clock_rate / ((tms9902->CLK4M) ? 4. : 3.) / 64.)));
	}
	else
	{	/* clock interval == 0 -> no timer */
		tms9902->timer->enable(0);
	}
}


static void set_rts(device_t *device, int state)
{
	tms9902_t *tms9902 = get_token(device);
	/*state = state != 0;*/

	if (state != tms9902->RTS)
	{
		tms9902->RTS = state;

		if (tms9902->intf->rts_callback)
			(*tms9902->intf->rts_callback)(device, state);
	}
}


static void set_brk(device_t *device, int state)
{
	tms9902_t *tms9902 = get_token(device);
	/*state = state != 0;*/

	if (state != tms9902->BRK)
	{
		tms9902->BRK = state;

		if (tms9902->intf->brk_callback)
			(*tms9902->intf->brk_callback)(device, state);
	}
}


static void initiate_transmit(device_t *device)
{
	tms9902_t *tms9902 = get_token(device);

	/* Load transmit register */
	tms9902->XSR = tms9902->XBR;
	tms9902->XSRE = 0;
	tms9902->XBRE = 1;
	field_interrupts(device);

	/* Do transmit at once (I will add a timer delay some day, maybe) */

	if (tms9902->intf->xmit_callback)
		(*tms9902->intf->xmit_callback)(device, tms9902->XSR & (0xff >> (3-tms9902->RCL)));

	tms9902->XSRE = 1;

	if ((! tms9902->XBRE) /*&& tms9902->RTS*/ && tms9902->CTS /*&& ! tms9902->BRK*/)
		/* load next byte */
		initiate_transmit(device);
	else if (tms9902->BRKON /*&& tms9902->RTS*/ && tms9902->CTS)
		/* enter break mode */
		set_brk(device, 1);
	else if ((! tms9902->RTSON) && ((! tms9902->CTS) || (tms9902->XBRE && ! tms9902->BRK)))
		/* clear RTS output */
		set_rts(device, 0);
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
READ8_DEVICE_HANDLER( tms9902_cru_r )
{
	int answer = 0;
	tms9902_t *tms9902 = get_token(device);

	offset &= 0x003;

	switch (offset)
	{
	case 0:
		answer = tms9902->RBR;
		break;

	case 1:
		answer = (tms9902->ROVER << 3)
					| (tms9902->ROVER << 1);
		break;

	case 2:
		answer = (tms9902->XSRE << 7)
					| (tms9902->XBRE << 6)
					| (tms9902->RBRL << 5)
					| ((tms9902->DSCH & tms9902->DSCENB) << 4)
					| ((tms9902->TIMELP & tms9902->TIMENB) << 3)
					| ((tms9902->XBRE & tms9902->XBIENB) << 1)
					| (tms9902->RBRL & tms9902->RIENB);
		break;

	case 3:
		answer = (tms9902->INT << 7)
					| ((tms9902->register_select != 0) << 6)
					| (tms9902->DSCH << 5)
					| (tms9902->CTS << 4)
					| (tms9902->DSR << 3)
					| (tms9902->RTS << 2)
					| (tms9902->TIMELP << 1)
					| (tms9902->TIMERR);
		break;
	}

	return answer;
}

/*
    Write 1 bit to tms9902.

    signification:
    ...
*/
WRITE8_DEVICE_HANDLER( tms9902_cru_w )
{
	tms9902_t *tms9902 = get_token(device);

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

			if (tms9902->register_select & register_select_LDCTRL)
			{	/* Control Register */
				if (offset <= 7)
				{
					switch (offset)
					{
					case 0:
					case 1:
						if (data)
							tms9902->RCL |= mask;
						else
							tms9902->RCL &= ~mask;
						break;

					case 3:
						tms9902->CLK4M = data;
						break;

					/* ... */
					}

					if (offset == 7)
						tms9902->register_select &= ~register_select_LDCTRL;
				}
			}
			else if (tms9902->register_select & register_select_LDIR)
			{	/* Interval Register */
				if (offset <= 7)
				{
					if (data)
						tms9902->TMR |= mask;
					else
						tms9902->TMR &= ~mask;

					if (offset == 7)
					{
						reload_interval_timer(device);
						tms9902->register_select &= ~register_select_LDIR;
					}
				}
			}
			else if (tms9902->register_select & (register_select_LRDR | register_select_LXDR))
			{	/* Receive&Transmit Data Rate Register */
				if (offset < 10)
				{
					if (tms9902->register_select & register_select_LRDR)
					{
						if (data)
							tms9902->RDR |= mask;
						else
							tms9902->RDR &= ~mask;
					}
					if (tms9902->register_select & register_select_LXDR)
					{
						if (data)
							tms9902->XDR |= mask;
						else
							tms9902->XDR &= ~mask;
					}
				}
				else if (offset == 10)
				{
					if (tms9902->register_select & register_select_LRDR)
						tms9902->RDV8 = data;
					if (tms9902->register_select & register_select_LXDR)
						tms9902->XDV8 = data;

					/* set data rate */
					/* ... */

					tms9902->register_select &= ~ (register_select_LRDR | register_select_LXDR);
				}
			}
			else
			{	/* Transmit Buffer Register */
				if (offset <= 7)
				{
					if (data)
						tms9902->XBR |= mask;
					else
						tms9902->XBR &= ~mask;

					if (offset == 7)
					{	/* transmit */
						tms9902->XBRE = 0;
						if (tms9902->XSRE && tms9902->RTS && tms9902->CTS && ! tms9902->BRK)
						{
							initiate_transmit(device);
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

			if ((offset == 0x0D) && (! data) && (tms9902->register_select & register_select_LDIR))
				/* clearing LDIR reloads the interval timer (right???) */
				reload_interval_timer(device);

			if (data)
				tms9902->register_select |= mask;
			else
				tms9902->register_select &= ~mask;
		}
		break;

	case 0x0F:	/* TSTMD */
		/* ... */
		break;

	case 0x10:	/* RTSON */
		tms9902->RTSON = data;
		if (data || (tms9902->XBRE && tms9902->XSRE && ! tms9902->BRK))
		{
			set_rts(device, data);
			if (data && tms9902->CTS)
			{
				if (tms9902->XSRE && (! tms9902->XBRE) && ! tms9902->BRK)
					initiate_transmit(device);
				else if (tms9902->BRKON)
					set_brk(device, 1);
			}
		}
		break;

	case 0x11:	/* BRKON */
		tms9902->BRKON = data;
		if (tms9902->BRK && ! data)
		{
			tms9902->BRK = 0;
			if ((! tms9902->XBRE) && tms9902->CTS)
				/* transmit next byte */
				initiate_transmit(device);
			else if (! tms9902->RTSON)
				/* clear RTS */
				set_rts(device, 0);
		}
		else if (tms9902->XBRE && tms9902->XSRE && tms9902->RTS && tms9902->CTS)
		{
			set_brk(device, data);
		}
		break;

	case 0x12:	/* RIENB */
		tms9902->RIENB = data;
		tms9902->RBRL = 0;
		field_interrupts(device);
		break;

	case 0x13:	/* XBIENB */
		tms9902->XBIENB = data;
		field_interrupts(device);
		break;

	case 0x14:	/* TIMENB */
		tms9902->TIMENB = data;
		tms9902->TIMELP = 0;
		tms9902->TIMERR = 0;
		field_interrupts(device);
		break;

	case 0x15:	/* DSCENB */
		tms9902->DSCENB = data;
		tms9902->DSCH = 0;
		field_interrupts(device);
		break;

	case 0x1F:	/* RESET */
		DEVICE_RESET_CALL( tms9902 );
		break;
	}
}


/*-------------------------------------------------
    DEVICE_STOP( tms9902 )
-------------------------------------------------*/

static DEVICE_STOP( tms9902 )
{
	tms9902_t *tms9902 = get_token(device);

	if (tms9902->timer)
	{
		tms9902->timer->reset();	/* FIXME - timers should only be allocated once */
		tms9902->timer = NULL;
	}
}

/*-------------------------------------------------
    DEVICE_RESET( tms9902 )
-------------------------------------------------*/

static DEVICE_RESET( tms9902 )
{
	tms9902_t *tms9902 = get_token(device);

	/*  disable all interrupts */
	tms9902->DSCENB = 0;
	tms9902->TIMENB = 0;
	tms9902->XBIENB = 0;
	tms9902->RIENB = 0;
	if (tms9902->intf->int_callback)
		(*tms9902->intf->int_callback)(device, 0);
	/* initialize transmitter */
	tms9902->XBRE = 1;
	tms9902->XSRE = 1;
	/* initialize receiver */
	tms9902->RBRL = 0;
	/* we probably need to clear receive error conditions, too... */
	/* clear RTS */
	tms9902->RTSON = 0;
	tms9902->RTS = 0;
	if (tms9902->intf->rts_callback)
		(*tms9902->intf->rts_callback)(device, 0);
	/* set all resister load flags to 1 */
	tms9902->register_select = 0xf;
	/* clear break condition */
	tms9902->BRKON = 0;
	tms9902->BRK = 0;
	if (tms9902->intf->brk_callback)
		(*tms9902->intf->brk_callback)(device, 0);
	field_interrupts(device);

}


/*-------------------------------------------------
    DEVICE_START( tms9902 )
-------------------------------------------------*/

static DEVICE_START( tms9902 )
{
	tms9902_t *tms9902 = get_token(device);

	assert(device != NULL);
	assert(device->tag() != NULL);
	assert(device->baseconfig().static_config() != NULL);

	tms9902->intf = (const tms9902_interface*)device->baseconfig().static_config();

	tms9902->clock_rate = tms9902->intf->clock_rate;

	tms9902->timer = device->machine().scheduler().timer_alloc(FUNC(decrementer_callback), (void *) device);
}



/*-------------------------------------------------
    DEVICE_GET_INFO( tms9902 )
-------------------------------------------------*/

DEVICE_GET_INFO( tms9902 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(tms9902_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(tms9902);	break;
		case DEVINFO_FCT_STOP:							info->stop  = DEVICE_STOP_NAME (tms9902);	break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(tms9902);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "TMS9902");					break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "TMS9902");					break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						/* Nothing */								break;
	}
}

DEFINE_LEGACY_DEVICE(TMS9902, tms9902);
