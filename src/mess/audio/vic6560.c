/***************************************************************************

  MOS video interface chip 6560 (and sound interface)
  PeT mess@utanet.at

  main part in vidhrdw
***************************************************************************/

#include <math.h>

#include "driver.h"
#include "mame.h"
#include "streams.h"

#define VERBOSE_DBG 0
#include "includes/cbm.h"
#include "includes/vic6560.h"

/*
 * assumed model:
 * each write to a ton/noise generated starts it new
 * each generator behaves like an timer
 * when it reaches 0, the next samplevalue is given out
 */

/*
 * noise channel
 * based on a document by diku0748@diku.dk (Asger Alstrup Nielsen)
 *
 * 23 bit shift register
 * initial value (0x7ffff8)
 * after shift bit 0 is set to bit 22 xor bit 17
 * dac sample bit22 bit20 bit16 bit13 bit11 bit7 bit4 bit2(lsb)
 *
 * emulation:
 * allocate buffer for 5 sec sampledata (fastest played frequency)
 * and fill this buffer in init with the required sample
 * fast turning off channel, immediate change of frequency
 */

#define NOISE_BUFFER_SIZE_SEC 5

#define TONE1_ON (vic6560[0xa]&0x80)
#define TONE2_ON (vic6560[0xb]&0x80)
#define TONE3_ON (vic6560[0xc]&0x80)
#define NOISE_ON (vic6560[0xd]&0x80)
#define VOLUME (vic6560[0xe]&0x0f)

#define TONE_FREQUENCY_MIN  (VIC656X_CLOCK/256/128)
#define TONE1_VALUE (8*(128-((vic6560[0xa]+1)&0x7f)))
#define TONE1_FREQUENCY (VIC656X_CLOCK/32/TONE1_VALUE)
#define TONE2_VALUE (4*(128-((vic6560[0xb]+1)&0x7f)))
#define TONE2_FREQUENCY (VIC656X_CLOCK/32/TONE2_VALUE)
#define TONE3_VALUE (2*(128-((vic6560[0xc]+1)&0x7f)))
#define TONE3_FREQUENCY (VIC656X_CLOCK/32/TONE3_VALUE)
#define NOISE_VALUE (32*(128-((vic6560[0xd]+1)&0x7f)))
#define NOISE_FREQUENCY (VIC656X_CLOCK/NOISE_VALUE)
#define NOISE_FREQUENCY_MAX (VIC656X_CLOCK/32/1)

static int tone1pos = 0, tone2pos = 0, tone3pos = 0,
	tonesize, tone1samples = 1, tone2samples = 1, tone3samples = 1,
	noisesize,		  /* number of samples */
	noisepos = 0,         /* pos of tone */
	noisesamples = 1;	  /* count of samples to give out per tone */

static sound_stream *channel;
static INT16 *tone;

static INT8 *noise;

void vic6560_soundport_w (int offset, int data)
{
    int old = vic6560[offset];
	stream_update(channel);

	switch (offset)
	{
	case 0xa:
		vic6560[offset] = data;
		if (!(old & 0x80) && TONE1_ON)
		{
			tone1pos = 0;
			tone1samples = Machine->sample_rate / TONE1_FREQUENCY;
			if (tone1samples == 0)
				tone1samples = 1;
		}
		DBG_LOG (1, "vic6560", (errorlog, "tone1 %.2x %d\n", data, TONE1_FREQUENCY));
		break;
	case 0xb:
		vic6560[offset] = data;
		if (!(old & 0x80) && TONE2_ON)
		{
			tone2pos = 0;
			tone2samples = Machine->sample_rate / TONE2_FREQUENCY;
			if (tone2samples == 0)
				tone2samples = 1;
		}
		DBG_LOG (1, "vic6560", (errorlog, "tone2 %.2x %d\n", data, TONE2_FREQUENCY));
		break;
	case 0xc:
		vic6560[offset] = data;
		if (!(old & 0x80) && TONE3_ON)
		{
			tone3pos = 0;
			tone3samples = Machine->sample_rate / TONE3_FREQUENCY;
			if (tone2samples == 0)
				tone2samples = 1;
		}
		DBG_LOG (1, "vic6560", (errorlog, "tone3 %.2x %d\n", data, TONE3_FREQUENCY));
		break;
	case 0xd:
		vic6560[offset] = data;
		if (NOISE_ON)
		{
			noisesamples = (int) ((double) NOISE_FREQUENCY_MAX * Machine->sample_rate
								  * NOISE_BUFFER_SIZE_SEC / NOISE_FREQUENCY);
			DBG_LOG (1, "vic6560", (errorlog, "noise %.2x %d sample:%d\n",
									data, NOISE_FREQUENCY, noisesamples));
			if ((double) noisepos / noisesamples >= 1.0)
			{
				noisepos = 0;
			}
		}
		else
		{
			noisepos = 0;
		}
		break;
	case 0xe:
		vic6560[offset] = (old & ~0xf) | (data & 0xf);
		DBG_LOG (3, "vic6560", (errorlog, "volume %d\n", data & 0xf));
		break;
	}
}

/************************************/
/* Sound handler update             */
/************************************/
static void vic6560_update (void *param,stream_sample_t **inputs, stream_sample_t **_buffer,int length)
{
	int i, v;
	stream_sample_t *buffer = _buffer[0];

	for (i = 0; i < length; i++)
	{
		v = 0;
		if (TONE1_ON /*||(tone1pos!=0) */ )
		{
			v += tone[tone1pos * tonesize / tone1samples];
			tone1pos++;
#if 0
			tone1pos %= tone1samples;
#else
			if (tone1pos >= tone1samples)
			{
				tone1pos = 0;
				tone1samples = Machine->sample_rate / TONE1_FREQUENCY;
				if (tone1samples == 0)
					tone1samples = 1;
			}
#endif
		}
		if (TONE2_ON /*||(tone2pos!=0) */ )
		{
			v += tone[tone2pos * tonesize / tone2samples];
			tone2pos++;
#if 0
			tone2pos %= tone2samples;
#else
			if (tone2pos >= tone2samples)
			{
				tone2pos = 0;
				tone2samples = Machine->sample_rate / TONE2_FREQUENCY;
				if (tone2samples == 0)
					tone2samples = 1;
			}
#endif
		}
		if (TONE3_ON /*||(tone3pos!=0) */ )
		{
			v += tone[tone3pos * tonesize / tone3samples];
			tone3pos++;
#if 0
			tone3pos %= tone3samples;
#else
			if (tone3pos >= tone3samples)
			{
				tone3pos = 0;
				tone3samples = Machine->sample_rate / TONE3_FREQUENCY;
				if (tone3samples == 0)
					tone3samples = 1;
			}
#endif
		}
		if (NOISE_ON)
		{
			v += noise[(int) ((double) noisepos * noisesize / noisesamples)];
			noisepos++;
			if ((double) noisepos / noisesamples >= 1.0)
			{
				noisepos = 0;
			}
		}
		v = (v * VOLUME) << 2;
		if (v > 32767)
			buffer[i] = 32767;
		else if (v < -32767)
			buffer[i] = -32767;
		else
			buffer[i] = v;



	}
}

/************************************/
/* Sound handler start          */
/************************************/

void *vic6560_custom_start(int clock, const struct CustomSound_interface *config)
{
	int i;

	channel = stream_create(0, 1, Machine->sample_rate, 0, vic6560_update);

	/* buffer for fastest played sample for 5 second
	 * so we have enough data for min 5 second */
	noisesize = NOISE_FREQUENCY_MAX * NOISE_BUFFER_SIZE_SEC;
	noise = (INT8*) auto_malloc (noisesize * sizeof (noise[0]));
	{
		int noiseshift = 0x7ffff8;
		char data;

		for (i = 0; i < noisesize; i++)
		{
			data = 0;
			if (noiseshift & 0x400000)
				data |= 0x80;
			if (noiseshift & 0x100000)
				data |= 0x40;
			if (noiseshift & 0x010000)
				data |= 0x20;
			if (noiseshift & 0x002000)
				data |= 0x10;
			if (noiseshift & 0x000800)
				data |= 0x08;
			if (noiseshift & 0x000080)
				data |= 0x04;
			if (noiseshift & 0x000010)
				data |= 0x02;
			if (noiseshift & 0x000004)
				data |= 0x01;
			noise[i] = data;
			if (((noiseshift & 0x400000) == 0) != ((noiseshift & 0x002000) == 0))
				noiseshift = (noiseshift << 1) | 1;
			else
				noiseshift <<= 1;
		}
	}
	tonesize = Machine->sample_rate / TONE_FREQUENCY_MIN;

	if (tonesize > 0)
	{
		tone = (INT16*) auto_malloc (tonesize * sizeof (tone[0]));

		for (i = 0; i < tonesize; i++)
		{
			tone[i] = (INT16)(sin (2 * M_PI * i / tonesize) * 127 + 0.5);
		}
	}
	else
	{
		tone = NULL;
	}
	return (void *) ~0;
}
