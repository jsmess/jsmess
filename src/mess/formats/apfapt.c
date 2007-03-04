/* .APT tape images */
#include "apfapt.h"

#define WAVEENTRY_LOW  -32768
#define WAVEENTRY_HIGH  32767
#define WAVEENTRY_NULL  0

static INT16 wave_state = WAVEENTRY_HIGH;
static INT16 xor_wave_state = WAVEENTRY_HIGH^WAVEENTRY_LOW;

#define APF_APT_BIT_0_LENGTH ((int)(APF_WAV_FREQUENCY*0.0005))
#define APF_APT_BIT_1_LENGTH ((int)(APF_WAV_FREQUENCY*0.001))

/* 500 microsecond of bit 0 and 1000 microsecond of bit 1 */

static INT16 *apf_emit_level(INT16 *p, int count)
{	
	int i;

	for (i=0; i<count; i++)
	{
		*(p++) = wave_state;
	}
	wave_state = wave_state^xor_wave_state;
	return p;
}

/* 4 periods at 1200hz */ 
static INT16* apf_output_bit(INT16 *p, UINT8 b)
{
	if (b)
	{
		p = apf_emit_level(p,(APF_APT_BIT_1_LENGTH>>1));
		p = apf_emit_level(p,(APF_APT_BIT_1_LENGTH>>1));
	}
	else
	{
		p = apf_emit_level(p,(APF_APT_BIT_0_LENGTH>>1));
		p = apf_emit_level(p,(APF_APT_BIT_0_LENGTH>>1));
	}

    return p;
}

static int apf_get_bit_size_in_samples(UINT8 b)
{
	if (b)
	{
		return APF_APT_BIT_1_LENGTH;
	}

	return APF_APT_BIT_0_LENGTH;
}


/*************************************************************************************/
static int apf_cassette_length;

/* each bit in the data represents a "1" or "0" waveform */
int apf_cassette_calculate_size_in_samples(int length, UINT8 *bytes)
{
	unsigned i;
	int size;


	size = 0;
	apf_cassette_length = length;


	for (i=0; i<length; i++)
	{
		int b;
		UINT8 data;

		data = bytes[i];

		for (b=0; b<8; b++)
		{
			size += apf_get_bit_size_in_samples((data>>7) & 0x01);
			data = data<<1;
		}
	}

	return size;
}

/*************************************************************************************/

int apf_cassette_fill_wave(INT16 *buffer, int length, UINT8 *bytes)
{
	int i;
	INT16 *p;

	p = buffer;

	for (i=0; i<apf_cassette_length; i++)
	{
		UINT8 data;
		int b;

		data = bytes[i];

		for (b=0; b<8; b++)
		{
			p = apf_output_bit(p,(data>>7) & 0x01);
			data = data<<1;
		}
	}

	return p - buffer;
}
