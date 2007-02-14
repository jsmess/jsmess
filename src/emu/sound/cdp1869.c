/***************************************************************************

    cdp1869.c

    TODO: white noise generator

    Sound handler

****************************************************************************/

#include "driver.h"
#include "sound/cdp1869.h"
#include "streams.h"

struct CDP1869
{
	sound_stream *stream;	/* returned by stream_create() */
	int basefreq;			/* chip's base frequency */

	int incr;				/* initial wave state */
	INT16 signal;			/* current signal */

	int toneoff;
	int wnoff;

	UINT8 tonediv;
	UINT8 tonefreq;
	UINT8 toneamp;

	UINT8 wnfreq;
	UINT8 wnamp;
};

/*************************************
 *
 *  Stream updater
 *
 *************************************/

static void cdp1869_update(void *param, stream_sample_t **inputs, stream_sample_t **_buffer, int length)
{
	struct CDP1869 *info = (struct CDP1869 *) param;
	stream_sample_t *buffer = _buffer[0];
	INT16 signal = info->signal;
	int rate = Machine->sample_rate / 2;

	/* get progress through wave */
	int incr = info->incr;

	int frequency = info->basefreq / (512 >> info->tonefreq) / (info->tonediv + 1);

	/* if we're not enabled, just fill with 0 */
	if ( info->toneoff )
	{
		memset( buffer, 0, length * sizeof(*buffer) );
	}
	else
	{
		while( length-- > 0 )
		{
			*buffer++ = signal;
			incr -= frequency;
			while( incr < 0 )
			{
				incr += rate;
				signal = -signal;
			}
		}

		/* store progress through wave */
		info->incr = incr;
		info->signal = signal;
	}
}

/*************************************
 *
 *  Sound handler start
 *
 *************************************/

static void *cdp1869_start(int sndindex, int clock, const void *config)
{
	struct CDP1869 *info;

	info = auto_malloc(sizeof(*info));
	memset(info, 0, sizeof(*info));

	info->stream = stream_create(0, 2, Machine->sample_rate, info, cdp1869_update );
	info->incr = 0;
	info->signal = 0x07fff;

	info->toneoff = 1;
	info->wnoff = 1;
	info->tonediv = 0;
	info->tonefreq = 0;
	info->toneamp = 0;
	info->wnfreq = 0;
	info->wnamp = 0;

	return info;
}

static void cdp1869_set_volume(int which, int stream, int value)
{
	struct CDP1869 *info = sndti_token(SOUND_CDP1869, which);

	stream_update( info->stream);

	value = 100 * value / 7;

	sndti_set_output_gain(SOUND_CDP1869, which, stream, value );
}

void cdp1869_set_toneamp(int which, int value)
{
	struct CDP1869 *info = sndti_token(SOUND_CDP1869, which);
	info->toneamp = value;

	cdp1869_set_volume(which, 0, 1.666 * value);
}

void cdp1869_set_tonefreq(int which, int value)
{
	struct CDP1869 *info = sndti_token(SOUND_CDP1869, which);
	info->tonefreq = value;
}

void cdp1869_set_toneoff(int which, int value)
{
	struct CDP1869 *info = sndti_token(SOUND_CDP1869, which);
	info->toneoff = value;
}

void cdp1869_set_tonediv(int which, int value)
{
	struct CDP1869 *info = sndti_token(SOUND_CDP1869, which);
	info->tonediv = value;
}

void cdp1869_set_wnamp(int which, int value)
{
	struct CDP1869 *info = sndti_token(SOUND_CDP1869, which);
	info->wnamp = value;
	cdp1869_set_volume(which, 1, 1.666 * value);
}

void cdp1869_set_wnfreq(int which, int value)
{
	struct CDP1869 *info = sndti_token(SOUND_CDP1869, which);
	info->wnfreq = value;
}

void cdp1869_set_wnoff(int which, int value)
{
	struct CDP1869 *info = sndti_token(SOUND_CDP1869, which);
	info->wnoff = value;
}

/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void cdp1869_set_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}

void cdp1869_get_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = cdp1869_set_info;		break;
		case SNDINFO_PTR_START:							info->start = cdp1869_start;			break;
		case SNDINFO_PTR_STOP:							/* nothing */							break;
		case SNDINFO_PTR_RESET:							/* nothing */							break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "CDP1869";					break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "RCA CDP1869";				break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2006, The MAME Team"; break;
	}
}
