/*

    TODO:

	- interlace mode
	- PAL output, currently using RGB
    - connect to sound system when possible
	- cpu synchronization

		SC1 and SC0 are used to provide CDP1864C-to-CPU synchronization for a jitter-free display.
		During every horizontal sync the CDP1864C samples SC0 and SC1 for SC0 = 1 and SC1 = 0
		(CDP1800 execute state). Detection of a fetch cycle causes the CDP1864C to skip cycles to
		attain synchronization. (i.e. picture moves 8 pixels to the right)

*/

#include "driver.h"
#include "sndintrf.h"
#include "streams.h"
#include "cpu/cdp1802/cdp1802.h"
#include "sound/beep.h"
#include "video/cdp1864.h"

#define CDP1864_DEFAULT_LATCH	0x35

#define CDP1864_CYCLES_DMA_START	2*8
#define CDP1864_CYCLES_DMA_ACTIVE	8*8
#define CDP1864_CYCLES_DMA_WAIT		6*8

typedef struct _cdp1864_t cdp1864_t;
struct _cdp1864_t
{
	const cdp1864_interface *intf;	/* interface */

	const device_config *screen;	/* screen */
	bitmap_t *bitmap;				/* bitmap */
	sound_stream *stream;			/* sound output */

	/* video state */
	int disp;						/* display on */
	int dmaout;						/* DMA request active */
	int bgcolor;					/* background color */

	/* sound state */
	int audio;						/* audio on */
	int latch;						/* sound latch */
	INT16 signal;					/* current signal */
	int incr;						/* initial wave state */

	/* timers */
	emu_timer *int_timer;			/* interrupt timer */
	emu_timer *efx_timer;			/* EFx timer */
	emu_timer *dma_timer;			/* DMA timer */
};

static const int CDP1864_BACKGROUND_COLOR_SEQUENCE[] = { 2, 0, 1, 4 };

INLINE cdp1864_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);

	return (cdp1864_t *)device->token;
}

/* Timer Callbacks */

static TIMER_CALLBACK(cdp1864_int_tick)
{
	const device_config *device = ptr;
	cdp1864_t *cdp1864 = get_safe_token(device);

	int scanline = video_screen_get_vpos(cdp1864->screen);

	if (scanline == CDP1864_SCANLINE_INT_START)
	{
		if (cdp1864->disp)
		{
			cdp1864->intf->on_int_changed(device, HOLD_LINE);
		}

		timer_adjust_oneshot(cdp1864->int_timer, video_screen_get_time_until_pos(cdp1864->screen, CDP1864_SCANLINE_INT_END, 0), 0);
	}
	else
	{
		if (cdp1864->disp)
		{
			cdp1864->intf->on_int_changed(device, CLEAR_LINE);
		}

		timer_adjust_oneshot(cdp1864->int_timer, video_screen_get_time_until_pos(cdp1864->screen, CDP1864_SCANLINE_INT_START, 0), 0);
	}
}

static TIMER_CALLBACK(cdp1864_efx_tick)
{
	const device_config *device = ptr;
	cdp1864_t *cdp1864 = get_safe_token(device);

	int scanline = video_screen_get_vpos(cdp1864->screen);

	switch (scanline)
	{
	case CDP1864_SCANLINE_EFX_TOP_START:
		cdp1864->intf->on_efx_changed(device, ASSERT_LINE);
		timer_adjust_oneshot(cdp1864->efx_timer, video_screen_get_time_until_pos(cdp1864->screen, CDP1864_SCANLINE_EFX_TOP_END, 0), 0);
		break;

	case CDP1864_SCANLINE_EFX_TOP_END:
		cdp1864->intf->on_efx_changed(device, CLEAR_LINE);
		timer_adjust_oneshot(cdp1864->efx_timer, video_screen_get_time_until_pos(cdp1864->screen, CDP1864_SCANLINE_EFX_BOTTOM_START, 0), 0);
		break;

	case CDP1864_SCANLINE_EFX_BOTTOM_START:
		cdp1864->intf->on_efx_changed(device, ASSERT_LINE);
		timer_adjust_oneshot(cdp1864->efx_timer, video_screen_get_time_until_pos(cdp1864->screen, CDP1864_SCANLINE_EFX_BOTTOM_END, 0), 0);
		break;

	case CDP1864_SCANLINE_EFX_BOTTOM_END:
		cdp1864->intf->on_efx_changed(device, CLEAR_LINE);
		timer_adjust_oneshot(cdp1864->efx_timer, video_screen_get_time_until_pos(cdp1864->screen, CDP1864_SCANLINE_EFX_TOP_START, 0), 0);
		break;
	}
}

static TIMER_CALLBACK(cdp1864_dma_tick)
{
	const device_config *device = ptr;
	cdp1864_t *cdp1864 = get_safe_token(device);

	int scanline = video_screen_get_vpos(cdp1864->screen);

	if (cdp1864->dmaout)
	{
		if (cdp1864->disp)
		{
			if (scanline >= CDP1864_SCANLINE_DISPLAY_START && scanline < CDP1864_SCANLINE_DISPLAY_END)
			{
				cdp1864->intf->on_dmao_changed(device, CLEAR_LINE);
			}
		}

		timer_adjust_oneshot(cdp1864->dma_timer, ATTOTIME_IN_CYCLES(CDP1864_CYCLES_DMA_WAIT, 0), 0);

		cdp1864->dmaout = 0;
	}
	else
	{
		if (cdp1864->disp)
		{
			if (scanline >= CDP1864_SCANLINE_DISPLAY_START && scanline < CDP1864_SCANLINE_DISPLAY_END)
			{
				cdp1864->intf->on_dmao_changed(device, HOLD_LINE);
			}
		}

		timer_adjust_oneshot(cdp1864->dma_timer, ATTOTIME_IN_CYCLES(CDP1864_CYCLES_DMA_ACTIVE, 0), 0);

		cdp1864->dmaout = 1;
	}
}

/* Palette Initialization */

static void cdp1864_init_palette(const device_config *device)
{
	cdp1864_t *cdp1864 = get_safe_token(device);
	int i;

	double res_total = cdp1864->intf->res_r + cdp1864->intf->res_g + cdp1864->intf->res_b + cdp1864->intf->res_bkg;

	int weight_r = (cdp1864->intf->res_r / res_total) * 100;
	int weight_g = (cdp1864->intf->res_g / res_total) * 100;
	int weight_b = (cdp1864->intf->res_b / res_total) * 100;
	int weight_bkg = (cdp1864->intf->res_bkg / res_total) * 100;

	for (i = 0; i < 16; i++)
	{
		int r, g, b, luma = 0;

		luma += (i & 4) ? weight_r : 0;
		luma += (i & 1) ? weight_g : 0;
		luma += (i & 2) ? weight_b : 0;
		luma += (i & 8) ? 0 : weight_bkg;

		luma = (luma * 0xff) / 100;

		r = (i & 4) ? luma : 0;
		g = (i & 1) ? luma : 0;
		b = (i & 2) ? luma : 0;

		palette_set_color_rgb( device->machine, i, r, g, b );
	}
}

/* Audio Output Enable */

void cdp1864_aoe_w(const device_config *device, int level)
{
	cdp1864_t *cdp1864 = get_safe_token(device);

	if (!level)
	{
		cdp1864->latch = CDP1864_DEFAULT_LATCH;
	}

	cdp1864->audio = level;
	
	beep_set_state(0, level); // TODO: remove this
}

/* Display On/Off */

READ8_DEVICE_HANDLER( cdp1864_dispon_r )
{
	cdp1864_t *cdp1864 = get_safe_token(device);

	cdp1864->disp = 1;

	return 0xff;
}

READ8_DEVICE_HANDLER( cdp1864_dispoff_r )
{
	cdp1864_t *cdp1864 = get_safe_token(device);

	cdp1864->disp = 0;

	cdp1864->intf->on_int_changed(device, CLEAR_LINE);
	cdp1864->intf->on_dmao_changed(device, CLEAR_LINE);

	return 0xff;
}

/* Step Background Color */

WRITE8_DEVICE_HANDLER( cdp1864_step_bgcolor_w )
{
	cdp1864_t *cdp1864 = get_safe_token(device);

	cdp1864->disp = 1;

	if (++cdp1864->bgcolor > 3) cdp1864->bgcolor = 0;
}

/* Load Tone Latch */

WRITE8_DEVICE_HANDLER( cdp1864_tone_latch_w )
{
	cdp1864_t *cdp1864 = get_safe_token(device);

	cdp1864->latch = data;
	beep_set_frequency(0, CDP1864_CLOCK / 8 / 4 / (data + 1) / 2); // TODO: remove this
}

/* DMA Write */

void cdp1864_dma_w(const device_config *device, UINT8 data, int color_on, int rdata, int gdata, int bdata)
{
	cdp1864_t *cdp1864 = get_safe_token(device);

	int sx = video_screen_get_hpos(cdp1864->screen) + 4;
	int y = video_screen_get_vpos(cdp1864->screen);
	int x;

	if (color_on == CLEAR_LINE)
	{
		rdata = 1;
		gdata = 1;
		bdata = 1;
	}

	for (x = 0; x < 8; x++)
	{
		int color = CDP1864_BACKGROUND_COLOR_SEQUENCE[cdp1864->bgcolor] + 8;

		if (BIT(data, 7))
		{
			color = (gdata << 2) | (bdata << 1) | rdata;
		}

		*BITMAP_ADDR16(cdp1864->bitmap, y, sx + x) = color;

		data <<= 1;
	}
}

/* Sound Update */

#ifdef UNUSED_FUNCTION
static void cdp1864_sound_update(const device_config *device, stream_sample_t **inputs, stream_sample_t **_buffer, int length)
{
	cdp1864_t *cdp1864 = get_safe_token(device);

	INT16 signal = cdp1864->signal;
	stream_sample_t *buffer = _buffer[0];

	memset(buffer, 0, length * sizeof(*buffer));

	if (cdp1864->audio)
	{
		double frequency = cdp1864->intf->clock / 8 / 4 / (cdp1864->latch + 1) / 2;

		int rate = device->machine->sample_rate / 2;

		/* get progress through wave */
		int incr = cdp1864->incr;

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
		cdp1864->incr = incr;
		cdp1864->signal = signal;
	}
}
#endif

/* Screen Update */

void cdp1864_update(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect)
{
	cdp1864_t *cdp1864 = get_safe_token(device);

	if (cdp1864->disp)
	{
		copybitmap(bitmap, cdp1864->bitmap, 0, 0, 0, 0, cliprect);
		bitmap_fill(cdp1864->bitmap, cliprect, CDP1864_BACKGROUND_COLOR_SEQUENCE[cdp1864->bgcolor] + 8);
	}
	else
	{
		bitmap_fill(bitmap, cliprect, get_black_pen(device->machine));
	}
}

/* Device Interface */

static DEVICE_START( cdp1864 )
{
	cdp1864_t *cdp1864 = get_safe_token(device);

	/* validate arguments */
	assert(device != NULL);
	assert(device->tag != NULL);
	assert(strlen(device->tag) < 20);

	cdp1864->intf = device->static_config;

	assert(cdp1864->intf != NULL);
	assert(cdp1864->intf->clock > 0);
	assert(cdp1864->intf->on_int_changed != NULL);
	assert(cdp1864->intf->on_dmao_changed != NULL);
	assert(cdp1864->intf->on_efx_changed != NULL);

	/* get the screen device */
	cdp1864->screen = device_list_find_by_tag(device->machine->config->devicelist, VIDEO_SCREEN, cdp1864->intf->screen_tag);
	assert(cdp1864->screen != NULL);

	/* allocate the temporary bitmap */
	cdp1864->bitmap = auto_bitmap_alloc(video_screen_get_width(cdp1864->screen), video_screen_get_height(cdp1864->screen), video_screen_get_format(cdp1864->screen));
	bitmap_fill(cdp1864->bitmap, 0, CDP1864_BACKGROUND_COLOR_SEQUENCE[cdp1864->bgcolor] + 8);

	/* initialize the palette */
	cdp1864_init_palette(device);

	/* create the timers */
	cdp1864->int_timer = timer_alloc(cdp1864_int_tick, (void *)device);
	cdp1864->efx_timer = timer_alloc(cdp1864_efx_tick, (void *)device);
	cdp1864->dma_timer = timer_alloc(cdp1864_dma_tick, (void *)device);

	/* register for state saving */
	state_save_register_item("cdp1864", device->tag, 0, cdp1864->disp);
	state_save_register_item("cdp1864", device->tag, 0, cdp1864->dmaout);
	state_save_register_item("cdp1864", device->tag, 0, cdp1864->bgcolor);

	state_save_register_item("cdp1864", device->tag, 0, cdp1864->audio);
	state_save_register_item("cdp1864", device->tag, 0, cdp1864->latch);
	state_save_register_item("cdp1864", device->tag, 0, cdp1864->signal);
	state_save_register_item("cdp1864", device->tag, 0, cdp1864->incr);

	state_save_register_bitmap("cdp1864", device->tag, 0, "cdp1864->bitmap", cdp1864->bitmap);
	return DEVICE_START_OK;
}

static DEVICE_RESET( cdp1864 )
{
	cdp1864_t *cdp1864 = get_safe_token(device);

	timer_adjust_oneshot(cdp1864->int_timer, video_screen_get_time_until_pos(cdp1864->screen, CDP1864_SCANLINE_INT_START, 0), 0);
	timer_adjust_oneshot(cdp1864->efx_timer, video_screen_get_time_until_pos(cdp1864->screen, CDP1864_SCANLINE_EFX_TOP_START, 0), 0);
	timer_adjust_oneshot(cdp1864->dma_timer, ATTOTIME_IN_CYCLES(CDP1864_CYCLES_DMA_START, 0), 0);
	
	cdp1864->disp = 0;
	cdp1864->dmaout = 0;
	cdp1864->bgcolor = 0;

	cdp1864->intf->on_int_changed(device, CLEAR_LINE);
	cdp1864->intf->on_dmao_changed(device, CLEAR_LINE);
	cdp1864->intf->on_efx_changed(device, CLEAR_LINE);

	cdp1864_aoe_w(device, 0);
}

static DEVICE_SET_INFO( cdp1864 )
{
	switch (state)
	{
		/* no parameters to set */
	}
}

DEVICE_GET_INFO( cdp1864 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(cdp1864_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_SET_INFO:						info->set_info = DEVICE_SET_INFO_NAME(cdp1864); break;
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(cdp1864);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(cdp1864);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							info->s = "RCA CDP1861";					break;
		case DEVINFO_STR_FAMILY:						info->s = "RCA CDP1800";					break;
		case DEVINFO_STR_VERSION:						info->s = "1.0";							break;
		case DEVINFO_STR_SOURCE_FILE:					info->s = __FILE__;							break;
		case DEVINFO_STR_CREDITS:						info->s = "Copyright MESS Team";			break;
	}
}
