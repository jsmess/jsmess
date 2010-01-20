/***************************************************************************

  MOS ted 7360 (and sound interface)
  PeT mess@utanet.at

  main part in video/

***************************************************************************/

#include "emu.h"
#include "streams.h"

#include "video/ted7360.h"


#define VERBOSE_LEVEL 0
#define DBG_LOG(N,M,A) \
	do { \
		if(VERBOSE_LEVEL >= N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s", attotime_to_double(timer_get_time(machine)), (char*) M ); \
			logerror A; \
		} \
	} while (0)


/* noise channel: look into vic6560.c */
#define NOISE_BUFFER_SIZE_SEC 5

#define TONE_ON (!(ted7360[0x11]&0x80))		/* or tone update!? */
#define TONE1_ON ((ted7360[0x11]&0x10))
#define TONE1_VALUE (ted7360[0xe]|((ted7360[0x12]&3)<<8))
#define TONE2_ON ((ted7360[0x11]&0x20))
#define TONE2_VALUE (ted7360[0xf]|((ted7360[0x10]&3)<<8))
#define VOLUME (ted7360[0x11]&0x0f)
#define NOISE_ON (ted7360[0x11]&0x40)

/*
 * pal 111860.781
 * ntsc 111840.45
 */
#define TONE_FREQUENCY(reg) ((TED7360_CLOCK>>3)/(1024-reg))
#define TONE_FREQUENCY_MIN (TONE_FREQUENCY(0))
#define NOISE_FREQUENCY (TED7360_CLOCK/8/(1024-TONE2_VALUE))
#define NOISE_FREQUENCY_MAX (TED7360_CLOCK/8)

static int noisesize,	/* number of samples */
	tone1pos = 0, tone2pos = 0,		   /* pos of tone */
	tone1samples = 1, tone2samples = 1,   /* count of samples to give out per tone */
	noisepos = 0, noisesamples = 1;

static sound_stream *channel;
static UINT8 *noise;

void ted7360_soundport_w (running_machine *machine, int offset, int data)
{
	stream_update(channel);
	/*    int old=ted7360[offset]; */
	switch (offset)
	{
	case 0xe:
	case 0x12:
		if (offset == 0x12)
			ted7360[offset] = (ted7360[offset] & ~3) | (data & 3);
		else
			ted7360[offset] = data;
		tone1samples = machine->sample_rate / TONE_FREQUENCY (TONE1_VALUE);
		DBG_LOG (1, "ted7360", ("tone1 %d %d sample:%d\n",
					TONE1_VALUE, TONE_FREQUENCY(TONE1_VALUE), tone1samples));

		break;
	case 0xf:
	case 0x10:
		ted7360[offset] = data;
		tone2samples = machine->sample_rate / TONE_FREQUENCY (TONE2_VALUE);
		DBG_LOG (1, "ted7360", ("tone2 %d %d sample:%d\n",
					TONE2_VALUE, TONE_FREQUENCY(TONE2_VALUE), tone2samples));

		noisesamples = (int) ((double) NOISE_FREQUENCY_MAX * machine->sample_rate
							  * NOISE_BUFFER_SIZE_SEC / NOISE_FREQUENCY);
		DBG_LOG (1, "ted7360", ("noise %d sample:%d\n",
					NOISE_FREQUENCY, noisesamples));
		if (!NOISE_ON || ((double) noisepos / noisesamples >= 1.0))
		{
			noisepos = 0;
		}
		break;
	case 0x11:
		ted7360[offset] = data;
		DBG_LOG(1, "ted7360", ("%s volume %d, %s %s %s\n",
				       TONE_ON?"on":"off",
				       VOLUME, TONE1_ON?"tone1":"", TONE2_ON?"tone2":"",
				       NOISE_ON?"noise":""));
		if (!TONE_ON||!TONE1_ON) tone1pos=0;
		if (!TONE_ON||!TONE2_ON) tone2pos=0;
		if (!TONE_ON||!NOISE_ON) noisepos=0;
		break;
	}
}



/************************************/
/* Sound handler update             */
/************************************/
static STREAM_UPDATE( ted7360_update )
{
	int i, v, a;
	stream_sample_t *buffer = outputs[0];

	for (i = 0; i < samples; i++)
	{
		v = 0;
		if (TONE1_ON)
		{
			if (tone1pos<=tone1samples/2 || !TONE_ON) {
			v += 0x2ff; // depends on the volume between sound and noise
			}
			tone1pos++;
			if (tone1pos>tone1samples) tone1pos=0;
		}
		if (TONE2_ON || NOISE_ON )
		{
			if (TONE2_ON)
			{						   /*higher priority ?! */
			if (tone2pos<=tone2samples/2 || !TONE_ON) {
				v += 0x2ff;
			}
			tone2pos++;
			if (tone2pos>tone2samples) tone2pos=0;
			}
			else
			{
			v += noise[(int) ((double) noisepos * noisesize / noisesamples)];
			noisepos++;
			if ((double) noisepos / noisesamples >= 1.0)
			{
				noisepos = 0;
			}
			}
		}

		a = VOLUME;
		if (a>8)
			a=8;
		v = v * a;
		buffer[i] = v;
	}
}

/************************************/
/* Sound handler start              */
/************************************/

static DEVICE_START(ted7360_sound)
{
	int i;

	channel = stream_create(device, 0, 1, device->machine->sample_rate, 0, ted7360_update);

	/* buffer for fastest played sample for 5 second
     * so we have enough data for min 5 second */
	noisesize = NOISE_FREQUENCY_MAX * NOISE_BUFFER_SIZE_SEC;
	noise = auto_alloc_array(device->machine, UINT8, noisesize);

	{
		int noiseshift = 0x7ffff8;
		UINT8 data;

		for (i = 0; i < noisesize; i++)
		{
			data = 0;
			if (noiseshift & 0x400000)
				data |= 0x80;
			if (noiseshift & 0x100000)
				data |= 0x40;
			if (noiseshift & 0x010000)
				data |= 0x20;
			if (noiseshift & 0x002000)
				data |= 0x10;
			if (noiseshift & 0x000800)
				data |= 0x08;
			if (noiseshift & 0x000080)
				data |= 0x04;
			if (noiseshift & 0x000010)
				data |= 0x02;
			if (noiseshift & 0x000004)
				data |= 0x01;
			noise[i] = data;
			if (((noiseshift & 0x400000) == 0) != ((noiseshift & 0x002000) == 0))
				noiseshift = (noiseshift << 1) | 1;
			else
				noiseshift <<= 1;
		}
	}
}


DEVICE_GET_INFO( ted7360_sound )
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(ted7360_sound);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "7360 TED");				break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);						break;
	}
}
