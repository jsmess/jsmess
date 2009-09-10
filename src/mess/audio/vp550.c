/**********************************************************************

	RCA VP550 - VIP Super Sound System emulation

	Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

/*

	TODO:

	- octave (CD22100)
	- amplitude (CD4508)
	- tempo interrupt
	- mono/stereo mode

*/

#include "driver.h"
#include "vp550.h"
#include "cpu/cdp1802/cdp1802.h"
#include "sound/cdp1863.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

#define CDP1863_A_TAG	"u1"
#define CDP1863_B_TAG	"u2"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _vp550_t vp550_t;
struct _vp550_t
{
	int ie;								/* interrupt enable */

	/* devices */
	const device_config *cdp1863_a;
	const device_config *cdp1863_b;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE vp550_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == VP550);
	return (vp550_t *)device->token;
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    vp550_q_w - Q line write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( vp550_q_w )
{
	vp550_t *vp550 = get_safe_token(device);

	cdp1863_oe_w(vp550->cdp1863_a, state);
	cdp1863_oe_w(vp550->cdp1863_b, state);
}

/*-------------------------------------------------
    vp550_octave_w - 
-------------------------------------------------*/

static WRITE8_DEVICE_HANDLER( vp550_octave_w )
{
}

/*-------------------------------------------------
    vp550_vlmna_w - amplitude A write
-------------------------------------------------*/

static WRITE8_DEVICE_HANDLER( vp550_vlmna_w )
{
}

/*-------------------------------------------------
    vp550_vlmnb_w - amplitude B write
-------------------------------------------------*/

static WRITE8_DEVICE_HANDLER( vp550_vlmnb_w )
{
}

/*-------------------------------------------------
    vp550_sync_w - interrupt enable write
-------------------------------------------------*/

static WRITE8_DEVICE_HANDLER( vp550_sync_w )
{
}

/*-------------------------------------------------
    vp550_install_readwrite_handler - install
	or uninstall write handlers
-------------------------------------------------*/

void vp550_install_write_handlers(const device_config *device, const address_space *program, int enabled)
{
	vp550_t *vp550 = get_safe_token(device);

	if (enabled)
	{
		memory_install_write8_device_handler(program, vp550->cdp1863_a, 0x8001, 0x8001, 0, 0, cdp1863_str_w);
		memory_install_write8_device_handler(program, vp550->cdp1863_b, 0x8002, 0x8002, 0, 0, cdp1863_str_w);
		memory_install_write8_device_handler(program, device, 0x8003, 0x8003, 0, 0, vp550_octave_w);
		memory_install_write8_device_handler(program, device, 0x8010, 0x8010, 0, 0, vp550_vlmna_w);
		memory_install_write8_device_handler(program, device, 0x8020, 0x8020, 0, 0, vp550_vlmnb_w);
		memory_install_write8_device_handler(program, device, 0x8030, 0x8030, 0, 0, vp550_sync_w);
	}
	else
	{
		memory_install_write8_handler(program, 0x8001, 0x8001, 0, 0, SMH_UNMAP);
		memory_install_write8_handler(program, 0x8002, 0x8002, 0, 0, SMH_UNMAP);
		memory_install_write8_handler(program, 0x8003, 0x8003, 0, 0, SMH_UNMAP);
		memory_install_write8_handler(program, 0x8010, 0x8010, 0, 0, SMH_UNMAP);
		memory_install_write8_handler(program, 0x8020, 0x8020, 0, 0, SMH_UNMAP);
		memory_install_write8_handler(program, 0x8030, 0x8030, 0, 0, SMH_UNMAP);
	}
}

/*-------------------------------------------------
    MACHINE_DRIVER( vp550 )
-------------------------------------------------*/

static MACHINE_DRIVER_START( vp550 )
	MDRV_CDP1863_ADD(CDP1863_A_TAG, 0, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_CDP1863_ADD(CDP1863_B_TAG, 0, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

/*-------------------------------------------------
    DEVICE_START( vp550 )
-------------------------------------------------*/

static DEVICE_START( vp550 )
{
	vp550_t *vp550 = get_safe_token(device);

	/* look up devices */
	vp550->cdp1863_a = devtag_get_device(device->machine, "vp550:u1");
	vp550->cdp1863_b = devtag_get_device(device->machine, "vp550:u2");
	assert(vp550->cdp1863_a != NULL);
	assert(vp550->cdp1863_b != NULL);

	/* register for state saving */
	state_save_register_global(device->machine, vp550->ie);
}

/*-------------------------------------------------
    DEVICE_RESET( vp550 )
-------------------------------------------------*/

static DEVICE_RESET( vp550 )
{
	vp550_t *vp550 = get_safe_token(device);

	device_reset(vp550->cdp1863_a);
	device_reset(vp550->cdp1863_b);

	cpu_set_input_line(device->machine->firstcpu, CDP1802_INPUT_LINE_INT, CLEAR_LINE);
}

/*-------------------------------------------------
    DEVICE_GET_INFO( vp550 )
-------------------------------------------------*/

DEVICE_GET_INFO( vp550 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(vp550_t);					break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = machine_config_vp550;break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(vp550);		break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(vp550);		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "RCA VP550");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "RCA VIP");					break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						/* Nothing */								break;
	}
}
