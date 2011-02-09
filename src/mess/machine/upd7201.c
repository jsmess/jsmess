/**********************************************************************

    NEC ??PD7201 Multiprotocol Serial Communications Controller

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "emu.h"
#include "upd7201.h"


/***************************************************************************
    PARAMETERS
***************************************************************************/


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _upd7201_state upd7201_state;
struct _upd7201_state
{
	devcb_resolved_write_line out_int_func;
};


/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE upd7201_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == UPD7201);

	return (upd7201_state *)downcast<legacy_device_base *>(device)->token();
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

WRITE_LINE_DEVICE_HANDLER( upd7201_ctsa_w )
{

}

WRITE_LINE_DEVICE_HANDLER( upd7201_ctsb_w )
{

}

/* data terminal ready a, hold acknowledge output */
READ_LINE_DEVICE_HANDLER( upd7201_dtra_r )
{
	return 0;
}

/* data terminal ready b */
READ_LINE_DEVICE_HANDLER( upd7201_dtrb_r )
{
	return 0;
}

/* hold acknowledge input (same pin as dtrb) */
WRITE_LINE_DEVICE_HANDLER( upd7201_hai_w )
{

}

WRITE_LINE_DEVICE_HANDLER( upd7201_rxda_w )
{

}

READ_LINE_DEVICE_HANDLER( upd7201_txda_r )
{
	return 0;
}

WRITE_LINE_DEVICE_HANDLER( upd7201_rxdb_w )
{

}

READ_LINE_DEVICE_HANDLER( upd7201_txdb_r )
{
	return 0;
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


/*****************************************************************************
    DEVICE INTERFACE
*****************************************************************************/

static DEVICE_START( upd7201 )
{
	upd7201_state *upd7201 = get_safe_token(device);
	const upd7201_interface *intf = (const upd7201_interface *)device->baseconfig().static_config();

	assert(device->baseconfig().static_config() != NULL);

	/* resolve callbacks */
	devcb_resolve_write_line(&upd7201->out_int_func, &intf->out_int_func, device);

	/* create the timers */

	/* register for state saving */
//  device->save_item(NAME(upd7201->));
}

static DEVICE_RESET( upd7201 )
{
	/* set initial values */
}

DEVICE_GET_INFO( upd7201 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(upd7201_state);			break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(upd7201);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->start = DEVICE_RESET_NAME(upd7201);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "NEC ?PD7201");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "NEC ?PD7201");				break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");		break;
	}
}

DEFINE_LEGACY_DEVICE(UPD7201, upd7201);
