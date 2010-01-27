/**********************************************************************

    MOS Technology 6530 Memory, I/O, Timer Array emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

	TODO:

	- interrupt flag register
	- internal RAM
	- internal ROM

*/

#include "mos6530.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 1

enum
{
	REGISTER_PORT_A = 0,
	REGISTER_PORT_A_DDR,
	REGISTER_PORT_B,
	REGISTER_PORT_B_DDR,
	REGISTER_TIMER_1T, REGISTER_TIMER = REGISTER_TIMER_1T,
	REGISTER_TIMER_8T, REGISTER_INTERRUPT_FLAG = REGISTER_TIMER_8T,
	REGISTER_TIMER_64T,
	REGISTER_TIMER_1024T
};

enum
{
	PORT_A = 0,
	PORT_B,
	PORT_COUNT
};

static const int PRESCALER[] = { 1, 8, 64, 1024 };

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _mos6530_t mos6530_t;
struct _mos6530_t
{
	devcb_resolved_write_line	out_irq_func;

	devcb_resolved_read8		in_port_func[PORT_COUNT];
	devcb_resolved_write8		out_port_func[PORT_COUNT];

	/* registers */
	UINT8 output[PORT_COUNT];	/* output latches */
	UINT8 ddr[PORT_COUNT];		/* DDR latches */
	UINT8 counter;				/* counter */
	int irq_enabled;

	/* timers */
	emu_timer *timer;			/* timer */
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE mos6530_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	return (mos6530_t *)device->token;
}

INLINE const mos6530_interface *get_interface(const device_config *device)
{
	assert(device != NULL);
	assert(device->type == MOS6530);
	return (const mos6530_interface *) device->static_config;
}

INLINE UINT8 read_port(mos6530_t *mos6530, int port)
{
	UINT8 data = mos6530->output[port] & mos6530->ddr[port];

	if (mos6530->ddr[port] != 0xff)
	{
		data |= devcb_call_read8(&mos6530->in_port_func[port], 0) & ~mos6530->ddr[port];
	}

	return data;
}

INLINE void write_port(mos6530_t *mos6530, int port, UINT8 data)
{
	mos6530->output[port] = data;

	devcb_call_write8(&mos6530->out_port_func[port], 0, mos6530->output[port] & mos6530->ddr[port]);
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    TIMER_CALLBACK( timer_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( timer_tick )
{
	const device_config *device = (const device_config *)ptr;
	mos6530_t *mos6530 = get_safe_token(device);

	if (mos6530->counter == 0xff)
	{
		if (mos6530->irq_enabled)
		{
			/* interrupt */
			logerror("MOS6530 '%s' Interrupt\n", device->tag.cstr());
			devcb_call_write_line(&mos6530->out_irq_func, ASSERT_LINE);
		}
	}

	mos6530->counter--;
}

/*-------------------------------------------------
    mos6530_register_r - register read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( mos6530_r )
{
	mos6530_t *mos6530 = get_safe_token(device);
	int port = BIT(offset, 1);

	UINT8 data = 0;

	switch (offset & 0x07)
	{
	case REGISTER_PORT_A:
	case REGISTER_PORT_B:
		data = read_port(mos6530, port);
		break;

	case REGISTER_PORT_A_DDR:
	case REGISTER_PORT_B_DDR:
		data = mos6530->ddr[port];
		break;

	default:
		switch (offset & 0x05)
		{
		case REGISTER_TIMER:
			data = mos6530->counter;
			devcb_call_write_line(&mos6530->out_irq_func, CLEAR_LINE);
			logerror("MOS6530 '%s' Interrupt Cleared\n", device->tag.cstr());
			break;

		case REGISTER_INTERRUPT_FLAG:
			break;
		}
	}

	return data;
}

/*-------------------------------------------------
    mos6530_register_w - register write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( mos6530_w )
{
	mos6530_t *mos6530 = get_safe_token(device);
	int port = BIT(offset, 1);

	switch (offset & 0x07)
	{
	case REGISTER_PORT_A:
	case REGISTER_PORT_B:
		if (LOG) logerror("MOS6530 '%s' Port %c Write %02x\n", device->tag.cstr(), 'A' + port, data);
		write_port(mos6530, port, data);
		break;

	case REGISTER_PORT_A_DDR:
	case REGISTER_PORT_B_DDR:
		if (LOG) logerror("MOS6530 '%s' Port %c DDR: %02x\n", device->tag.cstr(), 'A' + port, data);
		mos6530->ddr[port] = data;
		write_port(mos6530, port, data);
		break;

	case REGISTER_TIMER_1T:
	case REGISTER_TIMER_8T:
	case REGISTER_TIMER_64T:
	case REGISTER_TIMER_1024T:
		if (LOG) logerror("MOS6530 '%s' Timer: %02x, %uT, IRQ %s\n", device->tag.cstr(), data, PRESCALER[offset & 0x03], BIT(offset, 3) ? "enabled" : "disabled");
		mos6530->counter = data;
		mos6530->irq_enabled = BIT(offset, 3);
		timer_adjust_periodic(mos6530->timer, attotime_zero, 0, ATTOTIME_IN_HZ(device->clock / PRESCALER[offset & 0x03]));
		devcb_call_write_line(&mos6530->out_irq_func, CLEAR_LINE);
		break;
	}
}

/*-------------------------------------------------
    DEVICE_START( mos6530 )
-------------------------------------------------*/

static DEVICE_START( mos6530 )
{
	mos6530_t *mos6530 = (mos6530_t *)device->token;
	const mos6530_interface *intf = get_interface(device);

	/* resolve callbacks */
	devcb_resolve_write_line(&mos6530->out_irq_func, &intf->out_irq_func, device);
	devcb_resolve_read8(&mos6530->in_port_func[PORT_A], &intf->in_pa_func, device);
	devcb_resolve_read8(&mos6530->in_port_func[PORT_B], &intf->in_pb_func, device);
	devcb_resolve_write8(&mos6530->out_port_func[PORT_A], &intf->out_pa_func, device);
	devcb_resolve_write8(&mos6530->out_port_func[PORT_B], &intf->out_pb_func, device);

	/* allocate timer */
	mos6530->timer = timer_alloc(device->machine, timer_tick, (void *)device);

	/* register for state saving */
	state_save_register_device_item_array(device, 0, mos6530->output);
	state_save_register_device_item_array(device, 0, mos6530->ddr);
}

/*-------------------------------------------------
    DEVICE_RESET( mos6530 )
-------------------------------------------------*/

static DEVICE_RESET( mos6530 )
{
	mos6530_t *mos6530 = (mos6530_t *)device->token;

	/* set ports to input mode */
	mos6530->ddr[PORT_A] = 0;
	mos6530->ddr[PORT_B] = 0;
}

/*-------------------------------------------------
    DEVICE_GET_INFO( mos6530 )
-------------------------------------------------*/

DEVICE_GET_INFO( mos6530 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(mos6530_t);				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(mos6530);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(mos6530);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "MOS6530");					break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "MOS6500");					break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team");	break;
	}
}
