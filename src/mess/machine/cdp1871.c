/**********************************************************************

    RCA CDP1871 Keyboard Encoder emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "driver.h"
#include "cdp1871.h"

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

typedef struct _cdp1871_t cdp1871_t;
struct _cdp1871_t
{
	const cdp1871_interface *intf;

	int inhibit;					/* scan counter clock inhibit */
	int sense;						/* sense input scan counter */
	int drive;						/* drive output scan counter */
	int modifiers;					/* modifier inputs */
	
	int da;							/* data available flag */
	int next_da;					/* next value of data available flag */
	int rpt;						/* repeat flag */
	int next_rpt;					/* next value of repeat flag */

	/* timers */
	emu_timer *scan_timer;			/* keyboard scan timer */
};

INLINE cdp1871_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);

	return (cdp1871_t *)device->token;
}

static void change_output_lines(const device_config *device)
{
	cdp1871_t *cdp1871 = get_safe_token(device);

	if (cdp1871->next_da != cdp1871->da)
	{
		cdp1871->da = cdp1871->next_da;

		cdp1871->intf->on_da_changed(device, cdp1871->da);
	}

	if (cdp1871->next_rpt != cdp1871->rpt)
	{
		cdp1871->rpt = cdp1871->next_rpt;

		cdp1871->intf->on_rpt_changed(device, cdp1871->rpt);
	}
}

static void clock_scan_counters(const device_config *device)
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

static void detect_keypress(const device_config *device)
{
	cdp1871_t *cdp1871 = get_safe_token(device);
	static const char *const keynames[] = { "D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9", "D10", "D11" };

	if (input_port_read(device->machine, keynames[cdp1871->drive]) == (1 << cdp1871->sense))
	{
		if (!cdp1871->inhibit)
		{
			cdp1871->modifiers = input_port_read(device->machine, "MODIFIERS");

			cdp1871->inhibit = 1;
			cdp1871->next_da = 0;
		}
		else
		{
			cdp1871->next_rpt = 0;
		}
	}
	else
	{
		cdp1871->inhibit = 0;
		cdp1871->next_rpt = 1;
	}
}

static TIMER_CALLBACK( cdp1871_scan_tick )
{
	const device_config *device = ptr;

	change_output_lines(device);
	clock_scan_counters(device);
	detect_keypress(device);
}

/* Keyboard Data */

READ8_DEVICE_HANDLER( cdp1871_data_r )
{
	cdp1871_t *cdp1871 = get_safe_token(device);

	int shift = BIT(cdp1871->modifiers, 0);
	int control = BIT(cdp1871->modifiers, 1);
	int alpha = BIT(cdp1871->modifiers, 2);
	int table = 0;

	if (control) table = 3;	else if (shift)	table = 2; 	else if (alpha) table = 1;

	// reset DA on next TPB

	cdp1871->next_da = 1;

	return CDP1871_KEY_CODES[table][cdp1871->drive][cdp1871->sense];
}

/* Input Ports */

INPUT_PORTS_START( cdp1871 )
	PORT_START("D1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR(' ')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('´')

	PORT_START("D2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH2) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')

	PORT_START("D3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('@') PORT_CHAR('\'')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')

	PORT_START("D4")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')

	PORT_START("D5")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')

	PORT_START("D6")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\') PORT_CHAR('|')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_TILDE) PORT_CHAR('^') PORT_CHAR('~')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("- Delete") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('_')

	PORT_START("D7")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Line Feed") PORT_CODE(KEYCODE_HOME) PORT_CHAR(UCHAR_MAMEKEY(HOME)) PORT_CHAR(10)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Escape") PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Carriage Return") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Delete") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)

	PORT_START("D8")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_KEYBOARD )

	PORT_START("D9")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_KEYBOARD )

	PORT_START("D10")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_KEYBOARD )

	PORT_START("D11")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_KEYBOARD )

	PORT_START("MODIFIERS")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL)) PORT_CHAR(UCHAR_MAMEKEY(RCONTROL))
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Alpha Lock") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))
INPUT_PORTS_END

/* Device Interface */

static DEVICE_START( cdp1871 )
{
	cdp1871_t *cdp1871 = get_safe_token(device);

	/* validate arguments */
	assert(device != NULL);
	assert(device->tag != NULL);
	assert(strlen(device->tag) < 20);

	cdp1871->intf = device->static_config;

	assert(cdp1871->intf != NULL);
	assert(cdp1871->intf->clock > 0);

	/* set initial values */
	cdp1871->next_da = 1;
	cdp1871->next_rpt = 1;
	change_output_lines(device);
	
	/* create the timers */
	cdp1871->scan_timer = timer_alloc(device->machine, cdp1871_scan_tick, (void *)device);
	timer_adjust_periodic(cdp1871->scan_timer, attotime_zero, 0, ATTOTIME_IN_HZ(cdp1871->intf->clock));

	/* register for state saving */
	state_save_register_device_item(device, 0, cdp1871->inhibit);
	state_save_register_device_item(device, 0, cdp1871->sense);
	state_save_register_device_item(device, 0, cdp1871->drive);
	state_save_register_device_item(device, 0, cdp1871->modifiers);

	state_save_register_device_item(device, 0, cdp1871->da);
	state_save_register_device_item(device, 0, cdp1871->next_da);
	state_save_register_device_item(device, 0, cdp1871->rpt);
	state_save_register_device_item(device, 0, cdp1871->next_rpt);
	return DEVICE_START_OK;
}

static DEVICE_SET_INFO( cdp1871 )
{
	switch (state)
	{
		/* no parameters to set */
	}
}

DEVICE_GET_INFO( cdp1871 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(cdp1871_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_SET_INFO:						info->set_info = DEVICE_SET_INFO_NAME(cdp1871); break;
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(cdp1871);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/* Nothing */								break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "RCA CDP1871");					break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "RCA CDP1800");					break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);							break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");			break;
	}
}
