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

#include "emu.h"
#include "74145.h"
#include "coreutil.h"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _ttl74145_t ttl74145_t;
struct _ttl74145_t
{
	devcb_resolved_write_line output_line_0;
	devcb_resolved_write_line output_line_1;
	devcb_resolved_write_line output_line_2;
	devcb_resolved_write_line output_line_3;
	devcb_resolved_write_line output_line_4;
	devcb_resolved_write_line output_line_5;
	devcb_resolved_write_line output_line_6;
	devcb_resolved_write_line output_line_7;
	devcb_resolved_write_line output_line_8;
	devcb_resolved_write_line output_line_9;

	/* decoded number */
	int number;
};


/*****************************************************************************
    GLOBAL VARIABLES
*****************************************************************************/

const ttl74145_interface default_ttl74145 =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

INLINE ttl74145_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == TTL74145);

	return (ttl74145_t *)downcast<legacy_device_base *>(device)->token();
}


WRITE8_DEVICE_HANDLER( ttl74145_w )
{
	ttl74145_t *ttl74145 = get_safe_token(device);

	/* decode number */
	int new_number = bcd_2_dec(data & 0x0f);

	/* call output callbacks if the number changed */
	if (new_number != ttl74145->number)
	{
		devcb_call_write_line(&ttl74145->output_line_0, new_number == 0);
		devcb_call_write_line(&ttl74145->output_line_1, new_number == 1);
		devcb_call_write_line(&ttl74145->output_line_2, new_number == 2);
		devcb_call_write_line(&ttl74145->output_line_3, new_number == 3);
		devcb_call_write_line(&ttl74145->output_line_4, new_number == 4);
		devcb_call_write_line(&ttl74145->output_line_5, new_number == 5);
		devcb_call_write_line(&ttl74145->output_line_6, new_number == 6);
		devcb_call_write_line(&ttl74145->output_line_7, new_number == 7);
		devcb_call_write_line(&ttl74145->output_line_8, new_number == 8);
		devcb_call_write_line(&ttl74145->output_line_9, new_number == 9);
	}

	/* update state */
	ttl74145->number = new_number;
}


READ16_DEVICE_HANDLER( ttl74145_r )
{
	ttl74145_t *ttl74145 = get_safe_token(device);

	return (1 << ttl74145->number) & 0x3ff;
}


/***************************************************************************
    DEVICE INTERFACE
***************************************************************************/

static DEVICE_START( ttl74145 )
{
	const ttl74145_interface *intf = (const ttl74145_interface *)device->baseconfig().static_config();
	ttl74145_t *ttl74145 = get_safe_token(device);

	/* validate arguments */
	assert(device->baseconfig().static_config() != NULL);

	/* initialize with 0 */
	ttl74145->number = 0;

	/* resolve callbacks */
	devcb_resolve_write_line(&ttl74145->output_line_0, &intf->output_line_0, device);
	devcb_resolve_write_line(&ttl74145->output_line_1, &intf->output_line_1, device);
	devcb_resolve_write_line(&ttl74145->output_line_2, &intf->output_line_2, device);
	devcb_resolve_write_line(&ttl74145->output_line_3, &intf->output_line_3, device);
	devcb_resolve_write_line(&ttl74145->output_line_4, &intf->output_line_4, device);
	devcb_resolve_write_line(&ttl74145->output_line_5, &intf->output_line_5, device);
	devcb_resolve_write_line(&ttl74145->output_line_6, &intf->output_line_6, device);
	devcb_resolve_write_line(&ttl74145->output_line_7, &intf->output_line_7, device);
	devcb_resolve_write_line(&ttl74145->output_line_8, &intf->output_line_8, device);
	devcb_resolve_write_line(&ttl74145->output_line_9, &intf->output_line_9, device);

	/* register for state saving */
	state_save_register_item(device->machine(), "ttl74145", device->tag(), 0, ttl74145->number);
}


static DEVICE_RESET( ttl74145 )
{
	ttl74145_t *ttl74145 = get_safe_token(device);
	ttl74145->number = 0;
}


DEVICE_GET_INFO( ttl74145 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:			info->i = sizeof(ttl74145_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:	info->i = 0;								break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:					info->start = DEVICE_START_NAME(ttl74145);	break;
		case DEVINFO_FCT_STOP:					/* Nothing */								break;
		case DEVINFO_FCT_RESET:					info->reset = DEVICE_RESET_NAME(ttl74145);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:					strcpy(info->s, "TTL74145");				break;
		case DEVINFO_STR_FAMILY:				strcpy(info->s, "TTL74145");				break;
		case DEVINFO_STR_VERSION:				strcpy(info->s, "1.2");						break;
		case DEVINFO_STR_SOURCE_FILE:			strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:				strcpy(info->s, "Copyright MESS Team");		break;
	}
}

DEFINE_LEGACY_DEVICE(TTL74145, ttl74145);
