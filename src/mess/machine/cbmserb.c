/***************************************************************************

    Commodore Serial Bus



    2009-05 FP: Changed the implementation to be a MAME device. However,
    the main code has been kept the same as before (to reduce the risk of
    regressions, at this early stage). More work will be eventually done
    to improve emulation and to allow more devices (possibly of different
    kinds) to be connected to the bus like in the real thing.
    Notice that, imho, the current implementation is not very satisfactory:
    it is not really emulating the serial bus, but (in some sense) the plug
    of the devices which would be connected to the serial bus. Hence, the
    provided handlers pass the serial state of the floppy drives to the CPU
    of the main machine.
    As a byproduct of this implementation, the serial_bus device is currently
    tied to the floppy drives (i.e. you will find MDRV_CBM_SERBUS_ADD used
    in the floppy drive MACHINE_DRIVERs, not in the main computer ones)

    In other words, don't use this driver as an example of the right way
    to implement serial devices in MAME/MESS!

***************************************************************************/

#include "driver.h"

#include "includes/cbmserb.h"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _cbm_serial_bus_t cbm_serial_bus_t;
struct _cbm_serial_bus_t
{
	devcb_resolved_read8 read_atn_func;
	devcb_resolved_read8 read_clock_func;
	devcb_resolved_read8 read_data_func;
	devcb_resolved_read8 read_request_func;
	devcb_resolved_write8 write_atn_func;
	devcb_resolved_write8 write_clock_func;
	devcb_resolved_write8 write_data_func;
	devcb_resolved_write8 write_request_func;

	devcb_resolved_write8 write_reset_func;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE cbm_serial_bus_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == CBM_SERBUS);
	return (cbm_serial_bus_t *)device->token;
}

INLINE const cbm_serial_bus_interface *get_interface(const device_config *device)
{
	assert(device != NULL);
	assert((device->type == CBM_SERBUS));
	return (const cbm_serial_bus_interface *) device->static_config;
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/


/* bus handling */
READ8_DEVICE_HANDLER( cbm_serial_request_read )
{
	cbm_serial_bus_t *serbus = get_safe_token(device);
	UINT8 val;

	if (serbus->read_request_func.read != NULL)
		val = devcb_call_read8(&serbus->read_request_func, offset);
	else
	{
		logerror("%s:Serial Bus %s: Request is being read but has no handler\n", cpuexec_describe_context(device->machine), device->tag);
		val = 0;
	}

	return val;
}

WRITE8_DEVICE_HANDLER( cbm_serial_request_write )
{
	cbm_serial_bus_t *serbus = get_safe_token(device);

	if (serbus->write_request_func.write != NULL)
		devcb_call_write8(&serbus->write_request_func, offset, data);
	else
		logerror("%s:Serial Bus %s: Request is being written but has no handler\n", cpuexec_describe_context(device->machine), device->tag);
}

READ8_DEVICE_HANDLER( cbm_serial_atn_read )
{
	cbm_serial_bus_t *serbus = get_safe_token(device);
	UINT8 val;

	if (serbus->read_atn_func.read != NULL)
		val = devcb_call_read8(&serbus->read_atn_func, offset);
	else
	{
		logerror("%s:Serial Bus %s: ATN is being read but has no handler\n", cpuexec_describe_context(device->machine), device->tag);
		val = 0;
	}

	return val;
}

READ8_DEVICE_HANDLER( cbm_serial_data_read )
{
	cbm_serial_bus_t *serbus = get_safe_token(device);
	UINT8 val;

	if (serbus->read_data_func.read != NULL)
		val = devcb_call_read8(&serbus->read_data_func, offset);
	else
	{
		logerror("%s:Serial Bus %s: Data is being read but has no handler\n", cpuexec_describe_context(device->machine), device->tag);
		val = 0;
	}

	return val;
}

READ8_DEVICE_HANDLER( cbm_serial_clock_read )
{
	cbm_serial_bus_t *serbus = get_safe_token(device);
	UINT8 val;

	if (serbus->read_clock_func.read != NULL)
		val = devcb_call_read8(&serbus->read_clock_func, offset);
	else
	{
		logerror("%s:Serial Bus %s: Clock is being read but has no handler\n", cpuexec_describe_context(device->machine), device->tag);
		val = 0;
	}

	return val;
}

WRITE8_DEVICE_HANDLER( cbm_serial_atn_write )
{
	cbm_serial_bus_t *serbus = get_safe_token(device);

	if (serbus->write_atn_func.write != NULL)
		devcb_call_write8(&serbus->write_atn_func, offset, data);
	else
		logerror("%s:Serial Bus %s: ATN is being written but has no handler\n", cpuexec_describe_context(device->machine), device->tag);
}

WRITE8_DEVICE_HANDLER( cbm_serial_data_write )
{
	cbm_serial_bus_t *serbus = get_safe_token(device);

	if (serbus->write_data_func.write != NULL)
		devcb_call_write8(&serbus->write_data_func, offset, data);
	else
		logerror("%s:Serial Bus %s: Data is being written but has no handler\n", cpuexec_describe_context(device->machine), device->tag);
}

WRITE8_DEVICE_HANDLER( cbm_serial_clock_write )
{
	cbm_serial_bus_t *serbus = get_safe_token(device);

	if (serbus->write_clock_func.write != NULL)
		devcb_call_write8(&serbus->write_clock_func, offset, data);
	else
		logerror("%s:Serial Bus %s: Clock is being written but has no handler\n", cpuexec_describe_context(device->machine), device->tag);
}


/*-------------------------------------------------
    DEVICE_START( cbm_serial_bus )
-------------------------------------------------*/

static DEVICE_START( cbm_serial_bus )
{
	cbm_serial_bus_t *serbus = get_safe_token(device);
	const cbm_serial_bus_interface *intf = get_interface(device);

	memset(serbus, 0, sizeof(*serbus));

	devcb_resolve_read8(&serbus->read_atn_func, &intf->read_atn_func, device);
	devcb_resolve_read8(&serbus->read_clock_func, &intf->read_clock_func, device);
	devcb_resolve_read8(&serbus->read_data_func, &intf->read_data_func, device);
	devcb_resolve_read8(&serbus->read_request_func, &intf->read_request_func, device);
	devcb_resolve_write8(&serbus->write_atn_func, &intf->write_atn_func, device);
	devcb_resolve_write8(&serbus->write_clock_func, &intf->write_clock_func, device);
	devcb_resolve_write8(&serbus->write_data_func, &intf->write_data_func, device);
	devcb_resolve_write8(&serbus->write_request_func, &intf->write_request_func, device);

	devcb_resolve_write8(&serbus->write_reset_func, &intf->write_reset_func, device);
}

/*-------------------------------------------------
    DEVICE_RESET( cbm_serial_bus )
-------------------------------------------------*/

static DEVICE_RESET( cbm_serial_bus )
{
	cbm_serial_bus_t *serbus = get_safe_token(device);

	if (serbus->write_reset_func.write != NULL)
		devcb_call_write8(&serbus->write_reset_func, 0, 0);
	else
		logerror("%s:Serial Bus %s: Reset is being written but has no handler\n", cpuexec_describe_context(device->machine), device->tag);
}

/*-------------------------------------------------
    DEVICE_GET_INFO( cbm_serial_bus )
-------------------------------------------------*/

DEVICE_GET_INFO( cbm_serial_bus )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(cbm_serial_bus_t);			break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(cbm_serial_bus);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(cbm_serial_bus);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore Serial Bus");	break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Commodore Serial Bus");	break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						/* Nothing */								break;
	}
}
