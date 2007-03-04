/*********************************************************************

	bitbngr.c

	TRS style "bitbanger" serial port

*********************************************************************/

#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include "mame.h"
#include "devices/bitbngr.h"
#include "devices/printer.h"
#include "mess.h"

static void bitbanger_overthreshhold(int id);

struct bitbanger_info
{
	const struct bitbanger_config *config;
	double last_pulse_time;
	double *pulses;
	int *factored_pulses;
	int recorded_pulses;
	int value;
	void *timeout_timer;
	int over_threshhold;
};

static struct bitbanger_info *bitbangers[MAX_PRINTER];

static int bitbanger_init(mess_image *img)
{
	int id = image_index_in_device(img);
	struct bitbanger_info *bi;
	const struct bitbanger_config *config;
	const struct IODevice *dev;

	dev = image_device(img);
	config = (const struct bitbanger_config *) device_get_info_ptr(&dev->devclass, DEVINFO_PTR_BITBANGER_CONFIG);

	bi = (struct bitbanger_info *) auto_malloc(sizeof(struct bitbanger_info));
	bi->pulses = (double *) auto_malloc(config->maximum_pulses * sizeof(double));
	bi->factored_pulses = (int *) auto_malloc(config->maximum_pulses * sizeof(int));
	bi->config = config;
	bi->last_pulse_time = 0.0;
	bi->recorded_pulses = 0;
	bi->value = config->initial_value;
	bi->timeout_timer = timer_alloc(bitbanger_overthreshhold);
	bi->over_threshhold = 1;

	bitbangers[id] = bi;
	return INIT_PASS;
}



static void bitbanger_analyze(int id, struct bitbanger_info *bi)
{
	int i, factor, total_duration;
	double smallest_pulse;
	double d;

	/* compute the smallest pulse */
	smallest_pulse = bi->config->pulse_threshhold;
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
			d = bi->pulses[i] / smallest_pulse * factor + bi->config->pulse_tolerance;
			if ((i < (bi->recorded_pulses-1)) && (d - floor(d)) >= (bi->config->pulse_tolerance * 2))
			{
				i = -1;
				break;
			}
			bi->factored_pulses[i] = (int) d;
			total_duration += (int) d;
		}
	}
	while((i == -1) && (factor < bi->config->maximum_pulses));
	if (i == -1)
		return;

	/* filter the output */
	if (bi->config->filter(image_from_devtype_and_index(IO_BITBANGER, id), bi->factored_pulses, bi->recorded_pulses, total_duration))
		bi->recorded_pulses = 0;
}



static void bitbanger_addpulse(int id, struct bitbanger_info *bi, double pulse_width)
{
	/* exceeded total countable pulses? */
	if (bi->recorded_pulses == bi->config->maximum_pulses)
		memmove(bi->pulses, bi->pulses + 1, (--bi->recorded_pulses) * sizeof(double));

	/* record the pulse */
	bi->pulses[bi->recorded_pulses++] = pulse_width;

	/* analyze, if necessary */
	if (bi->recorded_pulses >= bi->config->minimum_pulses)
		bitbanger_analyze(id, bi);
}



static void bitbanger_overthreshhold(int id)
{
	struct bitbanger_info *bi = bitbangers[id];
	bitbanger_addpulse(id, bi, timer_get_time() - bi->last_pulse_time);
	bi->over_threshhold = 1;
	bi->recorded_pulses = 0;
}



void bitbanger_output(mess_image *img, int value)
{
	int id = image_index_in_device(img);
	struct bitbanger_info *bi = bitbangers[id];
	double current_time;
	double pulse_width;

	/* normalize input */
	value = value ? 1 : 0;

	/* only meaningful if we change */
	if (bi->value != value)
	{
		current_time = timer_get_time();
		pulse_width = current_time - bi->last_pulse_time;

		assert(pulse_width >= 0);

		if (!bi->over_threshhold && ((bi->recorded_pulses > 0) || (bi->value == bi->config->initial_value)))
			bitbanger_addpulse(id, bi, pulse_width);

		/* update state */
		bi->value = value;
		bi->last_pulse_time = current_time;
		bi->over_threshhold = 0;

		/* update timeout timer */
		timer_reset(bi->timeout_timer, bi->config->pulse_threshhold);
	}
}



void bitbanger_device_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:						info->i = IO_BITBANGER; break;
		case DEVINFO_INT_READABLE:					info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:					info->i = 1; break;
		case DEVINFO_INT_CREATABLE:					info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_INIT:						info->init = bitbanger_init; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_DEV_FILE:					strcpy(info->s = device_temp_str(), __FILE__); break;
		case DEVINFO_STR_FILE_EXTENSIONS:			strcpy(info->s = device_temp_str(), "prn"); break;
	}
}
