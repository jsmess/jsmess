/**********************************************************************

	RCA VP595 - VIP Super Sound System emulation

	Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

/*

	TODO:

	- CDP1863 frequency?

*/

#include "driver.h"
#include "vp595.h"
#include "cpu/cdp1802/cdp1802.h"
#include "sound/cdp1863.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

#define CDP1863_TAG		"cdp1863"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _vp595_t vp595_t;
struct _vp595_t
{
	/* devices */
	const device_config *cdp1863;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE vp595_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == VP595);
	return (vp595_t *)device->token;
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    vp595_q_w - Q line write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( vp595_q_w )
{
	vp595_t *vp595 = get_safe_token(device);

	cdp1863_oe_w(vp595->cdp1863, state);
}

/*-------------------------------------------------
    vp595_install_readwrite_handler - install
	or uninstall write handlers
-------------------------------------------------*/

void vp595_install_write_handlers(const device_config *device, const address_space *io, int enabled)
{
	vp595_t *vp595 = get_safe_token(device);

	if (enabled)
	{
		memory_install_write8_device_handler(io, vp595->cdp1863, 0x03, 0x03, 0, 0, cdp1863_str_w);
	}
	else
	{
		memory_install_write8_handler(io, 0x03, 0x03, 0, 0, SMH_UNMAP);
	}
}

/*-------------------------------------------------
    MACHINE_DRIVER( vp595 )
-------------------------------------------------*/

static MACHINE_DRIVER_START( vp595 )
	MDRV_CDP1863_ADD(CDP1863_TAG, XTAL_3_52128MHz/2, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

/*-------------------------------------------------
    DEVICE_START( vp595 )
-------------------------------------------------*/

static DEVICE_START( vp595 )
{
	vp595_t *vp595 = get_safe_token(device);

	/* look up devices */
	vp595->cdp1863 = devtag_get_device(device->machine, "vp595:cdp1863");
	assert(vp595->cdp1863 != NULL);

	/* register for state saving */
//	state_save_register_global(device->machine, vp595->);
}

/*-------------------------------------------------
    DEVICE_RESET( vp595 )
-------------------------------------------------*/

static DEVICE_RESET( vp595 )
{
	vp595_t *vp595 = get_safe_token(device);

	device_reset(vp595->cdp1863);
}

/*-------------------------------------------------
    DEVICE_GET_INFO( vp595 )
-------------------------------------------------*/

DEVICE_GET_INFO( vp595 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(vp595_t);					break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = machine_config_vp595;break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(vp595);		break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(vp595);		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "RCA VP595");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "RCA VIP");					break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						/* Nothing */								break;
	}
}
