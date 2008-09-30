/***************************************************************************

    Z80 DART (Z8470) implementation

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "driver.h"
#include "z80dart.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"



/***************************************************************************
    DEBUGGING
***************************************************************************/

#define VERBOSE		1

#define VPRINTF(x) do { if (VERBOSE) logerror x; } while (0)



/***************************************************************************
    CONSTANTS
***************************************************************************/

/* interrupt states */
#define INT_CHB_TRANSMIT	0x00		/* not confirmed */
#define INT_CHB_STATUS		0x01
#define INT_CHB_RECEIVE		0x02
#define INT_CHB_ERROR		0x03

#define INT_CHA_TRANSMIT	0x04		/* not confirmed */
#define INT_CHA_STATUS		0x05
#define INT_CHA_RECEIVE		0x06
#define INT_CHA_ERROR		0x07

/* DART write register 0 */
#define DART_WR0_RESET_MASK					0xc0	/* D7-D6: Reset control */
#define DART_WR0_RESET_NULL					0x00	/*  00 = NULL code */
#define DART_WR0_RESET_RX_CRC				0x40	/*  01 = Reset Rx CRC checker */
#define DART_WR0_RESET_TX_CRC				0x80	/*  10 = Reset Tx CRC generator */
#define DART_WR0_RESET_TX_LATCH				0xc0	/*  11 = Reset Tx Underrun/EOM latch */
#define DART_WR0_COMMAND_MASK				0x38	/* D5-D3: Command */
#define DART_WR0_COMMAND_NULL				0x00	/*  000 = NULL code */
													/*  001 = not used */
#define DART_WR0_COMMAND_RES_STATUS_INT		0x10	/*  010 = reset ext/status interrupts */
#define DART_WR0_COMMAND_CH_RESET			0x18	/*  011 = Channel reset */
#define DART_WR0_COMMAND_ENA_RX_INT			0x20	/*  100 = Enable int on next Rx character */
#define DART_WR0_COMMAND_RES_TX_INT			0x28	/*  101 = Reset Tx int pending */
#define DART_WR0_COMMAND_RES_ERROR			0x30	/*  110 = Error reset */
#define DART_WR0_COMMAND_RETI				0x38	/*  111 = Return from int (CH-A only) */
#define DART_WR0_REGISTER_MASK				0x07	/* D2-D0: Register select (0-7) */

/* DART write register 1 */
#define DART_WR1_READY_WAIT_ENA				0x80	/* D7 = READY/WAIT enable */
#define DART_WR1_READY_WAIT_FUNCTION		0x40	/* D6 = READY/WAIT function */
#define DART_WR1_READY_WAIT_ON_RT			0x20	/* D5 = READY/WAIT on R/T */
#define DART_WR1_RXINT_MASK					0x18	/* D4-D3 = Rx int control */
#define DART_WR1_RXINT_DISABLE				0x00	/*  00 = Rx int disable */
#define DART_WR1_RXINT_FIRST				0x08	/*  01 = Rx int on first character */
#define DART_WR1_RXINT_ALL_PARITY			0x10	/*  10 = int on all Rx characters (parity affects vector) */
#define DART_WR1_RXINT_ALL_NOPARITY			0x18	/*  11 = int on all Rx characters (parity ignored) */
#define DART_WR1_STATUS_AFFECTS_VECTOR		0x04	/* D2 = Status affects vector (CH-B only) */
#define DART_WR1_TXINT_ENABLE				0x02	/* D1 = Tx int enable */
#define DART_WR1_STATUSINT_ENABLE			0x01	/* D0 = Ext int enable */

/* DART write register 2 (CH-B only) */
#define DART_WR2_INT_VECTOR_MASK			0xff	/* D7-D0 = interrupt vector */

/* DART write register 3 */
#define DART_WR3_RX_DATABITS_MASK			0xc0	/* D7-D6 = Rx Data bits */
#define DART_WR3_RX_DATABITS_5				0x00	/*  00 = Rx 5 bits/character */
#define DART_WR3_RX_DATABITS_7				0x40	/*  01 = Rx 7 bits/character */
#define DART_WR3_RX_DATABITS_6				0x80	/*  10 = Rx 6 bits/character */
#define DART_WR3_RX_DATABITS_8				0xc0	/*  11 = Rx 8 bits/character */
#define DART_WR3_AUTO_ENABLES				0x20	/* D5 = Auto enables */
													/* D1-D4 = not used */
#define DART_WR3_RX_ENABLE					0x01	/* D0 = Rx enable */

/* DART write register 4 */
#define DART_WR4_CLOCK_MODE_MASK			0xc0	/* D7-D6 = Clock mode */
#define DART_WR4_CLOCK_MODE_x1				0x00	/*  00 = x1 clock mode */
#define DART_WR4_CLOCK_MODE_x16				0x40	/*  01 = x16 clock mode */
#define DART_WR4_CLOCK_MODE_x32				0x80	/*  10 = x32 clock mode */
#define DART_WR4_CLOCK_MODE_x64				0xc0	/*  11 = x64 clock mode */
													/* D5-D4 = not used */
#define DART_WR4_STOPBITS_MASK				0x0c	/* D3-D2 = Stop bits */
													/*  00 = not used */
#define DART_WR4_STOPBITS_1					0x04	/*  01 = 1 stop bit/character */
#define DART_WR4_STOPBITS_15				0x08	/*  10 = 1.5 stop bits/character */
#define DART_WR4_STOPBITS_2					0x0c	/*  11 = 2 stop bits/character */
#define DART_WR4_PARITY_EVEN				0x02	/* D1 = Parity even/odd */
#define DART_WR4_PARITY_ENABLE				0x01	/* D0 = Parity enable */

/* DART write register 5 */
#define DART_WR5_DTR						0x80	/* D7 = DTR */
#define DART_WR5_TX_DATABITS_MASK			0x60	/* D6-D5 = Tx Data bits */
#define DART_WR5_TX_DATABITS_5				0x00	/*  00 = Tx 5 bits/character */
#define DART_WR5_TX_DATABITS_7				0x20	/*  01 = Tx 7 bits/character */
#define DART_WR5_TX_DATABITS_6				0x40	/*  10 = Tx 6 bits/character */
#define DART_WR5_TX_DATABITS_8				0x60	/*  11 = Tx 8 bits/character */
#define DART_WR5_SEND_BREAK					0x10	/* D4 = Send break */
#define DART_WR5_TX_ENABLE					0x08	/* D3 = Tx Enable */
													/* D2 = not used */
#define DART_WR5_RTS						0x02	/* D1 = RTS */
													/* D0 = not used */

/* DART read register 0 */
#define DART_RR0_BREAK_ABORT				0x80	/* D7 = Break/abort */
													/* D6 = not used */
#define DART_RR0_CTS						0x20	/* D5 = CTS */
#define DART_RR0_RI							0x10	/* D4 = RI */
#define DART_RR0_DCD						0x08	/* D3 = DCD */
#define DART_RR0_TX_BUFFER_EMPTY			0x04	/* D2 = Tx buffer empty */
#define DART_RR0_INT_PENDING				0x02	/* D1 = int pending (CH-A only) */
#define DART_RR0_RX_CHAR_AVAILABLE			0x01	/* D0 = Rx character available */

/* DART read register 1 */
													/* D7 = not used */
#define DART_RR1_CRC_FRAMING_ERROR			0x40	/* D6 = Framing error */
#define DART_RR1_RX_OVERRUN_ERROR			0x20	/* D5 = Rx overrun error */
#define DART_RR1_PARITY_ERROR				0x10	/* D4 = Parity error */
													/* D3-D1 = not used */
#define DART_RR1_ALL_SENT					0x01	/* D0 = All sent */

/* DART read register 2 (CH-B only) */
#define DART_RR2_VECTOR_MASK				0xff	/* D7-D0 = Interrupt vector */



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _dart_channel dart_channel;
struct _dart_channel
{
	UINT8		regs[6];			/* 6 writeable registers */
	UINT8		status[4];			/* 3 readable registers */
	int			inbuf;				/* input buffer */
	int			outbuf;				/* output buffer */
	UINT8		int_on_next_rx;		/* interrupt on next rx? */
	emu_timer *receive_timer;		/* timer to clock data in */
	UINT8		receive_buffer[16];	/* buffer for incoming data */
	UINT8		receive_inptr;		/* index of data coming in */
	UINT8		receive_outptr;		/* index of data going out */
};


typedef struct _z80dart z80dart;
struct _z80dart
{
	dart_channel	chan[2];			/* 2 channels */
	UINT8		int_state[8];		/* interrupt states */

	void (*irq_cb)(const device_config *device, int state);
	write8_device_func dtr_changed_cb;
	write8_device_func rts_changed_cb;
	write8_device_func break_changed_cb;
	write8_device_func transmit_cb;
	int (*receive_poll_cb)(const device_config *device, int ch);
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static TIMER_CALLBACK(serial_callback);
static int z80dart_irq_state(const device_config *device);



/*

    Interrupt priorities:
        Ch A receive
        Ch A transmit
        Ch A external/status
        Ch B receive
        Ch B transmit
        Ch B external/status


    Initial configuration (both channels):
        005D:dart_reg_w(0,4) = 44
                    01 = x16 clock mode
                    00 = 8 bit sync character
                    01 = 1 stop bit/character
                    Parity odd
                    Parity disabled

        005D:dart_reg_w(0,3) = C1
                    11 = Rx 8 bits/character
                    No auto enables
                    No enter hunt phase
                    No Rx CRC enable
                    No address search mode
                    No sync character load inhibit
                    Rx enable

        005D:dart_reg_w(0,5) = 68
                    DTR = 0
                    11 = Tx 8 bits/character
                    No send break
                    Tx enable
                    SDLC
                    No RTS
                    No CRC enable

        005D:dart_reg_w(0,2) = 40
                    Vector = 0x40

        005D:dart_reg_w(0,1) = 1D
                    No READY/WAIT
                    No READY/WAIT function
                    No READY/WAIT on R/T
                    11 = int on all Rx characters (parity ignored)
                    Status affects vector
                    No Tx int enable
                    Ext int enable

*/


/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE z80dart *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == Z80DART);
	return (z80dart *)device->token;
}


INLINE void interrupt_check(const device_config *device)
{
	z80dart *dart = get_safe_token( device );

	/* if we have a callback, update it with the current state */
	if (dart->irq_cb != NULL)
		dart->irq_cb(device, (z80dart_irq_state(device) & Z80_DAISY_INT) ? ASSERT_LINE : CLEAR_LINE);
}


INLINE attotime compute_time_per_character(const device_config *device, int ch)
{
	/* fix me -- should compute properly and include data, stop, parity bits */
	return ATTOTIME_IN_HZ(9600 / 10);
}


INLINE void reset_channel(const device_config *device, int ch)
{
	z80dart *dart = get_safe_token( device );
	attotime tpc = compute_time_per_character(device, ch);
	dart_channel *chan = &dart->chan[ch];

	chan->status[0] = DART_RR0_TX_BUFFER_EMPTY;
	chan->status[1] = 0x00;
	chan->status[2] = 0x00;
	chan->int_on_next_rx = 0;
	chan->outbuf = -1;

	dart->int_state[0 + 4*ch] = 0;
	dart->int_state[1 + 4*ch] = 0;
	dart->int_state[2 + 4*ch] = 0;
	dart->int_state[3 + 4*ch] = 0;

	/* start the receive timer running */
	timer_adjust_periodic(chan->receive_timer, tpc, ch, tpc);
}



/***************************************************************************
    CONTROL REGISTER READ/WRITE
***************************************************************************/

/*-------------------------------------------------
    z80dart_c_w - write to a control register
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( z80dart_c_w )
{
	z80dart *dart = get_safe_token( device );
	dart_channel *chan = &dart->chan[offset];
	int reg = chan->regs[0] & 7;
	UINT8 old = chan->regs[reg];

	if (reg != 0 || (reg & 0xf8))
		VPRINTF(("%04X:dart_reg_w(%c,%d) = %02X\n", activecpu_get_pc(), 'A' + offset, reg, data));

	/* write a new value to the selected register */
	chan->regs[reg] = data;

	/* clear the register number for the next write */
	if (reg != 0)
		chan->regs[0] &= ~7;

	/* switch off the register for live state changes */
	switch (reg)
	{
		/* DART write register 0 */
		case 0:
			switch (data & DART_WR0_COMMAND_MASK)
			{
				case DART_WR0_COMMAND_CH_RESET:
					VPRINTF(("%04X:DART reset channel %c\n", activecpu_get_pc(), 'A' + offset));
					reset_channel(device, offset);
					interrupt_check(device);
					break;

				case DART_WR0_COMMAND_RES_STATUS_INT:
					dart->int_state[INT_CHA_STATUS - 4*offset] &= ~Z80_DAISY_INT;
					interrupt_check(device);
					break;

				case DART_WR0_COMMAND_ENA_RX_INT:
					chan->int_on_next_rx = TRUE;
					interrupt_check(device);
					break;

				case DART_WR0_COMMAND_RES_TX_INT:
					dart->int_state[INT_CHA_TRANSMIT - 4*offset] &= ~Z80_DAISY_INT;
					interrupt_check(device);
					break;

				case DART_WR0_COMMAND_RES_ERROR:
					dart->int_state[INT_CHA_ERROR - 4*offset] &= ~Z80_DAISY_INT;
					interrupt_check(device);
					break;
			}
			break;

		/* DART write register 1 */
		case 1:
			interrupt_check(device);
			break;

		/* DART write register 5 */
		case 5:
			if (((old ^ data) & DART_WR5_DTR) && dart->dtr_changed_cb)
				(*dart->dtr_changed_cb)(device, offset, (data & DART_WR5_DTR) != 0);
			if (((old ^ data) & DART_WR5_SEND_BREAK) && dart->break_changed_cb)
				(*dart->break_changed_cb)(device, offset, (data & DART_WR5_SEND_BREAK) != 0);
			if (((old ^ data) & DART_WR5_RTS) && dart->rts_changed_cb)
				(*dart->rts_changed_cb)(device, offset, (data & DART_WR5_RTS) != 0);
			break;
	}
}


/*-------------------------------------------------
    z80dart_c_r - read from a control register
-------------------------------------------------*/

READ8_DEVICE_HANDLER( z80dart_c_r )
{
	z80dart *dart = get_safe_token( device );
	dart_channel *chan = &dart->chan[offset];
	int reg = chan->regs[0] & 7;
	UINT8 result = chan->status[reg];

	/* switch off the register for live state changes */
	switch (reg)
	{
		/* DART read register 0 */
		case 0:
			result &= ~DART_RR0_INT_PENDING;
			if (z80dart_irq_state(device) & Z80_DAISY_INT)
				result |= DART_RR0_INT_PENDING;
			break;
	}

	VPRINTF(("%04X:dart_reg_r(%c,%d) = %02x\n", activecpu_get_pc(), 'A' + offset, reg, chan->status[reg]));

	return chan->status[reg];
}




/***************************************************************************
    DATA REGISTER READ/WRITE
***************************************************************************/

/*-------------------------------------------------
    z80dart_d_w - write to a data register
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( z80dart_d_w )
{
	z80dart *dart = get_safe_token( device );
	dart_channel *chan = &dart->chan[offset];

	VPRINTF(("%04X:dart_data_w(%c) = %02X\n", activecpu_get_pc(), 'A' + offset, data));

	/* if tx not enabled, just ignore it */
	if (!(chan->regs[5] & DART_WR5_TX_ENABLE))
		return;

	/* update the status register */
	chan->status[0] &= ~DART_RR0_TX_BUFFER_EMPTY;

	/* reset the transmit interrupt */
	dart->int_state[INT_CHA_TRANSMIT - 4*offset] &= ~Z80_DAISY_INT;
	interrupt_check(device);

	/* stash the character */
	chan->outbuf = data;
}


/*-------------------------------------------------
    z80dart_d_r - read from a data register
-------------------------------------------------*/

READ8_DEVICE_HANDLER( z80dart_d_r )
{
	z80dart *dart = get_safe_token( device );
	dart_channel *chan = &dart->chan[offset];

	/* update the status register */
	chan->status[0] &= ~DART_RR0_RX_CHAR_AVAILABLE;

	/* reset the receive interrupt */
	dart->int_state[INT_CHA_RECEIVE - 4*offset] &= ~Z80_DAISY_INT;
	interrupt_check(device);

	VPRINTF(("%04X:dart_data_r(%c) = %02X\n", activecpu_get_pc(), 'A' + offset, chan->inbuf));

	return chan->inbuf;
}



/***************************************************************************
    CONTROL LINE READ/WRITE
***************************************************************************/

/*-------------------------------------------------
    z80dart_get_dtr - return the state of the DTR
    line
-------------------------------------------------*/

int z80dart_get_dtr(const device_config *device, int ch)
{
	z80dart *dart = get_safe_token( device );
	dart_channel *chan = &dart->chan[ch];
	return ((chan->regs[5] & DART_WR5_DTR) != 0);
}


/*-------------------------------------------------
    z80dart_get_rts - return the state of the RTS
    line
-------------------------------------------------*/

int z80dart_get_rts(const device_config *device, int ch)
{
	z80dart *dart = get_safe_token( device );
	dart_channel *chan = &dart->chan[ch];
	return ((chan->regs[5] & DART_WR5_RTS) != 0);
}


/*-------------------------------------------------
    z80dart_set_cts - set the state of the CTS
    line
-------------------------------------------------*/

static TIMER_CALLBACK(change_input_line)
{
	const device_config *device = ptr;
	z80dart *dart = get_safe_token( device );
	dart_channel *chan = &dart->chan[param & 1];
	UINT8 line = (param >> 8) & 0xff;
	int state = (param >> 7) & 1;
	int ch = param & 1;
	UINT8 old;
	char linename[4];

	switch (line)
	{
	case DART_RR0_CTS:
		sprintf(linename, "%s", "CTS");
		break;
	case DART_RR0_DCD:
		sprintf(linename, "%s", "DCD");
		break;
	case DART_RR0_RI:
		sprintf(linename, "%s", "RI");
		break;
	}
	VPRINTF(("dart_change_input_line(%c, %s) = %d\n", 'A' + ch, linename, state));

	/* remember the old value */
	old = chan->status[0];

	/* set the bit in the status register */
	chan->status[0] &= ~line;
	if (state)
		chan->status[0] |= line;

	/* if state change interrupts are enabled, signal */
	if (((old ^ chan->status[0]) & line) && (chan->regs[1] & DART_WR1_STATUSINT_ENABLE))
	{
		dart->int_state[INT_CHA_STATUS - 4*ch] |= Z80_DAISY_INT;
		interrupt_check(device);
	}
}


/*-------------------------------------------------
    z80dart_set_cts - set the state of the CTS
    line
-------------------------------------------------*/

void z80dart_set_cts(const device_config *device, int ch, int state)
{
	/* operate deferred */
	timer_set(attotime_zero, (void *)device, (DART_RR0_CTS << 8) + (state != 0) * 0x80 + ch, change_input_line);
}


/*-------------------------------------------------
    z80dart_set_dcd - set the state of the DCD
    line
-------------------------------------------------*/

void z80dart_set_dcd(const device_config *device, int ch, int state)
{
	/* operate deferred */
	timer_set(attotime_zero, (void *)device, (DART_RR0_DCD << 8) + (state != 0) * 0x80 + ch, change_input_line);
}


/*-------------------------------------------------
    z80dart_set_ri - set the state of the RIA
    line
-------------------------------------------------*/

void z80dart_set_ri(const device_config *device, int state)
{
	/* operate deferred */
	timer_set(attotime_zero, (void *)device, (DART_RR0_RI << 8) + (state != 0) * 0x80, change_input_line);
}


/*-------------------------------------------------
    z80dart_receive_data - receive data on the
    input lines
-------------------------------------------------*/

void z80dart_receive_data(const device_config *device, int ch, UINT8 data)
{
	z80dart *dart = get_safe_token( device );
	dart_channel *chan = &dart->chan[ch];
	int newinptr;

	/* put it on the queue */
	newinptr = (chan->receive_inptr + 1) % ARRAY_LENGTH(chan->receive_buffer);
	if (newinptr != chan->receive_outptr)
	{
		chan->receive_buffer[chan->receive_inptr] = data;
		chan->receive_inptr = newinptr;
	}
	else
		logerror("z80dart_receive_data: buffer overrun\n");
}


/*-------------------------------------------------
    serial_callback - callback to pump
    data through
-------------------------------------------------*/

static TIMER_CALLBACK(serial_callback)
{
	const device_config *device = ptr;
	z80dart *dart = get_safe_token( device );
	dart_channel *chan = &dart->chan[param & 1];
	int ch = param & 1;
	int data = -1;

	/* first perform any outstanding transmits */
	if (chan->outbuf != -1)
	{
		VPRINTF(("serial_callback(%c): Transmitting %02x\n", 'A' + ch, chan->outbuf));

		/* actually transmit the character */
		if (dart->transmit_cb != NULL)
			(*dart->transmit_cb)(device, ch, chan->outbuf);

		/* update the status register */
		chan->status[0] |= DART_RR0_TX_BUFFER_EMPTY;

		/* set the transmit buffer empty interrupt if enabled */
		if (chan->regs[1] & DART_WR1_TXINT_ENABLE)
		{
			dart->int_state[INT_CHA_TRANSMIT - 4*ch] |= Z80_DAISY_INT;
			interrupt_check(device);
		}

		/* reset the output buffer */
		chan->outbuf = -1;
	}

	/* ask the polling callback if there is data to receive */
	if (dart->receive_poll_cb != NULL)
		data = (*dart->receive_poll_cb)(device, ch);

	/* if we have buffered data, pull it */
	if (chan->receive_inptr != chan->receive_outptr)
	{
		data = chan->receive_buffer[chan->receive_outptr];
		chan->receive_outptr = (chan->receive_outptr + 1) % ARRAY_LENGTH(chan->receive_buffer);
	}

	/* if we have data, receive it */
	if (data != -1)
	{
		VPRINTF(("serial_callback(%c): Receiving %02x\n", 'A' + ch, data));

		/* if rx not enabled, just ignore it */
		if (!(chan->regs[3] & DART_WR3_RX_ENABLE))
		{
			VPRINTF(("  (ignored because receive is disabled)\n"));
			return;
		}

		/* stash the data and update the status */
		chan->inbuf = data;
		chan->status[0] |= DART_RR0_RX_CHAR_AVAILABLE;

		/* update our interrupt state */
		switch (chan->regs[1] & DART_WR1_RXINT_MASK)
		{
			case DART_WR1_RXINT_FIRST:
				if (!chan->int_on_next_rx)
					break;

			case DART_WR1_RXINT_ALL_NOPARITY:
			case DART_WR1_RXINT_ALL_PARITY:
				dart->int_state[INT_CHA_RECEIVE - 4*ch] |= Z80_DAISY_INT;
				interrupt_check(device);
				break;
		}
		chan->int_on_next_rx = FALSE;
	}
}



/***************************************************************************
    DAISY CHAIN INTERFACE
***************************************************************************/

static const UINT8 int_priority[] =
{
	INT_CHA_RECEIVE,
	INT_CHA_TRANSMIT,
	INT_CHA_STATUS,
	INT_CHA_ERROR,
	INT_CHB_RECEIVE,
	INT_CHB_TRANSMIT,
	INT_CHB_STATUS,
	INT_CHB_ERROR
};


static int z80dart_irq_state(const device_config *device)
{
	z80dart *dart = get_safe_token( device );
	int state = 0;
	int i;

	VPRINTF(("dart IRQ state = B:%d%d%d%d A:%d%d%d%d\n",
				dart->int_state[0], dart->int_state[1], dart->int_state[2], dart->int_state[3],
				dart->int_state[4], dart->int_state[5], dart->int_state[6], dart->int_state[7]));

	/* loop over all interrupt sources */
	for (i = 0; i < 8; i++)
	{
		int inum = int_priority[i];

		/* if we're servicing a request, don't indicate more interrupts */
		if (dart->int_state[inum] & Z80_DAISY_IEO)
		{
			state |= Z80_DAISY_IEO;
			break;
		}
		state |= dart->int_state[inum];
	}

	return state;
}


static int z80dart_irq_ack(const device_config *device)
{
	z80dart *dart = get_safe_token( device );
	int i;

	/* loop over all interrupt sources */
	for (i = 0; i < 8; i++)
	{
		int inum = int_priority[i];

		/* find the first channel with an interrupt requested */
		if (dart->int_state[inum] & Z80_DAISY_INT)
		{
			VPRINTF(("dart IRQAck %d\n", inum));

			/* clear interrupt, switch to the IEO state, and update the IRQs */
			dart->int_state[inum] = Z80_DAISY_IEO;
			interrupt_check(device);
			return dart->chan[1].regs[2] + inum * 2;
		}
	}

	logerror("z80dart_irq_ack: failed to find an interrupt to ack!\n");
	return dart->chan[1].regs[2];
}


static void z80dart_irq_reti(const device_config *device)
{
	z80dart *dart = get_safe_token( device );
	int i;

	/* loop over all interrupt sources */
	for (i = 0; i < 8; i++)
	{
		int inum = int_priority[i];

		/* find the first channel with an IEO pending */
		if (dart->int_state[inum] & Z80_DAISY_IEO)
		{
			VPRINTF(("dart IRQReti %d\n", inum));

			/* clear the IEO state and update the IRQs */
			dart->int_state[inum] &= ~Z80_DAISY_IEO;
			interrupt_check(device);
			return;
		}
	}

	logerror("z80dart_irq_reti: failed to find an interrupt to clear IEO on!\n");
}


static DEVICE_START( z80dart )
{
	const z80dart_interface *intf = device->static_config;
	z80dart *dart = get_safe_token(device);

	dart->chan[0].receive_timer = timer_alloc(serial_callback, (void *)device);
	dart->chan[1].receive_timer = timer_alloc(serial_callback, (void *)device);

	dart->irq_cb = intf->irq_cb;
	dart->dtr_changed_cb = intf->dtr_changed_cb;
	dart->rts_changed_cb = intf->rts_changed_cb;
	dart->break_changed_cb = intf->break_changed_cb;
	dart->transmit_cb = intf->transmit_cb;
	dart->receive_poll_cb = intf->receive_poll_cb;

	return DEVICE_START_OK;
}


static DEVICE_RESET( z80dart )
{
	int ch;

	/* set up defaults */
	for (ch = 0; ch < 2; ch++)
	{
		reset_channel( device, ch );
	}
	interrupt_check(device);
}


static DEVICE_SET_INFO( z80dart )
{
	switch (state)
	{
		/* no parameters to set */
	}
}


DEVICE_GET_INFO( z80dart )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(z80dart);						break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;									break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_SET_INFO:						info->set_info = DEVICE_SET_INFO_NAME(z80dart);	break;
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(z80dart);		break;
		case DEVINFO_FCT_STOP:							/* Nothing */									break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(z80dart);		break;
		case DEVINFO_FCT_IRQ_STATE:						info->f = (genf *)z80dart_irq_state;			break;
		case DEVINFO_FCT_IRQ_ACK:						info->f = (genf *)z80dart_irq_ack;				break;
		case DEVINFO_FCT_IRQ_RETI:						info->f = (genf *)z80dart_irq_reti;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							info->s = "Zilog Z80 DART";						break;
		case DEVINFO_STR_FAMILY:						info->s = "Z80";								break;
		case DEVINFO_STR_VERSION:						info->s = "1.0";								break;
		case DEVINFO_STR_SOURCE_FILE:					info->s = __FILE__;								break;
		case DEVINFO_STR_CREDITS:						info->s = "Copyright the MESS Team";			break;
	}
}

