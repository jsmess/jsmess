/*

	TODO:

	- daisy chaining
	- correct irq_timer period
	
	- divide serial clock by 16
	- synchronous mode
	- 1.5/2 stop bits
	- interrupt on receiver break end
	- interrupt on character boundaries during break transmission
	- loopback mode
	- transmit parity bit

*/

#include "68901mfp.h"

typedef struct
{
	const mfp68901_interface *intf;

	UINT8 gpip;
	UINT8 aer;
	UINT8 ddr;

	UINT16 ier, ipr, isr, imr;
	UINT8 vr;
	mame_timer *irq_timer;

	UINT8 tacr, tbcr, tcdcr;
	UINT8 tdr[MFP68901_MAX_TIMERS];
	UINT8 tmc[MFP68901_MAX_TIMERS];
	int to[MFP68901_MAX_TIMERS];
	int ti[MFP68901_MAX_TIMERS];
	mame_timer *timer[MFP68901_MAX_TIMERS];

	UINT8 ucr;
	UINT8 udr;
	UINT8 scr;
	UINT8 tsr, rsr, next_rsr, rsr_read;
	UINT8 rx_buffer, tx_buffer;
	int	rx_bits, tx_bits;
	int	rx_parity, tx_parity;
	int	rx_state, tx_state, xmit_state;
	int rxtx_word, rxtx_start, rxtx_stop;
	mame_timer *rx_timer, *tx_timer;
} mfp_68901;

static mfp_68901 mfp[MAX_MFP];

static const int GPIO_MASK[] =
{ 
	MFP68901_IR_GPIP_0, MFP68901_IR_GPIP_1, MFP68901_IR_GPIP_2, MFP68901_IR_GPIP_3, 
	MFP68901_IR_GPIP_4, MFP68901_IR_GPIP_5, MFP68901_IR_GPIP_6, MFP68901_IR_GPIP_7 
};

static const int PRESCALER[] = { 0, 4, 10, 16, 50, 64, 100, 200 };

static void mfp68901_poll_gpio(int which)
{
	mfp_68901 *mfp_p = &mfp[which];

	UINT8 gpio = 0, gpold, gpnew;
	int bit;

	if (mfp_p->intf->gpio_r)
	{
		gpio = mfp_p->intf->gpio_r(0); // read the state of GPIO the pins from the driver
	}

	gpold = (mfp_p->gpip & ~mfp_p->ddr) ^ mfp_p->aer;
	gpnew = (gpio & ~mfp_p->ddr) ^ mfp_p->aer;

	for (bit = 0; bit < 8; bit++) // loop thru each bit from 0 to 7
	{
		if ((BIT(gpold, bit) == 1) && (BIT(gpnew, bit) == 0)) // if transition from 1 to 0 is detected...
		{
			if (mfp_p->ier & GPIO_MASK[bit]) // AND interrupt enabled bit is set...
			{
				mfp_p->ipr |= GPIO_MASK[bit]; // set interrupt pending bit
			}
		}
	}

	mfp_p->gpip = (gpio & ~mfp_p->ddr) | (mfp_p->gpip & mfp_p->ddr);
}

static void mfp68901_irq_ack(int which)
{
	mfp_68901 *mfp_p = &mfp[which];

	int ch;

	if (!mfp_p->isr) // if no interrupts are currently in service...
	{
		for (ch = 15; ch >= 0; ch--) // loop thru each channel from 15 to 0
		{
			if (BIT(mfp_p->imr, ch)) // if interrupt mask bit is set...
			{
				if (BIT(mfp_p->ipr, ch)) // AND interrupt pending bit is set...
				{
					logerror("MFP68901 #%u Interrupt Fired on Channel %u\n", which, ch);

					mfp_p->ipr &= ~(1 << ch); // clear interrupt pending bit

					if (mfp_p->vr & MFP68901_VR_S) // if software end-of-interrupt mode is enabled...
					{
						mfp_p->isr |= (1 << ch); // set interrupt in service bit (bit will be cleared later by the interrupt service routine)
					}

					mfp_p->intf->irq_callback(which, HOLD_LINE, (mfp_p->vr & 0xf0) | ch); // fire interrupt callback

					return;
				}
			}
		}
	}
}

static TIMER_CALLBACK( mfp68901_tick )
{
	mfp68901_poll_gpio(param);
	mfp68901_irq_ack(param);
}

/* USART */

static void mfp68901_rx_buffer_full(int which)
{
	mfp_68901 *mfp_p = &mfp[which];

	if (mfp_p->ier & MFP68901_IR_RCV_BUFFER_FULL)
	{
		mfp_p->ipr |= MFP68901_IR_RCV_BUFFER_FULL;
	}
}

static void mfp68901_rx_error(int which)
{
	mfp_68901 *mfp_p = &mfp[which];

	if (mfp_p->ier & MFP68901_IR_RCV_ERROR)
	{
		mfp_p->ipr |= MFP68901_IR_RCV_ERROR;
	}
	else
	{
		mfp68901_rx_buffer_full(which);
	}
}

static void mfp68901_tx_buffer_empty(int which)
{
	mfp_68901 *mfp_p = &mfp[which];

	if (mfp_p->ier & MFP68901_IR_XMIT_BUFFER_EMPTY)
	{
		mfp_p->ipr |= MFP68901_IR_XMIT_BUFFER_EMPTY;
	}
}

static void mfp68901_tx_error(int which)
{
	mfp_68901 *mfp_p = &mfp[which];

	if (mfp_p->ier & MFP68901_IR_XMIT_ERROR)
	{
		mfp_p->ipr |= MFP68901_IR_XMIT_ERROR;
	}
	else
	{
		mfp68901_tx_buffer_empty(which);
	}
}

static int mfp68901_parity(UINT8 b)
{
	b ^= b >> 4;
	b ^= b >> 2;
	b ^= b >> 1;
	return b & 1;
}

static TIMER_CALLBACK( rx_tick )
{
	mfp_68901 *mfp_p = &mfp[param];

	if (mfp_p->rsr & MFP68901_RSR_RCV_ENABLE)
	{
		switch (mfp_p->rx_state)
		{
		case MFP68901_SERIAL_START:
			if (mfp_p->intf->rx_pin == 0)
			{
				mfp_p->rsr |= MFP68901_RSR_CHAR_IN_PROGRESS;
				mfp_p->rx_bits = 0;
				mfp_p->rx_buffer = 0;
				mfp_p->rx_state = MFP68901_SERIAL_DATA;
				mfp_p->next_rsr = MFP68901_RSR_BREAK;
			}
			break;

		case MFP68901_SERIAL_DATA:
			if ((mfp_p->next_rsr & MFP68901_RSR_BREAK) && (*mfp_p->intf->rx_pin == 1) && mfp_p->rsr_read)
			{
				mfp_p->next_rsr &= ~MFP68901_RSR_BREAK;
			}

			mfp_p->rx_buffer >>= 1;
			mfp_p->rx_buffer |= (*mfp_p->intf->rx_pin << 7);
			mfp_p->rx_bits++;

			if (mfp_p->rx_bits == mfp_p->rxtx_word)
			{
				if (mfp_p->rxtx_word < 8)
				{
					mfp_p->rx_buffer >>= (8 - mfp_p->rxtx_word);
				}

				mfp_p->rsr &= ~MFP68901_RSR_CHAR_IN_PROGRESS;

				if (mfp_p->ucr & MFP68901_UCR_PARITY_ENABLED)
				{
					mfp_p->rx_state = MFP68901_SERIAL_PARITY;
				}
				else
				{
					mfp_p->rx_state = MFP68901_SERIAL_STOP;
				}
			}
			break;

		case MFP68901_SERIAL_PARITY:
			mfp_p->rx_parity = *mfp_p->intf->rx_pin;

			if (mfp_p->rx_parity != (mfp68901_parity(mfp_p->rx_buffer) ^ ((mfp_p->ucr & MFP68901_UCR_PARITY_EVEN) >> 1)))
			{
				mfp_p->next_rsr |= MFP68901_RSR_PARITY_ERROR;
			}

			mfp_p->rx_state = MFP68901_SERIAL_STOP;
			break;

		case MFP68901_SERIAL_STOP:
			if (*mfp_p->intf->rx_pin == 1)
			{
				if (!((mfp_p->rsr & MFP68901_RSR_SYNC_STRIP_ENABLE) && (mfp_p->rx_buffer == mfp_p->scr)))
				{
					if (!(mfp_p->rsr & MFP68901_RSR_OVERRUN_ERROR))
					{
						if (mfp_p->rsr & MFP68901_RSR_BUFFER_FULL)
						{
							// incoming word received but last word in receive buffer has not been read
							mfp_p->next_rsr |= MFP68901_RSR_OVERRUN_ERROR;
						}
						else
						{
							// incoming word received and receive buffer is empty
							mfp_p->rsr |= MFP68901_RSR_BUFFER_FULL;
							mfp_p->udr = mfp_p->rx_buffer;
							mfp68901_rx_buffer_full(param);
						}
					}
				}
			}
			else
			{
				if (mfp_p->rx_buffer)
				{
					// non-zero data word not followed by a stop bit
					mfp_p->next_rsr |= MFP68901_RSR_FRAME_ERROR;
				}
			}

			mfp_p->rx_state = MFP68901_SERIAL_START;
			break;
		}
	}
}

#ifdef UNUSED_FUNCTION
static void mfp68901_check_xe(int which)
{
	mfp_68901 *mfp_p = &mfp[which];

	if (!(mfp_p->tsr & MFP68901_TSR_XMIT_ENABLE))
	{
		if (mfp_p->tsr & MFP68901_TSR_AUTO_TURNAROUND)
		{
			mfp_p->tsr |= MFP68901_TSR_XMIT_ENABLE;
		}
		else
		{
			mfp_p->xmit_state = MFP68901_XMIT_OFF;
			mfp_p->tsr |= MFP68901_TSR_END_OF_XMIT;
			mfp68901_tx_error(which);
		}
	}
}
#endif

static void mpf68901_transmit_disabled(int which)
{
	mfp_68901 *mfp_p = &mfp[which];

	switch (mfp_p->tsr & MFP68901_TSR_OUTPUT_MASK)
	{
	case MFP68901_TSR_OUTPUT_HI_Z:
		// indeterminate
	case MFP68901_TSR_OUTPUT_LOW:
		*mfp_p->intf->tx_pin = 0;
		break;

	case MFP68901_TSR_OUTPUT_HIGH:
	case MFP68901_TSR_OUTPUT_LOOP:
		*mfp_p->intf->tx_pin = 1;
		break;
	}
}

static void mpf68901_transmit(int which)
{
	mfp_68901 *mfp_p = &mfp[which];

	switch (mfp_p->tx_state)
	{
	case MFP68901_SERIAL_START:
		if (mfp_p->tsr & MFP68901_TSR_UNDERRUN_ERROR)
		{
			if (mfp_p->tsr & MFP68901_TSR_XMIT_ENABLE)
			{
				*mfp_p->intf->tx_pin = 1;
			}
			else
			{
				mpf68901_transmit_disabled(which);
			}
		}
		else
		{
			if (mfp_p->tsr & MFP68901_TSR_BUFFER_EMPTY)
			{
				mfp_p->tsr |= MFP68901_TSR_UNDERRUN_ERROR;

				if (mfp_p->tsr & MFP68901_TSR_XMIT_ENABLE)
				{
					*mfp_p->intf->tx_pin = 1;
				}
				else
				{
					mpf68901_transmit_disabled(which);
				}
			}
			else
			{
				*mfp_p->intf->tx_pin = 0;
				mfp_p->tx_buffer = mfp_p->udr;
				mfp_p->tx_bits = 0;
				mfp_p->tx_state = MFP68901_SERIAL_DATA;
				mfp_p->tsr |= MFP68901_TSR_BUFFER_EMPTY;
				mfp68901_tx_buffer_empty(which);
			}
		}
		break;

	case MFP68901_SERIAL_DATA:
		*mfp_p->intf->tx_pin = mfp_p->tx_buffer & 0x01;
		mfp_p->tx_buffer >>= 1;
		mfp_p->tx_bits++;

		if (mfp_p->tx_bits == mfp_p->rxtx_word)
		{
			if (mfp_p->ucr & MFP68901_UCR_PARITY_ENABLED)
			{
				mfp_p->tx_state = MFP68901_SERIAL_PARITY;
			}
			else
			{
				mfp_p->tx_state = MFP68901_SERIAL_STOP;
			}
		}
		break;

	case MFP68901_SERIAL_PARITY:
		*mfp_p->intf->tx_pin = 0;
		mfp_p->tx_state = MFP68901_SERIAL_STOP;
		break;

	case MFP68901_SERIAL_STOP:
		*mfp_p->intf->tx_pin = 1;

		if (mfp_p->tsr & MFP68901_TSR_XMIT_ENABLE)
		{
			mfp_p->tx_state = MFP68901_SERIAL_START;
		}
		else
		{
			if (mfp_p->tsr & MFP68901_TSR_AUTO_TURNAROUND)
			{
				mfp_p->tsr |= MFP68901_TSR_XMIT_ENABLE;
				mfp_p->tx_state = MFP68901_SERIAL_START;
			}
			else
			{
				mfp_p->xmit_state = MFP68901_XMIT_OFF;
				mfp_p->tsr |= MFP68901_TSR_END_OF_XMIT;
				mfp68901_tx_error(which);
			}
		}
		break;
	}
}

static TIMER_CALLBACK( tx_tick )
{
	mfp_68901 *mfp_p = &mfp[param];

	switch (mfp_p->xmit_state)
	{
	case MFP68901_XMIT_OFF:
		mpf68901_transmit_disabled(param);
		break;

	case MFP68901_XMIT_STARTING:
		if (mfp_p->tsr & MFP68901_TSR_XMIT_ENABLE)
		{
			*mfp_p->intf->tx_pin = 1;
			mfp_p->xmit_state = MFP68901_XMIT_ON;
		}
		else
		{
			mfp_p->xmit_state = MFP68901_XMIT_OFF;
			mfp_p->tsr |= MFP68901_TSR_END_OF_XMIT;
		}
		break;

	case MFP68901_XMIT_BREAK:
		if (mfp_p->tsr & MFP68901_TSR_XMIT_ENABLE)
		{
			if (mfp_p->tsr & MFP68901_TSR_BREAK)
			{
				*mfp_p->intf->tx_pin = 1;
			}
			else
			{
				mfp_p->xmit_state = MFP68901_XMIT_ON;
			}
		}
		else
		{
			mfp_p->xmit_state = MFP68901_XMIT_OFF;
			mfp_p->tsr |= MFP68901_TSR_END_OF_XMIT;
		}
		break;

	case MFP68901_XMIT_ON:
		mpf68901_transmit(param);
		break;
	}
}

static UINT8 mfp68901_register_r(int which, int reg)
{
	mfp_68901 *mfp_p = &mfp[which];

	switch (reg)
	{
	case MFP68901_REGISTER_GPIP:  return mfp_p->gpip;
	case MFP68901_REGISTER_AER:   return mfp_p->aer;
	case MFP68901_REGISTER_DDR:   return mfp_p->ddr;

	case MFP68901_REGISTER_IERA:  return mfp_p->ier >> 8;
	case MFP68901_REGISTER_IERB:  return mfp_p->ier & 0xff;
	case MFP68901_REGISTER_IPRA:  return mfp_p->ipr >> 8;
	case MFP68901_REGISTER_IPRB:  return mfp_p->ipr & 0xff;
	case MFP68901_REGISTER_ISRA:  return mfp_p->isr >> 8;
	case MFP68901_REGISTER_ISRB:  return mfp_p->isr & 0xff;
	case MFP68901_REGISTER_IMRA:  return mfp_p->imr >> 8;
	case MFP68901_REGISTER_IMRB:  return mfp_p->imr & 0xff;
	case MFP68901_REGISTER_VR:    return mfp_p->vr;

	case MFP68901_REGISTER_TACR:  return mfp_p->tacr;
	case MFP68901_REGISTER_TBCR:  return mfp_p->tbcr;
	case MFP68901_REGISTER_TCDCR: return mfp_p->tcdcr;
	case MFP68901_REGISTER_TADR:  return mfp_p->tmc[MFP68901_TIMER_A];
	case MFP68901_REGISTER_TBDR:  return mfp_p->tmc[MFP68901_TIMER_B];
	case MFP68901_REGISTER_TCDR:  return mfp_p->tmc[MFP68901_TIMER_C];
	case MFP68901_REGISTER_TDDR:  return mfp_p->tmc[MFP68901_TIMER_D];

	case MFP68901_REGISTER_SCR:   return mfp_p->scr;
	case MFP68901_REGISTER_UCR:   return mfp_p->ucr;
	case MFP68901_REGISTER_RSR:
		mfp_p->rsr_read = 1;
		return mfp_p->rsr;

	case MFP68901_REGISTER_TSR:
		{
			// clear UE bit (in reality, this won't be cleared until one full clock cycle of the transmitter has passed since the bit was set)

			UINT8 tsr = mfp_p->tsr;
			mfp_p->tsr &= 0xbf;

			return tsr;
		}

	case MFP68901_REGISTER_UDR:
		// load RSR with latched value

		mfp_p->rsr = (mfp_p->next_rsr & 0x7c) | (mfp_p->rsr & 0x03);
		mfp_p->next_rsr = 0;

		// signal receiver error interrupt

		if (mfp_p->rsr & 0x78)
		{
			mfp68901_rx_error(which);
		}

		return mfp_p->udr;

	default:					  return 0;
	}
}

#define DIVISOR PRESCALER[data & 0x07]

static void mfp68901_register_w(int which, int reg, UINT8 data)
{
	mfp_68901 *mfp_p = &mfp[which];

	switch (reg)
	{
	case MFP68901_REGISTER_GPIP:
		logerror("MFP68901 #%u General Purpose I/O : %x\n", which, data);
		mfp_p->gpip = data & mfp_p->ddr;

		if (mfp_p->intf->gpio_w)
		{
			mfp_p->intf->gpio_w(0, mfp_p->gpip);
		}
		break;

	case MFP68901_REGISTER_AER:
		logerror("MFP68901 #%u Active Edge Register : %x\n", which, data);
		mfp_p->aer = data;
		// check transition and trigger interrupt if necessary
		break;

	case MFP68901_REGISTER_DDR:
		logerror("MFP68901 #%u Data Direction Register : %x\n", which, data);
		mfp_p->ddr = data;
		break;

	case MFP68901_REGISTER_IERA:
		logerror("MFP68901 #%u Interrupt Enable Register A : %x\n", which, data);
		mfp_p->ier = (data << 8) | (mfp_p->ier & 0xff);
		mfp_p->ipr &= mfp_p->ier;
		break;

	case MFP68901_REGISTER_IERB:
		logerror("MFP68901 #%u Interrupt Enable Register B : %x\n", which, data);
		mfp_p->ier = (mfp_p->ier & 0xff00) | data;
		mfp_p->ipr &= mfp_p->ier;
		break;

	case MFP68901_REGISTER_IPRA:
		logerror("MFP68901 #%u Interrupt Pending Register A : %x\n", which, data);
		mfp_p->ier &= (data << 8) | (mfp_p->ier & 0xff);
		break;

	case MFP68901_REGISTER_IPRB:
		logerror("MFP68901 #%u Interrupt Pending Register B : %x\n", which, data);
		mfp_p->ipr &= (mfp_p->ipr & 0xff00) | data;
		break;

	case MFP68901_REGISTER_ISRA:
		logerror("MFP68901 #%u Interrupt In-Service Register A : %x\n", which, data);
		mfp_p->isr = (data << 8) | (mfp_p->isr & 0xff);
		break;

	case MFP68901_REGISTER_ISRB:
		logerror("MFP68901 #%u Interrupt In-Service Register B : %x\n", which, data);
		mfp_p->isr = (mfp_p->isr & 0xff00) | data;
		break;

	case MFP68901_REGISTER_IMRA:
		logerror("MFP68901 #%u Interrupt Mask Register A : %x\n", which, data);
		mfp_p->imr = (data << 8) | (mfp_p->imr & 0xff);
		break;

	case MFP68901_REGISTER_IMRB:
		logerror("MFP68901 #%u Interrupt Mask Register B : %x\n", which, data);
		mfp_p->imr = (mfp_p->imr & 0xff00) | data;
		break;

	case MFP68901_REGISTER_VR:
		logerror("MFP68901 #%u Interrupt Vector : %x\n", which, data & 0xf0);

		mfp_p->vr = data & 0xf8;

		if ((mfp_p->vr & MFP68901_VR_S) == 0)
		{
			logerror("MFP68901 #%u Automatic End-Of-Interrupt Mode\n", which);

			mfp_p->isr = 0;
		}
		else
		{
			logerror("MFP68901 #%u Software End-Of-Interrupt Mode\n", which);
		}
		break;

	case MFP68901_REGISTER_TACR:
		mfp_p->tacr = data & 0x1f;

		switch (mfp_p->tacr & 0x0f)
		{
		case MFP68901_TCR_TIMER_STOPPED:
			logerror("MFP68901 #%u Timer A Stopped\n", which);
			mame_timer_enable(mfp_p->timer[MFP68901_TIMER_A], 0);
			break;

		case MFP68901_TCR_TIMER_DELAY_4:
		case MFP68901_TCR_TIMER_DELAY_10:
		case MFP68901_TCR_TIMER_DELAY_16:
		case MFP68901_TCR_TIMER_DELAY_50:
		case MFP68901_TCR_TIMER_DELAY_64:
		case MFP68901_TCR_TIMER_DELAY_100:
		case MFP68901_TCR_TIMER_DELAY_200:
			{
			int divisor = PRESCALER[mfp_p->tacr & 0x07];
			logerror("MFP68901 #%u Timer A Delay Mode : %u Prescale\n", which, divisor);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_A], MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / divisor), which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / divisor));
			}

		case MFP68901_TCR_TIMER_EVENT:
			logerror("MFP68901 #%u Timer A Event Count Mode\n", which);
			mame_timer_enable(mfp_p->timer[MFP68901_TIMER_A], 0);
			break;

		case MFP68901_TCR_TIMER_PULSE_4:
		case MFP68901_TCR_TIMER_PULSE_10:
		case MFP68901_TCR_TIMER_PULSE_16:
		case MFP68901_TCR_TIMER_PULSE_50:
		case MFP68901_TCR_TIMER_PULSE_64:
		case MFP68901_TCR_TIMER_PULSE_100:
		case MFP68901_TCR_TIMER_PULSE_200:
			{
			int divisor = PRESCALER[mfp_p->tacr & 0x07];
			logerror("MFP68901 #%u Timer A Pulse Width Mode : %u Prescale\n", which, divisor);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_A], time_never, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / divisor));
			mame_timer_enable(mfp_p->timer[MFP68901_TIMER_A], 0);
			}
			break;
		}

		if (mfp_p->tacr & MFP68901_TCR_TIMER_RESET)
		{
			logerror("MFP68901 #%u Timer A Reset\n", which);

			mfp_p->to[MFP68901_TIMER_A] = 0;

			if (mfp_p->intf->to_w)
			{
				mfp_p->intf->to_w(which, MFP68901_TIMER_A, mfp_p->to[MFP68901_TIMER_A]);
			}
		}
		break;

	case MFP68901_REGISTER_TBCR:
		mfp_p->tbcr = data & 0x1f;

		switch (mfp_p->tbcr & 0x0f)
		{
		case MFP68901_TCR_TIMER_STOPPED:
			logerror("MFP68901 #%u Timer B Stopped\n", which);
			mame_timer_enable(mfp_p->timer[MFP68901_TIMER_B], 0);
			break;

		case MFP68901_TCR_TIMER_DELAY_4:
		case MFP68901_TCR_TIMER_DELAY_10:
		case MFP68901_TCR_TIMER_DELAY_16:
		case MFP68901_TCR_TIMER_DELAY_50:
		case MFP68901_TCR_TIMER_DELAY_64:
		case MFP68901_TCR_TIMER_DELAY_100:
		case MFP68901_TCR_TIMER_DELAY_200:
			{
			int divisor = PRESCALER[mfp_p->tbcr & 0x07];
			logerror("MFP68901 #%u Timer B Delay Mode : %u Prescale\n", which, divisor);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_B], MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / divisor), which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / divisor));
			}
			break;

		case MFP68901_TCR_TIMER_EVENT:
			logerror("MFP68901 #%u Timer B Event Count Mode\n", which);
			mame_timer_enable(mfp_p->timer[MFP68901_TIMER_B], 0);
			break;

		case MFP68901_TCR_TIMER_PULSE_4:
		case MFP68901_TCR_TIMER_PULSE_10:
		case MFP68901_TCR_TIMER_PULSE_16:
		case MFP68901_TCR_TIMER_PULSE_50:
		case MFP68901_TCR_TIMER_PULSE_64:
		case MFP68901_TCR_TIMER_PULSE_100:
		case MFP68901_TCR_TIMER_PULSE_200:
			{
			int divisor = PRESCALER[mfp_p->tbcr & 0x07];
			logerror("MFP68901 #%u Timer B Pulse Width Mode : %u Prescale\n", which, DIVISOR);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_B], time_never, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / divisor));
			mame_timer_enable(mfp_p->timer[MFP68901_TIMER_B], 0);
			}
			break;
		}

		if (mfp_p->tacr & MFP68901_TCR_TIMER_RESET)
		{
			logerror("MFP68901 #%u Timer B Reset\n", which);

			mfp_p->to[MFP68901_TIMER_B] = 0;

			if (mfp_p->intf->to_w)
			{
				mfp_p->intf->to_w(which, MFP68901_TIMER_B, mfp_p->to[MFP68901_TIMER_B]);
			}
		}
		break;

	case MFP68901_REGISTER_TCDCR: 
		mfp_p->tcdcr = data & 0x6f;

		switch (mfp_p->tcdcr & 0x07)
		{
		case MFP68901_TCR_TIMER_STOPPED:
			logerror("MFP68901 #%u Timer D Stopped\n", which);
			mame_timer_enable(mfp_p->timer[MFP68901_TIMER_D], 0);
			break;

		case MFP68901_TCR_TIMER_DELAY_4:
		case MFP68901_TCR_TIMER_DELAY_10:
		case MFP68901_TCR_TIMER_DELAY_16:
		case MFP68901_TCR_TIMER_DELAY_50:
		case MFP68901_TCR_TIMER_DELAY_64:
		case MFP68901_TCR_TIMER_DELAY_100:
		case MFP68901_TCR_TIMER_DELAY_200:
			{
			int divisor = PRESCALER[mfp_p->tcdcr & 0x07];
			logerror("MFP68901 #%u Timer D Delay Mode : %u Prescale\n", which, divisor);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_D], MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / divisor), which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / divisor));
			}
			break;
		}

		switch ((mfp_p->tcdcr >> 4) & 0x07)
		{
		case MFP68901_TCR_TIMER_STOPPED:
			logerror("MFP68901 #%u Timer C Stopped\n", which);
			mame_timer_enable(mfp_p->timer[MFP68901_TIMER_C], 0);
			break;

		case MFP68901_TCR_TIMER_DELAY_4:
		case MFP68901_TCR_TIMER_DELAY_10:
		case MFP68901_TCR_TIMER_DELAY_16:
		case MFP68901_TCR_TIMER_DELAY_50:
		case MFP68901_TCR_TIMER_DELAY_64:
		case MFP68901_TCR_TIMER_DELAY_100:
		case MFP68901_TCR_TIMER_DELAY_200:
			{
			int divisor = PRESCALER[(mfp_p->tcdcr >> 4) & 0x07];
			logerror("MFP68901 #%u Timer C Delay Mode : %u Prescale\n", which, divisor);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_C], MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / divisor), which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / divisor));
			}
			break;
		}
		break;

	case MFP68901_REGISTER_TADR:
		logerror("MFP68901 #%u Timer A Data Register : %x\n", which, data);

		mfp_p->tdr[MFP68901_TIMER_A] = data;

		if (!mame_timer_enabled(mfp_p->timer[MFP68901_TIMER_A]))
		{
			mfp_p->tmc[MFP68901_TIMER_A] = data;
		}
		break;

	case MFP68901_REGISTER_TBDR:
		logerror("MFP68901 #%u Timer B Data Register : %x\n", which, data);

		mfp_p->tdr[MFP68901_TIMER_B] = data;

		if (!mame_timer_enabled(mfp_p->timer[MFP68901_TIMER_B]))
		{
			mfp_p->tmc[MFP68901_TIMER_B] = data;
		}
		break;

	case MFP68901_REGISTER_TCDR:
		logerror("MFP68901 #%u Timer C Data Register : %x\n", which, data);

		mfp_p->tdr[MFP68901_TIMER_C] = data;

		if (!mame_timer_enabled(mfp_p->timer[MFP68901_TIMER_C]))
		{
			mfp_p->tmc[MFP68901_TIMER_C] = data;
		}
		break;

	case MFP68901_REGISTER_TDDR:
		logerror("MFP68901 #%u Timer D Data Register : %x\n", which, data);

		mfp_p->tdr[MFP68901_TIMER_D] = data;

		if (!mame_timer_enabled(mfp_p->timer[MFP68901_TIMER_D]))
		{
			mfp_p->tmc[MFP68901_TIMER_D] = data;
		}
		break;

	case MFP68901_REGISTER_SCR:
		logerror("MFP68901 #%u Sync Character : %x\n", which, data);

		mfp_p->scr = data;
		break;

	case MFP68901_REGISTER_UCR:
		if (data & MFP68901_UCR_PARITY_ENABLED)
		{
			if (data & MFP68901_UCR_PARITY_EVEN)
				logerror("MFP68901 #%u Parity : Even\n", which);
			else
				logerror("MFP68901 #%u Parity : Odd\n", which);
		}
		else
		{
			logerror("MFP68901 #%u Parity : Disabled\n", which);
		}

		switch (data & 0x60)
		{
		case MFP68901_UCR_WORD_LENGTH_8:
			mfp_p->rxtx_word = 8;
			logerror("MFP68901 #%u Word Length : 8 bits\n", which);
			break;
		case MFP68901_UCR_WORD_LENGTH_7:
			mfp_p->rxtx_word = 7;
			logerror("MFP68901 #%u Word Length : 7 bits\n", which);
			break;
		case MFP68901_UCR_WORD_LENGTH_6:
			mfp_p->rxtx_word = 6;
			logerror("MFP68901 #%u Word Length : 6 bits\n", which);
			break;
		case MFP68901_UCR_WORD_LENGTH_5:
			mfp_p->rxtx_word = 5;
			logerror("MFP68901 #%u Word Length : 5 bits\n", which);
			break;
		}

		switch (data & 0x18)
		{
		case MFP68901_UCR_START_STOP_0_0:
			mfp_p->rxtx_start = 0;
			mfp_p->rxtx_stop = 0;
			logerror("MFP68901 #%u Start Bits : 0, Stop Bits : 0, Format : synchronous\n", which);
			break;
		case MFP68901_UCR_START_STOP_1_1:
			mfp_p->rxtx_start = 1;
			mfp_p->rxtx_stop = 1;
			logerror("MFP68901 #%u Start Bits : 1, Stop Bits : 1, Format : asynchronous\n", which);
			break;
		case MFP68901_UCR_START_STOP_1_15:
			mfp_p->rxtx_start = 1;
			mfp_p->rxtx_stop = 1;
			logerror("MFP68901 #%u Start Bits : 1, Stop Bits : 1½, Format : asynchronous\n", which);
			break;
		case MFP68901_UCR_START_STOP_1_2:
			mfp_p->rxtx_start = 1;
			mfp_p->rxtx_stop = 2;
			logerror("MFP68901 #%u Start Bits : 1, Stop Bits : 2, Format : asynchronous\n", which);
			break;
		}

		if (data & MFP68901_UCR_CLOCK_DIVIDE_16)
			logerror("MFP68901 #%u Rx/Tx Clock Divisor : 16\n", which);
		else
			logerror("MFP68901 #%u Rx/Tx Clock Divisor : 1\n", which);

		mfp_p->ucr = data;
		break;

	case MFP68901_REGISTER_RSR:
		if ((data & MFP68901_RSR_RCV_ENABLE) == 0)
		{
			logerror("MFP68901 #%u Receiver Disabled\n", which);
			mfp_p->rsr = 0;
		}
		else
		{
			logerror("MFP68901 #%u Receiver Enabled\n", which);

			if (data & MFP68901_RSR_SYNC_STRIP_ENABLE)
				logerror("MFP68901 #%u Sync Strip Enabled\n", which);
			else
				logerror("MFP68901 #%u Sync Strip Disabled\n", which);

			if (data & MFP68901_RSR_FOUND_SEARCH)
				logerror("MFP68901 #%u Receiver Search Mode Enabled\n", which);

			mfp_p->rsr = data & 0x0b;
		}
		break;

	case MFP68901_REGISTER_TSR:
		if ((data & MFP68901_TSR_XMIT_ENABLE) == 0)
		{
			logerror("MFP68901 #%u Transmitter Disabled\n", which);

			mfp_p->tsr = data & 0x27;
		}
		else
		{
			logerror("MFP68901 #%u Transmitter Enabled\n", which);

			switch (data & 0x06)
			{
			case MFP68901_TSR_OUTPUT_HI_Z:
				logerror("MFP68901 #%u Transmitter Disabled Output State : Hi-Z\n", which);
				break;
			case MFP68901_TSR_OUTPUT_LOW:
				logerror("MFP68901 #%u Transmitter Disabled Output State : 0\n", which);
				break;
			case MFP68901_TSR_OUTPUT_HIGH:
				logerror("MFP68901 #%u Transmitter Disabled Output State : 1\n", which);
				break;
			case MFP68901_TSR_OUTPUT_LOOP:
				logerror("MFP68901 #%u Transmitter Disabled Output State : Loop\n", which);
				break;
			}

			if (data & MFP68901_TSR_BREAK)
				logerror("MFP68901 #%u Transmitter Break Enabled\n", which);
			else
				logerror("MFP68901 #%u Transmitter Break Disabled\n", which);

			if (data & MFP68901_TSR_AUTO_TURNAROUND)
				logerror("MFP68901 #%u Transmitter Auto Turnaround Enabled\n", which);
			else
				logerror("MFP68901 #%u Transmitter Auto Turnaround Disabled\n", which);

			mfp_p->tsr = data & 0x2f;
			mfp_p->tsr |= MFP68901_TSR_BUFFER_EMPTY;  // x68000 expects the buffer to be empty, so this will do for now
		}
		break;

	case MFP68901_REGISTER_UDR:
		logerror("MFP68901 UDR %x\n", data);
		mfp_p->udr = data;
		//mfp_p->tsr &= ~MFP68901_TSR_BUFFER_EMPTY;
		break;
	}
}

void mfp68901_timer_count_a(int which)
{
	mfp_68901 *mfp_p = &mfp[which];

	if (mfp_p->tmc[MFP68901_TIMER_A] == 0x01)
	{
		mfp_p->to[MFP68901_TIMER_A] = mfp_p->to[MFP68901_TIMER_A] ? 0 : 1;

		if (mfp_p->to[MFP68901_TIMER_A])
		{
			if (mfp_p->intf->rx_clock == MFP68901_TAO_LOOPBACK)
			{
				mame_timer_adjust(mfp_p->rx_timer, time_zero, which, time_never);
			}

			if (mfp_p->intf->tx_clock == MFP68901_TAO_LOOPBACK)
			{
				mame_timer_adjust(mfp_p->tx_timer, time_zero, which, time_never);
			}
		}

		if (mfp_p->intf->to_w)
		{
			mfp_p->intf->to_w(which, MFP68901_TIMER_A, mfp_p->to[MFP68901_TIMER_A]);
		}

		if (mfp_p->ier & MFP68901_IR_TIMER_A)
		{
			mfp_p->ipr |= MFP68901_IR_TIMER_A;
		}

		mfp_p->tmc[MFP68901_TIMER_A] = mfp_p->tdr[MFP68901_TIMER_A];
	}
	else
	{
		mfp_p->tmc[MFP68901_TIMER_A]--;
	}
}

void mfp68901_timer_count_b(int which)
{
	mfp_68901 *mfp_p = &mfp[which];

	if (mfp_p->tmc[MFP68901_TIMER_B] == 0x01)
	{
		mfp_p->to[MFP68901_TIMER_B] = mfp_p->to[MFP68901_TIMER_B] ? 0 : 1;

		if (mfp_p->to[MFP68901_TIMER_B])
		{
			if (mfp_p->intf->rx_clock == MFP68901_TBO_LOOPBACK)
			{
				mame_timer_adjust(mfp_p->rx_timer, time_zero, which, time_never);
			}

			if (mfp_p->intf->tx_clock == MFP68901_TBO_LOOPBACK)
			{
				mame_timer_adjust(mfp_p->tx_timer, time_zero, which, time_never);
			}
		}

		if (mfp_p->intf->to_w)
		{
			mfp_p->intf->to_w(which, MFP68901_TIMER_B, mfp_p->to[MFP68901_TIMER_B]);
		}

		if (mfp_p->ier & MFP68901_IR_TIMER_B)
		{
			mfp_p->ipr |= MFP68901_IR_TIMER_B;
		}

		mfp_p->tmc[MFP68901_TIMER_B] = mfp_p->tdr[MFP68901_TIMER_B];
	}
	else
	{
		mfp_p->tmc[MFP68901_TIMER_B]--;
	}
}

void mfp68901_timer_count_c(int which)
{
	mfp_68901 *mfp_p = &mfp[which];

	if (mfp_p->tmc[MFP68901_TIMER_C] == 0x01)
	{
		mfp_p->to[MFP68901_TIMER_C] = mfp_p->to[MFP68901_TIMER_C] ? 0 : 1;

		if (mfp_p->to[MFP68901_TIMER_C])
		{
			if (mfp_p->intf->rx_clock == MFP68901_TCO_LOOPBACK)
			{
				mame_timer_adjust(mfp_p->rx_timer, time_zero, which, time_never);
			}

			if (mfp_p->intf->tx_clock == MFP68901_TCO_LOOPBACK)
			{
				mame_timer_adjust(mfp_p->tx_timer, time_zero, which, time_never);
			}
		}

		if (mfp_p->intf->to_w)
		{
			mfp_p->intf->to_w(which, MFP68901_TIMER_C, mfp_p->to[MFP68901_TIMER_C]);
		}

		if (mfp_p->ier & MFP68901_IR_TIMER_C)
		{
			mfp_p->ipr |= MFP68901_IR_TIMER_C;
		}

		mfp_p->tmc[MFP68901_TIMER_C] = mfp_p->tdr[MFP68901_TIMER_C];
	}
	else
	{
		mfp_p->tmc[MFP68901_TIMER_C]--;
	}
}

void mfp68901_timer_count_d(int which)
{
	mfp_68901 *mfp_p = &mfp[which];

	if (mfp_p->tmc[MFP68901_TIMER_D] == 0x01)
	{
		mfp_p->to[MFP68901_TIMER_D] = mfp_p->to[MFP68901_TIMER_D] ? 0 : 1;

		if (mfp_p->to[MFP68901_TIMER_D])
		{
			if (mfp_p->intf->rx_clock == MFP68901_TDO_LOOPBACK)
			{
				mame_timer_adjust(mfp_p->rx_timer, time_zero, which, time_never);
			}

			if (mfp_p->intf->tx_clock == MFP68901_TDO_LOOPBACK)
			{
				mame_timer_adjust(mfp_p->tx_timer, time_zero, which, time_never);
			}
		}

		if (mfp_p->intf->to_w)
		{
			mfp_p->intf->to_w(which, MFP68901_TIMER_D, mfp_p->to[MFP68901_TIMER_D]);
		}

		if (mfp_p->ier & MFP68901_IR_TIMER_D)
		{
			mfp_p->ipr |= MFP68901_IR_TIMER_D;
		}

		mfp_p->tmc[MFP68901_TIMER_D] = mfp_p->tdr[MFP68901_TIMER_D];
	}
	else
	{
		mfp_p->tmc[MFP68901_TIMER_D]--;
	}
}

void mfp68901_tai_w(int which, int value)
{
	mfp_68901 *mfp_p = &mfp[which];

	switch (mfp_p->tacr & 0x0f)
	{
	case MFP68901_TCR_TIMER_EVENT:
		if (((mfp_p->ti[MFP68901_TIMER_A] ^ BIT(mfp_p->aer, 4)) == 1) && ((value ^ BIT(mfp_p->aer, 4)) == 0))
		{
			mfp68901_timer_count_a(which);
		}
		mfp_p->ti[MFP68901_TIMER_A] = value;
		break;

	case MFP68901_TCR_TIMER_PULSE_4:
	case MFP68901_TCR_TIMER_PULSE_10:
	case MFP68901_TCR_TIMER_PULSE_16:
	case MFP68901_TCR_TIMER_PULSE_50:
	case MFP68901_TCR_TIMER_PULSE_64:
	case MFP68901_TCR_TIMER_PULSE_100:
	case MFP68901_TCR_TIMER_PULSE_200:
		mame_timer_enable(mfp_p->timer[MFP68901_TIMER_A], (BIT(mfp_p->aer, 4) == value) ? 1 : 0);

		if (((mfp_p->ti[MFP68901_TIMER_A] ^ BIT(mfp_p->aer, 4)) == 0) && ((value ^ BIT(mfp_p->aer, 4)) == 1))
		{
			if (mfp_p->ier & MFP68901_IR_GPIP_4)
			{
				mfp_p->ipr |= MFP68901_IR_GPIP_4;
			}
		}

		mfp_p->ti[MFP68901_TIMER_A] = value;
		break;
	}
}

void mfp68901_tbi_w(int which, int value)
{
	mfp_68901 *mfp_p = &mfp[which];

	switch (mfp_p->tbcr & 0x0f)
	{
	case MFP68901_TCR_TIMER_EVENT:
		if (((mfp_p->ti[MFP68901_TIMER_B] ^ BIT(mfp_p->aer, 3)) == 1) && ((value ^ BIT(mfp_p->aer, 3)) == 0))
		{
			mfp68901_timer_count_b(which);
		}
		mfp_p->ti[MFP68901_TIMER_B] = value;
		break;

	case MFP68901_TCR_TIMER_PULSE_4:
	case MFP68901_TCR_TIMER_PULSE_10:
	case MFP68901_TCR_TIMER_PULSE_16:
	case MFP68901_TCR_TIMER_PULSE_50:
	case MFP68901_TCR_TIMER_PULSE_64:
	case MFP68901_TCR_TIMER_PULSE_100:
	case MFP68901_TCR_TIMER_PULSE_200:
		mame_timer_enable(mfp_p->timer[MFP68901_TIMER_B], (BIT(mfp_p->aer, 3) == value) ? 1 : 0);

		if (((mfp_p->ti[MFP68901_TIMER_B] ^ BIT(mfp_p->aer, 3)) == 0) && ((value ^ BIT(mfp_p->aer, 3)) == 1))
		{
			if (mfp_p->ier & MFP68901_IR_GPIP_3)
			{
				mfp_p->ipr |= MFP68901_IR_GPIP_3;
			}
		}

		mfp_p->ti[MFP68901_TIMER_B] = value;
		break;
	}
}

static TIMER_CALLBACK( timer_a )
{
	mfp68901_timer_count_a(param);
}

static TIMER_CALLBACK( timer_b )
{
	mfp68901_timer_count_b(param);
}

static TIMER_CALLBACK( timer_c )
{
	mfp68901_timer_count_c(param);
}

static TIMER_CALLBACK( timer_d )
{
	mfp68901_timer_count_d(param);
}

void mfp68901_config(int which, const mfp68901_interface *intf)
{
	mfp_68901 *mfp_p = &mfp[which];

	if (which >= MAX_MFP)
	{
		return;
	}

	memset(mfp_p, 0, sizeof(mfp));

	mfp_p->tsr = MFP68901_TSR_BUFFER_EMPTY;

	mfp_p->intf = intf;

	mfp_p->timer[MFP68901_TIMER_A] = mame_timer_alloc(timer_a);
	mfp_p->timer[MFP68901_TIMER_B] = mame_timer_alloc(timer_b);
	mfp_p->timer[MFP68901_TIMER_C] = mame_timer_alloc(timer_c);
	mfp_p->timer[MFP68901_TIMER_D] = mame_timer_alloc(timer_d);

	mfp_p->rx_timer = mame_timer_alloc(rx_tick);
	
	if (mfp_p->intf->rx_clock > 0)
	{
		mame_timer_adjust(mfp_p->rx_timer, time_zero, which, MAME_TIME_IN_HZ(mfp_p->intf->rx_clock));
	}
	
	mfp_p->tx_timer = mame_timer_alloc(tx_tick);

	if (mfp_p->intf->tx_clock > 0)
	{
		mame_timer_adjust(mfp_p->tx_timer, time_zero, which, MAME_TIME_IN_HZ(mfp_p->intf->tx_clock));
	}

	mfp_p->irq_timer = mame_timer_alloc(mfp68901_tick);
	mame_timer_adjust(mfp_p->irq_timer, time_zero, which, MAME_TIME_IN_HZ(mfp_p->intf->chip_clock / 4));

	state_save_register_item("mfp68901", which, mfp_p->gpip);
	state_save_register_item("mfp68901", which, mfp_p->aer);
	state_save_register_item("mfp68901", which, mfp_p->ddr);
	state_save_register_item("mfp68901", which, mfp_p->ier);
	state_save_register_item("mfp68901", which, mfp_p->ipr);
	state_save_register_item("mfp68901", which, mfp_p->isr);
	state_save_register_item("mfp68901", which, mfp_p->imr);
	state_save_register_item("mfp68901", which, mfp_p->vr);
	state_save_register_item("mfp68901", which, mfp_p->tacr);
	state_save_register_item("mfp68901", which, mfp_p->tbcr);
	state_save_register_item("mfp68901", which, mfp_p->tcdcr);
	state_save_register_item_array("mfp68901", which, mfp_p->tdr);
	state_save_register_item_array("mfp68901", which, mfp_p->tmc);
	state_save_register_item_array("mfp68901", which, mfp_p->to);
	state_save_register_item_array("mfp68901", which, mfp_p->ti);
	state_save_register_item("mfp68901", which, mfp_p->scr);
	state_save_register_item("mfp68901", which, mfp_p->ucr);
	state_save_register_item("mfp68901", which, mfp_p->rsr);
	state_save_register_item("mfp68901", which, mfp_p->tsr);
	state_save_register_item("mfp68901", which, mfp_p->udr);
	state_save_register_item("mfp68901", which, mfp_p->tx_bits);
	state_save_register_item("mfp68901", which, mfp_p->rx_bits);
	state_save_register_item("mfp68901", which, mfp_p->tx_parity);
	state_save_register_item("mfp68901", which, mfp_p->rx_parity);
	state_save_register_item("mfp68901", which, mfp_p->rsr_read);
	state_save_register_item("mfp68901", which, mfp_p->next_rsr);
}

READ16_HANDLER( mfp68901_0_register_msb_r ) { return mfp68901_register_r(0, offset) << 8; }
READ16_HANDLER( mfp68901_1_register_msb_r ) { return mfp68901_register_r(1, offset) << 8; }
READ16_HANDLER( mfp68901_2_register_msb_r ) { return mfp68901_register_r(2, offset) << 8; }
READ16_HANDLER( mfp68901_3_register_msb_r ) { return mfp68901_register_r(3, offset) << 8; }

READ16_HANDLER( mfp68901_0_register_lsb_r ) { return mfp68901_register_r(0, offset); }
READ16_HANDLER( mfp68901_1_register_lsb_r ) { return mfp68901_register_r(1, offset); }
READ16_HANDLER( mfp68901_2_register_lsb_r ) { return mfp68901_register_r(2, offset); }
READ16_HANDLER( mfp68901_3_register_lsb_r ) { return mfp68901_register_r(3, offset); }

WRITE16_HANDLER( mfp68901_0_register_msb_w ) { if (ACCESSING_MSB) mfp68901_register_w(0, offset, (data >> 8) & 0xff); }
WRITE16_HANDLER( mfp68901_1_register_msb_w ) { if (ACCESSING_MSB) mfp68901_register_w(0, offset, (data >> 8) & 0xff); }
WRITE16_HANDLER( mfp68901_2_register_msb_w ) { if (ACCESSING_MSB) mfp68901_register_w(0, offset, (data >> 8) & 0xff); }
WRITE16_HANDLER( mfp68901_3_register_msb_w ) { if (ACCESSING_MSB) mfp68901_register_w(0, offset, (data >> 8) & 0xff); }

WRITE16_HANDLER( mfp68901_0_register_lsb_w ) { if (ACCESSING_LSB) mfp68901_register_w(0, offset, data & 0xff); }
WRITE16_HANDLER( mfp68901_1_register_lsb_w ) { if (ACCESSING_LSB) mfp68901_register_w(0, offset, data & 0xff); }
WRITE16_HANDLER( mfp68901_2_register_lsb_w ) { if (ACCESSING_LSB) mfp68901_register_w(0, offset, data & 0xff); }
WRITE16_HANDLER( mfp68901_3_register_lsb_w ) { if (ACCESSING_LSB) mfp68901_register_w(0, offset, data & 0xff); }
