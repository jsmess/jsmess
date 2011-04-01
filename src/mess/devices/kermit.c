/* Kermit protocol implementation.

   Transfer between an emulated machine and an image using the kermit protocol.

   Used in the HP48 S/SX/G/GX emulation.

   Based on the 'KERMIT PROTOCOL MANUAL', Sixth Edition, by Frank da Cruz,
   Columbia University Center for Computing Activities, New York, June 1986.
   Available at: http://www-vs.informatik.uni-ulm.de/teach/ws05/rn1/Kermit%20Protocol.pdf

   Used in the HP48 S/SX/G/GX emulation.
   For HP48 specific kermit notes, see http://www.columbia.edu/kermit/hp48.html

   Only the basic protocol is implemented.
   The following features are NOT supported:
   - 8-bit quoting (QBIN)
   - 2- or 3-byte checksums (only 1-byte supported)
   - repeat count
   - file attribute packets
   - sliding window
   - extended length packets
   - server mode

   Author: Antoine Mine'
   Date: 29/03/2008
 */


#include "emu.h"
#include "kermit.h"


/* debugging */
#define VERBOSE          0

#define LOG(x)  do { if (VERBOSE) logerror x; } while (0)

/* protocol bytes */
#define KERMIT_SOH 0x01 /* (default) start of header */

/* packet types */
#define KERMIT_DATA 'D'  /* data packet */
#define KERMIT_ACK  'Y'  /* acknowledge */
#define KERMIT_NAK  'N'  /* negative acknowledge */
#define KERMIT_SEND 'S'  /* send initiate (exchange parameters) */
#define KERMIT_EOT  'B'  /* break transmission */
#define KERMIT_FILE 'F'  /* file header */
#define KERMIT_EOF  'Z'  /* end of file */
#define KERMIT_ERR  'E'  /* error */

#define KERMIT_QCTL  '#' /* default escape character */
#define KERMIT_EOL   15  /* default terminator: carriage-return  */
#define KERMIT_MAXL  80  /* default maximum packet length */

/* protocol state, to drive action */
#define KERMIT_IDLE       0  /* nothing to do */
#define KERMIT_RESET      1  /* reset after sending the last packet */
#define KERMIT_RECEIVE    2  /* receiving */
#define KERMIT_SEND_DATA  3  /* sending data */
#define KERMIT_SEND_EOF   4  /* sending EOF */
#define KERMIT_SEND_EOT   5  /* sendinf EOT */

/* packet retry parameters */
#define KERMIT_MAX_RETRIES  5
#define KERMIT_RETRY_DELAY  attotime::from_seconds( 10 )


/* packet format is:

   <mark> <len> <seq> <type> <data0> ... <datan> <check>

   - <mark>  KERMIT_SOH
   - <len>   number of bytes after <len> (max=94), plus 32
   - <seq>   sequence number, modulo 63, plus 32
   - <type>  one of KERMIT_ packet types
   - <data>  payload, with control characters escaped with KERMIT_QCTL
   - <check> checksum, from <len> to <datan>, excluding <mark>

   the packet may be followed by arbitrary (non KERMIT_SOH) data, not counted
   in <len>, <check>, and not interpreted
   there is generally at least one KERMIT_EOL
*/

typedef struct {

	UINT8   m_pin[1024];     /* packet received */
	UINT8   m_pout[1024];    /* packet sent */
	UINT32  m_seq;           /* sequence number, starting at 0 */
	UINT16  m_posin;         /* position in pin */
	UINT16  m_posout;        /* position in pout */
	UINT16  m_nbout;         /* size of pout */

	UINT8   m_state;         /* current protocol state */
	UINT8   m_retries;       /* packet retries remaining */

	/* (relevant) configuration information, sent in S and D packet */
	UINT8   m_maxl; /* max packet length (value of <len>) */
	UINT8   m_npad; /* number of padding characters before packet */
	UINT8   m_padc; /* padding character */
	UINT8   m_eol;  /* character to add after packet */
	UINT8   m_qctl; /* escape character */

	emu_timer* m_resend;           /* auto-resend packet */

	device_image_interface* m_image;  /* underlying image */

	running_machine *m_machine;
	kermit_config* m_conf;

} kermit;


INLINE kermit *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == KERMIT);

	return (kermit*)downcast<legacy_device_base *>(device)->token();
}

static int kermit_is_char( UINT8 data )
{
	return data >= 32 && data <= 126;
}


static  UINT8 kermit_tochar( UINT8 data )
{
	if ( data > 94 )
	{
		logerror( "kermit: tochar: %i not in the range 0-94\n", data );
	}
	return data + 32;
}

static UINT8 kermit_unchar( UINT8 data )
{
	if ( data < 32 || data > 126 )
	{
		logerror( "kermit: unchar: %i not in the range 32-126\n", data );
	}
	return data - 32;
}

#define kermit_ctl(x) ((x) ^ 0x40)

static int kermit_is_ctl( UINT8 x )
{
	x &= 0x7f;
	return (x < 32) || (x == 127);
}

static UINT8 kermit_checksum( UINT8* packet )
{
	int i, len = kermit_unchar( packet[1] );
	UINT8 sum = 0;
	for ( i = 0; i < len; i++ )
		sum += packet[ i + 1 ];
	sum = kermit_tochar( (sum + (sum >> 6)) & 63 );
	return sum;
}

/* starts sending a new packet */
static void kermit_start_sending( kermit* state )
{
	int i;

	for (i=1;i<state->m_nbout;i++)
		if (kermit_is_ctl(state->m_pout[i]))
			logerror( "send: not char %i at pos %i\n", state->m_pout[i], i  );

	if ( state->m_nbout <= 0 ) return;

	/* prepend padding and append eol */
	if ( state->m_npad > 0 )
	{
		memmove( state->m_pout + state->m_npad, state->m_pout, state->m_nbout );
		memset( state->m_pout, state->m_padc, state->m_npad );
		state->m_nbout += state->m_npad;
	}
	state->m_pout[ state->m_nbout ] = state->m_eol;
	state->m_nbout++;

	if ( state->m_conf && state->m_conf->send )
	{
		state->m_conf->send( *state->m_machine, state->m_pout[ 0 ] );
	}
	state->m_posout = 1;

	state->m_retries = KERMIT_MAX_RETRIES;
	state->m_resend->adjust( KERMIT_RETRY_DELAY, 0, KERMIT_RETRY_DELAY );
}

static void kermit_reset( kermit* state );

static void kermit_resend( kermit* state )
{
	if ( state->m_conf && state->m_conf->send )
	{
		/* retry */
		if ( state->m_nbout == 0 ) return;
		if ( state->m_retries <= 0 )
		{
			kermit_reset( state );
			return;
		}

		state->m_conf->send( *state->m_machine, state->m_pout[ 0 ] );

		state->m_posout = 1;
		state->m_retries--;
		LOG(( "kermit: resend packet (%i tries left)\n", state->m_retries ));
	}
}

/* resend packet periodically */
static TIMER_CALLBACK( kermit_resend_cb )
{
	kermit* state = (kermit*) ptr;
	if ( state->m_posout >= state->m_nbout)
	{
		kermit_resend( state );
	}
}

/* fetches image data and wrap it into a kermit data packet */
static void kermit_send_data_packet( kermit* state )
{
	int len;
	LOG(( "kermit: send data packet\n" ));
	for ( len = 2; len < state->m_maxl-2; len++ )
	{
		UINT8 c;
		if ( state->m_image->image_feof() ) break;
		c = state->m_image->fgetc();
		if ( kermit_is_ctl( c ) )
		{
			/* escape control char */
			state->m_pout[ 2 + len ] = state->m_qctl;
			len++;
			state->m_pout[ 2 + len ] = kermit_ctl( c );
		}
		else if ( (c & 0x7f) == state->m_qctl )
		{
			/* escape escape char */
			state->m_pout[ 2 + len ] = state->m_qctl;
			len++;
			state->m_pout[ 2 + len ] = c;
		}
		else
		{
			state->m_pout[ 2 + len ] = c;
		}
	}
	state->m_pout[ 0 ] = KERMIT_SOH;
	state->m_pout[ 1 ] = kermit_tochar( len + 1 );
	state->m_pout[ 2 ] = kermit_tochar( state->m_seq & 63 );
	state->m_pout[ 3 ] = KERMIT_DATA;
	state->m_pout[ 2 + len ] = kermit_checksum( state->m_pout );
	state->m_nbout = len + 3;
	kermit_start_sending( state );
}

/* write data from packet to the image */
static void kermit_write_data_packet( kermit* state )
{
	int i, len = kermit_unchar( state->m_pin[1] );
	LOG(( "kermit: received data pack, len=%i\n", len ));
	for ( i = 4; i <= len; i++ )
	{
		UINT8 c = state->m_pin[i];
		if ( (c /*& 0x7f*/) == state->m_qctl )
		{
			/* unescape */
			i++;
			c = state->m_pin[i];
			if ( kermit_is_ctl( kermit_ctl( c ) ) )
			{
				c = kermit_ctl( c );
			}
		}
		state->m_image->fwrite(&c, 1 );
	}
}

/* validate received packet and acquire seq number, <0 if error */
static int kermit_validate_packet( kermit* state )
{
	int len;
	if ( state->m_pin[ 0 ] != KERMIT_SOH ) return -1;
	if ( !kermit_is_char( state->m_pin[1] ) ) return -1;
	if ( !kermit_is_char( state->m_pin[2] ) ) return -1;
	len = kermit_unchar( state->m_pin[1] );
	if ( state->m_pin[ 1 + len ] != kermit_checksum( state->m_pin ) ) return -1;
	return 0;
}

/* interpret configuration packet, <0 if error */
static int kermit_get_conf( kermit* state )
{
	int len = kermit_unchar( state->m_pin[1] );
	if ( len >= 4 ) state->m_maxl = kermit_unchar( state->m_pin[ 4 ] );
	if ( len >= 6 ) state->m_npad = kermit_unchar( state->m_pin[ 6 ] );
	if ( len >= 7 ) state->m_padc = kermit_ctl( state->m_pin[ 7 ] );
	if ( len >= 8 ) state->m_eol  = kermit_unchar( state->m_pin[ 8 ] );
	if ( len >= 9 ) state->m_qctl = state->m_pin[ 9 ];

	LOG(( "kermit: get conf: len=%i, maxl=%i, npad=%i, padc=%i, eol=%i, qctl=%i\n",
	      len, state->m_maxl, state->m_npad, state->m_padc, state->m_eol, state->m_qctl ));

	/* validation */
	if ( state->m_maxl < 10 || state->m_maxl > 94 ) return -1;
	return 0;
}

/* create a configuration packet, with header h */
static void kermit_send_conf_packet( kermit* state, UINT8 h )
{
	LOG(( "kermit: send conf packet of type %c\n", h ));
	state->m_seq = 0;
	state->m_pout[  0 ] = KERMIT_SOH;
	state->m_pout[  1 ] = kermit_tochar( 13 );
	state->m_pout[  2 ] = kermit_tochar( state->m_seq & 63 );
	state->m_pout[  3 ] = h;
	state->m_pout[  4 ] = kermit_tochar( state->m_maxl );   /* maxl */
	state->m_pout[  5 ] = kermit_tochar( 1 );             /* time: 1 sec */
	state->m_pout[  6 ] = kermit_tochar( state->m_npad );   /* npad */
	state->m_pout[  7 ] = kermit_ctl( state->m_padc );      /* padc */
	state->m_pout[  8 ] = kermit_tochar( state->m_eol );    /* eol */
	state->m_pout[  9 ] = state->m_qctl;                    /* qtcl */
	state->m_pout[ 10 ] = 'N';                            /* qbin: no 8-bit quoting */
	state->m_pout[ 11 ] = '1';                            /* chkt: single character checksum */
	state->m_pout[ 12 ] = ' ';                            /* rept: no repeat count */
	state->m_pout[ 13 ] = kermit_tochar( 0 );             /* capas: no capabilities */
	state->m_pout[ 14 ] = kermit_checksum( state->m_pout );
	state->m_nbout = 15;
	kermit_start_sending( state );
}

/* sends a packet with no data */
static void kermit_send_simple_packet( kermit* state, UINT8 h  )
{
	LOG(( "kermit: send empty packet of type %c, seq=%i\n", h, state->m_seq & 63 ));
	state->m_pout[ 0 ] = KERMIT_SOH;
	state->m_pout[ 1 ] = kermit_tochar( 3 );
	state->m_pout[ 2 ] = kermit_tochar( state->m_seq & 63 );
	state->m_pout[ 3 ] = h;
	state->m_pout[ 4 ] = kermit_checksum( state->m_pout );
	state->m_nbout = 5;
	kermit_start_sending( state );
}

/* sends a packet with string data */
static void kermit_send_string_packet( kermit* state, UINT8 h, const char* data )
{
	int i, len;
	LOG(( "kermit: send string packet of type %c, data=%s, seq=%i\n", h, data, state->m_seq & 63 ));
	for ( len = i = 0; (len < state->m_maxl - 5) && data[i]; i++  )
	{
		char c = data[i];
		if ( kermit_is_char( c ) && ( c != '_' ) && ( c != ' ' ) )
		{
		  state->m_pout[ 4 + len ] = c;
		  len++;
		}
	}
	state->m_pout[ 0 ] = KERMIT_SOH;
	state->m_pout[ 1 ] = kermit_tochar( len + 3 );
	state->m_pout[ 2 ] = kermit_tochar( state->m_seq & 63 );
	state->m_pout[ 3 ] = h;
	state->m_pout[ 4 + len ] = kermit_checksum( state->m_pout );
	state->m_nbout = len + 5;
	kermit_start_sending( state );
}

/* extract the string contained in received packet */
static char* kermit_string_in_packet( kermit* state )
{
	int len = kermit_unchar( state->m_pin[1] );
	state->m_pin[ len + 1 ] = 0;
	return (char*) state->m_pin + 4;
}

static void kermit_reset( kermit* state )
{
	/* default config */
	state->m_maxl = KERMIT_MAXL;
	state->m_npad = 0;
	state->m_padc = 0;
	state->m_eol  = KERMIT_EOL;
	state->m_qctl = KERMIT_QCTL;

	state->m_retries = KERMIT_MAX_RETRIES;
	state->m_posin   = 0;
	state->m_posout  = 0;
	state->m_nbout   = 0;
	state->m_state   = KERMIT_IDLE;
	state->m_seq     = 0;

	if ( state->m_image )
	{
		state->m_image->fseek( SEEK_SET, 0 );
	}

	state->m_resend->adjust( attotime::never, 0, attotime::never );
}


/* emulated machine sends a byte to the outside (us) */
void kermit_receive_byte( device_t *device, UINT8 data )
{
	kermit* state = get_safe_token(device);

	LOG(( "get %i %2x %c (%i)\n", data, data, data, state->m_posin ));

	/* get SOH */
	if ( (data == KERMIT_SOH) && (state->m_posin <= 1) )
	{
		state->m_pin[ 0 ] = data;
		state->m_posin = 1;
	}

	/* get packet contents */
	else if ( state->m_posin > 0 )
	{
		if ( state->m_posin >= sizeof( state->m_pout ) )
		{
			LOG(( "kermit: too many bytes received\n" ));
			state->m_posin = 0;
			kermit_send_simple_packet( state, KERMIT_NAK );
			return;
		}

		if (kermit_is_ctl(data))
			logerror( "received: not char %i at pos %i\n", data, state->m_posin  );

		state->m_pin[ state->m_posin ] = data;
		state->m_posin++;

		if ( state->m_posin > 5 )
		{
			int len, seq, typ;
			if ( !kermit_is_char( state->m_pin[1] ) ||
			     (kermit_unchar( state->m_pin[1] ) < 3) ||
			     (kermit_unchar( state->m_pin[1] ) > state->m_maxl) )
			{
				LOG(( "kermit: invalid packet size %i-32, not in 3-%i\n", state->m_pin[1], state->m_maxl ));
				kermit_resend( state );
				return;
			}

			len = kermit_unchar( state->m_pin[1] );

			if ( state->m_posin >= len + 2 )
			{
				/* got packet! */
				state->m_posin = 0;
				if ( kermit_validate_packet( state ) )
				{
					LOG(( "kermit: invalid packet\n" ));
					kermit_resend( state );
					return;
				}

				typ = state->m_pin[3];
				seq = kermit_unchar( state->m_pin[2] );

				LOG(( "kermit: received packet type=%c, seq=%i, len=%i\n",  typ, seq, len ));

				if ( !state->m_image )
				{
					kermit_send_string_packet( state, KERMIT_ERR, "NO IMAGE" );
					return;
				}


				switch ( typ )
				{

				/* receiving */

				case KERMIT_SEND:
					kermit_get_conf( state );
					if ( state->m_maxl >= KERMIT_MAXL )
					{
						state->m_maxl = KERMIT_MAXL;
					}
					kermit_send_conf_packet( state, KERMIT_ACK );
					state->m_state = KERMIT_RECEIVE;
					break;

				case KERMIT_FILE:
					LOG(( "kermit: got file name '%s'\n", kermit_string_in_packet( state ) ));
					state->m_seq = seq;
					kermit_send_simple_packet( state, KERMIT_ACK );
					break;

				case KERMIT_DATA:
				case KERMIT_EOF:
					if ( seq == ((state->m_seq + 1) & 63) )
					{
						/* next packet */
						if ( typ == KERMIT_DATA )
						{
							kermit_write_data_packet( state );
						}
						state->m_seq = seq;
						kermit_send_simple_packet( state, KERMIT_ACK );
					}
					else if ( seq == (state->m_seq & 63) )
					{
						/* same packet */
						kermit_send_simple_packet( state, KERMIT_ACK );
					}
					else
					{
						/* unexpected sequence number */
						LOG(( "kermit: got seq=%i, expected %i\n", seq, (state->m_seq + 1) & 63 ));
						state->m_seq++;
						kermit_send_simple_packet( state, KERMIT_NAK );
						state->m_seq--;
					}
					break;

				case KERMIT_EOT:
					/* send ack and reset */
					state->m_seq = seq;
					kermit_send_simple_packet( state, KERMIT_ACK );
					state->m_state = KERMIT_RESET;
					break;


                                /* sending */

				case KERMIT_NAK:
				case KERMIT_ACK:

					if ( (typ == KERMIT_NAK) && (seq == 0) && (state->m_state == KERMIT_IDLE) )
					{
						/* transfer start */
						kermit_send_conf_packet( state, KERMIT_SEND );
						break;
					}

					if ( ((seq == (state->m_seq & 63)) && (typ == KERMIT_ACK)) ||
					     ((seq == ((state->m_seq + 1) & 63)) && (typ == KERMIT_NAK)) )
					{
						/* send next packet */

						state->m_seq = state->m_seq + 1;

						switch ( state->m_state )
						{
						case KERMIT_IDLE:
							/* get conf & send file name */
							kermit_get_conf( state );
							kermit_send_string_packet( state, KERMIT_FILE, state->m_image->basename() );
							state->m_state = KERMIT_SEND_DATA;;
							break;

						case KERMIT_SEND_DATA:
							/* send next data packet or EOF */
							if ( state->m_image->image_feof() )
							{
								kermit_send_simple_packet( state, KERMIT_EOF );
								state->m_state = KERMIT_SEND_EOF;
							}
							else
							{
								kermit_send_data_packet( state );
							}
							break;

						case KERMIT_SEND_EOF:
						        /* send EOT */
							kermit_send_simple_packet( state, KERMIT_EOT );
							state->m_state = KERMIT_SEND_EOT;
							break;

						case KERMIT_SEND_EOT:
							/* reset */
							kermit_reset( state );
							state->m_state = KERMIT_RESET;
							break;

						}
					}

					else if ( (seq == (state->m_seq & 63)) && (typ == KERMIT_NAK) )
					{
						/* resend last packet */
						kermit_resend( state );
					}
					else
					{
						/* bad */
						LOG(( "kermit: expected to send %i, got seq=%i\n", (state->m_seq + 1) & 63, seq ));
						kermit_reset( state );
					}
					break;

                                /* errors */

				case KERMIT_ERR:
					LOG(( "kermit: got error '%s'\n", kermit_string_in_packet( state ) ));
					state->m_seq = seq;
					kermit_reset( state );
					break;

				default:
					LOG(( "kermit: ignoring packet type %c\n", state->m_pin[ 3 ] ));
					/* no abort, just ignoring */
				}
			}
		}
	}
}

void kermit_byte_transmitted( device_t *device )
{
	kermit* state = get_safe_token(device);

	LOG(( "transmitted %i %2x %c, %i / %i\n", state->m_pout[ state->m_posout-1 ], state->m_pout[ state->m_posout-1 ], state->m_pout[ state->m_posout-1 ], state->m_posout - 1, state->m_nbout - 1 ));

	/* at end of packet */
	if ( state->m_posout >= state->m_nbout )
	{
		if ( state->m_state == KERMIT_RESET )
		{
			kermit_reset( state );
		}
		return;
	}

	if ( state->m_conf && state->m_conf->send )
	{
		state->m_conf->send(*state->m_machine, state->m_pout[ state->m_posout ] );
	}
	state->m_posout++;
}

static DEVICE_START( kermit )
{
	kermit* state = get_safe_token(device);
	LOG(( "kermit: start\n" ));
	state->m_image = NULL;
	state->m_conf = (kermit_config*) device->baseconfig().static_config();
	state->m_machine = &device->machine();
	state->m_resend = device->machine().scheduler().timer_alloc(FUNC(kermit_resend_cb), state );
	kermit_reset( state );
}

static DEVICE_RESET( kermit )
{
	kermit* state = get_safe_token(device);
	LOG(( "kermit: reset\n" ));
	kermit_reset( state );
}

static DEVICE_IMAGE_LOAD( kermit )
{
	kermit* state = get_safe_token(&image.device());
	LOG(( "kermit: image load\n" ));
	state->m_image = &image;
	kermit_reset( state );
	return IMAGE_INIT_PASS;
}

static DEVICE_IMAGE_CREATE( kermit )
{
	kermit* state = get_safe_token(&image.device());
	LOG(( "kermit: image create\n" ));
	state->m_image = &image;
	kermit_reset( state );
	return IMAGE_INIT_PASS;
}

static DEVICE_IMAGE_UNLOAD( kermit )
{
	kermit* state = get_safe_token(&image.device());
	LOG(( "kermit: image unload\n" ));
	state->m_image = NULL;
	kermit_reset( state );
}

DEVICE_GET_INFO( kermit )

{
	switch ( state ) {
	case DEVINFO_INT_TOKEN_BYTES:               info->i = sizeof( kermit );                              break;
	case DEVINFO_INT_INLINE_CONFIG_BYTES:       info->i = 0;                                             break;
	case DEVINFO_INT_IMAGE_TYPE:	            info->i = IO_SERIAL;                                     break;
	case DEVINFO_INT_IMAGE_READABLE:            info->i = 1;                                             break;
	case DEVINFO_INT_IMAGE_WRITEABLE:	    info->i = 1;                                             break;
	case DEVINFO_INT_IMAGE_CREATABLE:	    info->i = 1;                                             break;
	case DEVINFO_FCT_START:	                    info->start = DEVICE_START_NAME( kermit );               break;
	case DEVINFO_FCT_RESET:	                    info->reset = DEVICE_RESET_NAME( kermit );               break;
	case DEVINFO_FCT_IMAGE_LOAD:		    info->f = (genf *) DEVICE_IMAGE_LOAD_NAME( kermit );     break;
	case DEVINFO_FCT_IMAGE_UNLOAD:		    info->f = (genf *) DEVICE_IMAGE_UNLOAD_NAME( kermit );   break;
	case DEVINFO_FCT_IMAGE_CREATE:		    info->f = (genf *) DEVICE_IMAGE_CREATE_NAME( kermit );   break;
	case DEVINFO_STR_IMAGE_BRIEF_INSTANCE_NAME: strcpy(info->s, "k");	                                     break;
	case DEVINFO_STR_IMAGE_INSTANCE_NAME:
	case DEVINFO_STR_NAME:		            strcpy(info->s, "Kermit");	                                     break;
	case DEVINFO_STR_FAMILY:                    strcpy(info->s, "Serial protocol");	                     break;
	case DEVINFO_STR_SOURCE_FILE:		    strcpy(info->s, __FILE__);                                      break;
	case DEVINFO_STR_IMAGE_FILE_EXTENSIONS:	    strcpy(info->s, "");                                            break;
	}
}

DEFINE_LEGACY_IMAGE_DEVICE(KERMIT, kermit);
