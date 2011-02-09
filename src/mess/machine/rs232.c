/**********************************************************************


    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "emu.h"
#include "rs232.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _rs232_equipment rs232_equipment;
struct _rs232_equipment
{
	device_t *device;

	devcb_resolved_write_line		out_rd_func;
	devcb_resolved_write_line		out_dcd_func;
	devcb_resolved_write_line		out_dtr_func;
	devcb_resolved_write_line		out_dsr_func;
	devcb_resolved_write_line		out_rts_func;
	devcb_resolved_write_line		out_cts_func;
	devcb_resolved_write_line		out_ri_func;
};

typedef struct _rs232_t rs232_t;
struct _rs232_t
{
	rs232_equipment	dte;
	rs232_equipment dce;

	int data;
	int dcd;
	int dtr;
	int dsr;
	int rts;
	int cts;
	int ri;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE rs232_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == RS232);
	return (rs232_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE const rs232_interface *get_interface(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == RS232);
	return (const rs232_interface *) device->baseconfig().static_config();
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

READ_LINE_DEVICE_HANDLER( rs232_rd_r )
{
	rs232_t *rs232 = get_safe_token(device);

	return rs232->data;
}

void rs232_td_w(device_t *bus, device_t *device, int state)
{
	rs232_t *rs232 = get_safe_token(bus);

	rs232->data = state;

	if (device == rs232->dte.device)
	{
		//devcb_call_write_line(&rs232->dce.out_rd_func, state);
	}
	else if (device == rs232->dce.device)
	{
		//devcb_call_write_line(&rs232->dte.out_rd_func, state);
	}
	else
	{
		//fatalerror("RS-232 Invalid device '%s' attempted to write data!", device->tag());
	}

	if (LOG) logerror("RS232 '%s' TD %u\n", bus->tag(), state);
}

READ_LINE_DEVICE_HANDLER( rs232_dcd_r )
{
	rs232_t *rs232 = get_safe_token(device);

	return rs232->dcd;
}

WRITE_LINE_DEVICE_HANDLER( rs232_dcd_w )
{
	rs232_t *rs232 = get_safe_token(device);

	rs232->dcd = state;

	if (LOG) logerror("RS232 '%s' DCD %u\n", device->tag(), state);

	//devcb_call_write_line(&rs232->dte.out_dcd_func, state);
}

READ_LINE_DEVICE_HANDLER( rs232_dtr_r )
{
	rs232_t *rs232 = get_safe_token(device);

	return rs232->dtr;
}

WRITE_LINE_DEVICE_HANDLER( rs232_dtr_w )
{
	rs232_t *rs232 = get_safe_token(device);

	rs232->dtr = state;

	if (LOG) logerror("RS232 '%s' DTR %u\n", device->tag(), state);

	//devcb_call_write_line(&rs232->dce.out_dtr_func, state);
}

READ_LINE_DEVICE_HANDLER( rs232_dsr_r )
{
	rs232_t *rs232 = get_safe_token(device);

	return rs232->dsr;
}

WRITE_LINE_DEVICE_HANDLER( rs232_dsr_w )
{
	rs232_t *rs232 = get_safe_token(device);

	rs232->dsr = state;

	if (LOG) logerror("RS232 '%s' DSR %u\n", device->tag(), state);

	//devcb_call_write_line(&rs232->dte.out_dsr_func, state);
}

READ_LINE_DEVICE_HANDLER( rs232_rts_r )
{
	rs232_t *rs232 = get_safe_token(device);

	return rs232->rts;
}

WRITE_LINE_DEVICE_HANDLER( rs232_rts_w )
{
	rs232_t *rs232 = get_safe_token(device);

	rs232->rts = state;

	if (LOG) logerror("RS232 '%s' RTS %u\n", device->tag(), state);

	//devcb_call_write_line(&rs232->dce.out_rts_func, state);
}

READ_LINE_DEVICE_HANDLER( rs232_cts_r )
{
	rs232_t *rs232 = get_safe_token(device);

	return rs232->cts;
}

WRITE_LINE_DEVICE_HANDLER( rs232_cts_w )
{
	rs232_t *rs232 = get_safe_token(device);

	rs232->cts = state;

	if (LOG) logerror("RS232 '%s' CTS %u\n", device->tag(), state);

	//devcb_call_write_line(&rs232->dte.out_cts_func, state);
}

READ_LINE_DEVICE_HANDLER( rs232_ri_r )
{
	rs232_t *rs232 = get_safe_token(device);

	return rs232->ri;
}

WRITE_LINE_DEVICE_HANDLER( rs232_ri_w )
{
	rs232_t *rs232 = get_safe_token(device);

	rs232->ri = state;

	if (LOG) logerror("RS232 '%s' RI %u\n", device->tag(), state);

	//devcb_call_write_line(&rs232->dte.out_ri_func, state);
}

/*-------------------------------------------------
    DEVICE_START( rs232 )
-------------------------------------------------*/

static DEVICE_START( rs232 )
{
	rs232_t *rs232 = get_safe_token(device);

	/* register for state saving */
	device->save_item(NAME(rs232->data));
	device->save_item(NAME(rs232->dcd));
	device->save_item(NAME(rs232->dtr));
	device->save_item(NAME(rs232->dsr));
	device->save_item(NAME(rs232->rts));
	device->save_item(NAME(rs232->cts));
	device->save_item(NAME(rs232->ri));
}

/*-------------------------------------------------
    DEVICE_GET_INFO( rs232 )
-------------------------------------------------*/

DEVICE_GET_INFO( rs232 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(rs232_t);									break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;												break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(rs232);						break;
		case DEVINFO_FCT_STOP:							/* Nothing */												break;
		case DEVINFO_FCT_RESET:							/* Nothing */												break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "RS-232");									break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "RS-232");									break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 				break;
	}
}

DEFINE_LEGACY_DEVICE(RS232, rs232);
