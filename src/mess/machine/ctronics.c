/***************************************************************************

    Centronics printer interface

***************************************************************************/

#include "emu.h"
#include "ctronics.h"
#include "devices/printer.h"


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static WRITE_LINE_DEVICE_HANDLER( centronics_printer_online );
static TIMER_CALLBACK( ack_callback );
static TIMER_CALLBACK( busy_callback );


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _centronics_state centronics_state;
struct _centronics_state
{
	running_device *printer;

	devcb_resolved_write_line out_ack_func;
	devcb_resolved_write_line out_busy_func;
	devcb_resolved_write_line out_not_busy_func;

	int strobe;
	int busy;
	int ack;
	int auto_fd;
	int pe;
	int fault;

	UINT8 data;
};


/*****************************************************************************
    INLINE FUNCTIONS
*****************************************************************************/

INLINE centronics_state *get_safe_token(running_device *device)
{
	assert(device != NULL);
	assert(device->type() == CENTRONICS);

	return (centronics_state *)downcast<legacy_device_base *>(device)->token();
}


/*****************************************************************************
    GLOBAL VARIABLES
*****************************************************************************/

const centronics_interface standard_centronics =
{
	FALSE,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


/*****************************************************************************
    PRINTER INTERFACE
*****************************************************************************/

static MACHINE_DRIVER_START( centronics )
	MDRV_PRINTER_ADD("printer")
	MDRV_PRINTER_ONLINE(centronics_printer_online)
MACHINE_DRIVER_END


/*****************************************************************************
    DEVICE INTERFACE
*****************************************************************************/

static DEVICE_START( centronics )
{
	centronics_state *centronics = get_safe_token(device);
	const centronics_interface *intf = (const centronics_interface *)device->baseconfig().static_config();

	/* validate some basic stuff */
	assert(device->baseconfig().static_config() != NULL);

	/* set some initial values */
	centronics->pe = FALSE;
	centronics->fault = FALSE;
	centronics->busy = TRUE;
	centronics->strobe = TRUE;

	/* get printer device */
	centronics->printer = device->subdevice("printer");

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
}

DEVICE_GET_INFO( centronics )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:			info->i = sizeof(centronics_state);		break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:	info->i = 0;							break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_MACHINE_CONFIG:		info->machine_config = MACHINE_DRIVER_NAME(centronics);	break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:					info->start = DEVICE_START_NAME(centronics);		break;
		case DEVINFO_FCT_STOP:					/* Nothing */										break;
		case DEVINFO_FCT_RESET:					info->reset = DEVICE_RESET_NAME(centronics);		break;

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
    centronics_printer_online - callback that
    sets us busy when the printer goes offline
-------------------------------------------------*/

void centronics_printer_online(running_device *device, int state)
{
	centronics_state *centronics = get_safe_token(device->owner());

	/* when going online, set PE and FAULT high and BUSY low */
	centronics->pe = state;
	centronics->fault = state;
	centronics->busy = !state;
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

		/* ready to receive more data, return BUSY to low */
		timer_set(machine, ATTOTIME_IN_USEC(7), ptr, FALSE, busy_callback);
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
		timer_set(machine, ATTOTIME_IN_USEC(10), ptr, FALSE, ack_callback);
	}
	else
	{
		/* timer to return ACK to high state */
		timer_set(machine, ATTOTIME_IN_USEC(5), ptr, TRUE, ack_callback);
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
    centronics_data_r - return current data
-------------------------------------------------*/

READ8_DEVICE_HANDLER( centronics_data_r )
{
	centronics_state *centronics = get_safe_token(device);
	return centronics->data;
}


/*-------------------------------------------------
    set_line - helper to set individual bits
-------------------------------------------------*/

static void set_line(running_device *device, int line, int state)
{
	centronics_state *centronics = get_safe_token(device);

	if (state)
		centronics->data |= 1 << line;
	else
		centronics->data &= ~(1 << line);
}


/*-------------------------------------------------
    centronics_dx_w - write line dx print data
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( centronics_d0_w ) { set_line(device, 0, state); }
WRITE_LINE_DEVICE_HANDLER( centronics_d1_w ) { set_line(device, 1, state); }
WRITE_LINE_DEVICE_HANDLER( centronics_d2_w ) { set_line(device, 2, state); }
WRITE_LINE_DEVICE_HANDLER( centronics_d3_w ) { set_line(device, 3, state); }
WRITE_LINE_DEVICE_HANDLER( centronics_d4_w ) { set_line(device, 4, state); }
WRITE_LINE_DEVICE_HANDLER( centronics_d5_w ) { set_line(device, 5, state); }
WRITE_LINE_DEVICE_HANDLER( centronics_d6_w ) { set_line(device, 6, state); }
WRITE_LINE_DEVICE_HANDLER( centronics_d7_w ) { set_line(device, 7, state); }


/*-------------------------------------------------
    centronics_strobe_w - signal that data is
    ready
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( centronics_strobe_w )
{
	centronics_state *centronics = get_safe_token(device);

	/* look for a high -> low transition */
	if (centronics->strobe == TRUE && state == FALSE && centronics->busy == FALSE)
	{
		/* STROBE has gone low, data is ready */
		timer_set(device->machine, attotime_zero, centronics, TRUE, busy_callback);
	}

	centronics->strobe = state;
}


/*-------------------------------------------------
    centronics_prime_w - initialize and reset
    printer (centronics mode)
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( centronics_prime_w )
{
	assert(((const centronics_interface *)device->baseconfig().static_config())->is_ibmpc == FALSE);

	/* reset printer if line is low */
	if (state == FALSE)
		DEVICE_RESET_CALL( centronics );
}


/*-------------------------------------------------
    centronics_init_w - initialize and reset
    printer (ibm mode)
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( centronics_init_w )
{
	assert(((const centronics_interface *)device->baseconfig().static_config())->is_ibmpc == TRUE);

	/* reset printer if line is low */
	if (state == FALSE)
		DEVICE_RESET_CALL( centronics );
}


/*-------------------------------------------------
    centronics_autofeed_w - auto line feed
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( centronics_autofeed_w )
{
	centronics_state *centronics = get_safe_token(device);
	assert(((const centronics_interface *)device->baseconfig().static_config())->is_ibmpc == TRUE);

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
	centronics_state *centronics = get_safe_token(device);
	return centronics->pe;
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
    centronics_vcc_r - return the state of the
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
	centronics_state *centronics = get_safe_token(device);
	return centronics->fault;
}

DEFINE_LEGACY_DEVICE(CENTRONICS, centronics);
