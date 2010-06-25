/**********************************************************************

    Intel 8155 - 2048-Bit Static MOS RAM with I/O Ports and Timer emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

    TODO:

    - strobed mode

*/

#include "i8155.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

enum
{
	I8155_REGISTER_COMMAND = 0,
	I8155_REGISTER_STATUS = 0,
	I8155_REGISTER_PORT_A,
	I8155_REGISTER_PORT_B,
	I8155_REGISTER_PORT_C,
	I8155_REGISTER_TIMER_LOW,
	I8155_REGISTER_TIMER_HIGH
};

enum
{
	I8155_PORT_A = 0,
	I8155_PORT_B,
	I8155_PORT_C,
	I8155_PORT_COUNT
};

enum
{
	I8155_PORT_MODE_INPUT = 0,
	I8155_PORT_MODE_OUTPUT,
	I8155_PORT_MODE_STROBED_PORT_A,	/* not supported */
	I8155_PORT_MODE_STROBED			/* not supported */
};

#define I8155_COMMAND_PA					0x01
#define I8155_COMMAND_PB					0x02
#define I8155_COMMAND_PC_MASK				0x0c
#define I8155_COMMAND_PC_ALT_1				0x00
#define I8155_COMMAND_PC_ALT_2				0x0c
#define I8155_COMMAND_PC_ALT_3				0x04	/* not supported */
#define I8155_COMMAND_PC_ALT_4				0x08	/* not supported */
#define I8155_COMMAND_IEA					0x10	/* not supported */
#define I8155_COMMAND_IEB					0x20	/* not supported */
#define I8155_COMMAND_TM_MASK				0xc0
#define I8155_COMMAND_TM_NOP				0x00
#define I8155_COMMAND_TM_STOP				0x40
#define I8155_COMMAND_TM_STOP_AFTER_TC		0x80
#define I8155_COMMAND_TM_START				0xc0

#define I8155_STATUS_INTR_A					0x01	/* not supported */
#define I8155_STATUS_A_BF					0x02	/* not supported */
#define I8155_STATUS_INTE_A					0x04	/* not supported */
#define I8155_STATUS_INTR_B					0x08	/* not supported */
#define I8155_STATUS_B_BF					0x10	/* not supported */
#define I8155_STATUS_INTE_B					0x20	/* not supported */
#define I8155_STATUS_TIMER					0x40

#define I8155_TIMER_MODE_MASK				0xc0
#define I8155_TIMER_MODE_LOW				0x00
#define I8155_TIMER_MODE_SQUARE_WAVE		0x40
#define I8155_TIMER_MODE_SINGLE_PULSE		0x80
#define I8155_TIMER_MODE_AUTOMATIC_RELOAD	0xc0

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _i8155_t i8155_t;
struct _i8155_t
{
	devcb_resolved_read8		in_port_func[I8155_PORT_COUNT];
	devcb_resolved_write8		out_port_func[I8155_PORT_COUNT];
	devcb_resolved_write_line	out_to_func;

	/* memory */
	UINT8 ram[256];				/* RAM */

	/* registers */
	UINT8 command;				/* command register */
	UINT8 status;				/* status register */
	UINT8 output[3];			/* output latches */

	/* counter */
	UINT16 count_length;		/* count length register */
	UINT16 counter;				/* counter register */
	int to;						/* timer output */

	/* timers */
	emu_timer *timer;			/* counter timer */
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE i8155_t *get_safe_token(running_device *device)
{
	assert(device != NULL);
	return (i8155_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE const i8155_interface *get_interface(running_device *device)
{
	assert(device != NULL);
	assert(device->type() == I8155);
	return (const i8155_interface *) device->baseconfig().static_config();
}

INLINE UINT8 get_timer_mode(i8155_t *i8155)
{
	return (i8155->count_length >> 8) & I8155_TIMER_MODE_MASK;
}

INLINE void timer_output(i8155_t *i8155)
{
	devcb_call_write_line(&i8155->out_to_func, i8155->to);

	if (LOG) logerror("8155 Timer Output: %u\n", i8155->to);
}

INLINE void pulse_timer_output(i8155_t *i8155)
{
	i8155->to = 0; timer_output(i8155);
	i8155->to = 1; timer_output(i8155);
}

INLINE int get_port_mode(i8155_t *i8155, int port)
{
	int mode = -1;

	switch (port)
	{
	case I8155_PORT_A:
		mode = (i8155->command & I8155_COMMAND_PA) ? I8155_PORT_MODE_OUTPUT : I8155_PORT_MODE_INPUT;
		break;

	case I8155_PORT_B:
		mode = (i8155->command & I8155_COMMAND_PB) ? I8155_PORT_MODE_OUTPUT : I8155_PORT_MODE_INPUT;
		break;

	case I8155_PORT_C:
		switch (i8155->command & I8155_COMMAND_PC_MASK)
		{
		case I8155_COMMAND_PC_ALT_1: mode = I8155_PORT_MODE_INPUT;			break;
		case I8155_COMMAND_PC_ALT_2: mode = I8155_PORT_MODE_OUTPUT;			break;
		case I8155_COMMAND_PC_ALT_3: mode = I8155_PORT_MODE_STROBED_PORT_A; break;
		case I8155_COMMAND_PC_ALT_4: mode = I8155_PORT_MODE_STROBED;		break;
		}
		break;
	}

	return mode;
}

INLINE UINT8 read_port(i8155_t *i8155, int port)
{
	UINT8 data = 0;

	switch (port)
	{
	case I8155_PORT_A:
	case I8155_PORT_B:
		switch (get_port_mode(i8155, port))
		{
		case I8155_PORT_MODE_INPUT:
			data = devcb_call_read8(&i8155->in_port_func[port], 0);
			break;

		case I8155_PORT_MODE_OUTPUT:
			data = i8155->output[port];
			break;
		}
		break;

	case I8155_PORT_C:
		switch (get_port_mode(i8155, I8155_PORT_C))
		{
		case I8155_PORT_MODE_INPUT:
			data = devcb_call_read8(&i8155->in_port_func[port], 0) & 0x3f;
			break;

		case I8155_PORT_MODE_OUTPUT:
			data = i8155->output[port] & 0x3f;
			break;

		default:
			logerror("8155 Unsupported Port C mode!\n");
		}
		break;
	}

	return data;
}

INLINE void write_port(i8155_t *i8155, int port, UINT8 data)
{
	switch (get_port_mode(i8155, port))
	{
	case I8155_PORT_MODE_OUTPUT:
		i8155->output[port] = data;
		devcb_call_write8(&i8155->out_port_func[port], 0, i8155->output[port]);
		break;
	}
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    TIMER_CALLBACK( counter_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( counter_tick )
{
	running_device *device = (running_device *)ptr;
	i8155_t *i8155 = get_safe_token(device);

	/* count down */
	i8155->counter--;

	if (get_timer_mode(i8155) == I8155_TIMER_MODE_LOW)
	{
		/* pulse on every count */
		pulse_timer_output(i8155);
	}

	if (i8155->counter == 0)
	{
		if (LOG) logerror("8155 Timer Count Reached\n");

		switch (i8155->command & I8155_COMMAND_TM_MASK)
		{
		case I8155_COMMAND_TM_STOP_AFTER_TC:
			/* stop timer */
			timer_enable(i8155->timer, 0);

			if (LOG) logerror("8155 Timer Stopped\n");
			break;
		}

		switch (get_timer_mode(i8155))
		{
		case I8155_TIMER_MODE_SQUARE_WAVE:
			/* toggle timer output */
			i8155->to = !i8155->to;
			timer_output(i8155);
			break;

		case I8155_TIMER_MODE_SINGLE_PULSE:
			/* single pulse upon TC being reached */
			pulse_timer_output(i8155);

			/* clear timer mode setting */
			i8155->command &= ~I8155_COMMAND_TM_MASK;
			break;

		case I8155_TIMER_MODE_AUTOMATIC_RELOAD:
			/* automatic reload, i.e. single pulse every time TC is reached */
			pulse_timer_output(i8155);
			break;
		}

		/* set timer flag */
		i8155->status |= I8155_STATUS_TIMER;

		/* reload timer counter */
		i8155->counter = i8155->count_length & 0x3fff;
	}
}

/*-------------------------------------------------
    i8155_register_r - register read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( i8155_r )
{
	i8155_t *i8155 = get_safe_token(device);

	UINT8 data = 0;

	switch (offset & 0x03)
	{
	case I8155_REGISTER_STATUS:
		data = i8155->status;

		/* clear timer flag */
		i8155->status &= ~I8155_STATUS_TIMER;
		break;

	case I8155_REGISTER_PORT_A:
		data = read_port(i8155, I8155_PORT_A);
		break;

	case I8155_REGISTER_PORT_B:
		data = read_port(i8155, I8155_PORT_B);
		break;

	case I8155_REGISTER_PORT_C:
		data = read_port(i8155, I8155_PORT_C);
		break;
	}

	return data;
}

/*-------------------------------------------------
    i8155_register_w - register write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( i8155_w )
{
	i8155_t *i8155 = get_safe_token(device);

	switch (offset & 0x07)
	{
	case I8155_REGISTER_COMMAND:
		i8155->command = data;

		if (LOG) logerror("8155 Port A Mode: %s\n", (data & I8155_COMMAND_PA) ? "output" : "input");
		if (LOG) logerror("8155 Port B Mode: %s\n", (data & I8155_COMMAND_PB) ? "output" : "input");

		if (LOG) logerror("8155 Port A Interrupt: %s\n", (data & I8155_COMMAND_IEA) ? "enabled" : "disabled");
		if (LOG) logerror("8155 Port B Interrupt: %s\n", (data & I8155_COMMAND_IEB) ? "enabled" : "disabled");

		switch (data & I8155_COMMAND_PC_MASK)
		{
		case I8155_COMMAND_PC_ALT_1:
			if (LOG) logerror("8155 Port C Mode: Alt 1\n");
			break;

		case I8155_COMMAND_PC_ALT_2:
			if (LOG) logerror("8155 Port C Mode: Alt 2\n");
			break;

		case I8155_COMMAND_PC_ALT_3:
			if (LOG) logerror("8155 Port C Mode: Alt 3\n");
			break;

		case I8155_COMMAND_PC_ALT_4:
			if (LOG) logerror("8155 Port C Mode: Alt 4\n");
			break;
		}

		switch (data & I8155_COMMAND_TM_MASK)
		{
		case I8155_COMMAND_TM_NOP:
			/* do not affect counter operation */
			break;

		case I8155_COMMAND_TM_STOP:
			/* NOP if timer has not started, stop counting if the timer is running */
			if (LOG) logerror("8155 Timer Command: Stop\n");
			timer_enable(i8155->timer, 0);
			break;

		case I8155_COMMAND_TM_STOP_AFTER_TC:
			/* stop immediately after present TC is reached (NOP if timer has not started) */
			if (LOG) logerror("8155 Timer Command: Stop after TC\n");
			break;

		case I8155_COMMAND_TM_START:
			if (LOG) logerror("8155 Timer Command: Start\n");

			if (timer_enabled(i8155->timer))
			{
				/* if timer is running, start the new mode and CNT length immediately after present TC is reached */
			}
			else
			{
				/* load mode and CNT length and start immediately after loading (if timer is not running) */
				i8155->counter = i8155->count_length;
				timer_adjust_periodic(i8155->timer, attotime_zero, 0, ATTOTIME_IN_HZ(device->clock()));
			}
			break;
		}
		break;

	case I8155_REGISTER_PORT_A:
		write_port(i8155, I8155_PORT_A, data);
		break;

	case I8155_REGISTER_PORT_B:
		write_port(i8155, I8155_PORT_B, data);
		break;

	case I8155_REGISTER_PORT_C:
		write_port(i8155, I8155_PORT_C, data);
		break;

	case I8155_REGISTER_TIMER_LOW:
		i8155->count_length = (i8155->count_length & 0xff00) | data;
		if (LOG) logerror("8155 Count Length Low: %04x\n", i8155->count_length);
		break;

	case I8155_REGISTER_TIMER_HIGH:
		i8155->count_length = (data << 8) | (i8155->count_length & 0xff);
		if (LOG) logerror("8155 Count Length High: %04x\n", i8155->count_length);

		switch (data & I8155_TIMER_MODE_MASK)
		{
		case I8155_TIMER_MODE_LOW:
			/* puts out LOW during second half of count */
			if (LOG) logerror("8155 Timer Mode: LOW\n");
			break;

		case I8155_TIMER_MODE_SQUARE_WAVE:
			/* square wave, i.e. the period of the square wave equals the count length programmed with automatic reload at terminal count */
			if (LOG) logerror("8155 Timer Mode: Square wave\n");
			break;

		case I8155_TIMER_MODE_SINGLE_PULSE:
			/* single pulse upon TC being reached */
			if (LOG) logerror("8155 Timer Mode: Single pulse\n");
			break;

		case I8155_TIMER_MODE_AUTOMATIC_RELOAD:
			/* automatic reload, i.e. single pulse every time TC is reached */
			if (LOG) logerror("8155 Timer Mode: Automatic reload\n");
			break;
		}
		break;
	}
}

/*-------------------------------------------------
    i8155_ram_r - memory read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( i8155_ram_r )
{
	i8155_t *i8155 = get_safe_token(device);

	return i8155->ram[offset & 0xff];
}

/*-------------------------------------------------
    i8155_ram_w - memory write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( i8155_ram_w )
{
	i8155_t *i8155 = get_safe_token(device);

	i8155->ram[offset & 0xff] = data;
}

/*-------------------------------------------------
    DEVICE_START( i8155 )
-------------------------------------------------*/

static DEVICE_START( i8155 )
{
	i8155_t *i8155 = (i8155_t *)downcast<legacy_device_base *>(device)->token();
	const i8155_interface *intf = get_interface(device);

	/* resolve callbacks */
	devcb_resolve_read8(&i8155->in_port_func[0], &intf->in_pa_func, device);
	devcb_resolve_read8(&i8155->in_port_func[1], &intf->in_pb_func, device);
	devcb_resolve_read8(&i8155->in_port_func[2], &intf->in_pc_func, device);
	devcb_resolve_write8(&i8155->out_port_func[0], &intf->out_pa_func, device);
	devcb_resolve_write8(&i8155->out_port_func[1], &intf->out_pb_func, device);
	devcb_resolve_write8(&i8155->out_port_func[2], &intf->out_pc_func, device);
	devcb_resolve_write_line(&i8155->out_to_func, &intf->out_to_func, device);

	/* create the timers */
	i8155->timer = timer_alloc(device->machine, counter_tick, (void *)device);

	/* register for state saving */
	state_save_register_device_item(device, 0, i8155->command);
	state_save_register_device_item(device, 0, i8155->status);
	state_save_register_device_item_array(device, 0, i8155->output);
	state_save_register_device_item(device, 0, i8155->count_length);
	state_save_register_device_item(device, 0, i8155->counter);
	state_save_register_device_item_array(device, 0, i8155->ram);
	state_save_register_device_item(device, 0, i8155->to);
}

/*-------------------------------------------------
    DEVICE_RESET( i8155 )
-------------------------------------------------*/

static DEVICE_RESET( i8155 )
{
	i8155_t *i8155 = (i8155_t *)downcast<legacy_device_base *>(device)->token();

	/* clear output registers */
	i8155->output[I8155_PORT_A] = 0;
	i8155->output[I8155_PORT_B] = 0;
	i8155->output[I8155_PORT_C] = 0;

	/* set ports to input mode */
	i8155_w(device, I8155_REGISTER_COMMAND, i8155->command & ~(I8155_COMMAND_PA | I8155_COMMAND_PB | I8155_COMMAND_PC_MASK));

	/* clear timer flag */
	i8155->status &= ~I8155_STATUS_TIMER;

	/* stop counting */
	timer_enable(i8155->timer, 0);

	/* clear timer output */
	i8155->to = 1;
	timer_output(i8155);
}

/*-------------------------------------------------
    DEVICE_GET_INFO( i8155 )
-------------------------------------------------*/

DEVICE_GET_INFO( i8155 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(i8155_t);					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(i8155);		break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(i8155);		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Intel 8155");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Intel MCS-85");			break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team");	break;
	}
}

DEFINE_LEGACY_DEVICE(I8155, i8155);
