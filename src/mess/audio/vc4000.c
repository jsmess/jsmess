/***************************************************************************

  PeT mess@utanet.at
  main part in video/

***************************************************************************/

#include "driver.h"
#include "streams.h"
#include "includes/vc4000.h"


static struct
{
    sound_stream *channel;
    UINT8 reg[1];
    int size, pos;
    unsigned level;
} vc4000_sound= { 0 };



void vc4000_soundport_w (running_machine *machine, int offset, int data)
{
	stream_update(vc4000_sound.channel);
	vc4000_sound.reg[offset] = data;
	switch (offset)
	{
	case 0:
	    vc4000_sound.pos = 0;
	    vc4000_sound.level = TRUE;
	    // frequency 7874/(data+1)
	    vc4000_sound.size=machine->sample_rate*(data+1)/7874;
	    break;
	}
}



/************************************/
/* Sound handler update             */
/************************************/

static STREAM_UPDATE( vc4000_update )
{
	int i;
	stream_sample_t *buffer = outputs[0];

	for (i = 0; i < samples; i++, buffer++)
	{
		*buffer = 0;
		if (vc4000_sound.reg[0] && vc4000_sound.pos <= vc4000_sound.size/2)
		{
			*buffer = 0x7fff;
		}
		if (vc4000_sound.pos <= vc4000_sound.size)
			vc4000_sound.pos++;
		if (vc4000_sound.pos > vc4000_sound.size)
			vc4000_sound.pos = 0;
	}
}



/************************************/
/* Sound handler start              */
/************************************/

static DEVICE_START(vc4000_sound)
{
    vc4000_sound.channel = stream_create(device, 0, 1, device->machine->sample_rate, 0, vc4000_update);
}


DEVICE_GET_INFO( vc4000_sound )
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(vc4000_sound);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "VC4000 Sound");				break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);						break;
	}
}
