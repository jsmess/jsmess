/***************************************************************************
 supervision sound hardware

 PeT mess@utanet.at
***************************************************************************/

#include <math.h>

#include "mame.h"
#include "timer.h"
#include "streams.h"

#include "includes/svision.h"

static sound_stream *mixer_channel;

SVISION_CHANNEL svision_channel[2];

void svision_soundport_w (SVISION_CHANNEL *channel, int offset, int data)
{
    stream_update(mixer_channel);
    logerror("%.6f channel 1 write %d %02x\n", timer_get_time(),offset&3, data);
    channel->reg[offset]=data;
    switch (offset) {
    case 0:
    case 1:
	if (channel->reg[0]) {
	    if (channel==svision_channel) 
		channel->size=(int)((Machine->sample_rate*channel->reg[0]<<6)/4e6);
	    else
		channel->size=(int)((Machine->sample_rate*channel->reg[0]<<6)/4e6);
	} else channel->size=0;
	channel->pos=0;
    }
    
}



/************************************/
/* Sound handler update             */
/************************************/

static void svision_update (void *param,stream_sample_t **inputs, stream_sample_t **_buffer,int length)
{
	stream_sample_t *left=_buffer[0], *right=_buffer[1];
	int i, j;
	SVISION_CHANNEL *channel;

	for (i = 0; i < length; i++, left++, right++)
	{
		*left = 0;
		*right = 0;
		for (channel=svision_channel, j=0; j<ARRAY_LENGTH(svision_channel); j++, channel++)
		{
			if (channel->pos<=channel->size/2)
			{
				if (channel->reg[2]&0x40)
				{
					*left+=(channel->reg[2]&0xf)<<8;
				}
				if (channel->reg[2]&0x20)
				{
					*right+=(channel->reg[2]&0xf)<<8;
				}
			}
			if (channel->reg[2]&0x60)
			{
				if (++channel->pos>=channel->size)
					channel->pos=0;
			}
		}
	}
}

/************************************/
/* Sound handler start              */
/************************************/

void *svision_custom_start(int clock, const struct CustomSound_interface *config)
{
	mixer_channel = stream_create(0, 2, Machine->sample_rate, 0, svision_update);
	return (void *) ~0;
}

