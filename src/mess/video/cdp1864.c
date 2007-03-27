#include "driver.h"
#include "video/generic.h"
#include "cpu/cdp1802/cdp1802.h"
#include "video/cdp1864.h"
#include "sound/beep.h"

/*

	RCA CDP1864 Video Chip

	http://homepage.mac.com/ruske/cosmacelf/cdp1864.pdf

*/

static CDP1864_CONFIG cdp1864;

PALETTE_INIT( tmc2000 )
{
	int background_color_sequence[] = { 5, 7, 6, 3 };

	palette_set_color(machine,  0, 0x4c, 0x96, 0x1c ); // white
	palette_set_color(machine,  1, 0x4c, 0x00, 0x1c ); // purple
	palette_set_color(machine,  2, 0x00, 0x96, 0x1c ); // cyan
	palette_set_color(machine,  3, 0x00, 0x00, 0x1c ); // blue
	palette_set_color(machine,  4, 0x4c, 0x96, 0x00 ); // yellow
	palette_set_color(machine,  5, 0x4c, 0x00, 0x00 ); // red
	palette_set_color(machine,  6, 0x00, 0x96, 0x00 ); // green
	palette_set_color(machine,  7, 0x00, 0x00, 0x00 ); // black

	cdp1864_set_background_color_sequence_w(background_color_sequence);
}

PALETTE_INIT( tmc2000e )	// TODO: incorrect colors?
{
	int background_color_sequence[] = { 2, 0, 1, 4 };

	palette_set_color(machine, 0, 0x00, 0x00, 0x00 ); // black		  0 % of max luminance
	palette_set_color(machine, 1, 0x00, 0x97, 0x00 ); // green		 59
	palette_set_color(machine, 2, 0x00, 0x00, 0x1c ); // blue		 11
	palette_set_color(machine, 3, 0x00, 0xb3, 0xb3 ); // cyan		 70
	palette_set_color(machine, 4, 0x4c, 0x00, 0x00 ); // red		 30
	palette_set_color(machine, 5, 0xe3, 0xe3, 0x00 ); // yellow		 89
	palette_set_color(machine, 6, 0x68, 0x00, 0x68 ); // magenta	 41
	palette_set_color(machine, 7, 0xff, 0xff, 0xff ); // white		100
	
	cdp1864_set_background_color_sequence_w(background_color_sequence);
}

void cdp1864_set_background_color_sequence_w(int color[])
{
	int i;
	for (i = 0; i < 4; i++)
		cdp1864.bgcolseq[i] = color[i];
}

WRITE8_HANDLER( cdp1864_step_background_color_w )
{
	cdp1864.display = 1;
	if (++cdp1864.bgcolor > 3) cdp1864.bgcolor = 0;
}

WRITE8_HANDLER( cdp1864_tone_divisor_latch_w )
{
	beep_set_frequency(0, CDP1864_CLK_FREQ / 8 / 4 / (data + 1) / 2);
}

void cdp1864_audio_output_w(int value)
{
	beep_set_state(0, value);
}

READ8_HANDLER( cdp1864_audio_enable_r )
{
	beep_set_state(0, 1);
	return 0;
}

READ8_HANDLER( cdp1864_audio_disable_r )
{
	beep_set_state(0, 0);
	return 0;
}

void cdp1864_enable_w(int value)
{
	cdp1864.display = value;
}

void cdp1864_reset_w(void)
{
	int i;

	cdp1864.bgcolor = 0;

	for (i = 0; i < 4; i++)
		cdp1864.bgcolseq[i] = i;

	cdp1864_audio_output_w(0);
	cdp1864_tone_divisor_latch_w(0, CDP1864_DEFAULT_LATCH);
}

VIDEO_UPDATE( cdp1864 )
{
	fillbitmap(bitmap, cdp1864.bgcolseq[cdp1864.bgcolor], cliprect);
	// TODO: draw videoram to screen using DMA
	return 0;
}
