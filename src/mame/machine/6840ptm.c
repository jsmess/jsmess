/**********************************************************************


    Motorola 6840 PTM interface and emulation

    This function is a simple emulation of up to 4 MC6840
    Programmable Timer Modules

    Written By El Condor based on previous work by Aaron Giles,
   'Re-Animator' and Mathis Rosenhauer.

    Todo:
         Confirm handling for 'Single Shot' operation.
         (Datasheet suggests that output starts high, going low
         on timeout, opposite of continuous case)
         Establish whether ptm6840_set_c? routines can replace
         hard coding of external clock frequencies.


    Operation:
    The interface is arranged as follows:

    Internal Clock frequency,
    Clock 1 frequency, Clock 2 frequency, Clock 3 frequency,
    Clock 1 output, Clock 2 output, Clock 3 output,
    IRQ function

    If the external clock frequencies are not fixed, they should be
    entered as '0', and the ptm6840_set_c?(which, state) functions
    should be used instead if necessary (This should allow the VBLANK
    clock on the MCR units to operate).

**********************************************************************/

#include "driver.h"
#include "timer.h"
#include "6840ptm.h"

#ifdef MAME_DEBUG
#define PTMVERBOSE 1
#else
#define PTMVERBOSE 0
#endif

#if PTMVERBOSE
#define PLOG(x)	logerror x
#else
#define PLOG(x)
#endif

#define PTM_6840_CTRL1   0
#define PTM_6840_CTRL2   1
#define PTM_6840_MSBBUF1 2
#define PTM_6840_LSB1	 3
#define PTM_6840_MSBBUF2 4
#define PTM_6840_LSB2    5
#define PTM_6840_MSBBUF3 6
#define PTM_6840_LSB3    7

typedef struct _ptm6840 ptm6840;
struct _ptm6840
{
	const ptm6840_interface *intf;

	UINT8	 control_reg[3],
				  output[3],// output states
					gate[3],// input  gate states
				   clock[3],// clock  states
			     enabled[3],
					mode[3],
				   fired[3],
			     t3_divisor,
			     		IRQ,
				 status_reg,
	  status_read_since_int,
				 lsb_buffer,
				 msb_buffer;

	   double internal_freq,
		   external_freq[3];

	// each PTM has 3 timers
		mame_timer	*timer1,
					*timer2,
					*timer3;

		UINT16     latch[3],
				 counter[3];
};

// local prototypes ///////////////////////////////////////////////////////

static void ptm6840_t1_timeout(int which);
static void ptm6840_t2_timeout(int which);
static void ptm6840_t3_timeout(int which);

// local vars /////////////////////////////////////////////////////////////

static ptm6840 ptm[PTM_6840_MAX];

#if PTMVERBOSE
static const char *opmode[] =
{
	"000 continous mode",
	"001 freq comparison mode",
	"010 continous mode",
	"011 pulse width comparison mode",
	"100 single shot mode",
	"101 freq comparison mode",
	"110 single shot mode",
	"111 pulse width comparison mode"
};
#endif

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// Get enabled status                                                    //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

int ptm6840_get_status(int which, int clock)
{
	ptm6840 *p = ptm + which;
	return p->enabled[clock-1];
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// Subtract from Counter                                                 //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

static void subtract_from_counter(int counter, int count, int which)
{
	double freq;
	ptm6840 *currptr = ptm + which;

	/* determine the clock frequency for this timer */
	if (currptr->control_reg[counter] & 0x02)
		{
			freq = TIME_IN_HZ(currptr->internal_freq);
		}
	else
		{
			freq = TIME_IN_HZ(currptr->external_freq[counter]);
		}

	/* dual-byte mode */
	if (currptr->control_reg[counter] & 0x04)
	{
		int lsb = currptr->counter[counter] & 0xff;
		int msb = currptr->counter[counter] >> 8;

		/* count the clocks */
		lsb -= count;

		/* loop while we're less than zero */
		while (lsb < 0)
		{
			/* borrow from the MSB */
			lsb += (currptr->latch[counter] & 0xff) + 1;
			msb--;
			/* if MSB goes less than zero, we've expired */
			if (msb < 0)
			{

				switch (counter)
				{
					case 0:
					ptm6840_t1_timeout(which);
					msb = (currptr->latch[counter] >> 8) + 1;
					case 1:
					ptm6840_t2_timeout(which);
					msb = (currptr->latch[counter] >> 8) + 1;
					case 2:
					ptm6840_t3_timeout(which);
					msb = (currptr->latch[counter] >> 8) + 1;
				}
			}
		}

		/* store the result */
		currptr->counter[counter] = (msb << 8) | lsb;
		switch (counter)
		{
			case 0:
			timer_adjust(currptr->timer1, TIME_IN_HZ(freq/((double)currptr->counter[0])), which, 0);
			case 1:
			timer_adjust(currptr->timer2, TIME_IN_HZ(freq/((double)currptr->counter[1])), which, 0);
			case 2:
			timer_adjust(currptr->timer3, TIME_IN_HZ(freq/((double)currptr->counter[2])), which, 0);
		}
	}

	/* word mode */
	else
	{
		int word = currptr->counter[counter];

		/* count the clocks */
		word -= count;

		/* loop while we're less than zero */
		while (word < 0)
		{
			/* borrow from the MSB */
			word += currptr->latch[counter] + 1;

			/* we've expired */
			switch (counter)
			{
				case 0:
				ptm6840_t1_timeout(which);
				case 1:
				ptm6840_t2_timeout(which);
				case 2:
				ptm6840_t3_timeout(which);
			}
		}

		/* store the result */
		currptr->counter[counter] = word;
		switch (counter)
		{
			case 0:
			timer_adjust(currptr->timer1, TIME_IN_HZ(freq/((double)currptr->counter[0])), which, 0);
			case 1:
			timer_adjust(currptr->timer2, TIME_IN_HZ(freq/((double)currptr->counter[1])), which, 0);
			case 2:
			timer_adjust(currptr->timer3, (TIME_IN_HZ(freq/((double)currptr->counter[2])))* currptr->t3_divisor, which, 0);
		}
	}
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// Update Internal Interrupts                                            //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

INLINE void update_interrupts(int which)
{
	ptm6840 *currptr = ptm + which;
	currptr->status_reg &= ~0x80;

	if ((currptr->status_reg & 0x01) && (currptr->control_reg[0] & 0x40)) currptr->status_reg |= 0x80;
	if ((currptr->status_reg & 0x02) && (currptr->control_reg[1] & 0x40)) currptr->status_reg |= 0x80;
	if ((currptr->status_reg & 0x04) && (currptr->control_reg[2] & 0x40)) currptr->status_reg |= 0x80;

	currptr->IRQ = currptr->status_reg >> 7;

	if ( currptr->intf->irq_func )
	{
		currptr->intf->irq_func(currptr->IRQ);
	}
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// Compute Counter                                                       //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

static UINT16 compute_counter(int counter, int which)
{
	ptm6840 *currptr = ptm + which;

	double freq;
	int remaining=0;

	/* if there's no timer, return the count */
	if (!currptr->enabled[counter])

	return currptr->counter[counter];

	/* determine the clock frequency for this timer */
	if (currptr->control_reg[counter] & 0x02)
	{
		freq = TIME_IN_HZ(currptr->internal_freq);
		PLOG(("MC6840 #%d: %d internal clock freq %lf \n", which,counter,freq));
	}
	else
	{
		freq = TIME_IN_HZ(currptr->external_freq[counter]);
		PLOG(("MC6840 #%d: %d external clock freq %lf \n", which,counter,freq));
	}
	/* see how many are left */
	switch (counter)
	{
		case 0:
		remaining = (int)(timer_timeleft(currptr->timer1) / freq);
		case 1:
		remaining = (int)(timer_timeleft(currptr->timer2) / freq);
		case 2:
		remaining = (int)(timer_timeleft(currptr->timer3) / freq);
	}

	/* adjust the count for dual byte mode */
	if (currptr->control_reg[counter] & 0x04)
	{
		int divisor = (currptr->counter[counter] & 0xff) + 1;
		int msb = remaining / divisor;
		int lsb = remaining % divisor;
		remaining = (msb << 8) | lsb;
	}
	PLOG(("MC6840 #%d: read counter(%d): %d\n", which, counter, remaining));
	return remaining;
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// Reload Counter                                                        //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

static void reload_count(int idx, int which)
{
	double freq;
	int count;
	ptm6840 *currptr = ptm + which;

	/* copy the latched value in */
	currptr->counter[idx] = currptr->latch[idx];

	/* determine the clock frequency for this timer */
	if (currptr->control_reg[idx] & 0x02)
	{
		freq = TIME_IN_HZ(currptr->internal_freq);
		PLOG(("MC6840 #%d: %d internal clock freq %lf \n", which,idx, freq));
	}
	else
	{
		freq = TIME_IN_HZ(currptr->external_freq[idx]);
		PLOG(("MC6840 #%d: %d external clock freq %lf \n", which,idx, freq));
	}

	/* determine the number of clock periods before we expire */
	count = currptr->counter[idx];
	if (currptr->control_reg[idx] & 0x04)
		count = ((count >> 8) + 1) * ((count & 0xff) + 1);
	else
		count = count + 1;

	currptr->fired[idx]=0;

	if ((currptr->mode[idx] == 4)|(currptr->mode[idx] == 6))
	{
		currptr->output[idx] = 1;
		switch (idx)
		{
			case 0:
			if ( currptr->intf->out1_func ) currptr->intf->out1_func(0, currptr->output[0]);
			case 1:
			if ( currptr->intf->out2_func ) currptr->intf->out2_func(0, currptr->output[1]);
			case 2:
			if ( currptr->intf->out3_func ) currptr->intf->out3_func(0, currptr->output[2]);
		}

	}

	/* set the timer */
	PLOG(("MC6840 #%d: reload_count(%d): freq = %lf  count = %d\n", which, idx, freq, count));
	switch (idx)
	{
		case 0:
		timer_adjust(currptr->timer1, TIME_IN_HZ(freq/((double)count)), which, 0);
		PLOG(("MC6840 #%d: reload_count(%d): output = %lf\n", which, idx, freq/((double)count)));

		if (!currptr->control_reg[0] & 0x02)
		{
			if (!currptr->intf->external_clock1)
			{
				currptr->enabled[0] = 0;
				timer_enable(currptr->timer1,FALSE);
			}
		}
		else
		{
			currptr->enabled[0] = 1;
			timer_enable(currptr->timer1,TRUE);
		}
		break;
		case 1:
		timer_adjust(currptr->timer2, TIME_IN_HZ(freq/((double)count)), which, 0);
		PLOG(("MC6840 #%d: reload_count(%d): output = %lf\n", which, idx, freq/((double)count)));

		if (!currptr->control_reg[1] & 0x02)
		{
			if (!currptr->intf->external_clock2)
			{
				currptr->enabled[1] = 0;
				timer_enable(currptr->timer2,FALSE);
			}
		}
		else
		{
			currptr->enabled[1] = 1;
			timer_enable(currptr->timer2,TRUE);
		}
		break;
		case 2:
		timer_adjust(currptr->timer3, (TIME_IN_HZ(freq/((double)count)))*currptr->t3_divisor, which, 0);
		PLOG(("MC6840 #%d: reload_count(%d): output = %lf\n", which, idx, (freq/((double)count))*currptr->t3_divisor));

		if (!currptr->control_reg[2] & 0x02)
		{
			if (!currptr->intf->external_clock3)
			{
				currptr->enabled[2] = 0;
				timer_enable(currptr->timer3,FALSE);
			}
		}
		else
		{
			currptr->enabled[2] = 1;
			timer_enable(currptr->timer3,TRUE);
		}

		break;
	}
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// Unconfigure Timer                                                     //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_unconfig(void)
{
	int i;

	i = 0;
	while ( i < PTM_6840_MAX )
	{
		if ( ptm[i].timer1 );
		timer_adjust(ptm[i].timer1, TIME_NEVER, i, 0);
		ptm[i].enabled[0] = 0;
		timer_enable(ptm[i].timer1,FALSE);
		ptm[i].timer1 = NULL;

		if ( ptm[i].timer2 );
		timer_adjust(ptm[i].timer2, TIME_NEVER, i, 0);
		ptm[i].enabled[1] = 0;
		timer_enable(ptm[i].timer2,FALSE);
		ptm[i].timer2 = NULL;

		if ( ptm[i].timer3 );
		timer_adjust(ptm[i].timer3, TIME_NEVER, i, 0);
		ptm[i].enabled[2] = 0;
		timer_enable(ptm[i].timer3,FALSE);
		ptm[i].timer3 = NULL;

		i++;
  	}
	memset (&ptm, 0, sizeof (ptm));
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// Configure Timer                                                       //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_config(int which, const ptm6840_interface *intf)
{
	ptm6840 *currptr = ptm + which;

	assert_always(mame_get_phase(Machine) == MAME_PHASE_INIT, "Can only call ptm6840_config at init time!");
	assert_always((which >= 0) && (which < PTM_6840_MAX), "ptm6840_config called on an invalid PTM!");
	assert_always(intf, "ptm6840_config called with an invalid interface!");
	ptm[which].intf = intf;
	ptm[which].internal_freq = TIME_IN_HZ(currptr->intf->internal_clock);

	if ( currptr->intf->external_clock1 )
	{
		ptm[which].external_freq[0] = TIME_IN_HZ(currptr->intf->external_clock1);
	}
	else
	{
		ptm[which].external_freq[0] = 1;
	}
	if ( currptr->intf->external_clock2 )
	{
		ptm[which].external_freq[1] = TIME_IN_HZ(currptr->intf->external_clock2);
	}
	else
	{
		ptm[which].external_freq[1] = 1;
	}
	if ( currptr->intf->external_clock3 )
	{
		ptm[which].external_freq[2] = TIME_IN_HZ(currptr->intf->external_clock3);
	}
	else
	{
		ptm[which].external_freq[2] = 1;
	}

	ptm[which].timer1 = timer_alloc(ptm6840_t1_timeout);
	ptm[which].timer2 = timer_alloc(ptm6840_t2_timeout);
	ptm[which].timer3 = timer_alloc(ptm6840_t3_timeout);

	timer_enable(ptm[which].timer1, FALSE);
	timer_enable(ptm[which].timer2, FALSE);
	timer_enable(ptm[which].timer3, FALSE);

	state_save_register_item("6840ptm", which, currptr->lsb_buffer);
	state_save_register_item("6840ptm", which, currptr->msb_buffer);
	state_save_register_item("6840ptm", which, currptr->status_read_since_int);
	state_save_register_item("6840ptm", which, currptr->status_reg);
	state_save_register_item("6840ptm", which, currptr->t3_divisor);
	state_save_register_item("6840ptm", which, currptr->internal_freq);
	state_save_register_item("6840ptm", which, currptr->IRQ);

	state_save_register_item("6840ptm", which, currptr->control_reg[0]);
	state_save_register_item("6840ptm", which, currptr->output[0]);
	state_save_register_item("6840ptm", which, currptr->gate[0]);
	state_save_register_item("6840ptm", which, currptr->clock[0]);
	state_save_register_item("6840ptm", which, currptr->mode[0]);
	state_save_register_item("6840ptm", which, currptr->fired[0]);
	state_save_register_item("6840ptm", which, currptr->enabled[0]);
	state_save_register_item("6840ptm", which, currptr->external_freq[0]);
	state_save_register_item("6840ptm", which, currptr->counter[0]);
	state_save_register_item("6840ptm", which, currptr->latch[0]);
	state_save_register_item("6840ptm", which, currptr->control_reg[1]);
	state_save_register_item("6840ptm", which, currptr->output[1]);
	state_save_register_item("6840ptm", which, currptr->gate[1]);
	state_save_register_item("6840ptm", which, currptr->clock[1]);
	state_save_register_item("6840ptm", which, currptr->mode[1]);
	state_save_register_item("6840ptm", which, currptr->fired[1]);
	state_save_register_item("6840ptm", which, currptr->enabled[1]);
	state_save_register_item("6840ptm", which, currptr->external_freq[1]);
	state_save_register_item("6840ptm", which, currptr->counter[1]);
	state_save_register_item("6840ptm", which, currptr->latch[1]);
	state_save_register_item("6840ptm", which, currptr->control_reg[2]);
	state_save_register_item("6840ptm", which, currptr->output[2]);
	state_save_register_item("6840ptm", which, currptr->gate[2]);
	state_save_register_item("6840ptm", which, currptr->clock[2]);
	state_save_register_item("6840ptm", which, currptr->mode[2]);
	state_save_register_item("6840ptm", which, currptr->fired[2]);
	state_save_register_item("6840ptm", which, currptr->enabled[2]);
	state_save_register_item("6840ptm", which, currptr->external_freq[2]);
	state_save_register_item("6840ptm", which, currptr->counter[2]);
	state_save_register_item("6840ptm", which, currptr->latch[2]);

	ptm6840_reset(which);

}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// Reset Timer                                                           //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_reset(int which)
{
	int i;
	ptm[which].control_reg[2]			= 0x00;
	ptm[which].control_reg[1]			= 0x00;
	ptm[which].control_reg[0]			= 0x01;
	ptm[which].status_reg				= 0x00;
	ptm[which].t3_divisor				= 1;

	for ( i = 0; i < 3; i++ )
	{
		ptm[which].status_read_since_int	= 0x00;
		ptm[which].counter[i]				= 0xffff;
		ptm[which].latch[i]					= 0xffff;
		ptm[which].output[i]				= 0;
		ptm[which].fired[i]					= 0;
	}
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// Read Timer                                                            //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

int ptm6840_read(int which, int offset)
{
	int val;
	ptm6840 *currptr = ptm + which;
	switch ( offset )
	{
		case PTM_6840_CTRL1 ://0
		{
			val = 0;
			break;
		}

		case PTM_6840_CTRL2 ://1
		{
			PLOG(("%06X: MC6840 #%d: Status read = %04X\n", activecpu_get_previouspc(), which, currptr->status_reg));
			currptr->status_read_since_int |= currptr->status_reg & 0x07;
			val = currptr->status_reg;
			break;
		}

		case PTM_6840_MSBBUF1://2
		{
			int result = compute_counter(0, which);

			/* clear the interrupt if the status has been read */
			if (currptr->status_read_since_int & (1 << 0))
			{
				currptr->status_reg &= ~(1 << 0);
				update_interrupts(which);
			}

			currptr->lsb_buffer = result & 0xff;

			PLOG(("%06X: MC6840 #%d: Counter %d read = %04X\n", activecpu_get_previouspc(), which, 0, result >> 8));
			val = result >> 8;
			break;
		}

		case PTM_6840_LSB1://3
		{
			val = currptr->lsb_buffer;
			break;
		}

		case PTM_6840_MSBBUF2://4
		{
			int result = compute_counter(1, which);

			/* clear the interrupt if the status has been read */
			if (currptr->status_read_since_int & (1 << 1))
			{
				currptr->status_reg &= ~(1 << 1);
				update_interrupts(which);
			}

			currptr->lsb_buffer = result & 0xff;

			PLOG(("%06X: MC6840 #%d: Counter %d read = %04X\n", activecpu_get_previouspc(), which, 1, result >> 8));
			val = result >> 8;
			break;
		}

		case PTM_6840_LSB2://5
		{
			val = currptr->lsb_buffer;
			break;
		}

		case PTM_6840_MSBBUF3://6
		{
			int result = compute_counter(2, which);

			/* clear the interrupt if the status has been read */
			if (currptr->status_read_since_int & (1 << 2))
			{
				currptr->status_reg &= ~(1 << 2);
				update_interrupts(which);
			}

			currptr->lsb_buffer = result & 0xff;

			PLOG(("%06X: MC6840 #%d: Counter %d read = %04X\n", activecpu_get_previouspc(), which, 2, result >> 8));
			val = result >> 8;
			break;
		}

		case PTM_6840_LSB3://7
		{
			val = currptr->lsb_buffer;
			break;
		}

		default:
		{
			val = 0;
			break;
		}

	}
	return val;
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// Write Timer                                                           //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_write (int which, int offset, int data)
{
	ptm6840 *currptr = ptm + which;

	int idx;
	int i;
	UINT8 diffs;

	switch ( offset )
	{
		case PTM_6840_CTRL1 ://0
		case PTM_6840_CTRL2 ://1
		{
			idx = (offset == 1) ? 1 : (currptr->control_reg[1] & 0x01) ? 0 : 2;
			diffs = data ^ currptr->control_reg[idx];
			currptr->t3_divisor = (currptr->control_reg[3] & 0x01) ? 8 : 1;
			currptr->mode[idx] = (data>>3)&0x07;
			currptr->control_reg[idx] = data;

			PLOG(("MC6840 #%d : Control register %d selected\n",which,idx));
			PLOG(("operation mode   = %s\n", opmode[ currptr->mode[idx] ]));
			PLOG(("value            = %04X\n", currptr->control_reg[idx]));
			PLOG(("t3divisor        = %d\n", currptr->t3_divisor));

			switch ( idx )
			{
				case 0:
				{
					if (!(currptr->control_reg[0] & 0x80 ))
					{ // output cleared
						if ( currptr->intf )
						{
							if ( currptr->intf->out1_func ) currptr->intf->out1_func(0, 0);
						}
					}
				}

				case 1:
				{
					if (!(currptr->control_reg[1] & 0x80 ))
					{ // output cleared
						if ( currptr->intf )
						{
							if ( currptr->intf->out2_func ) currptr->intf->out2_func(0, 0);
						}
					}
				}

				case 2:
				{
					if (!(currptr->control_reg[2] & 0x80 ))
					{ // output cleared
						if ( currptr->intf )
						{
							if ( currptr->intf->out3_func ) currptr->intf->out3_func(0, 0);
						}
					}
				}
			}
			/* reset? */
			if (idx == 0 && (diffs & 0x01))
			{
				/* holding reset down */
				if (data & 0x01)
				{
					PLOG(("MC6840 #%d : Timer reset\n",which));
					timer_enable(currptr->timer1,FALSE);
					timer_enable(currptr->timer2,FALSE);
					timer_enable(currptr->timer3,FALSE);
					currptr->enabled[0]=0;
					currptr->enabled[1]=0;
					currptr->enabled[2]=0;
				}

				/* releasing reset */
				else
				{
					for (i = 0; i < 3; i++)
					{
						reload_count(i,which);
					}
				}

				currptr->status_reg = 0;
				update_interrupts(which);

				/* changing the clock source? (e.g. Zwackery) */
				if (diffs & 0x02)
				reload_count(idx,which);
				break;
			}

			/* offsets 2, 4, and 6 are MSB buffer registers */
			case PTM_6840_MSBBUF1://2
			{
				PLOG(("MC6840 #%d msbbuf1 = %02X\n", which, data));
				currptr->msb_buffer = data;
				break;
			}

			case PTM_6840_MSBBUF2://4
			{
				PLOG(("MC6840 #%d msbbuf2 = %02X\n", which, data));
				currptr->msb_buffer = data;
				break;
			}

			case PTM_6840_MSBBUF3://6
			{
				PLOG(("MC6840 #%d msbbuf3 = %02X\n", which, data));
				currptr->msb_buffer = data;
				break;
			}

			/* offsets 3, 5, and 7 are Write Timer Latch commands */

			case PTM_6840_LSB1://3
			{
				currptr->latch[0] = (currptr->msb_buffer << 8) | (data & 0xff);
				/* clear the interrupt */
				currptr->status_reg &= ~(1 << 0);
				update_interrupts(which);
				/* reload the count if in an appropriate mode */
				if (!(currptr->control_reg[0] & 0x10))
					{
						reload_count(0, which);
					}
				PLOG(("%06X:MC6840 #%d: Counter %d latch = %04X\n", activecpu_get_previouspc(), which, 0, currptr->latch[0]));
				break;
			}

			case PTM_6840_LSB2://5
			{
				currptr->latch[1] = (currptr->msb_buffer << 8) | (data & 0xff);
				/* clear the interrupt */
				currptr->status_reg &= ~(1 << 1);
				update_interrupts(which);
				/* reload the count if in an appropriate mode */
				if (!(currptr->control_reg[1] & 0x10))
				{
					reload_count(1, which);
				}
				PLOG(("%06X:MC6840 #%d: Counter %d latch = %04X\n", activecpu_get_previouspc(), which, 1, currptr->latch[1]));
				break;
			}

			case PTM_6840_LSB3://7
			{
				currptr->latch[2] = (currptr->msb_buffer << 8) | (data & 0xff);
				/* clear the interrupt */
				currptr->status_reg &= ~(1 << 2);
				update_interrupts(which);
				/* reload the count if in an appropriate mode */
				if (!(currptr->control_reg[2] & 0x10))
				{
					reload_count(2, which);
				}
				PLOG(("%06X:MC6840 #%d: Counter %d latch = %04X\n", activecpu_get_previouspc(), which, 2, currptr->latch[2]));
				break;
			}
		}
	}
}
///////////////////////////////////////////////////////////////////////////
//                                                                       //
// ptm6840_t1_timeout: called if timer1 is mature                        //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

static void ptm6840_t1_timeout(int which)
{
	ptm6840 *p = ptm + which;

	PLOG(("**ptm6840 %d t1 timeout**\n", which));

	if ( p->control_reg[0] & 0x40 )
	{ // interrupt enabled
		p->status_reg |= 0x01;
		p->status_read_since_int &= ~0x01;
		update_interrupts(which);
	}

	if ( p->control_reg[0] & 0x80 )
	{ // output enabled
		if ( p->intf )
		{
			if ((p->mode[0] == 0)|(p->mode[0] == 2))
			{
				p->output[0] = p->output[0]?0:1;
				PLOG(("**ptm6840 %d t1 output %d **\n", which, p->output[0]));
				if ( p->intf->out1_func ) p->intf->out1_func(0, p->output[0]);
			}
			if ((p->mode[0] == 4)|(p->mode[0] == 6))
			{
				if (!p->fired[0])
				{
					p->output[0] = 1;
					PLOG(("**ptm6840 %d t1 output %d **\n", which, p->output[0]));
					if ( p->intf->out1_func ) p->intf->out1_func(0, p->output[0]);
					p->fired[0]=1;//no changes in output until reinit
				}
			}
		}
	}
	p->enabled[0]= 0;
	reload_count(0,which);
}
///////////////////////////////////////////////////////////////////////////
//                                                                       //
// ptm6840_t2_timeout: called if timer2 is mature                        //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

static void ptm6840_t2_timeout(int which)
{
	ptm6840 *p = ptm + which;

	PLOG(("**ptm6840 %d t2 timeout**\n", which));

	if ( p->control_reg[1] & 0x40 )
	{ // interrupt enabled
		p->status_reg |= 0x02;
		p->status_read_since_int &= ~0x02;
		update_interrupts(which);
	}

	if ( p->control_reg[1] & 0x80 )
	{ // output enabled
		if ( p->intf )
		{
			if ((p->mode[1] == 0)|(p->mode[1] == 2))
			{
				p->output[1] = p->output[1]?0:1;
				PLOG(("**ptm6840 %d t2 output %d **\n", which, p->output[1]));
				if ( p->intf->out2_func ) p->intf->out2_func(0, p->output[1]);
			}
			if ((p->mode[1] == 4)|(p->mode[1] == 6))
			{
				if (!p->fired[1])
				{
					p->output[1] = 1;
					PLOG(("**ptm6840 %d t2 output %d **\n", which, p->output[1]));
					if ( p->intf->out2_func ) p->intf->out2_func(0, p->output[1]);
					p->fired[1]=1;//no changes in output until reinit
				}
			}

		}
	}
	p->enabled[1]= 0;
	reload_count(1,which);
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// ptm6840_t3_timeout: called if timer3 is mature                        //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

static void ptm6840_t3_timeout(int which)
{
	ptm6840 *p = ptm + which;

	PLOG(("**ptm6840 %d t3 timeout**\n", which));

	if ( p->control_reg[2] & 0x40 )
	{ // interrupt enabled
		p->status_reg |= 0x04;
		p->status_read_since_int &= ~0x04;
		update_interrupts(which);
	}

	if ( p->control_reg[2] & 0x80 )
	{ // output enabled
		if ( p->intf )
		{
			if ((p->mode[2] == 0)|(p->mode[2] == 2))
			{
				p->output[2] = p->output[2]?0:1;
				PLOG(("**ptm6840 %d t3 output %d **\n", which, p->output[2]));
				if ( p->intf->out3_func ) p->intf->out3_func(0, p->output[2]);
			}
			if ((p->mode[2] == 4)|(p->mode[2] == 6))
			{
				if (!p->fired[2])
				{
					p->output[2] = 1;
					PLOG(("**ptm6840 %d t3 output %d **\n", which, p->output[2]));
					if ( p->intf->out3_func ) p->intf->out3_func(0, p->output[2]);
					p->fired[2]=1;//no changes in output until reinit
				}
			}

		}
	}
	p->enabled[2]= 0;
	reload_count(2,which);
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// ptm6840_set_g1: set gate1 status (0 Or 1)                             //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_set_g1(int which, int state)
{
	ptm6840 *p = ptm + which;


	if ((p->mode[0] == 0)|(p->mode[0] == 2)|(p->mode[0] == 4)|(p->mode[0] == 6))
		{
		if (state == 0 && p->gate[0])
		reload_count (0,which);
		}
	p->gate[0] = state;
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// ptm6840_set_c1: set clock1 status (0 Or 1)                            //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_set_c1(int which, int state)
{
	ptm6840 *p = ptm + which;

	p->clock[0] = state;

	if (!(p->control_reg[0] & 0x02))
	{
		if (state) subtract_from_counter(0, 1,which);
	}
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// ptm6840_set_g2: set gate2 status (0 Or 1)                             //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_set_g2(int which, int state)
{
	ptm6840 *p = ptm + which;

	if ((p->mode[1] == 0)|(p->mode[1] == 2)|(p->mode[1] == 4)|(p->mode[1] == 6))
		{
		if (state == 0 && p->gate[1])
		reload_count (1,which);
		}
	p->gate[1] = state;
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// ptm6840_set_c2: set clock2 status (0 Or 1)                            //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_set_c2(int which, int state)
{
	ptm6840 *p = ptm + which;

	p->clock[1] = state;

	if (!(p->control_reg[1] & 0x02))
	{
		if (state) subtract_from_counter(1, 1,which);
	}
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// ptm6840_set_g3: set gate3 status (0 Or 1)                             //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_set_g3(int which, int state)
{
	ptm6840 *p = ptm + which;

	if ((p->mode[2] == 0)|(p->mode[2] == 2)|(p->mode[2] == 4)|(p->mode[2] == 6))
		{
		if (state == 0 && p->gate[2])
		reload_count (2,which);
		}
		p->gate[2] = state;
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// ptm6840_set_c3: set clock3 status (0 Or 1)                            //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_set_c3(int which, int state)
{
	ptm6840 *p = ptm + which;

	p->clock[2] = state;
	if (!(p->control_reg[2] & 0x02))
	{
		if (state) subtract_from_counter(2, 1,which);
	}
}

///////////////////////////////////////////////////////////////////////////

READ8_HANDLER( ptm6840_0_r ) { return ptm6840_read(0, offset); }
READ8_HANDLER( ptm6840_1_r ) { return ptm6840_read(1, offset); }
READ8_HANDLER( ptm6840_2_r ) { return ptm6840_read(2, offset); }
READ8_HANDLER( ptm6840_3_r ) { return ptm6840_read(3, offset); }

WRITE8_HANDLER( ptm6840_0_w ) { ptm6840_write(0, offset, data); }
WRITE8_HANDLER( ptm6840_1_w ) { ptm6840_write(1, offset, data); }
WRITE8_HANDLER( ptm6840_2_w ) { ptm6840_write(2, offset, data); }
WRITE8_HANDLER( ptm6840_3_w ) { ptm6840_write(3, offset, data); }

READ16_HANDLER( ptm6840_0_msb_r ) { return ptm6840_read(0, offset); }
READ16_HANDLER( ptm6840_1_msb_r ) { return ptm6840_read(1, offset); }
READ16_HANDLER( ptm6840_2_msb_r ) { return ptm6840_read(2, offset); }
READ16_HANDLER( ptm6840_3_msb_r ) { return ptm6840_read(3, offset); }

WRITE16_HANDLER( ptm6840_0_msb_w ) { if (ACCESSING_MSB) ptm6840_write(0, offset, (data >> 8) & 0xff); }
WRITE16_HANDLER( ptm6840_1_msb_w ) { if (ACCESSING_MSB) ptm6840_write(1, offset, (data >> 8) & 0xff); }
WRITE16_HANDLER( ptm6840_2_msb_w ) { if (ACCESSING_MSB) ptm6840_write(2, offset, (data >> 8) & 0xff); }
WRITE16_HANDLER( ptm6840_3_msb_w ) { if (ACCESSING_MSB) ptm6840_write(3, offset, (data >> 8) & 0xff); }

READ16_HANDLER( ptm6840_0_lsb_r ) { return ptm6840_read(0, offset << 8 | 0x00ff); }
READ16_HANDLER( ptm6840_1_lsb_r ) { return ptm6840_read(1, offset << 8 | 0x00ff); }
READ16_HANDLER( ptm6840_2_lsb_r ) { return ptm6840_read(2, offset << 8 | 0x00ff); }
READ16_HANDLER( ptm6840_3_lsb_r ) { return ptm6840_read(3, offset << 8 | 0x00ff); }

WRITE16_HANDLER( ptm6840_0_lsb_w ) {if (ACCESSING_LSB) ptm6840_write(0, offset, data & 0xff);}
WRITE16_HANDLER( ptm6840_1_lsb_w ) {if (ACCESSING_LSB) ptm6840_write(1, offset, data & 0xff);}
WRITE16_HANDLER( ptm6840_2_lsb_w ) {if (ACCESSING_LSB) ptm6840_write(2, offset, data & 0xff);}
WRITE16_HANDLER( ptm6840_3_lsb_w ) {if (ACCESSING_LSB) ptm6840_write(3, offset, data & 0xff);}
