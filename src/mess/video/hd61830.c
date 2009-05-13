/**********************************************************************

    HD61830 LCD Timing Controller emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

	TODO:

	- clean up code
	- clear/set bit instructions
	- instruction timing
	- text mode

*/

#include "driver.h"
#include "hd61830.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

enum
{
	HD61380_REGISTER_MODE_CONTROL = 0,
	HD61380_REGISTER_CHARACTER_PITCH,
	HD61380_REGISTER_NUMBER_OF_CHARACTERS,
	HD61380_REGISTER_NUMBER_OF_TIME_DIVISIONS,
	HD61380_REGISTER_CURSOR_POSITION,
	HD61380_REGISTER_DISPLAY_START_LOW = 8,
	HD61380_REGISTER_DISPLAY_START_HIGH,
	HD61380_REGISTER_CURSOR_ADDRESS_LOW,
	HD61380_REGISTER_CURSOR_ADDRESS_HIGH,
	HD61380_REGISTER_DISPLAY_DATA_WRITE,
	HD61380_REGISTER_DISPLAY_DATA_READ,
	HD61380_REGISTER_CLEAR_BIT,						/* not supported */
	HD61380_REGISTER_SET_BIT						/* not supported */
};

#define HD61380_MODE_EXTERNAL_CG	0x01	/* not supported */
#define HD61380_MODE_GRAPHIC		0x02	/* text mode not supported */
#define HD61380_MODE_CURSOR			0x04	/* not supported */
#define HD61380_MODE_BLINK			0x08	/* not supported */
#define HD61380_MODE_MASTER			0x10	/* not supported */
#define HD61380_MODE_DISPLAY_ON		0x20

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _hd61830_t hd61830_t;
struct _hd61830_t
{
	devcb_resolved_read8		in_rd_func;
	devcb_resolved_write8		out_rd_func;

	int bf;							/* busy flag, not supported */

	UINT8 ir;						/* instruction register */
	UINT8 mcr;						/* mode control register */
	UINT8 dir;						/* data input register */
	UINT8 dor;						/* data output register */
	UINT16 rac1;					/* refresh address counter 1 */
	UINT16 rac2;					/* refresh address counter 2 */
	UINT16 cac;						/* cursor address counter */
	UINT8 lac;						/* line address counter */

	UINT16 dsa;						/* display start address */

	int vp;							/* vertical character pitch */
	int hp;							/* horizontal character pitch */
	int hn;							/* horizontal number of characters */
	int nx;							/* number of time divisions */
	int cp;							/* cursor position */

	const device_config *screen;	/* screen */
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE hd61830_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	return (hd61830_t *)device->token;
}

INLINE const hd61830_interface *get_interface(const device_config *device)
{
	assert(device != NULL);
	assert((device->type == HD61830));
	return (const hd61830_interface *) device->static_config;
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    hd61830_status_r - status read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( hd61830_status_r )
{
	hd61830_t *hd61830 = get_safe_token(device);

	return hd61830->bf << 7;
}

/*-------------------------------------------------
    hd61830_control_w - control write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( hd61830_control_w )
{
	hd61830_t *hd61830 = get_safe_token(device);

	hd61830->ir = data;
}

/*-------------------------------------------------
    hd61830_data_r - data read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( hd61830_data_r )
{
	hd61830_t *hd61830 = get_safe_token(device);

	UINT8 data = hd61830->dor;

	if (LOG) logerror("HD61380 '%s' Display Data Read %02x\n", device->tag, hd61830->dor);

	hd61830->dor = devcb_call_read8(&hd61830->in_rd_func, hd61830->cac);

	hd61830->cac++;

	return data;
}

/*-------------------------------------------------
    hd61830_data_w - data write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( hd61830_data_w )
{
	hd61830_t *hd61830 = get_safe_token(device);

	switch (hd61830->ir)
	{
	case HD61380_REGISTER_MODE_CONTROL:
		hd61830->mcr = data;

		if (LOG)
		{
			logerror("HD61380 '%s' %s CG\n", device->tag, (data & HD61380_MODE_EXTERNAL_CG) ? "External" : "Internal");
			logerror("HD61380 '%s' %s Display Mode\n", device->tag, (data & HD61380_MODE_GRAPHIC) ? "Graphic" : "Character");
			logerror("HD61380 '%s' %s Mode\n", device->tag, (data & HD61380_MODE_MASTER) ? "Master" : "Slave");
			logerror("HD61380 '%s' Cursor %s\n", device->tag, (data & HD61380_MODE_CURSOR) ? "On" : "Off");
			logerror("HD61380 '%s' Blink %s\n", device->tag, (data & HD61380_MODE_BLINK) ? "On" : "Off");
			logerror("HD61380 '%s' Display %s\n", device->tag, (data & HD61380_MODE_DISPLAY_ON) ? "On" : "Off");
		}
		break;
	
	case HD61380_REGISTER_CHARACTER_PITCH:
		hd61830->hp = (data & 0x07) + 1;
		hd61830->vp = (data >> 4) + 1;

		if (LOG) logerror("HD61380 '%s' Horizontal Character Pitch: %u\n", device->tag, hd61830->hp);
		if (LOG) logerror("HD61380 '%s' Vertical Character Pitch: %u\n", device->tag, hd61830->vp);
		break;

	case HD61380_REGISTER_NUMBER_OF_CHARACTERS:
		hd61830->hn = (data & 0x7f) + 1;

		if (LOG) logerror("HD61380 '%s' Number of Characters: %u\n", device->tag, hd61830->hn);
		break;

	case HD61380_REGISTER_NUMBER_OF_TIME_DIVISIONS:
		hd61830->nx = (data & 0x7f) + 1;

		if (LOG) logerror("HD61380 '%s' Number of Time Divisions: %u\n", device->tag, hd61830->nx);
		break;

	case HD61380_REGISTER_CURSOR_POSITION:
		hd61830->cp = (data & 0x7f) + 1;

		if (LOG) logerror("HD61380 '%s' Cursor Position: %u\n", device->tag, hd61830->cp);
		break;

	case HD61380_REGISTER_DISPLAY_START_LOW:
		hd61830->dsa = (hd61830->dsa & 0xff00) | data;

		if (LOG) logerror("HD61380 '%s' Display Start Address Low %04x\n", device->tag, hd61830->dsa);
		break;

	case HD61380_REGISTER_DISPLAY_START_HIGH:
		hd61830->dsa = (data << 8) | (hd61830->dsa & 0xff);

		if (LOG) logerror("HD61380 '%s' Display Start Address High %04x\n", device->tag, hd61830->dsa);
		break;

	case HD61380_REGISTER_CURSOR_ADDRESS_LOW:
		if (BIT(hd61830->cac, 7) && !BIT(data, 7))
		{
			hd61830->cac = ((((hd61830->cac & 0xff00) >> 8) + 1) << 8) | data;
		}
		else
		{
			hd61830->cac = (hd61830->cac & 0xff00) | data;
		}

		if (LOG) logerror("HD61380 '%s' Cursor Address Low %02x: %04x\n", device->tag, data, hd61830->cac);
		break;

	case HD61380_REGISTER_CURSOR_ADDRESS_HIGH:
		hd61830->cac = (data << 8) | (hd61830->cac & 0xff);

		if (LOG) logerror("HD61380 '%s' Cursor Address High %02x: %04x\n", device->tag, data, hd61830->cac);
		break;

	case HD61380_REGISTER_DISPLAY_DATA_WRITE:
		devcb_call_write8(&hd61830->out_rd_func, hd61830->cac, data);

		if (LOG) logerror("HD61380 '%s' Display Data Write %02x -> %04x row %u col %u\n", device->tag, data, hd61830->cac, hd61830->cac / 40, hd61830->cac % 40);

		hd61830->cac++;
		break;

	case HD61380_REGISTER_CLEAR_BIT:
		logerror("HD61380 '%s' Clear Bit not supported!\n", device->tag);
		break;

	case HD61380_REGISTER_SET_BIT:
		logerror("HD61380 '%s' Set Bit not supported!\n", device->tag);
		break;

	default:
		logerror("HD61830 '%s' Illegal Instruction %02x!\n", device->tag, hd61830->ir);
	}
}

/*-------------------------------------------------
    hd61830_update - update screen
-------------------------------------------------*/

void hd61830_update(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect)
{
	hd61830_t *hd61830 = get_safe_token(device);
	int y, sx, x;
	UINT16 lac = hd61830->dsa;

	for (y = 0; y < (hd61830->nx * 2); y++)
	{
		for (sx = 0; sx < hd61830->hn; sx++)
		{
			UINT8 data = devcb_call_read8(&hd61830->in_rd_func, lac++);

			for (x = 0; x < hd61830->hp; x++)
			{
				*BITMAP_ADDR16(bitmap, y, (sx * hd61830->hp) + x) = BIT(data, 0);
				data >>= 1;
			}
		}
	}
}

/*-------------------------------------------------
    DEVICE_START( hd61830 )
-------------------------------------------------*/

static DEVICE_START( hd61830 )
{
	hd61830_t *hd61830 = get_safe_token(device);
	const hd61830_interface *intf = get_interface(device);

	/* resolve callbacks */
	devcb_resolve_read8(&hd61830->in_rd_func, &intf->in_rd_func, device);
	devcb_resolve_write8(&hd61830->out_rd_func, &intf->out_rd_func, device);

	/* get the screen device */
	hd61830->screen = devtag_get_device(device->machine, intf->screen_tag);
	assert(hd61830->screen != NULL);

	/* register for state saving */
	state_save_register_device_item(device, 0, hd61830->bf);
	state_save_register_device_item(device, 0, hd61830->ir);
	state_save_register_device_item(device, 0, hd61830->mcr);
	state_save_register_device_item(device, 0, hd61830->dir);
	state_save_register_device_item(device, 0, hd61830->dor);
	state_save_register_device_item(device, 0, hd61830->rac1);
	state_save_register_device_item(device, 0, hd61830->rac2);
	state_save_register_device_item(device, 0, hd61830->cac);
	state_save_register_device_item(device, 0, hd61830->lac);
	state_save_register_device_item(device, 0, hd61830->dsa);
	state_save_register_device_item(device, 0, hd61830->vp);
	state_save_register_device_item(device, 0, hd61830->hp);
	state_save_register_device_item(device, 0, hd61830->hn);
	state_save_register_device_item(device, 0, hd61830->nx);
	state_save_register_device_item(device, 0, hd61830->cp);
}

/*-------------------------------------------------
    DEVICE_RESET( hd61830 )
-------------------------------------------------*/

static DEVICE_RESET( hd61830 )
{
	//hd61830_t *hd61830 = get_safe_token(device);

}

/*-------------------------------------------------
    DEVICE_GET_INFO( hd61830 )
-------------------------------------------------*/

DEVICE_GET_INFO( hd61830 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(hd61830_t);						break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;										break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(hd61830);			break;
		case DEVINFO_FCT_STOP:							/* Nothing */										break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(hd61830);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Hitachi HD61830");					break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Hitachi HD61830");					break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");								break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);							break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");				break;
	}
}
