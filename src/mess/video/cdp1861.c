#include "driver.h"
#include "cdp1861.h"

#define CDP1861_CYCLES_DMA_START	2*8
#define CDP1861_CYCLES_DMA_ACTIVE	8*8
#define CDP1861_CYCLES_DMA_WAIT		6*8

typedef struct _cdp1861_t cdp1861_t;
struct _cdp1861_t
{
	const cdp1861_interface *intf;

	const device_config *screen;	/* screen */
	bitmap_t *bitmap;				/* bitmap */

	int disp;						/* display on */
	int dmaout;						/* DMA request active */

	/* timers */
	emu_timer *int_timer;			/* interrupt timer */
	emu_timer *efx_timer;			/* EFx timer */
	emu_timer *dma_timer;			/* DMA timer */
};

INLINE cdp1861_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);

	return (cdp1861_t *)device->token;
}

static TIMER_CALLBACK(cdp1861_int_tick)
{
	const device_config *device = ptr;
	cdp1861_t *cdp1861 = get_safe_token(device);

	int scanline = video_screen_get_vpos(cdp1861->screen);

	if (scanline == CDP1861_SCANLINE_INT_START)
	{
		if (cdp1861->disp)
		{
			cdp1861->intf->on_int_changed(device, HOLD_LINE);
		}

		timer_adjust_oneshot(cdp1861->int_timer, video_screen_get_time_until_pos(cdp1861->screen, CDP1861_SCANLINE_INT_END, 0), 0);
	}
	else
	{
		if (cdp1861->disp)
		{
			cdp1861->intf->on_int_changed(device, CLEAR_LINE);
		}

		timer_adjust_oneshot(cdp1861->int_timer, video_screen_get_time_until_pos(cdp1861->screen, CDP1861_SCANLINE_INT_START, 0), 0);
	}
}

static TIMER_CALLBACK(cdp1861_efx_tick)
{
	const device_config *device = ptr;
	cdp1861_t *cdp1861 = get_safe_token(device);

	int scanline = video_screen_get_vpos(cdp1861->screen);

	switch (scanline)
	{
	case CDP1861_SCANLINE_EFX_TOP_START:
		cdp1861->intf->on_efx_changed(device, ASSERT_LINE);
		timer_adjust_oneshot(cdp1861->efx_timer, video_screen_get_time_until_pos(cdp1861->screen, CDP1861_SCANLINE_EFX_TOP_END, 0), 0);
		break;

	case CDP1861_SCANLINE_EFX_TOP_END:
		cdp1861->intf->on_efx_changed(device, CLEAR_LINE);
		timer_adjust_oneshot(cdp1861->efx_timer, video_screen_get_time_until_pos(cdp1861->screen, CDP1861_SCANLINE_EFX_BOTTOM_START, 0), 0);
		break;

	case CDP1861_SCANLINE_EFX_BOTTOM_START:
		cdp1861->intf->on_efx_changed(device, ASSERT_LINE);
		timer_adjust_oneshot(cdp1861->efx_timer, video_screen_get_time_until_pos(cdp1861->screen, CDP1861_SCANLINE_EFX_BOTTOM_END, 0), 0);
		break;

	case CDP1861_SCANLINE_EFX_BOTTOM_END:
		cdp1861->intf->on_efx_changed(device, CLEAR_LINE);
		timer_adjust_oneshot(cdp1861->efx_timer, video_screen_get_time_until_pos(cdp1861->screen, CDP1861_SCANLINE_EFX_TOP_START, 0), 0);
		break;
	}
}

static TIMER_CALLBACK(cdp1861_dma_tick)
{
	const device_config *device = ptr;
	cdp1861_t *cdp1861 = get_safe_token(device);

	int scanline = video_screen_get_vpos(cdp1861->screen);

	if (cdp1861->dmaout)
	{
		if (cdp1861->disp)
		{
			if (scanline >= CDP1861_SCANLINE_DISPLAY_START && scanline < CDP1861_SCANLINE_DISPLAY_END)
			{
				cdp1861->intf->on_dmao_changed(device, CLEAR_LINE);
			}
		}

		timer_adjust_oneshot(cdp1861->dma_timer, ATTOTIME_IN_CYCLES(CDP1861_CYCLES_DMA_WAIT, 0), 0);

		cdp1861->dmaout = 0;
	}
	else
	{
		if (cdp1861->disp)
		{
			if (scanline >= CDP1861_SCANLINE_DISPLAY_START && scanline < CDP1861_SCANLINE_DISPLAY_END)
			{
				cdp1861->intf->on_dmao_changed(device, HOLD_LINE);
			}
		}

		timer_adjust_oneshot(cdp1861->dma_timer, ATTOTIME_IN_CYCLES(CDP1861_CYCLES_DMA_ACTIVE, 0), 0);

		cdp1861->dmaout = 1;
	}
}

/* Display On/Off */

READ8_DEVICE_HANDLER( cdp1861_dispon_r )
{
	cdp1861_t *cdp1861 = get_safe_token(device);

	cdp1861->disp = 1;

	return 0xff;
}

WRITE8_DEVICE_HANDLER( cdp1861_dispoff_w )
{
	cdp1861_t *cdp1861 = get_safe_token(device);

	cdp1861->disp = 0;

	cdp1861->intf->on_int_changed(device, CLEAR_LINE);
	cdp1861->intf->on_dmao_changed(device, CLEAR_LINE);
}

/* DMA Write */

void cdp1861_dma_w(const device_config *device, UINT8 data)
{
	cdp1861_t *cdp1861 = get_safe_token(device);

	int sx = video_screen_get_hpos(cdp1861->screen) + 4;
	int y = video_screen_get_vpos(cdp1861->screen);
	int x;

	for (x = 0; x < 8; x++)
	{
		int color = BIT(data, 7);
		*BITMAP_ADDR16(cdp1861->bitmap, y, sx + x) = color;
		data <<= 1;
	}
}

/* Screen Update */

void cdp1861_update(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect)
{
	cdp1861_t *cdp1861 = get_safe_token(device);

	if (cdp1861->disp)
	{
		copybitmap(bitmap, cdp1861->bitmap, 0, 0, 0, 0, cliprect);
	}
	else
	{
		fillbitmap(bitmap, get_black_pen(device->machine), cliprect);
	}
}

/* Device Interface */

static DEVICE_START( cdp1861 )
{
	cdp1861_t *cdp1861 = get_safe_token(device);
	char unique_tag[30];

	/* validate arguments */
	assert(device != NULL);
	assert(device->tag != NULL);
	assert(strlen(device->tag) < 20);

	cdp1861->intf = device->static_config;

	assert(cdp1861->intf != NULL);
	assert(cdp1861->intf->clock > 0);
	assert(cdp1861->intf->on_int_changed != NULL);
	assert(cdp1861->intf->on_dmao_changed != NULL);
	assert(cdp1861->intf->on_efx_changed != NULL);

	/* get the screen device */
	cdp1861->screen = device_list_find_by_tag(device->machine->config->devicelist, VIDEO_SCREEN, cdp1861->intf->screen_tag);
	assert(cdp1861->screen != NULL);

	/* allocate the temporary bitmap */
	cdp1861->bitmap = auto_bitmap_alloc(video_screen_get_width(cdp1861->screen), video_screen_get_height(cdp1861->screen), video_screen_get_format(cdp1861->screen));

	/* create the timers */
	cdp1861->int_timer = timer_alloc(cdp1861_int_tick, (void *)device);
	cdp1861->efx_timer = timer_alloc(cdp1861_efx_tick, (void *)device);
	cdp1861->dma_timer = timer_alloc(cdp1861_dma_tick, (void *)device);

	/* register for state saving */
	state_save_combine_module_and_tag(unique_tag, "CDP1861", device->tag);

	state_save_register_item(unique_tag, 0, cdp1861->disp);
	state_save_register_item(unique_tag, 0, cdp1861->dmaout);
	state_save_register_bitmap(unique_tag, 0, "cdp1861->bitmap", cdp1861->bitmap);
	return DEVICE_START_OK;
}

static DEVICE_RESET( cdp1861 )
{
	cdp1861_t *cdp1861 = get_safe_token(device);

	timer_adjust_oneshot(cdp1861->int_timer, video_screen_get_time_until_pos(cdp1861->screen, CDP1861_SCANLINE_INT_START, 0), 0);
	timer_adjust_oneshot(cdp1861->efx_timer, video_screen_get_time_until_pos(cdp1861->screen, CDP1861_SCANLINE_EFX_TOP_START, 0), 0);
	timer_adjust_oneshot(cdp1861->dma_timer, ATTOTIME_IN_CYCLES(CDP1861_CYCLES_DMA_START, 0), 0);
	
	cdp1861->disp = 0;
	cdp1861->dmaout = 0;

	cdp1861->intf->on_int_changed(device, CLEAR_LINE);
	cdp1861->intf->on_dmao_changed(device, CLEAR_LINE);
	cdp1861->intf->on_efx_changed(device, CLEAR_LINE);
}

static DEVICE_SET_INFO( cdp1861 )
{
	switch (state)
	{
		/* no parameters to set */
	}
}

DEVICE_GET_INFO( cdp1861 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(cdp1861_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_SET_INFO:						info->set_info = DEVICE_SET_INFO_NAME(cdp1861); break;
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(cdp1861);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(cdp1861);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							info->s = "RCA CDP1861";					break;
		case DEVINFO_STR_FAMILY:						info->s = "RCA CDP1800";					break;
		case DEVINFO_STR_VERSION:						info->s = "1.0";							break;
		case DEVINFO_STR_SOURCE_FILE:					info->s = __FILE__;							break;
		case DEVINFO_STR_CREDITS:						info->s = "Copyright MESS Team";			break;
	}
}
