#include "driver.h"
#include "upd7220.h"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _upd7220_t upd7220_t;
struct _upd7220_t
{
	devcb_resolved_write_line	out_drq_func;
	devcb_resolved_write_line	out_hsync_func;
	devcb_resolved_write_line	out_vsync_func;

	const device_config *screen;	/* screen */
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE upd7220_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);

	return (upd7220_t *)device->token;
}

INLINE const upd7220_interface *get_interface(const device_config *device)
{
	assert(device != NULL);
	assert((device->type == UPD7220));
	return (const upd7220_interface *) device->static_config;
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    upd7220_r - register read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( upd7220_r )
{
	return 0;
}

/*-------------------------------------------------
    upd7220_w - register write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( upd7220_w )
{
}

/*-------------------------------------------------
    upd7220_dack_w - DMA acknowledge w/data
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( upd7220_dack_w )
{
}

/*-------------------------------------------------
    DEVICE_START( upd7220 )
-------------------------------------------------*/

static DEVICE_START( upd7220 )
{
	upd7220_t *upd7220 = get_safe_token(device);
	const upd7220_interface *intf = get_interface(device);

	/* resolve callbacks */
	devcb_resolve_write_line(&upd7220->out_drq_func, &intf->out_drq_func, device);
	devcb_resolve_write_line(&upd7220->out_hsync_func, &intf->out_hsync_func, device);
	devcb_resolve_write_line(&upd7220->out_vsync_func, &intf->out_vsync_func, device);

	/* get the screen device */
	upd7220->screen = devtag_get_device(device->machine, intf->screen_tag);
	assert(upd7220->screen != NULL);
}

/*-------------------------------------------------
    DEVICE_RESET( upd7220 )
-------------------------------------------------*/

static DEVICE_RESET( upd7220 )
{
	upd7220_t *upd7220 = get_safe_token(device);

	devcb_call_write_line(&upd7220->out_drq_func, CLEAR_LINE);
}

/*-------------------------------------------------
    DEVICE_SET_INFO( upd7220 )
-------------------------------------------------*/

static DEVICE_SET_INFO( upd7220 )
{
	switch (state)
	{
		/* no parameters to set */
	}
}

/*-------------------------------------------------
    DEVICE_GET_INFO( upd7220 )
-------------------------------------------------*/

DEVICE_GET_INFO( upd7220 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(upd7220_t);						break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;										break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_SET_INFO:						info->set_info = DEVICE_SET_INFO_NAME(upd7220);		break;
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(upd7220);			break;
		case DEVINFO_FCT_STOP:							/* Nothing */										break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(upd7220);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "NEC uPD7220");						break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "NEC uPD7220");						break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");								break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);							break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");				break;
	}
}
