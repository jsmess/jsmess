/**********************************************************************

    Motorola MC68901 Multi Function Peripheral emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

    TODO:

    - daisy chaining
    - correct gpio_timer period
    - disable GPIO3/4 interrupts when timer A/B in pulse mode
    - spurious interrupt

        If you look at the MFP datasheet it is obvious that it can generate the conditions for a spurious interrupt.
        However the fact that they indeed happen in the ST is quite interesting.

        The MFP will generate a spurious interrupt if interrupts are disabled (by changing the IERA/IERB registers)
        at the ???precise point???. The precise point would be after the system (but not necessarily the CPU, see below)
        triggered an MFP interrupt, and before the CPU drives the interrupt acknowledge cycle.

        If the MFP was connected directly to the CPU, spurious interrupts probably couldn???t happen. However in the
        ST, GLUE seats in the middle and handles all the interrupt timing. It is possible that GLUE introduces a
        delay between detecting a change in the MFP interrupt request signal and actually propagating the change to
        the CPU IPL signals (it is even possible that GLUE make some kind of latching). This would create a window
        long enough for the ???precise point??? described above.

        "yes, the spurious interrupt occurs when i mask a timer. i did not notice an occurance of the SPI when changing data and control registers.
        if i kill interrupts with the status reg before masking the timer interrupt, then the SPI occurs as soon as the status register is set to re-enable interrupts."

    - divide serial clock by 16
    - synchronous mode
    - 1.5/2 stop bits
    - interrupt on receiver break end
    - interrupt on character boundaries during break transmission
    - loopback mode

*/

#include "68901mfp.h"
#include "cpu/m68000/m68000.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG		(0)

enum
{
	MC68901_REGISTER_GPIP = 0,
	MC68901_REGISTER_AER,
	MC68901_REGISTER_DDR,
	MC68901_REGISTER_IERA,
	MC68901_REGISTER_IERB,
	MC68901_REGISTER_IPRA,
	MC68901_REGISTER_IPRB,
	MC68901_REGISTER_ISRA,
	MC68901_REGISTER_ISRB,
	MC68901_REGISTER_IMRA,
	MC68901_REGISTER_IMRB,
	MC68901_REGISTER_VR,
	MC68901_REGISTER_TACR,
	MC68901_REGISTER_TBCR,
	MC68901_REGISTER_TCDCR,
	MC68901_REGISTER_TADR,
	MC68901_REGISTER_TBDR,
	MC68901_REGISTER_TCDR,
	MC68901_REGISTER_TDDR,
	MC68901_REGISTER_SCR,
	MC68901_REGISTER_UCR,
	MC68901_REGISTER_RSR,
	MC68901_REGISTER_TSR,
	MC68901_REGISTER_UDR
};

enum
{
	MC68901_INT_GPI0 = 0,
	MC68901_INT_GPI1,
	MC68901_INT_GPI2,
	MC68901_INT_GPI3,
	MC68901_INT_TIMER_D,
	MC68901_INT_TIMER_C,
	MC68901_INT_GPI4,
	MC68901_INT_GPI5,
	MC68901_INT_TIMER_B,
	MC68901_INT_XMIT_ERROR,
	MC68901_INT_XMIT_BUFFER_EMPTY,
	MC68901_INT_RCV_ERROR,
	MC68901_INT_RCV_BUFFER_FULL,
	MC68901_INT_TIMER_A,
	MC68901_INT_GPI6,
	MC68901_INT_GPI7
};

enum
{
	MC68901_GPIP_0 = 0,
	MC68901_GPIP_1,
	MC68901_GPIP_2,
	MC68901_GPIP_3,
	MC68901_GPIP_4,
	MC68901_GPIP_5,
	MC68901_GPIP_6,
	MC68901_GPIP_7
};

enum
{
	MC68901_TIMER_A = 0,
	MC68901_TIMER_B,
	MC68901_TIMER_C,
	MC68901_TIMER_D,
	MC68901_MAX_TIMERS
};

enum
{
	MC68901_SERIAL_START = 0,
	MC68901_SERIAL_DATA,
	MC68901_SERIAL_PARITY,
	MC68901_SERIAL_STOP
};

enum
{
	MC68901_XMIT_OFF = 0,
	MC68901_XMIT_STARTING,
	MC68901_XMIT_ON,
	MC68901_XMIT_BREAK,
	MC68901_XMIT_STOPPING
};

#define MC68901_AER_GPIP_0				0x01
#define MC68901_AER_GPIP_1				0x02
#define MC68901_AER_GPIP_2				0x04
#define MC68901_AER_GPIP_3				0x08
#define MC68901_AER_GPIP_4				0x10
#define MC68901_AER_GPIP_5				0x20
#define MC68901_AER_GPIP_6				0x40
#define MC68901_AER_GPIP_7				0x80

#define MC68901_VR_S					0x08

#define MC68901_IR_GPIP_0				0x0001
#define MC68901_IR_GPIP_1				0x0002
#define MC68901_IR_GPIP_2				0x0004
#define MC68901_IR_GPIP_3				0x0008
#define MC68901_IR_TIMER_D				0x0010
#define MC68901_IR_TIMER_C				0x0020
#define MC68901_IR_GPIP_4				0x0040
#define MC68901_IR_GPIP_5				0x0080
#define MC68901_IR_TIMER_B				0x0100
#define MC68901_IR_XMIT_ERROR			0x0200
#define MC68901_IR_XMIT_BUFFER_EMPTY	0x0400
#define MC68901_IR_RCV_ERROR			0x0800
#define MC68901_IR_RCV_BUFFER_FULL		0x1000
#define MC68901_IR_TIMER_A				0x2000
#define MC68901_IR_GPIP_6				0x4000
#define MC68901_IR_GPIP_7				0x8000

#define MC68901_TCR_TIMER_STOPPED		0x00
#define MC68901_TCR_TIMER_DELAY_4		0x01
#define MC68901_TCR_TIMER_DELAY_10		0x02
#define MC68901_TCR_TIMER_DELAY_16		0x03
#define MC68901_TCR_TIMER_DELAY_50		0x04
#define MC68901_TCR_TIMER_DELAY_64		0x05
#define MC68901_TCR_TIMER_DELAY_100		0x06
#define MC68901_TCR_TIMER_DELAY_200		0x07
#define MC68901_TCR_TIMER_EVENT			0x08
#define MC68901_TCR_TIMER_PULSE_4		0x09
#define MC68901_TCR_TIMER_PULSE_10		0x0a
#define MC68901_TCR_TIMER_PULSE_16		0x0b
#define MC68901_TCR_TIMER_PULSE_50		0x0c
#define MC68901_TCR_TIMER_PULSE_64		0x0d
#define MC68901_TCR_TIMER_PULSE_100		0x0e
#define MC68901_TCR_TIMER_PULSE_200		0x0f
#define MC68901_TCR_TIMER_RESET			0x10

#define MC68901_UCR_PARITY_ENABLED		0x04
#define MC68901_UCR_PARITY_EVEN			0x02
#define MC68901_UCR_PARITY_ODD			0x00
#define MC68901_UCR_WORD_LENGTH_8		0x00
#define MC68901_UCR_WORD_LENGTH_7		0x20
#define MC68901_UCR_WORD_LENGTH_6		0x40
#define MC68901_UCR_WORD_LENGTH_5		0x60
#define MC68901_UCR_START_STOP_0_0		0x00
#define MC68901_UCR_START_STOP_1_1		0x08
#define MC68901_UCR_START_STOP_1_15		0x10
#define MC68901_UCR_START_STOP_1_2		0x18
#define MC68901_UCR_CLOCK_DIVIDE_16		0x80
#define MC68901_UCR_CLOCK_DIVIDE_1		0x00

#define MC68901_RSR_RCV_ENABLE			0x01
#define MC68901_RSR_SYNC_STRIP_ENABLE	0x02
#define MC68901_RSR_MATCH				0x04
#define MC68901_RSR_CHAR_IN_PROGRESS	0x04
#define MC68901_RSR_FOUND_SEARCH		0x08
#define MC68901_RSR_BREAK				0x08
#define MC68901_RSR_FRAME_ERROR			0x10
#define MC68901_RSR_PARITY_ERROR		0x20
#define MC68901_RSR_OVERRUN_ERROR		0x40
#define MC68901_RSR_BUFFER_FULL			0x80

#define MC68901_TSR_XMIT_ENABLE			0x01
#define MC68901_TSR_OUTPUT_HI_Z			0x00
#define MC68901_TSR_OUTPUT_LOW			0x02
#define MC68901_TSR_OUTPUT_HIGH			0x04
#define MC68901_TSR_OUTPUT_LOOP			0x06
#define MC68901_TSR_OUTPUT_MASK			0x06
#define MC68901_TSR_BREAK				0x08
#define MC68901_TSR_END_OF_XMIT			0x10
#define MC68901_TSR_AUTO_TURNAROUND		0x20
#define MC68901_TSR_UNDERRUN_ERROR		0x40
#define MC68901_TSR_BUFFER_EMPTY		0x80

static const int MC68901_INT_MASK_GPIO[] =
{
	MC68901_IR_GPIP_0, MC68901_IR_GPIP_1, MC68901_IR_GPIP_2, MC68901_IR_GPIP_3,
	MC68901_IR_GPIP_4, MC68901_IR_GPIP_5, MC68901_IR_GPIP_6, MC68901_IR_GPIP_7
};

static const int MC68901_INT_MASK_TIMER[] =
{
	MC68901_IR_TIMER_A, MC68901_IR_TIMER_B, MC68901_IR_TIMER_C, MC68901_IR_TIMER_D
};

static const int MC68901_GPIO_TIMER[] =
{
	MC68901_GPIP_4, MC68901_GPIP_3
};

static const int PRESCALER[] = { 0, 4, 10, 16, 50, 64, 100, 200 };

#define TXD(_data) devcb_call_write_line(&mc68901->out_so_func, _data);

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _mc68901_t mc68901_t;
struct _mc68901_t
{
	devcb_resolved_read8		in_gpio_func;
	devcb_resolved_write8		out_gpio_func;
	devcb_resolved_read_line	in_si_func;
	devcb_resolved_write_line	out_so_func;
	devcb_resolved_write_line	out_tao_func;
	devcb_resolved_write_line	out_tbo_func;
	devcb_resolved_write_line	out_tco_func;
	devcb_resolved_write_line	out_tdo_func;
	devcb_resolved_write_line	out_irq_func;

	int device_type;					/* device type */
	int timer_clock;					/* timer clock */

	/* registers */
	UINT8 gpip;							/* general purpose I/O register */
	UINT8 aer;							/* active edge register */
	UINT8 ddr;							/* data direction register */

	UINT16 ier;							/* interrupt enable register */
	UINT16 ipr;							/* interrupt pending register */
	UINT16 isr;							/* interrupt in-service register */
	UINT16 imr;							/* interrupt mask register */
	UINT8 vr;							/* vector register */

	UINT8 tacr;							/* timer A control register */
	UINT8 tbcr;							/* timer B control register */
	UINT8 tcdcr;						/* timers C and D control register */
	UINT8 tdr[MC68901_MAX_TIMERS];		/* timer data registers */

	UINT8 scr;							/* synchronous character register */
	UINT8 ucr;							/* USART control register */
	UINT8 tsr;							/* transmitter status register */
	UINT8 rsr;							/* receiver status register */
	UINT8 udr;							/* USART data register */

	/* counter timer state */
	UINT8 tmc[MC68901_MAX_TIMERS];		/* timer main counters */
	int ti[MC68901_MAX_TIMERS];			/* timer in latch */
	int to[MC68901_MAX_TIMERS];			/* timer out latch */

	/* interrupt state */
	int irqlevel;						/* interrupt level latch */

	/* serial state */
	UINT8 next_rsr;						/* receiver status register latch */
	int rsr_read;						/* receiver status register read flag */
	int rxtx_word;						/* word length */
	int rxtx_start;						/* start bits */
	int rxtx_stop;						/* stop bits */

	/* receive state */
	UINT8 rx_buffer;					/* receive buffer */
	int	rx_bits;						/* receive bit count */
	int	rx_parity;						/* receive parity bit */
	int	rx_state;						/* receive state */

	/* transmit state */
	UINT8 tx_buffer;					/* transmit buffer */
	int tx_bits;						/* transmit bit count */
	int tx_parity;						/* transmit parity bit */
	int tx_state;						/* transmit state */
	int xmit_state;						/* transmitter state */

	/* timers */
	emu_timer *timer[MC68901_MAX_TIMERS]; /* counter timers */
	emu_timer *rx_timer;				/* receive timer */
	emu_timer *tx_timer;				/* transmit timer */
	emu_timer *poll_timer;				/* general purpose I/O poll timer */
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE mc68901_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	return (mc68901_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE const mc68901_interface *get_interface(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == MC68901);
	return (const mc68901_interface *) device->baseconfig().static_config();
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    check_interrupts - set the interrupt request
    line state
-------------------------------------------------*/

static void check_interrupts(device_t *device)
{
	mc68901_t *mc68901 = get_safe_token(device);

	if (mc68901->ipr & mc68901->imr)
	{
		devcb_call_write_line(&mc68901->out_irq_func, ASSERT_LINE);
	}
	else
	{
		devcb_call_write_line(&mc68901->out_irq_func, CLEAR_LINE);
	}
}

/*-------------------------------------------------
    take_interrupt - mark an interrupt pending
-------------------------------------------------*/

static void take_interrupt(device_t *device, UINT16 mask)
{
	mc68901_t *mc68901 = get_safe_token(device);

	mc68901->ipr |= mask;

	check_interrupts(device);
}

/*-------------------------------------------------
    TIMER_CALLBACK( gpio_poll_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( gpio_poll_tick )
{
	device_t *device = (device_t *)ptr;
	mc68901_t *mc68901 = get_safe_token(device);

	UINT8 gpio = devcb_call_read8(&mc68901->in_gpio_func, 0);

	UINT8 gpold = (mc68901->gpip & ~mc68901->ddr) ^ mc68901->aer;
	UINT8 gpnew = (gpio & ~mc68901->ddr) ^ mc68901->aer;

	int bit;

	for (bit = 0; bit < 8; bit++) // loop thru each bit from 0 to 7
	{
		if ((BIT(gpold, bit) == 1) && (BIT(gpnew, bit) == 0)) // if transition from 1 to 0 is detected...
		{
			if (LOG) logerror("MC68901 Edge Transition Detected on GPIO%u\n", bit);

			if (mc68901->ier & MC68901_INT_MASK_GPIO[bit]) // AND interrupt enabled bit is set...
			{
				if (LOG) logerror("MC68901 Interrupt Pending for GPIO%u\n", bit);

				take_interrupt(device, MC68901_INT_MASK_GPIO[bit]); // set interrupt pending bit
			}
		}
	}

	mc68901->gpip = (gpio & ~mc68901->ddr) | (mc68901->gpip & mc68901->ddr);
}

/*-------------------------------------------------
    rx_buffer_full - signal receive buffer full
    interrupt
-------------------------------------------------*/

static void rx_buffer_full(device_t *device)
{
	mc68901_t *mc68901 = get_safe_token(device);

	if (mc68901->ier & MC68901_IR_RCV_BUFFER_FULL)
	{
		take_interrupt(device, MC68901_IR_RCV_BUFFER_FULL);
	}
}

/*-------------------------------------------------
    rx_error - signal receive error interrupt
-------------------------------------------------*/

static void rx_error(device_t *device)
{
	mc68901_t *mc68901 = get_safe_token(device);

	if (mc68901->ier & MC68901_IR_RCV_ERROR)
	{
		take_interrupt(device, MC68901_IR_RCV_ERROR);
	}
	else
	{
		rx_buffer_full(device);
	}
}

/*-------------------------------------------------
    tx_buffer_empty - signal transmit buffer
    empty interrupt
-------------------------------------------------*/

static void tx_buffer_empty(device_t *device)
{
	mc68901_t *mc68901 = get_safe_token(device);

	if (mc68901->ier & MC68901_IR_XMIT_BUFFER_EMPTY)
	{
		take_interrupt(device, MC68901_IR_XMIT_BUFFER_EMPTY);
	}
}

/*-------------------------------------------------
    tx_error - signal transmit error interrupt
-------------------------------------------------*/

static void tx_error(device_t *device)
{
	mc68901_t *mc68901 = get_safe_token(device);

	if (mc68901->ier & MC68901_IR_XMIT_ERROR)
	{
		take_interrupt(device, MC68901_IR_XMIT_ERROR);
	}
	else
	{
		tx_buffer_empty(device);
	}
}

/*-------------------------------------------------
    get_parity_bit - calculate parity bit
-------------------------------------------------*/

static int get_parity_bit(UINT8 b)
{
	b ^= b >> 4;
	b ^= b >> 2;
	b ^= b >> 1;
	return b & 1;
}

/*-------------------------------------------------
    serial_receive - receive serial bit
-------------------------------------------------*/

static void serial_receive(device_t *device)
{
	mc68901_t *mc68901 = get_safe_token(device);
	int rxd;

	if (!(mc68901->rsr & MC68901_RSR_RCV_ENABLE)) return;

	rxd = devcb_call_read_line(&mc68901->in_si_func);

	switch (mc68901->rx_state)
	{
	case MC68901_SERIAL_START:
		if (!rxd)
		{
			mc68901->rsr |= MC68901_RSR_CHAR_IN_PROGRESS;
			mc68901->rx_bits = 0;
			mc68901->rx_buffer = 0;
			mc68901->rx_state = MC68901_SERIAL_DATA;
			mc68901->next_rsr = MC68901_RSR_BREAK;
		}
		break;

	case MC68901_SERIAL_DATA:
		if ((mc68901->next_rsr & MC68901_RSR_BREAK) && (rxd == 1) && mc68901->rsr_read)
		{
			mc68901->next_rsr &= ~MC68901_RSR_BREAK;
		}

		mc68901->rx_buffer >>= 1;
		mc68901->rx_buffer |= (rxd << 7);
		mc68901->rx_bits++;

		if (mc68901->rx_bits == mc68901->rxtx_word)
		{
			if (mc68901->rxtx_word < 8)
			{
				mc68901->rx_buffer >>= (8 - mc68901->rxtx_word);
			}

			mc68901->rsr &= ~MC68901_RSR_CHAR_IN_PROGRESS;

			if (mc68901->ucr & MC68901_UCR_PARITY_ENABLED)
			{
				mc68901->rx_state = MC68901_SERIAL_PARITY;
			}
			else
			{
				mc68901->rx_state = MC68901_SERIAL_STOP;
			}
		}
		break;

	case MC68901_SERIAL_PARITY:
		mc68901->rx_parity = rxd;

		if (mc68901->rx_parity != (get_parity_bit(mc68901->rx_buffer) ^ ((mc68901->ucr & MC68901_UCR_PARITY_EVEN) >> 1)))
		{
			mc68901->next_rsr |= MC68901_RSR_PARITY_ERROR;
		}

		mc68901->rx_state = MC68901_SERIAL_STOP;
		break;

	case MC68901_SERIAL_STOP:
		if (rxd == 1)
		{
			if (!((mc68901->rsr & MC68901_RSR_SYNC_STRIP_ENABLE) && (mc68901->rx_buffer == mc68901->scr)))
			{
				if (!(mc68901->rsr & MC68901_RSR_OVERRUN_ERROR))
				{
					if (mc68901->rsr & MC68901_RSR_BUFFER_FULL)
					{
						// incoming word received but last word in receive buffer has not been read
						mc68901->next_rsr |= MC68901_RSR_OVERRUN_ERROR;
					}
					else
					{
						// incoming word received and receive buffer is empty
						mc68901->rsr |= MC68901_RSR_BUFFER_FULL;
						mc68901->udr = mc68901->rx_buffer;
						rx_buffer_full(device);
					}
				}
			}
		}
		else
		{
			if (mc68901->rx_buffer)
			{
				// non-zero data word not followed by a stop bit
				mc68901->next_rsr |= MC68901_RSR_FRAME_ERROR;
			}
		}

		mc68901->rx_state = MC68901_SERIAL_START;
		break;
	}
}

/*-------------------------------------------------
    TIMER_CALLBACK( rx_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( rx_tick )
{
	device_t *device = (device_t *)ptr;

	serial_receive(device);
}

/*-------------------------------------------------
    tx_disabled - transmitter disabled handler
-------------------------------------------------*/

static void tx_disabled(device_t *device)
{
	mc68901_t *mc68901 = get_safe_token(device);

	switch (mc68901->tsr & MC68901_TSR_OUTPUT_MASK)
	{
	case MC68901_TSR_OUTPUT_HI_Z:
		/* indeterminate state */
	case MC68901_TSR_OUTPUT_LOW:
		TXD(0);
		break;

	case MC68901_TSR_OUTPUT_HIGH:
	case MC68901_TSR_OUTPUT_LOOP:
		TXD(1);
		break;
	}
}

/*-------------------------------------------------
    tx_starting - transmitter starting handler
-------------------------------------------------*/

static void tx_starting(device_t *device)
{
	mc68901_t *mc68901 = get_safe_token(device);

	if (mc68901->tsr & MC68901_TSR_XMIT_ENABLE)
	{
		/* enable transmitter */
		TXD(1);
		mc68901->xmit_state = MC68901_XMIT_ON;
	}
	else
	{
		/* disable transmitter */
		mc68901->tsr |= MC68901_TSR_END_OF_XMIT;
		mc68901->xmit_state = MC68901_XMIT_OFF;
	}
}

/*-------------------------------------------------
    tx_break - transmit break handler
-------------------------------------------------*/

static void tx_break(device_t *device)
{
	mc68901_t *mc68901 = get_safe_token(device);

	if (mc68901->tsr & MC68901_TSR_XMIT_ENABLE)
	{
		if (mc68901->tsr & MC68901_TSR_BREAK)
		{
			/* transmit break */
			TXD(1);
		}
		else
		{
			/* enable transmitter */
			mc68901->xmit_state = MC68901_XMIT_ON;
		}
	}
	else
	{
		/* disable transmitter */
		mc68901->tsr |= MC68901_TSR_END_OF_XMIT;
		mc68901->xmit_state = MC68901_XMIT_OFF;
	}
}

/*-------------------------------------------------
    tx_enabled - transmitter enabled handler
-------------------------------------------------*/

static void tx_enabled(device_t *device)
{
	mc68901_t *mc68901 = get_safe_token(device);

	switch (mc68901->tx_state)
	{
	case MC68901_SERIAL_START:
		if (mc68901->tsr & MC68901_TSR_UNDERRUN_ERROR)
		{
			/* buffer underrun condition */
			if (mc68901->tsr & MC68901_TSR_XMIT_ENABLE)
			{
				/* transmit break */
				TXD(1);
			}
			else
			{
				/* transmitter disabled */
				tx_disabled(device);
			}
		}
		else
		{
			if (mc68901->tsr & MC68901_TSR_BUFFER_EMPTY)
			{
				/* transmit buffer is empty, signal underrun error */
				mc68901->tsr |= MC68901_TSR_UNDERRUN_ERROR;

				if (mc68901->tsr & MC68901_TSR_XMIT_ENABLE)
				{
					/* transmit break */
					TXD(1);
				}
				else
				{
					/* transmitter disabled */
					tx_disabled(device);
				}
			}
			else
			{
				/* transmit start bit */
				TXD(0);

				/* load transmit buffer */
				mc68901->tx_buffer = mc68901->udr;
				mc68901->tx_bits = 0;

				/* signal transmit buffer empty */
				mc68901->tsr |= MC68901_TSR_BUFFER_EMPTY;
				tx_buffer_empty(device);

				/* calculate parity */
				mc68901->tx_parity = get_parity_bit(mc68901->tx_buffer);

				/* next bit is data */
				mc68901->tx_state = MC68901_SERIAL_DATA;
			}
		}
		break;

	case MC68901_SERIAL_DATA:
		/* transmit data bit */
		TXD(mc68901->tx_buffer & 0x01);

		/* shift transmit buffer */
		mc68901->tx_buffer >>= 1;
		mc68901->tx_bits++;

		if (mc68901->tx_bits == mc68901->rxtx_word)
		{
			/* all data bits transferred */
			if (mc68901->ucr & MC68901_UCR_PARITY_ENABLED)
			{
				/* next bit is parity */
				mc68901->tx_state = MC68901_SERIAL_PARITY;
			}
			else
			{
				/* next bit is stop */
				mc68901->tx_state = MC68901_SERIAL_STOP;
			}
		}
		break;

	case MC68901_SERIAL_PARITY:
		if (mc68901->rxtx_word < 8)
		{
			/* transmit user-defined parity bit from buffer */
			TXD(mc68901->tx_buffer & 0x01);
		}
		else
		{
			/* transmit calculated parity bit */
			TXD(mc68901->tx_parity ^ ((mc68901->ucr & MC68901_UCR_PARITY_EVEN) >> 1));
		}

		/* next bit is stop */
		mc68901->tx_state = MC68901_SERIAL_STOP;
		break;

	case MC68901_SERIAL_STOP:
		/* transmit stop bit */
		TXD(1);

		if (mc68901->tsr & MC68901_TSR_XMIT_ENABLE)
		{
			/* next bit is start */
			mc68901->tx_state = MC68901_SERIAL_START;
		}
		else
		{
			if (mc68901->tsr & MC68901_TSR_AUTO_TURNAROUND)
			{
				/* enable transmitter */
				mc68901->tsr |= MC68901_TSR_XMIT_ENABLE;

				/* next bit is start */
				mc68901->tx_state = MC68901_SERIAL_START;
			}
			else
			{
				/* disable transmitter */
				mc68901->xmit_state = MC68901_XMIT_OFF;
				mc68901->tsr |= MC68901_TSR_END_OF_XMIT;

				/* signal transmit error */
				tx_error(device);
			}
		}
		break;
	}
}

/*-------------------------------------------------
    serial_transmit - transmit serial bit
-------------------------------------------------*/

static void serial_transmit(device_t *device)
{
	mc68901_t *mc68901 = get_safe_token(device);

	switch (mc68901->xmit_state)
	{
	case MC68901_XMIT_OFF:		tx_disabled(device);	break;
	case MC68901_XMIT_STARTING:	tx_starting(device);	break;
	case MC68901_XMIT_BREAK:	tx_break(device);		break;
	case MC68901_XMIT_ON:		tx_enabled(device);		break;
	}
}

/*-------------------------------------------------
    TIMER_CALLBACK( tx_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( tx_tick )
{
	device_t *device = (device_t *)ptr;

	serial_transmit(device);
}

/*-------------------------------------------------
    mc68901_register_r - register read
-------------------------------------------------*/

READ8_DEVICE_HANDLER( mc68901_register_r )
{
	mc68901_t *mc68901 = get_safe_token(device);

	assert(mc68901 != NULL);

	switch (offset)
	{
	case MC68901_REGISTER_GPIP:  return mc68901->gpip;
	case MC68901_REGISTER_AER:   return mc68901->aer;
	case MC68901_REGISTER_DDR:   return mc68901->ddr;

	case MC68901_REGISTER_IERA:  return mc68901->ier >> 8;
	case MC68901_REGISTER_IERB:  return mc68901->ier & 0xff;
	case MC68901_REGISTER_IPRA:  return mc68901->ipr >> 8;
	case MC68901_REGISTER_IPRB:  return mc68901->ipr & 0xff;
	case MC68901_REGISTER_ISRA:  return mc68901->isr >> 8;
	case MC68901_REGISTER_ISRB:  return mc68901->isr & 0xff;
	case MC68901_REGISTER_IMRA:  return mc68901->imr >> 8;
	case MC68901_REGISTER_IMRB:  return mc68901->imr & 0xff;
	case MC68901_REGISTER_VR:    return mc68901->vr;

	case MC68901_REGISTER_TACR:  return mc68901->tacr;
	case MC68901_REGISTER_TBCR:  return mc68901->tbcr;
	case MC68901_REGISTER_TCDCR: return mc68901->tcdcr;
	case MC68901_REGISTER_TADR:  return mc68901->tmc[MC68901_TIMER_A];
	case MC68901_REGISTER_TBDR:  return mc68901->tmc[MC68901_TIMER_B];
	case MC68901_REGISTER_TCDR:  return mc68901->tmc[MC68901_TIMER_C];
	case MC68901_REGISTER_TDDR:  return mc68901->tmc[MC68901_TIMER_D];

	case MC68901_REGISTER_SCR:   return mc68901->scr;
	case MC68901_REGISTER_UCR:   return mc68901->ucr;
	case MC68901_REGISTER_RSR:
		mc68901->rsr_read = 1;
		return mc68901->rsr;

	case MC68901_REGISTER_TSR:
		{
			/* clear UE bit (in reality, this won't be cleared until one full clock cycle of the transmitter has passed since the bit was set) */
			UINT8 tsr = mc68901->tsr;
			mc68901->tsr &= 0xbf;

			return tsr;
		}

	case MC68901_REGISTER_UDR:
		/* load RSR with latched value */
		mc68901->rsr = (mc68901->next_rsr & 0x7c) | (mc68901->rsr & 0x03);
		mc68901->next_rsr = 0;

		if (mc68901->rsr & 0x78)
		{
			/* signal receiver error interrupt */
			rx_error(device);
		}

		return mc68901->udr;

	default:					  return 0;
	}
}

/*-------------------------------------------------
    mc68901_register_w - register write
-------------------------------------------------*/

#define DIVISOR PRESCALER[data & 0x07]

WRITE8_DEVICE_HANDLER( mc68901_register_w )
{
	mc68901_t *mc68901 = get_safe_token(device);

	switch (offset)
	{
	case MC68901_REGISTER_GPIP:
		if (LOG) logerror("MC68901 General Purpose I/O : %x\n", data);
		mc68901->gpip = data & mc68901->ddr;

		devcb_call_write8(&mc68901->out_gpio_func, 0, mc68901->gpip);
		break;

	case MC68901_REGISTER_AER:
		if (LOG) logerror("MC68901 Active Edge Register : %x\n", data);
		mc68901->aer = data;
		break;

	case MC68901_REGISTER_DDR:
		if (LOG) logerror("MC68901 Data Direction Register : %x\n", data);
		mc68901->ddr = data;
		break;

	case MC68901_REGISTER_IERA:
		if (LOG) logerror("MC68901 Interrupt Enable Register A : %x\n", data);
		mc68901->ier = (data << 8) | (mc68901->ier & 0xff);
		mc68901->ipr &= mc68901->ier;
		check_interrupts(device);
		break;

	case MC68901_REGISTER_IERB:
		if (LOG) logerror("MC68901 Interrupt Enable Register B : %x\n", data);
		mc68901->ier = (mc68901->ier & 0xff00) | data;
		mc68901->ipr &= mc68901->ier;
		check_interrupts(device);
		break;

	case MC68901_REGISTER_IPRA:
		if (LOG) logerror("MC68901 Interrupt Pending Register A : %x\n", data);
		mc68901->ipr &= (data << 8) | (mc68901->ipr & 0xff);
		check_interrupts(device);
		break;

	case MC68901_REGISTER_IPRB:
		if (LOG) logerror("MC68901 Interrupt Pending Register B : %x\n", data);
		mc68901->ipr &= (mc68901->ipr & 0xff00) | data;
		check_interrupts(device);
		break;

	case MC68901_REGISTER_ISRA:
		if (LOG) logerror("MC68901 Interrupt In-Service Register A : %x\n", data);
		mc68901->isr &= (data << 8) | (mc68901->isr & 0xff);
		break;

	case MC68901_REGISTER_ISRB:
		if (LOG) logerror("MC68901 Interrupt In-Service Register B : %x\n", data);
		mc68901->isr &= (mc68901->isr & 0xff00) | data;
		break;

	case MC68901_REGISTER_IMRA:
		if (LOG) logerror("MC68901 Interrupt Mask Register A : %x\n", data);
		mc68901->imr = (data << 8) | (mc68901->imr & 0xff);
		mc68901->isr &= mc68901->imr;
		check_interrupts(device);
		break;

	case MC68901_REGISTER_IMRB:
		if (LOG) logerror("MC68901 Interrupt Mask Register B : %x\n", data);
		mc68901->imr = (mc68901->imr & 0xff00) | data;
		mc68901->isr &= mc68901->imr;
		check_interrupts(device);
		break;

	case MC68901_REGISTER_VR:
		if (LOG) logerror("MC68901 Interrupt Vector : %x\n", data & 0xf0);

		mc68901->vr = data & 0xf8;

		if (mc68901->vr & MC68901_VR_S)
		{
			if (LOG) logerror("MC68901 Software End-Of-Interrupt Mode\n");
		}
		else
		{
			if (LOG) logerror("MC68901 Automatic End-Of-Interrupt Mode\n");

			mc68901->isr = 0;
		}
		break;

	case MC68901_REGISTER_TACR:
		mc68901->tacr = data & 0x1f;

		switch (mc68901->tacr & 0x0f)
		{
		case MC68901_TCR_TIMER_STOPPED:
			if (LOG) logerror("MC68901 Timer A Stopped\n");
			timer_enable(mc68901->timer[MC68901_TIMER_A], 0);
			break;

		case MC68901_TCR_TIMER_DELAY_4:
		case MC68901_TCR_TIMER_DELAY_10:
		case MC68901_TCR_TIMER_DELAY_16:
		case MC68901_TCR_TIMER_DELAY_50:
		case MC68901_TCR_TIMER_DELAY_64:
		case MC68901_TCR_TIMER_DELAY_100:
		case MC68901_TCR_TIMER_DELAY_200:
			{
			int divisor = PRESCALER[mc68901->tacr & 0x07];
			if (LOG) logerror("MC68901 Timer A Delay Mode : %u Prescale\n", divisor);
			timer_adjust_periodic(mc68901->timer[MC68901_TIMER_A], ATTOTIME_IN_HZ(mc68901->timer_clock / divisor), 0, ATTOTIME_IN_HZ(mc68901->timer_clock / divisor));
			}
			break;

		case MC68901_TCR_TIMER_EVENT:
			if (LOG) logerror("MC68901 Timer A Event Count Mode\n");
			timer_enable(mc68901->timer[MC68901_TIMER_A], 0);
			break;

		case MC68901_TCR_TIMER_PULSE_4:
		case MC68901_TCR_TIMER_PULSE_10:
		case MC68901_TCR_TIMER_PULSE_16:
		case MC68901_TCR_TIMER_PULSE_50:
		case MC68901_TCR_TIMER_PULSE_64:
		case MC68901_TCR_TIMER_PULSE_100:
		case MC68901_TCR_TIMER_PULSE_200:
			{
			int divisor = PRESCALER[mc68901->tacr & 0x07];
			if (LOG) logerror("MC68901 Timer A Pulse Width Mode : %u Prescale\n", divisor);
			timer_adjust_periodic(mc68901->timer[MC68901_TIMER_A], attotime_never, 0, ATTOTIME_IN_HZ(mc68901->timer_clock / divisor));
			timer_enable(mc68901->timer[MC68901_TIMER_A], 0);
			}
			break;
		}

		if (mc68901->tacr & MC68901_TCR_TIMER_RESET)
		{
			if (LOG) logerror("MC68901 Timer A Reset\n");

			mc68901->to[MC68901_TIMER_A] = 0;

			devcb_call_write_line(&mc68901->out_tao_func, mc68901->to[MC68901_TIMER_A]);
		}
		break;

	case MC68901_REGISTER_TBCR:
		mc68901->tbcr = data & 0x1f;

		switch (mc68901->tbcr & 0x0f)
		{
		case MC68901_TCR_TIMER_STOPPED:
			if (LOG) logerror("MC68901 Timer B Stopped\n");
			timer_enable(mc68901->timer[MC68901_TIMER_B], 0);
			break;

		case MC68901_TCR_TIMER_DELAY_4:
		case MC68901_TCR_TIMER_DELAY_10:
		case MC68901_TCR_TIMER_DELAY_16:
		case MC68901_TCR_TIMER_DELAY_50:
		case MC68901_TCR_TIMER_DELAY_64:
		case MC68901_TCR_TIMER_DELAY_100:
		case MC68901_TCR_TIMER_DELAY_200:
			{
			int divisor = PRESCALER[mc68901->tbcr & 0x07];
			if (LOG) logerror("MC68901 Timer B Delay Mode : %u Prescale\n", divisor);
			timer_adjust_periodic(mc68901->timer[MC68901_TIMER_B], ATTOTIME_IN_HZ(mc68901->timer_clock / divisor), 0, ATTOTIME_IN_HZ(mc68901->timer_clock / divisor));
			}
			break;

		case MC68901_TCR_TIMER_EVENT:
			if (LOG) logerror("MC68901 Timer B Event Count Mode\n");
			timer_enable(mc68901->timer[MC68901_TIMER_B], 0);
			break;

		case MC68901_TCR_TIMER_PULSE_4:
		case MC68901_TCR_TIMER_PULSE_10:
		case MC68901_TCR_TIMER_PULSE_16:
		case MC68901_TCR_TIMER_PULSE_50:
		case MC68901_TCR_TIMER_PULSE_64:
		case MC68901_TCR_TIMER_PULSE_100:
		case MC68901_TCR_TIMER_PULSE_200:
			{
			int divisor = PRESCALER[mc68901->tbcr & 0x07];
			if (LOG) logerror("MC68901 Timer B Pulse Width Mode : %u Prescale\n", DIVISOR);
			timer_adjust_periodic(mc68901->timer[MC68901_TIMER_B], attotime_never, 0, ATTOTIME_IN_HZ(mc68901->timer_clock / divisor));
			timer_enable(mc68901->timer[MC68901_TIMER_B], 0);
			}
			break;
		}

		if (mc68901->tacr & MC68901_TCR_TIMER_RESET)
		{
			if (LOG) logerror("MC68901 Timer B Reset\n");

			mc68901->to[MC68901_TIMER_B] = 0;

			devcb_call_write_line(&mc68901->out_tbo_func, mc68901->to[MC68901_TIMER_B]);
		}
		break;

	case MC68901_REGISTER_TCDCR:
		mc68901->tcdcr = data & 0x6f;

		switch (mc68901->tcdcr & 0x07)
		{
		case MC68901_TCR_TIMER_STOPPED:
			if (LOG) logerror("MC68901 Timer D Stopped\n");
			timer_enable(mc68901->timer[MC68901_TIMER_D], 0);
			break;

		case MC68901_TCR_TIMER_DELAY_4:
		case MC68901_TCR_TIMER_DELAY_10:
		case MC68901_TCR_TIMER_DELAY_16:
		case MC68901_TCR_TIMER_DELAY_50:
		case MC68901_TCR_TIMER_DELAY_64:
		case MC68901_TCR_TIMER_DELAY_100:
		case MC68901_TCR_TIMER_DELAY_200:
			{
			int divisor = PRESCALER[mc68901->tcdcr & 0x07];
			if (LOG) logerror("MC68901 Timer D Delay Mode : %u Prescale\n", divisor);
			timer_adjust_periodic(mc68901->timer[MC68901_TIMER_D], ATTOTIME_IN_HZ(mc68901->timer_clock / divisor), 0, ATTOTIME_IN_HZ(mc68901->timer_clock / divisor));
			}
			break;
		}

		switch ((mc68901->tcdcr >> 4) & 0x07)
		{
		case MC68901_TCR_TIMER_STOPPED:
			if (LOG) logerror("MC68901 Timer C Stopped\n");
			timer_enable(mc68901->timer[MC68901_TIMER_C], 0);
			break;

		case MC68901_TCR_TIMER_DELAY_4:
		case MC68901_TCR_TIMER_DELAY_10:
		case MC68901_TCR_TIMER_DELAY_16:
		case MC68901_TCR_TIMER_DELAY_50:
		case MC68901_TCR_TIMER_DELAY_64:
		case MC68901_TCR_TIMER_DELAY_100:
		case MC68901_TCR_TIMER_DELAY_200:
			{
			int divisor = PRESCALER[(mc68901->tcdcr >> 4) & 0x07];
			if (LOG) logerror("MC68901 Timer C Delay Mode : %u Prescale\n", divisor);
			timer_adjust_periodic(mc68901->timer[MC68901_TIMER_C], ATTOTIME_IN_HZ(mc68901->timer_clock / divisor), 0, ATTOTIME_IN_HZ(mc68901->timer_clock / divisor));
			}
			break;
		}
		break;

	case MC68901_REGISTER_TADR:
		if (LOG) logerror("MC68901 Timer A Data Register : %x\n", data);

		mc68901->tdr[MC68901_TIMER_A] = data;

		if (!timer_enabled(mc68901->timer[MC68901_TIMER_A]))
		{
			mc68901->tmc[MC68901_TIMER_A] = data;
		}
		break;

	case MC68901_REGISTER_TBDR:
		if (LOG) logerror("MC68901 Timer B Data Register : %x\n", data);

		mc68901->tdr[MC68901_TIMER_B] = data;

		if (!timer_enabled(mc68901->timer[MC68901_TIMER_B]))
		{
			mc68901->tmc[MC68901_TIMER_B] = data;
		}
		break;

	case MC68901_REGISTER_TCDR:
		if (LOG) logerror("MC68901 Timer C Data Register : %x\n", data);

		mc68901->tdr[MC68901_TIMER_C] = data;

		if (!timer_enabled(mc68901->timer[MC68901_TIMER_C]))
		{
			mc68901->tmc[MC68901_TIMER_C] = data;
		}
		break;

	case MC68901_REGISTER_TDDR:
		if (LOG) logerror("MC68901 Timer D Data Register : %x\n", data);

		mc68901->tdr[MC68901_TIMER_D] = data;

		if (!timer_enabled(mc68901->timer[MC68901_TIMER_D]))
		{
			mc68901->tmc[MC68901_TIMER_D] = data;
		}
		break;

	case MC68901_REGISTER_SCR:
		if (LOG) logerror("MC68901 Sync Character : %x\n", data);

		mc68901->scr = data;
		break;

	case MC68901_REGISTER_UCR:
		if (data & MC68901_UCR_PARITY_ENABLED)
		{
			if (data & MC68901_UCR_PARITY_EVEN)
			{
				if (LOG) logerror("MC68901 Parity : Even\n");
			}
			else
			{
				if (LOG) logerror("MC68901 Parity : Odd\n");
			}
		}
		else
		{
			if (LOG) logerror("MC68901 Parity : Disabled\n");
		}

		switch (data & 0x60)
		{
		case MC68901_UCR_WORD_LENGTH_8:
			mc68901->rxtx_word = 8;
			if (LOG) logerror("MC68901 Word Length : 8 bits\n");
			break;
		case MC68901_UCR_WORD_LENGTH_7:
			mc68901->rxtx_word = 7;
			if (LOG) logerror("MC68901 Word Length : 7 bits\n");
			break;
		case MC68901_UCR_WORD_LENGTH_6:
			mc68901->rxtx_word = 6;
			if (LOG) logerror("MC68901 Word Length : 6 bits\n");
			break;
		case MC68901_UCR_WORD_LENGTH_5:
			mc68901->rxtx_word = 5;
			if (LOG) logerror("MC68901 Word Length : 5 bits\n");
			break;
		}

		switch (data & 0x18)
		{
		case MC68901_UCR_START_STOP_0_0:
			mc68901->rxtx_start = 0;
			mc68901->rxtx_stop = 0;
			if (LOG) logerror("MC68901 Start Bits : 0, Stop Bits : 0, Format : synchronous\n");
			break;
		case MC68901_UCR_START_STOP_1_1:
			mc68901->rxtx_start = 1;
			mc68901->rxtx_stop = 1;
			if (LOG) logerror("MC68901 Start Bits : 1, Stop Bits : 1, Format : asynchronous\n");
			break;
		case MC68901_UCR_START_STOP_1_15:
			mc68901->rxtx_start = 1;
			mc68901->rxtx_stop = 1;
			if (LOG) logerror("MC68901 Start Bits : 1, Stop Bits : 1??, Format : asynchronous\n");
			break;
		case MC68901_UCR_START_STOP_1_2:
			mc68901->rxtx_start = 1;
			mc68901->rxtx_stop = 2;
			if (LOG) logerror("MC68901 Start Bits : 1, Stop Bits : 2, Format : asynchronous\n");
			break;
		}

		if (data & MC68901_UCR_CLOCK_DIVIDE_16)
		{
			if (LOG) logerror("MC68901 Rx/Tx Clock Divisor : 16\n");
		}
		else
		{
			if (LOG) logerror("MC68901 Rx/Tx Clock Divisor : 1\n");
		}

		mc68901->ucr = data;
		break;

	case MC68901_REGISTER_RSR:
		if ((data & MC68901_RSR_RCV_ENABLE) == 0)
		{
			if (LOG) logerror("MC68901 Receiver Disabled\n");
			mc68901->rsr = 0;
		}
		else
		{
			if (LOG) logerror("MC68901 Receiver Enabled\n");

			if (data & MC68901_RSR_SYNC_STRIP_ENABLE)
			{
				if (LOG) logerror("MC68901 Sync Strip Enabled\n");
			}
			else
			{
				if (LOG) logerror("MC68901 Sync Strip Disabled\n");
			}

			if (data & MC68901_RSR_FOUND_SEARCH)
				if (LOG) logerror("MC68901 Receiver Search Mode Enabled\n");

			mc68901->rsr = data & 0x0b;
		}
		break;

	case MC68901_REGISTER_TSR:
		if ((data & MC68901_TSR_XMIT_ENABLE) == 0)
		{
			if (LOG) logerror("MC68901 Transmitter Disabled\n");

			mc68901->tsr = data & 0x27;
		}
		else
		{
			if (LOG) logerror("MC68901 Transmitter Enabled\n");

			switch (data & 0x06)
			{
			case MC68901_TSR_OUTPUT_HI_Z:
				if (LOG) logerror("MC68901 Transmitter Disabled Output State : Hi-Z\n");
				break;
			case MC68901_TSR_OUTPUT_LOW:
				if (LOG) logerror("MC68901 Transmitter Disabled Output State : 0\n");
				break;
			case MC68901_TSR_OUTPUT_HIGH:
				if (LOG) logerror("MC68901 Transmitter Disabled Output State : 1\n");
				break;
			case MC68901_TSR_OUTPUT_LOOP:
				if (LOG) logerror("MC68901 Transmitter Disabled Output State : Loop\n");
				break;
			}

			if (data & MC68901_TSR_BREAK)
			{
				if (LOG) logerror("MC68901 Transmitter Break Enabled\n");
			}
			else
			{
				if (LOG) logerror("MC68901 Transmitter Break Disabled\n");
			}

			if (data & MC68901_TSR_AUTO_TURNAROUND)
			{
				if (LOG) logerror("MC68901 Transmitter Auto Turnaround Enabled\n");
			}
			else
			{
				if (LOG) logerror("MC68901 Transmitter Auto Turnaround Disabled\n");
			}

			mc68901->tsr = data & 0x2f;
			mc68901->tsr |= MC68901_TSR_BUFFER_EMPTY;  // x68000 expects the buffer to be empty, so this will do for now
		}
		break;

	case MC68901_REGISTER_UDR:
		if (LOG) logerror("MC68901 UDR %x\n", data);
		mc68901->udr = data;
		//mc68901->tsr &= ~MC68901_TSR_BUFFER_EMPTY;
		break;
	}
}

/*-------------------------------------------------
    timer_count - timer count down
-------------------------------------------------*/

static void timer_count(device_t *device, int index)
{
	mc68901_t *mc68901 = get_safe_token(device);

	if (mc68901->tmc[index] == 0x01)
	{
		/* toggle timer output signal */
		mc68901->to[index] = !mc68901->to[index];

		switch (index)
		{
		case MC68901_TIMER_A:	devcb_call_write_line(&mc68901->out_tao_func, mc68901->to[index]);	break;
		case MC68901_TIMER_B:	devcb_call_write_line(&mc68901->out_tbo_func, mc68901->to[index]);	break;
		case MC68901_TIMER_C:	devcb_call_write_line(&mc68901->out_tco_func, mc68901->to[index]);	break;
		case MC68901_TIMER_D:	devcb_call_write_line(&mc68901->out_tdo_func, mc68901->to[index]);	break;
		}

		if (mc68901->ier & MC68901_INT_MASK_TIMER[index])
		{
			/* signal timer elapsed interrupt */
			take_interrupt(device, MC68901_INT_MASK_TIMER[index]);
		}

		/* load main counter */
		mc68901->tmc[index] = mc68901->tdr[index];
	}
	else
	{
		/* count down */
		mc68901->tmc[index]--;
	}
}

/*-------------------------------------------------
    timer_input - timer input
-------------------------------------------------*/

static void timer_input(device_t *device, int index, int value)
{
	mc68901_t *mc68901 = get_safe_token(device);

	int bit = MC68901_GPIO_TIMER[index];
	int aer = BIT(mc68901->aer, bit);
	int cr = index ? mc68901->tbcr : mc68901->tacr;

	switch (cr & 0x0f)
	{
	case MC68901_TCR_TIMER_EVENT:
		if (((mc68901->ti[index] ^ aer) == 1) && ((value ^ aer) == 0))
		{
			timer_count(device, index);
		}

		mc68901->ti[index] = value;
		break;

	case MC68901_TCR_TIMER_PULSE_4:
	case MC68901_TCR_TIMER_PULSE_10:
	case MC68901_TCR_TIMER_PULSE_16:
	case MC68901_TCR_TIMER_PULSE_50:
	case MC68901_TCR_TIMER_PULSE_64:
	case MC68901_TCR_TIMER_PULSE_100:
	case MC68901_TCR_TIMER_PULSE_200:
		timer_enable(mc68901->timer[index], (value == aer));

		if (((mc68901->ti[index] ^ aer) == 0) && ((value ^ aer) == 1))
		{
			if (mc68901->ier & MC68901_INT_MASK_GPIO[bit])
			{
				take_interrupt(device, MC68901_INT_MASK_GPIO[bit]);
			}
		}

		mc68901->ti[index] = value;
		break;
	}
}

/*-------------------------------------------------
    TIMER_CALLBACK( timer_a )
-------------------------------------------------*/

static TIMER_CALLBACK( timer_a ) { timer_count((device_t *)ptr, MC68901_TIMER_A); }
static TIMER_CALLBACK( timer_b ) { timer_count((device_t *)ptr, MC68901_TIMER_B); }
static TIMER_CALLBACK( timer_c ) { timer_count((device_t *)ptr, MC68901_TIMER_C); }
static TIMER_CALLBACK( timer_d ) { timer_count((device_t *)ptr, MC68901_TIMER_D); }

/*-------------------------------------------------
    mc68901_get_vector - get interrupt vector
-------------------------------------------------*/

int mc68901_get_vector(device_t *device)
{
	mc68901_t *mc68901 = (mc68901_t *)get_safe_token(device);

	int ch;

	for (ch = 15; ch >= 0; ch--)
	{
		if (BIT(mc68901->imr, ch) && BIT(mc68901->ipr, ch))
		{
			if (mc68901->vr & MC68901_VR_S)
			{
				/* set interrupt-in-service bit */
				mc68901->isr |= (1 << ch);
			}

			/* clear interrupt pending bit */
			mc68901->ipr &= ~(1 << ch);

			check_interrupts(device);

			return (mc68901->vr & 0xf0) | ch;
		}
	}

	return M68K_INT_ACK_SPURIOUS;
}

/*-------------------------------------------------
    mc68901_tai_w - timer A input
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( mc68901_tai_w )
{
	timer_input(device, MC68901_TIMER_A, state);
}

/*-------------------------------------------------
    mc68901_tbi_w - timer B input
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( mc68901_tbi_w )
{
	timer_input(device, MC68901_TIMER_B, state);
}

/*-------------------------------------------------
    mc68901_rx_clock_w - receiver clock input
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( mc68901_rx_clock_w )
{
	if (state)
	{
		serial_receive(device);
	}
}

/*-------------------------------------------------
    mc68901_tx_clock_w - transmitter clock input
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( mc68901_tx_clock_w )
{
	if (state)
	{
		serial_transmit(device);
	}
}

/*-------------------------------------------------
    DEVICE_START( mc68901 )
-------------------------------------------------*/

static DEVICE_START( mc68901 )
{
	mc68901_t *mc68901 = (mc68901_t *)downcast<legacy_device_base *>(device)->token();
	const mc68901_interface *intf = get_interface(device);

	/* resolve callbacks */
	devcb_resolve_read8(&mc68901->in_gpio_func, &intf->in_gpio_func, device);
	devcb_resolve_write8(&mc68901->out_gpio_func, &intf->out_gpio_func, device);
	devcb_resolve_read_line(&mc68901->in_si_func, &intf->in_si_func, device);
	devcb_resolve_write_line(&mc68901->out_so_func, &intf->out_so_func, device);
	devcb_resolve_write_line(&mc68901->out_tao_func, &intf->out_tao_func, device);
	devcb_resolve_write_line(&mc68901->out_tbo_func, &intf->out_tbo_func, device);
	devcb_resolve_write_line(&mc68901->out_tco_func, &intf->out_tco_func, device);
	devcb_resolve_write_line(&mc68901->out_tdo_func, &intf->out_tdo_func, device);
	devcb_resolve_write_line(&mc68901->out_irq_func, &intf->out_irq_func, device);

	/* set initial values */
	mc68901->timer_clock = intf->timer_clock;
	mc68901->tsr = MC68901_TSR_BUFFER_EMPTY;

	/* create the timers */
	mc68901->timer[MC68901_TIMER_A] = timer_alloc(device->machine, timer_a, (void *)device);
	mc68901->timer[MC68901_TIMER_B] = timer_alloc(device->machine, timer_b, (void *)device);
	mc68901->timer[MC68901_TIMER_C] = timer_alloc(device->machine, timer_c, (void *)device);
	mc68901->timer[MC68901_TIMER_D] = timer_alloc(device->machine, timer_d, (void *)device);

	if (intf->rx_clock > 0)
	{
		mc68901->rx_timer = timer_alloc(device->machine, rx_tick, (void *)device);
		timer_adjust_periodic(mc68901->rx_timer, attotime_zero, 0, ATTOTIME_IN_HZ(intf->rx_clock));
	}

	if (intf->tx_clock > 0)
	{
		mc68901->tx_timer = timer_alloc(device->machine, tx_tick, (void *)device);
		timer_adjust_periodic(mc68901->tx_timer, attotime_zero, 0, ATTOTIME_IN_HZ(intf->tx_clock));
	}

	mc68901->poll_timer = timer_alloc(device->machine, gpio_poll_tick, (void *)device);
	timer_adjust_periodic(mc68901->poll_timer, attotime_zero, 0, ATTOTIME_IN_HZ(device->clock() / 4));

	/* register for state saving */
	state_save_register_device_item(device, 0, mc68901->gpip);
	state_save_register_device_item(device, 0, mc68901->aer);
	state_save_register_device_item(device, 0, mc68901->ddr);
	state_save_register_device_item(device, 0, mc68901->ier);
	state_save_register_device_item(device, 0, mc68901->ipr);
	state_save_register_device_item(device, 0, mc68901->isr);
	state_save_register_device_item(device, 0, mc68901->imr);
	state_save_register_device_item(device, 0, mc68901->vr);
	state_save_register_device_item(device, 0, mc68901->tacr);
	state_save_register_device_item(device, 0, mc68901->tbcr);
	state_save_register_device_item(device, 0, mc68901->tcdcr);
	state_save_register_device_item_array(device, 0, mc68901->tdr);
	state_save_register_device_item_array(device, 0, mc68901->tmc);
	state_save_register_device_item_array(device, 0, mc68901->to);
	state_save_register_device_item_array(device, 0, mc68901->ti);
	state_save_register_device_item(device, 0, mc68901->scr);
	state_save_register_device_item(device, 0, mc68901->ucr);
	state_save_register_device_item(device, 0, mc68901->rsr);
	state_save_register_device_item(device, 0, mc68901->tsr);
	state_save_register_device_item(device, 0, mc68901->udr);
	state_save_register_device_item(device, 0, mc68901->rx_bits);
	state_save_register_device_item(device, 0, mc68901->tx_bits);
	state_save_register_device_item(device, 0, mc68901->rx_parity);
	state_save_register_device_item(device, 0, mc68901->tx_parity);
	state_save_register_device_item(device, 0, mc68901->rx_state);
	state_save_register_device_item(device, 0, mc68901->tx_state);
	state_save_register_device_item(device, 0, mc68901->rx_buffer);
	state_save_register_device_item(device, 0, mc68901->tx_buffer);
	state_save_register_device_item(device, 0, mc68901->xmit_state);
	state_save_register_device_item(device, 0, mc68901->rxtx_word);
	state_save_register_device_item(device, 0, mc68901->rxtx_start);
	state_save_register_device_item(device, 0, mc68901->rxtx_stop);
	state_save_register_device_item(device, 0, mc68901->rsr_read);
	state_save_register_device_item(device, 0, mc68901->next_rsr);
}

/*-------------------------------------------------
    DEVICE_RESET( mc68901 )
-------------------------------------------------*/

static DEVICE_RESET( mc68901 )
{
	mc68901_register_w(device, MC68901_REGISTER_GPIP, 0);
	mc68901_register_w(device, MC68901_REGISTER_AER, 0);
	mc68901_register_w(device, MC68901_REGISTER_DDR, 0);
	mc68901_register_w(device, MC68901_REGISTER_IERA, 0);
	mc68901_register_w(device, MC68901_REGISTER_IERB, 0);
	mc68901_register_w(device, MC68901_REGISTER_IPRA, 0);
	mc68901_register_w(device, MC68901_REGISTER_IPRB, 0);
	mc68901_register_w(device, MC68901_REGISTER_ISRA, 0);
	mc68901_register_w(device, MC68901_REGISTER_ISRB, 0);
	mc68901_register_w(device, MC68901_REGISTER_IMRA, 0);
	mc68901_register_w(device, MC68901_REGISTER_IMRB, 0);
	mc68901_register_w(device, MC68901_REGISTER_VR, 0);
	mc68901_register_w(device, MC68901_REGISTER_TACR, 0);
	mc68901_register_w(device, MC68901_REGISTER_TBCR, 0);
	mc68901_register_w(device, MC68901_REGISTER_TCDCR, 0);
	mc68901_register_w(device, MC68901_REGISTER_SCR, 0);
	mc68901_register_w(device, MC68901_REGISTER_UCR, 0);
	mc68901_register_w(device, MC68901_REGISTER_RSR, 0);
}

/*-------------------------------------------------
    DEVICE_GET_INFO( mc68901 )
-------------------------------------------------*/

DEVICE_GET_INFO( mc68901 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(mc68901_t);				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(mc68901);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(mc68901);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Motorola MC68901");		break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "MC68901 MFP");				break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team");	break;
	}
}

/*-------------------------------------------------
    DEVICE_GET_INFO( mk68901 )
-------------------------------------------------*/

DEVICE_GET_INFO( mk68901 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(mc68901_t);				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(mc68901);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(mc68901);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Mostek MK68901");			break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "MK68901 MFP");				break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team");	break;
	}
}

DEFINE_LEGACY_DEVICE(MC68901, mc68901);
DEFINE_LEGACY_DEVICE(MK68901, mk68901);
