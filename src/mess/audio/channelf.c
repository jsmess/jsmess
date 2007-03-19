#include <math.h>

#include "driver.h"
#include "streams.h"
#include "includes/channelf.h"

static sound_stream *channel;
static int sound_mode;
static int incr;
static float decay_mult;
static int envelope;
static UINT32 sample_counter = 0;

const int max_amplitude = 0x7fff;

void channelf_sound_w(int mode)
{
	if (mode == sound_mode)
		return;

    stream_update(channel);
	sound_mode = mode;

    switch(mode)
    {
		case 0:
			envelope = 0;
			break;
		case 1:
		case 2:
		case 3:
			envelope = max_amplitude;
			break;
	}
}



static void channelf_sh_update(void *param,stream_sample_t **inputs, stream_sample_t **_buffer,int length)
{
	UINT32 mask = 0, target = 0;
	stream_sample_t *buffer = _buffer[0];
	stream_sample_t *sample = buffer;

	switch( sound_mode )
	{
		case 0: /* sound off */
			memset(buffer,0,sizeof(*buffer)*length);
			return;

		case 1: /* high tone (2V) - 1000Hz */
			mask   = 0x00010000;
			target = 0x00010000;
			break;
		case 2: /* medium tone (4V) - 500Hz */
			mask   = 0x00020000;
			target = 0x00020000;
			break;
		case 3: /* low (weird) tone (32V & 8V) */
			mask   = 0x00140000;
			target = 0x00140000;
			break;
	}

	while (length-- > 0)
	{
		if ((sample_counter & mask) == target)
			*sample++ = envelope;
		else
			*sample++ = 0;
		sample_counter += incr;
		envelope *= decay_mult;
	}
}



void *channelf_sh_custom_start(int clock, const struct CustomSound_interface *config)
{
	int rate;

	channel = stream_create(0, 1, Machine->sample_rate, 0, channelf_sh_update);
	rate = Machine->sample_rate;

	/*
	 * 2V = 1000Hz ~= 3579535/224/16
	 * Note 2V on the schematic is not the 2V scanline counter -
	 *      it is the 2V vertical pixel counter
	 *      1 pixel = 4 scanlines high
	 *
     *
	 * This is a convenient way to generate the relevant frequencies,
	 * using a DDS (Direct Digital Synthesizer)
	 *
	 * Essentially, you want a counter to overflow some bit position
	 * at a fixed rate.  So, you figure out a number which you can add
	 * to the counter at every sample, so that you will achieve this
	 *
	 * In this case, we want to overflow bit 16 and the 2V rate, 1000Hz.
	 * This means we also get bit 17 = 4V, bit 18 = 8V, etc.
	 */

	/* This is the proper value to add per sample */
	incr = 65536.0/(rate/1000.0/2.0);

	/* This was measured, decay envelope with half life of ~9ms */
	/* (this is decay multiplier per sample) */
	decay_mult = exp((-0.693/9e-3)/rate);

	/* initial conditions */
	envelope = 0;

	return (void *) ~0;
}
