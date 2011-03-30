/**********************************************************************

    RCA VP550 - VIP Super Sound System emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

/*

    TODO:

    - tempo control
    - mono/stereo mode
    - VP551 memory map

*/

#include "emu.h"
#include "vp550.h"
#include "cpu/cosmac/cosmac.h"
#include "sound/cdp1863.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

#define MAX_CHANNELS	4

#define CDP1863_A_TAG	"u1"
#define CDP1863_B_TAG	"u2"
#define CDP1863_C_TAG	"cdp1863c"
#define CDP1863_D_TAG	"cdp1863d"

enum
{
	CHANNEL_A = 0,
	CHANNEL_B,
	CHANNEL_C,
	CHANNEL_D
};

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _vp550_t vp550_t;
struct _vp550_t
{
	int channels;						/* number of channels */

	/* devices */
	device_t *cdp1863[MAX_CHANNELS];
	timer_device *sync_timer;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE vp550_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert((device->type() == VP550) || (device->type() == VP551));
	return (vp550_t *)downcast<legacy_device_base *>(device)->token();
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

	int channel;

	for (channel = CHANNEL_A; channel < vp550->channels; channel++)
	{
		cdp1863_oe_w(vp550->cdp1863[channel], state);
	}
}

/*-------------------------------------------------
    vp550_sc1_w - SC1 line write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( vp550_sc1_w )
{
	if (state)
	{
		device_set_input_line(device, COSMAC_INPUT_LINE_INT, CLEAR_LINE);

		if (LOG) logerror("VP550 Clear Interrupt\n");
	}
}

/*-------------------------------------------------
    vp550_octave_w - octave select write
-------------------------------------------------*/

static WRITE8_DEVICE_HANDLER( vp550_octave_w )
{
	vp550_t *vp550 = get_safe_token(device);

	int channel = (data >> 2) & 0x03;
	int clock = 0;

	if (data & 0x10)
	{
		switch (data & 0x03)
		{
		case 0: clock = device->clock() / 8; break;
		case 1: clock = device->clock() / 4; break;
		case 2: clock = device->clock() / 2; break;
		case 3: clock = device->clock();	   break;
		}
	}

	if (vp550->cdp1863[channel]) cdp1863_set_clk2(vp550->cdp1863[channel], clock);

	if (LOG) logerror("VP550 Clock %c: %u Hz\n", 'A' + channel, clock);
}

/*-------------------------------------------------
    vp550_vlmn_w - amplitude write
-------------------------------------------------*/

static WRITE8_DEVICE_HANDLER( vp550_vlmn_w )
{
	//float gain = 0.625f * (data & 0x0f);

//  sound_set_output_gain(device, 0, gain);

	if (LOG) logerror("VP550 '%s' Volume: %u\n", device->tag(), data & 0x0f);
}

/*-------------------------------------------------
    vp550_sync_w - interrupt enable write
-------------------------------------------------*/

static WRITE8_DEVICE_HANDLER( vp550_sync_w )
{
	downcast<timer_device *>(device)->enable(BIT(data, 0));

	if (LOG) logerror("VP550 Interrupt Enable: %u\n", BIT(data, 0));
}

/*-------------------------------------------------
    vp550_install_readwrite_handler - install
    or uninstall write handlers
-------------------------------------------------*/

void vp550_install_write_handlers(device_t *device, address_space *program, int enabled)
{
	vp550_t *vp550 = get_safe_token(device);

	if (enabled)
	{
		program->install_legacy_write_handler(*vp550->cdp1863[CHANNEL_A], 0x8001, 0x8001, FUNC(cdp1863_str_w));
		program->install_legacy_write_handler(*vp550->cdp1863[CHANNEL_B], 0x8002, 0x8002, FUNC(cdp1863_str_w));
		program->install_legacy_write_handler(*device, 0x8003, 0x8003, FUNC(vp550_octave_w));
		program->install_legacy_write_handler(*vp550->cdp1863[CHANNEL_A], 0x8010, 0x8010, FUNC(vp550_vlmn_w));
		program->install_legacy_write_handler(*vp550->cdp1863[CHANNEL_B], 0x8020, 0x8020, FUNC(vp550_vlmn_w));
		program->install_legacy_write_handler(*vp550->sync_timer, 0x8030, 0x8030, FUNC(vp550_sync_w));
	}
	else
	{
		program->unmap_write(0x8001, 0x8001);
		program->unmap_write(0x8002, 0x8002);
		program->unmap_write(0x8003, 0x8003);
		program->unmap_write(0x8010, 0x8010);
		program->unmap_write(0x8020, 0x8020);
		program->unmap_write(0x8030, 0x8030);
	}
}

/*-------------------------------------------------
    vp551_install_readwrite_handler - install
    or uninstall write handlers
-------------------------------------------------*/

void vp551_install_write_handlers(device_t *device, address_space *program, int enabled)
{
}

/*-------------------------------------------------
    TIMER_DEVICE_CALLBACK( sync_tick )
-------------------------------------------------*/

static TIMER_DEVICE_CALLBACK( sync_tick )
{
	device_set_input_line(timer.machine().firstcpu, COSMAC_INPUT_LINE_INT, ASSERT_LINE);

	if (LOG) logerror("VP550 Interrupt\n");
}

/*-------------------------------------------------
    MACHINE_DRIVER( vp550 )
-------------------------------------------------*/

static MACHINE_CONFIG_FRAGMENT( vp550 )
	MCFG_TIMER_ADD_PERIODIC("sync", sync_tick, attotime::from_hz(50))

	MCFG_CDP1863_ADD(CDP1863_A_TAG, 0, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MCFG_CDP1863_ADD(CDP1863_B_TAG, 0, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_CONFIG_END

/*-------------------------------------------------
    MACHINE_DRIVER( vp551 )
-------------------------------------------------*/

static MACHINE_CONFIG_FRAGMENT( vp551 )
	MCFG_FRAGMENT_ADD(vp550)

	MCFG_CDP1863_ADD(CDP1863_C_TAG, 0, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MCFG_CDP1863_ADD(CDP1863_D_TAG, 0, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_CONFIG_END

/*-------------------------------------------------
    DEVICE_START( vp550 )
-------------------------------------------------*/

static DEVICE_START( vp550 )
{
	vp550_t *vp550 = get_safe_token(device);

	/* look up devices */
	vp550->cdp1863[CHANNEL_A] = device->machine().device("vp550:u1");
	vp550->cdp1863[CHANNEL_B] = device->machine().device("vp550:u2");
	vp550->sync_timer = device->machine().device<timer_device>("vp550:sync");

	/* set initial values */
	vp550->channels = 2;
}

/*-------------------------------------------------
    DEVICE_START( vp551 )
-------------------------------------------------*/

static DEVICE_START( vp551 )
{
	vp550_t *vp550 = get_safe_token(device);

	/* look up devices */
	vp550->cdp1863[CHANNEL_A] = device->subdevice(CDP1863_A_TAG);
	vp550->cdp1863[CHANNEL_B] = device->subdevice(CDP1863_B_TAG);
	vp550->cdp1863[CHANNEL_C] = device->subdevice(CDP1863_C_TAG);
	vp550->cdp1863[CHANNEL_D] = device->subdevice(CDP1863_D_TAG);
	vp550->sync_timer = downcast<timer_device *>(device->subdevice("sync"));

	/* set initial values */
	vp550->channels = 4;
}

/*-------------------------------------------------
    DEVICE_RESET( vp550 )
-------------------------------------------------*/

static DEVICE_RESET( vp550 )
{
	vp550_t *vp550 = get_safe_token(device);

	/* reset chips */
	vp550->cdp1863[CHANNEL_A]->reset();
	vp550->cdp1863[CHANNEL_B]->reset();

	/* disable interrupt timer */
	vp550->sync_timer->enable(0);

	/* clear interrupt */
	device_set_input_line(device->machine().firstcpu, COSMAC_INPUT_LINE_INT, CLEAR_LINE);
}

/*-------------------------------------------------
    DEVICE_RESET( vp551 )
-------------------------------------------------*/

static DEVICE_RESET( vp551 )
{
	vp550_t *vp550 = get_safe_token(device);

	DEVICE_RESET_CALL(vp550);

	/* reset chips */
	vp550->cdp1863[CHANNEL_C]->reset();
	vp550->cdp1863[CHANNEL_D]->reset();
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

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME( vp550 );	break;

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

/*-------------------------------------------------
    DEVICE_GET_INFO( vp551 )
-------------------------------------------------*/

DEVICE_GET_INFO( vp551 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(vp550_t);					break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME( vp551 );	break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(vp551);		break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(vp551);		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "RCA VP551");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "RCA VIP");					break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						/* Nothing */								break;
	}
}

DEFINE_LEGACY_DEVICE(VP550, vp550);
DEFINE_LEGACY_DEVICE(VP551, vp551);
