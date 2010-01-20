/***************************************************************************

  PeT mess@utanet.at
  main part in video/

***************************************************************************/

#include "emu.h"
#include "streams.h"
#include "includes/arcadia.h"


#define VOLUME (token->reg[2]&0xf)
#define ON (!(token->reg[2]&0x10))


typedef struct _arcadia_sound arcadia_sound;
struct _arcadia_sound
{
    sound_stream *channel;
    UINT8 reg[3];
    int size, pos;
    unsigned level;
};



INLINE arcadia_sound *get_token(const device_config *device)
{
	assert(device != NULL);
	assert(sound_get_type(device) == SOUND_ARCADIA);
	return (arcadia_sound *) device->token;
}



void arcadia_soundport_w (const device_config *device, int offset, int data)
{
	arcadia_sound *token = get_token(device);

	stream_update(token->channel);
	token->reg[offset] = data;
	switch (offset)
	{
		case 1:
			token->pos = 0;
			token->level = TRUE;
			// frequency 7874/(data+1)
			token->size = device->machine->sample_rate*((data&0x7f)+1)/7874;
		    break;
	}
}



/************************************/
/* Sound handler update             */
/************************************/

static STREAM_UPDATE( arcadia_update )
{
	int i;
	arcadia_sound *token = get_token(device);
	stream_sample_t *buffer = outputs[0];

	for (i = 0; i < samples; i++, buffer++)
	{
		*buffer = 0;
		if (token->reg[1] && token->pos <= token->size/2)
		{
			*buffer = 0x2ff * VOLUME; // depends on the volume between sound and noise
		}
		if (token->pos <= token->size)
			token->pos++;
		if (token->pos > token->size)
			token->pos = 0;
	}
}



/************************************/
/* Sound handler start              */
/************************************/

static DEVICE_START(arcadia_sound)
{
	arcadia_sound *token = get_token(device);
    token->channel = stream_create(device, 0, 1, device->machine->sample_rate, 0, arcadia_update);
}



DEVICE_GET_INFO(arcadia_sound)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(arcadia_sound);			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(arcadia_sound);		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Arcadia Custom");					break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);						break;
	}
}
