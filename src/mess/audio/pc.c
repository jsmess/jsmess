/***************************************************************************

	audio/pc.c

	Functions to emulate a PC PIC timer 2 used for simple tone generation

	TODO:
	Add support for the SN76496 used in the Tandy1000/PCjr
	It most probably is on port 0xc0, but I'm not sure...

****************************************************************************/

#include "driver.h"
#include "streams.h"
#include "machine/pcshare.h"
#include "machine/pit8253.h"
#include "audio/pc.h"
#include "deprecat.h"

#define SND_LOG(n,m,a)
#define BASECLOCK	1193180


static void *pc_sh_custom_start(int clock, const struct CustomSound_interface *config);

const struct CustomSound_interface pc_sound_interface =
{
	pc_sh_custom_start
};

static sound_stream *channel;
static int speaker_gate = 0;
static int input_state;



/*************************************
 *
 *	Sound handler start
 *
 *************************************/

static int pc_sh_start(void)
{
	logerror("pc_sh_start\n");
	speaker_gate = 0;
	input_state = 0;
	channel = stream_create(0, 1, Machine->sample_rate, 0, pc_sh_update);
    return 0;
}



static void *pc_sh_custom_start(int clock, const struct CustomSound_interface *config)
{
	pc_sh_start();
	return (void *) ~0;
}



void pc_sh_speaker(running_machine *machine, int data)
{
	int mode = 0;

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


void pc_sh_speaker_set_input(const device_config *device, int state)
{
	stream_update(channel);
	input_state = state;
}


/*************************************
 *
 *	Sound handler update
 *
 *************************************/

void pc_sh_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int length)
{
	INT16 signal = input_state ? 0x7fff : -0x7fff;
	stream_sample_t *buffer = outputs[0];
	stream_sample_t *sample = buffer;

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
		while( length-- > 0 )
			*sample++ = signal;
		break;
	}
}

