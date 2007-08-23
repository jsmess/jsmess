/*

	TODO:

	- serial I/O
	- daisy chaining
	- correct irq_timer period

*/

#include "68901mfp.h"

typedef struct
{
	const mfp68901_interface *intf;

	UINT8 gpip;
	UINT8 aer;
	UINT8 ddr;

	UINT8 iera, ierb;
	UINT8 ipra, iprb;
	UINT8 isra, isrb;
	UINT8 imra, imrb;
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
	UINT8 tsr, rsr, next_rsr;
	int	rx_bits, tx_bits;
	int	rx_parity, tx_parity;
	int rsr_read;
	mame_timer *rx_timer, *tx_timer;
} mfp_68901;

static mfp_68901 mfp[MAX_MFP];

static const int GPIO_MASK[] =
{ 
	MFP68901_IPRB_GPIP_0, MFP68901_IPRB_GPIP_1, MFP68901_IPRB_GPIP_2, MFP68901_IPRB_GPIP_3, 
	MFP68901_IPRB_GPIP_4, MFP68901_IPRB_GPIP_5, MFP68901_IPRA_GPIP_6, MFP68901_IPRA_GPIP_7 
};

static void mfp68901_poll_gpio(int which)
{
	mfp_68901 *mfp_p = &mfp[which];

	UINT8 gpio = 0, gpold, gpnew;
	UINT8 *ier, *ipr, *imr;
	int bit;

	if (mfp_p->intf->gpio_r)
	{
		gpio = mfp_p->intf->gpio_r(0);
	}

	gpold = (mfp_p->gpip & ~mfp_p->ddr) ^ mfp_p->aer;
	gpnew = (gpio & ~mfp_p->ddr) ^ mfp_p->aer;

	for (bit = 0; bit < 8; bit++)
	{
		if (bit < 6)
		{
			ier = &mfp_p->ierb;
			ipr = &mfp_p->iprb;
			imr = &mfp_p->imrb;
		}
		else
		{
			ier = &mfp_p->iera;
			ipr = &mfp_p->ipra;
			imr = &mfp_p->imra;
		}

		if ((BIT(gpold, bit) == 1) && (BIT(gpnew, bit) == 0))
		{
			if (*ier & GPIO_MASK[bit])
			{
				(*ipr) |= GPIO_MASK[bit];
			}
		}
	}

	mfp_p->gpip = (gpio & ~mfp_p->ddr) | (mfp_p->gpip & mfp_p->ddr);
}

static void mfp68901_irq_ack(int which)
{
	mfp_68901 *mfp_p = &mfp[which];

	UINT16 ipr = (mfp_p->ipra << 8) | mfp_p->iprb;
	UINT16 isr = (mfp_p->isra << 8) | mfp_p->isrb;
	UINT16 imr = (mfp_p->imra << 8) | mfp_p->imrb;
	int ch;

	if (!isr)
	{
		for (ch = 15; ch > 0; ch--)
		{
			if (BIT(ipr, ch) && BIT(imr, ch))
			{
				ipr -= (1 << ch);

				mfp_p->ipra = (ipr & 0xff00) >> 8;
				mfp_p->iprb = ipr & 0xff;

				if (mfp_p->vr & MFP68901_VR_S)
				{
					isr |= (1 << ch);

					mfp_p->isra = (isr & 0xff00) >> 8;
					mfp_p->isrb = isr & 0xff;
				}

				mfp_p->intf->irq_callback(which, HOLD_LINE, (mfp_p->vr & 0xf0) | (15 - ch));
				logerror("MFP68901 #%u Interrupt %u\n", which, 15 - ch);
				return;
			}

			ipr <<= 1;
			isr <<= 1;
			imr <<= 1;
		}

		mfp_p->intf->irq_callback(which, CLEAR_LINE, 0);
	}
}

static TIMER_CALLBACK( mfp68901_tick )
{
	mfp68901_poll_gpio(param);
	mfp68901_irq_ack(param);
}

static UINT8 mfp68901_register_r(int which, int reg)
{
	mfp_68901 *mfp_p = &mfp[which];

	switch (reg)
	{
	case MFP68901_REGISTER_GPIP:  return mfp_p->gpip;
	case MFP68901_REGISTER_AER:   return mfp_p->aer;
	case MFP68901_REGISTER_DDR:   return mfp_p->ddr;

	case MFP68901_REGISTER_IERA:  return mfp_p->iera;
	case MFP68901_REGISTER_IERB:  return mfp_p->ierb;
	case MFP68901_REGISTER_IPRA:  return mfp_p->ipra;
	case MFP68901_REGISTER_IPRB:  return mfp_p->iprb;
	case MFP68901_REGISTER_ISRA:  return mfp_p->isra;
	case MFP68901_REGISTER_ISRB:  return mfp_p->isrb;
	case MFP68901_REGISTER_IMRA:  return mfp_p->imra;
	case MFP68901_REGISTER_IMRB:  return mfp_p->imrb;
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
	case MFP68901_REGISTER_RSR:   return mfp_p->rsr;
	case MFP68901_REGISTER_TSR:   return mfp_p->tsr;
	case MFP68901_REGISTER_UDR:   return mfp_p->udr;

	default:					  return 0;
	}
}

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
		mfp_p->iera = data;
		mfp_p->ipra &= mfp_p->iera;
		break;
	case MFP68901_REGISTER_IERB:
		logerror("MFP68901 #%u Interrupt Enable Register B : %x\n", which, data);
		mfp_p->ierb = data;
		mfp_p->iprb &= mfp_p->ierb;
		break;
	case MFP68901_REGISTER_IPRA:
		logerror("MFP68901 #%u Interrupt Pending Register A : %x\n", which, data);
		mfp_p->ipra &= data;
		break;
	case MFP68901_REGISTER_IPRB:
		logerror("MFP68901 #%u Interrupt Pending Register B : %x\n", which, data);
		mfp_p->iprb &= data;
		break;
	case MFP68901_REGISTER_ISRA:
		logerror("MFP68901 #%u Interrupt In-Service Register A : %x\n", which, data);
		mfp_p->isra &= data;
		break;
	case MFP68901_REGISTER_ISRB:
		logerror("MFP68901 #%u Interrupt In-Service Register B : %x\n", which, data);
		mfp_p->isrb &= data;
		break;
	case MFP68901_REGISTER_IMRA:
		logerror("MFP68901 #%u Interrupt Mask Register A : %x\n", which, data);
		mfp_p->imra = data;
		break;
	case MFP68901_REGISTER_IMRB:
		logerror("MFP68901 #%u Interrupt Mask Register B : %x\n", which, data);
		mfp_p->imrb = data;
		break;
	case MFP68901_REGISTER_VR:
		logerror("MFP68901 #%u Interrupt Vector : %x\n", which, data & 0xf0);

		mfp_p->vr = data & 0xf8;

		if ((mfp_p->vr & MFP68901_VR_S) == 0)
		{
			logerror("MFP68901 #%u Automatic End-Of-Interrupt Mode\n", which);

			mfp_p->isra = mfp_p->isrb = 0;
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
		case MFP68901_TCR_TIMER_DELAY_4:
			logerror("MFP68901 #%u Timer A Delay Mode : 4 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_A], time_never, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 4));
			break;
		case MFP68901_TCR_TIMER_DELAY_10:
			logerror("MFP68901 #%u Timer A Delay Mode : 10 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_A], time_never, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 10));
			break;
		case MFP68901_TCR_TIMER_DELAY_16:
			logerror("MFP68901 #%u Timer A Delay Mode : 16 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_A], time_never, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 16));
			break;
		case MFP68901_TCR_TIMER_DELAY_50:
			logerror("MFP68901 #%u Timer A Delay Mode : 50 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_A], time_never, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 50));
			break;
		case MFP68901_TCR_TIMER_DELAY_64:
			logerror("MFP68901 #%u Timer A Delay Mode : 64 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_A], time_never, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 64));
			break;
		case MFP68901_TCR_TIMER_DELAY_100:
			logerror("MFP68901 #%u Timer A Delay Mode : 100 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_A], time_never, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 100));
			break;
		case MFP68901_TCR_TIMER_DELAY_200:
			logerror("MFP68901 #%u Timer A Delay Mode : 200 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_A], time_never, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 200));
			break;
		case MFP68901_TCR_TIMER_STOPPED:
			logerror("MFP68901 #%u Timer A Stopped\n", which);
			mame_timer_enable(mfp_p->timer[MFP68901_TIMER_A], 0);
			break;
		case MFP68901_TCR_TIMER_EVENT:
			logerror("MFP68901 #%u Timer A Event Count Mode\n", which);
			mame_timer_enable(mfp_p->timer[MFP68901_TIMER_A], 0);
			break;
		case MFP68901_TCR_TIMER_PULSE_4:
			logerror("MFP68901 #%u Timer A Pulse Width Mode : 4 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_A], time_zero, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 4));
			break;
		case MFP68901_TCR_TIMER_PULSE_10:
			logerror("MFP68901 #%u Timer A Pulse Width Mode : 10 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_A], time_zero, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 10));
			break;
		case MFP68901_TCR_TIMER_PULSE_16:
			logerror("MFP68901 #%u Timer A Pulse Width Mode : 16 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_A], time_zero, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 16));
			break;
		case MFP68901_TCR_TIMER_PULSE_50:
			logerror("MFP68901 #%u Timer A Pulse Width Mode : 50 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_A], time_zero, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 50));
			break;
		case MFP68901_TCR_TIMER_PULSE_64:
			logerror("MFP68901 #%u Timer A Pulse Width Mode : 64 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_A], time_zero, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 64));
			break;
		case MFP68901_TCR_TIMER_PULSE_100:
			logerror("MFP68901 #%u Timer A Pulse Width Mode : 100 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_A], time_zero, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 100));
			break;
		case MFP68901_TCR_TIMER_PULSE_200:
			logerror("MFP68901 #%u Timer A Pulse Width Mode : 200 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_A], time_zero, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 200));
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
		case MFP68901_TCR_TIMER_DELAY_4:
			logerror("MFP68901 #%u Timer B Delay Mode : 4 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_B], time_never, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 4));
			break;
		case MFP68901_TCR_TIMER_DELAY_10:
			logerror("MFP68901 #%u Timer B Delay Mode : 10 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_B], time_never, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 10));
			break;
		case MFP68901_TCR_TIMER_DELAY_16:
			logerror("MFP68901 #%u Timer B Delay Mode : 16 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_B], time_never, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 16));
			break;
		case MFP68901_TCR_TIMER_DELAY_50:
			logerror("MFP68901 #%u Timer B Delay Mode : 50 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_B], time_never, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 50));
			break;
		case MFP68901_TCR_TIMER_DELAY_64:
			logerror("MFP68901 #%u Timer B Delay Mode : 64 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_B], time_never, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 64));
			break;
		case MFP68901_TCR_TIMER_DELAY_100:
			logerror("MFP68901 #%u Timer B Delay Mode : 100 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_B], time_never, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 100));
			break;
		case MFP68901_TCR_TIMER_DELAY_200:
			logerror("MFP68901 #%u Timer B Delay Mode : 200 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_B], time_never, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 200));
			break;
		case MFP68901_TCR_TIMER_STOPPED:
			logerror("MFP68901 #%u Timer B Stopped\n", which);
			mame_timer_enable(mfp_p->timer[MFP68901_TIMER_B], 0);
			break;
		case MFP68901_TCR_TIMER_EVENT:
			logerror("MFP68901 #%u Timer B Event Count Mode\n", which);
			mame_timer_enable(mfp_p->timer[MFP68901_TIMER_B], 0);
			break;
		case MFP68901_TCR_TIMER_PULSE_4:
			logerror("MFP68901 #%u Timer B Pulse Width Mode : 4 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_B], time_zero, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 4));
			break;
		case MFP68901_TCR_TIMER_PULSE_10:
			logerror("MFP68901 #%u Timer B Pulse Width Mode : 10 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_B], time_zero, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 10));
			break;
		case MFP68901_TCR_TIMER_PULSE_16:
			logerror("MFP68901 #%u Timer B Pulse Width Mode : 16 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_B], time_zero, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 16));
			break;
		case MFP68901_TCR_TIMER_PULSE_50:
			logerror("MFP68901 #%u Timer B Pulse Width Mode : 50 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_B], time_zero, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 50));
			break;
		case MFP68901_TCR_TIMER_PULSE_64:
			logerror("MFP68901 #%u Timer B Pulse Width Mode : 64 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_B], time_zero, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 64));
			break;
		case MFP68901_TCR_TIMER_PULSE_100:
			logerror("MFP68901 #%u Timer B Pulse Width Mode : 100 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_B], time_zero, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 100));
			break;
		case MFP68901_TCR_TIMER_PULSE_200:
			logerror("MFP68901 #%u Timer B Pulse Width Mode : 200 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_B], time_zero, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 200));
			break;
		}

		if (mfp_p->tbcr & MFP68901_TCR_TIMER_RESET)
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
		case MFP68901_TCR_TIMER_DELAY_4:
			logerror("MFP68901 #%u Timer D Delay Mode : 4 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_D], time_never, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 4));
			break;
		case MFP68901_TCR_TIMER_DELAY_10:
			logerror("MFP68901 #%u Timer D Delay Mode : 10 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_D], time_never, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 10));
			break;
		case MFP68901_TCR_TIMER_DELAY_16:
			logerror("MFP68901 #%u Timer D Delay Mode : 16 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_D], time_never, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 16));
			break;
		case MFP68901_TCR_TIMER_DELAY_50:
			logerror("MFP68901 #%u Timer D Delay Mode : 50 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_D], time_never, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 50));
			break;
		case MFP68901_TCR_TIMER_DELAY_64:
			logerror("MFP68901 #%u Timer D Delay Mode : 64 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_D], time_never, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 64));
			break;
		case MFP68901_TCR_TIMER_DELAY_100:
			logerror("MFP68901 #%u Timer D Delay Mode : 100 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_D], time_never, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 100));
			break;
		case MFP68901_TCR_TIMER_DELAY_200:
			logerror("MFP68901 #%u Timer D Delay Mode : 200 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_D], time_never, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 200));
			break;
		case MFP68901_TCR_TIMER_STOPPED:
			logerror("MFP68901 #%u Timer D Stopped\n", which);
			mame_timer_enable(mfp_p->timer[MFP68901_TIMER_D], 0);
			break;
		}

		switch ((mfp_p->tcdcr >> 4) & 0x07)
		{
		case MFP68901_TCR_TIMER_DELAY_4:
			logerror("MFP68901 #%u Timer C Delay Mode : 4 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_C], time_never, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 4));
			break;
		case MFP68901_TCR_TIMER_DELAY_10:
			logerror("MFP68901 #%u Timer C Delay Mode : 10 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_C], time_never, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 10));
			break;
		case MFP68901_TCR_TIMER_DELAY_16:
			logerror("MFP68901 #%u Timer C Delay Mode : 16 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_C], time_never, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 16));
			break;
		case MFP68901_TCR_TIMER_DELAY_50:
			logerror("MFP68901 #%u Timer C Delay Mode : 50 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_C], time_never, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 50));
			break;
		case MFP68901_TCR_TIMER_DELAY_64:
			logerror("MFP68901 #%u Timer C Delay Mode : 64 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_C], time_never, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 64));
			break;
		case MFP68901_TCR_TIMER_DELAY_100:
			logerror("MFP68901 #%u Timer C Delay Mode : 100 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_C], time_never, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 100));
			break;
		case MFP68901_TCR_TIMER_DELAY_200:
			logerror("MFP68901 #%u Timer C Delay Mode : 200 Prescale\n", which);
			mame_timer_adjust(mfp_p->timer[MFP68901_TIMER_C], time_never, which, MAME_TIME_IN_HZ(mfp_p->intf->timer_clock / 200));
			break;
		case MFP68901_TCR_TIMER_STOPPED:
			logerror("MFP68901 #%u Timer C Stopped\n", which);
			mame_timer_enable(mfp_p->timer[MFP68901_TIMER_C], 0);
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

	// USART not implemented
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
			logerror("MFP68901 #%u Word Length : 8 bits\n", which);
			break;
		case MFP68901_UCR_WORD_LENGTH_7:
			logerror("MFP68901 #%u Word Length : 7 bits\n", which);
			break;
		case MFP68901_UCR_WORD_LENGTH_6:
			logerror("MFP68901 #%u Word Length : 6 bits\n", which);
			break;
		case MFP68901_UCR_WORD_LENGTH_5:
			logerror("MFP68901 #%u Word Length : 5 bits\n", which);
			break;
		}

		switch (data & 0x18)
		{
		case MFP68901_UCR_START_STOP_0_0:
			logerror("MFP68901 #%u Start Bits : 0, Stop Bits : 0, Format : synchronous\n", which);
			break;
		case MFP68901_UCR_START_STOP_1_1:
			logerror("MFP68901 #%u Start Bits : 1, Stop Bits : 1, Format : asynchronous\n", which);
			break;
		case MFP68901_UCR_START_STOP_1_15:
			logerror("MFP68901 #%u Start Bits : 1, Stop Bits : 1½, Format : asynchronous\n", which);
			break;
		case MFP68901_UCR_START_STOP_1_2:
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
		}
		break;
	case MFP68901_REGISTER_UDR:
		logerror("MFP68901 UDR %x\n", data);
		mfp_p->udr = data;
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

		if (mfp_p->iera & MFP68901_IPRA_TIMER_A)
		{
			mfp_p->ipra |= MFP68901_IPRA_TIMER_A;
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

		if (mfp_p->iera & MFP68901_IPRA_TIMER_B)
		{
			mfp_p->ipra |= MFP68901_IPRA_TIMER_B;
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

		if (mfp_p->ierb & MFP68901_IPRB_TIMER_C)
		{
			mfp_p->iprb |= MFP68901_IPRB_TIMER_C;
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

		if (mfp_p->ierb & MFP68901_IPRB_TIMER_D)
		{
			mfp_p->iprb |= MFP68901_IPRB_TIMER_D;
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
			if (mfp_p->ierb & MFP68901_IPRB_GPIP_4)
			{
				mfp_p->iprb |= MFP68901_IPRB_GPIP_4;
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
			if (mfp_p->ierb & MFP68901_IPRB_GPIP_3)
			{
				mfp_p->iprb |= MFP68901_IPRB_GPIP_3;
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

static TIMER_CALLBACK( rx_tick )
{
	// TODO
}

static TIMER_CALLBACK( tx_tick )
{
	// TODO
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
	state_save_register_item("mfp68901", which, mfp_p->iera);
	state_save_register_item("mfp68901", which, mfp_p->ierb);
	state_save_register_item("mfp68901", which, mfp_p->ipra);
	state_save_register_item("mfp68901", which, mfp_p->iprb);
	state_save_register_item("mfp68901", which, mfp_p->isra);
	state_save_register_item("mfp68901", which, mfp_p->isrb);
	state_save_register_item("mfp68901", which, mfp_p->imra);
	state_save_register_item("mfp68901", which, mfp_p->imrb);
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
