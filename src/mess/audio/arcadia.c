/***************************************************************************

  PeT mess@utanet.at
  main part in video/

  Refined with recording/analysis on MPT-03 (PAL UVI chip) by plgDavid

  NTSC UVI sound clock: 15734Hz (arcadia)
  PAL  UVI sound clock: 15625Hz (Soundic MPT-03 - owned by plgDavid)
***************************************************************************/

#include "emu.h"
#include "includes/arcadia.h"

//known UVI audio clocks
#define UVI_NTSC 15734
#define UVI_PAL  15625

/* we need to create pulse transitions that sound 'decent'
   with the current mess/mame interp scheme

  this is not needed anymore with the new trick in streams.c
*/

#define OSAMP  1

//lfsr is 9 bits long (and same as Atari TIA pure noise)
#define LFSR_MASK (1<<8)

//makes alien invaders samples noise sync.
#define LFSR_INIT 0x00f0

//lfsr states at resynch borders
//0x01c1
//0x01e0
//0x00f0  //good synch
//0x0178
//0x01bc


typedef struct _arcadia_sound arcadia_sound;
struct _arcadia_sound
{
    sound_stream *channel;
    UINT8 reg[3];
    int size, pos,tval,nval;
	unsigned mode, omode;
	unsigned volume;
	unsigned lfsr;
};



INLINE arcadia_sound *get_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == ARCADIA);
	return (arcadia_sound *) downcast<legacy_device_base *>(device)->token();
}



void arcadia_soundport_w (device_t *device, int offset, int data)
{
	arcadia_sound *token = get_token(device);

	token->channel->update();
	token->reg[offset] = data;

	//logerror("arcadia_sound write:%x=%x\n",offset,data);

	switch (offset)
	{
		case 1:
			//as per Gobbler samples:
			//the freq counter is only applied on the next change in the flip flop
			token->size = (data & 0x7f)*OSAMP;
			//logerror("arcadia_sound write: frq:%d\n",data);

			//reset LFSR
			if(!token->size)
				token->lfsr = LFSR_INIT;
		break;

		case 2:
			token->volume = (data & 0x07) * 0x800;
			token->mode   = (data & 0x18) >> 3;

			//logerror("arcadia_sound write: vol:%d mode:%d\n",token->volume,token->mode );

			if (token->mode != token->omode){
				//not 100% sure about this, maybe we should not reset anything
				//token->pos  = 0;
				token->tval = 0;
			}
			token->omode = token->mode;
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

		//if minimal pitch ?
		if (token->reg[1]){
			switch (token->mode){
				//dont play anything
				case 0:break;

				//tone only
				case 1:
					*buffer = token->volume * token->tval;
				break;

				//noise only
				case 2:
					*buffer = token->volume * token->nval;
				break;

				//tone AND noise (bitwise and)
				case 3:
					*buffer = token->volume * (token->tval & token->nval);
				break;
			}

			//counter
			token->pos++;

			if (token->pos >= token->size){

				//calculate new noise bit ( taps: 0000T000T)
				unsigned char newBit = token->lfsr & 1;         //first tap
				newBit = (newBit ^ ((token->lfsr & 0x10)?1:0) );//xor with second tap

				token->nval = token->lfsr & 1; //taking new output from LSB
				token->lfsr = token->lfsr >> 1;//shifting

				//insert new bit at end position (size-1) (only if non null)
				if (newBit)
					token->lfsr |= LFSR_MASK;

				//invert tone
				token->tval = !token->tval;

				token->pos = 0;
			}
		}
	}
}



/************************************/
/* Sound handler start              */
/************************************/

static DEVICE_START(arcadia_sound)
{
	arcadia_sound *token = get_token(device);
    token->channel = device->machine().sound().stream_alloc(*device, 0, 1, UVI_PAL*OSAMP, 0, arcadia_update);
    token->lfsr    = LFSR_INIT;
    token->tval    = 1;
	logerror("arcadia_sound start\n");
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

DEFINE_LEGACY_SOUND_DEVICE(ARCADIA, arcadia_sound);
