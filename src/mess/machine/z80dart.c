/***************************************************************************

    Z80 DART Dual Asynchronous Receiver/Transmitter emulation

    Copyright (c) 2008, The MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

/*

	TODO:

	- save state support
	- break detection
	- wr0 reset tx interrupt pending
	- wait/ready
	- 1.5 stop bits

*/

#include "driver.h"
#include "z80dart.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"

/***************************************************************************
    DEBUGGING
***************************************************************************/

#define VERBOSE 0

#define LOGERROR if (VERBOSE) logerror

/***************************************************************************
    CONSTANTS
***************************************************************************/

enum
{
	STATE_START = 0,
	STATE_DATA,
	STATE_PARITY,
	STATE_STOP,
	STATE_STOP2
};

enum
{
	INT_TRANSMIT = 0,
	INT_EXTERNAL,
	INT_RECEIVE,
	INT_SPECIAL
};

#define Z80DART_RR0_RX_CHAR_AVAILABLE	0x01
#define Z80DART_RR0_INTERRUPT_PENDING	0x02
#define Z80DART_RR0_TX_BUFFER_EMPTY		0x04
#define Z80DART_RR0_DCD					0x08
#define Z80DART_RR0_RI					0x10
#define Z80DART_RR0_CTS					0x20
#define Z80DART_RR0_BREAK				0x80 /* not supported */

#define Z80DART_RR1_ALL_SENT			0x01
#define Z80DART_RR1_PARITY_ERROR		0x10
#define Z80DART_RR1_RX_OVERRUN_ERROR	0x20
#define Z80DART_RR1_FRAMING_ERROR		0x40

#define Z80DART_WR0_REGISTER_MASK		0x07
#define Z80DART_WR0_COMMAND_MASK		0x38
#define Z80DART_WR0_NULL_CODE			0x00
#define Z80DART_WR0_RESET_EXT_STATUS	0x10
#define Z80DART_WR0_CHANNEL_RESET		0x18
#define Z80DART_WR0_ENABLE_INT_NEXT_RX	0x20
#define Z80DART_WR0_RESET_TX_INT		0x28 /* not supported */
#define Z80DART_WR0_ERROR_RESET			0x30
#define Z80DART_WR0_RETURN_FROM_INT		0x38 /* not supported */

#define Z80DART_WR1_EXT_INT_ENABLE		0x01
#define Z80DART_WR1_TX_INT_ENABLE		0x02
#define Z80DART_WR1_STATUS_VECTOR		0x04
#define Z80DART_WR1_RX_INT_ENABLE_MASK	0x18
#define Z80DART_WR1_RX_INT_DISABLE		0x00
#define Z80DART_WR1_RX_INT_FIRST		0x08
#define Z80DART_WR1_RX_INT_ALL_PARITY	0x10 /* not supported */
#define Z80DART_WR1_RX_INT_ALL			0x18
#define Z80DART_WR1_WRDY_ON_RX_TX		0x20 /* not supported */
#define Z80DART_WR1_WRDY_FUNCTION		0x40 /* not supported */
#define Z80DART_WR1_WRDY_ENABLE			0x80 /* not supported */

#define Z80DART_WR3_RX_ENABLE			0x01
#define Z80DART_WR3_AUTO_ENABLES		0x20
#define Z80DART_WR3_RX_WORD_LENGTH_MASK	0xc0
#define Z80DART_WR3_RX_WORD_LENGTH_5	0x00
#define Z80DART_WR3_RX_WORD_LENGTH_7	0x40
#define Z80DART_WR3_RX_WORD_LENGTH_6	0x80
#define Z80DART_WR3_RX_WORD_LENGTH_8	0xc0

#define Z80DART_WR4_PARITY_ENABLE		0x01 /* not supported */
#define Z80DART_WR4_PARITY_EVEN			0x02 /* not supported */
#define Z80DART_WR4_STOP_BITS_MASK		0x0c
#define Z80DART_WR4_STOP_BITS_1			0x04
#define Z80DART_WR4_STOP_BITS_1_5		0x08 /* not supported */
#define Z80DART_WR4_STOP_BITS_2			0x0c
#define Z80DART_WR4_CLOCK_MODE_MASK		0xc0
#define Z80DART_WR4_CLOCK_MODE_X1		0x00
#define Z80DART_WR4_CLOCK_MODE_X16		0x40
#define Z80DART_WR4_CLOCK_MODE_X32		0x80
#define Z80DART_WR4_CLOCK_MODE_X64		0xc0

#define Z80DART_WR5_RTS					0x02
#define Z80DART_WR5_TX_ENABLE			0x08
#define Z80DART_WR5_SEND_BREAK			0x10
#define Z80DART_WR5_TX_WORD_LENGTH_MASK	0x60
#define Z80DART_WR5_TX_WORD_LENGTH_5	0x00
#define Z80DART_WR5_TX_WORD_LENGTH_7	0x40
#define Z80DART_WR5_TX_WORD_LENGTH_6	0x80
#define Z80DART_WR5_TX_WORD_LENGTH_8	0xc0
#define Z80DART_WR5_DTR					0x80

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _dart_channel dart_channel;
struct _dart_channel
{
	/* register state */
	UINT8 rr[3];			/* read register */
	UINT8 wr[6];			/* write register */

	/* receiver state */
	UINT8 rx_data_fifo[3];	/* receive data FIFO */
	UINT8 rx_error_fifo[3];	/* receive error FIFO */
	UINT8 rx_shift;			/* 8-bit receive shift register */
	UINT8 rx_error;			/* current receive error */
	int rx_fifo;			/* receive FIFO pointer */

	int rx_clock;			/* receive clock pulse count */
	int rx_state;			/* receive state */
	int rx_bits;			/* bits received */
	int rx_first;			/* first character received */
	int rx_parity;			/* received data parity */
	int rx_break;			/* receive break condition */
	int rx_rr0_latch;		/* read register 0 latched */

	int ri;					/* ring indicator latch */
	int cts;				/* clear to send latch */
	int dcd;				/* data carrier detect latch */

	/* transmitter state */
	UINT8 tx_data;			/* transmit data register */
	UINT8 tx_shift;			/* transmit shift register */

	int tx_clock;			/* transmit clock pulse count */
	int tx_state;			/* transmit state */
	int tx_bits;			/* bits transmitted */
	int tx_parity;			/* transmitted data parity */

	int dtr;				/* data terminal ready */
	int rts;				/* request to send */
};

typedef struct _z80dart_t z80dart_t;
struct _z80dart_t
{
	const z80dart_interface *intf;	/* interface */

	dart_channel channel[2];		/* channels */

	int clock;						/* base clock */
	int int_state[8];				/* interrupt state */

	/* timers */
	emu_timer *rxca_timer;
	emu_timer *txca_timer;
	emu_timer *rxtxcb_timer;
};

/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static int z80dart_irq_state(const device_config *device);
static void z80dart_irq_reti(const device_config *device);

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE z80dart_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == Z80DART);
	
	return (z80dart_t *)device->token;
}

/***************************************************************************
    CALLBACK INTERFACE MANAGEMENT
***************************************************************************/

static int z80dart_rxd_r(const device_config *device, int channel)
{
	z80dart_t *z80dart = get_safe_token(device);

	int data = 1;

	if (z80dart->intf->rxd_r)
	{
		data = z80dart->intf->rxd_r(device, channel);
	}

	return data;
}

static void z80dart_txd_w(const device_config *device, int channel, int state)
{
	z80dart_t *z80dart = get_safe_token(device);

	if (z80dart->intf->on_txd_changed)
	{
		z80dart->intf->on_txd_changed(device, channel, state);
	}
}

static void z80dart_rts_w(const device_config *device, int channel, int state)
{
	z80dart_t *z80dart = get_safe_token(device);

	if (z80dart->intf->on_rts_changed)
	{
		z80dart->intf->on_rts_changed(device, channel, state);
	}

	LOGERROR("Z80DART \"%s\" Channel %c : RTS %u\n", device->tag, 'A' + channel, state);
}

static void z80dart_dtr_w(const device_config *device, int channel, int state)
{
	z80dart_t *z80dart = get_safe_token(device);

	if (z80dart->intf->on_dtr_changed)
	{
		z80dart->intf->on_dtr_changed(device, channel, state);
	}

	LOGERROR("Z80DART \"%s\" Channel %c : DTR %u\n", device->tag, 'A' + channel, state);
}

/***************************************************************************
    INTERNAL STATE MANAGEMENT
***************************************************************************/

static void z80dart_check_interrupt(const device_config *device)
{
	z80dart_t *z80dart = get_safe_token(device);
	int state = (z80dart_irq_state(device) & Z80_DAISY_INT) ? ASSERT_LINE : CLEAR_LINE;

	if (z80dart->intf->on_int_changed)
	{
		z80dart->intf->on_int_changed(device, state);
	}
}

static void z80dart_trigger_interrupt(const device_config *device, int channel, int level)
{
	z80dart_t *z80dart = get_safe_token(device);
	dart_channel *ch = &z80dart->channel[channel];
	UINT8 vector = ch->wr[2];
	int priority = (channel << 2) | level;

	LOGERROR("Z80DART \"%s\" Channel %c : Interrupt Request %u\n", device->tag, 'A' + channel, level);

	if ((channel == Z80DART_CH_B) && (ch->wr[1] & Z80DART_WR1_STATUS_VECTOR))
	{
		/* status affects vector */
		vector = (ch->wr[2] & 0xf1) | (!channel << 3) | (level << 1);
	}

	/* update vector register */
	ch->rr[2] = vector;

	/* trigger interrupt */
	z80dart->int_state[priority] |= Z80_DAISY_INT;
	z80dart->channel[Z80DART_CH_A].rr[0] |= Z80DART_RR0_INTERRUPT_PENDING;

	/* check for interrupt */
	z80dart_check_interrupt(device);
}

static int z80dart_get_clock_mode(const device_config *device, int channel)
{
	z80dart_t *z80dart = get_safe_token(device);
	dart_channel *ch = &z80dart->channel[channel];

	int clocks = 1;

	switch (ch->wr[4] & Z80DART_WR4_CLOCK_MODE_MASK)
	{
	case Z80DART_WR4_CLOCK_MODE_X1:
		clocks = 1;

	case Z80DART_WR4_CLOCK_MODE_X16:
		clocks = 16;

	case Z80DART_WR4_CLOCK_MODE_X32:
		clocks = 32;

	case Z80DART_WR4_CLOCK_MODE_X64:
		clocks = 64;
	}

	return clocks;
}

static float z80dart_get_stop_bits(const device_config *device, int channel)
{
	z80dart_t *z80dart = get_safe_token(device);
	dart_channel *ch = &z80dart->channel[channel];

	float bits = 1;

	switch (ch->wr[4] & Z80DART_WR4_STOP_BITS_MASK)
	{
	case Z80DART_WR4_STOP_BITS_1:
		bits = 1;

	case Z80DART_WR4_STOP_BITS_1_5:
		bits = 1.5;

	case Z80DART_WR4_STOP_BITS_2:
		bits = 2;
	}

	return bits;
}

static int z80dart_get_rx_word_length(const device_config *device, int channel)
{
	z80dart_t *z80dart = get_safe_token(device);
	dart_channel *ch = &z80dart->channel[channel];

	int bits = 5;

	switch (ch->wr[3] & Z80DART_WR3_RX_WORD_LENGTH_MASK)
	{
	case Z80DART_WR3_RX_WORD_LENGTH_5:
		bits = 5;

	case Z80DART_WR3_RX_WORD_LENGTH_6:
		bits = 6;

	case Z80DART_WR3_RX_WORD_LENGTH_7:
		bits = 7;

	case Z80DART_WR3_RX_WORD_LENGTH_8:
		bits = 8;
	}

	return bits;
}

static int z80dart_get_tx_word_length(const device_config *device, int channel)
{
	z80dart_t *z80dart = get_safe_token(device);
	dart_channel *ch = &z80dart->channel[channel];

	int bits = 5;

	switch (ch->wr[5] & Z80DART_WR5_TX_WORD_LENGTH_MASK)
	{
	case Z80DART_WR5_TX_WORD_LENGTH_5:
		bits = 5;

	case Z80DART_WR5_TX_WORD_LENGTH_6:
		bits = 6;

	case Z80DART_WR5_TX_WORD_LENGTH_7:
		bits = 7;

	case Z80DART_WR5_TX_WORD_LENGTH_8:
		bits = 8;
	}

	return bits;
}

static void z80dart_reset_channel(const device_config *device, int channel)
{
	z80dart_t *z80dart = get_safe_token(device);
	dart_channel *ch = &z80dart->channel[channel];

	/* disable receiver */
	ch->wr[3] &= ~Z80DART_WR3_RX_ENABLE;
	ch->rx_state = STATE_START;

	/* disable transmitter */
	ch->wr[5] &= ~Z80DART_WR5_TX_ENABLE;
	ch->tx_state = STATE_START;

	/* reset external lines */
	z80dart_rts_w(device, channel, 1);
	z80dart_dtr_w(device, channel, 1);

	if (channel == Z80DART_CH_A)
	{
		/* reset interrupt logic */
		int i;

		for (i = 0; i < 8; i++)
		{
			z80dart->int_state[i] = 0;
		}

		z80dart_check_interrupt(device);
	}
}

/***************************************************************************
    SERIAL RECEIVE
***************************************************************************/

static int detect_start_bit(const device_config *device, int channel)
{
	z80dart_t *z80dart = get_safe_token(device);
	dart_channel *ch = &z80dart->channel[channel];
	
	if (!(ch->wr[3] & Z80DART_WR3_RX_ENABLE))
		return 0;

	return !z80dart_rxd_r(device, channel);
}

static void shift_data_in(const device_config *device, int channel)
{
	z80dart_t *z80dart = get_safe_token(device);
	dart_channel *ch = &z80dart->channel[channel];

	if (ch->rx_bits < 8)
	{
		int rxd = z80dart_rxd_r(device, channel);

		ch->rx_shift >>= 1;
		ch->rx_shift = (rxd << 7) | (ch->rx_shift & 0x7f);
		ch->rx_parity ^= rxd;
		ch->rx_bits++;
	}
}

static int character_completed(const device_config *device, int channel)
{
	z80dart_t *z80dart = get_safe_token(device);
	dart_channel *ch = &z80dart->channel[channel];

	return ch->rx_bits == z80dart_get_rx_word_length(device, channel);
}

static void detect_parity_error(const device_config *device, int channel)
{
	z80dart_t *z80dart = get_safe_token(device);
	dart_channel *ch = &z80dart->channel[channel];
	
	int rxd = z80dart_rxd_r(device, channel);
	int parity = (ch->wr[1] & Z80DART_WR4_PARITY_EVEN) ? 1 : 0;

	if (rxd != (ch->rx_parity ^ parity))
	{
		/* parity error detected */
		ch->rx_error |= Z80DART_RR1_PARITY_ERROR;

		switch (ch->wr[1] & Z80DART_WR1_RX_INT_ENABLE_MASK)
		{
		case Z80DART_WR1_RX_INT_FIRST:
			if (!ch->rx_first)
			{
				z80dart_trigger_interrupt(device, channel, INT_SPECIAL);
			}
			break;
		
		case Z80DART_WR1_RX_INT_ALL_PARITY:
			z80dart_trigger_interrupt(device, channel, INT_SPECIAL);
			break;
		
		case Z80DART_WR1_RX_INT_ALL:
			z80dart_trigger_interrupt(device, channel, INT_RECEIVE);
			break;
		}
	}
}

static void detect_framing_error(const device_config *device, int channel)
{
	z80dart_t *z80dart = get_safe_token(device);
	dart_channel *ch = &z80dart->channel[channel];
	
	int rxd = z80dart_rxd_r(device, channel);

	if (!rxd)
	{
		/* framing error detected */
		ch->rx_error |= Z80DART_RR1_FRAMING_ERROR;

		switch (ch->wr[1] & Z80DART_WR1_RX_INT_ENABLE_MASK)
		{
		case Z80DART_WR1_RX_INT_FIRST:
			if (!ch->rx_first)
			{
				z80dart_trigger_interrupt(device, channel, INT_SPECIAL);
			}
			break;
		
		case Z80DART_WR1_RX_INT_ALL_PARITY:
		case Z80DART_WR1_RX_INT_ALL:
			z80dart_trigger_interrupt(device, channel, INT_SPECIAL);
			break;
		}
	}
}

static void receive(const device_config *device, int channel)
{
	z80dart_t *z80dart = get_safe_token(device);
	dart_channel *ch = &z80dart->channel[channel];
	
	float stop_bits = z80dart_get_stop_bits(device, channel);

	switch (ch->rx_state)
	{
	case STATE_START:
		/* check for start bit */
		if (detect_start_bit(device, channel))
		{
			/* start bit detected */
			ch->rx_shift = 0;
			ch->rx_error = 0;
			ch->rx_bits = 0;
			ch->rx_parity = 0;

			/* next bit is a data bit */
			ch->rx_state = STATE_DATA;
		}
		break;

	case STATE_DATA:
		/* shift bit into shift register */
		shift_data_in(device, channel);
		
		if (character_completed(device, channel))
		{
			/* all data bits received */
			if (ch->wr[4] & Z80DART_WR4_PARITY_ENABLE)
			{
				/* next bit is the parity bit */
				ch->rx_state = STATE_PARITY;
			}
			else
			{
				/* next bit is a STOP bit */
				if (stop_bits == 1)
					ch->rx_state = STATE_STOP2;
				else
					ch->rx_state = STATE_STOP;
			}
		}
		break;

	case STATE_PARITY:
		/* shift bit into shift register */
		shift_data_in(device, channel);

		/* check for parity error */
		detect_parity_error(device, channel);

		/* next bit is a STOP bit */
		if (stop_bits == 1)
			ch->rx_state = STATE_STOP2;
		else
			ch->rx_state = STATE_STOP;
		break;

	case STATE_STOP:
		/* shift bit into shift register */
		shift_data_in(device, channel);

		/* check for framing error */
		detect_framing_error(device, channel);

		/* next bit is the second STOP bit */
		ch->rx_state = STATE_STOP2;
		break;

	case STATE_STOP2:
		/* shift bit into shift register */
		shift_data_in(device, channel);

		/* check for framing error */
		detect_framing_error(device, channel);

		/* store data into FIFO */
		z80dart_receive_data(device, channel, ch->rx_shift);

		/* next bit is the START bit */
		ch->rx_state = STATE_START;
		break;
	}
}

/***************************************************************************
    SERIAL TRANSMIT
***************************************************************************/

#define TXD(state) z80dart_txd_w(device, channel, 1);

static void transmit(const device_config *device, int channel)
{
	z80dart_t *z80dart = get_safe_token(device);
	dart_channel *ch = &z80dart->channel[channel];
	
	int word_length = z80dart_get_tx_word_length(device, channel);
	float stop_bits = z80dart_get_stop_bits(device, channel);

	switch (ch->tx_state)
	{
	case STATE_START:
		if ((ch->wr[5] & Z80DART_WR5_TX_ENABLE) && !(ch->rr[0] & Z80DART_RR0_TX_BUFFER_EMPTY))
		{
			/* transmit start bit */
			TXD(0);

			ch->tx_bits = 0;
			ch->tx_shift = ch->tx_data;

			/* empty transmit buffer */
			ch->rr[0] |= Z80DART_RR0_TX_BUFFER_EMPTY;

			if (ch->wr[1] & Z80DART_WR1_TX_INT_ENABLE)
				z80dart_trigger_interrupt(device, channel, INT_TRANSMIT);

			ch->tx_state = STATE_DATA;
		}
		else if (ch->wr[5] & Z80DART_WR5_SEND_BREAK)
		{
			/* transmit break */
			TXD(0);
		}
		else
		{
			/* transmit marking line */
			TXD(1);
		}

		break;

	case STATE_DATA:
		/* transmit data bit */
		TXD(BIT(ch->tx_shift, 0));

		/* shift data */
		ch->tx_shift >>= 1;
		ch->tx_bits++;

		if (ch->rx_bits == word_length)
		{
			if (ch->wr[4] & Z80DART_WR4_PARITY_ENABLE)
				ch->tx_state = STATE_PARITY;
			else
			{
				if (stop_bits == 1)
					ch->tx_state = STATE_STOP2;
				else
					ch->tx_state = STATE_STOP;
			}
		}
		break;

	case STATE_PARITY:
		// TODO: calculate parity
		if (stop_bits == 1)
			ch->tx_state = STATE_STOP2;
		else
			ch->tx_state = STATE_STOP;
		break;

	case STATE_STOP:
		/* transmit stop bit */
		TXD(1);

		ch->tx_state = STATE_STOP2;
		break;

	case STATE_STOP2:
		/* transmit stop bit */
		TXD(1);

		/* if transmit buffer is empty */
		if (ch->rr[0] & Z80DART_RR0_TX_BUFFER_EMPTY)
		{
			/* then all characters have been sent */
			ch->rr[1] |= Z80DART_RR1_ALL_SENT;

			/* when the RTS bit is reset, the _RTS output goes high after the transmitter empties */
			if (!ch->rts)
				z80dart_rts_w(device, channel, 1);
		}

		ch->tx_state = STATE_START;
		break;
	}
}

/***************************************************************************
    CONTROL REGISTER READ/WRITE
***************************************************************************/

READ8_DEVICE_HANDLER( z80dart_c_r )
{
	z80dart_t *z80dart = get_safe_token(device);
	int channel = offset & 0x01;
	dart_channel *ch = &z80dart->channel[channel];
	UINT8 data = 0;

	int reg = ch->wr[0] & Z80DART_WR0_REGISTER_MASK;

	switch (reg)
	{
	case 0:
	case 1:
		data = ch->rr[reg];
		break;

	case 2:
		/* channel B only */
		if (channel == Z80DART_CH_B)
			data = ch->rr[reg];
		break;
	}

	LOGERROR("Z80DART \"%s\" Channel %c : Control Register Read '%02x'\n", device->tag, 'A' + channel, data);

	return data;
}

WRITE8_DEVICE_HANDLER( z80dart_c_w )
{
	z80dart_t *z80dart = get_safe_token(device);
	int channel = offset & 0x01;
	dart_channel *ch = &z80dart->channel[channel];

	int reg = ch->wr[0] & Z80DART_WR0_REGISTER_MASK;

	LOGERROR("Z80DART \"%s\" Channel %c : Control Register Write '%02x'\n", device->tag, 'A' + channel, data);

	/* write data to selected register */
	ch->wr[reg] = data;

	if (reg != 0)
	{
		/* mask out register index */
		ch->wr[0] &= ~Z80DART_WR0_REGISTER_MASK;
	}

	switch (reg)
	{
	case 0:
		switch (data & Z80DART_WR0_COMMAND_MASK)
		{
		case Z80DART_WR0_RESET_EXT_STATUS:
			/* reset external/status interrupt */
			ch->rr[0] &= ~(Z80DART_RR0_DCD | Z80DART_RR0_RI | Z80DART_RR0_CTS | Z80DART_RR0_BREAK);

			if (!ch->dcd) ch->rr[0] |= Z80DART_RR0_DCD;
			if (ch->ri) ch->rr[0] |= Z80DART_RR0_RI;
			if (ch->cts) ch->rr[0] |= Z80DART_RR0_CTS;

			ch->rx_rr0_latch = 0;

			LOGERROR("Z80DART \"%s\" Channel %c : Reset External/Status Interrupt\n", device->tag, 'A' + channel);
			break;

		case Z80DART_WR0_CHANNEL_RESET:
			/* channel reset */
			LOGERROR("Z80DART \"%s\" Channel %c : Channel Reset\n", device->tag, 'A' + channel);
			z80dart_reset_channel(device, channel);
			break;

		case Z80DART_WR0_ENABLE_INT_NEXT_RX:
			/* enable interrupt on next receive character */
			LOGERROR("Z80DART \"%s\" Channel %c : Enable Interrupt on Next Received Character\n", device->tag, 'A' + channel);
			ch->rx_first = 1;
			break;

		case Z80DART_WR0_RESET_TX_INT:
			/* reset transmitter interrupt pending */
			LOGERROR("Z80DART \"%s\" Channel %c : Reset Transmitter Interrupt Pending\n", device->tag, 'A' + channel);
			break;

		case Z80DART_WR0_ERROR_RESET:
			/* error reset */
			LOGERROR("Z80DART \"%s\" Channel %c : Error Reset\n", device->tag, 'A' + channel);
			ch->rr[1] &= ~(Z80DART_RR1_FRAMING_ERROR | Z80DART_RR1_RX_OVERRUN_ERROR | Z80DART_RR1_PARITY_ERROR);
			break;

		case Z80DART_WR0_RETURN_FROM_INT:
			/* return from interrupt */
			LOGERROR("Z80DART \"%s\" Channel %c : Return from Interrupt\n", device->tag, 'A' + channel);
			z80dart_irq_reti(device);
			break;
		}
		break;

	case 1:
		LOGERROR("Z80DART \"%s\" Channel %c : External Interrupt Enable %u\n", device->tag, 'A' + channel, (data & Z80DART_WR1_EXT_INT_ENABLE) ? 1 : 0);
		LOGERROR("Z80DART \"%s\" Channel %c : Transmit Interrupt Enable %u\n", device->tag, 'A' + channel, (data & Z80DART_WR1_TX_INT_ENABLE) ? 1 : 0);
		LOGERROR("Z80DART \"%s\" Channel %c : Status Affects Vector %u\n", device->tag, 'A' + channel, (data & Z80DART_WR1_STATUS_VECTOR) ? 1 : 0);
		LOGERROR("Z80DART \"%s\" Channel %c : Wait/Ready Enable %u\n", device->tag, 'A' + channel, (data & Z80DART_WR1_WRDY_ENABLE) ? 1 : 0);
		
		switch (data & Z80DART_WR1_RX_INT_ENABLE_MASK)
		{
		case Z80DART_WR1_RX_INT_DISABLE:
			LOGERROR("Z80DART \"%s\" Channel %c : Receiver Interrupt Disabled\n", device->tag, 'A' + channel);
			break;

		case Z80DART_WR1_RX_INT_FIRST:
			LOGERROR("Z80DART \"%s\" Channel %c : Receiver Interrupt on First Character\n", device->tag, 'A' + channel);
			break;

		case Z80DART_WR1_RX_INT_ALL_PARITY:
			LOGERROR("Z80DART \"%s\" Channel %c : Receiver Interrupt on All Characters, Parity Affects Vector\n", device->tag, 'A' + channel);
			break;

		case Z80DART_WR1_RX_INT_ALL:
			LOGERROR("Z80DART \"%s\" Channel %c : Receiver Interrupt on All Characters\n", device->tag, 'A' + channel);
			break;
		}

		z80dart_check_interrupt(device);
		break;

	case 2:
		/* interrupt vector */
		z80dart_check_interrupt(device);
		LOGERROR("Z80DART \"%s\" Channel %c : Interrupt Vector %02x\n", device->tag, 'A' + channel, data);
		break;

	case 3:
		LOGERROR("Z80DART \"%s\" Channel %c : Receiver Enable %u\n", device->tag, 'A' + channel, (data & Z80DART_WR3_RX_ENABLE) ? 1 : 0);
		LOGERROR("Z80DART \"%s\" Channel %c : Auto Enables %u\n", device->tag, 'A' + channel, (data & Z80DART_WR3_AUTO_ENABLES) ? 1 : 0);
		LOGERROR("Z80DART \"%s\" Channel %c : Receiver Bits/Character %u\n", device->tag, 'A' + channel, z80dart_get_rx_word_length(device, channel));
		break;

	case 4:
		LOGERROR("Z80DART \"%s\" Channel %c : Parity Enable %u\n", device->tag, 'A' + channel, (data & Z80DART_WR4_PARITY_ENABLE) ? 1 : 0);
		LOGERROR("Z80DART \"%s\" Channel %c : Parity %s\n", device->tag, 'A' + channel, (data & Z80DART_WR4_PARITY_EVEN) ? "Even" : "Odd");
		LOGERROR("Z80DART \"%s\" Channel %c : Stop Bits %f\n", device->tag, 'A' + channel, z80dart_get_stop_bits(device, channel));
		LOGERROR("Z80DART \"%s\" Channel %c : Clock Mode %uX\n", device->tag, 'A' + channel, z80dart_get_clock_mode(device, channel));
		break;

	case 5:
		LOGERROR("Z80DART \"%s\" Channel %c : Transmitter Enable %u\n", device->tag, 'A' + channel, (data & Z80DART_WR5_TX_ENABLE) ? 1 : 0);
		LOGERROR("Z80DART \"%s\" Channel %c : Transmitter Bits/Character %u\n", device->tag, 'A' + channel, z80dart_get_tx_word_length(device, channel));
		LOGERROR("Z80DART \"%s\" Channel %c : Send Break %u\n", device->tag, 'A' + channel, (data & Z80DART_WR5_SEND_BREAK) ? 1 : 0);
		LOGERROR("Z80DART \"%s\" Channel %c : Request to Send %u\n", device->tag, 'A' + channel, (data & Z80DART_WR5_RTS) ? 1 : 0);
		LOGERROR("Z80DART \"%s\" Channel %c : Data Terminal Ready %u\n", device->tag, 'A' + channel, (data & Z80DART_WR5_DTR) ? 1 : 0);

		if (data & Z80DART_WR5_RTS)
		{
			/* when the RTS bit is set, the _RTS output goes low */
			z80dart_rts_w(device, channel, 0);

			ch->rts = 1;
		}
		else
		{
			/* when the RTS bit is reset, the _RTS output goes high after the transmitter empties */
			ch->rts = 0;
		}

		/* data terminal ready output follows the state programmed into the DTR bit*/
		ch->dtr = (data & Z80DART_WR5_DTR) ? 0 : 1;
		z80dart_dtr_w(device, channel, ch->dtr);
		break;
	}
}

/***************************************************************************
    DATA REGISTER READ/WRITE
***************************************************************************/

READ8_DEVICE_HANDLER( z80dart_d_r )
{
	z80dart_t *z80dart = get_safe_token(device);
	int channel = offset & 0x01;
	dart_channel *ch = &z80dart->channel[channel];
	UINT8 data = 0;

	if (ch->rx_fifo >= 0)
	{
		/* load data from the FIFO */
		data = ch->rx_data_fifo[ch->rx_fifo];

		/* load error status from the FIFO, retain overrun and parity errors */
		ch->rr[1] = (ch->rr[1] & (Z80DART_RR1_RX_OVERRUN_ERROR | Z80DART_RR1_PARITY_ERROR)) | ch->rx_error_fifo[ch->rx_fifo];

		/* decrease FIFO pointer */
		ch->rx_fifo--;

		if (ch->rx_fifo < 0)
		{
			/* no more characters available in the FIFO */
			ch->rr[0] &= ~ Z80DART_RR0_RX_CHAR_AVAILABLE;
		}
	}

	LOGERROR("Z80DART \"%s\" Channel %c : Data Register Read '%02x'\n", device->tag, 'A' + channel, data);

	return data;
}

WRITE8_DEVICE_HANDLER( z80dart_d_w )
{
	z80dart_t *z80dart = get_safe_token(device);
	int channel = offset & 0x01;
	dart_channel *ch = &z80dart->channel[channel];

	ch->tx_data = data;
	
	ch->rr[0] &= ~Z80DART_RR0_TX_BUFFER_EMPTY;
	ch->rr[1] &= ~Z80DART_RR1_ALL_SENT;

	LOGERROR("Z80DART \"%s\" Channel %c : Data Register Write '%02x'\n", device->tag, 'A' + channel, data);
}

/***************************************************************************
    CONTROL LINE READ/WRITE
***************************************************************************/

void z80dart_receive_data(const device_config *device, int channel, UINT8 data)
{
	z80dart_t *z80dart = get_safe_token(device);
	dart_channel *ch = &z80dart->channel[channel];

	LOGERROR("Z80DART \"%s\" Channel %c : Receive Data Byte '%02x'\n", device->tag, 'A' + channel, data);

	if (ch->rx_fifo == 2)
	{
		/* receive overrun error detected */
		ch->rx_error |= Z80DART_RR1_RX_OVERRUN_ERROR;

		switch (ch->wr[1] & Z80DART_WR1_RX_INT_ENABLE_MASK)
		{
		case Z80DART_WR1_RX_INT_FIRST:
			if (!ch->rx_first)
			{
				z80dart_trigger_interrupt(device, channel, INT_SPECIAL);
			}
			break;
		
		case Z80DART_WR1_RX_INT_ALL_PARITY:
		case Z80DART_WR1_RX_INT_ALL:
			z80dart_trigger_interrupt(device, channel, INT_SPECIAL);
			break;
		}
	}
	else
	{
		ch->rx_fifo++;
	}

	/* store received character and error status into FIFO */
	ch->rx_data_fifo[ch->rx_fifo] = data;
	ch->rx_error_fifo[ch->rx_fifo] = ch->rx_error;

	ch->rr[0] |= Z80DART_RR0_RX_CHAR_AVAILABLE;

	/* receive interrupt */
	switch (ch->wr[1] & Z80DART_WR1_RX_INT_ENABLE_MASK)
	{
	case Z80DART_WR1_RX_INT_FIRST:
		if (ch->rx_first)
		{
			z80dart_trigger_interrupt(device, channel, INT_RECEIVE);

			ch->rx_first = 0;
		}
		break;

	case Z80DART_WR1_RX_INT_ALL_PARITY:
	case Z80DART_WR1_RX_INT_ALL:
		z80dart_trigger_interrupt(device, channel, INT_RECEIVE);
		break;
	}
}

void z80dart_cts_w(const device_config *device, int channel, int state)
{
	z80dart_t *z80dart = get_safe_token(device);
	dart_channel *ch = &z80dart->channel[channel];

	LOGERROR("Z80DART \"%s\" Channel %c : CTS %u\n", device->tag, 'A' + channel, state);

	if (ch->cts != state)
	{
		/* enable transmitter if in auto enables mode */
		if (!state)
			if (ch->wr[3] & Z80DART_WR3_AUTO_ENABLES)
				ch->wr[5] |= Z80DART_WR5_TX_ENABLE;

		/* set clear to send */
		ch->cts = state;

		if (!ch->rx_rr0_latch)
		{
			if (!ch->cts)
				ch->rr[0] |= Z80DART_RR0_CTS;
			else
				ch->rr[0] &= ~Z80DART_RR0_CTS;

			/* trigger interrupt */
			if (ch->wr[1] & Z80DART_WR1_EXT_INT_ENABLE)
			{
				/* trigger interrupt */
				z80dart_trigger_interrupt(device, channel, INT_EXTERNAL);

				/* latch read register 0 */
				ch->rx_rr0_latch = 1;
			}
		}
	}
}

void z80dart_dcd_w(const device_config *device, int channel, int state)
{
	z80dart_t *z80dart = get_safe_token(device);
	dart_channel *ch = &z80dart->channel[channel];

	LOGERROR("Z80DART \"%s\" Channel %c : DCD %u\n", device->tag, 'A' + channel, state);

	if (ch->dcd != state)
	{
		/* enable receiver if in auto enables mode */
		if (!state)
			if (ch->wr[3] & Z80DART_WR3_AUTO_ENABLES)
				ch->wr[3] |= Z80DART_WR3_RX_ENABLE;

		/* set data carrier detect */
		ch->dcd = state;

		if (!ch->rx_rr0_latch)
		{
			if (ch->dcd)
				ch->rr[0] |= Z80DART_RR0_DCD;
			else
				ch->rr[0] &= ~Z80DART_RR0_DCD;

			if (ch->wr[1] & Z80DART_WR1_EXT_INT_ENABLE)
			{
				/* trigger interrupt */
				z80dart_trigger_interrupt(device, channel, INT_EXTERNAL);

				/* latch read register 0 */
				ch->rx_rr0_latch = 1;
			}
		}
	}
}

void z80dart_ri_w(const device_config *device, int channel, int state)
{
	z80dart_t *z80dart = get_safe_token(device);
	dart_channel *ch = &z80dart->channel[channel];

	LOGERROR("Z80DART \"%s\" Channel %c : RI %u\n", device->tag, 'A' + channel, state);

	if (ch->ri != state)
	{
		/* set ring indicator state */
		ch->ri = state;

		if (!ch->rx_rr0_latch)
		{
			if (ch->ri)
				ch->rr[0] |= Z80DART_RR0_RI;
			else
				ch->rr[0] &= ~Z80DART_RR0_RI;

			if (ch->wr[1] & Z80DART_WR1_EXT_INT_ENABLE)
			{
				/* trigger interrupt */
				z80dart_trigger_interrupt(device, channel, INT_EXTERNAL);

				/* latch read register 0 */
				ch->rx_rr0_latch = 1;
			}
		}
	}
}

/***************************************************************************
    CLOCK MANAGEMENT
***************************************************************************/

void z80dart_rxca_w(const device_config *device)
{
	z80dart_t *z80dart = get_safe_token(device);
	dart_channel *ch = &z80dart->channel[Z80DART_CH_A];

	int clocks = z80dart_get_clock_mode(device, Z80DART_CH_A);

	LOGERROR("Z80DART \"%s\" Channel A : Receiver Clock Pulse\n", device->tag);
	
	ch->rx_clock++;

	if (ch->rx_clock == clocks)
	{
		ch->rx_clock = 0;

		/* receive data */
		receive(device, Z80DART_CH_A);
	}
}

void z80dart_txca_w(const device_config *device)
{
	z80dart_t *z80dart = get_safe_token(device);
	dart_channel *ch = &z80dart->channel[Z80DART_CH_A];

	int clocks = z80dart_get_clock_mode(device, Z80DART_CH_A);

	LOGERROR("Z80DART \"%s\" Channel A : Transmitter Clock Pulse\n", device->tag);
	
	ch->tx_clock++;

	if (ch->tx_clock == clocks)
	{
		ch->tx_clock = 0;

		/* transmit data */
		transmit(device, Z80DART_CH_A);
	}
}

void z80dart_rxtxcb_w(const device_config *device)
{
	z80dart_t *z80dart = get_safe_token(device);
	dart_channel *ch = &z80dart->channel[Z80DART_CH_B];

	int clocks = z80dart_get_clock_mode(device, Z80DART_CH_B);

	LOGERROR("Z80DART \"%s\" Channel A : Receiver/Transmitter Clock Pulse\n", device->tag);
	
	ch->rx_clock++;
	ch->tx_clock++;

	if (ch->rx_clock == clocks)
	{
		ch->rx_clock = 0;
		ch->tx_clock = 0;

		/* receive and transmit data */
		receive(device, Z80DART_CH_B);
		transmit(device, Z80DART_CH_B);
	}
}

static TIMER_CALLBACK( rxca_tick )
{
	z80dart_rxca_w(ptr);
}

static TIMER_CALLBACK( txca_tick )
{
	z80dart_txca_w(ptr);
}

static TIMER_CALLBACK( rxtxcb_tick )
{
	z80dart_rxtxcb_w(ptr);
}

/***************************************************************************
    DAISY CHAIN INTERFACE
***************************************************************************/

static int z80dart_irq_state(const device_config *device)
{
	z80dart_t *z80dart = get_safe_token( device );
	int state = 0;
	int i;

	LOGERROR("Z80DART \"%s\" : Interrupt State B:%d%d%d%d A:%d%d%d%d\n", device->tag, 
				z80dart->int_state[0], z80dart->int_state[1], z80dart->int_state[2], z80dart->int_state[3],
				z80dart->int_state[4], z80dart->int_state[5], z80dart->int_state[6], z80dart->int_state[7]);

	/* loop over all interrupt sources */
	for (i = 0; i < 8; i++)
	{
		/* if we're servicing a request, don't indicate more interrupts */
		if (z80dart->int_state[i] & Z80_DAISY_IEO)
		{
			state |= Z80_DAISY_IEO;
			break;
		}
		state |= z80dart->int_state[i];
	}

	LOGERROR("Z80DART \"%s\" : Interrupt State %u\n", device->tag, state);

	return state;
}

static int z80dart_irq_ack(const device_config *device)
{
	z80dart_t *z80dart = get_safe_token( device );
	int i;

	LOGERROR("Z80DART \"%s\" Interrupt Acknowledge\n", device->tag);

	/* loop over all interrupt sources */
	for (i = 0; i < 8; i++)
	{
		/* find the first channel with an interrupt requested */
		if (z80dart->int_state[i] & Z80_DAISY_INT)
		{
			/* clear interrupt, switch to the IEO state, and update the IRQs */
			z80dart->int_state[i] = Z80_DAISY_IEO;
			z80dart->channel[Z80DART_CH_A].rr[0] &= ~Z80DART_RR0_INTERRUPT_PENDING;
			z80dart_check_interrupt(device);

			LOGERROR("Z80DART \"%s\" : Interrupt Acknowledge Vector %02x\n", device->tag, z80dart->channel[Z80DART_CH_B].rr[2]);

			return z80dart->channel[Z80DART_CH_B].rr[2];
		}
	}

	logerror("z80dart_irq_ack: failed to find an interrupt to ack!\n");
	
	return z80dart->channel[Z80DART_CH_B].rr[2];
}

static void z80dart_irq_reti(const device_config *device)
{
	z80dart_t *z80dart = get_safe_token( device );
	int i;

	LOGERROR("Z80DART \"%s\" Return from Interrupt\n", device->tag);

	/* loop over all interrupt sources */
	for (i = 0; i < 8; i++)
	{
		/* find the first channel with an IEO pending */
		if (z80dart->int_state[i] & Z80_DAISY_IEO)
		{
			/* clear the IEO state and update the IRQs */
			z80dart->int_state[i] &= ~Z80_DAISY_IEO;
			z80dart_check_interrupt(device);
			return;
		}
	}

	logerror("z80dart_irq_reti: failed to find an interrupt to clear IEO on!\n");
}

/***************************************************************************
    READ/WRITE HANDLERS
***************************************************************************/

READ8_DEVICE_HANDLER( z80dart_r )
{
	return (offset & 2) ? z80dart_c_r(device, offset & 1) : z80dart_d_r(device, offset & 1);
}

WRITE8_DEVICE_HANDLER( z80dart_w )
{
	if (offset & 2)
		z80dart_c_w(device, offset & 1, data);
	else
		z80dart_d_w(device, offset & 1, data);
}

READ8_DEVICE_HANDLER( z80dart_alt_r )
{
	int channel = BIT(offset, 1);

	return (offset & 1) ? z80dart_c_r(device, channel) : z80dart_d_r(device, channel);
}

WRITE8_DEVICE_HANDLER( z80dart_alt_w )
{
	int channel = BIT(offset, 1);

	if (offset & 1)
		z80dart_c_w(device, channel, data);
	else
		z80dart_d_w(device, channel, data);
}

/***************************************************************************
    DEVICE INTERFACE
***************************************************************************/

static DEVICE_START( z80dart )
{
	const z80dart_interface *intf = device->static_config;
	z80dart_t *z80dart = get_safe_token(device);
	int cpunum = -1;

	assert(intf != NULL);
	z80dart->intf = intf;

	/* get clock */

	if (intf->cpu != NULL)
	{
		cpunum = mame_find_cpu_index(device->machine, intf->cpu);
	}

	if (cpunum != -1)
	{
		z80dart->clock = device->machine->config->cpu[cpunum].clock;
	}
	else
	{
		assert(intf->clock > 0);
		z80dart->clock = intf->clock;
	}

	/* allocate channel A receive timer */

	z80dart->rxca_timer = timer_alloc(rxca_tick, (void *)device);

	if (z80dart->intf->rx_clock_a)
		timer_adjust_periodic(z80dart->rxca_timer, attotime_zero, 0, ATTOTIME_IN_HZ(z80dart->intf->rx_clock_a));

	/* allocate channel A transmit timer */

	z80dart->txca_timer = timer_alloc(txca_tick, (void *)device);

	if (z80dart->intf->tx_clock_a)
		timer_adjust_periodic(z80dart->txca_timer, attotime_zero, 0, ATTOTIME_IN_HZ(z80dart->intf->tx_clock_a));

	/* allocate channel B receive/transmit timer */

	z80dart->rxtxcb_timer = timer_alloc(rxtxcb_tick, (void *)device);

	if (z80dart->intf->rx_tx_clock_b)
		timer_adjust_periodic(z80dart->rxtxcb_timer, attotime_zero, 0, ATTOTIME_IN_HZ(z80dart->intf->rx_tx_clock_b));

	/* register for state saving */
	//state_save_register_item_array("z80dart", device->tag, 0, z80dart->);

	return DEVICE_START_OK;
}

static DEVICE_RESET( z80dart )
{
	int channel;

	LOGERROR("Z80DART \"%s\" Reset\n", device->tag);

	for (channel = Z80DART_CH_A; channel <= Z80DART_CH_B; channel++)
	{
		z80dart_reset_channel(device, channel);
	}

	z80dart_check_interrupt(device);
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
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(z80dart_t);					break;
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
