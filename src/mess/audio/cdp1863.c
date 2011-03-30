/*

    TODO:

    - what happens if you connect both clocks?
    - convert to sound device when possible

*/

#include "emu.h"
#include "streams.h"
#include "audio/cdp1863.h"

#define CDP1863_DEFAULT_LATCH	0x35

typedef struct _cdp1863_t cdp1863_t;
struct _cdp1863_t
{
	const cdp1863_interface *intf;	/* interface */

	sound_stream *stream;			/* sound output */

	/* sound state */
	int oe;							/* output enable */
	int latch;						/* sound latch */
	INT16 signal;					/* current signal */
	int incr;						/* initial wave state */
};

INLINE cdp1863_t *get_safe_token(device_t *device)
{
	assert(device != NULL);

	return (cdp1863_t *)downcast<legacy_device_base *>(device)->token();
}

/* Load Tone Latch */

WRITE8_DEVICE_HANDLER( cdp1863_str_w )
{
	cdp1863_t *cdp1863 = get_safe_token(device);
	double frequency;

	cdp1863->latch = data;

	if (cdp1863->intf->clock1 > 0)
	{
		/* CLK1 is pre-divided by 4 */
		frequency = cdp1863->intf->clock1 / 4 / (data + 1) / 2;
	}
	else
	{
		/* CLK2 is pre-divided by 8 */
		frequency = cdp1863->intf->clock2 / 8 / (data + 1) / 2;
	}

	beep_set_frequency(0, frequency);
}

/* Output Enable */

void cdp1863_oe_w(device_t *device, int level)
{
	cdp1863_t *cdp1863 = get_safe_token(device);

	cdp1863->oe = level;

	beep_set_state(0, level);
}

#ifdef UNUSED_FUNCTION
static void cdp1863_sound_update(device_t *device, stream_sample_t **inputs, stream_sample_t **_buffer, int length)
{
	cdp1863_t *cdp1863 = get_safe_token(device);

	INT16 signal = cdp1863->signal;
	stream_sample_t *buffer = _buffer[0];

	memset(buffer, 0, length * sizeof(*buffer));

	if (cdp1863->oe)
	{
		double frequency;
		int rate = device->machine().sample_rate() / 2;

		/* get progress through wave */
		int incr = cdp1863->incr;

		if (cdp1863->intf->clock1 > 0)
		{
			/* CLK1 is pre-divided by 4 */
			frequency = cdp1863->intf->clock1 / 4 / (cdp1863->latch + 1) / 2;
		}
		else
		{
			/* CLK2 is pre-divided by 8 */
			frequency = cdp1863->intf->clock2 / 8 / (cdp1863->latch + 1) / 2;
		}

		if (signal < 0)
		{
			signal = -0x7fff;
		}
		else
		{
			signal = 0x7fff;
		}

		while( length-- > 0 )
		{
			*buffer++ = signal;
			incr -= frequency;
			while( incr < 0 )
			{
				incr += rate;
				signal = -signal;
			}
		}

		/* store progress through wave */
		cdp1863->incr = incr;
		cdp1863->signal = signal;
	}
}
#endif

/* Device Interface */

static DEVICE_START( cdp1863 )
{
	cdp1863_t *cdp1863 = get_safe_token(device);

	/* validate arguments */
	assert(device != NULL);
	assert(device->tag() != NULL);

	cdp1863->intf = device->baseconfig().static_config();

	assert(cdp1863->intf != NULL);
	assert((cdp1863->intf->clock1 > 0) || (cdp1863->intf->clock2 > 0));

	cdp1863->oe = 1;

	/* register for state saving */
	device->save_item(NAME(cdp1863->oe));
	device->save_item(NAME(cdp1863->latch));
	device->save_item(NAME(cdp1863->signal));
	device->save_item(NAME(cdp1863->incr));
}

static DEVICE_RESET( cdp1863 )
{
	cdp1863_t *cdp1863 = get_safe_token(device);

	cdp1863->latch = CDP1863_DEFAULT_LATCH;
}

DEVICE_GET_INFO( cdp1863 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(cdp1863_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(cdp1863);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(cdp1863);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "RCA CDP1863");					break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "RCA CDP1800");					break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);							break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");			break;
	}
}

DEFINE_LEGACY_DEVICE(CDP1863, cdp1863);
