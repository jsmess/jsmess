/**********************************************************************

    Ricoh RP5C01A Real Time Clock With Internal RAM emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

/*

    TODO:

    - divide frequencies from device clock
    - mask out unused bits in register writes
    - clock adjust
    - 12 hour clock
    - leap year counter
    - timer reset

*/

#include "emu.h"
#include "rp5c01a.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

enum
{
	RP5C01A_REGISTER_1_SECOND = 0,
	RP5C01A_REGISTER_10_SECOND,
	RP5C01A_REGISTER_1_MINUTE,
	RP5C01A_REGISTER_10_MINUTE,
	RP5C01A_REGISTER_1_HOUR,
	RP5C01A_REGISTER_10_HOUR,
	RP5C01A_REGISTER_DAY_OF_THE_WEEK,
	RP5C01A_REGISTER_1_DAY,
	RP5C01A_REGISTER_10_DAY,
	RP5C01A_REGISTER_1_MONTH,
	RP5C01A_REGISTER_10_MONTH, RP5C01A_REGISTER_12_24_SELECT = RP5C01A_REGISTER_10_MONTH,
	RP5C01A_REGISTER_1_YEAR, RP5C01A_REGISTER_LEAP_YEAR = RP5C01A_REGISTER_1_YEAR,
	RP5C01A_REGISTER_10_YEAR,
	RP5C01A_REGISTER_MODE,
	RP5C01A_REGISTER_TEST,
	RP5C01A_REGISTER_RESET
};

#define RP5C01A_MODE_TIMER_EN		0x08
#define RP5C01A_MODE_ALARM_EN		0x04
#define RP5C01A_MODE_MASK			0x03
#define RP5C01A_MODE_RAM_BLOCK11	0x03
#define RP5C01A_MODE_RAM_BLOCK10	0x02
#define RP5C01A_MODE_ALARM			0x01
#define RP5C01A_MODE_TIME			0x00

#define RP5C01A_TEST_4				0x08
#define RP5C01A_TEST_3				0x04
#define RP5C01A_TEST_2				0x02
#define RP5C01A_TEST_1				0x01

#define RP5C01A_RESET_ALARM			0x08
#define RP5C01A_RESET_TIMER			0x04
#define RP5C01A_RESET_16_HZ			0x02
#define RP5C01A_RESET_1_HZ			0x01

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _rp5c01a_t rp5c01a_t;
struct _rp5c01a_t
{
	devcb_resolved_write_line	out_alarm_func;

	UINT8 reg[4][13];			/* clock registers */

	UINT8 mode;					/* mode register */

	/* timers */
	emu_timer *clock_timer;
	emu_timer *alarm_timer;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE rp5c01a_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == RP5C01A);
	return (rp5c01a_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE const rp5c01a_interface *get_interface(device_t *device)
{
	assert(device != NULL);
	assert((device->type() == RP5C01A));
	return (const rp5c01a_interface *) device->baseconfig().static_config();
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    rp5c01a_adj_w - clock adjust write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( rp5c01a_adj_w )
{
//  rp5c01a_t *rp5c01a = get_safe_token(device);

}

/*-------------------------------------------------
    rp5c01a_r - register read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( rp5c01a_r )
{
	rp5c01a_t *rp5c01a = get_safe_token(device);

	UINT8 data = 0;

	switch (offset & 0x0f)
	{
	case RP5C01A_REGISTER_MODE:
		data = rp5c01a->mode;
		break;

	case RP5C01A_REGISTER_TEST:
	case RP5C01A_REGISTER_RESET:
		/* write only */
		break;

	default:
		data = rp5c01a->reg[rp5c01a->mode & RP5C01A_MODE_MASK][offset];
		break;
	}

	return data;
}

/*-------------------------------------------------
    rp5c01a_w - register write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( rp5c01a_w )
{
	rp5c01a_t *rp5c01a = get_safe_token(device);

	int mode = rp5c01a->mode & RP5C01A_MODE_MASK;

	switch (offset & 0x0f)
	{
	case RP5C01A_REGISTER_MODE:
		rp5c01a->mode = data;

		rp5c01a->clock_timer->enable(data & RP5C01A_MODE_TIMER_EN);

		if (LOG)
		{
			logerror("RP5C01A '%s' Mode %u\n", device->tag(), data & RP5C01A_MODE_MASK);
			logerror("RP5C01A '%s' Timer %s\n", device->tag(), (data & RP5C01A_MODE_TIMER_EN ) ? "enabled" : "disabled");
			logerror("RP5C01A '%s' Alarm %s\n", device->tag(), (data & RP5C01A_MODE_ALARM_EN ) ? "enabled" : "disabled");
		}
		break;

	case RP5C01A_REGISTER_TEST:
		if (LOG) logerror("RP5C01A '%s' Test %u not supported!\n", device->tag(), data);
		break;

	case RP5C01A_REGISTER_RESET:
		if (data & RP5C01A_RESET_ALARM)
		{
			int i;

			/* reset alarm registers */
			for (i = RP5C01A_REGISTER_1_MINUTE; i < RP5C01A_REGISTER_1_MONTH; i++)
			{
				rp5c01a->reg[RP5C01A_MODE_ALARM][i] = 0;
			}
		}

		rp5c01a->alarm_timer->enable(!(data & RP5C01A_RESET_16_HZ));

		if (LOG)
		{
			if (data & RP5C01A_RESET_ALARM) logerror("RP5C01A '%s' Alarm Reset\n", device->tag());
			if (data & RP5C01A_RESET_TIMER) logerror("RP5C01A '%s' Timer Reset not supported!\n", device->tag());
			logerror("RP5C01A '%s' 16Hz Signal %s\n", device->tag(), (data & RP5C01A_RESET_16_HZ) ? "disabled" : "enabled");
			logerror("RP5C01A '%s' 1Hz Signal %s\n", device->tag(), (data & RP5C01A_RESET_1_HZ) ? "disabled" : "enabled");
		}
		break;

	default:
		rp5c01a->reg[mode][offset & 0x0f] = data;

		if (LOG) logerror("RP5C01A '%s' Register %u Write %02x\n", device->tag(), offset & 0x0f, data);
		break;
	}
}

/*-------------------------------------------------
    advance_seconds - advance seconds counter
-------------------------------------------------*/

static void advance_seconds(rp5c01a_t *rp5c01a)
{
	static const int days_per_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	int seconds = (rp5c01a->reg[RP5C01A_MODE_TIME][RP5C01A_REGISTER_10_SECOND] * 10) + rp5c01a->reg[RP5C01A_MODE_TIME][RP5C01A_REGISTER_1_SECOND];
	int minutes = (rp5c01a->reg[RP5C01A_MODE_TIME][RP5C01A_REGISTER_10_MINUTE] * 10) + rp5c01a->reg[RP5C01A_MODE_TIME][RP5C01A_REGISTER_1_MINUTE];
	int hours = (rp5c01a->reg[RP5C01A_MODE_TIME][RP5C01A_REGISTER_10_HOUR] * 10) + rp5c01a->reg[RP5C01A_MODE_TIME][RP5C01A_REGISTER_1_HOUR];
	int days = (rp5c01a->reg[RP5C01A_MODE_TIME][RP5C01A_REGISTER_10_DAY] * 10) + rp5c01a->reg[RP5C01A_MODE_TIME][RP5C01A_REGISTER_1_DAY];
	int month = (rp5c01a->reg[RP5C01A_MODE_TIME][RP5C01A_REGISTER_10_MONTH] * 10) + rp5c01a->reg[RP5C01A_MODE_TIME][RP5C01A_REGISTER_1_MONTH];
	int year = (rp5c01a->reg[RP5C01A_MODE_TIME][RP5C01A_REGISTER_10_YEAR] * 10) + rp5c01a->reg[RP5C01A_MODE_TIME][RP5C01A_REGISTER_1_YEAR];
	int day_of_week = rp5c01a->reg[RP5C01A_MODE_TIME][RP5C01A_REGISTER_DAY_OF_THE_WEEK];
	int i;

	seconds++;

	if (seconds > 59)
	{
		seconds = 0;
		minutes++;
	}

	if (minutes > 59)
	{
		minutes = 0;
		hours++;
	}

	if (hours > 23)
	{
		hours = 0;
		days++;
		day_of_week++;
	}

	if (day_of_week > 6)
	{
		day_of_week++;
	}

	if (days > days_per_month[month - 1])
	{
		days = 1;
		month++;
	}

	if (month > 12)
	{
		month = 1;
		year++;
	}

	if (year > 99)
	{
		year = 0;
	}

	rp5c01a->reg[RP5C01A_MODE_TIME][RP5C01A_REGISTER_1_SECOND] = seconds % 10;
	rp5c01a->reg[RP5C01A_MODE_TIME][RP5C01A_REGISTER_10_SECOND] = seconds / 10;
	rp5c01a->reg[RP5C01A_MODE_TIME][RP5C01A_REGISTER_1_MINUTE] = minutes % 10;
	rp5c01a->reg[RP5C01A_MODE_TIME][RP5C01A_REGISTER_10_MINUTE] = minutes / 10;
	rp5c01a->reg[RP5C01A_MODE_TIME][RP5C01A_REGISTER_1_HOUR] = hours % 10;
	rp5c01a->reg[RP5C01A_MODE_TIME][RP5C01A_REGISTER_10_HOUR] = hours / 10;
	rp5c01a->reg[RP5C01A_MODE_TIME][RP5C01A_REGISTER_1_DAY] = days % 10;
	rp5c01a->reg[RP5C01A_MODE_TIME][RP5C01A_REGISTER_10_DAY] = days / 10;
	rp5c01a->reg[RP5C01A_MODE_TIME][RP5C01A_REGISTER_1_MONTH] = month % 10;
	rp5c01a->reg[RP5C01A_MODE_TIME][RP5C01A_REGISTER_10_MONTH] = month / 10;
	rp5c01a->reg[RP5C01A_MODE_TIME][RP5C01A_REGISTER_1_YEAR] = year % 10;
	rp5c01a->reg[RP5C01A_MODE_TIME][RP5C01A_REGISTER_10_YEAR] = year / 10;
	rp5c01a->reg[RP5C01A_MODE_TIME][RP5C01A_REGISTER_DAY_OF_THE_WEEK] = day_of_week;

	if (!(rp5c01a->mode & RP5C01A_RESET_1_HZ))
	{
		/* pulse 1Hz alarm signal */
		devcb_call_write_line(&rp5c01a->out_alarm_func, 0);
		devcb_call_write_line(&rp5c01a->out_alarm_func, 1);
	}
	else if (rp5c01a->mode & RP5C01A_MODE_ALARM_EN)
	{
		for (i = RP5C01A_REGISTER_1_MINUTE; i < RP5C01A_REGISTER_1_MONTH; i++)
		{
			if (rp5c01a->reg[RP5C01A_MODE_ALARM][i] != rp5c01a->reg[RP5C01A_MODE_TIME][i]) return;
		}

		/* trigger alarm */
		devcb_call_write_line(&rp5c01a->out_alarm_func, 0);
		devcb_call_write_line(&rp5c01a->out_alarm_func, 1);
	}
}

/*-------------------------------------------------
    TIMER_CALLBACK( clock_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( clock_tick )
{
	device_t *device = (device_t *)ptr;
	rp5c01a_t *rp5c01a = get_safe_token(device);

	advance_seconds(rp5c01a);
}

/*-------------------------------------------------
    TIMER_CALLBACK( alarm_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( alarm_tick )
{
	device_t *device = (device_t *)ptr;
	rp5c01a_t *rp5c01a = get_safe_token(device);

	devcb_call_write_line(&rp5c01a->out_alarm_func, 0);
	devcb_call_write_line(&rp5c01a->out_alarm_func, 1);
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
	rp5c01a->clock_timer = device->machine().scheduler().timer_alloc(FUNC(clock_tick), (void *)device);
	rp5c01a->clock_timer->adjust(attotime::zero, 0, attotime::from_hz(1));

	rp5c01a->alarm_timer = device->machine().scheduler().timer_alloc(FUNC(alarm_tick), (void *)device);
	rp5c01a->alarm_timer->adjust(attotime::zero, 0, attotime::from_hz(16));
	rp5c01a->alarm_timer->enable(0);

	/* register for state saving */
    state_save_register_global(device->machine(), rp5c01a->mode);
    state_save_register_global_array(device->machine(), rp5c01a->reg[0]);
    state_save_register_global_array(device->machine(), rp5c01a->reg[1]);
    state_save_register_global_array(device->machine(), rp5c01a->reg[2]);
    state_save_register_global_array(device->machine(), rp5c01a->reg[3]);
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

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(rp5c01a);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/* Nothing */								break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Ricoh RP5C01A");			break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Ricoh RP5C01A");			break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						/* Nothing */								break;
	}
}

DEFINE_LEGACY_DEVICE(RP5C01A, rp5c01a);
