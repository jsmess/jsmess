/***************************************************************************

    DMA-driven DAC driver
    by Aaron Giles

***************************************************************************/

#include "driver.h"
#include "sndintrf.h"
#include "streams.h"
#include "dmadac.h"



/*************************************
 *
 *  Debugging
 *
 *************************************/

#define VERBOSE		0

#if VERBOSE
#define LOG(x) logerror x
#else
#define LOG(x)
#endif



/*************************************
 *
 *  Constants
 *
 *************************************/

#define DEFAULT_SAMPLE_RATE			(44100 * 4)

#define FRAC_BITS					24
#define FRAC_ONE					(1 << FRAC_BITS)
#define FRAC_INT(a)					((a) >> FRAC_BITS)
#define FRAC_FRAC(a)				((a) & (FRAC_ONE - 1))

#define BUFFER_SIZE					32768
#define SECONDS_BETWEEN_ADJUSTS		1
#define ADJUSTS_TO_QUIESCE			1



/*************************************
 *
 *  Types
 *
 *************************************/

struct dmadac_channel_data
{
	/* sound stream and buffers */
	sound_stream *channel;
	UINT32	sample_rate;
	INT16 *	buffer;
	INT16	last;

	/* per-channel parameters */
	INT16	volume;
	UINT8	enabled;
	double	frequency;

	/* info for sample rate conversion */
	UINT32	step;
	UINT32	curoutfrac;
	UINT32	curoutpos;
	UINT32	curinpos;

	/* info for tracking against the system sample rate */
	UINT32	outsamples;
	UINT32	shortages;
	UINT32	overruns;
};



/*************************************
 *
 *  Internal globals
 *
 *************************************/

static double freqmult;
static UINT32 freqmult_quiece_time;
static UINT32 consecutive_shortages;
static UINT32 consecutive_overruns;



/*************************************
 *
 *  Step value computation
 *
 *************************************/

INLINE void compute_step(struct dmadac_channel_data *ch)
{
	ch->step = (UINT32)(((ch->frequency * freqmult) / (double)ch->sample_rate) * (double)FRAC_ONE);
}



/*************************************
 *
 *  Periodically adjust the effective
 *  frequency
 *
 *************************************/

static void adjust_freqmult(void)
{
	int shortages = 0;
	int overruns = 0;
	int i;

	/* first, sum up the data for all channels */
	for (i = 0; i < MAX_SOUND; i++)
	{
		struct dmadac_channel_data *info;

		/* stop when we run out of instances */
		if (!sndti_exists(SOUND_DMADAC, i))
			break;
		info = sndti_token(SOUND_DMADAC, i);
		assert(info != NULL);
		if (info->outsamples)
		{
			shortages += info->shortages;
			overruns += info->overruns;
			info->shortages = 0;
			info->overruns = 0;
			info->outsamples -= info->sample_rate * SECONDS_BETWEEN_ADJUSTS;
		}
	}

	/* don't do anything if we're quiescing */
	if (freqmult_quiece_time)
	{
		freqmult_quiece_time--;
		return;
	}

	/* if we've been short, we need to reduce the effective sample rate so that we stop running out */
	if (shortages)
	{
		consecutive_shortages++;
		freqmult -= 0.00001 * consecutive_shortages;
		LOG(("adjust_freqmult: %d shortages, %d consecutive, decrementing freqmult to %f\n", shortages, consecutive_shortages, freqmult));
		freqmult_quiece_time = ADJUSTS_TO_QUIESCE;
	}
	else
		consecutive_shortages = 0;

	/* if we've been over, we need to increase the effective sample rate so that we consume the data faster */
	if (overruns)
	{
		consecutive_overruns++;
		freqmult += 0.00001 * consecutive_overruns;
		LOG(("adjust_freqmult: %d overruns, %d consecutive, incrementing freqmult to %f\n", overruns, consecutive_overruns, freqmult));
		freqmult_quiece_time = ADJUSTS_TO_QUIESCE;
	}
	else
		consecutive_overruns = 0;

	/* now recompute the step value for each channel */
	for (i = 0; i < MAX_SOUND; i++)
	{
		struct dmadac_channel_data *info;

		/* stop when we run out of instances */
		if (!sndti_exists(SOUND_DMADAC, i))
			break;
		info = sndti_token(SOUND_DMADAC, i);
		if (!info)
			break;
		compute_step(info);
	}
}



/*************************************
 *
 *  Stream callback
 *
 *************************************/

static void dmadac_update(void *param, stream_sample_t **inputs, stream_sample_t **_buffer, int length)
{
	struct dmadac_channel_data *ch = param;
	UINT32 frac = ch->curoutfrac;
	UINT32 out = ch->curoutpos;
	UINT32 step = ch->step;
	INT16 last = ch->last;
	stream_sample_t *buffer = _buffer[0];

	/* track how many samples we've been asked to output; every second, consider adjusting */
	ch->outsamples += length;
	if (ch->outsamples >= ch->sample_rate * SECONDS_BETWEEN_ADJUSTS)
		adjust_freqmult();

	/* if we're not enabled, just fill with silence */
	if (!ch->enabled)
	{
		while (length-- > 0)
			*buffer++ = 0;
		return;
	}

	LOG(("dmadac_update(%d) - %d to consume, %d effective, %d in buffer, ", num, length, (int)(length * ch->frequency / (double)ch->sample_rate), ch->curinpos - ch->curoutpos));

	/* fill with data while we can */
	while (length > 0 && out < ch->curinpos)
	{
		INT32 tmult = frac >> (FRAC_BITS - 8);
		last = ((ch->buffer[(out - 1) % BUFFER_SIZE] * (0x100 - tmult)) + (ch->buffer[out % BUFFER_SIZE] * tmult)) >> 8;
		*buffer++ = (last * ch->volume) >> 8;
		frac += step;
		out += FRAC_INT(frac);
		frac = FRAC_FRAC(frac);
		length--;
	}

	/* trail the last byte if we run low */
	if (length > 0)
	{
		LOG(("dmadac_update: short by %d samples\n", length));
		ch->shortages++;
	}
	while (length-- > 0)
		*buffer++ = (last * ch->volume) >> 8;

	/* update the values */
	ch->curoutfrac = frac;
	ch->curoutpos = out;
	ch->last = last;

	/* adjust for wrapping */
	while (ch->curoutpos > BUFFER_SIZE && ch->curinpos > BUFFER_SIZE)
	{
		ch->curoutpos -= BUFFER_SIZE;
		ch->curinpos -= BUFFER_SIZE;
	}
	LOG(("%d left\n", ch->curinpos - ch->curoutpos));
}



/*************************************
 *
 *  Sound hardware init
 *
 *************************************/

static void *dmadac_start(int sndindex, int clock, const void *config)
{
	struct dmadac_channel_data *info;

	info = auto_malloc(sizeof(*info));
	memset(info, 0, sizeof(*info));

	/* init globals */
	freqmult = 1.0;
	freqmult_quiece_time = 0;
	consecutive_shortages = 0;
	consecutive_overruns = 0;

	/* allocate a clear a buffer */
	info->buffer = auto_malloc(sizeof(info->buffer[0]) * BUFFER_SIZE);
	memset(info->buffer, 0, sizeof(info->buffer[0]) * BUFFER_SIZE);

	/* reset the state */
	info->last = 0;
	info->volume = 0x100;
	info->enabled = 0;

	/* reset the framing */
	info->step = 0;
	info->curoutfrac = 0;
	info->curoutpos = 0;
	info->curinpos = 0;

	/* allocate a stream channel */
	info->sample_rate = DEFAULT_SAMPLE_RATE;
	info->channel = stream_create(0, 1, info->sample_rate, info, dmadac_update);

	return info;
}



/*************************************
 *
 *  Primary transfer routine
 *
 *************************************/

void dmadac_transfer(UINT8 first_channel, UINT8 num_channels, offs_t channel_spacing, offs_t frame_spacing, offs_t total_frames, INT16 *data)
{
	int i, j;

	/* flush out as much data as we can */
	for (i = 0; i < num_channels; i++)
	{
		struct dmadac_channel_data *info = sndti_token(SOUND_DMADAC, first_channel + i);
		stream_update(info->channel);
	}

	/* loop over all channels and accumulate the data */
	for (i = 0; i < num_channels; i++)
	{
		struct dmadac_channel_data *ch = sndti_token(SOUND_DMADAC, first_channel + i);
		if (ch->enabled)
		{
			INT16 *src = data + i * channel_spacing;
			offs_t frames_to_copy = total_frames;
			offs_t in = ch->curinpos;
			offs_t space_in_buf = (BUFFER_SIZE - 1) - (in - ch->curoutpos);

			/* are we overrunning? */
			if (in - ch->curoutpos > total_frames)
				ch->overruns++;

			/* if we want to copy in too much data, clamp to the maximum that will fit */
			if (space_in_buf < frames_to_copy)
			{
				LOG(("dmadac_transfer: attempted %d frames to channel %d, but only %d fit\n", frames_to_copy, first_channel + i, space_in_buf));
				frames_to_copy = space_in_buf;
			}

			/* copy the data */
			for (j = 0; j < frames_to_copy; j++)
			{
				ch->buffer[in++ % BUFFER_SIZE] = *src;
				src += frame_spacing;
			}
			ch->curinpos = in;
		}
	}

	LOG(("dmadac_transfer - %d samples, %d effective, %d in buffer\n", total_frames, (int)(total_frames * (double)DEFAULT_SAMPLE_RATE / dmadac[first_channel].frequency), dmadac[first_channel].curinpos - dmadac[first_channel].curoutpos));
}



/*************************************
 *
 *  Enable/disable DMA channel(s)
 *
 *************************************/

void dmadac_enable(UINT8 first_channel, UINT8 num_channels, UINT8 enable)
{
	int i;

	/* flush out as much data as we can */
	for (i = 0; i < num_channels; i++)
	{
		struct dmadac_channel_data *info = sndti_token(SOUND_DMADAC, first_channel + i);
		stream_update(info->channel);
		info->enabled = enable;
		if (!enable)
			info->curinpos = info->curoutpos = info->curoutfrac = 0;
	}
}



/*************************************
 *
 *  Set the frequency on DMA channel(s)
 *
 *************************************/

void dmadac_set_frequency(UINT8 first_channel, UINT8 num_channels, double frequency)
{
	int i;

	/* flush out as much data as we can */
	for (i = 0; i < num_channels; i++)
	{
		struct dmadac_channel_data *info = sndti_token(SOUND_DMADAC, first_channel + i);
		stream_update(info->channel);
		info->frequency = frequency;
		compute_step(info);
	}
}



/*************************************
 *
 *  Set the volume on DMA channel(s)
 *
 *************************************/

void dmadac_set_volume(UINT8 first_channel, UINT8 num_channels, UINT16 volume)
{
	int i;

	/* flush out as much data as we can */
	for (i = 0; i < num_channels; i++)
	{
		struct dmadac_channel_data *info = sndti_token(SOUND_DMADAC, first_channel + i);
		stream_update(info->channel);
		info->volume = volume;
	}
}



/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void dmadac_set_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void dmadac_get_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = dmadac_set_info;		break;
		case SNDINFO_PTR_START:							info->start = dmadac_start;				break;
		case SNDINFO_PTR_STOP:							/* nothing */							break;
		case SNDINFO_PTR_RESET:							/* nothing */							break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "DMA-driven DAC";				break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "DAC";						break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2004, The MAME Team"; break;
	}
}

