/**********************************************************************

    NEC uPD1771

    Made using recording/analysis on a Yeno (PAL Super Cassete Vision)
    by plgDavid

**********************************************************************/

#include "emu.h"
#include "streams.h"
#include "upd1771.h"


#define LOG 1

#define MAX_PACKET_SIZE 16 //protection/padding max is really 10



/*
  Each of the 32 waveform has been sampled at 192Khz, filtered, and each level has been
  calculated with averages on around 10 samples (removing the transition samples)
  then quantized to 8bit signed. (not clear on the exact DAC details yet)
*/
const char WAVEFORMS[8][32]={

{-5,-5,-5,-117,-116,-53,-10,127,120,108,97,-121,-121,-121,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-119,-119,-118,-2,-2,-2,-2,-2},
{6,-21,-1,-41,-1,25,-35,-35,-1,-16,34,29,-37,-30,-33,-20,38,-15,50,-20,-20,-15,7,-20,77,-15,-37,69,93,-21,-38,-37},
{-11,-4,-11,51,-9,-11,-11,84,87,-112,44,102,-86,-112,35,103,-12,51,-10,-12,-12,-9,-12,13,-11,-44,25,103,-12,-5,-90,-101},
{40,98,31,98,-1,13,58,3,-18,45,-5,-13,-5,-13,-5,-13,-5,-13,-5,-13,-10,-15,-121,5,-17,45,-128,8,-16,-12,-16,-9},
{-53,-101,-121,-128,-113,-77,-34,5,26,63,97,117,119,119,115,99,54,13,-13,-11,-2,3,31,52,62,74,60,51,38,22,8,-14},
{-86,-128,-60,3,65,101,119,44,37,41,51,53,55,58,58,29,-12,74,82,77,59,113,52,21,24,34,39,45,48,48,48,-13},
{-15,-18,-46,-67,-95,-111,-117,-124,-128,-123,-116,-105,-89,-72,-50,-21,2,16,46,76,95,111,118,119,119,119,117,110,97,75,47,18},
{-84,-121,-128,-105,-51,7,38,66,93,97,93,88,89,96,102,111,116,118,118,119,118,118,117,117,118,118,117,117,117,115,85,-14}

};

#define STATE_SILENCE 0
#define STATE_NOISE   1
#define STATE_TONE    2

typedef struct _upd1771_state upd1771_state;
struct _upd1771_state
{
	sound_stream *channel;
	devcb_resolved_write_line ack_out_func;
	emu_timer *timer;

	UINT8	packet[MAX_PACKET_SIZE];
	UINT8	index;
	UINT8	expected_bytes;

	UINT8   state;//0:silence, 1 noise, 2 tone

	//tone
	UINT8    t_timbre; //[0;  7]
	UINT8    t_offset; //[0; 32]
	UINT16   t_period; //[0;255]
	UINT8    t_volume; //[0; 31]

	UINT8    t_tpos;//timber pos
	UINT16   t_ppos;//period pos
};


INLINE upd1771_state *get_safe_token(running_device *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == SOUND);
	assert(sound_get_type(device) == SOUND_UPD1771C);
	return (upd1771_state *)device->token;
}


/*
*************TONE*****************
Tone consist of a wavetable playback mechanism.
Each wavetable is a looping period of 32 samples but can be played with an offset from any point in the table
effectively shrinking the sample loop, thus allowing different pitch "macros ranges" to be played.
This method is rather crude because the spectrum of the sound get heavily altered...
unless that was the intent.

Tone Write (4 bytes):

Byte0: 0x02

Byte1: 0bTTTOOOOO
  MSB 3 bits of Timbre (8 wavetables)
  LSB 5 bits offset in the table.

Byte2: 0bPPPPPPPP
  8bits of clock divider/period
  Anything under <= 0x20 give the same value

Byte3: 0b???VVVVV
   MSB 3 bits unknown
   LSB 5 bits of "Volume"

   Note: volume is not a volume in a normal sense but some kind
   of bit cropping/rounding.
*/



WRITE8_DEVICE_HANDLER( upd1771_w )
{
	upd1771_state *state = get_safe_token( device );

	//if (LOG)
	//  logerror( "upd1771_w: received byte 0x%02x\n", data );

	devcb_call_write_line( &state->ack_out_func, 0 );

	state->packet[state->index++]=data;

	switch(state->packet[0]){

		case 0:
		{
			state->state = STATE_SILENCE;
			state->index = 0;
			//logerror( "upd1771_w: ----------------silence  state reset\n");
		}break;

		case 1:
			if (state->index == 10){
				state->state = STATE_NOISE; //TODO
				state->index = 0;
				//logerror( "upd1771_w: ----------------noise state reset\n");
			}
			else
				timer_adjust_oneshot( state->timer, ticks_to_attotime( 512, device->clock ), 0 );
		break;

		case 2:
			if (state->index == 4){
				//logerror( "upd1771_w: ----------------tone  state reset\n");
				state->t_timbre = (state->packet[1] & 0xE0) >> 5;
				state->t_offset = (state->packet[1] & 0x1F);
				state->t_period =  state->packet[2];
				//smaller periods dont all equal to 0x20
                if (state->t_period < 0x20)
                    state->t_period = 0x20;

				state->t_volume =  state->packet[3] & 0x1f;
				state->state = STATE_TONE;
				state->index = 0;
			}
			else
				timer_adjust_oneshot( state->timer, ticks_to_attotime( 512, device->clock ), 0 );

		break;

		//garbage: wipe stack
		default:
			state->index = 0;
		break;
	}
}


static STREAM_UPDATE( upd1771c_update )
{
	upd1771_state *state = get_safe_token( device );
	stream_sample_t *buffer = outputs[0];

	switch(state->state){
		case STATE_TONE:
		{
			//logerror( "upd1771_STATE_TONE samps:%d %d %d %d %d %d\n",(int)samples,
			//    (int)state->t_timbre,(int)state->t_offset,(int)state->t_volume,(int)state->t_period,(int)state->t_tpos);

			while ( samples > 0 ){

				*buffer = (WAVEFORMS[state->t_timbre][state->t_tpos])*state->t_volume * 2;

			    state->t_ppos++;
				if (state->t_ppos >= state->t_period){

					state->t_tpos++;
					if (state->t_tpos == 32)
					   state->t_tpos = state->t_offset;

				   state->t_ppos = 0;
				}

				buffer++;
				samples--;
			}
		}break;

		default:
		{
		    //fill buffer with silence
			while ( samples > 0 )
			{
				*buffer = 0;
				buffer++;
				samples--;
			}
		}

		break;
	}

}


static TIMER_CALLBACK( upd1771c_callback )
{
	running_device *device = (running_device *)ptr;
	upd1771_state *state = get_safe_token( device );

	devcb_call_write_line( &state->ack_out_func, 1 );
}


static DEVICE_START( upd1771c )
{
	const upd1771_interface *intf = (const upd1771_interface *)device->baseconfig().static_config;
	upd1771_state *state = get_safe_token( device );
	int sample_rate = device->clock / 4;


	/* resolve callbacks */
	devcb_resolve_write_line( &state->ack_out_func, &intf->ack_callback, device );

	state->timer = timer_alloc( device->machine, upd1771c_callback, (void *)device );

	state->channel = stream_create( device, 0, 1, sample_rate, state, upd1771c_update );

	state_save_register_device_item_array( device, 0, state->packet );
	state_save_register_device_item(device, 0, state->index );
	state_save_register_device_item(device, 0, state->expected_bytes );
}


static DEVICE_RESET( upd1771c )
{
	upd1771_state *state = get_safe_token( device );

	state->index = 0;
	state->expected_bytes = 0;
}


static DEVICE_STOP( upd1771c )
{
}


DEVICE_GET_INFO( upd1771c )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:				info->i = sizeof(upd1771_state);				break;

		/* --- the following bits of info are returned as pointers to functions --- */
		case DEVINFO_FCT_START:						info->start = DEVICE_START_NAME( upd1771c );	break;
		case DEVINFO_FCT_RESET:						info->reset = DEVICE_RESET_NAME( upd1771c );	break;
		case DEVINFO_FCT_STOP:						info->stop  = DEVICE_STOP_NAME( upd1771c );		break;


		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:						strcpy(info->s, "NEC uPD1771C 017");			break;
		case DEVINFO_STR_FAMILY:					strcpy(info->s, "NEC uPD1771");					break;
		case DEVINFO_STR_VERSION:					strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:				strcpy(info->s, __FILE__);						break;
		case DEVINFO_STR_CREDITS:					strcpy(info->s, "Copyright the MAME & MESS Teams"); break;
	}
}

