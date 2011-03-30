/**********************************************************************

    RCA CDP1861 Video Display Controller emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "emu.h"
#include "cdp1861.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define CDP1861_CYCLES_DMA_START	2*8
#define CDP1861_CYCLES_DMA_ACTIVE	8*8
#define CDP1861_CYCLES_DMA_WAIT		6*8

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _cdp1861_t cdp1861_t;
struct _cdp1861_t
{
	devcb_resolved_write_line	out_int_func;
	devcb_resolved_write_line	out_dmao_func;
	devcb_resolved_write_line	out_efx_func;

	screen_device *screen;	/* screen */
	bitmap_t *bitmap;				/* bitmap */

	int disp;						/* display enabled */
	int dispon;						/* display on latch */
	int dispoff;					/* display off latch */
	int dmaout;						/* DMA request active */

	/* timers */
	emu_timer *int_timer;			/* interrupt timer */
	emu_timer *efx_timer;			/* EFx timer */
	emu_timer *dma_timer;			/* DMA timer */

	cpu_device *cpu;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE cdp1861_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == CDP1861);
	return (cdp1861_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE const cdp1861_interface *get_interface(device_t *device)
{
	assert(device != NULL);
	assert((device->type() == CDP1861));
	return (const cdp1861_interface *) device->baseconfig().static_config();
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    TIMER_CALLBACK( cdp1861_int_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( cdp1861_int_tick )
{
	device_t *device = (device_t *)ptr;
	cdp1861_t *cdp1861 = get_safe_token(device);

	int scanline = cdp1861->screen->vpos();

	if (scanline == CDP1861_SCANLINE_INT_START)
	{
		if (cdp1861->disp)
		{
			devcb_call_write_line(&cdp1861->out_int_func, ASSERT_LINE);
		}

		cdp1861->int_timer->adjust(cdp1861->screen->time_until_pos( CDP1861_SCANLINE_INT_END, 0));
	}
	else
	{
		if (cdp1861->disp)
		{
			devcb_call_write_line(&cdp1861->out_int_func, CLEAR_LINE);
		}

		cdp1861->int_timer->adjust(cdp1861->screen->time_until_pos(CDP1861_SCANLINE_INT_START, 0));
	}
}

/*-------------------------------------------------
    TIMER_CALLBACK( cdp1861_efx_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( cdp1861_efx_tick )
{
	device_t *device = (device_t *)ptr;
	cdp1861_t *cdp1861 = get_safe_token(device);

	int scanline = cdp1861->screen->vpos();

	switch (scanline)
	{
	case CDP1861_SCANLINE_EFX_TOP_START:
		devcb_call_write_line(&cdp1861->out_efx_func, ASSERT_LINE);
		cdp1861->efx_timer->adjust(cdp1861->screen->time_until_pos(CDP1861_SCANLINE_EFX_TOP_END, 0));
		break;

	case CDP1861_SCANLINE_EFX_TOP_END:
		devcb_call_write_line(&cdp1861->out_efx_func, CLEAR_LINE);
		cdp1861->efx_timer->adjust(cdp1861->screen->time_until_pos(CDP1861_SCANLINE_EFX_BOTTOM_START, 0));
		break;

	case CDP1861_SCANLINE_EFX_BOTTOM_START:
		devcb_call_write_line(&cdp1861->out_efx_func, ASSERT_LINE);
		cdp1861->efx_timer->adjust(cdp1861->screen->time_until_pos(CDP1861_SCANLINE_EFX_BOTTOM_END, 0));
		break;

	case CDP1861_SCANLINE_EFX_BOTTOM_END:
		devcb_call_write_line(&cdp1861->out_efx_func, CLEAR_LINE);
		cdp1861->efx_timer->adjust(cdp1861->screen->time_until_pos(CDP1861_SCANLINE_EFX_TOP_START, 0));
		break;
	}
}

/*-------------------------------------------------
    TIMER_CALLBACK( cdp1861_dma_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( cdp1861_dma_tick )
{
	device_t *device = (device_t *)ptr;
	cdp1861_t *cdp1861 = get_safe_token(device);

	int scanline = cdp1861->screen->vpos();

	if (cdp1861->dmaout)
	{
		if (cdp1861->disp)
		{
			if (scanline >= CDP1861_SCANLINE_DISPLAY_START && scanline < CDP1861_SCANLINE_DISPLAY_END)
			{
				devcb_call_write_line(&cdp1861->out_dmao_func, CLEAR_LINE);
			}
		}

		cdp1861->dma_timer->adjust(cdp1861->cpu->cycles_to_attotime(CDP1861_CYCLES_DMA_WAIT));

		cdp1861->dmaout = 0;
	}
	else
	{
		if (cdp1861->disp)
		{
			if (scanline >= CDP1861_SCANLINE_DISPLAY_START && scanline < CDP1861_SCANLINE_DISPLAY_END)
			{
				devcb_call_write_line(&cdp1861->out_dmao_func, ASSERT_LINE);
			}
		}

		cdp1861->dma_timer->adjust(cdp1861->cpu->cycles_to_attotime(CDP1861_CYCLES_DMA_ACTIVE));

		cdp1861->dmaout = 1;
	}
}

/*-------------------------------------------------
    cdp1861_dispon_w - turn display on
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( cdp1861_dispon_w )
{
	cdp1861_t *cdp1861 = get_safe_token(device);

	if (!cdp1861->dispon && state) cdp1861->disp = 1;

	cdp1861->dispon = state;
}

/*-------------------------------------------------
    cdp1861_dispoff_r - turn display off
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( cdp1861_dispoff_w )
{
	cdp1861_t *cdp1861 = get_safe_token(device);

	if (!cdp1861->dispon && !cdp1861->dispoff && state) cdp1861->disp = 0;

	cdp1861->dispoff = state;

	devcb_call_write_line(&cdp1861->out_int_func, CLEAR_LINE);
	devcb_call_write_line(&cdp1861->out_dmao_func, CLEAR_LINE);
}

/*-------------------------------------------------
    cdp1861_dma_w - write DMA byte
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( cdp1861_dma_w )
{
	cdp1861_t *cdp1861 = get_safe_token(device);

	int sx = cdp1861->screen->hpos() + 4;
	int y = cdp1861->screen->vpos();
	int x;

	for (x = 0; x < 8; x++)
	{
		int color = BIT(data, 7);
		*BITMAP_ADDR16(cdp1861->bitmap, y, sx + x) = color;
		data <<= 1;
	}
}

/*-------------------------------------------------
    cdp1861_update - update screen
-------------------------------------------------*/

void cdp1861_update(device_t *device, bitmap_t *bitmap, const rectangle *cliprect)
{
	cdp1861_t *cdp1861 = get_safe_token(device);

	if (cdp1861->disp)
	{
		copybitmap(bitmap, cdp1861->bitmap, 0, 0, 0, 0, cliprect);
	}
	else
	{
		bitmap_fill(bitmap, cliprect, get_black_pen(device->machine()));
	}
}

/*-------------------------------------------------
    DEVICE_START( cdp1861 )
-------------------------------------------------*/

static DEVICE_START( cdp1861 )
{
	cdp1861_t *cdp1861 = get_safe_token(device);
	const cdp1861_interface *intf = get_interface(device);

	/* resolve callbacks */
	devcb_resolve_write_line(&cdp1861->out_int_func, &intf->out_int_func, device);
	devcb_resolve_write_line(&cdp1861->out_dmao_func, &intf->out_dmao_func, device);
	devcb_resolve_write_line(&cdp1861->out_efx_func, &intf->out_efx_func, device);

	/* get the cpu */
	cdp1861->cpu = device->machine().device<cpu_device>(intf->cpu_tag);

	/* get the screen device */
	cdp1861->screen =  device->machine().device<screen_device>(intf->screen_tag);
	assert(cdp1861->screen != NULL);

	/* allocate the temporary bitmap */
	cdp1861->bitmap = auto_bitmap_alloc(device->machine(), cdp1861->screen->width(), cdp1861->screen->height(), cdp1861->screen->format());

	/* create the timers */
	cdp1861->int_timer = device->machine().scheduler().timer_alloc(FUNC(cdp1861_int_tick), (void *)device);
	cdp1861->efx_timer = device->machine().scheduler().timer_alloc(FUNC(cdp1861_efx_tick), (void *)device);
	cdp1861->dma_timer = device->machine().scheduler().timer_alloc(FUNC(cdp1861_dma_tick), (void *)device);

	/* register for state saving */
	device->save_item(NAME(cdp1861->disp));
	device->save_item(NAME(cdp1861->dispon));
	device->save_item(NAME(cdp1861->dispoff));
	device->save_item(NAME(cdp1861->dmaout));
	device->save_item(NAME(*cdp1861->bitmap));
}

/*-------------------------------------------------
    DEVICE_RESET( cdp1861 )
-------------------------------------------------*/

static DEVICE_RESET( cdp1861 )
{
	cdp1861_t *cdp1861 = get_safe_token(device);

	cdp1861->int_timer->adjust(cdp1861->screen->time_until_pos(CDP1861_SCANLINE_INT_START, 0));
	cdp1861->efx_timer->adjust(cdp1861->screen->time_until_pos(CDP1861_SCANLINE_EFX_TOP_START, 0));
	cdp1861->dma_timer->adjust(cdp1861->cpu->cycles_to_attotime(CDP1861_CYCLES_DMA_START));

	cdp1861->disp = 0;
	cdp1861->dmaout = 0;

	devcb_call_write_line(&cdp1861->out_int_func, CLEAR_LINE);
	devcb_call_write_line(&cdp1861->out_dmao_func, CLEAR_LINE);
	devcb_call_write_line(&cdp1861->out_efx_func, CLEAR_LINE);
}

/*-------------------------------------------------
    DEVICE_GET_INFO( cdp1861 )
-------------------------------------------------*/

DEVICE_GET_INFO( cdp1861 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(cdp1861_t);						break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;										break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(cdp1861);			break;
		case DEVINFO_FCT_STOP:							/* Nothing */										break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(cdp1861);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "RCA CDP1861");						break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "RCA CDP1800");						break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");								break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);							break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");				break;
	}
}

DEFINE_LEGACY_DEVICE(CDP1861, cdp1861);
