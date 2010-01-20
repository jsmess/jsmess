/*********************************************************************

    bitbngr.c

    TRS style "bitbanger" serial port

*********************************************************************/

#include <math.h>

#include "emu.h"
#include "bitbngr.h"
#include "printer.h"



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _bitbanger_token bitbanger_token;
struct _bitbanger_token
{
	double last_pulse_time;
	double *pulses;
	int *factored_pulses;
	int recorded_pulses;
	int value;
	emu_timer *timeout_timer;
	int over_threshhold;
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static TIMER_CALLBACK(bitbanger_overthreshhold);



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    get_token - safely gets the bitbanger data
-------------------------------------------------*/

INLINE bitbanger_token *get_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == BITBANGER);
	return (bitbanger_token *) device->token;
}



/*-------------------------------------------------
    get_config - safely gets the bitbanger config
-------------------------------------------------*/

INLINE const bitbanger_config *get_config(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == BITBANGER);
	return (const bitbanger_config *) device->static_config;
}



/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    DEVICE_START(bitbanger)
-------------------------------------------------*/

static DEVICE_START(bitbanger)
{
	bitbanger_token *bi;
	const bitbanger_config *config = get_config(device);

	bi = get_token(device);
	bi->pulses = auto_alloc_array(device->machine, double, config->maximum_pulses);
	bi->factored_pulses = auto_alloc_array(device->machine, int, config->maximum_pulses);
	bi->last_pulse_time = 0.0;
	bi->recorded_pulses = 0;
	bi->value = config->initial_value;
	bi->timeout_timer = timer_alloc(device->machine, bitbanger_overthreshhold, (void *) device);
	bi->over_threshhold = 1;
}



/*-------------------------------------------------
    bitbanger_analyze
-------------------------------------------------*/

static void bitbanger_analyze(const device_config *device)
{
	int i, factor, total_duration;
	double smallest_pulse;
	double d;
	bitbanger_token *bi = get_token(device);
	const bitbanger_config *config = get_config(device);

	/* compute the smallest pulse */
	smallest_pulse = config->pulse_threshhold;
	for (i = 0; i < bi->recorded_pulses; i++)
		if (smallest_pulse > bi->pulses[i])
			smallest_pulse = bi->pulses[i];

	/* figure out what factor the smallest pulse was */
	factor = 0;
	do
	{
		factor++;
		total_duration = 0;

		for (i = 0; i < bi->recorded_pulses; i++)
		{
			d = bi->pulses[i] / smallest_pulse * factor + config->pulse_tolerance;
			if ((i < (bi->recorded_pulses-1)) && (d - floor(d)) >= (config->pulse_tolerance * 2))
			{
				i = -1;
				break;
			}
			bi->factored_pulses[i] = (int) d;
			total_duration += (int) d;
		}
	}
	while((i == -1) && (factor < config->maximum_pulses));
	if (i == -1)
		return;

	/* filter the output */
	if (config->filter(device, bi->factored_pulses, bi->recorded_pulses, total_duration))
		bi->recorded_pulses = 0;
}



/*-------------------------------------------------
    bitbanger_addpulse
-------------------------------------------------*/

static void bitbanger_addpulse(const device_config *device, double pulse_width)
{
	bitbanger_token *bi = get_token(device);
	const bitbanger_config *config = get_config(device);

	/* exceeded total countable pulses? */
	if (bi->recorded_pulses == config->maximum_pulses)
		memmove(bi->pulses, bi->pulses + 1, (--bi->recorded_pulses) * sizeof(double));

	/* record the pulse */
	bi->pulses[bi->recorded_pulses++] = pulse_width;

	/* analyze, if necessary */
	if (bi->recorded_pulses >= config->minimum_pulses)
		bitbanger_analyze(device);
}



/*-------------------------------------------------
    TIMER_CALLBACK(bitbanger_overthreshhold)
-------------------------------------------------*/

static TIMER_CALLBACK(bitbanger_overthreshhold)
{
	const device_config *device = (const device_config *) ptr;
	bitbanger_token *bi = get_token(device);

	bitbanger_addpulse(device, attotime_to_double(timer_get_time(machine)) - bi->last_pulse_time);
	bi->over_threshhold = 1;
	bi->recorded_pulses = 0;
}



/*-------------------------------------------------
    bitbanger_output - outputs data to a bitbanger
    port
-------------------------------------------------*/

void bitbanger_output(const device_config *device, int value)
{
	bitbanger_token *bi = get_token(device);
	const bitbanger_config *config = get_config(device);

	double current_time;
	double pulse_width;

	/* normalize input */
	value = value ? 1 : 0;

	/* only meaningful if we change */
	if (bi->value != value)
	{
		current_time = attotime_to_double(timer_get_time(device->machine));
		pulse_width = current_time - bi->last_pulse_time;

		assert(pulse_width >= 0);

		if (!bi->over_threshhold && ((bi->recorded_pulses > 0) || (bi->value == config->initial_value)))
			bitbanger_addpulse(device, pulse_width);

		/* update state */
		bi->value = value;
		bi->last_pulse_time = current_time;
		bi->over_threshhold = 0;

		/* update timeout timer */
		timer_reset(bi->timeout_timer, double_to_attotime(config->pulse_threshhold));
	}
}



/*-------------------------------------------------
    DEVICE_GET_INFO(bitbanger) - device getinfo
    function
-------------------------------------------------*/

DEVICE_GET_INFO(bitbanger)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(bitbanger_token); break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0; break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL; break;
		case DEVINFO_INT_IMAGE_TYPE:					info->i = IO_PRINTER; break;
		case DEVINFO_INT_IMAGE_READABLE:				info->i = 0; break;
		case DEVINFO_INT_IMAGE_WRITEABLE:				info->i = 1; break;
		case DEVINFO_INT_IMAGE_CREATABLE:				info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(bitbanger); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Bitbanger"); break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Bitbanger"); break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__); break;
		case DEVINFO_STR_IMAGE_FILE_EXTENSIONS:			strcpy(info->s, "prn"); break;
	}
}
