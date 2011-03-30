/**********************************************************************

    Fairchild MM74C922/MM74C923 16/20-Key Encoder emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

    TODO:

    - update frequency from interface capacitor value
    - debounce from interface capacitor value

*/

#include "emu.h"
#include "mm74c922.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _mm74c922_t mm74c922_t;
struct _mm74c922_t
{
	devcb_resolved_read8		in_x_func[5];
	devcb_resolved_write_line	out_da_func;

	int inhibit;				/* scan counter clock inhibit */
	int x;						/* currently scanned column */
	int y;						/* latched row */
	int max_y;					/* number of rows */

	UINT8 data;					/* data latch */

	int da;						/* data available flag */
	int next_da;				/* next value of data available flag */

	/* timers */
	emu_timer *scan_timer;		/* keyboard scan timer */
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE mm74c922_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert((device->type() == MM74C922) || (device->type() == MM74C923));

	return (mm74c922_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE const mm74c922_interface *get_interface(device_t *device)
{
	assert(device != NULL);
	assert((device->type() == MM74C922) || (device->type() == MM74C923));

	return (const mm74c922_interface *) device->baseconfig().static_config();
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    change_output_lines - change output line
    state
-------------------------------------------------*/

static void change_output_lines(device_t *device)
{
	mm74c922_t *mm74c922 = get_safe_token(device);

	if (mm74c922->next_da != mm74c922->da)
	{
		mm74c922->da = mm74c922->next_da;

		if (LOG) logerror("MM74C922 '%s' Data Available: %u\n", device->tag(), mm74c922->da);

		devcb_call_write_line(&mm74c922->out_da_func, mm74c922->da);
	}
}

/*-------------------------------------------------
    clock_scan_counters - clock the keyboard
    scan counters
-------------------------------------------------*/

static void clock_scan_counters(device_t *device)
{
	mm74c922_t *mm74c922 = get_safe_token(device);

	if (!mm74c922->inhibit)
	{
		mm74c922->x++;
		mm74c922->x &= 0x03;
	}
}

/*-------------------------------------------------
    detect_keypress - detect key press
-------------------------------------------------*/

static void detect_keypress(device_t *device)
{
	mm74c922_t *mm74c922 = get_safe_token(device);

	if (mm74c922->inhibit)
	{
		UINT8 data = devcb_call_read8(&mm74c922->in_x_func[mm74c922->x], 0);

		if (BIT(data, mm74c922->y))
		{
			/* key released */
			mm74c922->inhibit = 0;
			mm74c922->next_da = 0;
			mm74c922->data = 0xff; // high-Z

			if (LOG) logerror("MM74C922 '%s' Key Released\n", device->tag());
		}
	}
	else
	{
		UINT8 data = devcb_call_read8(&mm74c922->in_x_func[mm74c922->x], 0);
		int y;

		for (y = 0; y < mm74c922->max_y; y++)
		{
			if (!BIT(data, y))
			{
				/* key depressed */
				mm74c922->inhibit = 1;
				mm74c922->next_da = 1;
				mm74c922->y = y;

				mm74c922->data = (y << 2) | mm74c922->x;

				if (LOG) logerror("MM74C922 '%s' Key Depressed: X %u Y %u = %02x\n", device->tag(), mm74c922->x, y, mm74c922->data);
				return;
			}
		}
	}
}

/*-------------------------------------------------
    TIMER_CALLBACK( mm74c922_scan_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( mm74c922_scan_tick )
{
	device_t *device = (device_t *)ptr;

	change_output_lines(device);
	clock_scan_counters(device);
	detect_keypress(device);
}

/*-------------------------------------------------
    mm74c922_data_r - interface retrieving
    keyboard data
-------------------------------------------------*/

READ8_DEVICE_HANDLER( mm74c922_r )
{
	mm74c922_t *mm74c922 = get_safe_token(device);

	if (LOG) logerror("MM74C922 '%s' Data Read: %02x\n", device->tag(), mm74c922->data);

	return mm74c922->data;
}

/*-------------------------------------------------
    DEVICE_START( mm74c922 )
-------------------------------------------------*/

static DEVICE_START( mm74c922 )
{
	mm74c922_t *mm74c922 = get_safe_token(device);
	const mm74c922_interface *intf = get_interface(device);

	/* resolve callbacks */
	devcb_resolve_read8(&mm74c922->in_x_func[0], &intf->in_x1_func, device);
	devcb_resolve_read8(&mm74c922->in_x_func[1], &intf->in_x2_func, device);
	devcb_resolve_read8(&mm74c922->in_x_func[2], &intf->in_x3_func, device);
	devcb_resolve_read8(&mm74c922->in_x_func[3], &intf->in_x4_func, device);
	devcb_resolve_read8(&mm74c922->in_x_func[4], &intf->in_x5_func, device);
	devcb_resolve_write_line(&mm74c922->out_da_func, &intf->out_da_func, device);

	/* set initial values */
	mm74c922->next_da = 0;
	change_output_lines(device);

	if (device->type() == MM74C922)
	{
		mm74c922->max_y = 4;
	}
	else
	{
		mm74c922->max_y = 5;
	}

	/* create the timers */
	mm74c922->scan_timer = device->machine().scheduler().timer_alloc(FUNC(mm74c922_scan_tick), (void *)device);
	mm74c922->scan_timer->adjust(attotime::zero, 0, attotime::from_hz(50));

	/* register for state saving */
	device->save_item(NAME(mm74c922->inhibit));
	device->save_item(NAME(mm74c922->x));
	device->save_item(NAME(mm74c922->y));
	device->save_item(NAME(mm74c922->max_y));
	device->save_item(NAME(mm74c922->data));
	device->save_item(NAME(mm74c922->da));
	device->save_item(NAME(mm74c922->next_da));
}

/*-------------------------------------------------
    DEVICE_GET_INFO( mm74c922 )
-------------------------------------------------*/

DEVICE_GET_INFO( mm74c922 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(mm74c922_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(mm74c922);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/* Nothing */								break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Fairchild MM74C922");		break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Fairchild MM74C922");		break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");		break;
	}
}

/*-------------------------------------------------
    DEVICE_GET_INFO( mm74c923 )
-------------------------------------------------*/

DEVICE_GET_INFO( mm74c923 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(mm74c922_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(mm74c922);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/* Nothing */								break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Fairchild MM74C923");		break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Fairchild MM74C922");		break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");		break;
	}
}

DEFINE_LEGACY_DEVICE(MM74C922, mm74c922);
DEFINE_LEGACY_DEVICE(MM74C923, mm74c923);
