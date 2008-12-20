/******************************************************************************
 *	kaypro.c
 *
 *	KAYPRO terminal beeper emulation
 *
 ******************************************************************************/

#include "driver.h"
#include "includes/kaypro.h"
#include "sound/custom.h"
#include "streams.h"
#include "deprecat.h"

#ifdef UNUSED_FUNCTION
static	sound_stream  *channel;
#endif
#define BELL_FREQ	1000
static	INT32 bell_signal;
#ifdef UNUSED_FUNCTION
static	INT32 bell_counter;
#endif

#define CLICK_FREQ	2000
static	INT32 click_signal;
#ifdef UNUSED_FUNCTION
static	INT32 click_counter;
#endif

#ifdef UNUSED_FUNCTION
static void kaypro_sound_update(void *param,stream_sample_t **inputs, stream_sample_t **outputs,int samples)
{
	stream_sample_t *buffer = outputs[0];

	while (samples-- > 0)
	{
		if ((bell_counter -= BELL_FREQ) < 0)
		{
			bell_counter += Machine->sample_rate;
			bell_signal = -(bell_signal * 127) / 128;
		}
		if ((click_counter -= CLICK_FREQ) < 0)
		{
			click_counter += Machine->sample_rate;
			click_signal = -(click_signal * 3) / 4;
		}
		*buffer++ = bell_signal + click_signal;
	}
}


static CUSTOM_START( kaypro_sh_start )
{
	channel = stream_create(device, 0, 1, device->machine->sample_rate, 0, kaypro_sound_update);
	return 0;
}
#endif


/******************************************************
 *	Ring my bell ;)
 ******************************************************/

void kaypro_bell(void)
{
	bell_signal = 0x3000;
}



/******************************************************
 *	Clicking keys (for the Kaypro 2x)
 ******************************************************/

void kaypro_click(void)
{
	click_signal = 0x3000;
}



