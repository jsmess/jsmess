/***************************************************************************

    audio/socrates.c

    This handles the two squarewaves (plus the one weird wave) channels
    on the V-tech Socrates system 27-0769 ASIC.

****************************************************************************/

#include "sndintrf.h"
#include "streams.h"
#include "socrates.h"

typedef struct 
{
	sound_stream *stream; 
	UINT8 freq[2]; /* channel 1,2 frequencies */
	UINT8 vol_ch1; /* channel 1 volume */
	UINT8 vol_ch2; /* channel 2 volume */
	UINT8 channel3; /* channel 3 weird register */
	UINT8 state[3]; /* output states for channels 1,2,3 */
	UINT8 accum[3]; /* accumulators for channels 1,2,3 */
	UINT8 DAC_output; /* output */
} SocratesASIC;


INLINE SocratesASIC *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == SOUND);
	assert(sound_get_type(device) == SOUND_SOCRATES);
	return (SocratesASIC *)device->token;
}

static void socrates_snd_clock(SocratesASIC *chip) /* called once per clock */
{
	int channel;
	for (channel = 0; channel < 2; channel++)
	{
		if (chip->accum[channel] == 0)
		{
		chip->state[channel] = (chip->state[channel]^0x1);
		chip->accum[channel] = chip->freq[channel];
		}
		else
		chip->accum[channel]--;
	}
	// handle channel 3 here
	chip->DAC_output = (chip->state[0]?(chip->vol_ch1<<3):0);
	chip->DAC_output += (chip->state[1]?(chip->vol_ch2<<2):0);
	// add channel 3 to dac output here
}

/*************************************
 *
 *  Stream updater
 *
 *************************************/
static STREAM_UPDATE( socrates_snd_pcm_update )
{
	INT32 mix[48000];
	INT32 *mixp;
	SocratesASIC *chip = (SocratesASIC *)param;
	int i;

	memset(mix, 0, sizeof(mix));

	mixp = &mix[0];
	for (i = 0; i < samples; i++)
	{
		socrates_snd_clock(chip);
		outputs[0][i] = (((int)chip->DAC_output<<8)|(int)chip->DAC_output);
	}
}
   


/*************************************
 *
 *  Sound handler start
 *
 *************************************/

static DEVICE_START( socrates_snd )
{
	SocratesASIC *chip = get_safe_token(device);
	chip->freq[0] = chip->freq[1] = 0xff; /* channel 1,2 frequency */
	chip->vol_ch1 = 0x01; /* channel 1 volume */
	chip->vol_ch2 = 0x01; /* channel 2 volume */
	chip->channel3 = 0x00; /* channel 3 weird register */
	chip->DAC_output = 0x00; /* output */
	chip->state[0] = chip->state[1] = chip->state[2] = 0;
	chip->accum[0] = chip->accum[1] = chip->accum[2] = 0xFF;
	chip->stream = stream_create(device, 0, 1, device->clock ? device->clock : device->machine->sample_rate, chip, socrates_snd_pcm_update);
}


void socrates_snd_reg0_w(const device_config *device, int data)
{
	SocratesASIC *chip = get_safe_token(device);
	stream_update(chip->stream);
	chip->freq[0] = data;
}

void socrates_snd_reg1_w(const device_config *device, int data)
{
	SocratesASIC *chip = get_safe_token(device);
	stream_update(chip->stream);
	chip->freq[1] = data;
}

void socrates_snd_reg2_w(const device_config *device, int data)
{
	SocratesASIC *chip = get_safe_token(device);
	stream_update(chip->stream);
	chip->vol_ch1 = data&0xF;
}

void socrates_snd_reg3_w(const device_config *device, int data)
{
	SocratesASIC *chip = get_safe_token(device);
	stream_update(chip->stream);
	chip->vol_ch2 = data&0xF;
}

void socrates_snd_reg4_w(const device_config *device, int data)
{
	SocratesASIC *chip = get_safe_token(device);
	stream_update(chip->stream);
	chip->channel3 = data;
}


/**************************************************************************
 * Generic get_info
 **************************************************************************/

DEVICE_GET_INFO( socrates_snd )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(SocratesASIC);				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( socrates_snd );	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Socrates Sound");					break;
		case DEVINFO_STR_FAMILY:					strcpy(info->s, "Socrates Sound");					break;
		case DEVINFO_STR_VERSION:					strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:						strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:					strcpy(info->s, "Copyright Jonathan Gevaryahu and The MESS Team"); break;
	}
}
