/*
    TMS9902 Asynchronous Communication Controller

    TMS9902 is an asynchronous serial controller for use with the TI990 and
    TMS9900 family.  It provides serial I/O, three extra I/O pins (namely RTS,
    DSR and CTS), and a timer.  It communicates with the CPU through the CRU
    I/O bus, and one interrupt pin.

               +----+--+----+
     <-   /INT |1   \--/  18| VCC
     <-   XOUT |2         17| /CE     <-
     ->    RIN |3         16| /PHI    <-
     <-  CRUIN |4         15| CRUCLK  <-
     <-   /RTS |5         14| S0      <-
     ->   /CTS |6         13| S1      <-
     ->   /DSR |7         12| S2      <-
     -> CRUOUT |8         11| S3      <-
           VSS |9         10| S4      <-
               +------------+

     The CRUIN line borrows its name from the connector of the connected CPU
     where it is an input, so CRUIN is an output of this chip. The same is true
     for CRUOUT.

     /PHI is a TTL clock input with 4 MHz maximum rate.

    IMPORTANT NOTE: The previous versions of TMS9902 attempted to write their
    output to a file. This implementation is able to communicate with an external
    UART via a socket connection and an external bridge. However, the work is
    not done yet, and until then the file writing is disabled.

    Raphael Nabet, 2003
    Michael Zapf, 2011
*/

#include <math.h>
#include "emu.h"

#include "tms9902.h"

#define VERBOSE 1

// Polling frequency. We use a much higher value to allow for line state changes
// happening between character transmissions (which happen in parallel in real
// communications but which must be serialized here)
#define POLLING_FREQ 20000

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _tms9902_t tms9902_t;
struct _tms9902_t
{
	/* driver-specific configuration */
	/* tms9902 clock rate (PHI* pin, normally connected to TMS9900 Phi3*) */
	/* Official range is 2MHz-3.3MHz.  Some tms9902s were sold as "MP9214", and
    were tested for speeds up to 4MHz, provided the clk4m control bit is set.
    (warning: 3MHz on a tms9900 is equivalent to 12MHz on a tms9995 or tms99000) */
	double clock_rate;

	/* MZ: removed bit field definitions; does not deliver much saving in space,
    but can prevent good optimization. */

	/* CRU interface */
	UINT8 register_select;	/* bits device select device register is being written to */

	/* inputs pins */
	bool CTSflag;
	bool DSRflag;

	/* output pin */
	bool RTSON;
	bool RTSflag;

	/* transmitter registers */
	bool BRKON;		/* Flag which causes BRK to go active */
	bool BRKflag;	/* Current BRK state */

	UINT8 XBR;		/* transmit buffer register */
	UINT8 XSR;		/* transmit shift register */

	bool XBRE;
	bool XSRE;

	/* receiver registers */

	UINT8 RBR;		/* receive buffer register */
	bool RBRL;
	bool ROVER;

	/* various status flags */

	bool DSCH;
	bool TIMELP;

	bool TIMERR;

	bool DSCENB;
	bool RIENB;
	bool XBIENB;
	bool TIMENB;

	bool INT;		// mirrors /INT output line, inverted

	bool CLK4M;

	/*
        Rate registers. The receive bit rate calculates as
        bitrate = clock1 / (2 * (8 ^ RDV8) * RDR)
        (similarly for transmit)

        where clock1 = clock_rate / (CLK4M? 4:3)
    */
	UINT16 RDR;
	bool RDV8;
	UINT16 XDR;
	bool XDV8;

	bool RIN;		// mirrors the state of the RIN pin
	bool RSBD;
	bool RFBD;
	bool RFER;
	bool RPER;
	bool RCVERR;

	UINT8 RCL;		// Character length
	bool ODDP;
	bool PENB;
	UINT8 STOPB;

	UINT8 TMR;		/* interval timer */

	/* clock registers */
	emu_timer *dectimer;			/* MESS timer, used to emulate the decrementer register */
	emu_timer *optimer;

	// This value is the ratio of data input versus the poll rate. The
	// data source should deliver data bytes at every 1/baudpoll call.
	// This is to ensure that data is delivered at a rate that is expected
	// from the emulated program.
	double baudpoll;

	/* Pointer to interface */
	const tms9902_interface *intf;
};

/* bits in register_select */
enum
{
	register_select_LXDR = 0x1,
	register_select_LRDR = 0x2,
	register_select_LDIR = 0x4,
	register_select_LDCTRL = 0x8
};

static void initiate_transmit(device_t *device);
static void send_break(device_t *device, bool state);
static DEVICE_RESET( tms9902 );

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE tms9902_t *get_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == TMS9902);
	return (tms9902_t *) downcast<legacy_device_base *>(device)->token();
}


/*
    should be called after any change to int_state or enabled_ints.
*/
static void field_interrupts(device_t *device)
{
	tms9902_t *tms9902 = get_token(device);
	bool new_int = (tms9902->DSCH && tms9902->DSCENB)
							|| (tms9902->RBRL && tms9902->RIENB)
							|| (tms9902->XBRE && tms9902->XBIENB)
							|| (tms9902->TIMELP && tms9902->TIMENB);
// if (VERBOSE>3) logerror("TMS9902: interrupt flags (DSCH = %02x, DSCENB = %02x), (RBRL = %02x, RIENB = %02x), (XBRE = %02x, XBIENB = %02x), (TIMELP = %02x, TIMENB = %02x)\n",
//  tms9902->DSCH, tms9902->DSCENB, tms9902->RBRL, tms9902->RIENB, tms9902->XBRE, tms9902->XBIENB, tms9902->TIMELP, tms9902->TIMENB);
	if (new_int != tms9902->INT)
	{
		// Only consider edges
		tms9902->INT = new_int;
		if (VERBOSE>3) logerror("TMS9902: /INT = %s\n", (tms9902->INT)? "asserted" : "cleared");
		if (tms9902->intf->int_callback)
			(*tms9902->intf->int_callback)(device, tms9902->INT? ASSERT_LINE : CLEAR_LINE);
	}
}

/*
    Called whenever the incoming CTS* line changes. This should be called by
    the device that contains the UART.
*/
void tms9902_rcv_cts(device_t *device, line_state state)
{
	tms9902_t *tms9902 = get_token(device);
	bool previous = tms9902->CTSflag;
	// CTS is an internal register of the TMS9902 with positive logic
	tms9902->CTSflag = (state==ASSERT_LINE)? 1:0;
	tms9902->DSCH = 1;

	if (VERBOSE>3) logerror("TMS9902: CTS* = %s\n", (state==ASSERT_LINE)? "asserted" : "cleared");

	if (tms9902->CTSflag != previous)
	{
		field_interrupts(device);

		if (!state && tms9902->RTSflag && tms9902->XSRE)
		{
			if ((! tms9902->XBRE) && !tms9902->BRKflag)
				initiate_transmit(device);
			else if (tms9902->BRKON)
				send_break(device, 1);
		}
	}
}

void tms9902_clock(device_t *device, bool state)
{
	tms9902_t *tms9902 = get_token(device);
	if (state)
		tms9902->optimer->adjust(attotime::from_msec(1), 0, attotime::from_hz(POLLING_FREQ));
	else
		tms9902->optimer->reset();
}

/*
    Called whenever the incoming DSR* line changes. This should be called by
    the device that contains the UART.
*/
void tms9902_rcv_dsr(device_t *device, line_state state)
{
	tms9902_t *tms9902 = get_token(device);
	bool previous = tms9902->DSRflag;
	if (VERBOSE>3) logerror("TMS9902: DSR* = %s\n", (state==ASSERT_LINE)? "asserted" : "cleared");
	tms9902->DSRflag = (state==ASSERT_LINE)? true:false;
	tms9902->DSCH = 1;

	if (tms9902->DSRflag != previous)
	{
		field_interrupts(device);
	}
}

/*
    Called whenever the incoming RIN line changes. This should be called by
    the device that contains the UART. Unlike the real thing, we deliver
    complete bytes in one go.
*/
void tms9902_rcv_data(device_t *device, UINT8 data)
{
	tms9902_t *tms9902 = get_token(device);

	// Put the received byte into the 1-byte receive buffer
	tms9902->RBR = data;

	// Clear last errors
	tms9902->RFER = 0;
	tms9902->RPER = 0;

	if (!tms9902->RBRL)
	{
		// Receive buffer was empty
		tms9902->RBRL = 1;
		if (VERBOSE>3) logerror("TMS9902: Receive buffer loaded with byte %02x\n", data);
		field_interrupts(device);
	}
	else
	{
		// Receive buffer was full
		tms9902->ROVER = 1;
		if (VERBOSE>1) logerror("TMS9902: Receive buffer still loaded; overflow error\n");
	}
}

//------------------------------------------------

/*
    Framing error. This can only be detected by a remotely attached real UART;
    if we get a report on a framing error we use it to announce the framing error
    as if it occured here.
    The flag is reset by the next correctly received byte.
*/
void tms9902_rcv_framing_error(device_t *device)
{
	tms9902_t *tms9902 = get_token(device);
	if (VERBOSE>2) logerror("TMS9902: Detected framing error\n");
	tms9902->RFER = 1;
}

/*
    Parity error. This can only be detected by a remotely attached real UART;
    if we get a report on a parity error we use it to announce the parity error
    as if it occured here.
    The flag is reset by the next correctly received byte.
*/
void tms9902_rcv_parity_error(device_t *device)
{
	tms9902_t *tms9902 = get_token(device);
	if (VERBOSE>2) logerror("TMS9902: Detected parity error\n");
	tms9902->RPER = 1;
}

/*
    Incoming BREAK condition. The TMS9902 does not show any directly visible
    reactions on a BREAK (no interrupt, no flag set). A BREAK is a time period
    of low level on the RIN pin which makes the chip re-synchronize on the
    next rising edge.
*/
void tms9902_rcv_break(device_t *device, bool value)
{
	if (VERBOSE>2) logerror("TMS9902: Receive BREAK=%d (no effect)\n", value? 1:0);
}

//------------------------------------------------


/*
    This call-back is called by the MESS timer system when the decrementer
    reaches 0.
*/
static TIMER_CALLBACK(decrementer_callback)
{
	device_t *device = (device_t *) ptr;
	tms9902_t *tms9902 = get_token(device);

	if (tms9902->TIMELP)
		tms9902->TIMERR = 1;
	else
		tms9902->TIMELP = 1;
}

/*
    Callback for the autonomous operations of the chip. This is normally
    controlled by an external clock of 3-4 MHz, internally divided by 3 or 4,
    depending on CLK4M. With this timer, reception of characters becomes
    possible.
    We deliver the baudpoll value which allows the callback function to know
    when the next data byte shall be delivered.
*/
static TIMER_CALLBACK(op_callback)
{
	device_t *device = (device_t *) ptr;
	tms9902_t *tms9902 = get_token(device);

	if (tms9902->intf->rcv_callback)
		(*tms9902->intf->rcv_callback)(device, tms9902->baudpoll);
}

/*
    load the content of clockinvl into the decrementer
*/
static void reload_interval_timer(device_t *device)
{
	tms9902_t *tms9902 = get_token(device);
	if (tms9902->TMR)
	{	/* reset clock interval */
		tms9902->dectimer->adjust(
						attotime::from_double((double) tms9902->TMR / (tms9902->clock_rate / ((tms9902->CLK4M) ? 4. : 3.) / 64.)),
						0,
						attotime::from_double((double) tms9902->TMR / (tms9902->clock_rate / ((tms9902->CLK4M) ? 4. : 3.) / 64.)));
	}
	else
	{	/* clock interval == 0 -> no timer */
		tms9902->dectimer->enable(0);
	}
}

static void send_break(device_t *device, bool state)
{
	tms9902_t *tms9902 = get_token(device);

	if (state != tms9902->BRKflag)
	{
		tms9902->BRKflag = state;

		// Signal BRK (on/off) to the remote site
		if (tms9902->intf->ctrl_callback)
			(*tms9902->intf->ctrl_callback)(device, EXCEPT, BRK, state? 1:0);
	}
}

// ==========================================================================

/*
    Sets the data rate for the receiver part. If a remote UART is attached,
    propagate this setting.
    The TMS9902 calculates the baud rate from the external clock, and the result
    does not match the known baud rates precisely (e.g. for 9600 baud the
    closest value is 9615). Other UARTs may have a different way to set baud
    rates. Thus we transmit the bit pattern and leave it up to the remote UART
    to calculate its own baud rate from it. Apart from that, the callback
    function should add information about the UART.

    CLK4M RDV8 RDR9 RDR8 | RDR7 RDR6 RDR5 RDR4 | RDR3 RDR2 RDR1 RDR0
*/
static void set_receive_data_rate(device_t *device)
{
	tms9902_t *tms9902 = get_token(device);
	int value = (tms9902->CLK4M? 0x800 : 0) | (tms9902->RDV8? 0x400 : 0) | tms9902->RDR;
	if (VERBOSE>3) logerror("receive rate = %04x\n", value);

	// Calculate the ratio between receive baud rate and polling frequency
	double fint = tms9902->clock_rate / ((tms9902->CLK4M) ? 4.0 : 3.0);
	double baud = fint / (2.0 * ((tms9902->RDV8)? 8:1) * tms9902->RDR);

	// We assume 10 bit per character (7 data usually add 1 parity; 1 start, 1 stop)
	// This value represents the ratio of data inputs of one poll.
	// Thus the callback function should add up this value on each poll
	// and deliver a data input not before it sums up to 1.
	tms9902->baudpoll = (double)(baud / (10*POLLING_FREQ));
	if (VERBOSE>3) logerror ("baudpoll = %lf\n", tms9902->baudpoll);

	if (tms9902->intf->ctrl_callback)
		(*tms9902->intf->ctrl_callback)(device, CONFIG, RATERECV, value);
}

/*
    Sets the data rate for the sender part. If a remote UART is attached,
    propagate this setting.
*/
static void set_transmit_data_rate(device_t *device)
{
	tms9902_t *tms9902 = get_token(device);
	int value = (tms9902->CLK4M? 0x800 : 0) | (tms9902->XDV8? 0x400 : 0) | tms9902->XDR;
	if (VERBOSE>3) logerror("transmit rate = %04x\n", value);
	if (tms9902->intf->ctrl_callback)
		(*tms9902->intf->ctrl_callback)(device, CONFIG, RATEXMIT, value);
}

static void set_stop_bits(device_t *device)
{
	tms9902_t *tms9902 = get_token(device);
	int value = tms9902->STOPB;
	if (tms9902->intf->ctrl_callback)
		(*tms9902->intf->ctrl_callback)(device, CONFIG, STOPBITS, value);
}

static void set_data_bits(device_t *device)
{
	tms9902_t *tms9902 = get_token(device);
	int value = tms9902->RCL;
	if (tms9902->intf->ctrl_callback)
		(*tms9902->intf->ctrl_callback)(device, CONFIG, DATABITS, value);
}

static void set_parity(device_t *device)
{
	tms9902_t *tms9902 = get_token(device);
	int value = (tms9902->PENB? 2:0) | (tms9902->ODDP? 1:0);
	if (tms9902->intf->ctrl_callback)
		(*tms9902->intf->ctrl_callback)(device, CONFIG, PARITY, value);
}

static void transmit_line_state(device_t *device)
{
	tms9902_t *tms9902 = get_token(device);

	// 00ab cdef = setting line RTS=a, CTS=b, DSR=c, DCD=d, DTR=e, RI=f
	// The 9902 only outputs RTS and BRK

	if (tms9902->intf->ctrl_callback)
		(*tms9902->intf->ctrl_callback)(device, LINES, RTS, (tms9902->RTSflag)? RTS : 0);
}

static void set_rts(device_t *device, line_state state)
{
	tms9902_t *tms9902 = get_token(device);
	bool lstate = (state==ASSERT_LINE);

	if (lstate != tms9902->RTSflag)
	{
		// Signal RTS to the modem
		if (VERBOSE>3) logerror("TMS9902: Set RTS=%d\n", lstate? 1:0);
		tms9902->RTSflag = lstate;
		transmit_line_state(device);
	}
}

// ==========================================================================

static void initiate_transmit(device_t *device)
{
	tms9902_t *tms9902 = get_token(device);

	/* Load transmit register */
	tms9902->XSR = tms9902->XBR;
	tms9902->XSRE = false;
	tms9902->XBRE = true;

	field_interrupts(device);

	if (VERBOSE>4) logerror("TMS9902: transmit XSR=%02x, RCL=%02x\n", tms9902->XSR, tms9902->RCL);

	if (tms9902->intf->xmit_callback)
		(*tms9902->intf->xmit_callback)(device, tms9902->XSR & (0xff >> (3-tms9902->RCL)));

	tms9902->XSRE = true;

	if ((!tms9902->XBRE) /*&& tms9902->RTS*/ && tms9902->CTSflag /*&& ! tms9902->BRK*/)
		/* load next byte */
		initiate_transmit(device);
	else if (tms9902->BRKON /*&& tms9902->RTS*/ && tms9902->CTSflag)
		/* enter break mode */
		send_break(device, true);
	else if ((! tms9902->RTSON) && ((! tms9902->CTSflag) || (tms9902->XBRE && !tms9902->BRKflag)))
		/* clear RTS output */
		set_rts(device, CLEAR_LINE);
}



/*----------------------------------------------------------------
    TMS9902 CRU interface.
----------------------------------------------------------------*/

/*
    Read a 8 bit chunk from tms9902.

    signification:
    bit 0-7: RBR0-7 Receive Buffer register
    bit 8: not used (always 0)
    bit 9: RCVERR Receive Error (RFER | ROVER | RPER)
    bit 10: RPER Receive Parity Error
    bit 11: ROVER Receive Overrun Error
    bit 12: RFER Receive Framing Error
    bit 13-15: not emulated, normally used for diagnostics
    bit 16: RBINT (RBRL&RIENB)
*/
READ8_DEVICE_HANDLER( tms9902_cru_r )
{
	UINT8 answer = 0;
	tms9902_t *tms9902 = get_token(device);

	offset &= 0x0003;

	switch (offset)
	{
	case 3:
		if (tms9902->INT) answer |= 0x80;
		if ((tms9902->register_select != 0) || tms9902->BRKON) answer |= 0x40;
		if (tms9902->DSCH) answer |= 0x20;
		if (tms9902->CTSflag) answer |= 0x10;
		if (tms9902->DSRflag) answer |= 0x08;
		if (tms9902->RTSflag) answer |= 0x04;
		if (tms9902->TIMELP)  answer |= 0x02;
		if (tms9902->TIMERR)  answer |= 0x01;
		break;

	case 2:
		if (tms9902->XSRE) answer |= 0x80;
		if (tms9902->XBRE) answer |= 0x40;
		if (tms9902->RBRL) answer |= 0x20;
		if (tms9902->DSCH & tms9902->DSCENB) answer |= 0x10;
		if (tms9902->TIMELP & tms9902->TIMENB) answer |= 0x08;
		if (tms9902->XBRE & tms9902->XBIENB) answer |= 0x02;
		if (tms9902->RBRL & tms9902->RIENB) answer |= 0x01;
		break;

	case 1:
		if (tms9902->RIN) answer |= 0x80;
		if (tms9902->RSBD) answer |= 0x40;
		if (tms9902->RFBD) answer |= 0x20;
		if (tms9902->RFER) answer |= 0x10;
		if (tms9902->ROVER) answer |= 0x08;
		if (tms9902->RPER) answer |= 0x04;
		if (tms9902->RCVERR) answer |= 0x02;
		break;

	case 0:
		answer = tms9902->RBR;
		if (VERBOSE>3) logerror("TMS9902: Reading receive buffer\n");
		break;
	}
	return answer;
}

static inline void set_bits8(UINT8 *reg, UINT8 bits, bool set)
{
	if (set)
		*reg |= bits;
	else
		*reg &= ~bits;
}

static inline void set_bits16(UINT16 *reg, UINT16 bits, bool set)
{
	if (set)
		*reg |= bits;
	else
		*reg &= ~bits;
}

/*
    TMS9902 CRU write
*/
WRITE8_DEVICE_HANDLER( tms9902_cru_w )
{
	tms9902_t *tms9902 = get_token(device);

	data &= 1;	/* clear extra bits */

	offset &= 0x1F;

	if (offset <= 0x0a)
	{
		UINT16 mask = (1 << offset);

		if (tms9902->register_select & register_select_LDCTRL)
		{	/* Control Register */
			switch (offset)
			{
			case 0:
				set_bits8(&tms9902->RCL, 0x01, (data!=0));
				// we assume that bits are written in increasing order
				// so we do not transmit the data bits twice
				break;
			case 1:
				set_bits8(&tms9902->RCL, 0x02, (data!=0));
				set_data_bits(device);
				break;
			case 2:
				break;
			case 3:
				tms9902->CLK4M = (data!=0);
				break;
			case 4:
				tms9902->ODDP = (data!=0);
				// we also assume that the parity type is set before the parity enable
				break;
			case 5:
				tms9902->PENB = (data!=0);
				set_parity(device);
				break;
			case 6:
				set_bits8(&tms9902->STOPB, 0x01, (data!=0));
				break;
			case 7:
				set_bits8(&tms9902->STOPB, 0x02, (data!=0));
				tms9902->register_select &= ~register_select_LDCTRL;
				set_stop_bits(device);
				break;
			default:
				if (VERBOSE>1) logerror("tms9902: Invalid control register address %d\n", offset);
			}
		}
		else if (tms9902->register_select & register_select_LDIR)
		{	/* Interval Register */
			if (offset <= 7)
			{
				set_bits8(&tms9902->TMR, mask, (data!=0));

				if (offset == 7)
				{
					reload_interval_timer(device);
					tms9902->register_select &= ~register_select_LDIR;
				}
			}
		}
		else if (tms9902->register_select & (register_select_LRDR | register_select_LXDR))
		{	/* Receive&Transmit Data Rate Register */
			if (offset < 10)
			{
				if (tms9902->register_select & register_select_LRDR)
				{
					set_bits16(&tms9902->RDR, mask, (data!=0));
				}
				if (tms9902->register_select & register_select_LXDR)
				{
					set_bits16(&tms9902->XDR, mask, (data!=0));
				}
			}
			else if (offset == 10)
			{
				if (tms9902->register_select & register_select_LRDR)
				{
					tms9902->RDV8 = (data!=0);
					set_receive_data_rate(device);
				}
				if (tms9902->register_select & register_select_LXDR)
				{
					tms9902->XDV8 = (data!=0);
					set_transmit_data_rate(device);
				}
				tms9902->register_select &= ~ (register_select_LRDR | register_select_LXDR);
			}
		}
		else
		{	/* Transmit Buffer Register */
			if (offset <= 7)
			{
				set_bits8(&tms9902->XBR, mask, (data!=0));

				if (offset == 7)
				{	/* transmit */
					tms9902->XBRE = 0;
					// Spec: When the transmitter is active, the contents of the Transmit
					// Buffer Register are transferred to the Transmit Shift Register
					// each time the previous character has been completely transmitted
					if (tms9902->XSRE && tms9902->RTSflag && tms9902->CTSflag && !tms9902->BRKflag)
					{
						initiate_transmit(device);
					}
				}
			}
		}
		return;
	}
	if (offset >= 0x0b && offset <= 0x0e)
	{
		int mask = 1 << (offset - 0x0b);

		if ((offset == 0x0d) && (data==0) && (tms9902->register_select & register_select_LDIR))
		{
			/* clearing LDIR reloads the interval timer (right???) */
			reload_interval_timer(device);
		}

		set_bits8(&tms9902->register_select, mask, (data!=0));
		return;
	}

	if (offset == 0x0f)
	{
		/* test mode; not implemented */
		return;
	}

	if (offset == 0x10)
	{
		if (data!=0)
		{
			tms9902->RTSON = true;
			set_rts(device, ASSERT_LINE);
			if (tms9902->CTSflag)
			{
				if (tms9902->XSRE && !tms9902->XBRE && !tms9902->BRKflag)
					initiate_transmit(device);
				else if (tms9902->BRKON)
					send_break(device, true);
			}
		}
		else
		{
			tms9902->RTSON = false;
			if (tms9902->XBRE && tms9902->XSRE && !tms9902->BRKflag)
			{
				set_rts(device, CLEAR_LINE);
			}
		}
		return;
	}

	if (offset == 0x11)
	{
		if (VERBOSE>3) logerror("TMS9902: set BRKON=%d; BRK=%d\n", data, tms9902->BRKflag? 1:0);
		tms9902->BRKON = (data!=0);
		if (tms9902->BRKflag && data==0)
		{
			// clear BRK
			tms9902->BRKflag = false;
			if ((!tms9902->XBRE) && tms9902->CTSflag)
			{
				/* transmit next byte */
				initiate_transmit(device);
			}
			else if (!tms9902->RTSON)
			{
				/* clear RTS */
				set_rts(device, CLEAR_LINE);
			}
		}
		else if (tms9902->XBRE && tms9902->XSRE && tms9902->RTSflag && tms9902->CTSflag)
		{
			send_break(device, (data!=0));
		}
		return;
	}

	if (offset == 0x12)
	{	// Receiver Interrupt Enable
		// According to spec, (re)setting this flag clears the RBRL flag
		// (the only way to clear the flag!)
		tms9902->RIENB = (data!=0);
		tms9902->RBRL = false;
		if (VERBOSE>4) logerror("TMS9902: set RBRL=0, set RIENB=%d\n", data);
		field_interrupts(device);
		return;
	}

	if (offset == 0x13)
	{
		/* Transmit Buffer Interrupt Enable */
		tms9902->XBIENB = (data!=0);
		if (VERBOSE>4) logerror("TMS9902: set XBIENB=%d\n", data);
		field_interrupts(device);
		return;
	}

	if (offset == 0x14)
	{
		/* Timer Interrupt Enable */
		tms9902->TIMENB = (data!=0);
		tms9902->TIMELP = false;
		tms9902->TIMERR = false;
		field_interrupts(device);
		return;
	}

	if (offset == 0x15)
	{
		/* Data Set Change Interrupt Enable */
		tms9902->DSCENB = (data!=0);
		tms9902->DSCH = false;
		if (VERBOSE>4) logerror("TMS9902: set DSCH=0, set DSCENB=%d\n", data);
		field_interrupts(device);
		return;
	}

	/* Bits 0x16 - 0x1e not used. */

	if (offset == 0x1f)
	{
		/* RESET */
		DEVICE_RESET_CALL( tms9902 );
		return;
	}
	else
		if (VERBOSE>1) logerror("tms9902: Write to undefined address %0x ignored.\n", offset);
}


/*-------------------------------------------------
    DEVICE_STOP( tms9902 )
-------------------------------------------------*/

static DEVICE_STOP( tms9902 )
{
	tms9902_t *tms9902 = get_token(device);

	if (tms9902->dectimer)
	{
		tms9902->dectimer->reset();
		tms9902->dectimer = NULL;
	}
}

/*-------------------------------------------------
    DEVICE_RESET( tms9902 )
-------------------------------------------------*/

static DEVICE_RESET( tms9902 )
{
	tms9902_t *tms9902 = get_token(device);

	/*  disable all interrupts */
	tms9902->DSCENB = false;	// Data Set Change Interrupt Enable
	tms9902->TIMENB = false;	// Timer Interrupt Enable
	tms9902->XBIENB = false;	// Transmit Buffer Interrupt Enable
	tms9902->RIENB = false;		// Read Buffer Interrupt Enable

	/* initialize transmitter */
	tms9902->XBRE = true;		// Transmit Buffer Register Empty
	tms9902->XSRE = true;		// Transmit Shift Register Empty

	/* initialize receiver */
	tms9902->RBRL = false;		// Read Buffer Register Loaded

	/* clear RTS */
	tms9902->RTSON = false;		// Request-to-send on (flag)
	tms9902->RTSflag = false;		// Request-to-send line

	/* set all resister load flags to 1 */
	tms9902->register_select = 0xf;

	/* clear break condition */
	tms9902->BRKON = false;
	tms9902->BRKflag = false;

	field_interrupts(device);
}


/*-------------------------------------------------
    DEVICE_START( tms9902 )
-------------------------------------------------*/

static DEVICE_START( tms9902 )
{
	tms9902_t *tms9902 = get_token(device);

	assert(device != NULL);
	assert(device->tag() != NULL);
	assert(device->static_config() != NULL);

	tms9902->intf = (const tms9902_interface*)device->static_config();

	tms9902->clock_rate = tms9902->intf->clock_rate;

	tms9902->dectimer = device->machine().scheduler().timer_alloc(FUNC(decrementer_callback), (void *) device);
	tms9902->optimer = device->machine().scheduler().timer_alloc(FUNC(op_callback), (void *) device);
}



/*-------------------------------------------------
    DEVICE_GET_INFO( tms9902 )
-------------------------------------------------*/

DEVICE_GET_INFO( tms9902 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(tms9902_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(tms9902);	break;
		case DEVINFO_FCT_STOP:							info->stop  = DEVICE_STOP_NAME (tms9902);	break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(tms9902);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "TMS9902");					break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "TMS9902");					break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						/* Nothing */								break;
	}
}

DEFINE_LEGACY_DEVICE(TMS9902, tms9902);
