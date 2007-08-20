/*
    6850 ACIA

    Preliminary and not yet tested outside of bfcobra.c!
*/
#include "driver.h"
#include "timer.h"
#include "6850acia.h"

#define CR1_0	0x03
#define CR4_2	0x1C
#define CR6_5	0x60
#define CR7		0x80

#define STATUS_RDRF	0x01
#define STATUS_TDRE	0x02
#define STATUS_DCD	0x04
#define STATUS_CTS	0x08
#define STATUS_FE	0x10
#define STATUS_OVRN	0x20
#define STATUS_PE	0x40
#define STATUS_IRQ	0x80

enum serial_state
{
	START,
	DATA,
	PARITY,
	STOP,
	COMPLETE
};

enum parity_type
{
	NONE,
	ODD,
	EVEN
};

static const int div_type[3] = { 1, 16, 64 };

static const int wordtype [8][3] =
{
	{7, EVEN, 2},
	{7, ODD, 2},
	{7, EVEN, 1},
	{7, ODD, 1},
	{8, NONE, 2},
	{8, NONE, 1},
	{8, EVEN, 1},
	{8, ODD, 1}
};

typedef struct _acia_6850_
{
	UINT8	ctrl;
	UINT8	status;

	UINT8	*rx_pin;
	UINT8	*tx_pin;

	UINT8	tdr;
	UINT8	rdr;
	UINT8	rx_shift;
	UINT8	tx_shift;

	int	rx_clock;
	int	tx_clock;

	int	divide;

	/* Counters */
	int	tx_bits;
	int	rx_bits;
	int	tx_parity;
	int	rx_parity;

	/* TX/RX state */
	int	bits;
	enum 	parity_type parity;
	int	stopbits;

	/* Signals */
	int	overrun;
	int	reset;
	enum 	serial_state rx_state;
	enum 	serial_state tx_state;

	mame_timer *rx_timer;
	mame_timer *tx_timer;

	void (*int_callback)(int param);
} acia_6850;

static acia_6850 acia[MAX_ACIA];

static TIMER_CALLBACK( receive_event );
static TIMER_CALLBACK( transmit_event );

/*
    Reset the chip
*/
static void reset(int which)
{
	acia_6850 *acia_p = &acia[which];

	acia_p->status = STATUS_TDRE;
	acia_p->tdr = 0;
	acia_p->rdr = 0;
	acia_p->tx_shift = 0;
	acia_p->rx_shift = 0;
	*acia_p->tx_pin = 1;

	acia_p->tx_state = START;
	acia_p->rx_state = START;
}

/*
    Called by drivers
*/
void acia6850_config(int which, const struct acia6850_interface *intf)
{
	acia_6850 *acia_p = &acia[which];

	if (which >= MAX_ACIA)
	{
		return;
	}

	acia_p->rx_clock = intf->rx_clock;
	acia_p->tx_clock = intf->tx_clock;
	acia_p->rx_pin = intf->rx_pin;
	acia_p->tx_pin = intf->tx_pin;
	acia_p->rx_timer = mame_timer_alloc(receive_event);
	acia_p->tx_timer = mame_timer_alloc(transmit_event);
	acia_p->int_callback = intf->int_callback;

	mame_timer_reset(acia_p->rx_timer, time_never);
	mame_timer_reset(acia_p->tx_timer, time_never);

	// TODO
	*acia_p->tx_pin = 1;

	state_save_register_item("acia6850", which, acia_p->ctrl);
	state_save_register_item("acia6850", which, acia_p->status);
	state_save_register_item("acia6850", which, acia_p->rx_clock);
	state_save_register_item("acia6850", which, acia_p->tx_clock);
	state_save_register_item("acia6850", which, acia_p->rx_shift);
	state_save_register_item("acia6850", which, acia_p->tx_shift);
	state_save_register_item("acia6850", which, acia_p->rdr);
	state_save_register_item("acia6850", which, acia_p->tdr);
	state_save_register_item("acia6850", which, acia_p->rx_bits);
	state_save_register_item("acia6850", which, acia_p->tx_bits);
	state_save_register_item("acia6850", which, acia_p->rx_parity);
	state_save_register_item("acia6850", which, acia_p->tx_parity);

	state_save_register_item("acia6850", which, acia_p->divide);
	state_save_register_item("acia6850", which, acia_p->overrun);
	state_save_register_item("acia6850", which, acia_p->reset);
}

/*
    Read Status Register
*/
UINT8 acia6850_stat_r(int which)
{
	return acia[which].status;
}

/*
    Write Control Register
*/
void acia6850_ctrl_w(int which, UINT8 data)
{
	int wordsel;
	int divide;
	acia_6850 *acia_p = &acia[which];

	acia_p->ctrl = data;

	/* 1. Counter divide select */
	divide = data & CR1_0;

	if (divide == 3)
	{
		acia_p->reset = 1;
		reset(which);
	}
	else
	{
		acia_p->reset = 0;
		acia_p->divide = div_type[divide];
	}

	/* 2. Word type */
	wordsel = (data & CR4_2) >> 2;
	acia_p->bits = wordtype[wordsel][0];
	acia_p->parity = wordtype[wordsel][1];
	acia_p->stopbits = wordtype[wordsel][2];

	/* 3. TX Control (TODO) */

	/* After writing the word type, start receive clock */
	if(!acia_p->reset)
	{
		mame_time period = scale_up_mame_time(MAME_TIME_IN_HZ(acia_p->rx_clock), acia_p->divide);
		/* TODO! */
		mame_timer_adjust(acia_p->rx_timer, period, which, period);
	}
}

static TIMER_CALLBACK( tdr_to_shift )
{
		int which = param;

		acia_6850 *acia_p = &acia[which];
		mame_time period = scale_up_mame_time(MAME_TIME_IN_HZ(acia_p->tx_clock), acia_p->divide);

		acia_p->tx_shift = acia[which].tdr;
		acia_p->status &= ~STATUS_TDRE;

		/* Start the transmit timer */
		mame_timer_adjust(acia_p->tx_timer, period, which, period);
}


/*
    Write transmit register
*/
void acia6850_data_w(int which, UINT8 data)
{
	if (!acia[which].reset)
	{
		acia[which].tdr = data;
		timer_call_after_resynch(which, tdr_to_shift);
		//printf("ACIA %d Transmit: %x (%c)\n", which, data, data);
	}
	else
	{
		logerror("ACIA %d: Data write while in reset! (%x)\n", which, activecpu_get_previouspc());
	}
}

/*
    Read character
*/
UINT8 acia6850_data_r(int which)
{
	acia[which].status &= ~(STATUS_RDRF | STATUS_IRQ | STATUS_PE | STATUS_OVRN);

	if (acia[which].int_callback)
		acia[which].int_callback(0);

	return acia[which].rdr;
}


/*
    Transmit a bit
*/
static TIMER_CALLBACK( transmit_event )
{
	int which = param;
	acia_6850 *acia_p = &acia[which];

	switch (acia_p->tx_state)
	{
		case START:
		{
			/* Stop bit */
			*acia_p->tx_pin = 0;
			acia_p->tx_bits = acia_p->bits;
			acia_p->tx_state = DATA;
			break;
		}
		case DATA:
		{
			int val = acia_p->tx_shift & 1;

			*acia_p->tx_pin = val;
			acia_p->tx_parity ^= val;
			acia_p->tx_shift >>= 1;

			if (--(acia_p->tx_bits) == 0)
			{
				acia_p->tx_state = acia_p->parity == NONE ? STOP : PARITY;
			}

			break;
		}
		case PARITY:
		{
			if (acia_p->parity == EVEN)
			{
				*acia_p->tx_pin = (acia_p->tx_parity & 1) ? 1 : 0;
			}
			else
			{
				*acia_p->tx_pin = (acia_p->tx_parity & 1) ? 0 : 1;
			}

			acia_p->tx_parity = 0;
			acia_p->tx_state = STOP;
			break;
		}
		case STOP:
		{
			/* TODO */
			*acia_p->tx_pin = 1;
			acia_p->tx_state = COMPLETE;
			break;
		}
		case COMPLETE:
		{
			/* TDR is now empty */
			acia_p->status |= STATUS_TDRE;

			/* Transmit timer no longer active */
			mame_timer_enable(acia_p->tx_timer, 0);

			acia_p->tx_state = START;
			break;
		}
	}
}

/* Called on receive timer event */
static TIMER_CALLBACK( receive_event )
{
	int which = param;
	acia_6850 *acia_p = &acia[which];

	switch (acia_p->rx_state)
	{
		case START:
		{
			/* Wait for low signal */
			if (*acia_p->rx_pin == 0)
			{
				acia_p->rx_shift = 0;
				acia_p->rx_bits = acia_p->bits;
				acia_p->rx_state = DATA;
			}
			break;
		}
		case DATA:
		{
			acia_p->rx_shift |= *acia_p->rx_pin ? 0x80 : 0;
			acia_p->rx_parity ^= *acia_p->rx_pin;

			if (--acia_p->rx_bits == 0)
			{
				acia_p->rx_state = acia_p->parity == NONE ? STOP : PARITY;
			}
			else
			{
				acia_p->rx_shift >>= 1;
			}
			break;
		}
		case PARITY:
		{
			acia_p->rx_parity ^= *acia_p->rx_pin;

			if (acia_p->parity == EVEN)
			{
				if (acia_p->rx_parity)
				{
					acia_p->status |= STATUS_PE;
				}
			}
			else
			{
				if (!acia_p->rx_parity)
				{
					acia_p->status |= STATUS_PE;
				}
			}

			acia_p->rx_parity = 0;
			acia_p->rx_state = STOP;

			// As long as data is in RDR
			break;
		}
		case STOP:
		{
			/* TODO:
                Multiple stop bits
                Overrun
            */
			acia_p->rx_state = COMPLETE;
			break;
		}
		case COMPLETE:
		{
			/*
                TODO: Check behaviour.
            */
			//if (!(acia_p->status & STATUS_RDRF))
			{
				acia_p->rdr = acia_p->rx_shift;
				acia_p->status |= STATUS_RDRF;
				acia_p->status |= STATUS_IRQ;

//              printf("ACIA %d Received full: %x\n", which, acia_p->rdr);

				/* TODO */
				if (acia_p->ctrl & 0x80)
				{
					if (acia_p->int_callback)
					{
						acia_p->int_callback(1);
					}
				}
			}

			acia_p->rx_state = START;
			break;
		}
	}
}

WRITE8_HANDLER( acia6850_0_ctrl_w ) { acia6850_ctrl_w(0, data); }
WRITE8_HANDLER( acia6850_1_ctrl_w ) { acia6850_ctrl_w(1, data); }
WRITE8_HANDLER( acia6850_2_ctrl_w ) { acia6850_ctrl_w(2, data); }
WRITE8_HANDLER( acia6850_3_ctrl_w ) { acia6850_ctrl_w(3, data); }

WRITE8_HANDLER( acia6850_0_data_w ) { acia6850_data_w(0, data); }
WRITE8_HANDLER( acia6850_1_data_w ) { acia6850_data_w(1, data); }
WRITE8_HANDLER( acia6850_2_data_w ) { acia6850_data_w(2, data); }
WRITE8_HANDLER( acia6850_3_data_w ) { acia6850_data_w(3, data); }

READ8_HANDLER( acia6850_0_stat_r ) { return acia6850_stat_r(0); }
READ8_HANDLER( acia6850_1_stat_r ) { return acia6850_stat_r(1); }
READ8_HANDLER( acia6850_2_stat_r ) { return acia6850_stat_r(2); }
READ8_HANDLER( acia6850_3_stat_r ) { return acia6850_stat_r(3); }

READ8_HANDLER( acia6850_0_data_r ) { return acia6850_data_r(0); }
READ8_HANDLER( acia6850_1_data_r ) { return acia6850_data_r(1); }
READ8_HANDLER( acia6850_2_data_r ) { return acia6850_data_r(2); }
READ8_HANDLER( acia6850_3_data_r ) { return acia6850_data_r(3); }

READ16_HANDLER( acia6850_0_stat_lsb_r ) { return acia6850_stat_r(0); }
READ16_HANDLER( acia6850_1_stat_lsb_r ) { return acia6850_stat_r(1); }
READ16_HANDLER( acia6850_2_stat_lsb_r ) { return acia6850_stat_r(2); }
READ16_HANDLER( acia6850_3_stat_lsb_r ) { return acia6850_stat_r(3); }

READ16_HANDLER( acia6850_0_stat_msb_r ) { return acia6850_stat_r(0) << 8; }
READ16_HANDLER( acia6850_1_stat_msb_r ) { return acia6850_stat_r(1) << 8; }
READ16_HANDLER( acia6850_2_stat_msb_r ) { return acia6850_stat_r(2) << 8; }
READ16_HANDLER( acia6850_3_stat_msb_r ) { return acia6850_stat_r(3) << 8; }

READ16_HANDLER( acia6850_0_data_lsb_r ) { return acia6850_data_r(0); }
READ16_HANDLER( acia6850_1_data_lsb_r ) { return acia6850_data_r(1); }
READ16_HANDLER( acia6850_2_data_lsb_r ) { return acia6850_data_r(2); }
READ16_HANDLER( acia6850_3_data_lsb_r ) { return acia6850_data_r(3); }

READ16_HANDLER( acia6850_0_data_msb_r ) { return acia6850_data_r(0) << 8; }
READ16_HANDLER( acia6850_1_data_msb_r ) { return acia6850_data_r(1) << 8; }
READ16_HANDLER( acia6850_2_data_msb_r ) { return acia6850_data_r(2) << 8; }
READ16_HANDLER( acia6850_3_data_msb_r ) { return acia6850_data_r(3) << 8; }

WRITE16_HANDLER( acia6850_0_ctrl_msb_w ) { if (ACCESSING_MSB) acia6850_ctrl_w(0, (data >> 8) & 0xff); }
WRITE16_HANDLER( acia6850_1_ctrl_msb_w ) { if (ACCESSING_MSB) acia6850_ctrl_w(1, (data >> 8) & 0xff); }
WRITE16_HANDLER( acia6850_2_ctrl_msb_w ) { if (ACCESSING_MSB) acia6850_ctrl_w(2, (data >> 8) & 0xff); }
WRITE16_HANDLER( acia6850_3_ctrl_msb_w ) { if (ACCESSING_MSB) acia6850_ctrl_w(3, (data >> 8) & 0xff); }

WRITE16_HANDLER( acia6850_0_ctrl_lsb_w ) { if (ACCESSING_LSB) acia6850_ctrl_w(0, data & 0xff); }
WRITE16_HANDLER( acia6850_1_ctrl_lsb_w ) { if (ACCESSING_LSB) acia6850_ctrl_w(1, data & 0xff); }
WRITE16_HANDLER( acia6850_2_ctrl_lsb_w ) { if (ACCESSING_LSB) acia6850_ctrl_w(2, data & 0xff); }
WRITE16_HANDLER( acia6850_3_ctrl_lsb_w ) { if (ACCESSING_LSB) acia6850_ctrl_w(3, data & 0xff); }

WRITE16_HANDLER( acia6850_0_data_msb_w ) { if (ACCESSING_MSB) acia6850_data_w(0, (data >> 8) & 0xff); }
WRITE16_HANDLER( acia6850_1_data_msb_w ) { if (ACCESSING_MSB) acia6850_data_w(1, (data >> 8) & 0xff); }
WRITE16_HANDLER( acia6850_2_data_msb_w ) { if (ACCESSING_MSB) acia6850_data_w(2, (data >> 8) & 0xff); }
WRITE16_HANDLER( acia6850_3_data_msb_w ) { if (ACCESSING_MSB) acia6850_data_w(3, (data >> 8) & 0xff); }

WRITE16_HANDLER( acia6850_0_data_lsb_w ) { if (ACCESSING_LSB) acia6850_data_w(0, data & 0xff); }
WRITE16_HANDLER( acia6850_1_data_lsb_w ) { if (ACCESSING_LSB) acia6850_data_w(1, data & 0xff); }
WRITE16_HANDLER( acia6850_2_data_lsb_w ) { if (ACCESSING_LSB) acia6850_data_w(2, data & 0xff); }
WRITE16_HANDLER( acia6850_3_data_lsb_w ) { if (ACCESSING_LSB) acia6850_data_w(3, data & 0xff); }
