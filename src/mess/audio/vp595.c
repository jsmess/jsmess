/**********************************************************************

    RCA VP595 - VIP Simple Sound System emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

#include "emu.h"
#include "vp595.h"
#include "sound/cdp1863.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define CDP1863_TAG		"u1"
#define CDP1863_XTAL	440560

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _vp595_t vp595_t;
struct _vp595_t
{
	/* devices */
	device_t *cdp1863;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE vp595_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == VP595);
	return (vp595_t *)downcast<legacy_device_base *>(device)->token();
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
    vp595_cdp1863_w - CDP1863 latch write
-------------------------------------------------*/

static WRITE8_DEVICE_HANDLER( vp595_cdp1863_w )
{
	/* inject 0x80 instead of 0x00 */
	if (data == 0) data = 0x80;

	cdp1863_str_w(device, 0, data);
}

/*-------------------------------------------------
    vp595_install_readwrite_handler - install
    or uninstall write handlers
-------------------------------------------------*/

void vp595_install_write_handlers(device_t *device, address_space *io, int enabled)
{
	vp595_t *vp595 = get_safe_token(device);

	if (enabled)
	{
		io->install_legacy_write_handler(*vp595->cdp1863, 0x03, 0x03, FUNC(vp595_cdp1863_w));
	}
	else
	{
		io->unmap_write(0x03, 0x03);
	}
}

/*-------------------------------------------------
    MACHINE_DRIVER( vp595 )
-------------------------------------------------*/

static MACHINE_CONFIG_FRAGMENT( vp595 )
	MCFG_CDP1863_ADD(CDP1863_TAG, 0, 0) // TODO: clock2 should be CDP1863_XTAL
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_CONFIG_END

/*-------------------------------------------------
    DEVICE_START( vp595 )
-------------------------------------------------*/

static DEVICE_START( vp595 )
{
	vp595_t *vp595 = get_safe_token(device);

	/* look up devices */
	vp595->cdp1863 = device->subdevice(CDP1863_TAG);

	/* HACK workaround */
	cdp1863_set_clk2(vp595->cdp1863, CDP1863_XTAL);
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

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME( vp595 );	break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(vp595);		break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/* Nothing */								break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "RCA VP595");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "RCA VIP");					break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						/* Nothing */								break;
	}
}

DEFINE_LEGACY_DEVICE(VP595, vp595);
