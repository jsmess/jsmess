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

#include "driver.h"
#include "ti99_4x.h"
#include "99_peb.h"
#include "tms9902.h"
#include "994x_ser.h"

#define MAX_RS232_CARDS 1/*2*/

/* prototypes */
static void int_callback(int which, int INT);
static void xmit_callback(int which, int data);
static int rs232_cru_r(int offset);
static void rs232_cru_w(int offset, int data);
static  READ8_HANDLER(rs232_mem_r);
static WRITE8_HANDLER(rs232_mem_w);

/* pointer to the rs232 ROM data */
static UINT8 *rs232_DSR;

static int pio_direction;	// a.k.a. PIOOC pio in output mode if 0
static int pio_handshakeout;
static int pio_handshakein;
static int pio_spareout;
static int pio_sparein;
static int flag0;	// spare
static int cts1;	// a.k.a. flag1
static int cts2;	// a.k.a. flag2
static int led;		// a.k.a. flag3
static int pio_out_buffer;
static int pio_in_buffer;

static mess_image *pio_fp;
static int pio_readable;
static int pio_writable;
static int pio_write = 1/*0*/;	// 1 if image is to be written to

static mess_image *rs232_fp;

static const ti99_peb_card_handlers_t rs232_handlers =
{
	rs232_cru_r,
	rs232_cru_w,
	rs232_mem_r,
	rs232_mem_w
};

static const tms9902reset_param tms9902_params =
{
	3000000.,				/* clock rate (3MHz) */
	int_callback,			/* called when interrupt pin state changes */
	NULL,//rts_callback,			/* called when Request To Send pin state changes */
	NULL,//brk_callback,			/* called when BReaK state changes */
	xmit_callback			/* called when a character is transmitted */
};



/*
	Initialize pio unit and open image
*/
DEVICE_LOAD( ti99_4_pio )
{
	int id = image_index_in_device(image);
	if ((id < 0) || (id >= MAX_RS232_CARDS))
		return INIT_FAIL;

	pio_fp = image;
	/* tell whether the image is writable */
	pio_readable = ! image_has_been_created(image);
	/* tell whether the image is writable */
	pio_writable = image_is_writable(image);
	if (pio_write && pio_writable)
		pio_handshakein = 0;	/* receiver ready */
	else
		pio_handshakein = 1;

	return INIT_PASS;
}

/*
	close a pio image
*/
DEVICE_UNLOAD( ti99_4_pio )
{
	int id = image_index_in_device(image);
	if ((id < 0) || (id >= MAX_RS232_CARDS))
		return;

	pio_fp = NULL;
	pio_writable = 0;
	pio_handshakein = 1;	/* receiver not ready */
	pio_sparein = 0;
}

/*
	Initialize rs232 unit and open image
*/
DEVICE_LOAD( ti99_4_rs232 )
{
	int id = image_index_in_device(image);

	/*if ((id < 0) || (id >= 2*MAX_RS232_CARDS))
		return INIT_FAIL;*/
	if (id != 0)
		return INIT_FAIL;

	rs232_fp = image;

	if (rs232_fp)
	{
		tms9902_set_cts(id, 1);
		tms9902_set_dsr(id, 1);
	}
	else
	{
		tms9902_set_cts(id, 0);
		tms9902_set_dsr(id, 0);
	}

	return INIT_PASS;
}

/*
	close a rs232 image
*/
DEVICE_UNLOAD( ti99_4_rs232 )
{
	/*int id = image_index_in_device(image);*/

	/*if ((id < 0) || (id >= 2*MAX_RS232_CARDS))
		return;*/
	/*if (id != 0)
		return;*/

	rs232_fp = NULL;
}

/*
	Reset rs232 card, set up handlers
*/
void ti99_rs232_init(void)
{
	rs232_DSR = memory_region(region_dsr) + offset_rs232_dsr;

	ti99_peb_set_card_handlers(0x1300, & rs232_handlers);

	pio_direction = 0;
	pio_handshakeout = 0;
	pio_spareout = 0;
	flag0 = 0;
	cts1 = 0;
	cts2 = 0;
	led = 0;

	tms9902_init(0, &tms9902_params);
	tms9902_init(1, &tms9902_params);
}

/*
	rs232 card clean-up
*/
void ti99_rs232_cleanup(void)
{
	tms9902_cleanup(0);
	tms9902_cleanup(1);
}

/*
	rs232 interrupt handler
*/
static void int_callback(int which, int INT)
{
	switch (which)
	{
	case 0:
		ti99_peb_set_ila_bit(inta_rs232_1_bit, INT);
		break;

	case 1:
		ti99_peb_set_ila_bit(inta_rs232_2_bit, INT);
		break;

	case 2:
		ti99_peb_set_ila_bit(inta_rs232_3_bit, INT);
		break;

	case 3:
		ti99_peb_set_ila_bit(inta_rs232_4_bit, INT);
		break;
	}
}

static void xmit_callback(int which, int data)
{
	UINT8 buf = data;

	if (rs232_fp)
		image_fwrite(rs232_fp, &buf, 1);
}

/*
	Read rs232 card CRU interface

	bit 0: always 0
	...
*/
static int rs232_cru_r(int offset)
{
	int reply;

	switch (offset >> 2)
	{
	case 0:
		/* custom buffer */
		switch (offset)
		{
		case 0:
			reply = 0x00;
			if (pio_direction)
				reply |= 0x02;
			if (pio_handshakein)
				reply |= 0x04;
			if (pio_sparein)
				reply |= 0x08;
			if (flag0)
				reply |= 0x10;
			if (cts1)
				reply |= 0x20;
			if (cts2)
				reply |= 0x40;
			if (led)
				reply |= 0x80;
			break;

		default:
			reply = 0;
			break;
		}
		break;

	case 1:
		/* first 9902 */
		reply = tms9902_cru_r(0, offset);
		break;

	case 2:
		/* second 9902 */
		reply = tms9902_cru_r(1, offset);
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
static void rs232_cru_w(int offset, int data)
{
	switch (offset >> 5)
	{
	case 0:
		/* custom buffer */
		switch (offset)
		{
		case 0:
			/* WRITE to rs232 card enable bit (bit 0) */
			/* handled in xxx_peb_cru_w() */
			break;

		case 1:
			pio_direction = data;
			break;

		case 2:
			if (data != pio_handshakeout)
			{
				pio_handshakeout = data;
				if (pio_write && pio_writable && (! pio_direction))
				{	/* PIO in output mode */
					if (!pio_handshakeout)
					{	/* write data strobe */
						/* write data and acknowledge */
						UINT8 buf = pio_out_buffer;
						if (image_fwrite(pio_fp, &buf, 1))
							pio_handshakein = 1;
					}
					else
					{
						/* end strobe */
						/* we can write some data: set receiver ready */
						pio_handshakein = 0;
					}
				}
				if ((!pio_write) && pio_readable /*&& pio_direction*/)
				{	/* PIO in input mode */
					if (!pio_handshakeout)
					{	/* receiver ready */
						/* send data and strobe */
						UINT8 buf;
						if (image_fread(pio_fp, &buf, 1))
							pio_in_buffer = buf;
						pio_handshakein = 0;
					}
					else
					{
						/* data acknowledge */
						/* we can send some data: set transmitter ready */
						pio_handshakein = 1;
					}
				}
			}
			break;

		case 3:
			pio_spareout = data;
			break;

		case 4:
			flag0 = data;
			break;

		case 5:
			cts1 = data;
			break;

		case 6:
			cts2 = data;
			break;

		case 7:
			led = data;
			break;
		}
		break;

	case 1:
		/* first 9902 */
		tms9902_cru_w(0, offset, data);
		break;

	case 2:
		/* second 9902 */
		tms9902_cru_w(1, offset, data);
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
		reply = pio_direction ? pio_in_buffer : pio_out_buffer;
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
		pio_out_buffer = data;
	}
	else
	{
		/* DSR ROM: do nothing */
	}
}
