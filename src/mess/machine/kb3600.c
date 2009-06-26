/**********************************************************************

    General Instruments AY-5-3600 Keyboard Encoder emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*
	
	TODO:

	- more accurate emulation of real chip
	- any key down

*/

#include "driver.h"
#include "kb3600.h"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _ay3600_t ay3600_t;
struct _ay3600_t
{
	devcb_resolved_write_line	out_data_ready_func;

	devcb_resolved_read_line	in_shift_func;
	devcb_resolved_read_line	in_control_func;

	int b;						/* output buffer */

	/* timers */
	emu_timer *scan_timer;		/* keyboard scan timer */
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE ay3600_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);

	return (ay3600_t *)device->token;
}

INLINE const ay3600_interface *get_interface(const device_config *device)
{
	assert(device != NULL);
	assert((device->type == AY3600PRO002));
	return (const ay3600_interface *) device->static_config;
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    TIMER_CALLBACK( ay3600_scan_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( ay3600_scan_tick )
{
	const device_config *device = ptr;
	ay3600_t *ay3600 = get_safe_token(device);
	const ay3600_interface *intf = get_interface(device);

	int x, y;
	int akd = 0;

	for (x = 0; x < 9; x++)
	{
		UINT16 data = intf->in_y_func(device, x);

		for (y = 0; y < 10; y++)
		{
			if (BIT(data, y))
			{
				int b = (x * 10) + y;

				akd = 1;

				if (b > 63)
				{
					b -= 64;
					b = 0x100 | b;
				}

				b |= (devcb_call_read_line(&ay3600->in_shift_func) << 6);
				b |= (devcb_call_read_line(&ay3600->in_control_func) << 7);

				if (ay3600->b != b)
				{
					ay3600->b = b;

					devcb_call_write_line(&ay3600->out_data_ready_func, 1);
					return;
				}
			}
		}
	}
	
	if (!akd)
	{
		ay3600->b = -1;
	}
}

/*-------------------------------------------------
    ay3600_b_r - keyboard data read
-------------------------------------------------*/

UINT16 ay3600_b_r(const device_config *device)
{
	ay3600_t *ay3600 = get_safe_token(device);

	UINT16 data = ay3600->b;

	devcb_call_write_line(&ay3600->out_data_ready_func, 0);

	return data;
}

/*-------------------------------------------------
    DEVICE_START( ay3600 )
-------------------------------------------------*/

static DEVICE_START( ay3600 )
{
	ay3600_t *ay3600 = get_safe_token(device);
	const ay3600_interface *intf = get_interface(device);

	/* resolve callbacks */
	devcb_resolve_write_line(&ay3600->out_data_ready_func, &intf->out_data_ready_func, device);
	devcb_resolve_read_line(&ay3600->in_shift_func, &intf->in_shift_func, device);
	devcb_resolve_read_line(&ay3600->in_control_func, &intf->in_control_func, device);

	/* create the timers */
	ay3600->scan_timer = timer_alloc(device->machine, ay3600_scan_tick, (void *)device);
	timer_adjust_periodic(ay3600->scan_timer, attotime_zero, 0, ATTOTIME_IN_HZ(60));

	/* register for state saving */
	state_save_register_device_item(device, 0, ay3600->b);
}

/*-------------------------------------------------
    DEVICE_GET_INFO( ay3600pro002 )
-------------------------------------------------*/

DEVICE_GET_INFO( ay3600pro002 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(ay3600_t);					break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(ay3600);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/* Nothing */								break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "GI AY-5-3600-PRO-002");	break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "GI AY-5-3600");			break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");		break;
	}
}
