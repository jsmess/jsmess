/**********************************************************************

    Luxor ABC-bus emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "emu.h"
#include "abcbus.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _abcbus_daisy_state abcbus_daisy_state;
struct _abcbus_daisy_state
{
	abcbus_daisy_state			*next;			/* next device */
	running_device				*device;		/* associated device */

	devcb_resolved_write8		out_cs_func;

	devcb_resolved_read8		in_stat_func;

	devcb_resolved_read8		in_inp_func;
	devcb_resolved_write8		out_utp_func;

	devcb_resolved_write8		out_c_func[4];

	devcb_resolved_write_line	out_rst_func;
};

typedef struct _abcbus_t abcbus_t;
struct _abcbus_t
{
	abcbus_daisy_state *daisy_state;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE abcbus_t *get_safe_token(running_device *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == ABCBUS);
	return (abcbus_t *)device->token;
}

INLINE const abcbus_daisy_chain *get_interface(running_device *device)
{
	assert(device != NULL);
	assert(device->type == ABCBUS);
	return (const abcbus_daisy_chain *) device->baseconfig().static_config;
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

WRITE8_DEVICE_HANDLER( abcbus_cs_w )
{
	abcbus_t *abcbus = get_safe_token(device);
	abcbus_daisy_state *daisy = abcbus->daisy_state;

	if (LOG) logerror("ABCBUS: '%s' CS %02x\n", device->tag(), data);

	for ( ; daisy != NULL; daisy = daisy->next)
	{
		devcb_call_write8(&daisy->out_cs_func, 0, data);
	}
}

READ8_DEVICE_HANDLER( abcbus_rst_r )
{
	abcbus_t *abcbus = get_safe_token(device);
	abcbus_daisy_state *daisy = abcbus->daisy_state;

	for ( ; daisy != NULL; daisy = daisy->next)
	{
		devcb_call_write_line(&daisy->out_rst_func, 0);
		devcb_call_write_line(&daisy->out_rst_func, 1);
	}

	return 0xff;
}

READ8_DEVICE_HANDLER( abcbus_inp_r )
{
	abcbus_t *abcbus = get_safe_token(device);
	abcbus_daisy_state *daisy = abcbus->daisy_state;
	UINT8 data = 0xff;

	for ( ; daisy != NULL; daisy = daisy->next)
	{
		data &= devcb_call_read8(&daisy->in_inp_func, 0);
	}

	if (LOG) logerror("ABCBUS: '%s' INP %02x\n", device->tag(), data);

	return data;
}

WRITE8_DEVICE_HANDLER( abcbus_utp_w )
{
	abcbus_t *abcbus = get_safe_token(device);
	abcbus_daisy_state *daisy = abcbus->daisy_state;

	if (LOG) logerror("ABCBUS: '%s' UTP %02x\n", device->tag(), data);

	for ( ; daisy != NULL; daisy = daisy->next)
	{
		devcb_call_write8(&daisy->out_utp_func, 0, data);
	}
}

READ8_DEVICE_HANDLER( abcbus_stat_r )
{
	abcbus_t *abcbus = get_safe_token(device);
	abcbus_daisy_state *daisy = abcbus->daisy_state;
	UINT8 data = 0xff;

	for ( ; daisy != NULL; daisy = daisy->next)
	{
		data &= devcb_call_read8(&daisy->in_stat_func, 0);
	}

	if (LOG) logerror("ABCBUS: '%s' STAT %02x\n", device->tag(), data);

	return data;
}

static void command_w(running_device *device, int index, UINT8 data)
{
	abcbus_t *abcbus = get_safe_token(device);
	abcbus_daisy_state *daisy = abcbus->daisy_state;

	if (LOG) logerror("ABCBUS: '%s' C%u %02x\n", device->tag(), index + 1, data);

	for ( ; daisy != NULL; daisy = daisy->next)
	{
		devcb_call_write8(&daisy->out_c_func[index], 0, data);
	}
}

WRITE8_DEVICE_HANDLER( abcbus_c1_w ) { command_w(device, 0, data); }
WRITE8_DEVICE_HANDLER( abcbus_c2_w ) { command_w(device, 1, data); }
WRITE8_DEVICE_HANDLER( abcbus_c3_w ) { command_w(device, 2, data); }
WRITE8_DEVICE_HANDLER( abcbus_c4_w ) { command_w(device, 3, data); }

/*-------------------------------------------------
    DEVICE_START( abcbus )
-------------------------------------------------*/

static DEVICE_START( abcbus )
{
	abcbus_t *abcbus = get_safe_token(device);
	const abcbus_daisy_chain *daisy = get_interface(device);

	abcbus_daisy_state *head = NULL;
	abcbus_daisy_state **tailptr = &head;

	/* create a linked list of devices */
	for ( ; daisy->tag != NULL; daisy++)
	{
		*tailptr = auto_alloc(device->machine, abcbus_daisy_state);

		/* find device */
		(*tailptr)->next = NULL;
		(*tailptr)->device = devtag_get_device(device->machine, daisy->tag);

		if ((*tailptr)->device == NULL)
		{
			fatalerror("Unable to locate device '%s'", daisy->tag);
		}

		/* resolve callbacks */
		devcb_resolve_write8(&(*tailptr)->out_cs_func, &daisy->out_cs_func, device);
		devcb_resolve_read8(&(*tailptr)->in_stat_func, &daisy->in_stat_func, device);
		devcb_resolve_read8(&(*tailptr)->in_inp_func, &daisy->in_inp_func, device);
		devcb_resolve_write8(&(*tailptr)->out_utp_func, &daisy->out_utp_func, device);
		devcb_resolve_write8(&(*tailptr)->out_c_func[0], &daisy->out_c1_func, device);
		devcb_resolve_write8(&(*tailptr)->out_c_func[1], &daisy->out_c2_func, device);
		devcb_resolve_write8(&(*tailptr)->out_c_func[2], &daisy->out_c3_func, device);
		devcb_resolve_write8(&(*tailptr)->out_c_func[3], &daisy->out_c4_func, device);
		devcb_resolve_write_line(&(*tailptr)->out_rst_func, &daisy->out_rst_func, device);

		tailptr = &(*tailptr)->next;
	}

	abcbus->daisy_state = head;
}

/*-------------------------------------------------
    DEVICE_GET_INFO( abcbus )
-------------------------------------------------*/

DEVICE_GET_INFO( abcbus )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(abcbus_t);									break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;												break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;							break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(abcbus);					break;
		case DEVINFO_FCT_STOP:							/* Nothing */												break;
		case DEVINFO_FCT_RESET:							/* Nothing */												break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "ABC bus");									break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "ABC-80");									break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 				break;
	}
}
