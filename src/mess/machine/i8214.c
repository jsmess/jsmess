/**********************************************************************

    Intel 8214 Priority Interrupt Controller emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "emu.h"
#include "i8214.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _i8214_t i8214_t;
struct _i8214_t
{
	devcb_resolved_write_line	out_int_func;
	devcb_resolved_write_line	out_enlg_func;

	int inte;					/* interrupt enable */
	int int_dis;				/* interrupt disable flip-flop */
	int a;						/* request level */
	int b;						/* current status register */
	UINT8 r;					/* interrupt request latch */
	int sgs;					/* status group select */
	int etlg;					/* enable this level group */
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE i8214_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == I8214);

	return (i8214_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE const i8214_interface *get_interface(device_t *device)
{
	assert(device != NULL);
	assert((device->type() == I8214));

	return (const i8214_interface *) device->baseconfig().static_config();
}

INLINE void trigger_interrupt(device_t *device, int level)
{
	i8214_t *i8214 = get_safe_token(device);

	if (LOG) logerror("I8214 '%s' Interrupt Level %u\n", device->tag(), level);

	i8214->a = level;

	/* disable interrupts */
	i8214->int_dis = 1;

	/* disable next level group */
	devcb_call_write_line(&i8214->out_enlg_func, 0);

	/* toggle interrupt line */
	devcb_call_write_line(&i8214->out_int_func, ASSERT_LINE);
	devcb_call_write_line(&i8214->out_int_func, CLEAR_LINE);
}

INLINE void check_interrupt(device_t *device)
{
	i8214_t *i8214 = get_safe_token(device);
	int level;

	if (i8214->int_dis || !i8214->etlg) return;

	for (level = 7; level >= 0; level--)
	{
		if (!BIT(i8214->r, 7 - level))
		{
			if (i8214->sgs)
			{
				if (level > i8214->b)
				{
					trigger_interrupt(device, level);
				}
			}
			else
			{
				trigger_interrupt(device, level);
			}
		}
	}
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    i8214_a_r - request level read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( i8214_a_r )
{
	i8214_t *i8214 = get_safe_token(device);

	UINT8 a = i8214->a & 0x07;

	if (LOG) logerror("I8214 '%s' A: %01x\n", device->tag(), a);

	return a;
}

/*-------------------------------------------------
    i8214_b_w - current status register write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( i8214_b_w )
{
	i8214_t *i8214 = get_safe_token(device);

	i8214->b = data & 0x07;
	i8214->sgs = BIT(data, 3);

	if (LOG) logerror("I8214 '%s' B: %01x\n", device->tag(), i8214->b);
	if (LOG) logerror("I8214 '%s' SGS: %u\n", device->tag(), i8214->sgs);

	/* enable interrupts */
	i8214->int_dis = 0;

	/* enable next level group */
	devcb_call_write_line(&i8214->out_enlg_func, 1);

	check_interrupt(device);
}

/*-------------------------------------------------
    i8214_etlg_w - enable this level group write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( i8214_etlg_w )
{
	i8214_t *i8214 = get_safe_token(device);

	if (LOG) logerror("I8214 '%s' ETLG: %u\n", device->tag(), state);

	i8214->etlg = state;
}

/*-------------------------------------------------
    i8214_inte_w - interrupt enable write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( i8214_inte_w )
{
	i8214_t *i8214 = get_safe_token(device);

	if (LOG) logerror("I8214 '%s' INTE: %u\n", device->tag(), state);

	i8214->inte = state;
}

/*-------------------------------------------------
    i8214_r_w - request level write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( i8214_r_w )
{
	i8214_t *i8214 = get_safe_token(device);

	if (LOG) logerror("I8214 '%s' R: %02x\n", device->tag(), data);

	i8214->r = data;

	check_interrupt(device);
}

/*-------------------------------------------------
    DEVICE_START( i8214 )
-------------------------------------------------*/

static DEVICE_START( i8214 )
{
	i8214_t *i8214 = get_safe_token(device);
	const i8214_interface *intf = get_interface(device);

	/* resolve callbacks */
	devcb_resolve_write_line(&i8214->out_int_func, &intf->out_int_func, device);
	devcb_resolve_write_line(&i8214->out_enlg_func, &intf->out_enlg_func, device);

	/* register for state saving */
	device->save_item(NAME(i8214->inte));
	device->save_item(NAME(i8214->int_dis));
	device->save_item(NAME(i8214->a));
	device->save_item(NAME(i8214->b));
	device->save_item(NAME(i8214->r));
	device->save_item(NAME(i8214->sgs));
	device->save_item(NAME(i8214->etlg));
}

/*-------------------------------------------------
    DEVICE_GET_INFO( i8214 )
-------------------------------------------------*/

DEVICE_GET_INFO( i8214 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(i8214_t);					break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(i8214);		break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/* Nothing */								break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Intel 8214");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Intel MCS-80");			break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");		break;
	}
}

DEFINE_LEGACY_DEVICE(I8214, i8214);
