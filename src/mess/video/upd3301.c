/**********************************************************************

    NEC uPD3301 Programmable CRT Controller emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "driver.h"
#include "upd3301.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _upd3301_t upd3301_t;
struct _upd3301_t
{
	devcb_resolved_write_line	out_int_func;
	devcb_resolved_write_line	out_drq_func;
	devcb_resolved_write_line	out_hrtc_func;
	devcb_resolved_write_line	out_vrtc_func;

	const device_config *screen;	/* screen */

	/* timers */
	emu_timer *vrtc_timer;			/* vertical sync timer */
	emu_timer *hrtc_timer;			/* horizontal sync timer */
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE upd3301_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert((device->type == UPD3301));
	return (upd3301_t *)device->token;
}

INLINE const upd3301_interface *get_interface(const device_config *device)
{
	assert(device != NULL);
	assert((device->type == UPD3301));
	return (const upd3301_interface *) device->static_config;
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    TIMER_CALLBACK( vrtc_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( vrtc_tick )
{
//	const device_config *device = ptr;
//	upd3301_t *upd3301 = get_safe_token(device);
}

/*-------------------------------------------------
    TIMER_CALLBACK( hrtc_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( hrtc_tick )
{
//	const device_config *device = ptr;
//	upd3301_t *upd3301 = get_safe_token(device);
}

/*-------------------------------------------------
    upd3301_r - register read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( upd3301_r )
{
	return 0;
}

/*-------------------------------------------------
    upd3301_w - register write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( upd3301_w )
{
}

/*-------------------------------------------------
    upd3301_lpen_w - light pen write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( upd3301_lpen_w )
{
}

/*-------------------------------------------------
    upd3301_dack_w - DMA acknowledge write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( upd3301_dack_w )
{
}

/*-------------------------------------------------
    upd3301_update - screen update
-------------------------------------------------*/

void upd3301_update(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect)
{
}

/*-------------------------------------------------
    DEVICE_START( upd3301 )
-------------------------------------------------*/

static DEVICE_START( upd3301 )
{
	upd3301_t *upd3301 = get_safe_token(device);
	const upd3301_interface *intf = get_interface(device);

	/* resolve callbacks */
	devcb_resolve_write_line(&upd3301->out_int_func, &intf->out_int_func, device);
	devcb_resolve_write_line(&upd3301->out_drq_func, &intf->out_drq_func, device);
	devcb_resolve_write_line(&upd3301->out_hrtc_func, &intf->out_hrtc_func, device);
	devcb_resolve_write_line(&upd3301->out_vrtc_func, &intf->out_vrtc_func, device);

	/* get the screen device */
	upd3301->screen = devtag_get_device(device->machine, intf->screen_tag);
	assert(upd3301->screen != NULL);

	/* create the timers */
	upd3301->vrtc_timer = timer_alloc(device->machine, vrtc_tick, (void *)device);
	upd3301->hrtc_timer = timer_alloc(device->machine, hrtc_tick, (void *)device);

	/* register for state saving */
//	state_save_register_device_item(device, 0, upd3301->);
}

/*-------------------------------------------------
    DEVICE_RESET( upd3301 )
-------------------------------------------------*/

static DEVICE_RESET( upd3301 )
{
}

/*-------------------------------------------------
    DEVICE_GET_INFO( upd3301 )
-------------------------------------------------*/

DEVICE_GET_INFO( upd3301 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(upd3301_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(upd3301);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(upd3301);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "NEC uPD3301");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "NEC uPD3301");				break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");		break;
	}
}
