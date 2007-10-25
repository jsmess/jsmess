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

SVISION_DMA svision_dma;

SVISION_NOISE svision_noise;

SVISION_CHANNEL svision_channel[2];

WRITE8_HANDLER( svision_sounddma_w )
{
	logerror("%.6f svision snddma write %04x %02x\n", mame_time_to_double(mame_timer_get_time()),offset+0x18,data);
	svision_dma.reg[offset] = data;
	switch (offset)
	{
		case 0:
		case 1:
			svision_dma.start = (svision_dma.reg[0] | (svision_dma.reg[1] << 8));
			break;
		case 2:
			svision_dma.size = (data ? data : 0x100) * 32;
			break;
		case 3:
			svision_dma.step = Machine->drv->cpu[0].clock / (256.0 * Machine->sample_rate * (1 + (data & 3)));
			svision_dma.right = data & 4;
			svision_dma.left = data & 8;
			svision_dma.ca14to16 = ((data & 0x70) >> 4) << 14;
			break;
		case 4:
			svision_dma.on = data & 0x80;
			if (svision_dma.on)
			{ 
				svision_dma.pos = 0.0; 
			}
			break;
	}
}

WRITE8_HANDLER( svision_noise_w )
{
	//  logerror("%.6f svision noise write %04x %02x\n",mame_timer_get_time(),offset+0x28,data);
	svision_noise.reg[offset]=data;
	switch (offset)
	{
		case 0:
			svision_noise.volume=data&0xf;
			svision_noise.step=Machine->drv->cpu[0].clock/(256.0*Machine->sample_rate*(1+(data>>4)));
			break;
		case 1:
			svision_noise.count = data + 1;
			break;
		case 2:
			svision_noise.type = (SVISION_NOISE_Type) (data & 1);
			svision_noise.play = data & 2;
			svision_noise.right = data & 4;
			svision_noise.left = data & 8;
			svision_noise.on = data & 0x10; /* honey bee start */
			svision_noise.state = 1;
			break;
	}
	svision_noise.pos=0.0;
}

void svision_soundport_w (SVISION_CHANNEL *channel, int offset, int data)
{
	UINT16 size;

	stream_update(mixer_channel);
	channel->reg[offset] = data;

	switch (offset)
	{
		case 0:
		case 1:
			size = channel->reg[0] | ((channel->reg[1] & 7) << 8);
			if (size)
			{
				//	channel->size=(int)(Machine->sample_rate*(size<<5)/4e6);
				channel->size= (int) (Machine->sample_rate * (size << 5) / Machine->drv->cpu[0].clock);
			}
			else
			{
				channel->size = 0;
			}
			channel->pos = 0;
			break;
		case 2:
			channel->on = data & 0x40;
			channel->waveform = (data & 0x30) >> 4;
			channel->volume = data & 0xf;
			break;
		case 3:
			channel->count = data + 1;
			break;
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
			if (channel->size != 0)
			{
				if (channel->on||channel->count)
				{
					int on = FALSE;
					switch (channel->waveform)
					{
						case 0:
							on = channel->pos <= (28 * channel->size) >> 5;
							break;
						case 1:
							on = channel->pos <= (24 * channel->size) >> 5;
							break;
						default:
						case 2:
							on = channel->pos <= channel->size / 2;
							break;
						case 3:
							on = channel->pos <= (9 * channel->size) >> 5;
							break;
					}
					{
						INT16 s = on ? channel->volume << 8 : 0;
						if (j == 0)
							*right += s;
						else
							*left += s;
					}
				}
				channel->pos++;
				if (channel->pos >= channel->size)
					channel->pos = 0;
			}
		}
		if (svision_noise.on && (svision_noise.play || svision_noise.count))
		{
			INT16 s = (svision_noise.value ? 1 << 8: 0) * svision_noise.volume;
			int b1, b2;
			if (svision_noise.left)
				*left += s;
			if (svision_noise.right)
				*right += s;
			svision_noise.pos += svision_noise.step;
			if (svision_noise.pos >= 1.0)
			{
				switch (svision_noise.type)
				{
					case SVISION_NOISE_Type7Bit:
						svision_noise.value = svision_noise.state & 0x40 ? 1 : 0;
						b1 = (svision_noise.state & 0x40) != 0;
						b2 = (svision_noise.state & 0x20) != 0;
						svision_noise.state=(svision_noise.state<<1)+(b1!=b2?1:0);
						break;
					case SVISION_NOISE_Type14Bit:
					default:
						svision_noise.value = svision_noise.state & 0x2000 ? 1 : 0;
						b1 = (svision_noise.state & 0x2000) != 0;
						b2 = (svision_noise.state & 0x1000) != 0;
						svision_noise.state = (svision_noise.state << 1) + (b1 != b2 ? 1 : 0);
				}
				svision_noise.pos -= 1;
			}
		}
		if (svision_dma.on)
		{
			UINT8 sample;
			INT16 s;
			UINT16 addr = svision_dma.start + (unsigned) svision_dma.pos / 2;
			if (addr >= 0x8000 && addr < 0xc000)
			{
				sample = memory_region(REGION_USER1)[(addr & 0x3fff) | svision_dma.ca14to16];
			}
			else
			{
				sample = program_read_byte(addr);
			}
			if (((unsigned)svision_dma.pos) & 1)
				s = (sample & 0xf);
			else
				s = (sample & 0xf0) >> 4;
			s <<= 8;
			if (svision_dma.left)
				*left += s;
			if (svision_dma.right)
				*right += s;
			svision_dma.pos += svision_dma.step;
			if (svision_dma.pos >= svision_dma.size)
			{
				svision_dma.finished = TRUE;
				svision_dma.on = FALSE;
				svision_irq();
			}
		}
	}
}

/************************************/
/* Sound handler start              */
/************************************/
void *svision_custom_start(int clock, const struct CustomSound_interface *config)
{
	memset(&svision_dma, 0, sizeof(svision_dma));
	memset(&svision_noise, 0, sizeof(svision_noise));
	memset(svision_channel, 0, sizeof(svision_channel));

	mixer_channel = stream_create(0, 2, Machine->sample_rate, 0, svision_update);
	return (void *) ~0;
}
