/**********************************************************************

	Ricoh RP5C01A Real Time Clock With Internal RAM emulation

	Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

/*

	TODO:

	- everything

*/

#include "driver.h"
#include "rp5c01a.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 1

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _rp5c01a_t rp5c01a_t;
struct _rp5c01a_t
{
	devcb_resolved_write_line	out_alarm_func;
    
	/* timers */
	emu_timer *clock_timer;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE rp5c01a_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == RP5C01A);
	return (rp5c01a_t *)device->token;
}

INLINE const rp5c01a_interface *get_interface(const device_config *device)
{
	assert(device != NULL);
	assert((device->type == RP5C01A));
	return (const rp5c01a_interface *) device->static_config;
}

INLINE UINT8 convert_to_bcd(int val)
{
	return ((val / 10) << 4) | (val % 10);
}

INLINE int bcd_to_integer(UINT8 val)
{
	return (((val & 0xf0) >> 4) * 10) + (val & 0x0f);
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    rp5c01a_adj_w - clock adjust write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( rp5c01a_adj_w )
{
}

/*-------------------------------------------------
    rp5c01a_r - register read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( rp5c01a_r )
{
	return 0;
}

/*-------------------------------------------------
    rp5c01a_w - register write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( rp5c01a_w )
{
}

/*-------------------------------------------------
    TIMER_CALLBACK( clock_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( clock_tick )
{
//	const device_config *device = ptr;
//	rp5c01a_t *rp5c01a = get_safe_token(device);
}

/*-------------------------------------------------
    DEVICE_START( rp5c01a )
-------------------------------------------------*/

static DEVICE_START( rp5c01a )
{
	rp5c01a_t *rp5c01a = get_safe_token(device);
	const rp5c01a_interface *intf = get_interface(device);

	/* resolve callbacks */
	devcb_resolve_write_line(&rp5c01a->out_alarm_func, &intf->out_alarm_func, device);

	/* create the timers */
	rp5c01a->clock_timer = timer_alloc(device->machine, clock_tick, (void *)device);
	timer_adjust_periodic(rp5c01a->clock_timer, attotime_zero, 0, ATTOTIME_IN_HZ(1));

	/* register for state saving */
//    state_save_register_global(device->machine, rp5c01a->);
}

static DEVICE_RESET( rp5c01a )
{
/*	rp5c01a_t *rp5c01a = get_safe_token(device);

	mame_system_time curtime, *systime = &curtime;

	mame_get_current_datetime(device->machine, &curtime);*/

	/* HACK: load time counter from system time */
/*	rp5c01a->time_counter[0] = convert_to_bcd(systime->local_time.second);
	rp5c01a->time_counter[1] = convert_to_bcd(systime->local_time.minute);
	rp5c01a->time_counter[2] = convert_to_bcd(systime->local_time.hour);
	rp5c01a->time_counter[3] = convert_to_bcd(systime->local_time.mday);
	rp5c01a->time_counter[4] = systime->local_time.weekday;
	rp5c01a->time_counter[4] |= (systime->local_time.month + 1) << 4;*/
}

/*-------------------------------------------------
    DEVICE_GET_INFO( rp5c01a )
-------------------------------------------------*/

DEVICE_GET_INFO( rp5c01a )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(rp5c01a_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(rp5c01a);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(rp5c01a);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Ricoh RP5C01A");			break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Ricoh RP5C01A");			break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						/* Nothing */								break;
	}
}
