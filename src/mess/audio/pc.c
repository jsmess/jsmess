/***************************************************************************

    sndhrdw/pc.c

	Functions to emulate a PC PIC timer 2 used for simple tone generation

	TODO:
	Add support for the SN76496 used in the Tandy1000/PCjr
	It most probably is on port 0xc0, but I'm not sure...

****************************************************************************/

#include "driver.h"
#include "streams.h"
#include "machine/pcshare.h"
#include "machine/pit8253.h"

#define SND_LOG(n,m,a)
#define BASECLOCK	1193180


static void *pc_sh_custom_start(int clock, const struct CustomSound_interface *config);

struct CustomSound_interface pc_sound_interface =
{
	pc_sh_custom_start
};

static sound_stream *channel;
static int speaker_gate = 0;



/*************************************
 *
 *	Sound handler start
 *
 *************************************/

static int pc_sh_start(void)
{
	logerror("pc_sh_start\n");
	channel = stream_create(0, 1, Machine->sample_rate, 0, pc_sh_update);
    return 0;
}



static void *pc_sh_custom_start(int clock, const struct CustomSound_interface *config)
{
	pc_sh_start();
	return (void *) ~0;
}



void pc_sh_speaker(int data)
{
	int mode = 0;
	pit8253_0_gate_w(2, data & 1);

	switch( data )
	{
		case 0: mode=0; break;
		case 1: case 2: mode=1; break;
		case 3: mode=2; break;
	}

	if( mode == speaker_gate )
		return;

    stream_update(channel);

    switch( mode )
	{
		case 0: /* completely off */
			SND_LOG(1,"PC_speaker",("off\n"));
			speaker_gate = 0;
            break;
		case 1: /* completely on */
			SND_LOG(1,"PC_speaker",("on\n"));
			speaker_gate = 1;
            break;
		case 2: /* play the tone */
			SND_LOG(1,"PC_speaker",("tone\n"));
			speaker_gate = 2;
            break;
    }
}

void pc_sh_speaker_change_clock(double pc_clock)
{
    stream_update(channel);
}



/*************************************
 *
 *	Sound handler update
 *
 *************************************/

void pc_sh_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int length)
{
	static INT16 signal = 0x7fff;
	stream_sample_t *buffer = outputs[0];
	static int incr = 0;
	stream_sample_t *sample = buffer;
	int baseclock, rate = Machine->sample_rate / 2;

	baseclock = pit8253_get_frequency(0, 2);

	switch( speaker_gate ) {
	case 0:
		/* speaker off */
		while( length-- > 0 )
			*sample++ = 0;
		break;

	case 1:
		/* speaker on */
		while( length-- > 0 )
			*sample++ = 0x7fff;
		break;

	case 2:
		/* speaker gate tone from PIT channel #2 */
		if (baseclock > rate)
		{
			/* if the tone is too high to play, don't play it */
			while( length-- > 0 )
				*sample++ = 0;
		}
		else
		{
			while( length-- > 0 )
			{
				*sample++ = signal;
				incr -= baseclock;
				while( incr < 0 )
				{
					incr += rate;
					signal = -signal;
				}
			}
		}
		break;
	}
}

