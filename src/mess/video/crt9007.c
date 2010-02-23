/**********************************************************************

    SMC CRT9007 CRT Video Processor and Controller VPAC emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "emu.h"
#include "crt9007.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 1

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _crt9007_t crt9007_t;
struct _crt9007_t
{
	devcb_resolved_read8		in_vd_func;
	devcb_resolved_write_line	out_int_func;
	devcb_resolved_write_line	out_hs_func;
	devcb_resolved_write_line	out_vs_func;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE crt9007_t *get_safe_token(running_device *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	return (crt9007_t *)device->token;
}

INLINE const crt9007_interface *get_interface(running_device *device)
{
	assert(device != NULL);
	assert(device->type == CRT9007);
	return (const crt9007_interface *) device->baseconfig().static_config;
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/
/*-------------------------------------------------
    crt9007_r - register read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( crt9007_r )
{
//	crt9007_t *crt9007 = get_safe_token(device);

	UINT8 data = 0;

	return data;
}

/*-------------------------------------------------
    crt9007_w - register write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( crt9007_w )
{
//	crt9007_t *crt9007 = get_safe_token(device);
}

/*-------------------------------------------------
    crt9007_update - update screen
-------------------------------------------------*/

void crt9007_update(running_device *device, bitmap_t *bitmap, const rectangle *cliprect)
{
}

/*-------------------------------------------------
    DEVICE_START( crt9007 )
-------------------------------------------------*/

static DEVICE_START( crt9007 )
{
	crt9007_t *crt9007 = (crt9007_t *)device->token;
	const crt9007_interface *intf = get_interface(device);

	/* resolve callbacks */
	devcb_resolve_read8(&crt9007->in_vd_func, &intf->in_vd_func, device);
	devcb_resolve_write_line(&crt9007->out_int_func, &intf->out_int_func, device);
	devcb_resolve_write_line(&crt9007->out_hs_func, &intf->out_hs_func, device);
	devcb_resolve_write_line(&crt9007->out_vs_func, &intf->out_vs_func, device);

	/* register for state saving */
//	state_save_register_device_item(device, 0, crt9007->);
}

/*-------------------------------------------------
    DEVICE_RESET( crt9007 )
-------------------------------------------------*/

static DEVICE_RESET( crt9007 )
{
//	crt9007_t *crt9007 = (crt9007_t *)device->token;
}

/*-------------------------------------------------
    DEVICE_GET_INFO( crt9007 )
-------------------------------------------------*/

DEVICE_GET_INFO( crt9007 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(crt9007_t);				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(crt9007);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(crt9007);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "SMC CRT9007");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "SMC CRT9007");				break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team");	break;
	}
}
