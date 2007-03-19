/***************************************************************************

  PeT mess@utanet.at
  main part in vidhrdw

***************************************************************************/

#include <math.h>
#include "mame.h"
#include "streams.h"

#include "includes/vc4000.h"

static struct
{
    sound_stream *channel;
    UINT8 reg[1];
    int size, pos;
    unsigned level;
} vc4000_sound= { 0 };



void vc4000_soundport_w (int offset, int data)
{
	stream_update(vc4000_sound.channel);
	vc4000_sound.reg[offset] = data;
	switch (offset)
	{
	case 0:
	    vc4000_sound.pos = 0;
	    vc4000_sound.level = TRUE;
	    // frequency 7874/(data+1)
	    vc4000_sound.size=Machine->sample_rate*(data+1)/7874;
	    break;
	}
}



/************************************/
/* Sound handler update             */
/************************************/

static void vc4000_update(void *param,stream_sample_t **inputs, stream_sample_t **_buffer,int length)
{
	int i;
	stream_sample_t *buffer = _buffer[0];

	for (i = 0; i < length; i++, buffer++)
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

void *vc4000_custom_start(int clock, const struct CustomSound_interface *config)
{
    vc4000_sound.channel = stream_create(0, 1, Machine->sample_rate, 0, vc4000_update);
    return (void *) ~0;
}



struct CustomSound_interface vc4000_sound_interface =
{
	vc4000_custom_start
};
