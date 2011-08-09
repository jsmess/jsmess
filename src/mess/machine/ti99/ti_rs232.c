/*
    TI-99 RS232 and Parallel interface card

    TI99 RS232 card ('rs232')
    TMS9902 ('rs232:tms9902_0')
    TMS9902 ('rs232:tms9902_1')
    TI99 RS232 attached serial device ('rs232:serdev0')
    TI99 RS232 attached serial device ('rs232:serdev1')
    TI99 PIO attached parallel device ('rs232:piodev')

    Currently this emulation does not directly interact with the serial
    interface on the host computer. However, using a socket connection it is
    possible to attach an external bridge which interacts with a real UART.

    TI RS232 card wiring
    --------------------
    The card uses this wiring (inverters not included)

     +-----+         Pins of the connector
     | 9902|          (common naming)
     | RIN |---<-------  2  (TD)
     | XOUT|--->-------  3  (RD)
     | RTS |--->-------  8  (DCD)
     | CTS |-<-+------- 20  (DTR)
     | DSR |-<-+    H--> 6  (DSR)
     +-----+     +-----> 5  (CTS)
                /
     +-----+   /
     | CRU |--+
     +-----+

    This wiring is typical for a DCE, not a DTE. The TI RS232 was obviously
    designed to look like a modem. The advantage is that you can use the same
    cables for connecting a modem to the RS232 interface or for connecting
    a second TI via its interface. To connect to a DTE you can use a 1-1
    wiring cable (1 on 1, 2 on 2 ...)

    The TI manual for the RS232 card suggests the following cables:

    TI RS232   -    Modem or other TI RS232
      2 -----<----- 3
      3 ----->----- 2
      6 ----->---- 20         (crossover cable)
     20 -----<----- 6

    TI RS232   -    Terminal (DTE)
      2 ----<------ 2
      3 ---->------ 3
      5 ---->------ 5
      6 ---->------ 6         (1-1 cable)
      8 ---->------ 8
     20 ----<------20

    If we want to use a PC serial interface to play the role of the TI
    interface we have to map the TI wiring to a suitable wiring for PC
    interfaces which are designed as DTEs. This is achieved by the functions
    map_lines_in, map_lines_out.

    Note that we now have to swap the cable types: Use a 1-1 cable to connect
    another TI or a modem on the other end, and use a crossover cable for
    another PC (the usual way of connecting).

    RS232 Over IP protocol
    ----------------------
    This implementation can make use of such an external bridge. Normal data
    are forwarded to the bridge and back, while line control is organized via
    special byte sequences. These sequences are introduced by a 0x1B byte (ESC).

    The protocol has two modes: normal and escape

    normal mode: transmit byte (!= 0x1b) unchanged
    escape mode: entered by ESC, bytes following:
       ESC = plain ESC byte
       length byte[length] = control sequence (length != 0x1b)

       byte[]:
          All configuration settings are related to a specified UART; UARTs may
          differ in their capabilities and may require specific settings
          (e.g. the TMS9902 specifies the line speed by a clock ratio, while
          others may have indexed, fixed rates or use integers)

          (x=unused)

          1ccc xaaa = configuration of parameter ccc; UART type aaa
             1111 xaaa rrrr rrrr rrrr 0000     = config receive rate on aaa
             1110 xaaa rrrr rrrr rrrr 0000     = config transmit rate on aaa
             1101 xaaa xxxx xxbb               = config databits bb (00=5 ... 11=8)
             1100 xaaa xxxx xxss               = config stop bits ss (00=1.5, 01=2, 1x=1)
             1011 xaaa xxxx xxpp               = config parity pp (1x=enable, x1=odd)

          01ab cdef = line state of RTS=a, CTS=b, DSR=c, DCD=d, DTR=e, RI=f
          00gh i000 = exception g=BRK, h=FRMERR, i=PARERR

    The protocol changes back to normal mode after transmitting the control
    sequence.
*/

#include "emu.h"
#include "peribox.h"
#include "machine/tms9902.h"
#include "ti_rs232.h"

#define CRU_BASE 0x1300
#define SENILA_0_BIT 0x80
#define SENILA_1_BIT 0x40

#define ser_region "ser_region"

#define RECV_MODE_NORMAL 1
#define RECV_MODE_ESC 2
#define RECV_MODE_ESC_LINES 3

#define VERBOSE 1

#define LOG logerror

// Second card: Change CRU base to 0x1500
// #define CRU_BASE 0x1500

typedef ti99_pebcard_config ti_rs232_config;

typedef struct _ti_rs232_state
{
	device_t *uart[2];
	device_t *serdev[2], *piodev;

	int 	selected;
	int		select_mask;
	int		select_value;

	UINT8	signals[2]; // Latches the state of the output lines for UART0/UART1
	int		recv_mode[2]; // May be NORMAL or ESC

	// Baud rate management
	// not part of the real device, but required for the connection to the
	// real UART
	double time_hold[2];

	// PIO flags
	int pio_direction;		// a.k.a. PIOOC pio in output mode if 0
	int pio_handshakeout;
	int pio_handshakein;
	int pio_spareout;
	int pio_sparein;
	int flag0;				// spare
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

#define ESC 0x1b

INLINE ti_rs232_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == TIRS232);

	return (ti_rs232_state *)downcast<legacy_device_base *>(device)->token();
}

static void incoming_dtr(device_t *device, line_state value);
static void output_line_state(device_t *tms9902, int mask, UINT8 value);


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
		// Turn on polling
		tms9902_clock(tms9902, true);
	}
	else if (devnumber==1)
	{
		tms9902 = image.device().siblingdevice("tms9902_1");
		// Turn on polling
		tms9902_clock(tms9902, true);
	}
	else
	{
		LOG("ti99/rs232: Could not find device tag number\n");
		return IMAGE_INIT_FAIL;
	}

	incoming_dtr(tms9902, image? ASSERT_LINE : CLEAR_LINE);

	return IMAGE_INIT_PASS;
}

static DEVICE_IMAGE_UNLOAD( ti99_rs232_serdev )
{
	device_t *tms9902 = NULL;
	int devnumber = get_index_from_tagname(&image.device());
	if (devnumber==0)
	{
		tms9902 = image.device().siblingdevice("tms9902_0");
		// Turn off polling
		tms9902_clock(tms9902, false);
	}
	else if (devnumber==1)
	{
		tms9902 = image.device().siblingdevice("tms9902_1");
		// Turn off polling
		tms9902_clock(tms9902, false);
	}
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
	card->pio_writable = !image.is_readonly();

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
    TODO: Check read operations
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
			// The CTS line is realized as CRU bits
			// Mind that this line is handled as an output going to the remote CTS
			if (card->signals[0] & CTS)
				reply |= 0x20;
			if (card->signals[1] & CTS)
				reply |= 0x40;
			if (card->led)
				reply |= 0x80;
			*value = reply;
			return;
		}
		if ((offset & 0x00c0)==0x0040)
		{
			*value = tms9902_cru_r(card->uart[0], offset>>4);
			return;
		}
		if ((offset & 0x00c0)==0x0080)
		{
			*value = tms9902_cru_r(card->uart[1], offset>>1);
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
			tms9902_cru_w(card->uart[0], offset>>1, data);
			return;
		}
		if ((offset & 0x00c0)==0x0080)
		{
			tms9902_cru_w(card->uart[1], offset>>1, data);
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
			// Set the CTS line for RS232/1
			LOG("TI-RS232/1: Setting CTS via CRU to %d\n", data);
			output_line_state(card->uart[0], CTS, (data)? CTS : 0);
			break;

		case 6:
			// Set the CTS line for RS232/2
			LOG("TI-RS232/2: Setting CTS via CRU to %d\n", data);
			output_line_state(card->uart[1], CTS, (data)? CTS : 0);
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
		if (VERBOSE>3) LOG("ti99/rs232: Sensing ILA\n");
		*value = card->ila;
		// The card ROM must be unselected, or we get two values
		// on the data bus

		// Not sure whether this is correct; there is no software that makes
		// use of it
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
    Propagates the /INT signal of the UARTs to the /INT line of the pbox.
*/
static TMS9902_INT_CALLBACK( int_callback )
{
	device_t *carddev = device->owner();
	ti_rs232_state *card = get_safe_token(carddev);
	int senila_bit = (device==card->uart[0])? SENILA_0_BIT : SENILA_1_BIT;

	if (state==FALSE)
		card->ila |= senila_bit;
	else
		card->ila &= ~senila_bit;

	card->lines.inta(state);
}

// ==========================================================

/*
    The DTR line of the interface card is wired to the CTS and DSR
    of the UART.
*/
static void incoming_dtr(device_t *tms9902, line_state value)
{
	ti_rs232_state *card = get_safe_token(tms9902->owner());
	int uartind = (tms9902==card->uart[0])? 0 : 1;

	if (VERBOSE>2) LOG("TI-RS232/%d: incoming DTR = %d\n", uartind+1, (value==ASSERT_LINE)? 1:0);

	tms9902_rcv_cts(tms9902, value);
	tms9902_rcv_dsr(tms9902, value);
}

/*
    Data transmission
*/
static void transmit_data(device_t *tms9902, UINT8 value)
{
	UINT8 buf = value;
	ti_rs232_state *card = get_safe_token(tms9902->owner());
	int uartind = (tms9902==card->uart[0])? 0 : 1;

	device_image_interface *serial;
	serial = dynamic_cast<device_image_interface *>(card->serdev[uartind]);
	if (!serial->exists())
	{
		if (VERBOSE>1) LOG("TI-RS232/%d: No serial output attached\n", uartind+1);
		return;
	}

	// Send a double ESC if this is not a control operation
	if (buf==0x1b)
	{
		if (VERBOSE>2) LOG("TI-RS232/%d: send ESC (requires another ESC)\n", uartind+1);
		serial->fwrite(&buf, 1);
	}
	if (VERBOSE>3) LOG("TI-RS232/%d: send %c <%02x>\n", uartind+1, buf, buf);
	serial->fwrite(&buf, 1);
}

/*
    Map the DCE-like wiring to a DTE-like wiring (and vice versa), V1

       Emulated      PC serial
       TI RS232      interface
     XOUT  2 -----------( 3) ---> TXD
      RIN  3 -----------( 2) <--- RXD
       nc  4 -----------( 5) <--- CTS  (cable)
      CRU  5 -|       |-( 8) <--- DCD
  DSR+CTS 20 -----------( 6) <--- DSR
     +12V  6 -----------(20) ---> DTR
      RTS  8 -----------( 4) ---> RTS


      Alternative mapping for the PORT emulator: (V2)

       Emulated      PC serial
       TI RS232      interface
     XOUT  2 -----------( 3) ---> TXD
      RIN  3 -----------( 2) <--- RXD
  DSR+CTS 20 -----------( 5) <--- CTS  (cable)
      RTS  8 -----------(20) ---> DTR
      CRU  5 -----------( 4) ---> RTS
      +12V 6 -|       |-( 6) <--- DSR
        nc 4 -----------( 8) <--- DCD
*/
static UINT8 map_lines_out(device_t *carddev, int uartind, UINT8 value)
{
	UINT8 ret = 0;
	int mapping = input_port_read(carddev->machine(), "SERIALMAP");

	//    00ab cdef = setting line RTS=a, CTS=b, DSR=c, DCD=d, DTR=e, RI=f

	if (VERBOSE>3) LOG("TI-RS232/%d: out connector pins = 0x%02x; translate for DTE\n", uartind+1, value);

	if (value & BRK)
	{
		if (VERBOSE>5) LOG("TI-RS232/%d: ... sending BRK\n", uartind+1);
		ret |= EXCEPT | BRK;
	}

	if (mapping==0)
	{
		// V1
		if (value & CTS)
		{
			if (VERBOSE>5) LOG("TI-RS232/%d: ... cannot map CTS line, ignoring\n", uartind+1);
		}
		if (value & DSR)
		{
			ret |= DTR;
			if (VERBOSE>5) LOG("TI-RS232/%d: ... setting DTR line\n", uartind+1);
		}
		if (value & DCD)
		{
			ret |= RTS;
			if (VERBOSE>5) LOG("TI-RS232/%d: ... setting RTS line\n", uartind+1);
		}
	}
	else
	{
		// V2
		if (value & CTS)
		{
			ret |= RTS;
			if (VERBOSE>5) LOG("TI-RS232/%d: ... setting RTS line\n", uartind+1);
		}
		if (value & DCD)
		{
			ret |= DTR;
			if (VERBOSE>5) LOG("TI-RS232/%d: ... setting DTR line\n", uartind+1);
		}
	}

	return ret;
}

static UINT8 map_lines_in(device_t *carddev, int uartind, UINT8 value)
{
	UINT8 ret = 0;
	int mapping = input_port_read(carddev->machine(), "SERIALMAP");

	//    00ab cdef = setting line RTS=a, CTS=b, DSR=c, DCD=d, DTR=e, RI=f

	if (VERBOSE>3) LOG("TI-RS232/%d: in connector pins = 0x%02x; translate for DTE\n", uartind+1, value);

	if (value & BRK)
	{
		if (VERBOSE>5) LOG("TI-RS232/%d: ... getting BRK\n", uartind+1);
		ret |= EXCEPT | BRK;
	}

	if (mapping==0)
	{
		// V1
		if (value & CTS)
		{
			if (VERBOSE>5) LOG("TI-RS232/%d: ... cannot map CTS line, ignoring\n", uartind+1);
		}
		if (value & DSR)
		{
			ret |= DTR;
			if (VERBOSE>5) LOG("TI-RS232/%d: ... setting DTR line\n", uartind+1);
		}
		if (value & DCD)
		{
			if (VERBOSE>5) LOG("TI-RS232/%d: ... cannot map DCD line, ignoring\n", uartind+1);
		}
	}
	else
	{
		// V2 (PORT application)
		if (value & CTS)
		{
			ret |= DTR;
			if (VERBOSE>5) LOG("TI-RS232/%d: ... setting DTR line\n", uartind+1);
		}
		if (value & DSR)
		{
			if (VERBOSE>5) LOG("TI-RS232/%d: ... cannot map DSR line, ignoring\n", uartind+1);
		}
		if (value & DCD)
		{
			if (VERBOSE>5) LOG("TI-RS232/%d: ... cannot map DCD line, ignoring\n", uartind+1);
		}
	}

	return ret;
}
/*
    Receive a character or a line state from the remote site. This method
    is called by a timer with some sufficiently high polling frequency. Note
    that the control lines are not subject to baud rates.
    The higher polling frequency will cause overloads in the TMS9902 which has
    a one-byte buffer only: Since the data source (e.g. the PC UART and the
    socket connection) may be buffered, we may get a cluster of bytes in rapid
    succession. In order to avoid this, this function uses the parameter
    "baudpoll" which is the ratio of the characters/second and the polling
    frequency. (char/sec is baud rate divided by 10.)
    Whenever we receive a character that is passed to the UART, we have to
    pause for 1/baudpoll iterations before getting the next byte from the
    data source.

    FIXME: This may fail when the emulated system tries to stop the remote
    system by deactivating RTS or DTR, but there are still incoming
    bytes in the socket or PC UART buffer. The buffered bytes may then cause
    an overflow in the emulated UART, since the application program expects
    the remote system to stop sending instantly.
    The only way to handle this is to mirror the activity within the serial
    bridge: Whenever a RTS=0 or DTR=0 is transmitted to the remote site, the
    serial bridge must stop delivering data bytes until the handshake opens the
    channel again.
*/
static void receive_data_or_line_state(device_t *tms9902, double baudpoll)
{
	ti_rs232_state *card = get_safe_token(tms9902->owner());
	device_image_interface *serial;
	UINT8 buffer;
	int uartind = (tms9902==card->uart[0])? 0 : 1;
	int len = 0;

	serial = dynamic_cast<device_image_interface *>(card->serdev[uartind]);

	if (!serial->exists())
	{
		if (VERBOSE>1) LOG("TI-RS232/%d: No serial input attached\n", uartind+1);
		return;
	}

	// If more than the minimum waiting time since the last data byte has
	// elapsed, we can get a new value.
	if (card->time_hold[uartind] > 1.0)
	{
		len = serial->fread(&buffer, 1);
		if (len==0) return;
	}
	else
	{
		// number of polls has not yet elapsed; we have to wait.
		card->time_hold[uartind] += baudpoll;
		return;
	}

	// No config parameters here, only data or line setting
	switch (card->recv_mode[uartind])
	{
	case RECV_MODE_NORMAL:
		if (buffer==0x1b)
		{
			if (VERBOSE>2) LOG("TI-RS232/%d: received: %c <%02x>, switch to ESC mode\n", uartind+1, buffer, buffer);
			card->recv_mode[uartind] = RECV_MODE_ESC;
		}
		else
		{
			if (VERBOSE>3) LOG("TI-RS232/%d: received: %c <%02x>, pass to UART\n", uartind+1, buffer, buffer);
			tms9902_rcv_data(tms9902, buffer);
			card->time_hold[uartind] = 0.0;
		}
		break;
	case RECV_MODE_ESC:
		if (buffer==0x1b)
		{
			card->recv_mode[uartind] = RECV_MODE_NORMAL;
			if (VERBOSE>2) LOG("TI-RS232/%d: leaving ESC mode, received: %c <%02x>, pass to UART\n", uartind+1, buffer, buffer);
			tms9902_rcv_data(tms9902, buffer);
			card->time_hold[uartind] = 0.0;
		}
		else
		{
			// the byte in buffer is the length byte
			if (VERBOSE>3) LOG("TI-RS232/%d: received length byte <%02x> in ESC mode\n", uartind+1, buffer);
			if (buffer != 1)
			{
				LOG("TI-RS232/%d: expected length 1 but got %02x, leaving ESC mode.\n", uartind+1, buffer);
				card->recv_mode[uartind] = RECV_MODE_NORMAL;
			}
			else
				card->recv_mode[uartind] = RECV_MODE_ESC_LINES;
		}
		break;

	case RECV_MODE_ESC_LINES:
		// Map the real serial interface lines to our emulated connector
		// The mapping is the same for both directions, so we use the same function
		if (buffer & EXCEPT)
		{
			// Exception states: BRK, FRMERR, PARERR
			tms9902_rcv_break(tms9902, ((buffer & BRK)!=0));

			if (buffer & FRMERR)
				tms9902_rcv_framing_error(tms9902);
			if (buffer & PARERR)
				tms9902_rcv_parity_error(tms9902);
		}
		else
		{
			buffer = map_lines_in(tms9902->owner(), uartind, buffer);
			if (VERBOSE>2) LOG("TI-RS232/%d: received (remapped) <%02x> in ESC mode\n", uartind+1, buffer);

			// The DTR line on the RS232 connector of the board is wired to both the
			// CTS and the DSR pin of the TMS9902
			// Apart from the data line, DTR is the only input line
			incoming_dtr(tms9902,  (buffer & DTR)? ASSERT_LINE : CLEAR_LINE);
		}

		card->recv_mode[uartind] = RECV_MODE_NORMAL;
		break;

	default:
		LOG("TI-RS232/%d: unknown mode: %d\n", uartind+1, card->recv_mode[uartind]);
	}
}

static TMS9902_RCV_CALLBACK( rcv_callback )
{
	// Called from the UART when it wants to receive a character
	// However, characters are not passed to it at this point
	// Instead, we check for signal line change or data transmission
	// and call the respective function
	receive_data_or_line_state(device, baudpoll);
}

static TMS9902_XMIT_CALLBACK( xmit_callback )
{
	transmit_data(device, (UINT8)data);
}

/*
    Control operations like configuration or line changes
*/
static void configure_interface(device_t *tms9902, int type, int value)
{
	ti_rs232_state *card = get_safe_token(tms9902->owner());
	UINT8 bufctrl[4];
	device_image_interface *serial;
	int uartind = (tms9902==card->uart[0])? 0 : 1;
	UINT8 esc = ESC;

	serial = dynamic_cast<device_image_interface *>(card->serdev[uartind]);

	if (!serial->exists())
	{
		if (VERBOSE>1) LOG("TI-RS232/%d: No serial output attached\n", uartind+1);
		return;
	}

	serial->fwrite(&esc, 1);
	bufctrl[0] = 0x02;
	bufctrl[1] = CONFIG | TYPE_TMS9902;

	switch (type) {
	case RATERECV:
		if (VERBOSE>2) LOG("TI-RS232/%d: send receive rate %04x\n", uartind+1, value);
		// value has 12 bits
		// 1ccc xaaa                         = config adapter type a
		// 1111 xaaa rrrr rrrr rrrr 0000     = config receive rate on a
		// 1110 xaaa rrrr rrrr rrrr 0000     = config transmit rate on a
		bufctrl[0] = 0x03; // length
		bufctrl[1] |= RATERECV;
		bufctrl[2] = (value & 0x0ff0)>>4;
		bufctrl[3] = (value & 0x0f)<<4;
		break;
	case RATEXMIT:
		if (VERBOSE>2) LOG("TI-RS232/%d: send transmit rate %04x\n", uartind+1, value);
		bufctrl[0] = 0x03; // length
		bufctrl[1] |= RATEXMIT;
		bufctrl[2] = (value & 0x0ff0)>>4;
		bufctrl[3] = (value & 0x0f)<<4;
		break;
	case STOPBITS:
		if (VERBOSE>2) LOG("TI-RS232/%d: send stop bit config %02x\n", uartind+1, value&0x03);
		bufctrl[1] |= STOPBITS;
		bufctrl[2] = (value & 0x03);
		break;
	case DATABITS:
		if (VERBOSE>2) LOG("TI-RS232/%d: send data bit config %02x\n", uartind+1, value&0x03);
		bufctrl[1] |= DATABITS;
		bufctrl[2] = (value & 0x03);
		break;
	case PARITY:
		if (VERBOSE>2) LOG("TI-RS232/%d: send parity config %02x\n", uartind+1, value&0x03);
		bufctrl[1] |= PARITY;
		bufctrl[2] = (value & 0x03);
		break;
	default:
		if (VERBOSE>1) LOG("TI-RS232/%d: error - unknown config type %02x\n", uartind+1, type);
	}

	serial->fwrite(bufctrl, bufctrl[0]+1);
}

static void set_bit(device_t *device, int uartind, int line, int value)
{
	ti_rs232_state *card = get_safe_token(device);
	if (VERBOSE>5)
	{
		switch (line)
		{
		case CTS: LOG("TI-RS232/%d: set CTS(out)=%s\n", uartind+1, (value)? "asserted" : "cleared"); break;
		case DCD: LOG("TI-RS232/%d: set DCD(out)=%s\n", uartind+1, (value)? "asserted" : "cleared"); break;
		case BRK: LOG("TI-RS232/%d: set BRK(out)=%s\n", uartind+1, (value)? "asserted" : "cleared"); break;
		}
	}

	if (value)
		card->signals[uartind] |= line;
	else
		card->signals[uartind] &= ~line;
}

/*
   Line changes
*/
static void output_exception(device_t *tms9902, int param, UINT8 value)
{
	ti_rs232_state *card = get_safe_token(tms9902->owner());
	device_image_interface *serial;
	UINT8 bufctrl[2];
	UINT8 esc = ESC;

	int uartind = (tms9902==card->uart[0])? 0 : 1;

	serial = dynamic_cast<device_image_interface *>(card->serdev[uartind]);

	if (!serial->exists())
	{
		if (VERBOSE>1) LOG("TI-RS232/%d: No serial output attached\n", uartind+1);
		return;
	}

	serial->fwrite(&esc, 1);

	bufctrl[0] = 1;
	// 0100 0xxv = exception xx: 02=BRK, 04=FRMERR, 06=PARERR; v=0,1 (only for BRK)
	// BRK is the only output exception
	bufctrl[1] = EXCEPT | param | (value&1);
	serial->fwrite(bufctrl, 2);
}

/*
   Line changes
*/
static void output_line_state(device_t *tms9902, int mask, UINT8 value)
{
	ti_rs232_state *card = get_safe_token(tms9902->owner());
	device_image_interface *serial;
	UINT8 bufctrl[2];
	UINT8 esc = ESC;

	int uartind = (tms9902==card->uart[0])? 0 : 1;

	serial = dynamic_cast<device_image_interface *>(card->serdev[uartind]);

	if (!serial->exists())
	{
		if (VERBOSE>1) LOG("TI-RS232/%d: No serial output attached\n", uartind+1);
		return;
	}

	serial->fwrite(&esc, 1);

	//  01ab cdef = setting line RTS=a, CTS=b, DSR=c, DCD=d, DTR=e, RI=f
	bufctrl[0] = 1;

	// The CTS line (coming from a CRU bit) is connected to the CTS pin
	if (mask & CTS) set_bit(tms9902->owner(), uartind, CTS, value & CTS);

	// The RTS line (from 9902) is connected to the DCD pin
	if (mask & RTS) set_bit(tms9902->owner(), uartind, DCD, value & RTS);

	// The DSR pin is hardwired to +5V
	set_bit(tms9902->owner(), uartind, DSR, 1);

	// As of here, the lines are set according to the schematics of the
	// serial interface.

	// Now translate the signals of the board to those of a DTE-like device
	// so that we can pass the signal to the real PC serial interface
	// (can be imagined as if we emulated the cable)
	bufctrl[1] = map_lines_out(tms9902->owner(), uartind, card->signals[uartind]);
	serial->fwrite(bufctrl, 2);
}

static TMS9902_CTRL_CALLBACK( ctrl_callback )
{
	switch (type)
	{
	case CONFIG:
		configure_interface(device, param, value); break;
	case EXCEPT:
		output_exception(device, param, value); break;
	default:
		output_line_state(device, param, value);
	}
}


static WRITE_LINE_DEVICE_HANDLER( senila )
{
	// put the value on the data bus. We store it in a state variable.
	ti_rs232_state *card = get_safe_token(device);
	if (VERBOSE>3) LOG("TI-RS232/1-2: Setting SENILA=%d\n", state);
	card->senila = state;
}

static const tms9902_interface tms9902_params =
{
	3000000.,				/* clock rate (3MHz) */
	int_callback,			/* called when interrupt pin state changes */
	rcv_callback,			/* called when a character is received */
	xmit_callback,			/* called when a character is transmitted */
	ctrl_callback
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
		case DEVINFO_FCT_IMAGE_UNLOAD:		        info->f = (genf *) DEVICE_IMAGE_UNLOAD_NAME( ti99_rs232_serdev );    break;
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
	peb_callback_if *topeb = (peb_callback_if *)device->static_config();

	astring region;
	astring_assemble_3(&region, device->tag(), ":", ser_region);
	card->rom = device->machine().region(region.cstr())->base();
	card->lines.inta.resolve(topeb->inta, *device);
	// READY and INTB are not used
	card->uart[0] = device->subdevice("tms9902_0");
	card->uart[1] = device->subdevice("tms9902_1");
	card->serdev[0] = device->subdevice("serdev0");
	card->serdev[1] = device->subdevice("serdev1");
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

		set_bit(device, 0, CTS, 0);
		set_bit(device, 1, CTS, 0);

		card->led = 0;
		card->pio_write = 1;
		card->recv_mode[0] = RECV_MODE_NORMAL;
		card->recv_mode[1] = RECV_MODE_NORMAL;

		card->select_mask = 0x7e000;
		card->select_value = 0x74000;

		card->time_hold[0] = card->time_hold[1] = 0.0;

		// The GenMod modification changes the address bus width of the Geneve.
		// All peripheral cards need to be manually modified to properly decode
		// the wider address. The next lines perform this soldering job
		// automagically.
		if (input_port_read(device->machine(), "MODE")==GENMOD)
		{
			// GenMod card modification
			card->select_mask = 0x1fe000;
			card->select_value = 0x174000;
		}
	}
}

static MACHINE_CONFIG_FRAGMENT( ti_rs232 )
	MCFG_TMS9902_ADD("tms9902_0", tms9902_params)
	MCFG_TMS9902_ADD("tms9902_1", tms9902_params)
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

