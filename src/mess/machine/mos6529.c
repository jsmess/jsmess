/**********************************************************************

    MOS Technology 6529 Single Port Interface Adapter emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "mos6529.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _mos6529_t mos6529_t;
struct _mos6529_t
{
	devcb_resolved_read8		in_p_func;
	devcb_resolved_write8		out_p_func;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE mos6529_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == MOS6529);

	return (mos6529_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE const mos6529_interface *get_interface(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == MOS6529);

	return (const mos6529_interface *) device->baseconfig().static_config();
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    mos6529_r - port read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( mos6529_r )
{
	mos6529_t *mos6529 = get_safe_token(device);

	return devcb_call_read8(&mos6529->in_p_func, 0);
}

/*-------------------------------------------------
    mos6529__w - port write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( mos6529_w )
{
	mos6529_t *mos6529 = get_safe_token(device);

	devcb_call_write8(&mos6529->out_p_func, 0, data);
}

/*-------------------------------------------------
    DEVICE_START( mos6529 )
-------------------------------------------------*/

static DEVICE_START( mos6529 )
{
	mos6529_t *mos6529 = get_safe_token(device);
	const mos6529_interface *intf = get_interface(device);

	/* resolve callbacks */
	devcb_resolve_read8(&mos6529->in_p_func, &intf->in_p_func, device);
	devcb_resolve_write8(&mos6529->out_p_func, &intf->out_p_func, device);
}

/*-------------------------------------------------
    DEVICE_GET_INFO( mos6529 )
-------------------------------------------------*/

DEVICE_GET_INFO( mos6529 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(mos6529_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(mos6529);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/* Nothing */								break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "MOS6529");					break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "MOS6500");					break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team");	break;
	}
}

DEFINE_LEGACY_DEVICE(MOS6529, mos6529);
