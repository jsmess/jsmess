#include "sndintrf.h"
#include "streams.h"
#include "flt_rc.h"
#include <math.h>


struct filter_rc_info
{
	sound_stream *	stream;
	int				k;
	int				memory;
};



static void filter_rc_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
	stream_sample_t *src = inputs[0];
	stream_sample_t *dst = outputs[0];
	struct filter_rc_info *info = param;
	int memory = info->memory;

	while (samples--)
	{
		*dst++ = *src + ((memory - *src) * info->k) / 0x10000;
		memory = *src++;
	}
	info->memory = memory;
}


static void *filter_rc_start(int sndindex, int clock, const void *config)
{
	struct filter_rc_info *info;

	info = auto_malloc(sizeof(*info));
	memset(info, 0, sizeof(*info));

	info->stream = stream_create(1, 1, Machine->sample_rate, info, filter_rc_update);

	return info;
}


void filter_rc_set_RC(int num, int R1, int R2, int R3, int C)
{
	struct filter_rc_info *info = sndti_token(SOUND_FILTER_RC, num);
	float f_R1,f_R2,f_R3,f_C;
	float Req;

	if(!info)
		return;

	stream_update(info->stream);

	if (C == 0)
	{
		/* filter disabled */
		info->k = 0;
		return;
	}

	f_R1 = R1;
	f_R2 = R2;
	f_R3 = R3;
	f_C = (float)C * 1E-12;	/* convert pF to F */

	/* Cut Frequency = 1/(2*Pi*Req*C) */

	Req = (f_R1 * (f_R2 + f_R3)) / (f_R1 + f_R2 + f_R3);

	/* k = (1-(EXP(-TIMEDELTA/RC)))    */
	info->k = 0x10000 * (1 - (exp(-1 / (Req * f_C) / Machine->sample_rate)));
}



/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void filter_rc_set_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void filter_rc_get_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = filter_rc_set_info;	break;
		case SNDINFO_PTR_START:							info->start = filter_rc_start;			break;
		case SNDINFO_PTR_STOP:							/* Nothing */							break;
		case SNDINFO_PTR_RESET:							/* Nothing */							break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "RC Filter";					break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "Filters";					break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2004, The MAME Team"; break;
	}
}

