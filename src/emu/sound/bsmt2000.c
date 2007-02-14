/**********************************************************************************************
 *
 *   Data East BSMT2000 driver
 *   by Aaron Giles
 *
 **********************************************************************************************/


#include <math.h>

#include "sndintrf.h"
#include "streams.h"
#include "bsmt2000.h"



/**********************************************************************************************

     CONSTANTS

***********************************************************************************************/

/* NOTE: the original chip did not support interpolation, but it sounds */
/* nicer if you enable it. For accuracy's sake, we leave it off by default. */
#define ENABLE_INTERPOLATION	0

#define LOG_COMMANDS			0
#define MAKE_WAVS				0

#if MAKE_WAVS
#include "wavwrite.h"
#endif


#define MAX_SAMPLE_CHUNK		10000

#define FRAC_BITS				14
#define FRAC_ONE				(1 << FRAC_BITS)
#define FRAC_MASK				(FRAC_ONE - 1)

#define REG_CURRPOS				0
#define REG_UNKNOWN1			1
#define REG_RATE				2
#define REG_LOOPEND				3
#define REG_LOOPSTART			4
#define REG_BANK				5
#define REG_RIGHTVOL			6
#define REG_LEFTVOL				7
#define REG_TOTAL				8

#define REG_ALT_RIGHTVOL		8



/**********************************************************************************************

     INTERNAL DATA STRUCTURES

***********************************************************************************************/

/* struct describing a single playing voice */
typedef struct _bsmt2000_voice bsmt2000_voice;
struct _bsmt2000_voice
{
	/* external state */
	UINT16		reg[REG_TOTAL];			/* 9 registers */
	UINT32		position;				/* current position */
	UINT32		loop_start_position;	/* loop start position */
	UINT32		loop_stop_position;		/* loop stop position */
	UINT32		adjusted_rate;			/* adjusted rate */
};

typedef struct _bsmt2000_chip bsmt2000_chip;
struct _bsmt2000_chip
{
	sound_stream *stream;				/* which stream are we using */
	int			sample_rate;			/* output sample rate */
	INT8 *		region_base;			/* pointer to the base of the region */
	int			total_banks;			/* number of total banks in the region */
	int			voices;					/* number of voices */

	bsmt2000_voice *voice;				/* the voices */
	bsmt2000_voice compressed;			/* the compressed voice */

	INT32 *		scratch;

#if MAKE_WAVS
	void *		wavraw;					/* raw waveform */
#endif
};



/**********************************************************************************************

     interpolate
     backend_interpolate -- interpolate between two samples

***********************************************************************************************/

#define interpolate(sample1, sample2, accum)										\
		(sample1 * (INT32)(0x10000 - (accum & 0xffff)) + 							\
		 sample2 * (INT32)(accum & 0xffff)) >> 16;

#define interpolate2(sample1, sample2, accum)										\
		(sample1 * (INT32)(0x8000 - (accum & 0x7fff)) + 							\
		 sample2 * (INT32)(accum & 0x7fff)) >> 15;



/**********************************************************************************************

     generate_samples -- generate samples for all voices at the chip's frequency

***********************************************************************************************/

static void generate_samples(bsmt2000_chip *chip, INT32 *left, INT32 *right, int samples)
{
	bsmt2000_voice *voice;
	int v;

	/* skip if nothing to do */
	if (!samples)
		return;

	/* clear out the accumulator */
	memset(left, 0, samples * sizeof(left[0]));
	memset(right, 0, samples * sizeof(right[0]));

	/* loop over voices */
	for (v = 0; v < chip->voices; v++)
	{
		voice = &chip->voice[v];

		/* compute the region base */
		if (voice->reg[REG_BANK] < chip->total_banks)
		{
			INT8 *base = &chip->region_base[voice->reg[REG_BANK] * 0x10000];
			INT32 *lbuffer = left, *rbuffer = right;
			UINT32 rate = voice->adjusted_rate;
			UINT32 pos = voice->position;
			INT32 lvol = voice->reg[REG_LEFTVOL];
			INT32 rvol = voice->reg[REG_RIGHTVOL];
			int remaining = samples;

			/* loop while we still have samples to generate */
			while (remaining--)
			{
#if ENABLE_INTERPOLATION
				/* fetch two samples */
				INT32 val1 = base[pos >> 16];
				INT32 val2 = base[(pos >> 16) + 1];

				/* interpolate */
				val1 = interpolate(val1, val2, pos);
#else
				INT32 val1 = base[pos >> 16];
#endif
				pos += rate;

				/* apply volumes and add */
				*lbuffer++ += val1 * lvol;
				*rbuffer++ += val1 * rvol;

				/* check for loop end */
				if (pos >= voice->loop_stop_position)
					pos += voice->loop_start_position - voice->loop_stop_position;
			}

			/* update the position */
			voice->position = pos;
		}
	}

	/* compressed voice (11-voice model only) */
	voice = &chip->compressed;
	if (chip->voices == 11 && voice->reg[REG_BANK] < chip->total_banks)
	{
		INT8 *base = &chip->region_base[voice->reg[REG_BANK] * 0x10000];
		INT32 *lbuffer = left, *rbuffer = right;
		UINT32 rate = voice->adjusted_rate;
		UINT32 pos = voice->position;
		INT32 lvol = voice->reg[REG_LEFTVOL];
		INT32 rvol = voice->reg[REG_RIGHTVOL];
		int remaining = samples;

		/* loop while we still have samples to generate */
		while (remaining-- && pos < voice->loop_stop_position)
		{
#if ENABLE_INTERPOLATION
			/* fetch two samples -- note: this is wrong, just a guess!!!*/
			INT32 val1 = (INT8)((base[pos >> 16] << ((pos >> 13) & 4)) & 0xf0);
			INT32 val2 = (INT8)((base[(pos + 0x8000) >> 16] << (((pos + 0x8000) >> 13) & 4)) & 0xf0);

			/* interpolate */
			val1 = interpolate2(val1, val2, pos);
#else
			INT32 val1 = (INT8)((base[pos >> 16] << ((pos >> 13) & 4)) & 0xf0);
#endif
			pos += rate;

			/* apply volumes and add */
			*lbuffer++ += val1 * lvol;
			*rbuffer++ += val1 * rvol;
		}

		/* update the position */
		voice->position = pos;
	}
}



/**********************************************************************************************

     bsmt2000_update -- update the sound chip so that it is in sync with CPU execution

***********************************************************************************************/

static void bsmt2000_update(void *param, stream_sample_t **inputs, stream_sample_t **buffer, int length)
{
	bsmt2000_chip *chip = param;
	INT32 *lsrc = chip->scratch, *rsrc = chip->scratch;
	stream_sample_t *ldest = buffer[0];
	stream_sample_t *rdest = buffer[1];

#if MAKE_WAVS
	/* start the logging once we have a sample rate */
	if (!chip->wavraw)
		chip->wavraw = wav_open("raw.wav", chip->sample_rate, 2);
#endif

	/* loop until all samples are output */
	while (length)
	{
		int samples = (length > MAX_SAMPLE_CHUNK) ? MAX_SAMPLE_CHUNK : length;
		int samp;

		/* determine left/right source data */
		lsrc = chip->scratch;
		rsrc = chip->scratch + samples;
		generate_samples(chip, lsrc, rsrc, samples);

		/* copy the data */
		for (samp = 0; samp < samples; samp++)
		{
			*ldest++ = lsrc[samp] >> 9;
			*rdest++ = rsrc[samp] >> 9;
		}

#if MAKE_WAVS
		/* log the raw data */
		if (chip->wavraw)
			wav_add_data_32lr(chip->wavraw, lsrc, rsrc, samples, 4);
#endif

		/* account for these samples */
		length -= samples;
	}
}



/**********************************************************************************************

     bsmt2000_start -- start emulation of the BSMT2000

***********************************************************************************************/

INLINE void init_voice(bsmt2000_voice *voice)
{
	memset(&voice->reg, 0, sizeof(voice->reg));
	voice->position = 0;
	voice->adjusted_rate = 0;
	voice->reg[REG_LEFTVOL] = 0x7fff;
	voice->reg[REG_RIGHTVOL] = 0x7fff;
}


INLINE void init_all_voices(bsmt2000_chip *chip)
{
	int i;

	/* init the voices */
	for (i = 0; i < chip->voices; i++)
		init_voice(&chip->voice[i]);

	/* init the compressed voice (runs at a fixed rate of ~8kHz?) */
	init_voice(&chip->compressed);
	chip->compressed.adjusted_rate = 0x02aa << 4;
}


static void register_voice_for_save(bsmt2000_voice *voice, int index)
{
	state_save_register_item_array("bsmt2000", index, voice->reg);
	state_save_register_item("bsmt2000", index, voice->position);
	state_save_register_item("bsmt2000", index, voice->loop_start_position);
	state_save_register_item("bsmt2000", index, voice->loop_stop_position);
	state_save_register_item("bsmt2000", index, voice->adjusted_rate);
}


static void *bsmt2000_start(int sndindex, int clock, const void *config)
{
	const struct BSMT2000interface *intf = config;
	bsmt2000_chip *chip;
	int i;

	chip = auto_malloc(sizeof(*chip));
	memset(chip, 0, sizeof(*chip));

	/* allocate the voices */
	chip->voices = intf->voices;
	chip->voice = auto_malloc(chip->voices * sizeof(bsmt2000_voice));

	/* create the stream */
	chip->sample_rate = clock / 1024;
	chip->stream = stream_create(0, 2, chip->sample_rate, chip, bsmt2000_update);

	/* initialize the regions */
	chip->region_base = (INT8 *)memory_region(intf->region);
	chip->total_banks = memory_region_length(intf->region) / 0x10000;

	/* init the voices */
	init_all_voices(chip);

	/* allocate memory */
	chip->scratch = auto_malloc(sizeof(chip->scratch[0]) * 2 * MAX_SAMPLE_CHUNK);

	/* register for save states */
	for (i = 0; i < chip->voices; i++)
		register_voice_for_save(&chip->voice[i], sndindex * 32 + i);
	register_voice_for_save(&chip->compressed, sndindex * 32 + 31);

	/* success */
	return chip;
}



/**********************************************************************************************

     bsmt2000_reset -- reset emulation of the BSMT2000

***********************************************************************************************/

static void bsmt2000_reset(void *_chip)
{
	bsmt2000_chip *chip = _chip;
	init_all_voices(chip);
}



/**********************************************************************************************

     bsmt2000_reg_write -- handle a write to the selected BSMT2000 register

***********************************************************************************************/

static void bsmt2000_reg_write(bsmt2000_chip *chip, offs_t offset, UINT16 data, UINT16 mem_mask)
{
	bsmt2000_voice *voice = &chip->voice[offset % chip->voices];
	int regindex = offset / chip->voices;

#if LOG_COMMANDS
	logerror("BSMT write: V%d R%d = %04X\n", offset % chip->voices, regindex, data);
#endif

	/* update the register */
	if (regindex < REG_TOTAL)
		COMBINE_DATA(&voice->reg[regindex]);

	/* force an update */
	stream_update(chip->stream);

	/* update parameters for standard voices */
	switch (regindex)
	{
		case REG_CURRPOS:
			voice->position = voice->reg[REG_CURRPOS] << 16;
			break;

		case REG_RATE:
			voice->adjusted_rate = voice->reg[REG_RATE] << 5;
			break;

		case REG_LOOPSTART:
			voice->loop_start_position = voice->reg[REG_LOOPSTART] << 16;
			break;

		case REG_LOOPEND:
			voice->loop_stop_position = voice->reg[REG_LOOPEND] << 16;
			break;

		case REG_ALT_RIGHTVOL:
			COMBINE_DATA(&voice->reg[REG_RIGHTVOL]);
			break;
	}

	/* update parameters for compressed voice (11-voice model only) */
	if (chip->voices == 11 && offset >= 0x6d)
	{
		voice = &chip->compressed;
		switch (offset)
		{
			case 0x6d:
				COMBINE_DATA(&voice->reg[REG_LOOPEND]);
				voice->loop_stop_position = voice->reg[REG_LOOPEND] << 16;
				break;

			case 0x6f:
				COMBINE_DATA(&voice->reg[REG_BANK]);
				break;

			case 0x74:
				COMBINE_DATA(&voice->reg[REG_RIGHTVOL]);
				break;

			case 0x75:
				COMBINE_DATA(&voice->reg[REG_CURRPOS]);
				voice->position = voice->reg[REG_CURRPOS] << 16;
				break;

			case 0x78:
				COMBINE_DATA(&voice->reg[REG_LEFTVOL]);
				break;
		}
	}
}



/**********************************************************************************************

     BSMT2000_data_0_w -- handle a write to the current register

***********************************************************************************************/

WRITE16_HANDLER( BSMT2000_data_0_w )
{
	bsmt2000_reg_write(sndti_token(SOUND_BSMT2000, 0), offset, data, mem_mask);
}





/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void bsmt2000_set_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void bsmt2000_get_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = bsmt2000_set_info;		break;
		case SNDINFO_PTR_START:							info->start = bsmt2000_start;			break;
		case SNDINFO_PTR_STOP:							/* nothing */							break;
		case SNDINFO_PTR_RESET:							info->reset = bsmt2000_reset;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "BSMT2000";					break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "Data East Wavetable";		break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2004, The MAME Team"; break;
	}
}

