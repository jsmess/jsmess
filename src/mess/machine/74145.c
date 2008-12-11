/*****************************************************************************
 *
 * machine/74145.c
 *
 * BCD-to-Decimal decoder
 *
 *        __ __
 *     0-|  v  |-VCC
 *     1-|     |-A
 *     2-|     |-B
 *     3-|     |-C
 *     4-|     |-D
 *     5-|     |-9
 *     6-|     |-8
 *   GND-|_____|-7
 *
 *
 * Truth table
 *  _______________________________
 * | Inputs  | Outputs             |
 * | D C B A | 0 1 2 3 4 5 6 7 8 9 |
 * |-------------------------------|
 * | L L L L | L H H H H H H H H H |
 * | L L L H | H L H H H H H H H H |
 * | L L H L | H H L H H H H H H H |
 * | L L H H | H H H L H H H H H H |
 * | L H L L | H H H H L H H H H H |
 * |-------------------------------|
 * | L H L H | H H H H H L H H H H |
 * | L H H L | H H H H H H L H H H |
 * | L H H H | H H H H H H H L H H |
 * | H L L L | H H H H H H H H L H |
 * | H L L H | H H H H H H H H H L |
 * |-------------------------------|
 * | H L H L | H H H H H H H H H H |
 * | H L H H | H H H H H H H H H H |
 * | H H L L | H H H H H H H H H H |
 * | H H L H | H H H H H H H H H H |
 * | H H H L | H H H H H H H H H H |
 * | H H H H | H H H H H H H H H H |
 *  -------------------------------
 *
 ****************************************************************************/

#include "driver.h"
#include "74145.h"


/*****************************************************************************
 Type definitions
*****************************************************************************/


typedef struct _ttl74145_t ttl74145_t;
struct _ttl74145_t
{
	/* Pointer to our interface */
	const ttl74145_interface *intf;

	/* Decoded number */
	UINT16 number;
};

/*****************************************************************************
 Implementation
*****************************************************************************/

INLINE ttl74145_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);

	return (ttl74145_t *)device->token;
}

WRITE8_DEVICE_HANDLER( ttl74145_w ) { 
	/* Decode number */
	ttl74145_t *ttl74145 = get_safe_token(device);
	UINT16 new_number = bcd_2_dec(data & 0x0f);
	
	/* Call output callbacks if the number changed */
	if (new_number != ttl74145->number)
	{
		const ttl74145_interface *i = ttl74145->intf;
	
		if (i->output_line_0) i->output_line_0(device, new_number == 0);
		if (i->output_line_1) i->output_line_1(device, new_number == 1);
		if (i->output_line_2) i->output_line_2(device, new_number == 2);
		if (i->output_line_3) i->output_line_3(device, new_number == 3);
		if (i->output_line_4) i->output_line_4(device, new_number == 4);
		if (i->output_line_5) i->output_line_5(device, new_number == 5);
		if (i->output_line_6) i->output_line_6(device, new_number == 6);
		if (i->output_line_7) i->output_line_7(device, new_number == 7);
		if (i->output_line_8) i->output_line_8(device, new_number == 8);
		if (i->output_line_9) i->output_line_9(device, new_number == 9);
	}
	
	/* Update state */
	ttl74145->number = new_number;		
}

READ16_DEVICE_HANDLER( ttl74145_r ) { 
	ttl74145_t *ttl74145 = get_safe_token(device);
	
	return ttl74145->number; 
}

/* Device Interface */

static DEVICE_START( ttl74145 )
{
	ttl74145_t *ttl74145 = get_safe_token(device);
	// validate arguments

	assert(device != NULL);
	assert(device->tag != NULL);
	assert(strlen(device->tag) < 20);
	assert(device->static_config != NULL);

	ttl74145->intf = device->static_config;
	ttl74145->number = 0;
	
	// register for state saving
	state_save_register_item(device->machine, "ttl74145", device->tag, 0, ttl74145->number);
	return DEVICE_START_OK;
}

static DEVICE_RESET( ttl74145 )
{
	ttl74145_t *ttl74145 = get_safe_token(device);
	ttl74145->number = 0;
}

static DEVICE_SET_INFO( ttl74145 )
{
	switch (state)
	{
		/* no parameters to set */
	}
}

DEVICE_GET_INFO( ttl74145 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(ttl74145_t);					break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_SET_INFO:						info->set_info = DEVICE_SET_INFO_NAME(ttl74145); break;
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(ttl74145);		break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(ttl74145);		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							info->s = "TTL74145";						break;
		case DEVINFO_STR_FAMILY:						info->s = "TTL74145";						break;
		case DEVINFO_STR_VERSION:						info->s = "1.0";							break;
		case DEVINFO_STR_SOURCE_FILE:					info->s = __FILE__;							break;
		case DEVINFO_STR_CREDITS:						info->s = "Copyright MESS Team";			break;
	}
}
