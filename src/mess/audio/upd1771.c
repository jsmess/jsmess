/**********************************************************************

    NEC uPD1771

**********************************************************************/

#include "sndintrf.h"
#include "streams.h"
#include "upd1771.h"


#define LOG 1

#define MAX_PACKET_SIZE 10


typedef struct _upd1771_state upd1771_state;
struct _upd1771_state
{
	sound_stream *channel;
	devcb_resolved_write_line ack_out_func;
	emu_timer *timer;

	UINT8	packet[MAX_PACKET_SIZE];
	UINT8	index;
	UINT8	expected_bytes;
};


INLINE upd1771_state *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == SOUND);
	assert(sound_get_type(device) == SOUND_UPD1771C);
	return (upd1771_state *)device->token;
}


WRITE8_DEVICE_HANDLER( upd1771_w )
{
	upd1771_state *state = get_safe_token( device );

	if (LOG)
		logerror( "upd1771_w: received byte 0x%02x\n", data );

	state->packet[ state->index ] = data;
	state->index = state->index + 1;

	devcb_call_write_line( &state->ack_out_func, 0 );

	/* Start of a new command */
	if ( state->expected_bytes == 0 )
	{
		switch ( data )
		{
		case 0x00:		/* 00 - 1 byte command */
			state->expected_bytes = 1;
			break;
		case 0x01:		/* 01 - 10 byte command */
			state->expected_bytes = 10;
		case 0x02:		/* 02 - 4 byte command */
			state->expected_bytes = 4;
			break;
		default:
			if (LOG)
				logerror("upd1771: unknown command %02x received\n", data );
			state->expected_bytes = 99;
			break;
		}
	}

	if ( state->index == state->expected_bytes )
	{
		/* The command packet has been transferred, handle the command */
		if (LOG)
			logerror("upd1771_w: handle command %02x\n", state->packet[0] );
		state->expected_bytes = 0;
	}
	else
	{
		/* We need more data to complete the command packet */
		/* Trigger ACK after 512??? cycles */
		timer_adjust_oneshot( state->timer, ticks_to_attotime( 512, device->clock ), 0 );
	}

	state->index = state->index % MAX_PACKET_SIZE;
}


static STREAM_UPDATE( upd1771c_update )
{
//	upd1771_state *state = (upd1771_state *)param;
	stream_sample_t *buffer = outputs[0];

	while ( samples > 0 )
	{
		*buffer = 0;
		buffer++;
		samples--;
	}
}


static TIMER_CALLBACK( upd1771c_callback )
{
	const device_config *device = (const device_config *)ptr;
	upd1771_state *state = get_safe_token( device );

	devcb_call_write_line( &state->ack_out_func, 1 );
}


static DEVICE_START( upd1771c )
{
	const upd1771_interface *intf = (const upd1771_interface *)device->static_config;
	upd1771_state *state = get_safe_token( device );
	int sample_rate = device->clock / 4;

	/* resolve callbacks */
	devcb_resolve_write_line( &state->ack_out_func, &intf->ack_callback, device );

	state->timer = timer_alloc( device->machine, upd1771c_callback, (void *)device );

	state->channel = stream_create( device, 0, 1, sample_rate, state, upd1771c_update );

	state_save_register_device_item_array( device, 0, state->packet );
	state_save_register_device_item(device, 0, state->index );
	state_save_register_device_item(device, 0, state->expected_bytes );
}


static DEVICE_RESET( upd1771c )
{
	upd1771_state *state = get_safe_token( device );

	state->index = 0;
	state->expected_bytes = 0;
}


DEVICE_GET_INFO( upd1771c )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:				info->i = sizeof(upd1771_state);				break;

		/* --- the following bits of info are returned as pointers to functions --- */
		case DEVINFO_FCT_START:						info->start = DEVICE_START_NAME( upd1771c );	break;
		case DEVINFO_FCT_RESET:						info->reset = DEVICE_RESET_NAME( upd1771c );	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:						strcpy(info->s, "NEC uPD1771C");				break;
		case DEVINFO_STR_FAMILY:					strcpy(info->s, "NEC uPD1771");					break;
		case DEVINFO_STR_VERSION:					strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:				strcpy(info->s, __FILE__);						break;
		case DEVINFO_STR_CREDITS:					strcpy(info->s, "Copyright the MAME & MESS Teams"); break;
	}
}

