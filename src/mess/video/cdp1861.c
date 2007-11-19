#include "driver.h"
#include "cpu/cdp1802/cdp1802.h"
#include "video/generic.h"
#include "video/cdp1861.h"

static mame_bitmap *cdptmpbitmap;

static emu_timer *cdp1861_int_timer;
static emu_timer *cdp1861_efx_timer;
static emu_timer *cdp1861_dma_timer;

typedef struct
{
	int disp;
	int dmaout;
} CDP1861_VIDEO_CONFIG;

static CDP1861_VIDEO_CONFIG cdp1861;

int cdp1861_efx;

static TIMER_CALLBACK(cdp1861_int_tick)
{
	int scanline = video_screen_get_vpos(0);

	if (scanline == CDP1861_SCANLINE_INT_START)
	{
		if (cdp1861.disp)
		{
			cpunum_set_input_line(0, CDP1802_INPUT_LINE_INT, HOLD_LINE);
		}

		timer_adjust(cdp1861_int_timer, video_screen_get_time_until_pos(0, CDP1861_SCANLINE_INT_END, 0), 0, attotime_zero);
	}
	else
	{
		if (cdp1861.disp)
		{
			cpunum_set_input_line(0, CDP1802_INPUT_LINE_INT, CLEAR_LINE);
		}

		timer_adjust(cdp1861_int_timer, video_screen_get_time_until_pos(0, CDP1861_SCANLINE_INT_START, 0), 0, attotime_zero);
	}
}

static TIMER_CALLBACK(cdp1861_efx_tick)
{
	int scanline = video_screen_get_vpos(0);

	switch (scanline)
	{
	case CDP1861_SCANLINE_EFX_TOP_START:
		cdp1861_efx = ASSERT_LINE;
		timer_adjust(cdp1861_efx_timer, video_screen_get_time_until_pos(0, CDP1861_SCANLINE_EFX_TOP_END, 0), 0, attotime_zero);
		break;

	case CDP1861_SCANLINE_EFX_TOP_END:
		cdp1861_efx = CLEAR_LINE;
		timer_adjust(cdp1861_efx_timer, video_screen_get_time_until_pos(0, CDP1861_SCANLINE_EFX_BOTTOM_START, 0), 0, attotime_zero);
		break;

	case CDP1861_SCANLINE_EFX_BOTTOM_START:
		cdp1861_efx = ASSERT_LINE;
		timer_adjust(cdp1861_efx_timer, video_screen_get_time_until_pos(0, CDP1861_SCANLINE_EFX_BOTTOM_END, 0), 0, attotime_zero);
		break;

	case CDP1861_SCANLINE_EFX_BOTTOM_END:
		cdp1861_efx = CLEAR_LINE;
		timer_adjust(cdp1861_efx_timer, video_screen_get_time_until_pos(0, CDP1861_SCANLINE_EFX_TOP_START, 0), 0, attotime_zero);
		break;
	}
}

static TIMER_CALLBACK(cdp1861_dma_tick)
{
	int scanline = video_screen_get_vpos(0);

	if (cdp1861.dmaout)
	{
		if (cdp1861.disp)
		{
			if (scanline >= CDP1861_SCANLINE_DISPLAY_START && scanline < CDP1861_SCANLINE_DISPLAY_END)
			{
				cpunum_set_input_line(0, CDP1802_INPUT_LINE_DMAOUT, CLEAR_LINE);
			}
		}

		timer_adjust(cdp1861_dma_timer, ATTOTIME_IN_CYCLES(CDP1861_CYCLES_DMA_WAIT, 0), 0, attotime_zero);
		
		cdp1861.dmaout = 0;
	}
	else
	{
		if (cdp1861.disp)
		{
			if (scanline >= CDP1861_SCANLINE_DISPLAY_START && scanline < CDP1861_SCANLINE_DISPLAY_END)
			{
				cpunum_set_input_line(0, CDP1802_INPUT_LINE_DMAOUT, HOLD_LINE);
			}
		}

		timer_adjust(cdp1861_dma_timer, ATTOTIME_IN_CYCLES(CDP1861_CYCLES_DMA_ACTIVE, 0), 0, attotime_zero);
		
		cdp1861.dmaout = 1;
	}
}

void cdp1861_dma_w(UINT8 data)
{
	int sx = video_screen_get_hpos(0) + 4;
	int y = video_screen_get_vpos(0);
	int x;

	for (x = 0; x < 8; x++)
	{
		int color = (data & 0x80) >> 7;
		*BITMAP_ADDR16(cdptmpbitmap, y, sx + x) = Machine->pens[color];
		data <<= 1;
	}
}

MACHINE_RESET( cdp1861 )
{
	timer_adjust(cdp1861_int_timer, video_screen_get_time_until_pos(0, CDP1861_SCANLINE_INT_START, 0), 0, attotime_zero);
	timer_adjust(cdp1861_efx_timer, video_screen_get_time_until_pos(0, CDP1861_SCANLINE_EFX_TOP_START, 0), 0, attotime_zero);
	timer_adjust(cdp1861_dma_timer, ATTOTIME_IN_CYCLES(CDP1861_CYCLES_DMA_START, 0), 0, attotime_zero);

	cdp1861.disp = 0;
	cdp1861.dmaout = 0;

	cpunum_set_input_line(0, CDP1802_INPUT_LINE_INT, CLEAR_LINE);
	cpunum_set_input_line(0, CDP1802_INPUT_LINE_DMAOUT, CLEAR_LINE);
	cdp1861_efx = CLEAR_LINE;
}

READ8_HANDLER( cdp1861_dispon_r )
{
	cdp1861.disp = 1;

	return 0xff;
}

WRITE8_HANDLER( cdp1861_dispoff_w )
{
	cdp1861.disp = 0;
	cpunum_set_input_line(0, CDP1802_INPUT_LINE_INT, CLEAR_LINE);
	cpunum_set_input_line(0, CDP1802_INPUT_LINE_DMAOUT, CLEAR_LINE);
}

VIDEO_START( cdp1861 )
{
	cdp1861_int_timer = timer_alloc(cdp1861_int_tick);
	cdp1861_efx_timer = timer_alloc(cdp1861_efx_tick);
	cdp1861_dma_timer = timer_alloc(cdp1861_dma_tick);

	/* allocate the temporary bitmap */
	cdptmpbitmap = auto_bitmap_alloc(Machine->screen[0].width, Machine->screen[0].height, Machine->screen[0].format);

	/* ensure the contents of the bitmap are saved */
	state_save_register_bitmap("video", 0, "cdptmpbitmap", cdptmpbitmap);

	state_save_register_global(cdp1861.disp);
	state_save_register_global(cdp1861.dmaout);
}

VIDEO_UPDATE( cdp1861 )
{
	if (cdp1861.disp)
	{
		copybitmap(bitmap, cdptmpbitmap, 0, 0, 0, 0, cliprect, TRANSPARENCY_NONE, 0);
	}
	else
	{
		fillbitmap(bitmap, get_black_pen(Machine), cliprect);
	}

	return 0;
}
