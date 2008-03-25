/***************************************************************************

  audio/special.c

  Functions to emulate sound hardware of Specialist MX
  ( based on code of DAI interface )

****************************************************************************/

#include "driver.h"
#include "sound/custom.h"
#include "machine/pit8253.h"
#include "includes/dai.h"
#include "streams.h"
#include "deprecat.h"

static void *specimx_sh_start(int clock, const struct CustomSound_interface *config);
static void specimx_sh_update(void *param,stream_sample_t **inputs, stream_sample_t **_buffer,int length);

static sound_stream *mixer_channel;

const struct CustomSound_interface specimx_sound_interface =
{
	specimx_sh_start,
	NULL,
	NULL
};

static void *specimx_sh_start(int clock, const struct CustomSound_interface *config)
{
	mixer_channel = stream_create(0, 2, Machine->sample_rate, 0, specimx_sh_update);
	return (void *) ~0;
}

static void specimx_sh_update(void *param,stream_sample_t **inputs, stream_sample_t **buffer,int length)
{
	INT16 channel_0_signal;
	INT16 channel_1_signal;
	INT16 channel_2_signal;
	static int channel_0_incr = 0;
	static int channel_1_incr = 0;
	static int channel_2_incr = 0;
	int channel_0_baseclock;
	int channel_1_baseclock;
	int channel_2_baseclock;

	int rate = Machine->sample_rate / 2;

	stream_sample_t *sample_left = buffer[0];
	stream_sample_t *sample_right = buffer[1];

	channel_0_baseclock = pit8253_get_frequency(0, 0);
	channel_1_baseclock = pit8253_get_frequency(0, 1);
	channel_2_baseclock = pit8253_get_frequency(0, 2);

	channel_0_signal = pit8253_get_output (0,0) ? 3000 : -3000;
	channel_1_signal = pit8253_get_output (0,1) ? 3000 : -3000;
	channel_2_signal = pit8253_get_output (0,2) ? 3000 : -3000;


	while (length--)
	{
		*sample_left = 0;
		*sample_right = 0;

		/* music channel 0 */

		*sample_left += channel_0_signal;
		channel_0_incr -= channel_0_baseclock;
		while( channel_0_incr < 0 )
		{
			channel_0_incr += rate;
			channel_0_signal = -channel_0_signal;
		}


		/* music channel 1 */

		*sample_left += channel_1_signal;
		channel_1_incr -= channel_1_baseclock;
		while( channel_1_incr < 0 )
		{
			channel_1_incr += rate;
			channel_1_signal = -channel_1_signal;
		}

		/* music channel 2 */

		*sample_left += channel_2_signal;
		channel_2_incr -= channel_2_baseclock;
		while( channel_2_incr < 0 )
		{
			channel_2_incr += rate;
			channel_2_signal = -channel_2_signal;
		}
		
		sample_left++;
		sample_right++;
	}
}

void specimx_sh_change_clock(double clock)
{
	stream_update(mixer_channel);
}
