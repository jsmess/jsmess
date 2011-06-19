/******************************************************************************
 PeT mess@utanet.at 2000,2001
******************************************************************************/

#include "emu.h"
#include "includes/lynx.h"


/* accordingly to atari's reference manual
   there were no stereo lynx produced (the manual knows only production until mid 1991)
   the howard/developement board might have stereo support
   the revised lynx 2 hardware might have stereo support at least at the stereo jacks

   some games support stereo
*/


/*
AUDIO_A EQU $FD20
AUDIO_B EQU $FD28
AUDIO_C EQU $FD30
AUDIO_D EQU $FD38

VOLUME_CNTRL    EQU 0
FEEDBACK_ENABLE EQU 1   ; enables 11/10/5..0
OUTPUT_VALUE    EQU 2
SHIFTER_L   EQU 3
AUD_BAKUP   EQU 4
AUD_CNTRL1  EQU 5
AUD_COUNT   EQU 6
AUD_CNTRL2  EQU 7

; AUD_CNTRL1
FEEDBACK_7  EQU %10000000
AUD_RESETDONE   EQU %01000000
AUD_INTEGRATE   EQU %00100000
AUD_RELOAD  EQU %00010000
AUD_CNTEN   EQU %00001000
AUD_LINK    EQU %00000111
; link timers (0->2->4 / 1->3->5->7->Aud0->Aud1->Aud2->Aud3->1
AUD_64us    EQU %00000110
AUD_32us    EQU %00000101
AUD_16us    EQU %00000100
AUD_8us EQU %00000011
AUD_4us EQU %00000010
AUD_2us EQU %00000001
AUD_1us EQU %00000000

; AUD_CNTRL2 (read only)
; B7..B4    ; shifter bits 11..8
; B3    ; who knows
; B2    ; last clock state (0->1 causes count)
; B1    ; borrow in (1 causes count)
; B0    ; borrow out (count EQU 0 and borrow in)

ATTEN_A EQU $FD40
ATTEN_B EQU $FD41
ATTEN_C EQU $FD42
ATTEN_D EQU $FD43
; B7..B4 attenuation left ear (0 silent ..15/16 volume)
; B3..B0       "     right ear

MPAN    EQU $FD44
; B7..B4 left ear
; B3..B0 right ear
; B7/B3 EQU Audio D
; a 1 enables attenuation for channel and side


MSTEREO EQU $FD50   ; a 1 disables audio connection
AUD_D_LEFT  EQU %10000000
AUD_C_LEFT  EQU %01000000
AUD_B_LEFT  EQU %00100000
AUD_A_LEFT  EQU %00010000
AUD_D_RIGHT EQU %00001000
AUD_C_RIGHT EQU %00000100
AUD_B_RIGHT EQU %00000010
AUD_A_RIGHT EQU %00000001

 */

typedef struct {
    union {
	UINT8 data[8];
	struct {
	    UINT8 volume;
	    UINT8 feedback;
	    INT8 output;
	    UINT8 shifter;
	    UINT8 bakup;
	    UINT8 control1;
	    UINT8 counter;
	    UINT8 control2;
	} n;
    } reg;
    UINT8 attenuation;
    int mask;
    int shifter;
    int ticks;
    int count;
} LYNX_AUDIO;

typedef struct _lynx_sound_state lynx_sound_state;
struct _lynx_sound_state
{
	sound_stream *mixer_channel;
	int usec_per_sample;
	int *shift_mask;
	int *shift_xor;
	UINT8 attenuation_enable;
	UINT8 master_enable;
	LYNX_AUDIO audio[4];
};

INLINE lynx_sound_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == LYNX || device->type() == LYNX2);
	return (lynx_sound_state *)downcast<legacy_device_base *>(device)->token();
}

static void lynx_audio_reset_channel(LYNX_AUDIO *This)
{
    memset(This->reg.data, 0, (char*)(This+1)-(char*)(This->reg.data));
}

void lynx_audio_count_down(device_t *device, int nr)
{
    lynx_sound_state *state = get_safe_token(device);
    LYNX_AUDIO *This=state->audio+nr;
    if (This->reg.n.control1&8 && (This->reg.n.control1&7)!=7) return;
    if (nr==0) state->mixer_channel->update();
    This->count--;
}

static void lynx_audio_shift(device_t *device, LYNX_AUDIO *channel)
{
    lynx_sound_state *state = get_safe_token(device);
    int nr = (int)(channel - state->audio);
    channel->shifter=((channel->shifter<<1)&0x3ff)
	|state->shift_xor[channel->shifter&channel->mask];

    if (channel->reg.n.control1&0x20) {
	if (channel->shifter&1) {
	    channel->reg.n.output+=channel->reg.n.volume;
	} else {
	    channel->reg.n.output-=channel->reg.n.volume;
	}
    }
    switch (nr) {
    case 0: lynx_audio_count_down(device, 1); break;
    case 1: lynx_audio_count_down(device, 2); break;
    case 2: lynx_audio_count_down(device, 3); break;
    case 3: lynx_timer_count_down(device->machine(), 1); break;
    }
}

static void lynx_audio_execute(device_t *device, LYNX_AUDIO *channel)
{
    lynx_sound_state *state = get_safe_token(device);
    if (channel->reg.n.control1&8) { // count_enable
        channel->ticks+=state->usec_per_sample;
	if ((channel->reg.n.control1&7)==7) { // timer input
	    if (channel->count<0) {
		channel->count+=channel->reg.n.counter;
		lynx_audio_shift(device, channel);
	    }
	} else {
	    int t=1<<(channel->reg.n.control1&7);
	    for (;;) {
		for (;(channel->ticks>=t)&&channel->count>=0; channel->ticks-=t)
		    channel->count--;
		if (channel->ticks<t) break;
		if (channel->count<0) {
		    channel->count=channel->reg.n.counter;
		    lynx_audio_shift(device, channel);
		}
	    }
	}
	if (!(channel->reg.n.control1&0x20)) {
	    channel->reg.n.output=channel->shifter&1?0-channel->reg.n.volume:channel->reg.n.volume;
	}
    } else {
	channel->ticks=0;
	channel->count=0;
    }
}


UINT8 lynx_audio_read(device_t *device, int offset)
{
    lynx_sound_state *state = get_safe_token(device);
    UINT8 data=0;
    state->mixer_channel->update();
    switch (offset) {
    case 0x20: case 0x21: case 0x22: case 0x24: case 0x25:
    case 0x28: case 0x29: case 0x2a: case 0x2c: case 0x2d:
    case 0x30: case 0x31: case 0x32: case 0x34: case 0x35:
    case 0x38: case 0x39: case 0x3a: case 0x3c: case 0x3d:
	data=state->audio[(offset>>3)&3].reg.data[offset&7];
	break;
    case 0x23: case 0x2b: case 0x33: case 0x3b:
	data=state->audio[(offset>>3)&3].shifter&0xff;
	break;
    case 0x26:case 0x2e:case 0x36:case 0x3e:
	data=state->audio[(offset>>3)&3].count;
	break;
    case 0x27: case 0x2f: case 0x37: case 0x3f:
	data=(state->audio[(offset>>3)&3].shifter>>4)&0xf0;
	data|=state->audio[(offset>>3)&3].reg.data[offset&7]&0x0f;
	break;
    case 0x40: case 0x41: case 0x42: case 0x43:
	data=state->audio[offset&3].attenuation;
	break;
    case 0x44:
	data=state->attenuation_enable;
	break;
    case 0x50:
	data=state->master_enable;
	break;
    }
    return data;
}

void lynx_audio_write(device_t *device, int offset, UINT8 data)
{
	lynx_sound_state *state = get_safe_token(device);
//  logerror("%.6f audio write %.2x %.2x\n", machine.time(), offset, data);
    LYNX_AUDIO *channel=state->audio+((offset>>3)&3);
    state->mixer_channel->update();
    switch (offset) {
    case 0x20: case 0x22: case 0x24: case 0x26:
    case 0x28: case 0x2a: case 0x2c: case 0x2e:
    case 0x30: case 0x32: case 0x34: case 0x36:
    case 0x38: case 0x3a: case 0x3c: case 0x3e:
	state->audio[(offset>>3)&3].reg.data[offset&7]=data;
	break;
    case 0x23: case 0x2b: case 0x33: case 0x3b:
	state->audio[(offset>>3)&3].reg.data[offset&7]=data;
	state->audio[(offset>>3)&3].shifter&=~0xff;
	state->audio[(offset>>3)&3].shifter|=data;
	break;
    case 0x27: case 0x2f: case 0x37: case 0x3f:
	state->audio[(offset>>3)&3].reg.data[offset&7]=data;
	state->audio[(offset>>3)&3].shifter&=~0xf00;
	state->audio[(offset>>3)&3].shifter|=(data&0xf0)<<4;
	break;
    case 0x21: case 0x25:
    case 0x29: case 0x2d:
    case 0x31: case 0x35:
    case 0x39: case 0x3d:
	channel->reg.data[offset&7]=data;
	channel->mask=channel->reg.n.feedback;
	channel->mask|=(channel->reg.data[5]&0x80)<<1;
	break;
    case 0x40: case 0x41: case 0x42: case 0x43: // lynx2 only, howard extension board
	state->audio[offset&3].attenuation=data;
	break;
    case 0x44:
	state->attenuation_enable=data; //lynx2 only, howard extension board
	break;
    case 0x50:
	state->master_enable=data;//lynx2 only, howard write only
	break;
    }
}

/************************************/
/* Sound handler update             */
/************************************/
static STREAM_UPDATE( lynx_update )
{
	lynx_sound_state *state = get_safe_token(device);
	int i, j;
	LYNX_AUDIO *channel;
	int v;
	stream_sample_t *buffer = outputs[0];

	for (i = 0; i < samples; i++, buffer++)
	{
		*buffer = 0;
		for (channel=state->audio, j=0; j<ARRAY_LENGTH(state->audio); j++, channel++)
		{
			lynx_audio_execute(device, channel);
			v=channel->reg.n.output;
			*buffer+=v*15;
		}
	}
}

static STREAM_UPDATE( lynx2_update )
{
	lynx_sound_state *state = get_safe_token(device);
	stream_sample_t *left=outputs[0], *right=outputs[1];
	int i, j;
	LYNX_AUDIO *channel;
	int v;

	for (i = 0; i < samples; i++, left++, right++)
	{
		*left = 0;
		*right= 0;
		for (channel=state->audio, j=0; j<ARRAY_LENGTH(state->audio); j++, channel++)
		{
			lynx_audio_execute(device, channel);
			v=channel->reg.n.output;
			if (!(state->master_enable&(0x10<<j)))
			{
				if (state->attenuation_enable&(0x10<<j)) {
					*left+=v*(channel->attenuation>>4);
				} else {
					*left+=v*15;
				}
			}
			if (!(state->master_enable&(1<<j)))
			{
				if (state->attenuation_enable&(1<<j)) {
					*right+=v*(channel->attenuation&0xf);
				} else {
					*right+=v*15;
				}
			}
		}
	}
}

static void lynx_audio_init(device_t *device)
{
	lynx_sound_state *state = get_safe_token(device);
	int i;
	state->shift_mask = auto_alloc_array(device->machine(), int, 512);
	assert(state->shift_mask);

	state->shift_xor = auto_alloc_array(device->machine(), int, 4096);
	assert(state->shift_xor);

	for (i=0; i<512; i++)
	{
		state->shift_mask[i]=0;
		if (i&1) state->shift_mask[i]|=1;
		if (i&2) state->shift_mask[i]|=2;
		if (i&4) state->shift_mask[i]|=4;
		if (i&8) state->shift_mask[i]|=8;
		if (i&0x10) state->shift_mask[i]|=0x10;
		if (i&0x20) state->shift_mask[i]|=0x20;
		if (i&0x40) state->shift_mask[i]|=0x400;
		if (i&0x80) state->shift_mask[i]|=0x800;
		if (i&0x100) state->shift_mask[i]|=0x80;
	}
	for (i=0; i<4096; i++)
	{
		int j;
		state->shift_xor[i]=1;
		for (j=4096/2; j>0; j>>=1)
		{
			if (i & j)
				state->shift_xor[i] ^= 1;
		}
	}
}

static DEVICE_RESET( lynx_sound )
{
	lynx_sound_state *state = get_safe_token(device);
	int i;
	for (i=0; i<ARRAY_LENGTH(state->audio); i++)
	{
		lynx_audio_reset_channel(state->audio+i);
	}
}



/************************************/
/* Sound handler start              */
/************************************/

static DEVICE_START(lynx_sound)
{
	lynx_sound_state *state = get_safe_token(device);
	state->mixer_channel = device->machine().sound().stream_alloc(*device, 0, 1, device->machine().sample_rate(), 0, lynx_update);

	state->usec_per_sample = 1000000 / device->machine().sample_rate();

	lynx_audio_init(device);
}


static DEVICE_START(lynx2_sound)
{
	lynx_sound_state *state = get_safe_token(device);
	state->mixer_channel = device->machine().sound().stream_alloc(*device, 0, 2, device->machine().sample_rate(), 0, lynx2_update);

	state->usec_per_sample = 1000000 / device->machine().sample_rate();

	lynx_audio_init(device);
}


DEVICE_GET_INFO( lynx_sound )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(lynx_sound_state);			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(lynx_sound);	break;
		case DEVINFO_FCT_RESET:							info->start = DEVICE_RESET_NAME(lynx_sound);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Mikey");				break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);						break;
	}
}


DEVICE_GET_INFO( lynx2_sound )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(lynx_sound_state);			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(lynx2_sound);	break;
		case DEVINFO_FCT_RESET:							info->start = DEVICE_RESET_NAME(lynx_sound);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Mikey (Lynx II)");				break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);						break;
	}
}

DEFINE_LEGACY_SOUND_DEVICE(LYNX, lynx_sound);
DEFINE_LEGACY_SOUND_DEVICE(LYNX2, lynx2_sound);
