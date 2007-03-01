/*
    Ricoh RF5C400 emulator

    Written by Ville Linde
    Additional code by the hoot development team
*/

#include "sndintrf.h"
#include "streams.h"
#include "rf5c400.h"
#include <math.h>

struct rf5c400_info
{
	const struct RF5C400interface *intf;

	INT16 *rom;
	UINT32 rom_length;

	sound_stream *stream;

	int current_channel;
	int keyon_channel;

	struct RF5C400_CHANNEL
	{
		UINT16	startH;
		UINT16	startL;
		UINT16	freq;
		UINT16	endL;
		UINT16	endHloopH;
		UINT16	loopL;
		UINT16	pan;
		UINT16	effect;
		UINT16	volume;
		UINT16	flag;

		UINT64 pos;
		UINT64 step;
		UINT16 keyon;
	} channels[32];
};

static int volume_table[256];
static double pan_table[0x64];

enum {
	FLG_TIMBRE		= 0x0400,
};

/* PCM type */
enum {
	TYPE_MASK		= 0x00C0,
	TYPE_16			= 0x0000,
	TYPE_8LOW		= 0x0040,
	TYPE_8HIGH		= 0x0080,
};

/* key control */
enum {
	KEY_OFF = 0,
	KEY_ON,
	KEY_REL,
};


/*****************************************************************************/

static void rf5c400_update(void *param, stream_sample_t **inputs, stream_sample_t **buffer, int length)
{
	int i, ch;
	struct rf5c400_info *info = param;
	INT16 *rom = info->rom;
	UINT32 start, end, loop;
	UINT64 pos;
	UINT8 vol, lvol, rvol, type;

	memset(buffer[0], 0, length*sizeof(*buffer[0]));
	memset(buffer[1], 0, length*sizeof(*buffer[1]));

	for (ch=0; ch < 32; ch++)
	{
		struct RF5C400_CHANNEL *channel = &info->channels[ch];

		start = ((channel->startH & 0xFF00) << 8) | channel->startL;
		end = ((channel->endHloopH & 0xFF) << 16) | channel->endL;
		loop = ((channel->endHloopH & 0xFF00) << 8) | channel->loopL;
		pos = channel->pos;
		vol = channel->volume & 0xFF;
		lvol = channel->pan >> 8;
		rvol = channel->pan & 0xFF;
		type = channel->volume >> 8;

		for (i=0; i < length; i++)
		{
			INT16 tmp;
			INT32 sample;

			if ( (channel->keyon == KEY_OFF) ||
				 ((channel->keyon == KEY_REL) && !(channel->flag & FLG_TIMBRE)) )
			{
				break;
			}


			tmp = rom[pos>>16];
			switch ( type & TYPE_MASK )
			{
				case TYPE_16:
					sample = tmp;
					break;
				case TYPE_8LOW:
					sample = (INT16)(tmp << 8);
					break;
				case TYPE_8HIGH:
					sample = (INT16)(tmp & 0xFF00);
					break;
				default:
					sample = 0;
					break;
			}

			if ( sample & 0x8000 )
			{
				sample ^= 0x7FFF;
			}

			sample *= volume_table[vol];
			sample >>= 6;
			buffer[0][i] += (sample * pan_table[lvol]);
			buffer[1][i] += (sample * pan_table[rvol]);

			pos += channel->step;
			if ( (pos>>16) > info->rom_length || (pos>>16) > end)
			{
				if ( channel->keyon == KEY_ON )
				{
					pos -= loop<<16;
					pos &= 0xFFFFFF0000ULL;
				} else {
					channel->keyon = KEY_OFF;
				}
			}
		}
		channel->pos = pos;
	}

	for (i=0; i < length; i++)
	{
		buffer[0][i] >>= 3;
		buffer[1][i] >>= 3;
	}
}

static void rf5c400_init_chip(struct rf5c400_info *info, int sndindex, int clock)
{
	int i;

	info->rom = (INT16*)memory_region(info->intf->region);
	info->rom_length = memory_region_length(info->intf->region) / 2;

	// init volume table
	{
		double max=255.0;
		for (i = 0; i < 256; i++) {
			volume_table[i]=(UINT16)max;
			max /= pow(10.0,(double)((2.5/(256.0/16.0))/20));
		}
		for(i=0; i<0x64; i++) {
			pan_table[i] = sqrt( (double)(0x64 - i) ) / sqrt( (double)0x64 );
		}
	}

	info->stream = stream_create(0, 2, clock/384, info, rf5c400_update);
}


static void *rf5c400_start(int sndindex, int clock, const void *config)
{
	struct rf5c400_info *info;

	info = auto_malloc(sizeof(*info));
	memset(info, 0, sizeof(*info));

	info->intf = config;

	rf5c400_init_chip(info, sndindex, clock);

	return info;
}

/*****************************************************************************/

static UINT16 rf5c400_status = 0;
static UINT16 rf5c400_r(int chipnum, int offset)
{
	switch(offset)
	{
		case 0x00:
		{
			return rf5c400_status;
		}

		case 0x04:
		{
			return 0;
		}
	}

	return 0;
}

static void rf5c400_w(int chipnum, int offset, UINT16 data)
{
	struct rf5c400_info *info = sndti_token(SOUND_RF5C400, chipnum);

	if (offset < 0x400)
	{
		switch(offset)
		{
			case 0x00:
			{
				rf5c400_status = data;
				break;
			}

			case 0x01:		// channel control
			{
				switch ( data & 0x60 )
				{
					case 0x60:
						info->channels[data&0x1f].pos =
							((info->channels[data&0x1f].startH & 0xFF00) << 8) | info->channels[data&0x1f].startL;
						info->channels[data&0x1f].pos <<= 16;
						info->channels[data&0x1f].keyon = KEY_ON;
						break;
					case 0x40:
						info->channels[data&0x1f].keyon = KEY_REL;
						break;
					default:
						info->channels[data&0x1f].keyon = KEY_OFF;
						break;
				}
				break;
			}
			default:
			{
				//mame_printf_debug("rf5c400_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
				break;
			}
		}
		//mame_printf_debug("rf5c400_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
	}
	else
	{
		// channel registers
		int ch = (offset >> 5) & 0x1f;
		int reg = (offset & 0x1f);

		struct RF5C400_CHANNEL *channel = &info->channels[ch];

		switch (reg)
		{
			case 0x00:		// sample start address, bits 23 - 16
			{
				channel->startH = data;
				break;
			}
			case 0x01:		// sample start address, bits 15 - 0
			{
				channel->startL = data;
				break;
			}
			case 0x02:		// sample playing frequency
			{
				channel->step = ((data & 0x1fff) << (data >> 13)) * 4;
				channel->freq = data;
				break;
			}
			case 0x03:		// sample end address, bits 15 - 0
			{
				channel->endL = data;
				break;
			}
			case 0x04:		// sample end address, bits 23 - 16 , sample loop 23 - 16
			{
				channel->endHloopH = data;
				break;
			}
			case 0x05:		// sample loop offset, bits 15 - 0
			{
				channel->loopL = data;
				break;
			}
			case 0x06:		// channel volume
			{
				channel->pan = data;
				break;
			}
			case 0x07:		// effect depth
			{
				channel->effect = data;
				break;
			}
			case 0x08:		// volume, flag
			{
				channel->volume = data;
				break;
			}
			case 0x0E:		// flag
			{
				channel->flag = data;
				break;
			}
			case 0x10:		// unknown
			{
				break;
			}
		}
	}
}

READ16_HANDLER( RF5C400_0_r )
{
	return rf5c400_r(0, offset);
}

WRITE16_HANDLER( RF5C400_0_w )
{
	rf5c400_w(0, offset, data);
}

/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void rf5c400_set_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void rf5c400_get_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = rf5c400_set_info;		break;
		case SNDINFO_PTR_START:							info->start = rf5c400_start;			break;
		case SNDINFO_PTR_STOP:							/* nothing */							break;
		case SNDINFO_PTR_RESET:							/* nothing */							break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "RF5C400";					break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "Ricoh PCM";					break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2004, The MAME Team"; break;
	}
}
