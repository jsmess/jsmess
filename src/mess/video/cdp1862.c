/**********************************************************************

    RCA CDP1862 COS/MOS Color Generator Controller emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

    TODO:

    - calculate colors from luminance/chrominance resistors

*/

#include "emu.h"
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
	devcb_resolved_read_line		in_rd_func;
	devcb_resolved_read_line		in_bd_func;
	devcb_resolved_read_line		in_gd_func;

	screen_device *screen;	/* screen */
	bitmap_t *bitmap;				/* bitmap */

	int bgcolor;					/* background color */
	int con;						/* color on */
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE cdp1862_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == CDP1862);
	return (cdp1862_t *)downcast<legacy_device_base *>(device)->token();
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    initialize_palette - palette initialization
-------------------------------------------------*/

static void initialize_palette(device_t *device)
{
	const cdp1862_interface *intf = (const cdp1862_interface *)device->baseconfig().static_config();
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

		palette_set_color_rgb(device->machine(), i, r, g, b);
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

WRITE8_DEVICE_HANDLER( cdp1862_dma_w )
{
	cdp1862_t *cdp1862 = get_safe_token(device);

	int rd = 1, bd = 1, gd = 1;
	int sx = cdp1862->screen->hpos() + 4;
	int y = cdp1862->screen->vpos();
	int x;

	if (!cdp1862->con)
	{
		rd = devcb_call_read_line(&cdp1862->in_rd_func);
		bd = devcb_call_read_line(&cdp1862->in_bd_func);
		gd = devcb_call_read_line(&cdp1862->in_gd_func);
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

void cdp1862_update(device_t *device, bitmap_t *bitmap, const rectangle *cliprect)
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
	const cdp1862_interface *intf = (const cdp1862_interface *)device->baseconfig().static_config();

	/* resolve callbacks */
	devcb_resolve_read_line(&cdp1862->in_rd_func, &intf->in_rd_func, device);
	devcb_resolve_read_line(&cdp1862->in_bd_func, &intf->in_bd_func, device);
	devcb_resolve_read_line(&cdp1862->in_gd_func, &intf->in_gd_func, device);

	/* get the screen device */
	cdp1862->screen = device->machine().device<screen_device>(intf->screen_tag);
	assert(cdp1862->screen != NULL);

	/* allocate the temporary bitmap */
	cdp1862->bitmap = auto_bitmap_alloc(device->machine(), cdp1862->screen->width(), cdp1862->screen->height(), cdp1862->screen->format());

	/* initialize the palette */
	initialize_palette(device);

	/* register for state saving */
	device->save_item(NAME(cdp1862->bgcolor));
	device->save_item(NAME(cdp1862->con));
	device->save_item(NAME(*cdp1862->bitmap));
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

DEFINE_LEGACY_DEVICE(CDP1862, cdp1862);
