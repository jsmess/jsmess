#include "emu.h"
#include "sound/speaker.h"
#include "cpu/upd7810/upd7810.h"
#include "includes/gamepock.h"


static void hd44102ch_w( gamepock_state *state, int which, int c_d, UINT8 data )
{
	if ( c_d )
	{
		UINT8	y;
		/* Data */
		state->hd44102ch[which].ram[ state->hd44102ch[which].address ] = data;

		/* Increment/decrement Y counter */
		y = ( state->hd44102ch[which].address & 0x3F ) + state->hd44102ch[which].y_inc;
		if ( y == 0xFF )
		{
			y = 49;
		}
		if ( y == 50 )
		{
			y = 0;
		}
		state->hd44102ch[which].address = ( state->hd44102ch[which].address & 0xC0 ) | y;
	}
	else
	{
		/* Command */
		switch ( data )
		{
		case 0x38:		/* Display off */
			state->hd44102ch[which].enabled = 0;
			break;
		case 0x39:		/* Display on */
			state->hd44102ch[which].enabled = 1;
			break;
		case 0x3A:		/* Y decrement mode */
			state->hd44102ch[which].y_inc = 0xFF;
			break;
		case 0x3B:		/* Y increment mode */
			state->hd44102ch[which].y_inc = 0x01;
			break;
		case 0x3E:		/* Display start page #0 */
		case 0x7E:		/* Display start page #1 */
		case 0xBE:		/* Display start page #2 */
		case 0xFE:		/* Display start page #3 */
			state->hd44102ch[which].start_page = data & 0xC0;
			break;
		default:
			if ( ( data & 0x3F ) < 50 )
			{
				state->hd44102ch[which].address = data;
			}
			break;
		}
	}
}

static void hd44102ch_init( gamepock_state *state, int which )
{
	memset( &state->hd44102ch[which], 0, sizeof( HD44102CH ) );
	state->hd44102ch[which].y_inc = 0x01;
}

static void gamepock_lcd_update(gamepock_state *state)
{
	/* Check whether HD44102CH #1 is enabled */
	if ( state->port_a & 0x08 )
	{
		hd44102ch_w( state, 0, state->port_a & 0x04, state->port_b );
	}

	/* Check whether HD44102CH #2 is enabled */
	if ( state->port_a & 0x10 )
	{
		hd44102ch_w( state, 1, state->port_a & 0x04, state->port_b );
	}

	/* Check whether HD44102CH #3 is enabled */
	if ( state->port_a & 0x20 )
	{
		hd44102ch_w( state, 2, state->port_a & 0x04, state->port_b );
	}
}

WRITE8_HANDLER( gamepock_port_a_w )
{
	gamepock_state *state = space->machine->driver_data<gamepock_state>();
	UINT8	old_port_a = state->port_a;

	state->port_a = data;

	if ( ! ( old_port_a & 0x02 ) && ( state->port_a & 0x02 ) )
	{
		gamepock_lcd_update(state);
	}
}

WRITE8_HANDLER( gamepock_port_b_w )
{
	gamepock_state *state = space->machine->driver_data<gamepock_state>();
	state->port_b = data;
}

READ8_HANDLER( gamepock_port_b_r )
{
	logerror("gamepock_port_b_r: not implemented\n");
	return 0xFF;
}

READ8_HANDLER( gamepock_port_c_r )
{
	gamepock_state *state = space->machine->driver_data<gamepock_state>();
	UINT8	data = 0xFF;

	if ( state->port_a & 0x80 )
	{
		data &= input_port_read(space->machine, "IN0");
	}

	if ( state->port_a & 0x40 )
	{
		data &= input_port_read(space->machine, "IN1");
	}

	return data;
}

MACHINE_RESET( gamepock )
{
	gamepock_state *state = machine->driver_data<gamepock_state>();
	hd44102ch_init( state, 0 );
	hd44102ch_init( state, 1 );
	hd44102ch_init( state, 2 );
}

VIDEO_UPDATE( gamepock )
{
	gamepock_state *state = screen->machine->driver_data<gamepock_state>();
	UINT8	ad;
	int		i,j;

	/* Handle HD44102CH #0 */
	ad = state->hd44102ch[0].start_page;
	for ( i = 0; i < 4; i++ )
	{
		for ( j = 0; j < 50; j++ )
		{
			*BITMAP_ADDR16(bitmap, i * 8 + 0, 49 - j ) = ( state->hd44102ch[0].ram[ad+j] & 0x01 ) ? 0 : 1;
			*BITMAP_ADDR16(bitmap, i * 8 + 1, 49 - j ) = ( state->hd44102ch[0].ram[ad+j] & 0x02 ) ? 0 : 1;
			*BITMAP_ADDR16(bitmap, i * 8 + 2, 49 - j ) = ( state->hd44102ch[0].ram[ad+j] & 0x04 ) ? 0 : 1;
			*BITMAP_ADDR16(bitmap, i * 8 + 3, 49 - j ) = ( state->hd44102ch[0].ram[ad+j] & 0x08 ) ? 0 : 1;
			*BITMAP_ADDR16(bitmap, i * 8 + 4, 49 - j ) = ( state->hd44102ch[0].ram[ad+j] & 0x10 ) ? 0 : 1;
			*BITMAP_ADDR16(bitmap, i * 8 + 5, 49 - j ) = ( state->hd44102ch[0].ram[ad+j] & 0x20 ) ? 0 : 1;
			*BITMAP_ADDR16(bitmap, i * 8 + 6, 49 - j ) = ( state->hd44102ch[0].ram[ad+j] & 0x40 ) ? 0 : 1;
			*BITMAP_ADDR16(bitmap, i * 8 + 7, 49 - j ) = ( state->hd44102ch[0].ram[ad+j] & 0x80 ) ? 0 : 1;
		}
		ad += 0x40;
	}

	/* Handle HD44102CH #1 */
	ad = state->hd44102ch[1].start_page;
	for ( i = 4; i < 8; i++ )
	{
		for ( j = 0; j < 50; j++ )
		{
			*BITMAP_ADDR16(bitmap, i * 8 + 0, j ) = ( state->hd44102ch[1].ram[ad+j] & 0x01 ) ? 0 : 1;
			*BITMAP_ADDR16(bitmap, i * 8 + 1, j ) = ( state->hd44102ch[1].ram[ad+j] & 0x02 ) ? 0 : 1;
			*BITMAP_ADDR16(bitmap, i * 8 + 2, j ) = ( state->hd44102ch[1].ram[ad+j] & 0x04 ) ? 0 : 1;
			*BITMAP_ADDR16(bitmap, i * 8 + 3, j ) = ( state->hd44102ch[1].ram[ad+j] & 0x08 ) ? 0 : 1;
			*BITMAP_ADDR16(bitmap, i * 8 + 4, j ) = ( state->hd44102ch[1].ram[ad+j] & 0x10 ) ? 0 : 1;
			*BITMAP_ADDR16(bitmap, i * 8 + 5, j ) = ( state->hd44102ch[1].ram[ad+j] & 0x20 ) ? 0 : 1;
			*BITMAP_ADDR16(bitmap, i * 8 + 6, j ) = ( state->hd44102ch[1].ram[ad+j] & 0x40 ) ? 0 : 1;
			*BITMAP_ADDR16(bitmap, i * 8 + 7, j ) = ( state->hd44102ch[1].ram[ad+j] & 0x80 ) ? 0 : 1;
		}
		ad += 0x40;
	}

	/* Handle HD44102CH #2 */
	ad = state->hd44102ch[2].start_page;
	for ( i = 0; i < 4; i++ )
	{
		for ( j = 0; j < 25; j++ )
		{
			*BITMAP_ADDR16(bitmap, i * 8 + 0, 50 + j ) = ( state->hd44102ch[2].ram[ad+j] & 0x01 ) ? 0 : 1;
			*BITMAP_ADDR16(bitmap, i * 8 + 1, 50 + j ) = ( state->hd44102ch[2].ram[ad+j] & 0x02 ) ? 0 : 1;
			*BITMAP_ADDR16(bitmap, i * 8 + 2, 50 + j ) = ( state->hd44102ch[2].ram[ad+j] & 0x04 ) ? 0 : 1;
			*BITMAP_ADDR16(bitmap, i * 8 + 3, 50 + j ) = ( state->hd44102ch[2].ram[ad+j] & 0x08 ) ? 0 : 1;
			*BITMAP_ADDR16(bitmap, i * 8 + 4, 50 + j ) = ( state->hd44102ch[2].ram[ad+j] & 0x10 ) ? 0 : 1;
			*BITMAP_ADDR16(bitmap, i * 8 + 5, 50 + j ) = ( state->hd44102ch[2].ram[ad+j] & 0x20 ) ? 0 : 1;
			*BITMAP_ADDR16(bitmap, i * 8 + 6, 50 + j ) = ( state->hd44102ch[2].ram[ad+j] & 0x40 ) ? 0 : 1;
			*BITMAP_ADDR16(bitmap, i * 8 + 7, 50 + j ) = ( state->hd44102ch[2].ram[ad+j] & 0x80 ) ? 0 : 1;
		}
		for ( j = 25; j < 50; j++ )
		{
			*BITMAP_ADDR16(bitmap, 32 + i * 8 + 0, 25 + j ) = ( state->hd44102ch[2].ram[ad+j] & 0x01 ) ? 0 : 1;
			*BITMAP_ADDR16(bitmap, 32 + i * 8 + 1, 25 + j ) = ( state->hd44102ch[2].ram[ad+j] & 0x02 ) ? 0 : 1;
			*BITMAP_ADDR16(bitmap, 32 + i * 8 + 2, 25 + j ) = ( state->hd44102ch[2].ram[ad+j] & 0x04 ) ? 0 : 1;
			*BITMAP_ADDR16(bitmap, 32 + i * 8 + 3, 25 + j ) = ( state->hd44102ch[2].ram[ad+j] & 0x08 ) ? 0 : 1;
			*BITMAP_ADDR16(bitmap, 32 + i * 8 + 4, 25 + j ) = ( state->hd44102ch[2].ram[ad+j] & 0x10 ) ? 0 : 1;
			*BITMAP_ADDR16(bitmap, 32 + i * 8 + 5, 25 + j ) = ( state->hd44102ch[2].ram[ad+j] & 0x20 ) ? 0 : 1;
			*BITMAP_ADDR16(bitmap, 32 + i * 8 + 6, 25 + j ) = ( state->hd44102ch[2].ram[ad+j] & 0x40 ) ? 0 : 1;
			*BITMAP_ADDR16(bitmap, 32 + i * 8 + 7, 25 + j ) = ( state->hd44102ch[2].ram[ad+j] & 0x80 ) ? 0 : 1;
		}
		ad += 0x40;
	}

	return 0;
}

/* This is called whenever the T0 pin switches state */
int gamepock_io_callback( device_t *device, int ioline, int state )
{
	device_t *speaker = device->machine->device("speaker");
	if ( ioline == UPD7810_TO )
	{
		speaker_level_w(speaker, state & 1 );
	}
	return 0;
}


