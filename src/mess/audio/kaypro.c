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

static	sound_stream  *channel;
#define BELL_FREQ	1000
static	INT32 bell_signal;
static	INT32 bell_counter;

#define CLICK_FREQ	2000
static	INT32 click_signal;
static	INT32 click_counter;

static void kaypro_sound_update(void *param,stream_sample_t **inputs, stream_sample_t **_buffer,int length)
{
	stream_sample_t *buffer = _buffer[0];

	while (length-- > 0)
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



static int kaypro_sh_start(int clock, const struct CustomSound_interface *config)
{
	channel = stream_create(0, 1, Machine->sample_rate, 0, kaypro_sound_update);
	return 0;
}



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



