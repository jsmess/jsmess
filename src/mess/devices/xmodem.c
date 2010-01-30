/* XMODEM protocol implementation.

   Transfer between an emulated machine and an image using the XMODEM protocol.

   Used in the HP48 G/GX emulation.

   Based on the "Xmodem protol specification" by Ward Christensen.

   Does not support any extension (such as 16-bit CRC, 1K packet size,
   batchs, YMODEM, ZMODEM).

   Author: Antoine Mine'
   Date: 29/03/2008
 */

#include "emu.h"
#include "xmodem.h"


/* debugging */
#define VERBOSE          0

#define LOG(x)  do { if (VERBOSE) logerror x; } while (0)


/* XMODEM protocol bytes */
#define XMODEM_SOH 0x01  /* data packet header */
#define XMODEM_EOT 0x04  /* end of transmission */
#define XMODEM_ACK 0x06  /* acknowledge (packet received) */
#define XMODEM_NAK 0x15  /* not-acknowledge (10-second timeout) */
#define XMODEM_CAN 0x18  /* cancel */

/* XMODEM protocol is:

   sender        receiver

            <--   <NAK>
   packet   -->
            <--   <ACK>
            ...
   packet   -->
            <--   <ACK>
   <EOT>    -->
            <--   <ACK>

   and packet is:  <SOH> <id> <0xff-id> <data0> ... <data127> <checksum>

   <id> is 1 for the first packet, 2 for the 2d, etc...
   <checksum> is the sum of <data0> to <data127> modulo 256

   there are always 128 bytes of data in a packet, so, any file transfered is
   rounded to a multiple of 128 bytes, using 0-filler
*/


/* our state */
#define XMODEM_NOIMAGE    0
#define XMODEM_IDLE       1
#define XMODEM_SENDING    2 /* image to emulated machine */
#define XMODEM_RECEIVING  3 /* emulated machine to image */

typedef struct {

	UINT8   block[132];          /* 3-byte header + 128-byte block + checksum */
	UINT32  id;                  /* block id, starting at 1 */
	UINT16  pos;                 /* position in block, including header */
	UINT8   state;               /* one of XMODEM_ */

	running_device* image;  /* underlying image */

	emu_timer* timer;            /* used to send periodic NAKs */

	running_machine *machine;

	xmodem_config* conf;

} xmodem;



/* compute checksum of the data part of the packet, i.e., sum modulo 256 */
static UINT8 xmodem_chksum( xmodem* state )
{
	UINT8 sum = 0;
	int i;
	for ( i = 0; i < 128; i++ ) sum += state->block[ i + 3 ];
	return sum;
}

/* fills state->block with the data for packet number state->id
   returns != 0 if fail or end of file
*/
static int xmodem_make_send_block( xmodem* state )
{
	if ( ! state->image ) return -1;
	memset( state->block + 3, 0, 128 );
	if ( image_fseek( state->image, (state->id - 1) * 128, SEEK_SET ) ) return -1;
	if ( image_fread( state->image, state->block + 3, 128 ) <= 0 ) return -1;
	state->block[0] = XMODEM_SOH;
	state->block[1] = state->id & 0xff;
	state->block[2] = 0xff - state->block[1];
	state->block[131] = xmodem_chksum( state );
	return 0;
}

/* checks the received state->block packet and stores the data in the image
   returns != 0 if fail (bad packet or bad image)
 */
static int xmodem_get_receive_block( xmodem* state )
{
	int next_id = state->id + 1;
	if ( ! state->image ) return -1;
	if ( ! image_is_writable( state->image ) ) return -1;
	if ( state->block[0] != XMODEM_SOH ) return -1;
	if ( state->block[1] != 0xff - state->block[2] ) return -1;
	if ( state->block[131] != xmodem_chksum( state ) ) return -1;
	if ( state->block[1] != (next_id & 0xff) ) return -1;
	if ( image_fseek( state->image, (next_id - 1) * 128, SEEK_SET ) ) return -1;
	if ( image_fwrite( state->image, state->block + 3, 128 ) != 128 ) return -1;
	return 0;
}

/* the outside (us) sends a byte to the emulated machine */
static void xmodem_send_byte( xmodem* state, UINT8 data )
{
	if ( state->conf && state->conf->send )
	{
		state->conf->send( state->machine, data );
	}
}

static void xmodem_send_packet_byte( xmodem* state )
{
	assert( state->pos < 132 );
	xmodem_send_byte( state, state->block[ state->pos ] );
	state->pos++;
}


static TIMER_CALLBACK( xmodem_nak_cb )
{
	xmodem* state = (xmodem*) ptr;
	if ( state->state != XMODEM_IDLE ) return;
	LOG(( "xmodem: sending NAK keep-alive\n" ));
	xmodem_send_byte( state, XMODEM_NAK );
}

static void xmodem_make_idle( xmodem* state )
{
	LOG(( "xmodem: entering idle state\n" ));
	state->state = XMODEM_IDLE;
	state->id = 0;
	state->pos = 0;
        /* When idle, we send NAK periodically to tell the emulated machine that we are
       always ready to receive.
       The 2 sec start time is here so that the machine does not get NAK instead of
       ACK or EOT as the last byte of a transfer.
     */
	timer_adjust_periodic( state->timer, ATTOTIME_IN_SEC( 2 ), 0, ATTOTIME_IN_SEC( 2 ) );
}

/* emulated machine has read the last byte we sent */
void xmodem_byte_transmitted( running_device *device )
{
	xmodem* state = (xmodem*) device->token;
	if ( (state->state == XMODEM_SENDING) && (state->pos < 132) )
	{
		/* send next byte */
		xmodem_send_packet_byte( state );
	}
}

/* emulated machine sends a byte to the outside (us) */
void xmodem_receive_byte( running_device *device, UINT8 data )
{
	xmodem* state = (xmodem*) device->token;
	switch ( state->state )
	{

	case XMODEM_NOIMAGE:
		break;

	case XMODEM_IDLE:
		if ( data == XMODEM_NAK )
		{
			/* start sending */
			LOG(( "xmodem: got NAK, start sending\n" ));
			state->id = 1;
			if ( xmodem_make_send_block( state ) )
			{
				/* error */
				LOG(( "xmodem: nothing to send, sending NAK\n" ));
				xmodem_send_byte( state, XMODEM_NAK );
				xmodem_make_idle( state );
			}
			else
			{
				/* send first packet */
				state->state = XMODEM_SENDING;
				state->pos = 0;
				xmodem_send_packet_byte( state );
			}
			break;
		}
		else if ( data == XMODEM_SOH )
		{
			/* start receiving */
			LOG(( "xmodem: got SOH, start receiving\n" ));
			state->state = XMODEM_RECEIVING;
			state->block[ 0 ] = data;
			state->pos = 1;
			state->id = 0;
		}
		else
		{
			LOG(( "xmodem: ignored data %02x while idle\n", data ));
		}
		break;

	case XMODEM_SENDING:
		if ( (data != XMODEM_NAK) && (data != XMODEM_ACK) )
		{
			/* error */
			LOG(( "xmodem: invalid date %02x while sending, sending CAN\n", data ));
			xmodem_send_byte( state, XMODEM_CAN );
			xmodem_make_idle( state );
			break;
		}
		if ( data == XMODEM_ACK )
		{
			/* send next packet */
			state->id++;
			LOG(( "xmodem: got ACK, sending next packet (%i)\n", state->id ));
			if ( xmodem_make_send_block( state ) )
			{
				/* end of file */
				LOG(( "xmodem: no more packet, sending EOT\n" ));
				xmodem_send_byte( state, XMODEM_EOT );
				xmodem_make_idle( state );
				break;
			}

		}
		else
		{
			/* empty - resend last packet */
			LOG(( "xmodem: got NAK, resending packet %i\n", state->id ));
		}
		state->pos = 0;
		xmodem_send_packet_byte( state );
		break;


	case XMODEM_RECEIVING:
		assert( state->pos < 132 );
		state->block[ state->pos ] = data;
		state->pos++;
		if ( state->pos == 1 )
		{
			/* header byte */
			if ( data == XMODEM_EOT )
			{
				/* end of file */
				LOG(( "xmodem: got EOT, stop receiving\n" ));
				xmodem_send_byte( state, XMODEM_ACK );
				xmodem_make_idle( state );
				break;
			}
		}
		else if ( state->pos == 132 )
		{
			LOG(( "xmodem: received packet %i\n", state->id ));
			/* end of block */
			if ( xmodem_get_receive_block( state ) )
			{
				/* error */
				LOG(( "xmodem: packet is invalid, sending NAK\n" ));
				xmodem_send_byte( state, XMODEM_NAK );
			}
			else
			{
				/* ok */
				LOG(( "xmodem: packet is valid, sending ACK\n" ));
				xmodem_send_byte( state, XMODEM_ACK );
				state->id++;
			}
			state->pos = 0;
		}
		break;

	}
}

static DEVICE_START( xmodem )
{
	xmodem* state = (xmodem*) device->token;
	LOG(( "xmodem: start\n" ));
	state->state = XMODEM_NOIMAGE;
	state->image = NULL;
	state->conf = (xmodem_config*) device->baseconfig().static_config;
	state->machine = device->machine;
	state->timer = timer_alloc(device->machine,  xmodem_nak_cb, state );
}

static DEVICE_RESET( xmodem )
{
	xmodem* state = (xmodem*) device->token;
	LOG(( "xmodem: reset\n" ));
	if ( state->state != XMODEM_NOIMAGE ) xmodem_make_idle( state );
}

static DEVICE_IMAGE_LOAD( xmodem )
{
	xmodem* state = (xmodem*) image->token;
	LOG(( "xmodem: image load\n" ));
	state->image = image;
	xmodem_make_idle( state );
	return INIT_PASS;
}

static DEVICE_IMAGE_CREATE( xmodem )
{
	xmodem* state = (xmodem*) image->token;
	LOG(( "xmodem: image create\n" ));
	state->image = image;
	xmodem_make_idle( state );
	return INIT_PASS;
}

static DEVICE_IMAGE_UNLOAD( xmodem )
{
	xmodem* state = (xmodem*) image->token;
	LOG(( "xmodem: image unload\n" ));
	state->state = XMODEM_NOIMAGE;
	state->image = NULL;
}

DEVICE_GET_INFO( xmodem )

{
	switch ( state ) {
	case DEVINFO_INT_TOKEN_BYTES:               info->i = sizeof( xmodem );                              break;
	case DEVINFO_INT_INLINE_CONFIG_BYTES:       info->i = 0;                                             break;
	case DEVINFO_INT_CLASS:	                    info->i = DEVICE_CLASS_PERIPHERAL;                       break;
	case DEVINFO_INT_IMAGE_TYPE:	            info->i = IO_SERIAL;                                     break;
	case DEVINFO_INT_IMAGE_READABLE:            info->i = 1;                                             break;
	case DEVINFO_INT_IMAGE_WRITEABLE:	    info->i = 1;                                             break;
	case DEVINFO_INT_IMAGE_CREATABLE:	    info->i = 1;                                             break;
	case DEVINFO_FCT_START:	                    info->start = DEVICE_START_NAME( xmodem );               break;
	case DEVINFO_FCT_RESET:	                    info->reset = DEVICE_RESET_NAME( xmodem );               break;
	case DEVINFO_FCT_IMAGE_LOAD:		    info->f = (genf *) DEVICE_IMAGE_LOAD_NAME( xmodem );     break;
	case DEVINFO_FCT_IMAGE_UNLOAD:		    info->f = (genf *) DEVICE_IMAGE_UNLOAD_NAME( xmodem );   break;
	case DEVINFO_FCT_IMAGE_CREATE:		    info->f = (genf *) DEVICE_IMAGE_CREATE_NAME( xmodem );   break;
	case DEVINFO_STR_IMAGE_BRIEF_INSTANCE_NAME: strcpy(info->s, "x");	                                     break;
	case DEVINFO_STR_IMAGE_INSTANCE_NAME:
	case DEVINFO_STR_NAME:		            strcpy(info->s, "xmodem");	                                     break;
	case DEVINFO_STR_FAMILY:                    strcpy(info->s, "serial protocol");	                     break;
	case DEVINFO_STR_SOURCE_FILE:		    strcpy(info->s, __FILE__);                                      break;
	case DEVINFO_STR_IMAGE_FILE_EXTENSIONS:	    strcpy(info->s, "");                                            break;
	}
}

