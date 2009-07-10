/**********************************************************************

    OKI MSM58321 Real Time Clock emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

	TODO:

	- count
	- stop
	- reset
	- reference registers

*/

#include "driver.h"
#include "msm58321.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 1

enum
{
	REGISTER_S1 = 0,
	REGISTER_S0,
	REGISTER_MI1,
	REGISTER_MI10,
	REGISTER_H1,
	REGISTER_H10,
	REGISTER_W,
	REGISTER_D1,
	REGISTER_D10,
	REGISTER_MO1,
	REGISTER_MO10,
	REGISTER_Y1,
	REGISTER_Y10,
	REGISTER_RESET,
	REGISTER_REF0,
	REGISTER_REF1
};

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _msm58321_t msm58321_t;
struct _msm58321_t
{
	devcb_resolved_write_line	out_busy_func;

	UINT8 reg[13];				/* registers */
	int busy;					/* busy flag */

	/* timers */
	emu_timer *busy_timer;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE msm58321_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);

	return (msm58321_t *)device->token;
}

INLINE const msm58321_interface *get_interface(const device_config *device)
{
	assert(device != NULL);
	assert(device->type == MSM58321);
	return (const msm58321_interface *) device->static_config;
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

READ8_DEVICE_HANDLER( msm58321_r )
{
	msm58321_t *msm58321 = get_safe_token(device);

	UINT8 data = 0;

	switch(offset & 0x0f)
	{
	case REGISTER_RESET:
		break;

	case REGISTER_REF0:
	case REGISTER_REF1:
		break;

	default:
		data = msm58321->reg[offset];
		break;
	}

	return data;
}

WRITE8_DEVICE_HANDLER( msm58321_w )
{
	msm58321_t *msm58321 = get_safe_token(device);

	switch(offset & 0x0f)
	{
	case REGISTER_RESET:
		if (LOG) logerror("MSM58321 '%s' Reset\n", device->tag);
		break;

	case REGISTER_REF0:
	case REGISTER_REF1:
		if (LOG) logerror("MSM58321 '%s' Reference Signal\n", device->tag);
		break;

	default:
		if (LOG) logerror("MSM58321 '%s' Register %01x = %01x\n", device->tag, offset, data & 0x0f);
		msm58321->reg[offset] = data & 0x0f;
		break;
	}
}

WRITE_LINE_DEVICE_HANDLER( msm58321_stop_w )
{
//	msm58321_t *msm58321 = get_safe_token(device);

	if (LOG) logerror("MSM58321 '%s' STOP: %u\n", device->tag, state);
}

WRITE_LINE_DEVICE_HANDLER( msm58321_test_w )
{
//	msm58321_t *msm58321 = get_safe_token(device);

	if (LOG) logerror("MSM58321 '%s' TEST: %u\n", device->tag, state);
}

/*-------------------------------------------------
    TIMER_CALLBACK( busy_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( busy_tick )
{
	const device_config *device = ptr;
	msm58321_t *msm58321 = get_safe_token(device);

	devcb_call_write_line(&msm58321->out_busy_func, msm58321->busy);

	msm58321->busy = !msm58321->busy;
}

/*-------------------------------------------------
    DEVICE_START( msm58321 )
-------------------------------------------------*/

static DEVICE_START( msm58321 )
{
	msm58321_t *msm58321 = get_safe_token(device);
	const msm58321_interface *intf = get_interface(device);

	/* resolve callbacks */
	devcb_resolve_write_line(&msm58321->out_busy_func, &intf->out_busy_func, device);

	/* create busy timer */
	msm58321->busy_timer = timer_alloc(device->machine, busy_tick, (void *)device);
	timer_adjust_periodic(msm58321->busy_timer, attotime_zero, 0, ATTOTIME_IN_HZ(2));

	/* register for state saving */
//	state_save_register_device_item(device, 0, msm58321->);
}

/*-------------------------------------------------
    DEVICE_GET_INFO( msm58321 )
-------------------------------------------------*/

DEVICE_GET_INFO( msm58321 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(msm58321_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(msm58321);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/* Nothing */								break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "OKI MSM58321");			break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "OKI MSM58321");			break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");		break;
	}
}
