/*
    TI99/4(a) RS232 card emulation

    This card features two RS232C ports (which share one single connector) and
    one centronics-like fully bi-directional parallel port.  Each TI99 system
    can accomodate two such RS232 cards (provided a resistor is moved in one of
    the cards to change its CRU base address from >1300 to >1500), for a total
    of four RS232 ports and two parallel ports.

    The card is fairly simple: two 9902 chips handle one serial port each, two
    unidirectional TTL buffers handle the PIO data bus, there is extra TTL for
    address decoding and CRU interface, and a DSR ROM.

    References:
    <http://www.nouspikel.com/ti99/rs232c.htm>: great description of the
        hardware and software aspects.
    <ftp://ftp.whtech.com//datasheets/TMS9902A.max>: tms9902a datasheet
    <ftp://ftp.whtech.com//datasheets/Hardware manuals/ti rs232 card schematic.max>

    Raphael Nabet, 2003.
*/

#include "emu.h"
#include "ti99_4x.h"
#include "devices/ti99_peb.h"
#include "tms9902.h"
#include "994x_ser.h"

#include "machine/tms9902.h"

/* prototypes */
static int rs232_cru_r(running_machine *machine, int offset);
static void rs232_cru_w(running_machine *machine, int offset, int data);
static READ8_HANDLER(rs232_mem_r);
static WRITE8_HANDLER(rs232_mem_w);

/* pointer to the rs232 ROM data */
static UINT8 *rs232_DSR;

typedef struct _pio_t pio_t;
struct _pio_t
{
	int pio_direction;	// a.k.a. PIOOC pio in output mode if 0
	int pio_handshakeout;
	int pio_handshakein;
	int pio_spareout;
	int pio_sparein;
	int flag0;	// spare
	int cts1;	// a.k.a. flag1
	int cts2;	// a.k.a. flag2
	int led;		// a.k.a. flag3
	int pio_out_buffer;
	int pio_in_buffer;
	int pio_readable;
	int pio_writable;
	int pio_write;  	// 1 if image is to be written to
};

typedef struct _card_t card_t;
struct _card_t
{
	int i;
};

INLINE pio_t *get_safe_token_pio(running_device *device)
{
	assert(device != NULL);
	assert(device->type() == TI99_4_PIO);

	return (pio_t *)downcast<legacy_device_base *>(device)->token();
}

static const ti99_peb_card_handlers_t rs232_handlers =
{
	rs232_cru_r,
	rs232_cru_w,
	rs232_mem_r,
	rs232_mem_w
};

/*
    Initialize pio unit and open image
*/
static DEVICE_IMAGE_LOAD( ti99_4_pio )
{
	pio_t *pio = get_safe_token_pio(image);
	/* tell whether the image is writable */
	pio->pio_readable = ! image.has_been_created();
	/* tell whether the image is writable */
	pio->pio_writable = image.is_writable();

	if (pio->pio_write && pio->pio_writable)
		pio->pio_handshakein = 0;	/* receiver ready */
	else
		pio->pio_handshakein = 1;

	return IMAGE_INIT_PASS;
}

/*
    close a pio image
*/
static DEVICE_IMAGE_UNLOAD( ti99_4_pio )
{
	pio_t *pio = get_safe_token_pio(image);
	pio->pio_writable = 0;
	pio->pio_handshakein = 1;	/* receiver not ready */
	pio->pio_sparein = 0;
}

static DEVICE_START( ti99_4_pio )
{
}

static DEVICE_RESET( ti99_4_pio )
{
	pio_t *pio = get_safe_token_pio(device);
	pio->pio_direction = 0;
	pio->pio_handshakeout = 0;
	pio->pio_spareout = 0;
	pio->flag0 = 0;
	pio->cts1 = 0;
	pio->cts2 = 0;
	pio->led = 0;
	pio->pio_write = 1;
}
/*
    Initialize rs232 unit and open image
*/
static DEVICE_IMAGE_LOAD( ti99_4_rs232 )
{
	running_device *tms9902 = NULL;

	if (strcmp(image.device().tag(), "rs232:port0")) {
		tms9902 = image.device().machine->device("rs232:tms9902_0");
	}
	if (strcmp(image.device().tag(), "rs232:port1")) {
		tms9902 = image.device().machine->device("rs232:tms9902_1");
	}

	if (image)
	{
		tms9902_set_cts(tms9902, 1);
		tms9902_set_dsr(tms9902, 1);
	}
	else
	{
		tms9902_set_cts(tms9902, 0);
		tms9902_set_dsr(tms9902, 0);
	}

	return IMAGE_INIT_PASS;
}

/*
    close a rs232 image
*/
static DEVICE_IMAGE_UNLOAD( ti99_4_rs232 )
{
}

/*
    Initializes rs232 card, set up handlers
*/
static DEVICE_START( ti99_4_rs232 )
{
	rs232_DSR = memory_region(device->machine, region_dsr) + offset_rs232_dsr;
}

/*
    Reset rs232 card, set up handlers
*/
static DEVICE_RESET( ti99_4_rs232 )
{
	rs232_DSR = memory_region(device->machine, region_dsr) + offset_rs232_dsr;
	running_device *box = device->machine->device("per_exp_box");
	ti99_peb_set_card_handlers(box, 0x1300, & rs232_handlers);
}

static READ8_DEVICE_HANDLER(ti99_4_pio_r)
{
	pio_t *pio = get_safe_token_pio(device);
	UINT8 reply = 0x00;
	if (pio->pio_direction)
		reply |= 0x02;
	if (pio->pio_handshakein)
		reply |= 0x04;
	if (pio->pio_sparein)
		reply |= 0x08;
	if (pio->flag0)
		reply |= 0x10;
	if (pio->cts1)
		reply |= 0x20;
	if (pio->cts2)
		reply |= 0x40;
	if (pio->led)
		reply |= 0x80;
	return reply;
}

static READ8_DEVICE_HANDLER(ti99_4_pio_buf_r)
{
	pio_t *pio = get_safe_token_pio(device);
	return pio->pio_direction ? pio->pio_in_buffer : pio->pio_out_buffer;
}

static WRITE8_DEVICE_HANDLER(ti99_4_pio_buf_w)
{
	pio_t *pio = get_safe_token_pio(device);
	pio->pio_out_buffer = data;
}

static WRITE8_DEVICE_HANDLER(ti99_4_pio_w)
{
	pio_t *pio = get_safe_token_pio(device);
	device_image_interface *image = dynamic_cast<device_image_interface *>(device);
	switch (offset & 7)
	{
		case 0:
			/* WRITE to rs232 card enable bit (bit 0) */
			/* handled in xxx_peb_cru_w() */
			break;

		case 1:
			pio->pio_direction = data;
			break;

		case 2:
			if (data != pio->pio_handshakeout)
			{
				pio->pio_handshakeout = data;
				if (pio->pio_write && pio->pio_writable && (! pio->pio_direction))
				{	/* PIO in output mode */
					if (!pio->pio_handshakeout)
					{	/* write data strobe */
						/* write data and acknowledge */
						UINT8 buf = pio->pio_out_buffer;
						if (image->fwrite(&buf, 1))
							pio->pio_handshakein = 1;
					}
					else
					{
						/* end strobe */
						/* we can write some data: set receiver ready */
						pio->pio_handshakein = 0;
					}
				}
				if ((!pio->pio_write) && pio->pio_readable /*&& pio_direction*/)
				{	/* PIO in input mode */
					if (!pio->pio_handshakeout)
					{	/* receiver ready */
						/* send data and strobe */
						UINT8 buf;
						if (image->fread(&buf, 1))
							pio->pio_in_buffer = buf;
						pio->pio_handshakein = 0;
					}
					else
					{
						/* data acknowledge */
						/* we can send some data: set transmitter ready */
						pio->pio_handshakein = 1;
					}
				}
			}
			break;

		case 3:
			pio->pio_spareout = data;
			break;

		case 4:
			pio->flag0 = data;
			break;

		case 5:
			pio->cts1 = data;
			break;

		case 6:
			pio->cts2 = data;
			break;

		case 7:
			pio->led = data;
			break;
	}
}
/*
    Read rs232 card CRU interface

    bit 0: always 0
    ...
*/
static int rs232_cru_r(running_machine *machine, int offset)
{
	int reply;

	switch (offset >> 2)
	{
	case 0:
		/* custom buffer */
		switch (offset)
		{
		case 0:
			reply = ti99_4_pio_r(machine->device("rs232:pio"), offset);
			break;

		default:
			reply = 0;
			break;
		}
		break;

	case 1:
		/* first 9902 */
		reply = tms9902_cru_r(machine->device("rs232:tms9902_0"), offset);
		break;

	case 2:
		/* second 9902 */
		reply = tms9902_cru_r(machine->device("rs232:tms9902_1"), offset);
		break;

	default:
		/* NOP */
		reply = 0;	/* ??? */
		break;
	}

	return reply;
}

/*
    Write rs232 card CRU interface
*/
static void rs232_cru_w(running_machine *machine, int offset, int data)
{
	switch (offset >> 5)
	{
	case 0:
		/* custom buffer */
		ti99_4_pio_w(machine->device("rs232:pio"), offset, data);
		break;

	case 1:
		/* first 9902 */
		tms9902_cru_w(machine->device("rs232:tms9902_0"), offset, data);
		break;

	case 2:
		/* second 9902 */
		tms9902_cru_w(machine->device("rs232:tms9902_1"), offset, data);
		break;

	default:
		/* NOP */
		break;
	}
}


/*
    read a byte in rs232 DSR space
*/
static  READ8_HANDLER(rs232_mem_r)
{
	int reply;

	if (offset < 0x1000)
	{
		/* DSR ROM */
		reply = rs232_DSR[offset];
	}
	else
	{
		/* PIO */
		reply = ti99_4_pio_buf_r(space->machine->device("rs232:pio"), offset);
	}

	return reply;
}

/*
    write a byte in rs232 DSR space
*/
static WRITE8_HANDLER(rs232_mem_w)
{
	if (offset >= 0x1000)
	{
		/* PIO */
		ti99_4_pio_buf_w(space->machine->device("rs232:pio"), offset, data);
	}
	else
	{
		/* DSR ROM: do nothing */
	}
}

/*
    rs232 interrupt handler
*/
static TMS9902_INT_CALLBACK( int_callback_0 )
{
	running_device *box = device->machine->device("per_exp_box");
	ti99_peb_set_ila_bit(box, inta_rs232_1_bit, INT);
}
static TMS9902_INT_CALLBACK( int_callback_1 )
{
	running_device *box = device->machine->device("per_exp_box");
	ti99_peb_set_ila_bit(box, inta_rs232_2_bit, INT);
}

static TMS9902_XMIT_CALLBACK( xmit_callback_0 )
{
	UINT8 buf = data;
	device_image_interface *rs232_fp = dynamic_cast<device_image_interface *>(device->machine->device("rs232:port0"));

	if (rs232_fp)
		rs232_fp->fwrite(&buf, 1);
}

static TMS9902_XMIT_CALLBACK( xmit_callback_1 )
{
	UINT8 buf = data;
	device_image_interface *rs232_fp = dynamic_cast<device_image_interface *>(device->machine->device("rs232:port1"));

	if (rs232_fp)
		rs232_fp->fwrite(&buf, 1);
}
static const tms9902_interface tms9902_params_0 =
{
	3000000.,				/* clock rate (3MHz) */
	int_callback_0,			/* called when interrupt pin state changes */
	NULL,//rts_callback,            /* called when Request To Send pin state changes */
	NULL,//brk_callback,            /* called when BReaK state changes */
	xmit_callback_0			/* called when a character is transmitted */
};

static const tms9902_interface tms9902_params_1 =
{
	3000000.,				/* clock rate (3MHz) */
	int_callback_1,			/* called when interrupt pin state changes */
	NULL,//rts_callback,            /* called when Request To Send pin state changes */
	NULL,//brk_callback,            /* called when BReaK state changes */
	xmit_callback_1			/* called when a character is transmitted */
};

DEVICE_GET_INFO( ti99_4_rs232 )
{
	switch ( state )
	{
		case DEVINFO_INT_TOKEN_BYTES:				info->i = sizeof(card_t);							break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:		info->i = 0;								break;
		case DEVINFO_INT_IMAGE_TYPE:	            info->i = IO_SERIAL;                                      break;
		case DEVINFO_INT_IMAGE_READABLE:            info->i = 1;                                               break;
		case DEVINFO_INT_IMAGE_WRITEABLE:			info->i = 1;                                               break;
		case DEVINFO_INT_IMAGE_CREATABLE:	    	info->i = 1;                                               break;

		case DEVINFO_FCT_START:		                info->start = DEVICE_START_NAME( ti99_4_rs232 );              break;
		case DEVINFO_FCT_RESET:						info->reset = DEVICE_RESET_NAME( ti99_4_rs232 );			break;
		case DEVINFO_FCT_IMAGE_LOAD:		        info->f = (genf *) DEVICE_IMAGE_LOAD_NAME( ti99_4_rs232 );    break;
		case DEVINFO_FCT_IMAGE_UNLOAD:		        info->f = (genf *) DEVICE_IMAGE_UNLOAD_NAME(ti99_4_rs232 );  break;
		case DEVINFO_STR_NAME:		                strcpy( info->s, "TI99 RS232 port");	                         break;
		case DEVINFO_STR_FAMILY:                    strcpy(info->s, "TI99 RS232 port");	                         break;
		case DEVINFO_STR_IMAGE_FILE_EXTENSIONS:	    strcpy(info->s, "");                                           break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 		break;
	}
}

DECLARE_LEGACY_IMAGE_DEVICE(TI99_4_RS232, ti99_4_rs232);
DEFINE_LEGACY_IMAGE_DEVICE(TI99_4_RS232, ti99_4_rs232);

#define MDRV_TI99_4_RS232_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, TI99_4_RS232, 0)


DEVICE_GET_INFO( ti99_4_pio )
{
	switch ( state )
	{
		case DEVINFO_INT_TOKEN_BYTES:				info->i = sizeof(pio_t);							break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:		info->i = 0;								break;
		case DEVINFO_INT_IMAGE_TYPE:	            info->i = IO_PARALLEL;                                      break;
		case DEVINFO_INT_IMAGE_READABLE:            info->i = 1;                                               break;
		case DEVINFO_INT_IMAGE_WRITEABLE:			info->i = 1;                                               break;
		case DEVINFO_INT_IMAGE_CREATABLE:	    	info->i = 1;                                               break;

		case DEVINFO_FCT_START:		                info->start = DEVICE_START_NAME( ti99_4_pio );              break;
		case DEVINFO_FCT_RESET:						info->reset = DEVICE_RESET_NAME( ti99_4_pio );			break;
		case DEVINFO_FCT_IMAGE_LOAD:		        info->f = (genf *) DEVICE_IMAGE_LOAD_NAME( ti99_4_pio );    break;
		case DEVINFO_FCT_IMAGE_UNLOAD:		        info->f = (genf *) DEVICE_IMAGE_UNLOAD_NAME(ti99_4_pio );  break;
		case DEVINFO_STR_NAME:		                strcpy( info->s, "TI99 PIO port");	                         break;
		case DEVINFO_STR_FAMILY:                    strcpy(info->s, "TI99 PIO port");	                         break;
		case DEVINFO_STR_IMAGE_FILE_EXTENSIONS:	    strcpy(info->s, "");                                           break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 		break;
	}
}


DEFINE_LEGACY_IMAGE_DEVICE(TI99_4_PIO, ti99_4_pio);

static MACHINE_CONFIG_FRAGMENT( ti99_4_rs232 )
	MDRV_TMS9902_ADD("tms9902_0", tms9902_params_0)
	MDRV_TMS9902_ADD("tms9902_1", tms9902_params_1)
	MDRV_TI99_4_RS232_ADD("port0")
	MDRV_TI99_4_RS232_ADD("port1")
	MDRV_TI99_4_PIO_ADD("pio")
MACHINE_CONFIG_END

static DEVICE_START( ti99_4_rs232_card )
{
}

static DEVICE_RESET( ti99_4_rs232_card )
{
}

DEVICE_GET_INFO( ti99_4_rs232_card )
{
	switch ( state )
	{
		case DEVINFO_INT_TOKEN_BYTES:				info->i = sizeof(card_t);							break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:		info->i = 0;								break;
		case DEVINFO_FCT_START:		                info->start = DEVICE_START_NAME( ti99_4_rs232_card );              break;
		case DEVINFO_FCT_RESET:						info->reset = DEVICE_RESET_NAME( ti99_4_rs232_card );			break;

		case DEVINFO_PTR_MACHINE_CONFIG:			info->machine_config = MACHINE_CONFIG_NAME(ti99_4_rs232);		break;
		case DEVINFO_STR_NAME:		                strcpy( info->s, "TI99 RS232 card");	                         break;
		case DEVINFO_STR_FAMILY:                    strcpy(info->s, "TI99 RS232 card");	                         break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 		break;
	}
}

DEFINE_LEGACY_DEVICE(TI99_4_RS232_CARD, ti99_4_rs232_card);
