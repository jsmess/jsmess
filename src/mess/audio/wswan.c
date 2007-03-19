/**************************************************************************************

  Wonderswan sound emulation

  Wilbert Pol

  Sound emulation is very preliminary and far from complete

**************************************************************************************/

#include "driver.h"
#include "includes/wswan.h"
#include "streams.h"

static sound_stream *channel;

struct CHAN {
	UINT16	freq;			/* frequency */
	UINT32	period;			/* period */
	UINT32	pos;			/* position */
	UINT8	vol_left;		/* volume left */
	UINT8	vol_right;		/* volume right */
	UINT8	on;			/* on/off */
	INT8	signal;			/* signal */
};

struct SND {
	struct CHAN audio1;		/* Audio channel 1 */
	struct CHAN audio2;		/* Audio channel 2 */
	struct CHAN audio3;		/* Audio channel 3 */
	struct CHAN audio4;		/* Audio channel 4 */
	INT8	sweep_step;		/* Sweep step */
	UINT32	sweep_time;		/* Sweep time */
	UINT32	sweep_count;		/* Sweep counter */
	UINT8	noise_type;		/* Noise generator type */
	UINT8	noise_reset;		/* Noise reset */
	UINT8	noise_enable;		/* Noise enable */
	UINT16	sample_address;		/* Sample address */
	UINT8	audio2_voice;		/* Audio 2 voice */
	UINT8	audio3_sweep;		/* Audio 3 sweep */
	UINT8	audio4_noise;		/* Audio 4 noise */
	UINT8	mono;			/* mono */
	UINT8	voice_data;		/* voice data */
	UINT8	output_volume;		/* output volume */
	UINT8	external_stereo;	/* external stereo */
	UINT8	external_speaker;	/* external speaker */
	UINT16	noise_shift;		/* Noise counter shift register */
	UINT8	master_volume;		/* Master volume */
};

struct SND snd;

void wswan_ch_set_freq( struct CHAN *ch, UINT16 freq ) {
	ch->freq = freq;
	ch->period = Machine->sample_rate / ( 3072000  / ( ( 2048 - freq ) << 5 ) );
}

WRITE8_HANDLER( wswan_sound_port_w ) {
	stream_update( channel);
	switch( offset ) {
	case 0x80:				/* Audio 1 freq (lo) */
		wswan_ch_set_freq( &snd.audio1, ( snd.audio1.freq & 0xFF00 ) | data );
		break;
	case 0x81:				/* Audio 1 freq (hi) */
		wswan_ch_set_freq( &snd.audio1, ( data << 8 ) | ( snd.audio1.freq & 0x00FF ) );
		break;
	case 0x82:				/* Audio 2 freq (lo) */
		wswan_ch_set_freq( &snd.audio2, ( snd.audio2.freq & 0xFF00 ) | data );
		break;
	case 0x83:				/* Audio 2 freq (hi) */
		wswan_ch_set_freq( &snd.audio2, ( data << 8 ) | ( snd.audio2.freq & 0x00FF ) );
		break;
	case 0x84:				/* Audio 3 freq (lo) */
		wswan_ch_set_freq( &snd.audio3, ( snd.audio3.freq & 0xFF00 ) | data );
		break;
	case 0x85:				/* Audio 3 freq (hi) */
		wswan_ch_set_freq( &snd.audio3, ( data << 8 ) | ( snd.audio3.freq & 0x00FF ) );
		break;
	case 0x86:				/* Audio 4 freq (lo) */
		wswan_ch_set_freq( &snd.audio4, ( snd.audio4.freq & 0xFF00 ) | data );
		break;
	case 0x87:				/* Audio 4 freq (hi) */
		wswan_ch_set_freq( &snd.audio4, ( data << 8 ) | ( snd.audio4.freq & 0x00FF ) );
		break;
	case 0x88:				/* Audio 1 volume */
		snd.audio1.vol_left = ( data & 0xF0 ) >> 4;
		snd.audio1.vol_right = data & 0x0F;
		break;
	case 0x89:				/* Audio 2 volume */
		snd.voice_data = data;
		snd.audio2.vol_left = ( data & 0xF0 ) >> 4;
		snd.audio2.vol_right = data & 0x0F;
		break;
	case 0x8A:				/* Audio 3 volume */
		snd.audio3.vol_left = ( data & 0xF0 ) >> 4;
		snd.audio3.vol_right = data & 0x0F;
		break;
	case 0x8B:				/* Audio 4 volume */
		snd.audio4.vol_left = ( data & 0xF0 ) >> 4;
		snd.audio4.vol_right = data & 0x0F;
		break;
	case 0x8C:				/* Sweep step */
		snd.sweep_step = (INT8)data;
		break;
	case 0x8D:				/* Sweep time */
		snd.sweep_time = Machine->sample_rate / ( 3072000 / ( 8192 * (data + 1) ) );
		break;
	case 0x8E:				/* Noise control */
		snd.noise_type = data & 0x07;
		snd.noise_reset = ( data & 0x08 ) >> 3;
		snd.noise_enable = ( data & 0x10 ) >> 4;
		break;
	case 0x8F:				/* Sample location */
		snd.sample_address = data << 6;
		break;
	case 0x90:				/* Audio control */
		snd.audio1.on = data & 0x01;
		snd.audio2.on = ( data & 0x02 ) >> 1;
		snd.audio3.on = ( data & 0x04 ) >> 2;
		snd.audio4.on = ( data & 0x08 ) >> 3;
		snd.audio2_voice = ( data & 0x20 ) >> 5;
		snd.audio3_sweep = ( data & 0x40 ) >> 6;
		snd.audio4_noise = ( data & 0x80 ) >> 7;
		break;
	case 0x91:				/* Audio output */
		snd.mono = data & 0x01;
		snd.output_volume = ( data & 0x06 ) >> 1;
		snd.external_stereo = ( data & 0x08 ) >> 3;
		snd.external_speaker = 1;
		break;
	case 0x92:				/* Noise counter shift register (lo) */
		snd.noise_shift = ( snd.noise_shift & 0xFF00 ) | data;
		break;
	case 0x93:				/* Noise counter shift register (hi) */
		snd.noise_shift = ( data << 8 ) | ( snd.noise_shift & 0x00FF );
		break;
	case 0x94:				/* Master volume */
		snd.master_volume = data;
		break;
	}
}

static void wswan_sh_update(void *param,stream_sample_t **inputs, stream_sample_t **buffer,int length)
{
	stream_sample_t sample, left, right;

	while( length-- > 0 )
	{
		left = right = 0;

		if ( snd.audio1.on ) {
			sample = snd.audio1.signal;
			snd.audio1.pos++;
			if ( snd.audio1.pos >= snd.audio1.period / 2 ) {
				snd.audio1.pos = 0;
				snd.audio1.signal = -snd.audio1.signal;
			}
			left += snd.audio1.vol_left * sample;
			right += snd.audio1.vol_right * sample;
		}

		if ( snd.audio2.on ) {
			if ( snd.audio2_voice ) {
				sample = (INT8)snd.voice_data;
				left += sample;
				right += sample;
			} else {
				sample = snd.audio2.signal;
				snd.audio2.pos++;
				if ( snd.audio2.pos >= snd.audio2.period / 2 ) {
					snd.audio2.pos = 0;
					snd.audio2.signal = -snd.audio2.signal;
				}
				left += snd.audio2.vol_left * sample;
				right += snd.audio2.vol_right * sample;
			}
		}

		if ( snd.audio3.on ) {
			sample = snd.audio3.signal;
			snd.audio3.pos++;
			if ( snd.audio3.pos >= snd.audio3.period / 2 ) {
				snd.audio3.pos = 0;
				snd.audio3.signal = -snd.audio3.signal;
			}
			if ( snd.audio3_sweep && snd.sweep_time ) {
				snd.sweep_count++;
				if ( snd.sweep_count >= snd.sweep_time ) {
					snd.sweep_count = 0;
					snd.audio3.freq += snd.sweep_step;
					snd.audio3.period = Machine->sample_rate / ( 3072000  / ( ( 2048 - snd.audio3.freq ) << 5 ) );
				}
			}
			left += snd.audio3.vol_left * sample;
			right += snd.audio3.vol_right * sample;
		}

		if ( snd.audio4.on ) {
			if ( snd.audio4_noise ) {
				sample = 0;
			} else {
				sample = snd.audio4.signal;
				snd.audio4.pos++;
				if ( snd.audio4.pos >= snd.audio4.period / 2 ) {
					snd.audio4.pos = 0;
					snd.audio4.signal = -snd.audio4.signal;
				}
			}
			left += snd.audio4.vol_left * sample;
			right += snd.audio4.vol_right * sample;
		}

		left <<= 5;
		right <<= 5;

		*(buffer[0]++) = left;
		*(buffer[1]++) = right;
	}
}

void *wswan_sh_start(int clock, const struct CustomSound_interface *config)
{
	channel = stream_create(0, 2, Machine->sample_rate, 0, wswan_sh_update);

	snd.audio1.on = 0;
	snd.audio1.signal = 16;
	snd.audio1.pos = 0;
	snd.audio2.on = 0;
	snd.audio2.signal = 16;
	snd.audio2.pos = 0;
	snd.audio3.on = 0;
	snd.audio3.signal = 16;
	snd.audio3.pos = 0;
	snd.audio4.on = 0;
	snd.audio4.signal = 16;
	snd.audio4.pos = 0;

	return (void *) ~0;
}
