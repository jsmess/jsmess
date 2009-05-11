/**********************************************************************

    8155 - 2048-Bit Static MOS RAM with I/O Ports and Timer emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

	TODO:

	- strobed mode

*/

#include "8155pio.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

enum
{
	PIO8155_REGISTER_COMMAND = 0,
	PIO8155_REGISTER_STATUS = 0,
	PIO8155_REGISTER_PORT_A,
	PIO8155_REGISTER_PORT_B,
	PIO8155_REGISTER_PORT_C,
	PIO8155_REGISTER_TIMER_LOW,
	PIO8155_REGISTER_TIMER_HIGH
};

enum
{
	PIO8155_PORT_A = 0,
	PIO8155_PORT_B,
	PIO8155_PORT_C,
	PIO8155_PORT_COUNT
};

enum
{
	PIO8155_PORT_MODE_INPUT = 0,
	PIO8155_PORT_MODE_OUTPUT,
	PIO8155_PORT_MODE_STROBED_PORT_A,	/* not supported */
	PIO8155_PORT_MODE_STROBED			/* not supported */
};

#define PIO8155_COMMAND_PA					0x01
#define PIO8155_COMMAND_PB					0x02
#define PIO8155_COMMAND_PC_MASK				0x0c
#define PIO8155_COMMAND_PC_ALT_1			0x00
#define PIO8155_COMMAND_PC_ALT_2			0x0c
#define PIO8155_COMMAND_PC_ALT_3			0x04	/* not supported */
#define PIO8155_COMMAND_PC_ALT_4			0x08	/* not supported */
#define PIO8155_COMMAND_IEA					0x10	/* not supported */
#define PIO8155_COMMAND_IEB					0x20	/* not supported */
#define PIO8155_COMMAND_TM_MASK				0xc0
#define PIO8155_COMMAND_TM_NOP				0x00
#define PIO8155_COMMAND_TM_STOP				0x40
#define PIO8155_COMMAND_TM_STOP_AFTER_TC	0x80
#define PIO8155_COMMAND_TM_START			0xc0

#define PIO8155_STATUS_INTR_A				0x01	/* not supported */
#define PIO8155_STATUS_A_BF					0x02	/* not supported */
#define PIO8155_STATUS_INTE_A				0x04	/* not supported */
#define PIO8155_STATUS_INTR_B				0x08	/* not supported */
#define PIO8155_STATUS_B_BF					0x10	/* not supported */
#define PIO8155_STATUS_INTE_B				0x20	/* not supported */
#define PIO8155_STATUS_TIMER				0x40

#define PIO8155_TIMER_MODE_MASK				0xc0
#define PIO8155_TIMER_MODE_LOW				0x00
#define PIO8155_TIMER_MODE_SQUARE_WAVE		0x40
#define PIO8155_TIMER_MODE_SINGLE_PULSE		0x80
#define PIO8155_TIMER_MODE_AUTOMATIC_RELOAD	0xc0

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _pio8155_t pio8155_t;
struct _pio8155_t
{
	devcb_resolved_read8		in_port_func[PIO8155_PORT_COUNT];
	devcb_resolved_write8		out_port_func[PIO8155_PORT_COUNT];
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

INLINE pio8155_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	return (pio8155_t *)device->token;
}

INLINE const pio8155_interface *get_interface(const device_config *device)
{
	assert(device != NULL);
	assert(device->type == PIO8155);
	return (const pio8155_interface *) device->static_config;
}

INLINE UINT8 get_timer_mode(pio8155_t *pio8155)
{
	return (pio8155->count_length >> 8) & PIO8155_TIMER_MODE_MASK;
}

INLINE void timer_output(pio8155_t *pio8155)
{
	devcb_call_write_line(&pio8155->out_to_func, pio8155->to);

	if (LOG) logerror("8155 Timer Output: %u\n", pio8155->to);
}

INLINE void pulse_timer_output(pio8155_t *pio8155)
{
	pio8155->to = 0; timer_output(pio8155);
	pio8155->to = 1; timer_output(pio8155);
}

INLINE int get_port_mode(pio8155_t *pio8155, int port)
{
	int mode = -1;

	switch (port)
	{
	case PIO8155_PORT_A:
		mode = (pio8155->command & PIO8155_COMMAND_PA) ? PIO8155_PORT_MODE_OUTPUT : PIO8155_PORT_MODE_INPUT;
		break;

	case PIO8155_PORT_B:
		mode = (pio8155->command & PIO8155_COMMAND_PB) ? PIO8155_PORT_MODE_OUTPUT : PIO8155_PORT_MODE_INPUT;
		break;

	case PIO8155_PORT_C:
		switch (pio8155->command & PIO8155_COMMAND_PC_MASK)
		{
		case PIO8155_COMMAND_PC_ALT_1: mode = PIO8155_PORT_MODE_INPUT;			break;
		case PIO8155_COMMAND_PC_ALT_2: mode = PIO8155_PORT_MODE_OUTPUT;			break;
		case PIO8155_COMMAND_PC_ALT_3: mode = PIO8155_PORT_MODE_STROBED_PORT_A; break;
		case PIO8155_COMMAND_PC_ALT_4: mode = PIO8155_PORT_MODE_STROBED;		break;
		}
		break;
	}

	return mode;
}

INLINE UINT8 read_port(pio8155_t *pio8155, int port)
{
	UINT8 data = 0;

	switch (port)
	{
	case PIO8155_PORT_A:
	case PIO8155_PORT_B:
		switch (get_port_mode(pio8155, port))
		{
		case PIO8155_PORT_MODE_INPUT:
			data = devcb_call_read8(&pio8155->in_port_func[port], 0);
			break;

		case PIO8155_PORT_MODE_OUTPUT:
			data = pio8155->output[port];
			break;
		}
		break;

	case PIO8155_PORT_C:
		switch (get_port_mode(pio8155, PIO8155_PORT_C))
		{
		case PIO8155_PORT_MODE_INPUT:
			data = devcb_call_read8(&pio8155->in_port_func[port], 0) & 0x3f;
			break;

		case PIO8155_PORT_MODE_OUTPUT:
			data = pio8155->output[port] & 0x3f;
			break;

		default:
			logerror("8155 Unsupported Port C mode!\n");
		}
		break;
	}

	return data;
}

INLINE void write_port(pio8155_t *pio8155, int port, UINT8 data)
{
	switch (get_port_mode(pio8155, port))
	{
	case PIO8155_PORT_MODE_OUTPUT:
		pio8155->output[port] = data;
		devcb_call_write8(&pio8155->out_port_func[port], 0, pio8155->output[port]);
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
	const device_config *device = ptr;
	pio8155_t *pio8155 = get_safe_token(device);

	/* count down */
	pio8155->counter--;

	if (get_timer_mode(pio8155) == PIO8155_TIMER_MODE_LOW)
	{
		/* pulse on every count */
		pulse_timer_output(pio8155);
	}

	if (pio8155->counter == 0)
	{
		if (LOG) logerror("8155 Timer Count Reached\n");

		switch (pio8155->command & PIO8155_COMMAND_TM_MASK)
		{
		case PIO8155_COMMAND_TM_STOP_AFTER_TC:
			/* stop timer */
			timer_enable(pio8155->timer, 0);

			if (LOG) logerror("8155 Timer Stopped\n");
			break;
		}

		switch (get_timer_mode(pio8155))
		{
		case PIO8155_TIMER_MODE_SQUARE_WAVE:
			/* toggle timer output */
			pio8155->to = !pio8155->to;
			timer_output(pio8155);
			break;

		case PIO8155_TIMER_MODE_SINGLE_PULSE:
			/* single pulse upon TC being reached */
			pulse_timer_output(pio8155);

			/* clear timer mode setting */
			pio8155->command &= ~PIO8155_COMMAND_TM_MASK;
			break;

		case PIO8155_TIMER_MODE_AUTOMATIC_RELOAD:
			/* automatic reload, i.e. single pulse every time TC is reached */
			pulse_timer_output(pio8155);
			break;
		}

		/* set timer flag */
		pio8155->status |= PIO8155_STATUS_TIMER;

		/* reload timer counter */
		pio8155->counter = pio8155->count_length & 0x3fff;
	}
}

/*-------------------------------------------------
    pio8155_register_r - register read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( pio8155_r )
{
	pio8155_t *pio8155 = get_safe_token(device);

	UINT8 data = 0;

	switch (offset & 0x03)
	{
	case PIO8155_REGISTER_STATUS:
		data = pio8155->status;

		/* clear timer flag */
		pio8155->status &= ~PIO8155_STATUS_TIMER;
		break;

	case PIO8155_REGISTER_PORT_A:
		data = read_port(pio8155, PIO8155_PORT_A);
		break;

	case PIO8155_REGISTER_PORT_B:
		data = read_port(pio8155, PIO8155_PORT_B);
		break;

	case PIO8155_REGISTER_PORT_C:
		data = read_port(pio8155, PIO8155_PORT_C);
		break;
	}

	return data;
}

/*-------------------------------------------------
    pio8155_register_w - register write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( pio8155_w )
{
	pio8155_t *pio8155 = get_safe_token(device);
	
	switch (offset & 0x07)
	{
	case PIO8155_REGISTER_COMMAND:
		pio8155->command = data;

		if (LOG) logerror("8155 Port A Mode: %s\n", (data & PIO8155_COMMAND_PA) ? "output" : "input");
		if (LOG) logerror("8155 Port B Mode: %s\n", (data & PIO8155_COMMAND_PB) ? "output" : "input");

		if (LOG) logerror("8155 Port A Interrupt: %s\n", (data & PIO8155_COMMAND_IEA) ? "enabled" : "disabled");
		if (LOG) logerror("8155 Port B Interrupt: %s\n", (data & PIO8155_COMMAND_IEB) ? "enabled" : "disabled");

		switch (data & PIO8155_COMMAND_PC_MASK)
		{
		case PIO8155_COMMAND_PC_ALT_1:
			if (LOG) logerror("8155 Port C Mode: Alt 1\n");
			break;

		case PIO8155_COMMAND_PC_ALT_2:
			if (LOG) logerror("8155 Port C Mode: Alt 2\n");
			break;

		case PIO8155_COMMAND_PC_ALT_3:
			if (LOG) logerror("8155 Port C Mode: Alt 3\n");
			break;

		case PIO8155_COMMAND_PC_ALT_4:
			if (LOG) logerror("8155 Port C Mode: Alt 4\n");
			break;
		}

		switch (data & PIO8155_COMMAND_TM_MASK)
		{
		case PIO8155_COMMAND_TM_NOP:
			/* do not affect counter operation */
			break;

		case PIO8155_COMMAND_TM_STOP:
			/* NOP if timer has not started, stop counting if the timer is running */
			if (LOG) logerror("8155 Timer Command: Stop\n");
			timer_enable(pio8155->timer, 0);
			break;

		case PIO8155_COMMAND_TM_STOP_AFTER_TC:
			/* stop immediately after present TC is reached (NOP if timer has not started) */
			if (LOG) logerror("8155 Timer Command: Stop after TC\n");
			break;

		case PIO8155_COMMAND_TM_START:
			if (LOG) logerror("8155 Timer Command: Start\n");

			if (timer_enabled(pio8155->timer))
			{
				/* if timer is running, start the new mode and CNT length immediately after present TC is reached */
			}
			else
			{
				/* load mode and CNT length and start immediately after loading (if timer is not running) */
				pio8155->counter = pio8155->count_length;
				timer_adjust_periodic(pio8155->timer, attotime_zero, 0, ATTOTIME_IN_HZ(device->clock));
			}
			break;
		}
		break;

	case PIO8155_REGISTER_PORT_A:
		write_port(pio8155, PIO8155_PORT_A, data);
		break;

	case PIO8155_REGISTER_PORT_B:
		write_port(pio8155, PIO8155_PORT_B, data);
		break;

	case PIO8155_REGISTER_PORT_C:
		write_port(pio8155, PIO8155_PORT_C, data);
		break;

	case PIO8155_REGISTER_TIMER_LOW:
		pio8155->count_length = (pio8155->count_length & 0xff00) | data;
		if (LOG) logerror("8155 Count Length Low: %04x\n", pio8155->count_length);
		break;

	case PIO8155_REGISTER_TIMER_HIGH:
		pio8155->count_length = (data << 8) | (pio8155->count_length & 0xff);
		if (LOG) logerror("8155 Count Length High: %04x\n", pio8155->count_length);

		switch (data & PIO8155_TIMER_MODE_MASK)
		{
		case PIO8155_TIMER_MODE_LOW:
			/* puts out LOW during second half of count */
			if (LOG) logerror("8155 Timer Mode: LOW\n");
			break;

		case PIO8155_TIMER_MODE_SQUARE_WAVE:
			/* square wave, i.e. the period of the square wave equals the count length programmed with automatic reload at terminal count */
			if (LOG) logerror("8155 Timer Mode: Square wave\n");
			break;

		case PIO8155_TIMER_MODE_SINGLE_PULSE:
			/* single pulse upon TC being reached */
			if (LOG) logerror("8155 Timer Mode: Single pulse\n");
			break;

		case PIO8155_TIMER_MODE_AUTOMATIC_RELOAD:
			/* automatic reload, i.e. single pulse every time TC is reached */
			if (LOG) logerror("8155 Timer Mode: Automatic reload\n");
			break;
		}
		break;
	}
}

/*-------------------------------------------------
    pio8155_ram_r - memory read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( pio8155_ram_r )
{
	pio8155_t *pio8155 = get_safe_token(device);

	return pio8155->ram[offset & 0xff];
}

/*-------------------------------------------------
    pio8155_ram_w - memory write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( pio8155_ram_w )
{
	pio8155_t *pio8155 = get_safe_token(device);

	pio8155->ram[offset & 0xff] = data;
}

/*-------------------------------------------------
    DEVICE_START( pio8155 )
-------------------------------------------------*/

static DEVICE_START( pio8155 )
{
	pio8155_t *pio8155 = device->token;
	const pio8155_interface *intf = get_interface(device);

	/* resolve callbacks */
	devcb_resolve_read8(&pio8155->in_port_func[0], &intf->in_pa_func, device);
	devcb_resolve_read8(&pio8155->in_port_func[1], &intf->in_pb_func, device);
	devcb_resolve_read8(&pio8155->in_port_func[2], &intf->in_pc_func, device);
	devcb_resolve_write8(&pio8155->out_port_func[0], &intf->out_pa_func, device);
	devcb_resolve_write8(&pio8155->out_port_func[1], &intf->out_pb_func, device);
	devcb_resolve_write8(&pio8155->out_port_func[2], &intf->out_pc_func, device);
	devcb_resolve_write_line(&pio8155->out_to_func, &intf->out_to_func, device);

	/* create the timers */
	pio8155->timer = timer_alloc(device->machine, counter_tick, (void *)device);

	/* register for state saving */
	state_save_register_device_item(device, 0, pio8155->command);
	state_save_register_device_item(device, 0, pio8155->status);
	state_save_register_device_item_array(device, 0, pio8155->output);
	state_save_register_device_item(device, 0, pio8155->count_length);
	state_save_register_device_item(device, 0, pio8155->counter);
	state_save_register_device_item_array(device, 0, pio8155->ram);
	state_save_register_device_item(device, 0, pio8155->to);
}

/*-------------------------------------------------
    DEVICE_RESET( pio8155 )
-------------------------------------------------*/

static DEVICE_RESET( pio8155 )
{
	pio8155_t *pio8155 = device->token;

	/* clear output registers */
	pio8155->output[PIO8155_PORT_A] = 0;
	pio8155->output[PIO8155_PORT_B] = 0;
	pio8155->output[PIO8155_PORT_C] = 0;

	/* set ports to input mode */
	pio8155_w(device, PIO8155_REGISTER_COMMAND, pio8155->command & ~(PIO8155_COMMAND_PA | PIO8155_COMMAND_PB | PIO8155_COMMAND_PC_MASK));

	/* clear timer flag */
	pio8155->status &= ~PIO8155_STATUS_TIMER;

	/* stop counting */
	timer_enable(pio8155->timer, 0);

	/* clear timer output */
	pio8155->to = 1;
	timer_output(pio8155);
}

/*-------------------------------------------------
    DEVICE_GET_INFO( pio8155 )
-------------------------------------------------*/

DEVICE_GET_INFO( pio8155 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(pio8155_t);				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(pio8155);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(pio8155);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "OKI MSM81C55");			break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "8155");					break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team");	break;
	}
}
