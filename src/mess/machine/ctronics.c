/***************************************************************************

    Centronics printer interface

***************************************************************************/

#include "driver.h"
#include "ctronics.h"


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static TIMER_CALLBACK( ack_callback );
static TIMER_CALLBACK( busy_callback );


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _centronics_state centronics_state;
struct _centronics_state
{
	const device_config *printer;

	devcb_resolved_write_line out_ack_func;
	devcb_resolved_write_line out_busy_func;
	devcb_resolved_write_line out_not_busy_func;

	int strobe;
	int busy;
	int ack;
	int auto_fd;

	UINT8 data;
};


/*****************************************************************************
    INLINE FUNCTIONS
*****************************************************************************/

INLINE centronics_state *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == CENTRONICS);

	return (centronics_state *)device->token;
}


/*****************************************************************************
    DEVICE INTERFACE
*****************************************************************************/

static DEVICE_START( centronics )
{
	centronics_state *centronics = get_safe_token(device);
	const centronics_interface *intf = device->static_config;
	char printer_tag[256];

	/* validate some basic stuff */
	assert(device->static_config != NULL);

	/* get printer device */
	strcpy(printer_tag, device->tag);
	strcat(printer_tag, "_p");
	centronics->printer = devtag_get_device(device->machine, PRINTER, printer_tag);

	assert(centronics->printer != NULL);

	/* make sure it's running */
	if (!centronics->printer->started)
	{
		device_delay_init(device);
		return;
	}

	/* resolve callbacks */
	devcb_resolve_write_line(&centronics->out_ack_func, &intf->out_ack_func, device);
	devcb_resolve_write_line(&centronics->out_busy_func, &intf->out_busy_func, device);
	devcb_resolve_write_line(&centronics->out_not_busy_func, &intf->out_not_busy_func, device);

	/* register for state saving */
	state_save_register_device_item(device, 0, centronics->auto_fd);
	state_save_register_device_item(device, 0, centronics->strobe);
	state_save_register_device_item(device, 0, centronics->busy);
	state_save_register_device_item(device, 0, centronics->ack);
	state_save_register_device_item(device, 0, centronics->data);
}

static DEVICE_RESET( centronics )
{
	centronics_state *centronics = get_safe_token(device);
	centronics->busy = printer_is_ready(centronics->printer) ? FALSE : TRUE;
}

static DEVICE_SET_INFO( centronics )
{
	switch (state)
	{
		/* no parameters to set */
	}
}

DEVICE_GET_INFO( centronics )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:			info->i = sizeof(centronics_state);		break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:	info->i = 0;							break;
		case DEVINFO_INT_CLASS:					info->i = DEVICE_CLASS_PERIPHERAL;		break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_SET_INFO:				info->set_info = DEVICE_SET_INFO_NAME( centronics );	break;
		case DEVINFO_FCT_START:					info->start = DEVICE_START_NAME( centronics );			break;
		case DEVINFO_FCT_STOP:					/* Nothing */										break;
		case DEVINFO_FCT_RESET:					info->reset = DEVICE_RESET_NAME( centronics );			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:					strcpy(info->s, "Centronics");			break;
		case DEVINFO_STR_FAMILY:				strcpy(info->s, "Centronics");			break;
		case DEVINFO_STR_VERSION:				strcpy(info->s, "1.0");					break;
		case DEVINFO_STR_SOURCE_FILE:			strcpy(info->s, __FILE__);				break;
		case DEVINFO_STR_CREDITS:				strcpy(info->s, "Copyright MESS Team");	break;
	}
}


/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    update_busy_state - check printer device
    status and update busy state
-------------------------------------------------*/

static int update_busy_state(const device_config *device)
{
	centronics_state *centronics = get_safe_token(device);
	centronics->busy = printer_is_ready(centronics->printer) ? FALSE : TRUE;
	return centronics->busy;
}


static TIMER_CALLBACK( ack_callback )
{
	centronics_state *centronics = (centronics_state *)ptr;

	/* signal change */
	devcb_call_write_line(&centronics->out_ack_func, param);
	centronics->ack = param;

	if (param == FALSE)
	{
		/* data is now ready, output it */
		printer_output(centronics->printer, centronics->data);

		/* timer to return BUSY to low, 5us max */
		timer_set(machine, ATTOTIME_IN_USEC(4), ptr, FALSE, busy_callback);
	}
}


static TIMER_CALLBACK( busy_callback )
{
	centronics_state *centronics = (centronics_state *)ptr;

	/* signal change */
	devcb_call_write_line(&centronics->out_busy_func, param);
	devcb_call_write_line(&centronics->out_not_busy_func, !param);
	centronics->busy = param;

	if (param == TRUE)
	{
		/* timer to turn ACK low to receive data */
		timer_set(machine, ATTOTIME_IN_NSEC(250), ptr, FALSE, ack_callback);
	}
	else
	{
		/* timer to return ACK to high state, 5us max */
		timer_set(machine, ATTOTIME_IN_USEC(4), ptr, TRUE, ack_callback);
	}
}


/*-------------------------------------------------
    centronics_data_w - write print data
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( centronics_data_w )
{
	centronics_state *centronics = get_safe_token(device);
	centronics->data = data;
}


/*-------------------------------------------------
    centronics_strobe_w - signal that data is
    ready
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( centronics_strobe_w )
{
	centronics_state *centronics = get_safe_token(device);

	/* ignore if we are busy */
	if (state == FALSE && centronics->busy == FALSE)
	{
		/* STROBE has gone low, data is ready */
		timer_set(device->machine, ATTOTIME_IN_NSEC(250), centronics, TRUE, busy_callback);
	}

	centronics->strobe = state;
}


/*-------------------------------------------------
    centronics_prime_w - initialize and reset
    printer (centronics mode)
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( centronics_prime_w )
{
	assert(((const centronics_interface *)device->inline_config)->is_ibmpc == FALSE);

	/* reset printer if line is low */
	if (state == FALSE)
		device_reset_centronics(device);
}


/*-------------------------------------------------
    centronics_init_w - initialize and reset
    printer (ibm mode)
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( centronics_init_w )
{
	assert(((const centronics_interface *)device->inline_config)->is_ibmpc == TRUE);

	/* reset printer if line is low */
	if (state == FALSE)
		device_reset_centronics(device);
}


/*-------------------------------------------------
    centronics_autofeed_w - auto line feed
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( centronics_autofeed_w )
{
	centronics_state *centronics = get_safe_token(device);
	assert(((const centronics_interface *)device->inline_config)->is_ibmpc == TRUE);

	centronics->auto_fd = state;
}


/*-------------------------------------------------
    centronics_ack_r - return the state of the
    ack line
-------------------------------------------------*/

READ_LINE_DEVICE_HANDLER( centronics_ack_r )
{
	centronics_state *centronics = get_safe_token(device);
	return centronics->ack;
}


/*-------------------------------------------------
    centronics_busy_r - return the state of the
    busy line
-------------------------------------------------*/

READ_LINE_DEVICE_HANDLER( centronics_busy_r )
{
	centronics_state *centronics = get_safe_token(device);
	return centronics->busy;
}


/*-------------------------------------------------
    centronics_pe_r - return the state of the
    pe line
-------------------------------------------------*/

READ_LINE_DEVICE_HANDLER( centronics_pe_r )
{
	/* use the busy line as pe line for now */
	return !update_busy_state(device);
}


/*-------------------------------------------------
    centronics_not_busy_r - return the state of the
    not busy line
-------------------------------------------------*/

READ_LINE_DEVICE_HANDLER( centronics_not_busy_r )
{
	centronics_state *centronics = get_safe_token(device);
	return !centronics->busy;
}


/*-------------------------------------------------
    centronics_not_busy_r - return the state of the
    vcc line
-------------------------------------------------*/

READ_LINE_DEVICE_HANDLER( centronics_vcc_r )
{
	/* always return high */
	return TRUE;
}


/*-------------------------------------------------
    centronics_fault_r - return the state of the
    fault line
-------------------------------------------------*/

READ_LINE_DEVICE_HANDLER( centronics_fault_r )
{
	/* use the busy line as fault line for now */
	return !update_busy_state(device);
}
