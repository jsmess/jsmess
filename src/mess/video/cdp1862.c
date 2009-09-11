/**********************************************************************

    RCA CDP1862 COS/MOS Color Generator Controller emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

    TODO:

	- calculate colors from luminance/chrominance resistors
	- refactor DMA write to WRITE8_DEVICE_HANDLER and add callbacks for RD/BD/GD

*/

#include "driver.h"
#include "cdp1862.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

static const int CDP1862_BACKGROUND_COLOR_SEQUENCE[] = { 2, 0, 1, 4 };

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _cdp1862_t cdp1862_t;
struct _cdp1862_t
{
	const device_config *screen;	/* screen */
	bitmap_t *bitmap;				/* bitmap */

	int bgcolor;					/* background color */
	int con;						/* color on */
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE cdp1862_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == CDP1862);
	return (cdp1862_t *)device->token;
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    initialize_palette - palette initialization
-------------------------------------------------*/

static void initialize_palette(const device_config *device)
{
	const cdp1862_interface *intf = device->static_config;
	int i;

	double res_total = intf->chr_r + intf->chr_g + intf->chr_b + intf->chr_bkg;

	int weight_r = (intf->chr_r / res_total) * 100;
	int weight_g = (intf->chr_g / res_total) * 100;
	int weight_b = (intf->chr_b / res_total) * 100;
	int weight_bkg = (intf->chr_bkg / res_total) * 100;

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

		palette_set_color_rgb(device->machine, i, r, g, b);
	}
}

/*-------------------------------------------------
    cdp1862_bkg_w - background step write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( cdp1862_bkg_w )
{
	cdp1862_t *cdp1862 = get_safe_token(device);

	if (++cdp1862->bgcolor > 3) cdp1862->bgcolor = 0;
}

/*-------------------------------------------------
    cdp1862_dma_w - DMA byte write
-------------------------------------------------*/

void cdp1862_dma_w(const device_config *device, UINT8 data, int rd, int bd, int gd)
{
	cdp1862_t *cdp1862 = get_safe_token(device);

	int sx = video_screen_get_hpos(cdp1862->screen) + 4;
	int y = video_screen_get_vpos(cdp1862->screen);
	int x;

	if (cdp1862->con)
	{
		/* latch white dot color */
		rd = 1;
		gd = 1;
		bd = 1;
	}

	for (x = 0; x < 8; x++)
	{
		int color = CDP1862_BACKGROUND_COLOR_SEQUENCE[cdp1862->bgcolor] + 8;

		if (BIT(data, 7))
		{
			color = (gd << 2) | (bd << 1) | rd;
		}

		*BITMAP_ADDR16(cdp1862->bitmap, y, sx + x) = color;

		data <<= 1;
	}
}

/*-------------------------------------------------
    cdp1862_con_w - color on write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( cdp1862_con_w )
{
	cdp1862_t *cdp1862 = get_safe_token(device);

	if (!state)
	{
		cdp1862->con = 0;
	}
}

/*-------------------------------------------------
    cdp1862_update - screen update
-------------------------------------------------*/

void cdp1862_update(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect)
{
	cdp1862_t *cdp1862 = get_safe_token(device);

	copybitmap(bitmap, cdp1862->bitmap, 0, 0, 0, 0, cliprect);
	bitmap_fill(cdp1862->bitmap, cliprect, CDP1862_BACKGROUND_COLOR_SEQUENCE[cdp1862->bgcolor] + 8);
}

/*-------------------------------------------------
    DEVICE_START( cdp1862 )
-------------------------------------------------*/

static DEVICE_START( cdp1862 )
{
	cdp1862_t *cdp1862 = get_safe_token(device);
	const cdp1862_interface *intf = device->static_config;

	/* get the screen device */
	cdp1862->screen = devtag_get_device(device->machine, intf->screen_tag);
	assert(cdp1862->screen != NULL);

	/* allocate the temporary bitmap */
	cdp1862->bitmap = auto_bitmap_alloc(device->machine, video_screen_get_width(cdp1862->screen), video_screen_get_height(cdp1862->screen), video_screen_get_format(cdp1862->screen));

	/* initialize the palette */
	initialize_palette(device);

	/* register for state saving */
	state_save_register_device_item(device, 0, cdp1862->bgcolor);
	state_save_register_device_item(device, 0, cdp1862->con);
	state_save_register_device_item_bitmap(device, 0, cdp1862->bitmap);
}

/*-------------------------------------------------
    DEVICE_RESET( cdp1862 )
-------------------------------------------------*/

static DEVICE_RESET( cdp1862 )
{
	cdp1862_t *cdp1862 = get_safe_token(device);

	cdp1862->bgcolor = 0;
	cdp1862->con = 1;
}

/*-------------------------------------------------
    DEVICE_GET_INFO( cdp1862 )
-------------------------------------------------*/

DEVICE_GET_INFO( cdp1862 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(cdp1862_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(cdp1862);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(cdp1862);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "RCA CDP1862");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "RCA CDP1800");				break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");		break;
	}
}
