/***************************************************************************

    OKI MSM6255 Dot Matrix LCD Controller implementation

    Copyright MESS team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "driver.h"
#include "msm6255.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define MSM6255_MOR_GRAPHICS		0x01
#define MSM6255_MOR_4_BIT_PARALLEL	0x02
#define MSM6255_MOR_2_BIT_PARALLEL	0x04
#define MSM6255_MOR_DISPLAY_ON		0x08
#define MSM6255_MOR_CURSOR_BLINK	0x10
#define MSM6255_MOR_CURSOR_ON		0x20
#define MSM6255_MOR_BLINK_TIME_16	0x40

#define MSM6255_PR_HP_4				0x03
#define MSM6255_PR_HP_5				0x04
#define MSM6255_PR_HP_6				0x05
#define MSM6255_PR_HP_7				0x06
#define MSM6255_PR_HP_8				0x07
#define MSM6255_PR_HP_MASK			0x07
#define MSM6255_PR_VP_MASK			0xf0

#define MSM6255_HNR_HN_MASK			0x7f

#define MSM6255_DVR_DN_MASK			0x7f

#define MSM6255_CPR_CPD_MASK		0x0f
#define MSM6255_CPR_CPU_MASK		0xf0

enum
{
	MSM6255_REGISTER_MOR = 0,
	MSM6255_REGISTER_PR,
	MSM6255_REGISTER_HNR,
	MSM6255_REGISTER_DVR,
	MSM6255_REGISTER_CPR,
	MSM6255_REGISTER_SLR,
	MSM6255_REGISTER_SUR,
	MSM6255_REGISTER_CLR,
	MSM6255_REGISTER_CUR
};

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _msm6255_t msm6255_t;
struct _msm6255_t
{
	const msm6255_interface *intf;	/* interface */

	const device_config *screen;	/* screen */

	UINT8 ir;						/* instruction register */
	UINT8 mor;						/* mode control register */
	UINT8 pr;						/* character pitch register */
	UINT8 hnr;						/* horizontal character number register */
	UINT8 dvr;						/* duty number register */
	UINT8 cpr;						/* cursor form register */
	UINT8 slr;						/* start address (lower) register */
	UINT8 sur;						/* start address (upper) register */
	UINT8 clr;						/* cursor address (lower) register */
	UINT8 cur;						/* cursor address (upper) register */

	int cursor;						/* is cursor displayed */
	int frame;						/* frame counter */
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE msm6255_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	return (msm6255_t *)device->token;
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    msm6255_register_r - register read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( msm6255_register_r )
{
	msm6255_t *msm6255 = get_safe_token(device);

	if (offset & 0x01)
	{
		return msm6255->ir;
	}
	else
	{
		switch (msm6255->ir)
		{
		case MSM6255_REGISTER_MOR:
			return 0x00; // write-only

		case MSM6255_REGISTER_PR:
			return msm6255->pr;

		case MSM6255_REGISTER_HNR:
			return msm6255->hnr;

		case MSM6255_REGISTER_DVR:
			return 0x00; // write-only

		case MSM6255_REGISTER_CPR:
			return msm6255->cpr;

		case MSM6255_REGISTER_SLR:
			return msm6255->slr;

		case MSM6255_REGISTER_SUR:
			return msm6255->sur;

		case MSM6255_REGISTER_CLR:
			return msm6255->clr;

		case MSM6255_REGISTER_CUR:
			return msm6255->cur;

		default:
			return 0;
		}
	}
}

/*-------------------------------------------------
    msm6255_register_w - register write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( msm6255_register_w )
{
	msm6255_t *msm6255 = get_safe_token(device);

	if (offset & 0x01)
	{
		msm6255->ir = data & 0x0f;
	}
	else
	{
		switch (msm6255->ir)
		{
		case MSM6255_REGISTER_MOR:
			msm6255->mor = data & 0x7f;
			break;

		case MSM6255_REGISTER_PR:
			msm6255->pr = data & 0xf7;
			break;

		case MSM6255_REGISTER_HNR:
			msm6255->hnr = data & 0x7f;
			break;

		case MSM6255_REGISTER_DVR:
			msm6255->dvr = data;
			break;

		case MSM6255_REGISTER_CPR:
			msm6255->cpr = data;
			break;

		case MSM6255_REGISTER_SLR:
			msm6255->slr = data;
			break;

		case MSM6255_REGISTER_SUR:
			msm6255->sur = data;
			break;

		case MSM6255_REGISTER_CLR:
			msm6255->clr = data;
			break;

		case MSM6255_REGISTER_CUR:
			msm6255->cur = data;
			break;
		}
	}
}

/*-------------------------------------------------
    update_cursor - update cursor state
-------------------------------------------------*/

static void update_cursor(const device_config *device)
{
	msm6255_t *msm6255 = get_safe_token(device);

	if (msm6255->mor & MSM6255_MOR_CURSOR_ON)
	{
		if (msm6255->mor & MSM6255_MOR_CURSOR_BLINK)
		{
			if (msm6255->mor & MSM6255_MOR_BLINK_TIME_16)
			{
				if (msm6255->frame == 16)
				{
					msm6255->cursor = !msm6255->cursor;
					msm6255->frame = 0;
				}
				else
				{
					msm6255->frame++;
				}
			}
			else
			{
				if (msm6255->frame == 32)
				{
					msm6255->cursor = !msm6255->cursor;
					msm6255->frame = 0;
				}
				else
				{
					msm6255->frame++;
				}
			}
		}
		else
		{
			msm6255->cursor = 1;
		}
	}
	else
	{
		msm6255->cursor = 0;
	}
}

/*-------------------------------------------------
    draw_scanline - draw one scanline
-------------------------------------------------*/

static void draw_scanline(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect, int y, UINT16 ma, UINT8 ra)
{
	msm6255_t *msm6255 = get_safe_token(device);

	UINT8 hp = (msm6255->pr & MSM6255_PR_HP_MASK) + 1;
	UINT8 hn = (msm6255->hnr & MSM6255_HNR_HN_MASK) + 1;
	UINT8 cpu = msm6255->cpr & MSM6255_CPR_CPU_MASK;
	UINT8 cpd = msm6255->cpr & MSM6255_CPR_CPD_MASK;
	UINT16 car = (msm6255->cur << 8) | msm6255->clr;

	int sx, x;

	for (sx = 0; sx < hn; sx++)
	{
		UINT8 data = msm6255->intf->char_ram_r(device, ma, ra);

		if (msm6255->cursor)
		{
			if (ma == car)
			{
				if (ra >= cpu && ra <= cpd)
				{
					data ^= 0xff;
				}
			}
		}

		for (x = 0; x < hp; x++)
		{
			*BITMAP_ADDR16(bitmap, y, (sx * hp) + x) = BIT(data, 7);

			data <<= 1;
		}

		ma++;
	}
}

/*-------------------------------------------------
    update_graphics - draw graphics mode screen
-------------------------------------------------*/

static void update_graphics(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect)
{
	msm6255_t *msm6255 = get_safe_token(device);

	UINT8 hn = (msm6255->hnr & MSM6255_HNR_HN_MASK) + 1;
	UINT8 nx = (msm6255->dvr & MSM6255_DVR_DN_MASK) + 1;
	UINT16 sar = (msm6255->sur << 8) | msm6255->slr;

	int y;

	msm6255->cursor = 0;
	msm6255->frame = 0;

	for (y = 0; y < nx; y++)
	{
		/* draw upper half scanline */
		UINT16 ma = sar + (y * hn);
		draw_scanline(device, bitmap, cliprect, y, ma, 0);

		/* draw lower half scanline */
		ma = sar + ((y + nx) * hn);
		draw_scanline(device, bitmap, cliprect, y + nx, ma, 0);
	}
}

/*-------------------------------------------------
    update_text - draw text mode screen
-------------------------------------------------*/

static void update_text(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect)
{
	msm6255_t *msm6255 = get_safe_token(device);

	UINT8 hn = (msm6255->hnr & MSM6255_HNR_HN_MASK) + 1;
	UINT8 vp = (msm6255->pr & MSM6255_PR_VP_MASK) + 1;
	UINT8 nx = (msm6255->dvr & MSM6255_DVR_DN_MASK) + 1;
	UINT16 sar = (msm6255->sur << 8) | msm6255->slr;

	int sy, y;

	update_cursor(device);

	for (sy = 0; sy < nx; sy++)
	{
		for (y = 0; y < vp; y++)
		{
			/* draw upper half scanline */
			UINT16 ma = sar + ((sy * vp) + y) * hn;
			draw_scanline(device, bitmap, cliprect, (sy * vp) + y, ma, y);

			/* draw lower half scanline */
			ma = sar + (((sy + nx) * vp) + y) * hn;
			draw_scanline(device, bitmap, cliprect, (sy * vp) + y, ma, y);
		}
	}
}

/*-------------------------------------------------
    update_text - draw screen
-------------------------------------------------*/

void msm6255_update(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect)
{
	msm6255_t *msm6255 = get_safe_token(device);

	if (msm6255->mor & MSM6255_MOR_DISPLAY_ON)
	{
		if (msm6255->mor & MSM6255_MOR_GRAPHICS)
		{
			update_graphics(device, bitmap, cliprect);
		}
		else
		{
			update_text(device, bitmap, cliprect);
		}
	}
	else
	{
		bitmap_fill(bitmap, cliprect, get_black_pen(device->machine));
	}
}

/*-------------------------------------------------
    DEVICE_START( msm6255 )
-------------------------------------------------*/

static DEVICE_START( msm6255 )
{
	msm6255_t *msm6255 = get_safe_token(device);

	/* resolve callbacks */
	msm6255->intf = device->static_config;
	assert(msm6255->intf->char_ram_r != NULL);

	/* get the screen */
	msm6255->screen = devtag_get_device(device->machine, msm6255->intf->screen_tag);
	assert(msm6255->screen != NULL);

	/* register for state saving */
	state_save_register_device_item(device, 0, msm6255->ir);
	state_save_register_device_item(device, 0, msm6255->mor);
	state_save_register_device_item(device, 0, msm6255->pr);
	state_save_register_device_item(device, 0, msm6255->hnr);
	state_save_register_device_item(device, 0, msm6255->dvr);
	state_save_register_device_item(device, 0, msm6255->cpr);
	state_save_register_device_item(device, 0, msm6255->slr);
	state_save_register_device_item(device, 0, msm6255->sur);
	state_save_register_device_item(device, 0, msm6255->clr);
	state_save_register_device_item(device, 0, msm6255->cur);
	state_save_register_device_item(device, 0, msm6255->cursor);
	state_save_register_device_item(device, 0, msm6255->frame);
}

/*-------------------------------------------------
    DEVICE_RESET( msm6255 )
-------------------------------------------------*/

static DEVICE_RESET( msm6255 )
{
	msm6255_t *msm6255 = get_safe_token(device);

	msm6255->frame = 0;
}

/*-------------------------------------------------
    DEVICE_GET_INFO( msm6255 )
-------------------------------------------------*/

DEVICE_GET_INFO( msm6255 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(msm6255_t);					break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;									break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(msm6255);		break;
		case DEVINFO_FCT_STOP:							/* Nothing */									break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(msm6255);		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "OKI MSM6255");					break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "OKI MSM6255");					break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);						break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");			break;
	}
}
