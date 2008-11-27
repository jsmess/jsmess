/*

    TODO:

	- NTSC output, currently using RGB

*/

#include "driver.h"
#include "cdp1862.h"

typedef struct _cdp1862_t cdp1862_t;
struct _cdp1862_t
{
	const cdp1862_interface *intf;

	const device_config *screen;	/* screen */
	bitmap_t *bitmap;				/* bitmap */

	int bgcolor;					/* background color */
	int con;						/* color on */
};

static const int CDP1862_BACKGROUND_COLOR_SEQUENCE[] = { 2, 0, 1, 4 };

INLINE cdp1862_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);

	return (cdp1862_t *)device->token;
}

/* Palette Initialization */

static void cdp1862_init_palette(const device_config *device)
{
	cdp1862_t *cdp1862 = get_safe_token(device);
	int i;

	double res_total = cdp1862->intf->res_r + cdp1862->intf->res_g + cdp1862->intf->res_b + cdp1862->intf->res_bkg;

	int weight_r = (cdp1862->intf->res_r / res_total) * 100;
	int weight_g = (cdp1862->intf->res_g / res_total) * 100;
	int weight_b = (cdp1862->intf->res_b / res_total) * 100;
	int weight_bkg = (cdp1862->intf->res_bkg / res_total) * 100;

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

/* Step Background Color */

WRITE8_DEVICE_HANDLER( cdp1862_bkg_w )
{
	cdp1862_t *cdp1862 = get_safe_token(device);

	if (++cdp1862->bgcolor > 3) cdp1862->bgcolor = 0;
}

/* DMA write */

void cdp1862_dma_w(const device_config *device, UINT8 data, int color_on, int rdata, int gdata, int bdata)
{
	cdp1862_t *cdp1862 = get_safe_token(device);

	int sx = video_screen_get_hpos(cdp1862->screen) + 4;
	int y = video_screen_get_vpos(cdp1862->screen);
	int x;

	if (color_on == ASSERT_LINE)
	{
		/* enable color data input latch */
		cdp1862->con = ASSERT_LINE;
	}
	
	if (cdp1862->con == CLEAR_LINE)
	{
		/* latch white dot color */
		rdata = 1;
		gdata = 1;
		bdata = 1;
	}

	for (x = 0; x < 8; x++)
	{
		int color = CDP1862_BACKGROUND_COLOR_SEQUENCE[cdp1862->bgcolor] + 8;

		if (BIT(data, 7))
		{
			color = (gdata << 2) | (bdata << 1) | rdata;
		}

		*BITMAP_ADDR16(cdp1862->bitmap, y, sx + x) = color;

		data <<= 1;
	}
}

/* Screen Update */

void cdp1862_update(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect)
{
	cdp1862_t *cdp1862 = get_safe_token(device);

	copybitmap(bitmap, cdp1862->bitmap, 0, 0, 0, 0, cliprect);
	fillbitmap(cdp1862->bitmap, CDP1862_BACKGROUND_COLOR_SEQUENCE[cdp1862->bgcolor] + 8, cliprect);
}

/* Device Interface */

static DEVICE_START( cdp1862 )
{
	cdp1862_t *cdp1862 = get_safe_token(device);

	/* validate arguments */
	assert(device != NULL);
	assert(device->tag != NULL);
	assert(strlen(device->tag) < 20);

	cdp1862->intf = device->static_config;

	assert(cdp1862->intf != NULL);
	assert(cdp1862->intf->clock > 0);

	/* get the screen device */
	cdp1862->screen = device_list_find_by_tag(device->machine->config->devicelist, VIDEO_SCREEN, cdp1862->intf->screen_tag);
	assert(cdp1862->screen != NULL);

	/* allocate the temporary bitmap */
	cdp1862->bitmap = auto_bitmap_alloc(video_screen_get_width(cdp1862->screen), video_screen_get_height(cdp1862->screen), video_screen_get_format(cdp1862->screen));

	/* initialize the palette */
	cdp1862_init_palette(device);

	/* register for state saving */
	state_save_register_item("cdp1862", device->tag, 0, cdp1862->bgcolor);
	state_save_register_item("cdp1862", device->tag, 0, cdp1862->con);

	state_save_register_bitmap("cdp1862", device->tag, 0, "cdp1862_bitmap", cdp1862->bitmap);

	return DEVICE_START_OK;
}

static DEVICE_RESET( cdp1862 )
{
	cdp1862_t *cdp1862 = get_safe_token(device);

	cdp1862->bgcolor = 0;
	cdp1862->con = CLEAR_LINE;
}

static DEVICE_SET_INFO( cdp1862 )
{
	switch (state)
	{
		/* no parameters to set */
	}
}

DEVICE_GET_INFO( cdp1862 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(cdp1862_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_SET_INFO:						info->set_info = DEVICE_SET_INFO_NAME(cdp1862); break;
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(cdp1862);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(cdp1862);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							info->s = "RCA CDP1862";					break;
		case DEVINFO_STR_FAMILY:						info->s = "RCA CDP1800";					break;
		case DEVINFO_STR_VERSION:						info->s = "1.0";							break;
		case DEVINFO_STR_SOURCE_FILE:					info->s = __FILE__;							break;
		case DEVINFO_STR_CREDITS:						info->s = "Copyright MESS Team";			break;
	}
}
