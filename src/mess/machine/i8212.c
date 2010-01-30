/**********************************************************************

    Intel 8212 8-Bit Input/Output Port emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "emu.h"
#include "i8212.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 1

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _i8212_t i8212_t;
struct _i8212_t
{
	devcb_resolved_write_line	out_int_func;
	devcb_resolved_read8		in_di_func;
	devcb_resolved_write8		out_do_func;

	int md;						/* mode */
	int stb;					/* strobe */
	UINT8 data;					/* data latch */
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE i8212_t *get_safe_token(running_device *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	return (i8212_t *)device->token;
}

INLINE const i8212_interface *get_interface(running_device *device)
{
	assert(device != NULL);
	assert((device->type == I8212));
	return (const i8212_interface *) device->baseconfig().static_config;
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    i8212_r - data output read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( i8212_r )
{
	i8212_t *i8212 = get_safe_token(device);

	/* clear interrupt line */
	devcb_call_write_line(&i8212->out_int_func, CLEAR_LINE);

	if (LOG) logerror("I8212 '%s' INT: %u\n", device->tag.cstr(), CLEAR_LINE);

	return i8212->data;
}

/*-------------------------------------------------
    i8212_w - data input write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( i8212_w )
{
	i8212_t *i8212 = get_safe_token(device);

	/* latch data */
	i8212->data = data;

	/* output data */
	devcb_call_write8(&i8212->out_do_func, 0, i8212->data);
}

/*-------------------------------------------------
    i8212_md_w - mode write
-------------------------------------------------*/

#ifdef UNUSED_FUNCTION
WRITE_LINE_DEVICE_HANDLER( i8212_md_w )
{
	i8212_t *i8212 = get_safe_token(device);

	if (LOG) logerror("I8212 '%s' Mode: %s\n", device->tag.cstr(), state ? "output" : "input");

	i8212->md = state;
}
#endif

/*-------------------------------------------------
    i8212_stb_w - strobe write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( i8212_stb_w )
{
	i8212_t *i8212 = get_safe_token(device);

	if (LOG) logerror("I8212 '%s' STB: %u\n", device->tag.cstr(), state);

	if (i8212->md == I8212_MODE_INPUT)
	{
		if (i8212->stb && !state)
		{
			/* input data */
			i8212->data = devcb_call_read8(&i8212->in_di_func, 0);

			/* assert interrupt line */
			devcb_call_write_line(&i8212->out_int_func, ASSERT_LINE);

			if (LOG) logerror("I8212 '%s' INT: %u\n", device->tag.cstr(), ASSERT_LINE);
		}
	}

	i8212->stb = state;
}

/*-------------------------------------------------
    DEVICE_START( i8212 )
-------------------------------------------------*/

static DEVICE_START( i8212 )
{
	i8212_t *i8212 = get_safe_token(device);
	const i8212_interface *intf = get_interface(device);

	/* resolve callbacks */
	devcb_resolve_write_line(&i8212->out_int_func, &intf->out_int_func, device);
	devcb_resolve_read8(&i8212->in_di_func, &intf->in_di_func, device);
	devcb_resolve_write8(&i8212->out_do_func, &intf->out_do_func, device);

	/* register for state saving */
	state_save_register_device_item(device, 0, i8212->md);
	state_save_register_device_item(device, 0, i8212->stb);
	state_save_register_device_item(device, 0, i8212->data);
}

/*-------------------------------------------------
    DEVICE_RESET( i8212 )
-------------------------------------------------*/

static DEVICE_RESET( i8212 )
{
	i8212_t *i8212 = get_safe_token(device);

	i8212->data = 0;

	if (i8212->md == I8212_MODE_OUTPUT)
	{
		/* output data */
		devcb_call_write8(&i8212->out_do_func, 0, i8212->data);
	}
}

/*-------------------------------------------------
    DEVICE_GET_INFO( i8212 )
-------------------------------------------------*/

DEVICE_GET_INFO( i8212 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(i8212_t);					break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(i8212);		break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(i8212);		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Intel 8212");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Intel MCS-80");			break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");		break;
	}
}
