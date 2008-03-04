/*

	TODO:

	- poll TAI/TBI pins
	- daisy chaining
	- correct gpio_timer period
	- disable GPIO3/4 interrupts when timer A/B in pulse mode
	- spurious interrupt

		If you look at the MFP datasheet it is obvious that it can generate the conditions for a spurious interrupt.
		However the fact that they indeed happen in the ST is quite interesting.

		The MFP will generate a spurious interrupt if interrupts are disabled (by changing the IERA/IERB registers)
		at the “precise point”. The precise point would be after the system (but not necessarily the CPU, see below)
		triggered an MFP interrupt, and before the CPU drives the interrupt acknowledge cycle.

		If the MFP was connected directly to the CPU, spurious interrupts probably couldn’t happen. However in the
		ST, GLUE seats in the middle and handles all the interrupt timing. It is possible that GLUE introduces a
		delay between detecting a change in the MFP interrupt request signal and actually propagating the change to
		the CPU IPL signals (it is even possible that GLUE make some kind of latching). This would create a window
		long enough for the “precise point” described above.

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

#define LOG		(0)

struct _mc68901_t
{
	int device_type;
	const mc68901_interface *intf;

	UINT8 gpip;
	UINT8 aer;
	UINT8 ddr;

	UINT16 ier, ipr, isr, imr;
	UINT8 vr;
	int irqlevel;

	UINT8 tacr, tbcr, tcdcr;
	UINT8 tdr[MC68901_MAX_TIMERS];
	UINT8 tmc[MC68901_MAX_TIMERS];
	int to[MC68901_MAX_TIMERS];
	int ti[MC68901_MAX_TIMERS];
	emu_timer *timer[MC68901_MAX_TIMERS];

	UINT8 ucr;
	UINT8 udr;
	UINT8 scr;
	UINT8 tsr, rsr, next_rsr, rsr_read;
	UINT8 rx_buffer, tx_buffer;
	int	rx_bits, tx_bits;
	int	rx_parity, tx_parity;
	int	rx_state, tx_state, xmit_state;
	int rxtx_word, rxtx_start, rxtx_stop;
} mfp_68901;

static const int MC68901_INT_MASK_GPIO[] =
{
	MC68901_IR_GPIP_0, MC68901_IR_GPIP_1, MC68901_IR_GPIP_2, MC68901_IR_GPIP_3,
	MC68901_IR_GPIP_4, MC68901_IR_GPIP_5, MC68901_IR_GPIP_6, MC68901_IR_GPIP_7
};

static const int MC68901_INT_MASK_TIMER[] =
{
	MC68901_IR_TIMER_A, MC68901_IR_TIMER_B, MC68901_IR_TIMER_C, MC68901_IR_TIMER_D
};

static const int MC68901_LOOPBACK_TIMER[] =
{
	MC68901_TAO_LOOPBACK, MC68901_TBO_LOOPBACK, MC68901_TCO_LOOPBACK, MC68901_TDO_LOOPBACK
};

static const int MC68901_GPIO_TIMER[] =
{
	MC68901_GPIP_4, MC68901_GPIP_3
};

static const int PRESCALER[] = { 0, 4, 10, 16, 50, 64, 100, 200 };

static void mc68901_check_interrupts(mc68901_t *chip)
{
	int irqlevel = (chip->ipr & chip->imr) ? HOLD_LINE : CLEAR_LINE;

	if (irqlevel != chip->irqlevel)
	{
		chip->intf->irq_callback(chip, irqlevel);
	}
}

static void mc68901_interrupt(mc68901_t *chip, UINT16 mask)
{
	chip->ipr |= mask;

	mc68901_check_interrupts(chip);
}

static TIMER_CALLBACK( mc68901_gpio_poll_tick )
{
	mc68901_t *chip = ptr;

	UINT8 gpio = chip->intf->gpio_r(0);

	UINT8 gpold = (chip->gpip & ~chip->ddr) ^ chip->aer;
	UINT8 gpnew = (gpio & ~chip->ddr) ^ chip->aer;

	int bit;

	for (bit = 0; bit < 8; bit++) // loop thru each bit from 0 to 7
	{
		if ((BIT(gpold, bit) == 1) && (BIT(gpnew, bit) == 0)) // if transition from 1 to 0 is detected...
		{
			logerror("MC68901 Edge Transition Detected on GPIO%u\n", bit);

			if (chip->ier & MC68901_INT_MASK_GPIO[bit]) // AND interrupt enabled bit is set...
			{
				logerror("MC68901 Interrupt Pending for GPIO%u\n", bit);

				mc68901_interrupt(chip, MC68901_INT_MASK_GPIO[bit]); // set interrupt pending bit
			}
		}
	}

	chip->gpip = (gpio & ~chip->ddr) | (chip->gpip & chip->ddr);
}

/* USART */

static void mc68901_rx_buffer_full(mc68901_t *chip)
{
	if (chip->ier & MC68901_IR_RCV_BUFFER_FULL)
	{
		mc68901_interrupt(chip, MC68901_IR_RCV_BUFFER_FULL);
	}
}

static void mc68901_rx_error(mc68901_t *chip)
{
	if (chip->ier & MC68901_IR_RCV_ERROR)
	{
		mc68901_interrupt(chip, MC68901_IR_RCV_ERROR);
	}
	else
	{
		mc68901_rx_buffer_full(chip);
	}
}

static void mc68901_tx_buffer_empty(mc68901_t *chip)
{
	if (chip->ier & MC68901_IR_XMIT_BUFFER_EMPTY)
	{
		mc68901_interrupt(chip, MC68901_IR_XMIT_BUFFER_EMPTY);
	}
}

static void mc68901_tx_error(mc68901_t *chip)
{
	if (chip->ier & MC68901_IR_XMIT_ERROR)
	{
		mc68901_interrupt(chip, MC68901_IR_XMIT_ERROR);
	}
	else
	{
		mc68901_tx_buffer_empty(chip);
	}
}

static int mc68901_parity(UINT8 b)
{
	b ^= b >> 4;
	b ^= b >> 2;
	b ^= b >> 1;
	return b & 1;
}

static void mc68901_serial_rx(mc68901_t *chip)
{
	if (chip->rsr & MC68901_RSR_RCV_ENABLE)
	{
		switch (chip->rx_state)
		{
		case MC68901_SERIAL_START:
			if (chip->intf->rx_pin == 0)
			{
				chip->rsr |= MC68901_RSR_CHAR_IN_PROGRESS;
				chip->rx_bits = 0;
				chip->rx_buffer = 0;
				chip->rx_state = MC68901_SERIAL_DATA;
				chip->next_rsr = MC68901_RSR_BREAK;
			}
			break;

		case MC68901_SERIAL_DATA:
			if ((chip->next_rsr & MC68901_RSR_BREAK) && (*chip->intf->rx_pin == 1) && chip->rsr_read)
			{
				chip->next_rsr &= ~MC68901_RSR_BREAK;
			}

			chip->rx_buffer >>= 1;
			chip->rx_buffer |= (*chip->intf->rx_pin << 7);
			chip->rx_bits++;

			if (chip->rx_bits == chip->rxtx_word)
			{
				if (chip->rxtx_word < 8)
				{
					chip->rx_buffer >>= (8 - chip->rxtx_word);
				}

				chip->rsr &= ~MC68901_RSR_CHAR_IN_PROGRESS;

				if (chip->ucr & MC68901_UCR_PARITY_ENABLED)
				{
					chip->rx_state = MC68901_SERIAL_PARITY;
				}
				else
				{
					chip->rx_state = MC68901_SERIAL_STOP;
				}
			}
			break;

		case MC68901_SERIAL_PARITY:
			chip->rx_parity = *chip->intf->rx_pin;

			if (chip->rx_parity != (mc68901_parity(chip->rx_buffer) ^ ((chip->ucr & MC68901_UCR_PARITY_EVEN) >> 1)))
			{
				chip->next_rsr |= MC68901_RSR_PARITY_ERROR;
			}

			chip->rx_state = MC68901_SERIAL_STOP;
			break;

		case MC68901_SERIAL_STOP:
			if (*chip->intf->rx_pin == 1)
			{
				if (!((chip->rsr & MC68901_RSR_SYNC_STRIP_ENABLE) && (chip->rx_buffer == chip->scr)))
				{
					if (!(chip->rsr & MC68901_RSR_OVERRUN_ERROR))
					{
						if (chip->rsr & MC68901_RSR_BUFFER_FULL)
						{
							// incoming word received but last word in receive buffer has not been read
							chip->next_rsr |= MC68901_RSR_OVERRUN_ERROR;
						}
						else
						{
							// incoming word received and receive buffer is empty
							chip->rsr |= MC68901_RSR_BUFFER_FULL;
							chip->udr = chip->rx_buffer;
							mc68901_rx_buffer_full(chip);
						}
					}
				}
			}
			else
			{
				if (chip->rx_buffer)
				{
					// non-zero data word not followed by a stop bit
					chip->next_rsr |= MC68901_RSR_FRAME_ERROR;
				}
			}

			chip->rx_state = MC68901_SERIAL_START;
			break;
		}
	}
}

static TIMER_CALLBACK( rx_tick )
{
	mc68901_serial_rx(ptr);
}

static void mc68901_transmit_disabled(mc68901_t *chip)
{
	switch (chip->tsr & MC68901_TSR_OUTPUT_MASK)
	{
	case MC68901_TSR_OUTPUT_HI_Z:
		// indeterminate
	case MC68901_TSR_OUTPUT_LOW:
		*chip->intf->tx_pin = 0;
		break;

	case MC68901_TSR_OUTPUT_HIGH:
	case MC68901_TSR_OUTPUT_LOOP:
		*chip->intf->tx_pin = 1;
		break;
	}
}

static void mc68901_transmit(mc68901_t *chip)
{
	switch (chip->tx_state)
	{
	case MC68901_SERIAL_START:
		if (chip->tsr & MC68901_TSR_UNDERRUN_ERROR)
		{
			if (chip->tsr & MC68901_TSR_XMIT_ENABLE)
			{
				*chip->intf->tx_pin = 1;
			}
			else
			{
				mc68901_transmit_disabled(chip);
			}
		}
		else
		{
			if (chip->tsr & MC68901_TSR_BUFFER_EMPTY)
			{
				chip->tsr |= MC68901_TSR_UNDERRUN_ERROR;

				if (chip->tsr & MC68901_TSR_XMIT_ENABLE)
				{
					*chip->intf->tx_pin = 1;
				}
				else
				{
					mc68901_transmit_disabled(chip);
				}
			}
			else
			{
				*chip->intf->tx_pin = 0;
				chip->tx_buffer = chip->udr;
				chip->tx_bits = 0;
				chip->tx_state = MC68901_SERIAL_DATA;
				chip->tsr |= MC68901_TSR_BUFFER_EMPTY;
				mc68901_tx_buffer_empty(chip);
				chip->tx_parity = mc68901_parity(chip->tx_buffer);
			}
		}
		break;

	case MC68901_SERIAL_DATA:
		*chip->intf->tx_pin = chip->tx_buffer & 0x01;
		chip->tx_buffer >>= 1;
		chip->tx_bits++;

		if (chip->tx_bits == chip->rxtx_word)
		{
			if (chip->ucr & MC68901_UCR_PARITY_ENABLED)
			{
				chip->tx_state = MC68901_SERIAL_PARITY;
			}
			else
			{
				chip->tx_state = MC68901_SERIAL_STOP;
			}
		}
		break;

	case MC68901_SERIAL_PARITY:
		if (chip->rxtx_word < 8)
		{
			// highest bit in buffer is user-specified parity bit

			*chip->intf->tx_pin = chip->tx_buffer & 0x01;
		}
		else
		{
			// use previously calculated parity

			*chip->intf->tx_pin = chip->tx_parity ^ ((chip->ucr & MC68901_UCR_PARITY_EVEN) >> 1);
		}

		chip->tx_state = MC68901_SERIAL_STOP;
		break;

	case MC68901_SERIAL_STOP:
		*chip->intf->tx_pin = 1;

		if (chip->tsr & MC68901_TSR_XMIT_ENABLE)
		{
			chip->tx_state = MC68901_SERIAL_START;
		}
		else
		{
			if (chip->tsr & MC68901_TSR_AUTO_TURNAROUND)
			{
				chip->tsr |= MC68901_TSR_XMIT_ENABLE;
				chip->tx_state = MC68901_SERIAL_START;
			}
			else
			{
				chip->xmit_state = MC68901_XMIT_OFF;
				chip->tsr |= MC68901_TSR_END_OF_XMIT;
				mc68901_tx_error(chip);
			}
		}
		break;
	}
}

static void mc68901_serial_tx(mc68901_t *chip)
{
	switch (chip->xmit_state)
	{
	case MC68901_XMIT_OFF:
		mc68901_transmit_disabled(chip);
		break;

	case MC68901_XMIT_STARTING:
		if (chip->tsr & MC68901_TSR_XMIT_ENABLE)
		{
			*chip->intf->tx_pin = 1;
			chip->xmit_state = MC68901_XMIT_ON;
		}
		else
		{
			chip->xmit_state = MC68901_XMIT_OFF;
			chip->tsr |= MC68901_TSR_END_OF_XMIT;
		}
		break;

	case MC68901_XMIT_BREAK:
		if (chip->tsr & MC68901_TSR_XMIT_ENABLE)
		{
			if (chip->tsr & MC68901_TSR_BREAK)
			{
				*chip->intf->tx_pin = 1;
			}
			else
			{
				chip->xmit_state = MC68901_XMIT_ON;
			}
		}
		else
		{
			chip->xmit_state = MC68901_XMIT_OFF;
			chip->tsr |= MC68901_TSR_END_OF_XMIT;
		}
		break;

	case MC68901_XMIT_ON:
		mc68901_transmit(chip);
		break;
	}
}

static TIMER_CALLBACK( tx_tick )
{
	mc68901_serial_tx(ptr);
}

UINT8 mc68901_register_r(mc68901_t *chip, int reg)
{
	assert(chip != NULL);

	switch (reg)
	{
	case MC68901_REGISTER_GPIP:  return chip->gpip;
	case MC68901_REGISTER_AER:   return chip->aer;
	case MC68901_REGISTER_DDR:   return chip->ddr;

	case MC68901_REGISTER_IERA:  return chip->ier >> 8;
	case MC68901_REGISTER_IERB:  return chip->ier & 0xff;
	case MC68901_REGISTER_IPRA:  return chip->ipr >> 8;
	case MC68901_REGISTER_IPRB:  return chip->ipr & 0xff;
	case MC68901_REGISTER_ISRA:  return chip->isr >> 8;
	case MC68901_REGISTER_ISRB:  return chip->isr & 0xff;
	case MC68901_REGISTER_IMRA:  return chip->imr >> 8;
	case MC68901_REGISTER_IMRB:  return chip->imr & 0xff;
	case MC68901_REGISTER_VR:    return chip->vr;

	case MC68901_REGISTER_TACR:  return chip->tacr;
	case MC68901_REGISTER_TBCR:  return chip->tbcr;
	case MC68901_REGISTER_TCDCR: return chip->tcdcr;
	case MC68901_REGISTER_TADR:  return chip->tmc[MC68901_TIMER_A];
	case MC68901_REGISTER_TBDR:  return chip->tmc[MC68901_TIMER_B];
	case MC68901_REGISTER_TCDR:  return chip->tmc[MC68901_TIMER_C];
	case MC68901_REGISTER_TDDR:  return chip->tmc[MC68901_TIMER_D];

	case MC68901_REGISTER_SCR:   return chip->scr;
	case MC68901_REGISTER_UCR:   return chip->ucr;
	case MC68901_REGISTER_RSR:
		chip->rsr_read = 1;
		return chip->rsr;

	case MC68901_REGISTER_TSR:
		{
			// clear UE bit (in reality, this won't be cleared until one full clock cycle of the transmitter has passed since the bit was set)

			UINT8 tsr = chip->tsr;
			chip->tsr &= 0xbf;

			return tsr;
		}

	case MC68901_REGISTER_UDR:
		// load RSR with latched value

		chip->rsr = (chip->next_rsr & 0x7c) | (chip->rsr & 0x03);
		chip->next_rsr = 0;

		// signal receiver error interrupt

		if (chip->rsr & 0x78)
		{
			mc68901_rx_error(chip);
		}

		return chip->udr;

	default:					  return 0;
	}
}

#define DIVISOR PRESCALER[data & 0x07]

void mc68901_register_w(mc68901_t *chip, int reg, UINT8 data)
{
	assert(chip != NULL);

	switch (reg)
	{
	case MC68901_REGISTER_GPIP:
		logerror("MC68901 General Purpose I/O : %x\n", data);
		chip->gpip = data & chip->ddr;

		if (chip->intf->gpio_w)
		{
			chip->intf->gpio_w(0, chip->gpip);
		}
		break;

	case MC68901_REGISTER_AER:
		logerror("MC68901 Active Edge Register : %x\n", data);
		chip->aer = data;
		break;

	case MC68901_REGISTER_DDR:
		logerror("MC68901 Data Direction Register : %x\n", data);
		chip->ddr = data;
		break;

	case MC68901_REGISTER_IERA:
		logerror("MC68901 Interrupt Enable Register A : %x\n", data);
		chip->ier = (data << 8) | (chip->ier & 0xff);
		chip->ipr &= chip->ier;
		mc68901_check_interrupts(chip);
		break;

	case MC68901_REGISTER_IERB:
		logerror("MC68901 Interrupt Enable Register B : %x\n", data);
		chip->ier = (chip->ier & 0xff00) | data;
		chip->ipr &= chip->ier;
		mc68901_check_interrupts(chip);
		break;

	case MC68901_REGISTER_IPRA:
		logerror("MC68901 Interrupt Pending Register A : %x\n", data);
		chip->ipr &= (data << 8) | (chip->ipr & 0xff);
		mc68901_check_interrupts(chip);
		break;

	case MC68901_REGISTER_IPRB:
		logerror("MC68901 Interrupt Pending Register B : %x\n", data);
		chip->ipr &= (chip->ipr & 0xff00) | data;
		mc68901_check_interrupts(chip);
		break;

	case MC68901_REGISTER_ISRA:
		logerror("MC68901 Interrupt In-Service Register A : %x\n", data);
		chip->isr &= (data << 8) | (chip->isr & 0xff);
		break;

	case MC68901_REGISTER_ISRB:
		logerror("MC68901 Interrupt In-Service Register B : %x\n", data);
		chip->isr &= (chip->isr & 0xff00) | data;
		break;

	case MC68901_REGISTER_IMRA:
		logerror("MC68901 Interrupt Mask Register A : %x\n", data);
		chip->imr = (data << 8) | (chip->imr & 0xff);
		chip->isr &= chip->imr;
		mc68901_check_interrupts(chip);
		break;

	case MC68901_REGISTER_IMRB:
		logerror("MC68901 Interrupt Mask Register B : %x\n", data);
		chip->imr = (chip->imr & 0xff00) | data;
		chip->isr &= chip->imr;
		mc68901_check_interrupts(chip);
		break;

	case MC68901_REGISTER_VR:
		logerror("MC68901 Interrupt Vector : %x\n", data & 0xf0);

		chip->vr = data & 0xf8;

		if (chip->vr & MC68901_VR_S)
		{
			logerror("MC68901 Software End-Of-Interrupt Mode\n");
		}
		else
		{
			logerror("MC68901 Automatic End-Of-Interrupt Mode\n");

			chip->isr = 0;
		}
		break;

	case MC68901_REGISTER_TACR:
		chip->tacr = data & 0x1f;

		switch (chip->tacr & 0x0f)
		{
		case MC68901_TCR_TIMER_STOPPED:
			logerror("MC68901 Timer A Stopped\n");
			timer_enable(chip->timer[MC68901_TIMER_A], 0);
			break;

		case MC68901_TCR_TIMER_DELAY_4:
		case MC68901_TCR_TIMER_DELAY_10:
		case MC68901_TCR_TIMER_DELAY_16:
		case MC68901_TCR_TIMER_DELAY_50:
		case MC68901_TCR_TIMER_DELAY_64:
		case MC68901_TCR_TIMER_DELAY_100:
		case MC68901_TCR_TIMER_DELAY_200:
			{
			int divisor = PRESCALER[chip->tacr & 0x07];
			logerror("MC68901 Timer A Delay Mode : %u Prescale\n", divisor);
			timer_adjust_periodic(chip->timer[MC68901_TIMER_A], ATTOTIME_IN_HZ(chip->intf->timer_clock / divisor), 0, ATTOTIME_IN_HZ(chip->intf->timer_clock / divisor));
			}
			break;

		case MC68901_TCR_TIMER_EVENT:
			logerror("MC68901 Timer A Event Count Mode\n");
			timer_enable(chip->timer[MC68901_TIMER_A], 0);
			break;

		case MC68901_TCR_TIMER_PULSE_4:
		case MC68901_TCR_TIMER_PULSE_10:
		case MC68901_TCR_TIMER_PULSE_16:
		case MC68901_TCR_TIMER_PULSE_50:
		case MC68901_TCR_TIMER_PULSE_64:
		case MC68901_TCR_TIMER_PULSE_100:
		case MC68901_TCR_TIMER_PULSE_200:
			{
			int divisor = PRESCALER[chip->tacr & 0x07];
			logerror("MC68901 Timer A Pulse Width Mode : %u Prescale\n", divisor);
			timer_adjust_periodic(chip->timer[MC68901_TIMER_A], attotime_never, 0, ATTOTIME_IN_HZ(chip->intf->timer_clock / divisor));
			timer_enable(chip->timer[MC68901_TIMER_A], 0);
			}
			break;
		}

		if (chip->tacr & MC68901_TCR_TIMER_RESET)
		{
			logerror("MC68901 Timer A Reset\n");

			chip->to[MC68901_TIMER_A] = 0;

			if (chip->intf->to_w)
			{
				chip->intf->to_w(chip, MC68901_TIMER_A, chip->to[MC68901_TIMER_A]);
			}
		}
		break;

	case MC68901_REGISTER_TBCR:
		chip->tbcr = data & 0x1f;

		switch (chip->tbcr & 0x0f)
		{
		case MC68901_TCR_TIMER_STOPPED:
			logerror("MC68901 Timer B Stopped\n");
			timer_enable(chip->timer[MC68901_TIMER_B], 0);
			break;

		case MC68901_TCR_TIMER_DELAY_4:
		case MC68901_TCR_TIMER_DELAY_10:
		case MC68901_TCR_TIMER_DELAY_16:
		case MC68901_TCR_TIMER_DELAY_50:
		case MC68901_TCR_TIMER_DELAY_64:
		case MC68901_TCR_TIMER_DELAY_100:
		case MC68901_TCR_TIMER_DELAY_200:
			{
			int divisor = PRESCALER[chip->tbcr & 0x07];
			logerror("MC68901 Timer B Delay Mode : %u Prescale\n", divisor);
			timer_adjust_periodic(chip->timer[MC68901_TIMER_B], ATTOTIME_IN_HZ(chip->intf->timer_clock / divisor), 0, ATTOTIME_IN_HZ(chip->intf->timer_clock / divisor));
			}
			break;

		case MC68901_TCR_TIMER_EVENT:
			logerror("MC68901 Timer B Event Count Mode\n");
			timer_enable(chip->timer[MC68901_TIMER_B], 0);
			break;

		case MC68901_TCR_TIMER_PULSE_4:
		case MC68901_TCR_TIMER_PULSE_10:
		case MC68901_TCR_TIMER_PULSE_16:
		case MC68901_TCR_TIMER_PULSE_50:
		case MC68901_TCR_TIMER_PULSE_64:
		case MC68901_TCR_TIMER_PULSE_100:
		case MC68901_TCR_TIMER_PULSE_200:
			{
			int divisor = PRESCALER[chip->tbcr & 0x07];
			logerror("MC68901 Timer B Pulse Width Mode : %u Prescale\n", DIVISOR);
			timer_adjust_periodic(chip->timer[MC68901_TIMER_B], attotime_never, 0, ATTOTIME_IN_HZ(chip->intf->timer_clock / divisor));
			timer_enable(chip->timer[MC68901_TIMER_B], 0);
			}
			break;
		}

		if (chip->tacr & MC68901_TCR_TIMER_RESET)
		{
			logerror("MC68901 Timer B Reset\n");

			chip->to[MC68901_TIMER_B] = 0;

			if (chip->intf->to_w)
			{
				chip->intf->to_w(chip, MC68901_TIMER_B, chip->to[MC68901_TIMER_B]);
			}
		}
		break;

	case MC68901_REGISTER_TCDCR:
		chip->tcdcr = data & 0x6f;

		switch (chip->tcdcr & 0x07)
		{
		case MC68901_TCR_TIMER_STOPPED:
			logerror("MC68901 Timer D Stopped\n");
			timer_enable(chip->timer[MC68901_TIMER_D], 0);
			break;

		case MC68901_TCR_TIMER_DELAY_4:
		case MC68901_TCR_TIMER_DELAY_10:
		case MC68901_TCR_TIMER_DELAY_16:
		case MC68901_TCR_TIMER_DELAY_50:
		case MC68901_TCR_TIMER_DELAY_64:
		case MC68901_TCR_TIMER_DELAY_100:
		case MC68901_TCR_TIMER_DELAY_200:
			{
			int divisor = PRESCALER[chip->tcdcr & 0x07];
			logerror("MC68901 Timer D Delay Mode : %u Prescale\n", divisor);
			timer_adjust_periodic(chip->timer[MC68901_TIMER_D], ATTOTIME_IN_HZ(chip->intf->timer_clock / divisor), 0, ATTOTIME_IN_HZ(chip->intf->timer_clock / divisor));
			}
			break;
		}

		switch ((chip->tcdcr >> 4) & 0x07)
		{
		case MC68901_TCR_TIMER_STOPPED:
			logerror("MC68901 Timer C Stopped\n");
			timer_enable(chip->timer[MC68901_TIMER_C], 0);
			break;

		case MC68901_TCR_TIMER_DELAY_4:
		case MC68901_TCR_TIMER_DELAY_10:
		case MC68901_TCR_TIMER_DELAY_16:
		case MC68901_TCR_TIMER_DELAY_50:
		case MC68901_TCR_TIMER_DELAY_64:
		case MC68901_TCR_TIMER_DELAY_100:
		case MC68901_TCR_TIMER_DELAY_200:
			{
			int divisor = PRESCALER[(chip->tcdcr >> 4) & 0x07];
			logerror("MC68901 Timer C Delay Mode : %u Prescale\n", divisor);
			timer_adjust_periodic(chip->timer[MC68901_TIMER_C], ATTOTIME_IN_HZ(chip->intf->timer_clock / divisor), 0, ATTOTIME_IN_HZ(chip->intf->timer_clock / divisor));
			}
			break;
		}
		break;

	case MC68901_REGISTER_TADR:
		logerror("MC68901 Timer A Data Register : %x\n", data);

		chip->tdr[MC68901_TIMER_A] = data;

		if (!timer_enabled(chip->timer[MC68901_TIMER_A]))
		{
			chip->tmc[MC68901_TIMER_A] = data;
		}
		break;

	case MC68901_REGISTER_TBDR:
		logerror("MC68901 Timer B Data Register : %x\n", data);

		chip->tdr[MC68901_TIMER_B] = data;

		if (!timer_enabled(chip->timer[MC68901_TIMER_B]))
		{
			chip->tmc[MC68901_TIMER_B] = data;
		}
		break;

	case MC68901_REGISTER_TCDR:
		logerror("MC68901 Timer C Data Register : %x\n", data);

		chip->tdr[MC68901_TIMER_C] = data;

		if (!timer_enabled(chip->timer[MC68901_TIMER_C]))
		{
			chip->tmc[MC68901_TIMER_C] = data;
		}
		break;

	case MC68901_REGISTER_TDDR:
		logerror("MC68901 Timer D Data Register : %x\n", data);

		chip->tdr[MC68901_TIMER_D] = data;

		if (!timer_enabled(chip->timer[MC68901_TIMER_D]))
		{
			chip->tmc[MC68901_TIMER_D] = data;
		}
		break;

	case MC68901_REGISTER_SCR:
		logerror("MC68901 Sync Character : %x\n", data);

		chip->scr = data;
		break;

	case MC68901_REGISTER_UCR:
		if (data & MC68901_UCR_PARITY_ENABLED)
		{
			if (data & MC68901_UCR_PARITY_EVEN)
				logerror("MC68901 Parity : Even\n");
			else
				logerror("MC68901 Parity : Odd\n");
		}
		else
		{
			logerror("MC68901 Parity : Disabled\n");
		}

		switch (data & 0x60)
		{
		case MC68901_UCR_WORD_LENGTH_8:
			chip->rxtx_word = 8;
			logerror("MC68901 Word Length : 8 bits\n");
			break;
		case MC68901_UCR_WORD_LENGTH_7:
			chip->rxtx_word = 7;
			logerror("MC68901 Word Length : 7 bits\n");
			break;
		case MC68901_UCR_WORD_LENGTH_6:
			chip->rxtx_word = 6;
			logerror("MC68901 Word Length : 6 bits\n");
			break;
		case MC68901_UCR_WORD_LENGTH_5:
			chip->rxtx_word = 5;
			logerror("MC68901 Word Length : 5 bits\n");
			break;
		}

		switch (data & 0x18)
		{
		case MC68901_UCR_START_STOP_0_0:
			chip->rxtx_start = 0;
			chip->rxtx_stop = 0;
			logerror("MC68901 Start Bits : 0, Stop Bits : 0, Format : synchronous\n");
			break;
		case MC68901_UCR_START_STOP_1_1:
			chip->rxtx_start = 1;
			chip->rxtx_stop = 1;
			logerror("MC68901 Start Bits : 1, Stop Bits : 1, Format : asynchronous\n");
			break;
		case MC68901_UCR_START_STOP_1_15:
			chip->rxtx_start = 1;
			chip->rxtx_stop = 1;
			logerror("MC68901 Start Bits : 1, Stop Bits : 1½, Format : asynchronous\n");
			break;
		case MC68901_UCR_START_STOP_1_2:
			chip->rxtx_start = 1;
			chip->rxtx_stop = 2;
			logerror("MC68901 Start Bits : 1, Stop Bits : 2, Format : asynchronous\n");
			break;
		}

		if (data & MC68901_UCR_CLOCK_DIVIDE_16)
			logerror("MC68901 Rx/Tx Clock Divisor : 16\n");
		else
			logerror("MC68901 Rx/Tx Clock Divisor : 1\n");

		chip->ucr = data;
		break;

	case MC68901_REGISTER_RSR:
		if ((data & MC68901_RSR_RCV_ENABLE) == 0)
		{
			logerror("MC68901 Receiver Disabled\n");
			chip->rsr = 0;
		}
		else
		{
			logerror("MC68901 Receiver Enabled\n");

			if (data & MC68901_RSR_SYNC_STRIP_ENABLE)
				logerror("MC68901 Sync Strip Enabled\n");
			else
				logerror("MC68901 Sync Strip Disabled\n");

			if (data & MC68901_RSR_FOUND_SEARCH)
				logerror("MC68901 Receiver Search Mode Enabled\n");

			chip->rsr = data & 0x0b;
		}
		break;

	case MC68901_REGISTER_TSR:
		if ((data & MC68901_TSR_XMIT_ENABLE) == 0)
		{
			logerror("MC68901 Transmitter Disabled\n");

			chip->tsr = data & 0x27;
		}
		else
		{
			logerror("MC68901 Transmitter Enabled\n");

			switch (data & 0x06)
			{
			case MC68901_TSR_OUTPUT_HI_Z:
				logerror("MC68901 Transmitter Disabled Output State : Hi-Z\n");
				break;
			case MC68901_TSR_OUTPUT_LOW:
				logerror("MC68901 Transmitter Disabled Output State : 0\n");
				break;
			case MC68901_TSR_OUTPUT_HIGH:
				logerror("MC68901 Transmitter Disabled Output State : 1\n");
				break;
			case MC68901_TSR_OUTPUT_LOOP:
				logerror("MC68901 Transmitter Disabled Output State : Loop\n");
				break;
			}

			if (data & MC68901_TSR_BREAK)
				logerror("MC68901 Transmitter Break Enabled\n");
			else
				logerror("MC68901 Transmitter Break Disabled\n");

			if (data & MC68901_TSR_AUTO_TURNAROUND)
				logerror("MC68901 Transmitter Auto Turnaround Enabled\n");
			else
				logerror("MC68901 Transmitter Auto Turnaround Disabled\n");

			chip->tsr = data & 0x2f;
			chip->tsr |= MC68901_TSR_BUFFER_EMPTY;  // x68000 expects the buffer to be empty, so this will do for now
		}
		break;

	case MC68901_REGISTER_UDR:
		logerror("MC68901 UDR %x\n", data);
		chip->udr = data;
		//chip->tsr &= ~MC68901_TSR_BUFFER_EMPTY;
		break;
	}
}

static void mc68901_timer_count(mc68901_t *chip, int index)
{
	if (chip->tmc[index] == 0x01)
	{
		chip->to[index] = chip->to[index] ? 0 : 1;

		if (chip->to[index])
		{
			if (chip->intf->rx_clock == MC68901_LOOPBACK_TIMER[index])
			{
				mc68901_serial_rx(chip);
			}

			if (chip->intf->tx_clock == MC68901_LOOPBACK_TIMER[index])
			{
				mc68901_serial_tx(chip);
			}
		}

		if (chip->intf->to_w)
		{
			chip->intf->to_w(chip, index, chip->to[index]);
		}

		if (chip->ier & MC68901_INT_MASK_TIMER[index])
		{
			mc68901_interrupt(chip, MC68901_INT_MASK_TIMER[index]);
		}

		chip->tmc[index] = chip->tdr[index];
	}
	else
	{
		chip->tmc[index]--;
	}
}

static void mc68901_ti_w(mc68901_t *chip, int index, int value)
{
	int bit = MC68901_GPIO_TIMER[index];
	int aer = BIT(chip->aer, bit);
	int cr = index ? chip->tbcr : chip->tacr;

	switch (cr & 0x0f)
	{
	case MC68901_TCR_TIMER_EVENT:
		if (((chip->ti[index] ^ aer) == 1) && ((value ^ aer) == 0))
		{
			mc68901_timer_count(chip, index);
		}

		chip->ti[index] = value;
		break;

	case MC68901_TCR_TIMER_PULSE_4:
	case MC68901_TCR_TIMER_PULSE_10:
	case MC68901_TCR_TIMER_PULSE_16:
	case MC68901_TCR_TIMER_PULSE_50:
	case MC68901_TCR_TIMER_PULSE_64:
	case MC68901_TCR_TIMER_PULSE_100:
	case MC68901_TCR_TIMER_PULSE_200:
		timer_enable(chip->timer[index], (value == aer));

		if (((chip->ti[index] ^ aer) == 0) && ((value ^ aer) == 1))
		{
			if (chip->ier & MC68901_INT_MASK_GPIO[bit])
			{
				mc68901_interrupt(chip, MC68901_INT_MASK_GPIO[bit]);
			}
		}

		chip->ti[index] = value;
		break;
	}
}

static TIMER_CALLBACK( timer_a )
{
	mc68901_timer_count(ptr, MC68901_TIMER_A);
}

static TIMER_CALLBACK( timer_b )
{
	mc68901_timer_count(ptr, MC68901_TIMER_B);
}

static TIMER_CALLBACK( timer_c )
{
	mc68901_timer_count(ptr, MC68901_TIMER_C);
}

static TIMER_CALLBACK( timer_d )
{
	mc68901_timer_count(ptr, MC68901_TIMER_D);
}

/* External Interface */

void mc68901_tai_w(mc68901_t *chip, int value)
{
	assert(chip != NULL);

	mc68901_ti_w(chip, MC68901_TIMER_A, value);
}

void mc68901_tbi_w(mc68901_t *chip, int value)
{
	assert(chip != NULL);

	mc68901_ti_w(chip, MC68901_TIMER_B, value);
}

int mc68901_get_vector(mc68901_t *chip)
{
	int ch;

	assert(chip != NULL);

	for (ch = 15; ch >= 0; ch--)
	{
		if (BIT(chip->imr, ch) && BIT(chip->ipr, ch))
		{
			if (chip->vr & MC68901_VR_S)
			{
				chip->isr |= (1 << ch);
			}

			chip->ipr &= ~(1 << ch);
			mc68901_check_interrupts(chip);

			return (chip->vr & 0xf0) | ch;
		}
	}

	return MC68000_INT_ACK_SPURIOUS;
}

/* Device Interface */

static DEVICE_START( mc68901 )
{
	mc68901_t *mc68901;
	char unique_tag[30];

	assert(machine != NULL);
	assert(tag != NULL);
	assert(strlen(tag) < 20);

	// allocate the object that holds the state

	mc68901 = auto_malloc(sizeof(*mc68901));
	memset(mc68901, 0, sizeof(*mc68901));

	//mc68901->device_type = device_type;
	mc68901->intf = static_config;

	// initial settings

	mc68901->tsr = MC68901_TSR_BUFFER_EMPTY;

	// counter timers

	mc68901->timer[MC68901_TIMER_A] = timer_alloc(timer_a, mc68901);
	mc68901->timer[MC68901_TIMER_B] = timer_alloc(timer_b, mc68901);
	mc68901->timer[MC68901_TIMER_C] = timer_alloc(timer_c, mc68901);
	mc68901->timer[MC68901_TIMER_D] = timer_alloc(timer_d, mc68901);

	// serial receive timer

	if (mc68901->intf->rx_clock > 0)
	{
		timer_pulse(ATTOTIME_IN_HZ(mc68901->intf->rx_clock), mc68901, 0, rx_tick);
	}

	// serial transmit timer

	if (mc68901->intf->tx_clock > 0)
	{
		timer_pulse(ATTOTIME_IN_HZ(mc68901->intf->tx_clock), mc68901, 0, tx_tick);
	}

	// general purpose I/O poll timer

	if (mc68901->intf->gpio_r)
	{
		timer_pulse(ATTOTIME_IN_HZ(mc68901->intf->chip_clock / 4), mc68901, 0, mc68901_gpio_poll_tick);
	}

	// register save state support

	state_save_combine_module_and_tag(unique_tag, "MC68901", tag);

	state_save_register_item(unique_tag, 0, mc68901->gpip);
	state_save_register_item(unique_tag, 0, mc68901->aer);
	state_save_register_item(unique_tag, 0, mc68901->ddr);
	state_save_register_item(unique_tag, 0, mc68901->ier);
	state_save_register_item(unique_tag, 0, mc68901->ipr);
	state_save_register_item(unique_tag, 0, mc68901->isr);
	state_save_register_item(unique_tag, 0, mc68901->imr);
	state_save_register_item(unique_tag, 0, mc68901->vr);
	state_save_register_item(unique_tag, 0, mc68901->tacr);
	state_save_register_item(unique_tag, 0, mc68901->tbcr);
	state_save_register_item(unique_tag, 0, mc68901->tcdcr);
	state_save_register_item_array(unique_tag, 0, mc68901->tdr);
	state_save_register_item_array(unique_tag, 0, mc68901->tmc);
	state_save_register_item_array(unique_tag, 0, mc68901->to);
	state_save_register_item_array(unique_tag, 0, mc68901->ti);
	state_save_register_item(unique_tag, 0, mc68901->scr);
	state_save_register_item(unique_tag, 0, mc68901->ucr);
	state_save_register_item(unique_tag, 0, mc68901->rsr);
	state_save_register_item(unique_tag, 0, mc68901->tsr);
	state_save_register_item(unique_tag, 0, mc68901->udr);
	state_save_register_item(unique_tag, 0, mc68901->rx_bits);
	state_save_register_item(unique_tag, 0, mc68901->tx_bits);
	state_save_register_item(unique_tag, 0, mc68901->rx_parity);
	state_save_register_item(unique_tag, 0, mc68901->tx_parity);
	state_save_register_item(unique_tag, 0, mc68901->rx_state);
	state_save_register_item(unique_tag, 0, mc68901->tx_state);
	state_save_register_item(unique_tag, 0, mc68901->rx_buffer);
	state_save_register_item(unique_tag, 0, mc68901->tx_buffer);
	state_save_register_item(unique_tag, 0, mc68901->xmit_state);
	state_save_register_item(unique_tag, 0, mc68901->rxtx_word);
	state_save_register_item(unique_tag, 0, mc68901->rxtx_start);
	state_save_register_item(unique_tag, 0, mc68901->rxtx_stop);
	state_save_register_item(unique_tag, 0, mc68901->rsr_read);
	state_save_register_item(unique_tag, 0, mc68901->next_rsr);
	state_save_register_item(unique_tag, 0, mc68901->irqlevel);

	return mc68901;
}

static DEVICE_RESET( mc68901 )
{
	mc68901_t *mc68901 = token;

	mc68901_register_w(mc68901, MC68901_REGISTER_GPIP, 0);
	mc68901_register_w(mc68901, MC68901_REGISTER_AER, 0);
	mc68901_register_w(mc68901, MC68901_REGISTER_DDR, 0);
	mc68901_register_w(mc68901, MC68901_REGISTER_IERA, 0);
	mc68901_register_w(mc68901, MC68901_REGISTER_IERB, 0);
	mc68901_register_w(mc68901, MC68901_REGISTER_IPRA, 0);
	mc68901_register_w(mc68901, MC68901_REGISTER_IPRB, 0);
	mc68901_register_w(mc68901, MC68901_REGISTER_ISRA, 0);
	mc68901_register_w(mc68901, MC68901_REGISTER_ISRB, 0);
	mc68901_register_w(mc68901, MC68901_REGISTER_IMRA, 0);
	mc68901_register_w(mc68901, MC68901_REGISTER_IMRB, 0);
	mc68901_register_w(mc68901, MC68901_REGISTER_VR, 0);
	mc68901_register_w(mc68901, MC68901_REGISTER_TACR, 0);
	mc68901_register_w(mc68901, MC68901_REGISTER_TBCR, 0);
	mc68901_register_w(mc68901, MC68901_REGISTER_TCDCR, 0);
	mc68901_register_w(mc68901, MC68901_REGISTER_SCR, 0);
	mc68901_register_w(mc68901, MC68901_REGISTER_UCR, 0);
	mc68901_register_w(mc68901, MC68901_REGISTER_RSR, 0);
}

static DEVICE_SET_INFO( mc68901 )
{
	switch (state)
	{
		/* no parameters to set */
	}
}

DEVICE_GET_INFO( mc68901 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_SET_INFO:						info->set_info = DEVICE_SET_INFO_NAME(mc68901); break;
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(mc68901);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(mc68901);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							info->s = "Motorola 68901";					break;
		case DEVINFO_STR_FAMILY:						info->s = "MC68901 MFP";					break;
		case DEVINFO_STR_VERSION:						info->s = "1.0";							break;
		case DEVINFO_STR_SOURCE_FILE:					info->s = __FILE__;							break;
		case DEVINFO_STR_CREDITS:						info->s = "Copyright the MESS Team"; 		break;
	}
}
