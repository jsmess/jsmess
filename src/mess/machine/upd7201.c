/**********************************************************************

    NEC ?PD7201 Multiprotocol Serial Communications Controller emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "driver.h"
#include "upd7201.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _upd7201_t upd7201_t;
struct _upd7201_t
{
	int dummy;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE upd7201_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);

	return (upd7201_t *)device->token;
}

INLINE const upd7201_interface *get_interface(const device_config *device)
{
	assert(device != NULL);
	assert((device->type == UPD7201));
	return (const upd7201_interface *) device->static_config;
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

READ8_DEVICE_HANDLER( upd7201_cd_ba_r )
{
	return 0;
}

WRITE8_DEVICE_HANDLER( upd7201_cd_ba_w )
{
}

READ8_DEVICE_HANDLER( upd7201_ba_cd_r )
{
	return 0;
}

WRITE8_DEVICE_HANDLER( upd7201_ba_cd_w )
{
}

READ8_DEVICE_HANDLER( upd7201_hai_r )
{
	return 0;
}

WRITE8_DEVICE_HANDLER( upd7201_hai_w )
{
}

READ8_DEVICE_HANDLER( upd7201_intak_r )
{
	return 0;
}

WRITE_LINE_DEVICE_HANDLER( upd7201_synca_w )
{
}

WRITE_LINE_DEVICE_HANDLER( upd7201_syncb_w )
{
}

WRITE_LINE_DEVICE_HANDLER( upd7201_rxca_w )
{
}

WRITE_LINE_DEVICE_HANDLER( upd7201_rxcb_w )
{
}

WRITE_LINE_DEVICE_HANDLER( upd7201_txca_w )
{
}

WRITE_LINE_DEVICE_HANDLER( upd7201_txcb_w )
{
}

/*-------------------------------------------------
    DEVICE_START( upd7201 )
-------------------------------------------------*/

static DEVICE_START( upd7201 )
{
//  upd7201_t *upd7201 = get_safe_token(device);
//  const upd7201_interface *intf = get_interface(device);

	/* resolve callbacks */

	/* set initial values */

	/* create the timers */

	/* register for state saving */
//  state_save_register_device_item(device, 0, upd7201->);
}

/*-------------------------------------------------
    DEVICE_GET_INFO( upd7201 )
-------------------------------------------------*/

DEVICE_GET_INFO( upd7201 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(upd7201_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(upd7201);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/* Nothing */								break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "NEC ?PD7201");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "NEC ?PD7201");				break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");		break;
	}
}
