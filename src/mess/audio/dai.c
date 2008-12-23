/***************************************************************************

  audio/dai.c

  Functions to emulate sound hardware of DAI Personal Computer

  Krzysztof Strzecha

****************************************************************************/

#include "driver.h"
#include "sound/custom.h"
#include "machine/pit8253.h"
#include "machine/8255ppi.h"
#include "includes/dai.h"
#include "streams.h"

static CUSTOM_START( dai_sh_start );
static STREAM_UPDATE( dai_sh_update );

static sound_stream *mixer_channel;

static int dai_input[3];

static const UINT16 dai_osc_volume_table[] = {
					   0,  500, 1000, 1500,
					2000, 2500, 3000, 3500,
					4000, 4500, 5000, 5500,
					6000, 6500, 7000, 7500};

static const UINT16 dai_noise_volume_table[] = {
					     0,    0,    0,    0,
					     0,    0,    0,    0,
				           500, 1000, 1500, 2000,
					  2500, 3000, 3500, 4000};

const custom_sound_interface dai_sound_interface =
{
	dai_sh_start,
	NULL,
	NULL
};


void dai_set_input(int index, int state)
{
	stream_update( mixer_channel );

	dai_input[index] = state;
}


static CUSTOM_START( dai_sh_start )
{
	dai_input[0] = dai_input[1] = dai_input[2] = 0;

	mixer_channel = stream_create(device, 0, 2, device->machine->sample_rate, 0, dai_sh_update);
	
	logerror ("sample rate: %d\n", device->machine->sample_rate);

	return (void *) ~0;
}

static STREAM_UPDATE( dai_sh_update )
{
	INT16 channel_0_signal;
	INT16 channel_1_signal;
	INT16 channel_2_signal;

	stream_sample_t *sample_left = outputs[0];
	stream_sample_t *sample_right = outputs[1];

	channel_0_signal = dai_input[0] ? dai_osc_volume_table[dai_osc_volume[0]] : -dai_osc_volume_table[dai_osc_volume[0]];
	channel_1_signal = dai_input[1] ? dai_osc_volume_table[dai_osc_volume[1]] : -dai_osc_volume_table[dai_osc_volume[1]];
	channel_2_signal = dai_input[2] ? dai_osc_volume_table[dai_osc_volume[2]] : -dai_osc_volume_table[dai_osc_volume[2]];

	while (samples--)
	{
		*sample_left = 0;
		*sample_right = 0;

		/* music channel 0 */

		*sample_left += channel_0_signal;

		/* music channel 1 */

		*sample_left += channel_1_signal;

		/* music channel 2 */

		*sample_left += channel_2_signal;

		/* noise channel */

		*sample_left += mame_rand(device->machine)&0x01 ? dai_noise_volume_table[dai_noise_volume] : -dai_noise_volume_table[dai_noise_volume];

		sample_left++;
		sample_right++;
	}
}

