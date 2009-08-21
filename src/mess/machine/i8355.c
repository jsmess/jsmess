/**********************************************************************

    Intel 8355 - 16,384-Bit ROM with I/O emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

	TODO:

	- attempt to direct-map

*/

#include "i8355.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

enum
{
	I8355_REGISTER_PORT_A = 0,
	I8355_REGISTER_PORT_B,
	I8355_REGISTER_PORT_A_DDR,
	I8355_REGISTER_PORT_B_DDR
};

enum
{
	PORT_A = 0,
	PORT_B,
	PORT_COUNT
};

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _i8355_t i8355_t;
struct _i8355_t
{
	devcb_resolved_read8		in_rom_func;
	devcb_resolved_read8		in_port_func[PORT_COUNT];
	devcb_resolved_write8		out_port_func[PORT_COUNT];

	/* registers */
	UINT8 output[PORT_COUNT];	/* output latches */
	UINT8 ddr[PORT_COUNT];		/* DDR latches */

	/* ROM */
	const UINT8 *rom;			/* ROM */
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE i8355_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	return (i8355_t *)device->token;
}

INLINE const i8355_interface *get_interface(const device_config *device)
{
	assert(device != NULL);
	assert(device->type == I8355);
	return (const i8355_interface *) device->static_config;
}

INLINE UINT8 read_port(i8355_t *i8355, int port)
{
	UINT8 data = i8355->output[port] & i8355->ddr[port];

	if (i8355->ddr[port] != 0xff)
	{
		data |= devcb_call_read8(&i8355->in_port_func[port], 0) & ~i8355->ddr[port];
	}

	return data;
}

INLINE void write_port(i8355_t *i8355, int port, UINT8 data)
{
	i8355->output[port] = data;

	devcb_call_write8(&i8355->out_port_func[port], 0, i8355->output[port] & i8355->ddr[port]);
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    i8355_register_r - register read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( i8355_r )
{
	i8355_t *i8355 = get_safe_token(device);
	int port = offset & 0x01;

	UINT8 data = 0;

	switch (offset & 0x03)
	{
	case I8355_REGISTER_PORT_A:
	case I8355_REGISTER_PORT_B:
		data = read_port(i8355, port);
		break;

	case I8355_REGISTER_PORT_A_DDR:
	case I8355_REGISTER_PORT_B_DDR:
		/* write only */
		break;
	}

	return data;
}

/*-------------------------------------------------
    i8355_register_w - register write
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( i8355_w )
{
	i8355_t *i8355 = get_safe_token(device);
	int port = offset & 0x01;

	switch (offset & 0x03)
	{
	case I8355_REGISTER_PORT_A:
	case I8355_REGISTER_PORT_B:
		if (LOG) logerror("I8355 '%s' Port %c Write %02x\n", device->tag, 'A' + port, data);
		write_port(i8355, port, data);
		break;

	case I8355_REGISTER_PORT_A_DDR:
	case I8355_REGISTER_PORT_B_DDR:
		if (LOG) logerror("I8355 '%s' Port %c DDR: %02x\n", device->tag, 'A' + port, data);
		i8355->ddr[port] = data;
		write_port(i8355, port, data);
		break;
	}
}

/*-------------------------------------------------
    i8355_rom_r - memory read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( i8355_rom_r )
{
	i8355_t *i8355 = get_safe_token(device);

	return i8355->rom[offset & 0x7ff];
}

/*-------------------------------------------------
    DEVICE_START( i8355 )
-------------------------------------------------*/

static DEVICE_START( i8355 )
{
	i8355_t *i8355 = device->token;
	const i8355_interface *intf = get_interface(device);

	/* resolve callbacks */
	devcb_resolve_read8(&i8355->in_port_func[PORT_A], &intf->in_pa_func, device);
	devcb_resolve_read8(&i8355->in_port_func[PORT_B], &intf->in_pb_func, device);
	devcb_resolve_write8(&i8355->out_port_func[PORT_A], &intf->out_pa_func, device);
	devcb_resolve_write8(&i8355->out_port_func[PORT_B], &intf->out_pb_func, device);

	/* find memory region */
	i8355->rom = memory_region(device->machine, device->tag);
	assert(i8355->rom != NULL);

	/* register for state saving */
	state_save_register_device_item_array(device, 0, i8355->output);
	state_save_register_device_item_array(device, 0, i8355->ddr);
}

/*-------------------------------------------------
    DEVICE_RESET( i8355 )
-------------------------------------------------*/

static DEVICE_RESET( i8355 )
{
	i8355_t *i8355 = device->token;

	/* set ports to input mode */
	i8355->ddr[PORT_A] = 0;
	i8355->ddr[PORT_B] = 0;
}

/*-------------------------------------------------
    DEVICE_GET_INFO( i8355 )
-------------------------------------------------*/

DEVICE_GET_INFO( i8355 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(i8355_t);					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(i8355);		break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(i8355);		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Intel 8355");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Intel MCS-85");			break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team");	break;
	}
}
