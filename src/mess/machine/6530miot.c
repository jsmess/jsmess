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
#include "6530miot.h"


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

typedef struct _miot6530_port miot6530_port;
struct _miot6530_port
{
	UINT8				in;
	UINT8				out;
	UINT8				ddr;
	miot_read_func		in_func;
	miot_write_func		out_func;
};


typedef struct _miot6530_state miot6530_state;
struct _miot6530_state
{
	const miot6530_interface *intf;

	miot6530_port	port[2];

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
    into a miot6530_state
-------------------------------------------------*/

INLINE miot6530_state *get_safe_token(running_device *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == MIOT6530);
	return (miot6530_state *)device->token;
}


/*-------------------------------------------------
    update_irqstate - update the IRQ state
    based on interrupt enables
-------------------------------------------------*/

INLINE void update_irqstate(running_device *device)
{
	miot6530_state *miot = get_safe_token(device);
	UINT8 out = miot->port[1].out;

	if ( miot->irqenable )
		out = ( ( miot->irqstate & TIMER_FLAG ) ? 0x00 : 0x80 ) | ( out & 0x7F );

	if (miot->port[1].out_func != NULL)
		(*miot->port[1].out_func)(device, out, out);
	else
		logerror("6530MIOT chip %s: Port B is being written to but has no handler.\n", device->tag.cstr());
}


/*-------------------------------------------------
    get_timer - return the current timer value
-------------------------------------------------*/

INLINE UINT8 get_timer(miot6530_state *miot)
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
	miot6530_state *miot = get_safe_token(device);

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
    miot6530_w - master I/O write access
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( miot6530_w )
{
	miot6530_state *miot = get_safe_token(device);

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
		miot6530_port *port = &miot->port[(offset >> 1) & 1];

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

			if (port->out_func != NULL)
				(*port->out_func)(device, data, olddata);
			else
				logerror("6530MIOT chip %s: Port %c is being written to but has no handler.  PC: %08X - %02X\n", device->tag.cstr(), 'A' + (offset & 1), cpu_get_pc(device->machine->firstcpu), data);
		}
	}
}


/*-------------------------------------------------
    miot6530_r - master I/O read access
-------------------------------------------------*/

READ8_DEVICE_HANDLER( miot6530_r )
{
	miot6530_state *miot = get_safe_token(device);
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
		miot6530_port *port = &miot->port[(offset >> 1) & 1];

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
			if (port->in_func != NULL)
			{
				port->in = (*port->in_func)(device, port->in);
			}
			else
				logerror("6530MIOT chip %s: Port %c is being read but has no handler.  PC: %08X\n", device->tag.cstr(), 'A' + (offset & 1), cpu_get_pc(device->machine->firstcpu));

			/* apply the DDR to the result */
			val = (out & port->ddr) | (port->in & ~port->ddr);
		}
	}
	return val;
}


/*-------------------------------------------------
    miot6530_porta_in_set - set port A input
    value
-------------------------------------------------*/

void miot6530_porta_in_set(running_device *device, UINT8 data, UINT8 mask)
{
	miot6530_state *miot = get_safe_token(device);
	miot->port[0].in = (miot->port[0].in & ~mask) | (data & mask);
}


/*-------------------------------------------------
    miot6530_portb_in_set - set port B input
    value
-------------------------------------------------*/

void miot6530_portb_in_set(running_device *device, UINT8 data, UINT8 mask)
{
	miot6530_state *miot = get_safe_token(device);
	miot->port[1].in = (miot->port[1].in & ~mask) | (data & mask);
}


/*-------------------------------------------------
    miot6530_porta_in_get - return port A input
    value
-------------------------------------------------*/

UINT8 miot6530_porta_in_get(running_device *device)
{
	miot6530_state *miot = get_safe_token(device);
	return miot->port[0].in;
}


/*-------------------------------------------------
    miot6530_portb_in_get - return port B input
    value
-------------------------------------------------*/

UINT8 miot6530_portb_in_get(running_device *device)
{
	miot6530_state *miot = get_safe_token(device);
	return miot->port[1].in;
}


/*-------------------------------------------------
    miot6530_porta_in_get - return port A output
    value
-------------------------------------------------*/

UINT8 miot6530_porta_out_get(running_device *device)
{
	miot6530_state *miot = get_safe_token(device);
	return miot->port[0].out;
}


/*-------------------------------------------------
    miot6530_portb_in_get - return port B output
    value
-------------------------------------------------*/

UINT8 miot6530_portb_out_get(running_device *device)
{
	miot6530_state *miot = get_safe_token(device);
	return miot->port[1].out;
}


/***************************************************************************
    DEVICE INTERFACE
***************************************************************************/

static DEVICE_START( miot6530 )
{
	miot6530_state *miot = get_safe_token(device);

	/* validate arguments */
	assert(device != NULL);
	assert(device->tag != NULL);

	/* set static values */
	miot->intf = (const miot6530_interface*)device->baseconfig().static_config;
	miot->clock = device->clock;

	/* configure the ports */
	miot->port[0].in_func = miot->intf->in_a_func;
	miot->port[0].out_func = miot->intf->out_a_func;
	miot->port[1].in_func = miot->intf->in_b_func;
	miot->port[1].out_func = miot->intf->out_b_func;

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


static DEVICE_RESET( miot6530 )
{
	miot6530_state *miot = get_safe_token(device);

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


DEVICE_GET_INFO( miot6530 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(miot6530_state);		break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;							break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;		break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(miot6530);break;
		case DEVINFO_FCT_STOP:							/* Nothing */							break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(miot6530);break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "6530 (MIOT)");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "I/O devices");				break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);						break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright Nicola Salmoria and the MAME Team"); break;
	}
}
