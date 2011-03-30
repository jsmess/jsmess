/***************************************************************************

  audio/dai.c

  Functions to emulate sound hardware of DAI Personal Computer

  Krzysztof Strzecha

****************************************************************************/

#include "emu.h"
#include "machine/pit8253.h"
#include "machine/i8255a.h"
#include "includes/dai.h"

static STREAM_UPDATE( dai_sh_update );

typedef struct _dai_sound_state dai_sound_state;
struct _dai_sound_state
{
	sound_stream *mixer_channel;
	int dai_input[3];
	UINT8 osc_volume[3];
	UINT8 noise_volume;
};

INLINE dai_sound_state *get_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == DAI);
	return (dai_sound_state *) downcast<legacy_device_base *>(device)->token();
}

static const UINT16 dai_osc_volume_table[] = {
					   0,  500, 1000, 1500,
					2000, 2500, 3000, 3500,
					4000, 4500, 5000, 5500,
					6000, 6500, 7000, 7500
};

static const UINT16 dai_noise_volume_table[] = {
					     0,    0,    0,    0,
					     0,    0,    0,    0,
				           500, 1000, 1500, 2000,
					  2500, 3000, 3500, 4000
};

void dai_set_volume(device_t *device, int offset, UINT8 data)
{
	dai_sound_state *sndstate = get_token(device);

	switch (offset)
	{
	case 0x04:
		sndstate->osc_volume[0] = data&0x0f;
		sndstate->osc_volume[1] = (data&0xf0)>>4;
		break;

	case 0x05:
		sndstate->osc_volume[2] = data&0x0f;
		sndstate->noise_volume = (data&0xf0)>>4;
	}
}

void dai_set_input(device_t *device, int index, int state)
{
	dai_sound_state *sndstate = get_token(device);
	sndstate->mixer_channel->update();

	sndstate->dai_input[index] = state;
}


static DEVICE_START(dai_sound)
{
	dai_sound_state *state = get_token(device);

	state->mixer_channel = device->machine().sound().stream_alloc(*device, 0, 2, device->machine().sample_rate(), 0, dai_sh_update);

	logerror ("sample rate: %d\n", device->machine().sample_rate());
}

static STREAM_UPDATE( dai_sh_update )
{
	dai_sound_state *state = get_token(device);
	INT16 channel_0_signal;
	INT16 channel_1_signal;
	INT16 channel_2_signal;

	stream_sample_t *sample_left = outputs[0];
	stream_sample_t *sample_right = outputs[1];

	channel_0_signal = state->dai_input[0] ? dai_osc_volume_table[state->osc_volume[0]] : -dai_osc_volume_table[state->osc_volume[0]];
	channel_1_signal = state->dai_input[1] ? dai_osc_volume_table[state->osc_volume[1]] : -dai_osc_volume_table[state->osc_volume[1]];
	channel_2_signal = state->dai_input[2] ? dai_osc_volume_table[state->osc_volume[2]] : -dai_osc_volume_table[state->osc_volume[2]];

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

		*sample_left += device->machine().rand()&0x01 ? dai_noise_volume_table[state->noise_volume] : -dai_noise_volume_table[state->noise_volume];

		sample_left++;
		sample_right++;
	}
}



DEVICE_GET_INFO( dai_sound )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(dai_sound_state);			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(dai_sound);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Dai Custom");				break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);						break;
	}
}

DEFINE_LEGACY_SOUND_DEVICE(DAI, dai_sound);
