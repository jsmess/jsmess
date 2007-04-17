#include "driver.h"
#include "video/generic.h"
#include "cpu/cdp1802/cdp1802.h"
#include "video/cdp1864.h"
#include "sound/beep.h"

static mame_timer *cdp1864_int_timer;
static mame_timer *cdp1864_efx_timer;
static mame_timer *cdp1864_dma_timer;

typedef struct
{
	int disp;
	int audio;
	int bgcolor;
	int latch;
	int dmaout;
	int dmaptr;
	UINT8 data[CDP1864_VISIBLE_LINES * 8];
	int (*colorram_r)(UINT16 addr);
} CDP1864_CONFIG;

static CDP1864_CONFIG cdp1864;

static int cdp1864_bgcolseq[] = { 2, 0, 1, 4 };

int cdp1864_efx;

static void cdp1864_int_tick(int ref)
{
	int scanline = video_screen_get_vpos(0);

	if (scanline == CDP1864_SCANLINE_INT_START)
	{
		if (cdp1864.disp)
		{
			cpunum_set_input_line(0, CDP1802_INPUT_LINE_INT, HOLD_LINE);
		}

		mame_timer_adjust(cdp1864_int_timer, video_screen_get_time_until_pos(0, CDP1864_SCANLINE_INT_END, 0), 0, time_zero);
	}
	else
	{
		if (cdp1864.disp)
		{
			cpunum_set_input_line(0, CDP1802_INPUT_LINE_INT, CLEAR_LINE);
		}

		mame_timer_adjust(cdp1864_int_timer, video_screen_get_time_until_pos(0, CDP1864_SCANLINE_INT_START, 0), 0, time_zero);
	}
}

static void cdp1864_efx_tick(int ref)
{
	int scanline = video_screen_get_vpos(0);

	switch (scanline)
	{
	case CDP1864_SCANLINE_EFX_TOP_START:
		cdp1864_efx = ASSERT_LINE;
		mame_timer_adjust(cdp1864_efx_timer, video_screen_get_time_until_pos(0, CDP1864_SCANLINE_EFX_TOP_END, 0), 0, time_zero);
		break;

	case CDP1864_SCANLINE_EFX_TOP_END:
		cdp1864_efx = CLEAR_LINE;
		mame_timer_adjust(cdp1864_efx_timer, video_screen_get_time_until_pos(0, CDP1864_SCANLINE_EFX_BOTTOM_START, 0), 0, time_zero);
		break;

	case CDP1864_SCANLINE_EFX_BOTTOM_START:
		cdp1864_efx = ASSERT_LINE;
		mame_timer_adjust(cdp1864_efx_timer, video_screen_get_time_until_pos(0, CDP1864_SCANLINE_EFX_BOTTOM_END, 0), 0, time_zero);
		break;

	case CDP1864_SCANLINE_EFX_BOTTOM_END:
		cdp1864_efx = CLEAR_LINE;
		mame_timer_adjust(cdp1864_efx_timer, video_screen_get_time_until_pos(0, CDP1864_SCANLINE_EFX_TOP_START, 0), 0, time_zero);
		break;
	}
}

static void cdp1864_dma_tick(int ref)
{
	if (cdp1864.dmaout)
	{
		cpunum_set_input_line(0, CDP1802_INPUT_LINE_DMAOUT, CLEAR_LINE);
		mame_timer_adjust(cdp1864_dma_timer, MAME_TIME_IN_CYCLES(CDP1864_CYCLES_DMAOUT_HIGH, 0), 0, time_zero);
		cdp1864.dmaout = 0;
	}
	else
	{
		cpunum_set_input_line(0, CDP1802_INPUT_LINE_DMAOUT, HOLD_LINE);
		mame_timer_adjust(cdp1864_dma_timer, MAME_TIME_IN_CYCLES(CDP1864_CYCLES_DMAOUT_LOW, 0), 0, time_zero);
		cdp1864.dmaout = 1;
	}
}

void cdp1864_dma_w(UINT8 data)
{
	cdp1864.data[cdp1864.dmaptr] = data;
	cdp1864.dmaptr++;

	if (cdp1864.dmaptr == CDP1864_VISIBLE_LINES * 8)
	{
		mame_timer_adjust(cdp1864_dma_timer, double_to_mame_time(TIME_NEVER), 0, time_zero);
	}
}

void cdp1864_sc(int state)
{
	if (state == CDP1802_STATE_CODE_S3_INTERRUPT)
	{
		cdp1864.dmaout = 0;
		cdp1864.dmaptr = 0;

		mame_timer_adjust(cdp1864_dma_timer, MAME_TIME_IN_CYCLES(CDP1864_CYCLES_INT_DELAY, 0), 0, time_zero);
	}
}

MACHINE_RESET( cdp1864 )
{
	cdp1864_int_timer = mame_timer_alloc(cdp1864_int_tick);
	cdp1864_efx_timer = mame_timer_alloc(cdp1864_efx_tick);
	cdp1864_dma_timer = mame_timer_alloc(cdp1864_dma_tick);

	mame_timer_adjust(cdp1864_int_timer, video_screen_get_time_until_pos(0, CDP1864_SCANLINE_INT_START, 0), 0, time_zero);
	mame_timer_adjust(cdp1864_efx_timer, video_screen_get_time_until_pos(0, CDP1864_SCANLINE_EFX_TOP_START, 0), 0, time_zero);
	mame_timer_adjust(cdp1864_dma_timer, double_to_mame_time(TIME_NEVER), 0, time_zero);

	cdp1864.disp = 0;
	cdp1864.dmaout = 0;
	cdp1864.bgcolor = 0;

	cdp1864_efx = CLEAR_LINE;

	cdp1864_audio_output_enable(0);
}

WRITE8_HANDLER( cdp1864_step_bgcolor_w )
{
	cdp1864.disp = 1;
	if (++cdp1864.bgcolor > 3) cdp1864.bgcolor = 0;
}

WRITE8_HANDLER( cdp1864_tone_latch_w )
{
	cdp1864.latch = data;
	beep_set_frequency(0, CDP1864_CLK_FREQ / 8 / 4 / (data + 1) / 2);
}

void cdp1864_audio_output_enable(int value)
{
	if (value == 0)
	{
		cdp1864.latch = CDP1864_DEFAULT_LATCH;
	}
	
	cdp1864.audio = value;

	beep_set_state(0, value);
}

READ8_HANDLER( cdp1864_dispon_r )
{
	cdp1864.disp = 1;

	return 0xff;
}

READ8_HANDLER( cdp1864_dispoff_r )
{
	cdp1864.disp = 0;

	return 0xff;
}

VIDEO_START( cdp1864 )
{
	state_save_register_global(cdp1864.disp);
	state_save_register_global(cdp1864.audio);
	state_save_register_global(cdp1864.bgcolor);
	state_save_register_global(cdp1864.latch);
	state_save_register_global(cdp1864.dmaout);
	state_save_register_global(cdp1864.dmaptr);
	state_save_register_global_array(cdp1864.data);
	state_save_register_global(cdp1864_efx);

	return 0;
}

VIDEO_UPDATE( cdp1864 )
{
	if (cdp1864.disp)
	{
		int i, bit;

		fillbitmap(bitmap, cdp1864_bgcolseq[cdp1864.bgcolor], cliprect);

		for (i = 0; i < CDP1864_VISIBLE_LINES * 8; i++)
		{
			int x = (i % 8) * 8;
			int y = i / 8;

			UINT8 data = cdp1864.data[i];

			for (bit = 0; bit < 8; bit++)
			{
				if (data & 0x80)
				{
					int color = cdp1864.colorram_r(i);
					plot_pixel(bitmap, CDP1864_SCREEN_START + x + bit, CDP1864_SCANLINE_SCREEN_START + y, Machine->pens[color]);
				}
				
				data <<= 1;
			}
		}
	}
	else
	{
		fillbitmap(bitmap, get_black_pen(Machine), cliprect);
	}

	return 0;
}

static void cdp1864_init_palette(double res_r, double res_g, double res_b, double res_bkg)
{
	int i, r, g, b, luma;

	double res_total = res_r + res_g + res_b;

	int weight_r = (res_r / res_total) * 100;
	int weight_g = (res_g / res_total) * 100;
	int weight_b = (res_b / res_total) * 100;

	for (i = 0; i < 8; i++)
	{
		luma = (i & 4) ? weight_r : 0;
		luma += (i & 1) ? weight_g : 0;
		luma += (i & 2) ? weight_b : 0;

		r = (i & 4) ? luma : 0;
		g = (i & 1) ? luma : 0;
		b = (i & 2) ? luma : 0;

		luma = (luma * 0xff) / 100;

		palette_set_color( Machine, i, r, g, b );
	}
}

void cdp1864_configure(const CDP1864_interface *intf)
{
	cdp1864.colorram_r = intf->colorram_r;
	cdp1864_init_palette(intf->res_r, intf->res_g, intf->res_b, intf->res_bkg);
}
