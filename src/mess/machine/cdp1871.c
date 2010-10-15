/**********************************************************************

    RCA CDP1871 Keyboard Encoder emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "emu.h"
#include "cdp1871.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

static const UINT8 CDP1871_KEY_CODES[4][11][8] =
{
	// normal
	{
		{ 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37 },
		{ 0x38, 0x39, 0x3a, 0x3b, 0x2c, 0x2d, 0x2e, 0x2f },
		{ 0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67 },
		{ 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f },
		{ 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77 },
		{ 0x78, 0x79, 0x7a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f },
		{ 0x20, 0xff, 0x0a, 0x1b, 0xff, 0x0d, 0xff, 0x7f },
		{ 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87 },
		{ 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f },
		{ 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97 },
		{ 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f }
	},

	// alpha
	{
		{ 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37 },
		{ 0x38, 0x39, 0x3a, 0x3b, 0x2c, 0x2d, 0x2e, 0x2f },
		{ 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47 },
		{ 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f },
		{ 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57 },
		{ 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f },
		{ 0x20, 0xff, 0x0a, 0x1b, 0xff, 0x0d, 0xff, 0x7f },
		{ 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87 },
		{ 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f },
		{ 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97 },
		{ 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f }
	},

	// shift
	{
		{ 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27 },
		{ 0x28, 0x29, 0x2a, 0x2b, 0x3c, 0x3d, 0x3e, 0x3f },
		{ 0x60, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47 },
		{ 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f },
		{ 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57 },
		{ 0x58, 0x59, 0x5a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f },
		{ 0x20, 0xff, 0x0a, 0x1b, 0xff, 0x0d, 0xff, 0x7f },
		{ 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87 },
		{ 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f },
		{ 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97 },
		{ 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f }
	},

	// control
	{
		{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
		{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
		{ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 },
		{ 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f },
		{ 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17 },
		{ 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f },
		{ 0x20, 0xff, 0x0a, 0x1b, 0xff, 0x0d, 0xff, 0x7f },
		{ 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87 },
		{ 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f },
		{ 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97 },
		{ 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f }
	}
};

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _cdp1871_t cdp1871_t;
struct _cdp1871_t
{
	devcb_resolved_write_line	out_da_func;
	devcb_resolved_write_line	out_rpt_func;
	devcb_resolved_read8		in_d1_func;
	devcb_resolved_read8		in_d2_func;
	devcb_resolved_read8		in_d3_func;
	devcb_resolved_read8		in_d4_func;
	devcb_resolved_read8		in_d5_func;
	devcb_resolved_read8		in_d6_func;
	devcb_resolved_read8		in_d7_func;
	devcb_resolved_read8		in_d8_func;
	devcb_resolved_read8		in_d9_func;
	devcb_resolved_read8		in_d10_func;
	devcb_resolved_read8		in_d11_func;
	devcb_resolved_read_line	in_shift_func;
	devcb_resolved_read_line	in_control_func;
	devcb_resolved_read_line	in_alpha_func;

	int inhibit;					/* scan counter clock inhibit */
	int sense;						/* sense input scan counter */
	int drive;						/* modifier inputs */

	int shift;						/* latched shift modifier */
	int control;					/* latched control modifier */

	int da;							/* data available flag */
	int next_da;					/* next value of data available flag */
	int rpt;						/* repeat flag */
	int next_rpt;					/* next value of repeat flag */

	/* timers */
	emu_timer *scan_timer;			/* keyboard scan timer */
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE cdp1871_t *get_safe_token(running_device *device)
{
	assert(device != NULL);

	return (cdp1871_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE const cdp1871_interface *get_interface(running_device *device)
{
	assert(device != NULL);
	assert((device->type() == CDP1871));
	return (const cdp1871_interface *) device->baseconfig().static_config();
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    change_output_lines - change output line
    state
-------------------------------------------------*/

static void change_output_lines(running_device *device)
{
	cdp1871_t *cdp1871 = get_safe_token(device);

	if (cdp1871->next_da != cdp1871->da)
	{
		cdp1871->da = cdp1871->next_da;

		devcb_call_write_line(&cdp1871->out_da_func, cdp1871->da);
	}

	if (cdp1871->next_rpt != cdp1871->rpt)
	{
		cdp1871->rpt = cdp1871->next_rpt;

		devcb_call_write_line(&cdp1871->out_rpt_func, cdp1871->rpt);
	}
}

/*-------------------------------------------------
    clock_scan_counters - clock the keyboard
    scan counters
-------------------------------------------------*/

static void clock_scan_counters(running_device *device)
{
	cdp1871_t *cdp1871 = get_safe_token(device);

	if (!cdp1871->inhibit)
	{
		cdp1871->sense++;

		if (cdp1871->sense == 8)
		{
			cdp1871->sense = 0;
			cdp1871->drive++;

			if (cdp1871->drive == 11)
			{
				cdp1871->drive = 0;
			}
		}
	}
}

/*-------------------------------------------------
    detect_keypress - detect key press
-------------------------------------------------*/

static void detect_keypress(running_device *device)
{
	cdp1871_t *cdp1871 = get_safe_token(device);

	UINT8 data = 0;

	switch (cdp1871->drive)
	{
	case 0:		data = devcb_call_read8(&cdp1871->in_d1_func, 0);	break;
	case 1:		data = devcb_call_read8(&cdp1871->in_d2_func, 0);	break;
	case 2:		data = devcb_call_read8(&cdp1871->in_d3_func, 0);	break;
	case 3:		data = devcb_call_read8(&cdp1871->in_d4_func, 0);	break;
	case 4:		data = devcb_call_read8(&cdp1871->in_d5_func, 0);	break;
	case 5:		data = devcb_call_read8(&cdp1871->in_d6_func, 0);	break;
	case 6:		data = devcb_call_read8(&cdp1871->in_d7_func, 0);	break;
	case 7:		data = devcb_call_read8(&cdp1871->in_d8_func, 0);	break;
	case 8:		data = devcb_call_read8(&cdp1871->in_d9_func, 0);	break;
	case 9:		data = devcb_call_read8(&cdp1871->in_d10_func, 0);	break;
	case 10:	data = devcb_call_read8(&cdp1871->in_d11_func, 0);	break;
	}

	if (data == (1 << cdp1871->sense))
	{
		if (!cdp1871->inhibit)
		{
			cdp1871->shift = devcb_call_read_line(&cdp1871->in_shift_func);
			cdp1871->control = devcb_call_read_line(&cdp1871->in_control_func);
			cdp1871->inhibit = 1;
			cdp1871->next_da = ASSERT_LINE;
		}
		else
		{
			cdp1871->next_rpt = ASSERT_LINE;
		}
	}
	else
	{
		cdp1871->inhibit = 0;
		cdp1871->next_rpt = CLEAR_LINE;
	}
}

/*-------------------------------------------------
    TIMER_CALLBACK( cdp1871_scan_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( cdp1871_scan_tick )
{
	running_device *device = (running_device *)ptr;

	change_output_lines(device);
	clock_scan_counters(device);
	detect_keypress(device);
}

/*-------------------------------------------------
    cdp1871_data_r - interface retrieving
    keyboard data
-------------------------------------------------*/

READ8_DEVICE_HANDLER( cdp1871_data_r )
{
	cdp1871_t *cdp1871 = get_safe_token(device);

	int table = 0;
	int alpha = devcb_call_read_line(&cdp1871->in_alpha_func);

	if (cdp1871->control) table = 3; else if (cdp1871->shift) table = 2; else if (alpha) table = 1;

	// reset DA on next TPB

	cdp1871->next_da = CLEAR_LINE;

	return CDP1871_KEY_CODES[table][cdp1871->drive][cdp1871->sense];
}

/*-------------------------------------------------
    cdp1871_da_r - keyboard data available
-------------------------------------------------*/

READ_LINE_DEVICE_HANDLER( cdp1871_da_r )
{
	cdp1871_t *cdp1871 = get_safe_token(device);

	return cdp1871->da;
}

/*-------------------------------------------------
    cdp1871_rpt_r - keyboard repeat
-------------------------------------------------*/

READ_LINE_DEVICE_HANDLER( cdp1871_rpt_r )
{
	cdp1871_t *cdp1871 = get_safe_token(device);

	return cdp1871->rpt;
}

/*-------------------------------------------------
    DEVICE_START( cdp1871 )
-------------------------------------------------*/

static DEVICE_START( cdp1871 )
{
	cdp1871_t *cdp1871 = get_safe_token(device);
	const cdp1871_interface *intf = get_interface(device);

	/* resolve callbacks */
	devcb_resolve_write_line(&cdp1871->out_da_func, &intf->out_da_func, device);
	devcb_resolve_write_line(&cdp1871->out_rpt_func, &intf->out_rpt_func, device);
	devcb_resolve_read8(&cdp1871->in_d1_func, &intf->in_d1_func, device);
	devcb_resolve_read8(&cdp1871->in_d2_func, &intf->in_d2_func, device);
	devcb_resolve_read8(&cdp1871->in_d3_func, &intf->in_d3_func, device);
	devcb_resolve_read8(&cdp1871->in_d4_func, &intf->in_d4_func, device);
	devcb_resolve_read8(&cdp1871->in_d5_func, &intf->in_d5_func, device);
	devcb_resolve_read8(&cdp1871->in_d6_func, &intf->in_d6_func, device);
	devcb_resolve_read8(&cdp1871->in_d7_func, &intf->in_d7_func, device);
	devcb_resolve_read8(&cdp1871->in_d8_func, &intf->in_d8_func, device);
	devcb_resolve_read8(&cdp1871->in_d9_func, &intf->in_d9_func, device);
	devcb_resolve_read8(&cdp1871->in_d10_func, &intf->in_d10_func, device);
	devcb_resolve_read8(&cdp1871->in_d11_func, &intf->in_d11_func, device);
	devcb_resolve_read_line(&cdp1871->in_shift_func, &intf->in_shift_func, device);
	devcb_resolve_read_line(&cdp1871->in_control_func, &intf->in_control_func, device);
	devcb_resolve_read_line(&cdp1871->in_alpha_func, &intf->in_alpha_func, device);

	/* set initial values */
	cdp1871->next_da = CLEAR_LINE;
	cdp1871->next_rpt = CLEAR_LINE;
	change_output_lines(device);

	/* create the timers */
	cdp1871->scan_timer = timer_alloc(device->machine, cdp1871_scan_tick, (void *)device);
	timer_adjust_periodic(cdp1871->scan_timer, attotime_zero, 0, ATTOTIME_IN_HZ(device->clock()));

	/* register for state saving */
	state_save_register_device_item(device, 0, cdp1871->inhibit);
	state_save_register_device_item(device, 0, cdp1871->sense);
	state_save_register_device_item(device, 0, cdp1871->drive);
	state_save_register_device_item(device, 0, cdp1871->shift);
	state_save_register_device_item(device, 0, cdp1871->control);
	state_save_register_device_item(device, 0, cdp1871->da);
	state_save_register_device_item(device, 0, cdp1871->next_da);
	state_save_register_device_item(device, 0, cdp1871->rpt);
	state_save_register_device_item(device, 0, cdp1871->next_rpt);
}

/*-------------------------------------------------
    DEVICE_GET_INFO( cdp1871 )
-------------------------------------------------*/

DEVICE_GET_INFO( cdp1871 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(cdp1871_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(cdp1871);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/* Nothing */								break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "RCA CDP1871");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "RCA CDP1800");				break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");		break;
	}
}

DEFINE_LEGACY_DEVICE(CDP1871, cdp1871);
