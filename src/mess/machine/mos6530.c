/***************************************************************************

  MIOT 6530 emulation

The timer seems to follow these rules:
- When the timer flag changes from 0 to 1 the timer continues to count
  down at a 1 cycle rate.
- When the timer is being read or written the timer flag is reset.
- When the timer flag is set and the timer contents are 0, the counting
  stops.

From the operation of the KIM1 it expects the irqflag to be set whenever
the unit is reset. This is something that is not clear from the datasheet
and should be verified against real hardware.

***************************************************************************/

#include "emu.h"
#include "mos6530.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

enum
{
	TIMER_IDLE,
	TIMER_COUNTING,
	TIMER_FINISHING
};

#define TIMER_FLAG		0x80



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _mos6530_port mos6530_port;
struct _mos6530_port
{
	devcb_resolved_read8		in_port_func;
	devcb_resolved_write8		out_port_func;

	UINT8				in;
	UINT8				out;
	UINT8				ddr;
};


typedef struct _mos6530_state mos6530_state;
struct _mos6530_state
{
	devcb_resolved_write_line	out_irq_func;

	mos6530_port	port[2];

	UINT8			irqstate;
	UINT8			irqenable;

	UINT8			timershift;
	UINT8			timerstate;
	emu_timer *		timer;

	UINT32			clock;
};



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    get_safe_token - convert a device's token
    into a mos6530_state
-------------------------------------------------*/

INLINE mos6530_state *get_safe_token(running_device *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == MIOT6530);
	return (mos6530_state *)device->token;
}


/*-------------------------------------------------
    update_irqstate - update the IRQ state
    based on interrupt enables
-------------------------------------------------*/

INLINE void update_irqstate(running_device *device)
{
	mos6530_state *miot = get_safe_token(device);
	UINT8 out = miot->port[1].out;

	if ( miot->irqenable )
		out = ( ( miot->irqstate & TIMER_FLAG ) ? 0x00 : 0x80 ) | ( out & 0x7F );

	if (miot->port[1].out_port_func.target != NULL)
		devcb_call_write8(&miot->port[1].out_port_func, 0, out);
	else
		logerror("6530MIOT chip %s: Port B is being written to but has no handler.\n", device->tag());
}


/*-------------------------------------------------
    get_timer - return the current timer value
-------------------------------------------------*/

INLINE UINT8 get_timer(mos6530_state *miot)
{
	/* if idle, return 0 */
	if (miot->timerstate == TIMER_IDLE)
		return 0;

	/* if counting, return the number of ticks remaining */
	else if (miot->timerstate == TIMER_COUNTING)
		return attotime_to_ticks(timer_timeleft(miot->timer), miot->clock) >> miot->timershift;

	/* if finishing, return the number of ticks without the shift */
	else
		return attotime_to_ticks(timer_timeleft(miot->timer), miot->clock);
}


/***************************************************************************
    INTERNAL FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    timer_end_callback - callback to process the
    timer
-------------------------------------------------*/

static TIMER_CALLBACK( timer_end_callback )
{
	running_device *device = (running_device *)ptr;
	mos6530_state *miot = get_safe_token(device);

	assert(miot->timerstate != TIMER_IDLE);

	/* if we finished counting, switch to the finishing state */
	if (miot->timerstate == TIMER_COUNTING)
	{
		miot->timerstate = TIMER_FINISHING;
		timer_adjust_oneshot(miot->timer, ticks_to_attotime(256, miot->clock), 0);

		/* signal timer IRQ as well */
		miot->irqstate |= TIMER_FLAG;
		update_irqstate(device);
	}

	/* if we finished finishing, switch to the idle state */
	else if (miot->timerstate == TIMER_FINISHING)
	{
		miot->timerstate = TIMER_IDLE;
		timer_adjust_oneshot(miot->timer, attotime_never, 0);
	}
}



/***************************************************************************
    I/O ACCESS
***************************************************************************/

/*-------------------------------------------------
    mos6530_w - master I/O write access
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( mos6530_w )
{
	mos6530_state *miot = get_safe_token(device);

	/* if A2 == 1, we are writing to the timer */
	if (offset & 0x04)
	{
		static const UINT8 timershift[4] = { 0, 3, 6, 10 };
		attotime curtime = timer_get_time(device->machine);
		INT64 target;

		/* A0-A1 contain the timer divisor */
		miot->timershift = timershift[offset & 3];

		/* A3 contains the timer IRQ enable */
		if (offset & 8)
			miot->irqenable |= TIMER_FLAG;
		else
			miot->irqenable &= ~TIMER_FLAG;

		/* writes here clear the timer flag */
		if (miot->timerstate != TIMER_FINISHING || get_timer(miot) != 0xff)
			miot->irqstate &= ~TIMER_FLAG;
		update_irqstate(device);

		/* update the timer */
		miot->timerstate = TIMER_COUNTING;
		target = attotime_to_ticks(curtime, miot->clock) + 1 + (data << miot->timershift);
		timer_adjust_oneshot(miot->timer, attotime_sub(ticks_to_attotime(target, miot->clock), curtime), 0);
	}

	/* if A2 == 0, we are writing to the I/O section */
	else
	{
		/* A1 selects the port */
		mos6530_port *port = &miot->port[(offset >> 1) & 1];

		/* if A0 == 1, we are writing to the port's DDR */
		if (offset & 1)
			port->ddr = data;

		/* if A0 == 0, we are writing to the port's output */
		else
		{
			UINT8 olddata = port->out;
			port->out = data;

			if ( ( offset & 2 ) && miot->irqenable )
			{
				olddata = ( ( miot->irqstate & TIMER_FLAG ) ? 0x00 : 0x80 ) | ( olddata & 0x7F );
				data = ( ( miot->irqstate & TIMER_FLAG ) ? 0x00 : 0x80 ) | ( data & 0x7F );
			}

			if (port->out_port_func.target != NULL)
				devcb_call_write8(&port->out_port_func, 0, data);
			else
				logerror("6530MIOT chip %s: Port %c is being written to but has no handler.  PC: %08X - %02X\n", device->tag(), 'A' + (offset & 1), cpu_get_pc(device->machine->firstcpu), data);
		}
	}
}


/*-------------------------------------------------
    mos6530_r - master I/O read access
-------------------------------------------------*/

READ8_DEVICE_HANDLER( mos6530_r )
{
	mos6530_state *miot = get_safe_token(device);
	UINT8 val = 0;

	/* if A2 == 1 and A0 == 1, we are reading interrupt flags */
	if ((offset & 0x05) == 0x05)
	{
		val = miot->irqstate;
	}

	/* if A2 == 1 and A0 == 0, we are reading the timer */
	else if ((offset & 0x05) == 0x04)
	{
		val = get_timer(miot);

		/* A3 contains the timer IRQ enable */
		if (offset & 8)
			miot->irqenable |= TIMER_FLAG;
		else
			miot->irqenable &= ~TIMER_FLAG;

		/* implicitly clears the timer flag */
		if (miot->timerstate != TIMER_FINISHING || val != 0xff)
			miot->irqstate &= ~TIMER_FLAG;
		update_irqstate(device);
	}

	/* if A2 == 0 and A0 == anything, we are reading from ports */
	else
	{
		/* A1 selects the port */
		mos6530_port *port = &miot->port[(offset >> 1) & 1];

		/* if A0 == 1, we are reading the port's DDR */
		if (offset & 1)
			val = port->ddr;

		/* if A0 == 0, we are reading the port as an input */
		else
		{
			UINT8	out = port->out;

			if ( ( offset & 2 ) && miot->irqenable )
				out = ( ( miot->irqstate & TIMER_FLAG ) ? 0x00 : 0x80 ) | ( out & 0x7F );

			/* call the input callback if it exists */
			if (port->in_port_func.target != NULL)
			{
				port->in = devcb_call_read8(&port->in_port_func, 0);
			}
			else
				logerror("6530MIOT chip %s: Port %c is being read but has no handler.  PC: %08X\n", device->tag(), 'A' + (offset & 1), cpu_get_pc(device->machine->firstcpu));

			/* apply the DDR to the result */
			val = (out & port->ddr) | (port->in & ~port->ddr);
		}
	}
	return val;
}


/*-------------------------------------------------
    mos6530_porta_in_set - set port A input
    value
-------------------------------------------------*/

void mos6530_porta_in_set(running_device *device, UINT8 data, UINT8 mask)
{
	mos6530_state *miot = get_safe_token(device);
	miot->port[0].in = (miot->port[0].in & ~mask) | (data & mask);
}


/*-------------------------------------------------
    mos6530_portb_in_set - set port B input
    value
-------------------------------------------------*/

void mos6530_portb_in_set(running_device *device, UINT8 data, UINT8 mask)
{
	mos6530_state *miot = get_safe_token(device);
	miot->port[1].in = (miot->port[1].in & ~mask) | (data & mask);
}


/*-------------------------------------------------
    mos6530_porta_in_get - return port A input
    value
-------------------------------------------------*/

UINT8 mos6530_porta_in_get(running_device *device)
{
	mos6530_state *miot = get_safe_token(device);
	return miot->port[0].in;
}


/*-------------------------------------------------
    mos6530_portb_in_get - return port B input
    value
-------------------------------------------------*/

UINT8 mos6530_portb_in_get(running_device *device)
{
	mos6530_state *miot = get_safe_token(device);
	return miot->port[1].in;
}


/*-------------------------------------------------
    mos6530_porta_in_get - return port A output
    value
-------------------------------------------------*/

UINT8 mos6530_porta_out_get(running_device *device)
{
	mos6530_state *miot = get_safe_token(device);
	return miot->port[0].out;
}


/*-------------------------------------------------
    mos6530_portb_in_get - return port B output
    value
-------------------------------------------------*/

UINT8 mos6530_portb_out_get(running_device *device)
{
	mos6530_state *miot = get_safe_token(device);
	return miot->port[1].out;
}


/***************************************************************************
    DEVICE INTERFACE
***************************************************************************/

static DEVICE_START( mos6530 )
{
	mos6530_state *miot = get_safe_token(device);
	const mos6530_interface *intf = (const mos6530_interface*)device->baseconfig().static_config;

	/* validate arguments */
	assert(device != NULL);
	assert(device->tag() != NULL);

	/* set static values */
	miot->clock = device->clock;

	/* resolve callbacks */
	devcb_resolve_read8(&miot->port[0].in_port_func, &intf->in_pa_func, device);
	devcb_resolve_read8(&miot->port[1].in_port_func, &intf->in_pb_func, device);
	devcb_resolve_write8(&miot->port[0].out_port_func, &intf->out_pa_func, device);
	devcb_resolve_write8(&miot->port[1].out_port_func, &intf->out_pb_func, device);

	/* allocate timers */
	miot->timer = timer_alloc(device->machine, timer_end_callback, (void *)device);

	/* register for save states */
	state_save_register_device_item(device, 0, miot->port[0].in);
	state_save_register_device_item(device, 0, miot->port[0].out);
	state_save_register_device_item(device, 0, miot->port[0].ddr);
	state_save_register_device_item(device, 0, miot->port[1].in);
	state_save_register_device_item(device, 0, miot->port[1].out);
	state_save_register_device_item(device, 0, miot->port[1].ddr);

	state_save_register_device_item(device, 0, miot->irqstate);
	state_save_register_device_item(device, 0, miot->irqenable);

	state_save_register_device_item(device, 0, miot->timershift);
	state_save_register_device_item(device, 0, miot->timerstate);
}


static DEVICE_RESET( mos6530 )
{
	mos6530_state *miot = get_safe_token(device);

	/* reset I/O states */
	miot->port[0].out = 0;
	miot->port[0].ddr = 0;
	miot->port[1].out = 0;
	miot->port[1].ddr = 0;

	/* reset IRQ states */
	miot->irqenable = 0;
	miot->irqstate = TIMER_FLAG;
	update_irqstate(device);

	/* reset timer states */
	miot->timershift = 0;
	miot->timerstate = TIMER_IDLE;
	timer_adjust_oneshot(miot->timer, attotime_never, 0);
}


DEVICE_GET_INFO( mos6530 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(mos6530_state);			break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(mos6530);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(mos6530);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "MOS6530");					break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "MOS6500");					break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");		break;
	}
}
