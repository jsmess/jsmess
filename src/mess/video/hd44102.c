/**********************************************************************

    HD44102 Dot Matrix Liquid Crystal Graphic Display Column Driver emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

    TODO:

    - 4 bit mode
    - busy flag
    - reset flag

*/

#include "emu.h"
#include "hd44102.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

#define HD44102_CONTROL_DISPLAY_OFF			0x38
#define HD44102_CONTROL_DISPLAY_ON			0x39
#define HD44102_CONTROL_COUNT_DOWN_MODE		0x3a
#define HD44102_CONTROL_COUNT_UP_MODE		0x3b
#define HD44102_CONTROL_Y_ADDRESS_MASK		0x3f
#define HD44102_CONTROL_X_ADDRESS_MASK		0xc0
#define HD44102_CONTROL_DISPLAY_START_PAGE	0x3e

#define HD44102_STATUS_BUSY					0x80	/* not supported */
#define HD44102_STATUS_COUNT_UP				0x40
#define HD44102_STATUS_DISPLAY_OFF			0x20
#define HD44102_STATUS_RESET				0x10	/* not supported */

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _hd44102_t hd44102_t;
struct _hd44102_t
{
	screen_device *screen;	/* screen */

	UINT8 ram[4][50];				/* display memory */

	UINT8 status;					/* status register */
	UINT8 output;					/* output register */

	int cs2;						/* chip select */
	int page;						/* display start page */
	int x;							/* X address */
	int y;							/* Y address */
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE hd44102_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == HD44102);

	return (hd44102_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE hd44102_config *get_safe_config(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == HD44102);

	return (hd44102_config *)downcast<const legacy_device_config_base &>(device->baseconfig()).inline_config();
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    hd44102_status_r - status read
-------------------------------------------------*/

static READ8_DEVICE_HANDLER( hd44102_status_r )
{
	hd44102_t *hd44102 = get_safe_token(device);

	return hd44102->status;
}

/*-------------------------------------------------
    hd44102_control_w - control write
-------------------------------------------------*/

static WRITE8_DEVICE_HANDLER( hd44102_control_w )
{
	hd44102_t *hd44102 = get_safe_token(device);

	if (hd44102->status & HD44102_STATUS_BUSY) return;

	switch (data)
	{
	case HD44102_CONTROL_DISPLAY_OFF:
		if (LOG) logerror("HD44102 '%s' Display Off\n", device->tag());

		hd44102->status |= HD44102_STATUS_DISPLAY_OFF;
		break;

	case HD44102_CONTROL_DISPLAY_ON:
		if (LOG) logerror("HD44102 '%s' Display On\n", device->tag());

		hd44102->status &= ~HD44102_STATUS_DISPLAY_OFF;
		break;

	case HD44102_CONTROL_COUNT_DOWN_MODE:
		if (LOG) logerror("HD44102 '%s' Count Down Mode\n", device->tag());

		hd44102->status &= ~HD44102_STATUS_COUNT_UP;
		break;

	case HD44102_CONTROL_COUNT_UP_MODE:
		if (LOG) logerror("HD44102 '%s' Count Up Mode\n", device->tag());

		hd44102->status |= HD44102_STATUS_COUNT_UP;
		break;

	default:
		{
		int x = (data & HD44102_CONTROL_X_ADDRESS_MASK) >> 6;
		int y = data & HD44102_CONTROL_Y_ADDRESS_MASK;

		if ((data & HD44102_CONTROL_Y_ADDRESS_MASK) == HD44102_CONTROL_DISPLAY_START_PAGE)
		{
			if (LOG) logerror("HD44102 '%s' Display Start Page %u\n", device->tag(), x);

			hd44102->page = x;
		}
		else if (y > 49)
		{
			logerror("HD44102 '%s' Invalid Address X %u Y %u (%02x)!\n", device->tag(), data, x, y);
		}
		else
		{
			if (LOG) logerror("HD44102 '%s' Address X %u Y %u (%02x)\n", device->tag(), data, x, y);

			hd44102->x = x;
			hd44102->y = y;
		}
		}
	}
}

/*-------------------------------------------------
    count_up_or_down - counts Y up or down
-------------------------------------------------*/

static void count_up_or_down(hd44102_t *hd44102)
{
	if (hd44102->status & HD44102_STATUS_COUNT_UP)
	{
		if (++hd44102->y > 49) hd44102->y = 0;
	}
	else
	{
		if (--hd44102->y < 0) hd44102->y = 49;
	}
}

/*-------------------------------------------------
    hd44102_data_r - data read
-------------------------------------------------*/

static READ8_DEVICE_HANDLER( hd44102_data_r )
{
	hd44102_t *hd44102 = get_safe_token(device);

	UINT8 data = hd44102->output;

	hd44102->output = hd44102->ram[hd44102->x][hd44102->y];

	count_up_or_down(hd44102);

	return data;
}

/*-------------------------------------------------
    hd44102_data_w - data write
-------------------------------------------------*/

static WRITE8_DEVICE_HANDLER( hd44102_data_w )
{
	hd44102_t *hd44102 = get_safe_token(device);

	hd44102->ram[hd44102->x][hd44102->y] = data;

	count_up_or_down(hd44102);
}

/*-------------------------------------------------
    hd44102_update - update screen
-------------------------------------------------*/

void hd44102_update(device_t *device, bitmap_t *bitmap, const rectangle *cliprect)
{
	hd44102_t *hd44102 = get_safe_token(device);
	const hd44102_config *config = get_safe_config(device);

	int x, y, z;

	for (y = 0; y < 50; y++)
	{
		z = hd44102->page << 3;

		for (x = 0; x < 32; x++)
		{
			UINT8 data = hd44102->ram[z / 8][y];

			int sy = config->sy + z;
			int sx = config->sx + y;

			if ((sy >= cliprect->min_y) && (sy <= cliprect->max_y) && (sx >= cliprect->min_x) && (sx <= cliprect->max_x))
			{
				int color = (hd44102->status & HD44102_STATUS_DISPLAY_OFF) ? 0 : BIT(data, z % 8);

				*BITMAP_ADDR16(bitmap, sy, sx) = color;
			}

			z++;
			z %= 32;
		}
	}
}

/*-------------------------------------------------
    hd44102_cs2_w - chip select write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( hd44102_cs2_w )
{
	hd44102_t *hd44102 = get_safe_token(device);

	hd44102->cs2 = state;
}

/*-------------------------------------------------
    hd44102_r - register read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( hd44102_r )
{
	hd44102_t *hd44102 = get_safe_token(device);
	UINT8 data = 0;

	if (hd44102->cs2)
	{
		data = (offset & 0x01) ? hd44102_data_r(device, offset) : hd44102_status_r(device, offset);
	}

	return data;
}

/*-------------------------------------------------
    hd44102_w - register write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( hd44102_w )
{
	hd44102_t *hd44102 = get_safe_token(device);

	if (hd44102->cs2)
	{
		(offset & 0x01) ? hd44102_data_w(device, offset, data) : hd44102_control_w(device, offset, data);
	}
}

/*-------------------------------------------------
    DEVICE_START( hd44102 )
-------------------------------------------------*/

static DEVICE_START( hd44102 )
{
	hd44102_t *hd44102 = get_safe_token(device);
	const hd44102_config *config = get_safe_config(device);

	/* get the screen device */
	hd44102->screen = device->machine().device<screen_device>(config->screen_tag);
	assert(hd44102->screen != NULL);

	/* register for state saving */
	device->save_item(NAME(hd44102->cs2));
	device->save_item(NAME(hd44102->status));
	device->save_item(NAME(hd44102->output));
	device->save_item(NAME(hd44102->page));
	device->save_item(NAME(hd44102->x));
	device->save_item(NAME(hd44102->y));
}

/*-------------------------------------------------
    DEVICE_RESET( hd44102 )
-------------------------------------------------*/

static DEVICE_RESET( hd44102 )
{
	hd44102_t *hd44102 = get_safe_token(device);

	hd44102->status = HD44102_STATUS_DISPLAY_OFF | HD44102_STATUS_COUNT_UP;
}

/*-------------------------------------------------
    DEVICE_GET_INFO( hd44102 )
-------------------------------------------------*/

DEVICE_GET_INFO( hd44102 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(hd44102_t);						break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = sizeof(hd44102_config);					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(hd44102);			break;
		case DEVINFO_FCT_STOP:							/* Nothing */										break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(hd44102);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Hitachi HD44102");					break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Hitachi HD44102");					break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");								break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);							break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");				break;
	}
}

DEFINE_LEGACY_DEVICE(HD44102, hd44102);
