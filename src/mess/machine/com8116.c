/**********************************************************************

    COM8116 - Dual Baud Rate Generator (Programmable Divider) emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

#include "emu.h"
#include "com8116.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _com8116_t com8116_t;
struct _com8116_t
{
	devcb_resolved_write_line	out_fx4_func;
	devcb_resolved_write_line	out_fr_func;
	devcb_resolved_write_line	out_ft_func;

	UINT32 fr_divisors[16];		/* receiver divisor ROM */
	UINT32 ft_divisors[16];		/* transmitter divisor ROM */

	int fr;						/* receiver frequency */
	int ft;						/* transmitter frequency */

	/* timers */
	emu_timer *fx4_timer;
	emu_timer *fr_timer;
	emu_timer *ft_timer;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE com8116_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == COM8116);
	return (com8116_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE const com8116_interface *get_interface(device_t *device)
{
	assert(device != NULL);
	assert((device->type() == COM8116));
	return (const com8116_interface *) device->baseconfig().static_config();
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    com8116_str_w - receiver strobe write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( com8116_str_w )
{
	com8116_t *com8116 = get_safe_token(device);

	if (LOG) logerror("COM8116 '%s' Receiver Divider %01x\n", device->tag(), data & 0x0f);

	com8116->fr = data & 0x0f;

	com8116->fr_timer->adjust(attotime::zero, 0, attotime::from_hz(device->clock() / com8116->fr_divisors[com8116->fr] / 2));
}

/*-------------------------------------------------
    com8116_stt_w - transmitter strobe write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( com8116_stt_w )
{
	com8116_t *com8116 = get_safe_token(device);

	if (LOG) logerror("COM8116 '%s' Transmitter Divider %01x\n", device->tag(), data & 0x0f);

	com8116->ft = data & 0x0f;

	com8116->ft_timer->adjust(attotime::zero, 0, attotime::from_hz(device->clock() / com8116->ft_divisors[com8116->fr] / 2));
}

/*-------------------------------------------------
    TIMER_CALLBACK( fx4_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( fx4_tick )
{
	device_t *device = (device_t *)ptr;
	com8116_t *com8116 = get_safe_token(device);

	devcb_call_write_line(&com8116->out_fx4_func, 1);
}

/*-------------------------------------------------
    TIMER_CALLBACK( fr_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( fr_tick )
{
	device_t *device = (device_t *)ptr;
	com8116_t *com8116 = get_safe_token(device);

	devcb_call_write_line(&com8116->out_fr_func, 1);
}

/*-------------------------------------------------
    TIMER_CALLBACK( ft_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( ft_tick )
{
	device_t *device = (device_t *)ptr;
	com8116_t *com8116 = get_safe_token(device);

	devcb_call_write_line(&com8116->out_ft_func, 1);
}

/*-------------------------------------------------
    DEVICE_START( com8116 )
-------------------------------------------------*/

static DEVICE_START( com8116 )
{
	com8116_t *com8116 = get_safe_token(device);
	const com8116_interface *intf = get_interface(device);
	int i;

	/* resolve callbacks */
	devcb_resolve_write_line(&com8116->out_fx4_func, &intf->out_fx4_func, device);
	devcb_resolve_write_line(&com8116->out_fr_func, &intf->out_fr_func, device);
	devcb_resolve_write_line(&com8116->out_ft_func, &intf->out_ft_func, device);

	for (i = 0; i < 16; i++)
	{
		assert(intf->fr_divisors[i] != 0);
		assert(intf->ft_divisors[i] != 0);

		com8116->fr_divisors[i] = intf->fr_divisors[i];
		com8116->ft_divisors[i] = intf->ft_divisors[i];
	}

	/* create the timers */
	if (com8116->out_fx4_func.target)
	{
		com8116->fx4_timer = device->machine().scheduler().timer_alloc(FUNC(fx4_tick), (void *)device);
		com8116->fx4_timer->adjust(attotime::zero, 0, attotime::from_hz(device->clock() / 4));
	}

	com8116->fr_timer = device->machine().scheduler().timer_alloc(FUNC(fr_tick), (void *)device);
	com8116->ft_timer = device->machine().scheduler().timer_alloc(FUNC(ft_tick), (void *)device);

	/* register for state saving */
    state_save_register_global(device->machine(), com8116->fr);
    state_save_register_global(device->machine(), com8116->ft);
}

/*-------------------------------------------------
    DEVICE_GET_INFO( com8116 )
-------------------------------------------------*/

DEVICE_GET_INFO( com8116 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(com8116_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(com8116);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/* Nothing */								break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "COM8116");					break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "COM8116");					break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						/* Nothing */								break;
	}
}

DEFINE_LEGACY_DEVICE(COM8116, com8116);
