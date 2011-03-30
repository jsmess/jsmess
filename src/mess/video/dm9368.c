/**********************************************************************

    Fairchild DM9368 7-Segment Decoder/Driver/Latch emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "emu.h"
#include "dm9368.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

static const UINT8 OUTPUT[16] =
{
	0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x67, 0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71
};

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _dm9368_t dm9368_t;
struct _dm9368_t
{
	int digit;
	UINT8 latch;

	int rbi;

	device_t *rbo_device;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE dm9368_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == DM9368);

	return (dm9368_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE dm9368_config *get_safe_config(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == DM9368);

	return (dm9368_config *)downcast<const legacy_device_config_base &>(device->baseconfig()).inline_config();
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    dm9368_w - latch input
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( dm9368_w )
{
	dm9368_t *dm9368 = get_safe_token(device);

	int a = data & 0x0f;

	if (!dm9368->rbi && !a)
	{
		if (LOG) logerror("DM9368 '%s' Blanked Rippling Zero\n", device->tag());

		/* blank rippling 0 */
		output_set_digit_value(dm9368->digit, 0);

		if (dm9368->rbo_device)
		{
			dm9368_rbi_w(dm9368->rbo_device, 0);
		}
	}
	else
	{
		if (LOG) logerror("DM9368 '%s' Output Data: %u = %02x\n", device->tag(), a, OUTPUT[a]);

		output_set_digit_value(dm9368->digit, OUTPUT[a]);

		if (dm9368->rbo_device)
		{
			dm9368_rbi_w(dm9368->rbo_device, 1);
		}
	}
}

/*-------------------------------------------------
    dm9368_rbi_w - ripple blanking input
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( dm9368_rbi_w )
{
	dm9368_t *dm9368 = get_safe_token(device);

	dm9368->rbi = state;

	if (LOG) logerror("DM9368 '%s' Ripple Blanking Input: %u\n", device->tag(), state);
}

/*-------------------------------------------------
    DEVICE_START( dm9368 )
-------------------------------------------------*/

static DEVICE_START( dm9368 )
{
	dm9368_t *dm9368 = get_safe_token(device);
	const dm9368_config *config = get_safe_config(device);

	dm9368->digit = config->digit;

	/* get the screen device */
	if (config->rbo_tag)
	{
		dm9368->rbo_device = device->machine().device(config->rbo_tag);
	}

	/* register for state saving */
	device->save_item(NAME(dm9368->latch));
	device->save_item(NAME(dm9368->rbi));
}

/*-------------------------------------------------
    DEVICE_GET_INFO( dm9368 )
-------------------------------------------------*/

DEVICE_GET_INFO( dm9368 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(dm9368_t);							break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = sizeof(dm9368_config);					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(dm9368);			break;
		case DEVINFO_FCT_STOP:							/* Nothing */										break;
		case DEVINFO_FCT_RESET:							/* Nothing */										break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Fairchild DM9368");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Fairchild DM9368");				break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");								break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);							break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");				break;
	}
}

DEFINE_LEGACY_DEVICE(DM9368, dm9368);
