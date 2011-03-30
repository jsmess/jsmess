/*
    TI-99 RS232 and Parallel interface card
    Currently this emulation does not interact with the serial interface
    on the host computer. Planned for a later release.
    As most serial applications require a feedback from the receiver's side
    the serial interface implementation will usually *not* work.

    TI99 RS232 card ('rs232')
    TMS9902 ('rs232:tms9902_0')
    TMS9902 ('rs232:tms9902_1')
    TI99 RS232 attached serial device ('rs232:serdev0')
    TI99 RS232 attached serial device ('rs232:serdev1')
    TI99 PIO attached parallel device ('rs232:piodev')

*/
#include "emu.h"
#include "peribox.h"
#include "machine/tms9902.h"
#include "ti_rs232.h"

#define CRU_BASE 0x1300
#define SENILA_0_BIT 0x80
#define SENILA_1_BIT 0x40

#define ser_region "ser_region"

// Second card: Change CRU base to 0x1500
// #define CRU_BASE 0x1500

typedef ti99_pebcard_config ti_rs232_config;

typedef struct _ti_rs232_state
{
	device_t *uart0, *uart1, *serdev0, *serdev1, *piodev;

	int 	selected;
	int		select_mask;
	int		select_value;

	// PIO flags
	int pio_direction;		// a.k.a. PIOOC pio in output mode if 0
	int pio_handshakeout;
	int pio_handshakein;
	int pio_spareout;
	int pio_sparein;
	int flag0;				// spare
	int cts1;				// a.k.a. flag1
	int cts2;				// a.k.a. flag2
	int led;				// a.k.a. flag3
	int pio_out_buffer;
	int pio_in_buffer;
	int pio_readable;
	int pio_writable;
	int pio_write;			// 1 if image is to be written to

	UINT8	*rom;

	/* Keeps the state of the SENILA line. */
	UINT8	senila;

	/* Keeps the value put on the bus when SENILA becomes active. */
	UINT8	ila;

	/* Callback lines to the main system. */
	ti99_peb_connect	lines;

} ti_rs232_state;

INLINE ti_rs232_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == TIRS232);

	return (ti_rs232_state *)downcast<legacy_device_base *>(device)->token();
}

/**************************************************************************/
/* Ports */

static DEVICE_START( ti99_rs232_serdev )
{
}

static DEVICE_START( ti99_piodev )
{
}

/*
    Find the index of the image name. We assume the format
    <name><number>, i.e. the number is the longest string from the right
    which can be interpreted as a number.
*/
static int get_index_from_tagname(device_t *image)
{
	const char *tag = image->tag();
	int maxlen = strlen(tag);
	int i;
	for (i=maxlen-1; i >=0; i--)
		if (tag[i] < 48 || tag[i] > 57) break;

	return atoi(tag+i+1);
}

/*
    Initialize rs232 unit and open image
*/
static DEVICE_IMAGE_LOAD( ti99_rs232_serdev )
{
	device_t *tms9902 = NULL;

	// TODO: strcmp does not work this way (wrong constant or wrong function)
	int devnumber = get_index_from_tagname(&image.device());
	if (devnumber==0)
	{
		tms9902 = image.device().siblingdevice("tms9902_0");
	}
	else if (devnumber==1)
	{
		tms9902 = image.device().siblingdevice("tms9902_1");
	}
	else
	{
		logerror("ti99/rs232: Could not find device tag number\n");
		return IMAGE_INIT_FAIL;
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
    Initialize pio unit and open image
*/
static DEVICE_IMAGE_LOAD( ti99_piodev )
{
	device_t *carddev = image.device().owner();
	ti_rs232_state *card = get_safe_token(carddev);

	/* tell whether the image is writable */
	card->pio_readable = !image.has_been_created();
	/* tell whether the image is writable */
	card->pio_writable = image.is_writable();

	if (card->pio_write && card->pio_writable)
		card->pio_handshakein = 0;	/* receiver ready */
	else
		card->pio_handshakein = 1;

	return IMAGE_INIT_PASS;
}

/*
    close a pio image
*/
static DEVICE_IMAGE_UNLOAD( ti99_piodev )
{
	device_t *carddev = image.device().owner();
	ti_rs232_state *card = get_safe_token(carddev);

	card->pio_writable = 0;
	card->pio_handshakein = 1;	/* receiver not ready */
	card->pio_sparein = 0;
}

/****************************************************************************/

/*
    CRU read
*/
static READ8Z_DEVICE_HANDLER( cru_rz )
{
	ti_rs232_state *card = get_safe_token(device);

	if ((offset & 0xff00)==CRU_BASE)
	{
		if ((offset & 0x00c0)==0x0000)
		{
			UINT8 reply = 0x00;
			if (card->pio_direction)
				reply |= 0x02;
			if (card->pio_handshakein)
				reply |= 0x04;
			if (card->pio_sparein)
				reply |= 0x08;
			if (card->flag0)
				reply |= 0x10;
			if (card->cts1)
				reply |= 0x20;
			if (card->cts2)
				reply |= 0x40;
			if (card->led)
				reply |= 0x80;
			*value = reply;
			return;
		}
		if ((offset & 0x00c0)==0x0040)
		{
			*value = tms9902_cru_r(card->uart0, offset>>4);
			return;
		}
		if ((offset & 0x00c0)==0x0080)
		{
			*value = tms9902_cru_r(card->uart1, offset>>1);
			return;
		}
	}
}

/*
    CRU write
*/
static WRITE8_DEVICE_HANDLER( cru_w )
{
	ti_rs232_state *card = get_safe_token(device);

	if ((offset & 0xff00)==CRU_BASE)
	{
		if ((offset & 0x00c0)==0x0040)
		{
			tms9902_cru_w(card->uart0, offset>>1, data);
			return;
		}
		if ((offset & 0x00c0)==0x0080)
		{
			tms9902_cru_w(card->uart1, offset, data);
			return;
		}

		device_image_interface *image = dynamic_cast<device_image_interface *>(card->piodev);
		int bit = (offset & 0x00ff)>>1;
		switch (bit)
		{
		case 0:
			card->selected = data;
			break;

		case 1:
			card->pio_direction = data;
			break;

		case 2:
			if (data != card->pio_handshakeout)
			{
				card->pio_handshakeout = data;
				if (card->pio_write && card->pio_writable && (!card->pio_direction))
				{	/* PIO in output mode */
					if (!card->pio_handshakeout)
					{	/* write data strobe */
						/* write data and acknowledge */
						UINT8 buf = card->pio_out_buffer;
						int ret = image->fwrite(&buf, 1);
						if (ret)
							card->pio_handshakein = 1;
					}
					else
					{
						/* end strobe */
						/* we can write some data: set receiver ready */
						card->pio_handshakein = 0;
					}
				}
				if ((!card->pio_write) && card->pio_readable /*&& pio_direction*/)
				{	/* PIO in input mode */
					if (!card->pio_handshakeout)
					{	/* receiver ready */
						/* send data and strobe */
						UINT8 buf;
						if (image->fread(&buf, 1))
							card->pio_in_buffer = buf;
						card->pio_handshakein = 0;
					}
					else
					{
						/* data acknowledge */
						/* we can send some data: set transmitter ready */
						card->pio_handshakein = 1;
					}
				}
			}
			break;

		case 3:
			card->pio_spareout = data;
			break;

		case 4:
			card->flag0 = data;
			break;

		case 5:
			card->cts1 = data;
			break;

		case 6:
			card->cts2 = data;
			break;

		case 7:
			card->led = data;
			break;
		}
		return;
	}
}

/*
    Memory read
*/
static READ8Z_DEVICE_HANDLER( data_rz )
{
	ti_rs232_state *card = get_safe_token(device);

	if (card->senila)
	{
		logerror("ti99/rs232: Sensing ILA\n");
		*value = card->ila;
		// hope that the card rom was *not* selected, or we get two values
		// on the address bus

		// Not sure whether this is right. Unfortunately, this is difficult
		// to verify since the system does not make use of it, although the
		// lines are there.
		card->ila = 0;
	}
	if (((offset & card->select_mask)==card->select_value) && card->selected)
	{
		if ((offset & 0x1000)==0x0000)
		{
			*value = card->rom[offset&0x0fff];
		}
		else
		{
			*value = card->pio_direction ? card->pio_in_buffer : card->pio_out_buffer;
		}
	}
}

/*
    Memory write
*/
static WRITE8_DEVICE_HANDLER( data_w )
{
	ti_rs232_state *card = get_safe_token(device);

	if (((offset & card->select_mask)==card->select_value) && card->selected)
	{
		if ((offset & 0x1001)==0x1000)
		{
			card->pio_out_buffer = data;
		}
	}
}

/**************************************************************************/

/*
    rs232 interrupt handler
*/
static TMS9902_INT_CALLBACK( int_callback_0 )
{
	device_t *carddev = device->owner();
	ti_rs232_state *card = get_safe_token(carddev);
	if (INT)
		card->ila |= SENILA_0_BIT;
	else
		card->ila &= ~SENILA_0_BIT;

	devcb_call_write_line(&card->lines.inta, INT);
}

static TMS9902_INT_CALLBACK( int_callback_1 )
{
	device_t *carddev = device->owner();
	ti_rs232_state *card = get_safe_token(carddev);
	if (INT)
		card->ila |= SENILA_1_BIT;
	else
		card->ila &= ~SENILA_1_BIT;

	devcb_call_write_line(&card->lines.inta, INT);
}

static TMS9902_XMIT_CALLBACK( xmit_callback_0 )
{
	UINT8 buf = data;
	device_image_interface *rs232_fp = dynamic_cast<device_image_interface *>(device->siblingdevice("serdev0"));

	if (rs232_fp)
		rs232_fp->fwrite(&buf, 1);
	else
		logerror("ti99/rs232: serdev0 not found\n");
}

static TMS9902_XMIT_CALLBACK( xmit_callback_1 )
{
	UINT8 buf = data;
	device_image_interface *rs232_fp = dynamic_cast<device_image_interface *>(device->siblingdevice("serdev1"));

	if (rs232_fp)
		rs232_fp->fwrite(&buf, 1);
	else
		logerror("ti99/rs232: serdev1 not found\n");
}

static WRITE_LINE_DEVICE_HANDLER( senila )
{
	// put the value on the data bus. We store it in a state variable.
	ti_rs232_state *card = get_safe_token(device);
	card->senila = state;
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


static const ti99_peb_card tirs232_card =
{
	data_rz, data_w,				// memory access read/write
	cru_rz, cru_w,					// CRU access
	senila, NULL,					// SENILA/B access
	NULL, NULL						// 16 bit access (none here)
};

/*
    Defines the serial serdev.
*/
DEVICE_GET_INFO( ti99_rs232_serdev )
{
	switch ( state )
	{
		case DEVINFO_INT_INLINE_CONFIG_BYTES:		info->i = 0;								break;
		case DEVINFO_INT_IMAGE_TYPE:	            info->i = IO_SERIAL;                                      break;
		case DEVINFO_INT_IMAGE_READABLE:            info->i = 1;                                               break;
		case DEVINFO_INT_IMAGE_WRITEABLE:			info->i = 1;                                               break;
		case DEVINFO_INT_IMAGE_CREATABLE:	    	info->i = 1;                                               break;

		case DEVINFO_FCT_START:		                info->start = DEVICE_START_NAME( ti99_rs232_serdev );              break;
//      case DEVINFO_FCT_RESET:                     info->reset = DEVICE_RESET_NAME( ti99_rs232_serdev );           break;
		case DEVINFO_FCT_IMAGE_LOAD:		        info->f = (genf *) DEVICE_IMAGE_LOAD_NAME( ti99_rs232_serdev );    break;
		case DEVINFO_STR_NAME:		                strcpy( info->s, "TI99 RS232 attached serial device");	                         break;
		case DEVINFO_STR_FAMILY:                    strcpy(info->s, "Peripheral expansion");	                         break;
		case DEVINFO_STR_IMAGE_FILE_EXTENSIONS:	    strcpy(info->s, "");                                           break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 		break;
	}
}

/*
    Defines the PIO (parallel IO) serdev
*/
DEVICE_GET_INFO( ti99_piodev )
{
	switch ( state )
	{
		case DEVINFO_INT_INLINE_CONFIG_BYTES:		info->i = 0;								break;
		case DEVINFO_INT_IMAGE_TYPE:	            info->i = IO_PARALLEL;                                      break;
		case DEVINFO_INT_IMAGE_READABLE:            info->i = 1;                                               break;
		case DEVINFO_INT_IMAGE_WRITEABLE:			info->i = 1;                                               break;
		case DEVINFO_INT_IMAGE_CREATABLE:	    	info->i = 1;                                               break;

		case DEVINFO_FCT_START:		                info->start = DEVICE_START_NAME( ti99_piodev );              break;
		case DEVINFO_FCT_IMAGE_LOAD:		        info->f = (genf *) DEVICE_IMAGE_LOAD_NAME( ti99_piodev );    break;
		case DEVINFO_FCT_IMAGE_UNLOAD:		        info->f = (genf *) DEVICE_IMAGE_UNLOAD_NAME(ti99_piodev );  break;
		case DEVINFO_STR_NAME:		                strcpy( info->s, "TI99 PIO attached device");	                         break;
		case DEVINFO_STR_FAMILY:                    strcpy(info->s, "Peripheral expansion");	                         break;
		case DEVINFO_STR_IMAGE_FILE_EXTENSIONS:	    strcpy(info->s, "");                                           break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 		break;
	}
}

DECLARE_LEGACY_IMAGE_DEVICE(TI99_RS232, ti99_rs232_serdev);
DEFINE_LEGACY_IMAGE_DEVICE(TI99_RS232, ti99_rs232_serdev);
DECLARE_LEGACY_IMAGE_DEVICE(TI99_PIO, ti99_piodev);
DEFINE_LEGACY_IMAGE_DEVICE(TI99_PIO, ti99_piodev);

static DEVICE_START( ti_rs232 )
{
	ti_rs232_state *card = get_safe_token(device);
	peb_callback_if *topeb = (peb_callback_if *)device->baseconfig().static_config();

	astring *region = new astring();
	astring_assemble_3(region, device->tag(), ":", ser_region);
	card->rom = device->machine().region(astring_c(region))->base();
	devcb_resolve_write_line(&card->lines.inta, &topeb->inta, device);
	// READY and INTB are not used
	card->uart0 = device->subdevice("tms9902_0");
	card->uart1 = device->subdevice("tms9902_1");
	card->serdev0 = device->subdevice("serdev0");
	card->serdev1 = device->subdevice("serdev1");
	card->piodev = device->subdevice("piodev");
}

static DEVICE_STOP( ti_rs232 )
{
}

static DEVICE_RESET( ti_rs232 )
{
	ti_rs232_state *card = get_safe_token(device);
	/* Register the card */
	device_t *peb = device->owner();

	if (input_port_read(device->machine(), "SERIAL")==SERIAL_TI)
	{
		int success = mount_card(peb, device, &tirs232_card, get_pebcard_config(device)->slot);
		if (!success) return;

		card->pio_direction = 0;
		card->pio_handshakeout = 0;
		card->pio_spareout = 0;
		card->flag0 = 0;
		card->cts1 = 0;
		card->cts2 = 0;
		card->led = 0;
		card->pio_write = 1;

		card->select_mask = 0x7e000;
		card->select_value = 0x74000;

		if (input_port_read(device->machine(), "MODE")==GENMOD)
		{
			// GenMod card modification
			card->select_mask = 0x1fe000;
			card->select_value = 0x174000;
		}
	}
}

static MACHINE_CONFIG_FRAGMENT( ti_rs232 )
	MCFG_TMS9902_ADD("tms9902_0", tms9902_params_0)
	MCFG_TMS9902_ADD("tms9902_1", tms9902_params_1)
	MCFG_DEVICE_ADD("serdev0", TI99_RS232, 0)
	MCFG_DEVICE_ADD("serdev1", TI99_RS232, 0)
	MCFG_DEVICE_ADD("piodev", TI99_PIO, 0)
MACHINE_CONFIG_END

ROM_START( ti_rs232 )
	ROM_REGION(0x1000, ser_region, 0)
	ROM_LOAD_OPTIONAL("rs232.bin", 0x0000, 0x1000, CRC(eab382fb) SHA1(ee609a18a21f1a3ddab334e8798d5f2a0fcefa91)) /* TI rs232 DSR ROM */
ROM_END

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)             p##ti_rs232##s
#define DEVTEMPLATE_FEATURES            DT_HAS_START | DT_HAS_STOP | DT_HAS_RESET | DT_HAS_INLINE_CONFIG | DT_HAS_ROM_REGION | DT_HAS_MACHINE_CONFIG
#define DEVTEMPLATE_NAME                "TI99 RS232/PIO interface card"
#define DEVTEMPLATE_SHORTNAME           "ti99rs232"
#define DEVTEMPLATE_FAMILY              "Peripheral expansion"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE( TIRS232, ti_rs232 );

