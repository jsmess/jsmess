/*********************************************************************

    ap2_slot.c

    Implementation of Apple II device slots

*********************************************************************/

#include "ap2_slot.h"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _apple2_slot_token apple2_slot_token;
struct _apple2_slot_token
{
	running_device *slot_device;
};



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE void assert_valid(running_device *device)
{
	assert(device != NULL);
	assert(device->type == APPLE2_SLOT);
}



INLINE const apple2_slot_config *get_config(running_device *device)
{
	assert_valid(device);
	return (const apple2_slot_config *) device->baseconfig().inline_config;
}



INLINE apple2_slot_token *get_token(running_device *device)
{
	assert_valid(device);
	return (apple2_slot_token *) device->token;
}



/***************************************************************************
    CORE IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    apple2_slot - looks up a slot
-------------------------------------------------*/

running_device *apple2_slot(running_machine *machine, int slotnum)
{
	char buffer[7];

	assert((slotnum >= 0) && (slotnum <= 7));
	snprintf(buffer, ARRAY_LENGTH(buffer), "slot_%d", slotnum);

	return devtag_get_device(machine, buffer);
}



/*-------------------------------------------------
    DEVICE_START(apple2_slot) - device start
    function
-------------------------------------------------*/

static DEVICE_START(apple2_slot)
{
	const apple2_slot_config *config = get_config(device);
	apple2_slot_token *token = get_token(device);

	if (config->tag != NULL)
	{
		/* locate the device */
		token->slot_device = devtag_get_device(device->machine, config->tag);

		assert(token->slot_device != NULL);
	}
	else
	{
		token->slot_device = NULL;
	}
}



/*-------------------------------------------------
    apple2_slot_r - slot read function
-------------------------------------------------*/

READ8_DEVICE_HANDLER(apple2_slot_r)
{
	UINT8 result = 0x00;
	const apple2_slot_config *config = get_config(device);
	apple2_slot_token *token = get_token(device);

	/* do we actually have a device, and can we read? */
	if ((token->slot_device != NULL) && (config->rh != NULL))
	{
		result = (*config->rh)(token->slot_device, offset);
	}

	return result;
}



/*-------------------------------------------------
    apple2_slot_w - slot write function
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER(apple2_slot_w)
{
	const apple2_slot_config *config = get_config(device);
	apple2_slot_token *token = get_token(device);

	/* do we actually have a device, and can we read? */
	if ((token->slot_device != NULL) && (config->wh != NULL))
	{
		(*config->wh)(token->slot_device, offset, data);
	}
}

/*-------------------------------------------------
    apple2_c800_slot_r - slot read function
-------------------------------------------------*/

READ8_DEVICE_HANDLER(apple2_c800_slot_r)
{
	UINT8 result = 0x00;
	const apple2_slot_config *config = get_config(device);
	apple2_slot_token *token = get_token(device);

	/* do we actually have a device, and can we read? */
	if ((token->slot_device != NULL) && (config->rhc800 != NULL))
	{
		result = (*config->rhc800)(token->slot_device, offset);
	}

	return result;
}

/*-------------------------------------------------
    apple2_c800_slot_w - slot write function
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER(apple2_c800_slot_w)
{
	const apple2_slot_config *config = get_config(device);
	apple2_slot_token *token = get_token(device);

	/* do we actually have a device, and can we read? */
	if ((token->slot_device != NULL) && (config->whc800 != NULL))
	{
		(*config->whc800)(token->slot_device, offset, data);
	}
}

/*-------------------------------------------------
    apple2_slot_ROM_w - slot write function
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER(apple2_slot_ROM_w)
{
	const apple2_slot_config *config = get_config(device);
	apple2_slot_token *token = get_token(device);

	/* do we actually have a device, and can we read? */
	if ((token->slot_device != NULL) && (config->whcnxx != NULL))
	{
		(*config->whcnxx)(token->slot_device, offset, data);
	}
}

/*-------------------------------------------------
    DEVICE_GET_INFO(apple2_slot) - device get info
    function
-------------------------------------------------*/

DEVICE_GET_INFO(apple2_slot)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(apple2_slot_token);		break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = sizeof(apple2_slot_config);		break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(apple2_slot);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/* Nothing */								break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Apple II Slot");					break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Apple II Slot");					break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);							break;
	}
}
