/*

	TODO:
	
	- pulse mode
	- serial comms
	- interrupt queueing
	- gpio interrupts
	- trigger interrupt on AER change

*/

#include "68901mfp.h"

typedef struct
{
	UINT8 gpip;
	UINT8 aer;
	UINT8 ddr;

	UINT8 iera, ierb;
	UINT8 ipra, iprb;
	UINT8 isra, isrb;
	UINT8 imra, imrb;
	UINT8 vr;

	UINT8 tacr, tbcr, tcdcr;
	UINT8 tadr, tbdr, tcdr, tddr;
	UINT8 tarr, tbrr, tcrr, tdrr;
	int tao, tbo, tco, tdo;
	int tai, tbi;

	UINT8 scr;
	UINT8 ucr;
	UINT8 rsr;
	UINT8 tsr;
	UINT8 udr;

	mame_timer *timer_a, *timer_b, *timer_c, *timer_d;

	int	chip_clock;
	int	timer_clock;

	void (*tao_w)(int which, int value);
	void (*tbo_w)(int which, int value);
	void (*tco_w)(int which, int value);
	void (*tdo_w)(int which, int value);

	void (*irq_callback)(int which, int state, int vector);

	read8_handler gpio_r;
	write8_handler gpio_w;
} mfp_68901;

static mfp_68901 mfp[MAX_MFP];

static TIMER_CALLBACK( timer_a );
static TIMER_CALLBACK( timer_b );
static TIMER_CALLBACK( timer_c );
static TIMER_CALLBACK( timer_d );

void mfp68901_irq_ack(int which)
{
	mfp_68901 *mfp_p = &mfp[which];

	UINT16 ipr = (mfp_p->ipra << 8) | mfp_p->iprb;
	UINT16 isr = (mfp_p->isra << 8) | mfp_p->isrb;
	UINT16 imr = (mfp_p->imra << 8) | mfp_p->imrb;
	int ch;

	for (ch = 15; ch > 0; ch--)
	{
		if (BIT(isr, ch))
		{
			break;
		}

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

			mfp_p->irq_callback(which, ASSERT_LINE, (mfp_p->vr & 0xf0) | (15 - ch));
			return;
		}

		ipr <<= 1;
		isr <<= 1;
		imr <<= 1;
	}

	mfp_p->irq_callback(which, CLEAR_LINE, 0);
}

static UINT8 mfp68901_register_r(int which, int reg)
{
	mfp_68901 *mfp_p = &mfp[which];

	switch (reg)
	{
	case 0x00:
		{
		UINT8 gpio = mfp_p->gpio_r(0);
		gpio |= (mfp_p->gpip & mfp_p->ddr);
		// signal interrupts if necessary
		mfp68901_irq_ack(which);
		return gpio;
		}
	case 0x01: return mfp_p->aer;
	case 0x02: return mfp_p->ddr;
	case 0x03: return mfp_p->iera;
	case 0x04: return mfp_p->ierb;
	case 0x05: return mfp_p->ipra;
	case 0x06: return mfp_p->iprb;
	case 0x07: return mfp_p->isra;
	case 0x08: return mfp_p->isrb;
	case 0x09: return mfp_p->imra;
	case 0x0a: return mfp_p->imrb;
	case 0x0b: return mfp_p->vr;
	case 0x0c: return mfp_p->tacr;
	case 0x0d: return mfp_p->tbcr;
	case 0x0e: return mfp_p->tcdcr;
	case 0x0f: return mfp_p->tarr;
	case 0x10: return mfp_p->tbrr;
	case 0x11: return mfp_p->tcrr;
	case 0x12: return mfp_p->tdrr;
	case 0x13: return mfp_p->scr;
	case 0x14: return mfp_p->ucr;
	case 0x15: return mfp_p->rsr;
	case 0x16: return mfp_p->tsr;
	case 0x17: return mfp_p->udr;
	default:   return 0;
	}
}

static void mfp68901_register_w(int which, int reg, UINT8 data)
{
	mfp_68901 *mfp_p = &mfp[which];

	switch (reg)
	{
	case 0x00:
		mfp_p->gpip = data & mfp_p->ddr;
		mfp_p->gpio_r(mfp_p->gpip);
		break;
	case 0x01:
		mfp_p->aer = data;
		// check transition and trigger interrupt if necessary
		mfp68901_irq_ack(which);
		break;
	case 0x02:
		mfp_p->ddr = data;
		break;

	case 0x03:
		mfp_p->iera = data;
		mfp_p->ipra &= mfp_p->iera;
		mfp68901_irq_ack(which);
		break;
	case 0x04:
		mfp_p->ierb = data;
		mfp_p->iprb &= mfp_p->ierb;
		mfp68901_irq_ack(which);
		break;
	case 0x07:
		mfp_p->isra &= data;
		mfp68901_irq_ack(which);
		break;
	case 0x08:
		mfp_p->isrb &= data;
		mfp68901_irq_ack(which);
		break;
	case 0x09:
		mfp_p->imra = data;
		mfp68901_irq_ack(which);
		break;
	case 0x0a:
		mfp_p->imrb = data;
		mfp68901_irq_ack(which);
		break;
	case 0x0b:
		mfp_p->vr = data & 0xf8;
		if ((mfp_p->vr & MFP68901_VR_S) == 0)
		{
			mfp_p->isra = mfp_p->isrb = 0;
			mfp68901_irq_ack(which);
		}
		break;

	case 0x0c:
		mfp_p->tacr = data & 0x1f;

		switch (mfp_p->tacr & 0x0f)
		{
		case MFP68901_TCR_TIMER_DELAY_4:
			mame_timer_adjust(mfp_p->timer_a, time_zero, 0, MAME_TIME_IN_HZ(mfp_p->timer_clock / 4));
			break;
		case MFP68901_TCR_TIMER_DELAY_10:
			mame_timer_adjust(mfp_p->timer_a, time_zero, 0, MAME_TIME_IN_HZ(mfp_p->timer_clock / 10));
			break;
		case MFP68901_TCR_TIMER_DELAY_16:
			mame_timer_adjust(mfp_p->timer_a, time_zero, 0, MAME_TIME_IN_HZ(mfp_p->timer_clock / 16));
			break;
		case MFP68901_TCR_TIMER_DELAY_50:
			mame_timer_adjust(mfp_p->timer_a, time_zero, 0, MAME_TIME_IN_HZ(mfp_p->timer_clock / 50));
			break;
		case MFP68901_TCR_TIMER_DELAY_64:
			mame_timer_adjust(mfp_p->timer_a, time_zero, 0, MAME_TIME_IN_HZ(mfp_p->timer_clock / 64));
			break;
		case MFP68901_TCR_TIMER_DELAY_100:
			mame_timer_adjust(mfp_p->timer_a, time_zero, 0, MAME_TIME_IN_HZ(mfp_p->timer_clock / 100));
			break;
		case MFP68901_TCR_TIMER_DELAY_200:
			mame_timer_adjust(mfp_p->timer_a, time_zero, 0, MAME_TIME_IN_HZ(mfp_p->timer_clock / 200));
			break;
		case MFP68901_TCR_TIMER_STOPPED:
		case MFP68901_TCR_TIMER_EVENT:
		case MFP68901_TCR_TIMER_PULSE_4:
		case MFP68901_TCR_TIMER_PULSE_10:
		case MFP68901_TCR_TIMER_PULSE_16:
		case MFP68901_TCR_TIMER_PULSE_50:
		case MFP68901_TCR_TIMER_PULSE_64:
		case MFP68901_TCR_TIMER_PULSE_100:
		case MFP68901_TCR_TIMER_PULSE_200:
			mame_timer_enable(mfp_p->timer_a, 0);
			break;
		}

		if (mfp_p->tacr & MFP68901_TCR_TIMER_RESET)
		{
			mfp_p->tao = 0;

			if (mfp_p->tao_w)
			{
				mfp_p->tao_w(which, mfp_p->tao);
			}
		}
		break;
	case 0x0d:
		mfp_p->tbcr = data & 0x1f;

		switch (mfp_p->tbcr & 0x0f)
		{
		case MFP68901_TCR_TIMER_DELAY_4:
			mame_timer_adjust(mfp_p->timer_b, time_zero, 0, MAME_TIME_IN_HZ(mfp_p->timer_clock / 4));
			break;
		case MFP68901_TCR_TIMER_DELAY_10:
			mame_timer_adjust(mfp_p->timer_b, time_zero, 0, MAME_TIME_IN_HZ(mfp_p->timer_clock / 10));
			break;
		case MFP68901_TCR_TIMER_DELAY_16:
			mame_timer_adjust(mfp_p->timer_b, time_zero, 0, MAME_TIME_IN_HZ(mfp_p->timer_clock / 16));
			break;
		case MFP68901_TCR_TIMER_DELAY_50:
			mame_timer_adjust(mfp_p->timer_b, time_zero, 0, MAME_TIME_IN_HZ(mfp_p->timer_clock / 50));
			break;
		case MFP68901_TCR_TIMER_DELAY_64:
			mame_timer_adjust(mfp_p->timer_b, time_zero, 0, MAME_TIME_IN_HZ(mfp_p->timer_clock / 64));
			break;
		case MFP68901_TCR_TIMER_DELAY_100:
			mame_timer_adjust(mfp_p->timer_b, time_zero, 0, MAME_TIME_IN_HZ(mfp_p->timer_clock / 100));
			break;
		case MFP68901_TCR_TIMER_DELAY_200:
			mame_timer_adjust(mfp_p->timer_b, time_zero, 0, MAME_TIME_IN_HZ(mfp_p->timer_clock / 200));
			break;
		case MFP68901_TCR_TIMER_STOPPED:
		case MFP68901_TCR_TIMER_EVENT:
		case MFP68901_TCR_TIMER_PULSE_4:
		case MFP68901_TCR_TIMER_PULSE_10:
		case MFP68901_TCR_TIMER_PULSE_16:
		case MFP68901_TCR_TIMER_PULSE_50:
		case MFP68901_TCR_TIMER_PULSE_64:
		case MFP68901_TCR_TIMER_PULSE_100:
		case MFP68901_TCR_TIMER_PULSE_200:
			mame_timer_enable(mfp_p->timer_b, 0);
			break;
		}

		if (mfp_p->tbcr & MFP68901_TCR_TIMER_RESET)
		{
			mfp_p->tbo = 0;

			if (mfp_p->tbo_w)
			{
				mfp_p->tbo_w(which, mfp_p->tbo);
			}
		}
		break;
	case 0x0e: 
		mfp_p->tcdcr = data & 0x6f;

		switch (mfp_p->tcdcr & 0x07)
		{
		case MFP68901_TCR_TIMER_DELAY_4:
			mame_timer_adjust(mfp_p->timer_d, time_zero, 0, MAME_TIME_IN_HZ(mfp_p->timer_clock / 4));
			break;
		case MFP68901_TCR_TIMER_DELAY_10:
			mame_timer_adjust(mfp_p->timer_d, time_zero, 0, MAME_TIME_IN_HZ(mfp_p->timer_clock / 10));
			break;
		case MFP68901_TCR_TIMER_DELAY_16:
			mame_timer_adjust(mfp_p->timer_d, time_zero, 0, MAME_TIME_IN_HZ(mfp_p->timer_clock / 16));
			break;
		case MFP68901_TCR_TIMER_DELAY_50:
			mame_timer_adjust(mfp_p->timer_d, time_zero, 0, MAME_TIME_IN_HZ(mfp_p->timer_clock / 50));
			break;
		case MFP68901_TCR_TIMER_DELAY_64:
			mame_timer_adjust(mfp_p->timer_d, time_zero, 0, MAME_TIME_IN_HZ(mfp_p->timer_clock / 64));
			break;
		case MFP68901_TCR_TIMER_DELAY_100:
			mame_timer_adjust(mfp_p->timer_d, time_zero, 0, MAME_TIME_IN_HZ(mfp_p->timer_clock / 100));
			break;
		case MFP68901_TCR_TIMER_DELAY_200:
			mame_timer_adjust(mfp_p->timer_d, time_zero, 0, MAME_TIME_IN_HZ(mfp_p->timer_clock / 200));
			break;
		case MFP68901_TCR_TIMER_STOPPED:
			mame_timer_enable(mfp_p->timer_d, 0);
			break;
		}

		switch ((mfp_p->tcdcr >> 4) & 0x07)
		{
		case MFP68901_TCR_TIMER_DELAY_4:
			mame_timer_adjust(mfp_p->timer_c, time_zero, 0, MAME_TIME_IN_HZ(mfp_p->timer_clock / 4));
			break;
		case MFP68901_TCR_TIMER_DELAY_10:
			mame_timer_adjust(mfp_p->timer_c, time_zero, 0, MAME_TIME_IN_HZ(mfp_p->timer_clock / 10));
			break;
		case MFP68901_TCR_TIMER_DELAY_16:
			mame_timer_adjust(mfp_p->timer_c, time_zero, 0, MAME_TIME_IN_HZ(mfp_p->timer_clock / 16));
			break;
		case MFP68901_TCR_TIMER_DELAY_50:
			mame_timer_adjust(mfp_p->timer_c, time_zero, 0, MAME_TIME_IN_HZ(mfp_p->timer_clock / 50));
			break;
		case MFP68901_TCR_TIMER_DELAY_64:
			mame_timer_adjust(mfp_p->timer_c, time_zero, 0, MAME_TIME_IN_HZ(mfp_p->timer_clock / 64));
			break;
		case MFP68901_TCR_TIMER_DELAY_100:
			mame_timer_adjust(mfp_p->timer_c, time_zero, 0, MAME_TIME_IN_HZ(mfp_p->timer_clock / 100));
			break;
		case MFP68901_TCR_TIMER_DELAY_200:
			mame_timer_adjust(mfp_p->timer_c, time_zero, 0, MAME_TIME_IN_HZ(mfp_p->timer_clock / 200));
			break;
		case MFP68901_TCR_TIMER_STOPPED:
			mame_timer_enable(mfp_p->timer_c, 0);
			break;
		}
		break;
	case 0x0f:
		mfp_p->tadr = data;

		if (!mame_timer_enabled(mfp_p->timer_a))
		{
			mfp_p->tarr = data;
		}
		break;
	case 0x10:
		mfp_p->tbdr = data;

		if (!mame_timer_enabled(mfp_p->timer_b))
		{
			mfp_p->tbrr = data;
		}
		break;
	case 0x11:
		mfp_p->tcdr = data;

		if (!mame_timer_enabled(mfp_p->timer_c))
		{
			mfp_p->tcrr = data;
		}
		break;
	case 0x12:
		mfp_p->tddr = data;

		if (!mame_timer_enabled(mfp_p->timer_d))
		{
			mfp_p->tdrr = data;
		}
		break;

	// USART not implemented
	case 0x13: mfp_p->scr = data;
		break;
	case 0x14: mfp_p->ucr = data;
		break;
	case 0x15: mfp_p->rsr = data;
		break;
	case 0x16: mfp_p->tsr = data;
		break;
	case 0x17: mfp_p->udr = data;
		break;
	}
}

void mfp68901_config(int which, const struct mfp68901_interface *intf)
{
	mfp_68901 *mfp_p = &mfp[which];

	if (which >= MAX_MFP)
	{
		return;
	}

	mfp_p->chip_clock = intf->chip_clock;
	mfp_p->timer_clock = intf->timer_clock;
	mfp_p->irq_callback = intf->irq_callback;
	mfp_p->gpio_r = intf->gpio_r;
	mfp_p->gpio_w = intf->gpio_w;
	
	mfp_p->timer_a = mame_timer_alloc(timer_a);
	mfp_p->timer_b = mame_timer_alloc(timer_b);
	mfp_p->timer_c = mame_timer_alloc(timer_c);
	mfp_p->timer_d = mame_timer_alloc(timer_d);

	mame_timer_reset(mfp_p->timer_a, time_never);
	mame_timer_reset(mfp_p->timer_b, time_never);
	mame_timer_reset(mfp_p->timer_c, time_never);
	mame_timer_reset(mfp_p->timer_d, time_never);

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
	state_save_register_item("mfp68901", which, mfp_p->tadr);
	state_save_register_item("mfp68901", which, mfp_p->tbdr);
	state_save_register_item("mfp68901", which, mfp_p->tcdr);
	state_save_register_item("mfp68901", which, mfp_p->tddr);
	state_save_register_item("mfp68901", which, mfp_p->tarr);
	state_save_register_item("mfp68901", which, mfp_p->tbrr);
	state_save_register_item("mfp68901", which, mfp_p->tcrr);
	state_save_register_item("mfp68901", which, mfp_p->tdrr);
	state_save_register_item("mfp68901", which, mfp_p->tao);
	state_save_register_item("mfp68901", which, mfp_p->tbo);
	state_save_register_item("mfp68901", which, mfp_p->tco);
	state_save_register_item("mfp68901", which, mfp_p->tdo);
	state_save_register_item("mfp68901", which, mfp_p->tai);
	state_save_register_item("mfp68901", which, mfp_p->tbi);
	state_save_register_item("mfp68901", which, mfp_p->scr);
	state_save_register_item("mfp68901", which, mfp_p->ucr);
	state_save_register_item("mfp68901", which, mfp_p->rsr);
	state_save_register_item("mfp68901", which, mfp_p->tsr);
	state_save_register_item("mfp68901", which, mfp_p->udr);
	state_save_register_item("mfp68901", which, mfp_p->chip_clock);
	state_save_register_item("mfp68901", which, mfp_p->timer_clock);
}

void mfp68901_timer_count_a(int which)
{
	mfp_68901 *mfp_p = &mfp[which];

	if (mfp_p->tarr == 0x01)
	{
		if (mfp_p->iera & 0x20)
		{
			mfp_p->ipra |= 0x20;

			if (mfp_p->imra & 0x20)
			{
				mfp68901_irq_ack(which);
				
				if (mfp_p->tao)
				{
					mfp_p->tao = 0;
				}
				else
				{
					mfp_p->tao = 1;
				}

				if (mfp_p->tao_w)
				{
					mfp_p->tao_w(which, mfp_p->tao);
				}
			}
		}

		mfp_p->tarr = mfp_p->tadr;
	}
	else
	{
		mfp_p->tarr--;
	}
}

void mfp68901_timer_count_b(int which)
{
	mfp_68901 *mfp_p = &mfp[which];

	if (mfp_p->tbrr == 0x01)
	{
		if (mfp_p->iera & 0x01)
		{
			mfp_p->ipra |= 0x01;

			if (mfp_p->imra & 0x01)
			{
				mfp68901_irq_ack(which);
				
				if (mfp_p->tbo)
				{
					mfp_p->tbo = 0;
				}
				else
				{
					mfp_p->tbo = 1;
				}

				if (mfp_p->tbo_w)
				{
					mfp_p->tbo_w(which, mfp_p->tbo);
				}
			}
		}

		mfp_p->tbrr = mfp_p->tbdr;
	}
	else
	{
		mfp_p->tbrr--;
	}
}

void mfp68901_timer_count_c(int which)
{
	mfp_68901 *mfp_p = &mfp[which];

	if (mfp_p->tcrr == 0x01)
	{
		if (mfp_p->ierb & 0x20)
		{
			mfp_p->iprb |= 0x20;

			if (mfp_p->imrb & 0x20)
			{
				mfp68901_irq_ack(which);
				
				if (mfp_p->tco)
				{
					mfp_p->tco = 0;
				}
				else
				{
					mfp_p->tco = 1;
				}

				if (mfp_p->tco_w)
				{
					mfp_p->tco_w(which, mfp_p->tco);
				}
			}
		}

		mfp_p->tcrr = mfp_p->tcdr;
	}
	else
	{
		mfp_p->tcrr--;
	}
}

void mfp68901_timer_count_d(int which)
{
	mfp_68901 *mfp_p = &mfp[which];

	if (mfp_p->tdrr == 0x01)
	{
		if (mfp_p->ierb & 0x01)
		{
			mfp_p->iprb |= 0x01;

			if (mfp_p->imrb & 0x01)
			{
				mfp68901_irq_ack(which);
				
				if (mfp_p->tdo)
				{
					mfp_p->tdo = 0;
				}
				else
				{
					mfp_p->tdo = 1;
				}

				if (mfp_p->tdo_w)
				{
					mfp_p->tdo_w(which, mfp_p->tdo);
				}
			}
		}

		mfp_p->tdrr = mfp_p->tddr;
	}
	else
	{
		mfp_p->tdrr--;
	}
}

void mfp68901_tai_w(int which, int value)
{
	mfp_68901 *mfp_p = &mfp[which];

	switch (mfp_p->tacr & 0x0f)
	{
	case MFP68901_TCR_TIMER_DELAY_4:
	case MFP68901_TCR_TIMER_DELAY_10:
	case MFP68901_TCR_TIMER_DELAY_16:
	case MFP68901_TCR_TIMER_DELAY_50:
	case MFP68901_TCR_TIMER_DELAY_64:
	case MFP68901_TCR_TIMER_DELAY_100:
	case MFP68901_TCR_TIMER_DELAY_200:
	case MFP68901_TCR_TIMER_STOPPED:
		return;

	case MFP68901_TCR_TIMER_EVENT:
		if (mfp_p->aer & MFP68901_AER_GPIP_4)
		{
			// count down on 0->1 transition
			if (mfp_p->tai == 0 && value == 1)
			{
				mfp_p->tai = value;
				mfp68901_timer_count_a(which);
			}
		}
		else
		{
			// count down on 1->0 transition
			if (mfp_p->tai == 1 && value == 0)
			{
				mfp_p->tai = value;
				mfp68901_timer_count_a(which);
			}
		}
		break;

	case MFP68901_TCR_TIMER_PULSE_4:
	case MFP68901_TCR_TIMER_PULSE_10:
	case MFP68901_TCR_TIMER_PULSE_16:
	case MFP68901_TCR_TIMER_PULSE_50:
	case MFP68901_TCR_TIMER_PULSE_64:
	case MFP68901_TCR_TIMER_PULSE_100:
	case MFP68901_TCR_TIMER_PULSE_200:
		break;
	}

	mfp68901_irq_ack(which);
}

void mfp68901_tbi_w(int which, int value)
{
	mfp_68901 *mfp_p = &mfp[which];

	switch (mfp_p->tbcr & 0x0f)
	{
	case MFP68901_TCR_TIMER_DELAY_4:
	case MFP68901_TCR_TIMER_DELAY_10:
	case MFP68901_TCR_TIMER_DELAY_16:
	case MFP68901_TCR_TIMER_DELAY_50:
	case MFP68901_TCR_TIMER_DELAY_64:
	case MFP68901_TCR_TIMER_DELAY_100:
	case MFP68901_TCR_TIMER_DELAY_200:
	case MFP68901_TCR_TIMER_STOPPED:
		return;

	case MFP68901_TCR_TIMER_EVENT:
		if (mfp_p->aer & MFP68901_AER_GPIP_3)
		{
			// count down on 0->1 transition
			if (mfp_p->tbi == 0 && value == 1)
			{
				mfp_p->tbi = value;
				mfp68901_timer_count_b(which);
			}
		}
		else
		{
			// count down on 1->0 transition
			if (mfp_p->tbi == 1 && value == 0)
			{
				mfp_p->tbi = value;
				mfp68901_timer_count_b(which);
			}
		}
		break;

	case MFP68901_TCR_TIMER_PULSE_4:
	case MFP68901_TCR_TIMER_PULSE_10:
	case MFP68901_TCR_TIMER_PULSE_16:
	case MFP68901_TCR_TIMER_PULSE_50:
	case MFP68901_TCR_TIMER_PULSE_64:
	case MFP68901_TCR_TIMER_PULSE_100:
	case MFP68901_TCR_TIMER_PULSE_200:
		break;
	}

	mfp68901_irq_ack(which);
}

static TIMER_CALLBACK( timer_a )
{
	int which = param;

	mfp68901_timer_count_a(which);
}

static TIMER_CALLBACK( timer_b )
{
	int which = param;

	mfp68901_timer_count_b(which);
}

static TIMER_CALLBACK( timer_c )
{
	int which = param;

	mfp68901_timer_count_c(which);
}

static TIMER_CALLBACK( timer_d )
{
	int which = param;

	mfp68901_timer_count_d(which);
}

READ16_HANDLER( mfp68901_0_register16_r ) { return mfp68901_register_r(0, offset / 2); }
READ16_HANDLER( mfp68901_1_register16_r ) { return mfp68901_register_r(1, offset / 2); }
READ16_HANDLER( mfp68901_2_register16_r ) { return mfp68901_register_r(2, offset / 2); }
READ16_HANDLER( mfp68901_3_register16_r ) { return mfp68901_register_r(3, offset / 2); }

WRITE16_HANDLER( mfp68901_0_register_msb_w ) { if (ACCESSING_MSB) mfp68901_register_w(0, offset / 2, (data >> 8) & 0xff); }
WRITE16_HANDLER( mfp68901_1_register_msb_w ) { if (ACCESSING_MSB) mfp68901_register_w(0, offset / 2, (data >> 8) & 0xff); }
WRITE16_HANDLER( mfp68901_2_register_msb_w ) { if (ACCESSING_MSB) mfp68901_register_w(0, offset / 2, (data >> 8) & 0xff); }
WRITE16_HANDLER( mfp68901_3_register_msb_w ) { if (ACCESSING_MSB) mfp68901_register_w(0, offset / 2, (data >> 8) & 0xff); }

WRITE16_HANDLER( acia6850_0_register_lsb_w ) { if (ACCESSING_LSB) mfp68901_register_w(0, offset / 2, data & 0xff); }
WRITE16_HANDLER( acia6850_1_register_lsb_w ) { if (ACCESSING_LSB) mfp68901_register_w(0, offset / 2, data & 0xff); }
WRITE16_HANDLER( acia6850_2_register_lsb_w ) { if (ACCESSING_LSB) mfp68901_register_w(0, offset / 2, data & 0xff); }
WRITE16_HANDLER( acia6850_3_register_lsb_w ) { if (ACCESSING_LSB) mfp68901_register_w(0, offset / 2, data & 0xff); }
