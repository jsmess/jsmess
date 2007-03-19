
/********************************
DAVE SOUND CHIP FOUND IN ENTERPRISE
*********************************/

/* working:

	- pure tone
	- sampled sounds
	- 1khz, 50hz and 1hz ints
	- external ints (int1 and int2) - not correct speed yet

*/

#include "driver.h"
#include "audio/dave.h"

#define STEP 0x08000

static DAVE dave;
static DAVE_INTERFACE *dave_iface;

//static unsigned char Dave_IntRegRead(void);
//static void Dave_IntRegWrite(unsigned char);
//static void     Dave_SetInterruptWanted(void);

/* temp here */
static int nick_virq = 0;


static void Dave_reset(void)
{
	int i;

	for (i=0; i<32; i++)
	{
		dave.Regs[i] = 0;
	}
}


static void dave_refresh_ints(void)
{
	int int_wanted;

	logerror("int latch: %02x enable: %02x input: %02x\n", (int) dave.int_latch, (int) dave.int_enable, (int) dave.int_input);
	
	int_wanted = (((dave.int_enable<<1) & dave.int_latch)!=0);

	if (dave_iface->int_callback)
	{
		dave_iface->int_callback(int_wanted);
	}
}


static void dave_refresh_selectable_int(void)
{
	/* update 1khz/50hz/tg latch and int input */
	switch ((dave.Regs[7]>>5) & 0x03)
	{
		/* 1khz */
		case 0:
		{
			dave.int_latch &=~(1<<1);
			dave.int_latch |= (dave.int_irq>>1) & 0x02;

			/* set int input state */
			dave.int_input &= ~(1<<0);
			dave.int_input |= (dave.one_khz_state & 0x01)<<0;
		}
		break;

		/* 50hz */
		case 1:
		{
			dave.int_latch &=~(1<<1);
			dave.int_latch |= dave.int_irq & 0x02;

			/* set int input state */
			dave.int_input &= ~(1<<0);
			dave.int_input |= (dave.fifty_hz_state & 0x01)<<0;
		}
		break;
		

		default:
			break;
	}

	dave_refresh_ints();
}

static void dave_1khz_callback(int dummy)
{
//	logerror("1khz int\n");

	/* time over - want int */
	dave.one_khz_state^=0x0ffffffff;

	/* lo-high transition causes int */
	if (dave.one_khz_state!=0)
	{
		dave.int_irq |=(1<<2);
	}


	/* update fifty hz counter */
	dave.fifty_hz_count--;

	if (dave.fifty_hz_count==0)
	{
		/* these two lines are temp here */
		nick_virq^=0x0ffffffff;
		Dave_SetExternalIntState(DAVE_INT1_ID, nick_virq);


		dave.fifty_hz_count = DAVE_FIFTY_HZ_COUNTER_RELOAD;
		dave.fifty_hz_state^=0x0ffffffff;
	
		if (dave.fifty_hz_state!=0)
		{
			dave.int_irq |= (1<<1);
		}
	}

	dave.one_hz_count--;

	if (dave.one_hz_count==0)
	{
		/* reload counter */
		dave.one_hz_count = DAVE_ONE_HZ_COUNTER_RELOAD;
	
		/* change state */
		dave.int_input ^= (1<<2);

		if (dave.int_input & (1<<2))
		{
			/* transition from 0->1 */
			/* int requested */
			dave.int_latch |=(1<<3);
		}
	}

	dave_refresh_selectable_int();
}



void	Dave_Init(void)
{
	int i;

	//logerror("dave init\n");

	Dave_reset();

	/* temp! */
	nick_virq = 0;

	/* initialise 1khz timer */
	dave.int_latch = 0;
	dave.int_input = 0;
	dave.int_enable = 0;
	dave.timer_irq = 0;
	dave.fifty_hz_state = 0;
	dave.one_khz_state = 0;
	dave.fifty_hz_count = DAVE_FIFTY_HZ_COUNTER_RELOAD;
	dave.one_hz_count = DAVE_ONE_HZ_COUNTER_RELOAD;
	timer_pulse(TIME_IN_HZ(1000), 0, dave_1khz_callback);
	
	for (i=0; i<3; i++)
	{
		dave.Period[i] = ((STEP  * Machine->sample_rate)/125000);
		dave.Count[i] = ((STEP  * Machine->sample_rate)/125000);
		dave.level[i] = 0;
	}
}

static void dave_update_sound(void *param,stream_sample_t **inputs, stream_sample_t **_buffer,int length)
{		
	stream_sample_t *buffer1, *buffer2;
	/* 0 = channel 0 left volume, 1 = channel 0 right volume,
	2 = channel 1 left volume, 3 = channel 1 right volume,
	4 = channel 2 left volume, 5 = channel 2 right volume 
	6 = noise channel left volume, 7 = noise channel right volume */
	int output_volumes[8];
	int left_volume;
	int right_volume;

	//logerror("sound update!\n");

	buffer1 = _buffer[0];
	buffer2 = _buffer[1];

	while (length)
	{
		int vol[4];
		int i;


		/* vol[] keeps track of how long each square wave stays */
		/* in the 1 position during the sample period. */
		vol[0] = vol[1] = vol[2] = vol[3] = 0;

		for (i = 0;i < 3;i++)
		{
			if ((dave.Regs[7] & (1<<i))==0)
			{

				if (dave.level[i]) vol[i] += dave.Count[i];
				dave.Count[i] -= STEP;
				/* Period[i] is the half period of the square wave. Here, in each */
				/* loop I add Period[i] twice, so that at the end of the loop the */
				/* square wave is in the same status (0 or 1) it was at the start. */
				/* vol[i] is also incremented by Period[i], since the wave has been 1 */
				/* exactly half of the time, regardless of the initial position. */
				/* If we exit the loop in the middle, Output[i] has to be inverted */
				/* and vol[i] incremented only if the exit status of the square */
				/* wave is 1. */
				while (dave.Count[i] <= 0)
				{
					dave.Count[i] += dave.Period[i];
					if (dave.Count[i] > 0)
					{
						dave.level[i] ^= 0x0ffffffff;
						if (dave.level[i]) vol[i] += dave.Period[i];
						break;
					}
					dave.Count[i] += dave.Period[i];
					vol[i] += dave.Period[i];
				}
				if (dave.level[i]) vol[i] -= dave.Count[i];
			}
		}

		/* update volume outputs */

		/* setup output volumes for each channel */
		/* channel 0 */
		output_volumes[0] = ((dave.level[0] & dave.level_and[0]) | dave.level_or[0]) & dave.mame_volumes[0];
		output_volumes[1] = ((dave.level[0] & dave.level_and[1]) | dave.level_or[1]) & dave.mame_volumes[4];
		/* channel 1 */
		output_volumes[2] = ((dave.level[1] & dave.level_and[2]) | dave.level_or[2]) & dave.mame_volumes[1];
		output_volumes[3] = ((dave.level[1] & dave.level_and[3]) | dave.level_or[3]) & dave.mame_volumes[5];
		/* channel 2 */
		output_volumes[4] = ((dave.level[2] & dave.level_and[4]) | dave.level_or[4]) & dave.mame_volumes[2];
		output_volumes[5] = ((dave.level[2] & dave.level_and[5]) | dave.level_or[5]) & dave.mame_volumes[6];
		/* channel 3 */
		output_volumes[6] = ((dave.level[3] & dave.level_and[6]) | dave.level_or[6]) & dave.mame_volumes[3];
		output_volumes[7] = ((dave.level[3] & dave.level_and[7]) | dave.level_or[7]) & dave.mame_volumes[7];

		left_volume = (output_volumes[0] + output_volumes[2] + output_volumes[4] + output_volumes[6])>>2;
		right_volume = (output_volumes[1] + output_volumes[3] + output_volumes[5] + output_volumes[7])>>2;

		*(buffer1++) = left_volume;
		*(buffer2++) = right_volume;

		length--;
	}
}


/* dave has 3 tone channels and 1 noise channel.
the volumes are mixed internally and output as left and right volume */
void *Dave_sh_start(int clock, const struct CustomSound_interface *config)
{
	/* 3 tone channels + 1 noise channel */
	dave.sound_stream = stream_create( 0, 2, Machine->sample_rate, NULL, dave_update_sound);
	return (void *) ~0;
}



/* used to update sound output based on data writes */
static WRITE8_HANDLER(Dave_sound_w)
{

	//logerror("sound w: %04x %02x\n",offset,data);

	/* update stream */
	stream_update(dave.sound_stream);

	/* new write */
	switch (offset)
	{
		/* channel 0 down-counter */
		case 0:
		case 1:
		/* channel 1 down-counter */
		case 2:
		case 3:
		/* channel 2 down-counter */
		case 4:
		case 5:
		{
			int count = 0;
			int channel_index = offset>>1; 

			/* Fout = 125,000 / (n+1) hz */

			/* sample rate/clock */
			

			/* get down-count */
			switch (offset & 0x01)
			{
				case 0:
				{
					count = (data & 0x0ff) | ((dave.Regs[offset+1] & 0x0f)<<8);
				}
				break;

				case 1:
				{
					count = (dave.Regs[offset-1] & 0x0ff) | ((data & 0x0f)<<8);

				}
				break;
			}
			
			count++;

		
			dave.Period[channel_index] = ((STEP  * Machine->sample_rate)/125000) * count;

		}
		break;

		/* channel 0 left volume */
		case 8:
		/* channel 1 left volume */
		case 9:
		/* channel 2 left volume */
		case 10:
		/* noise channel left volume */
		case 11:
		/* channel 0 right volume */
		case 12:
		/* channel 1 right volume */
		case 13:
		/* channel 2 right volume */
		case 14:
		/* noise channel right volume */
		case 15:
		{
			/* update mame version of volume from data written */
			/* 0x03f->0x07e00. Max is 0x07fff */
			/* I believe the volume is linear - to be checked! */
			dave.mame_volumes[offset-8] = (data & 0x03f)<<9;
		}
		break;

		case 7:
		{
			/*	force => the value of this register is forced regardless of the wave
					state,
				remove => this value is force to zero so that it has no influence over
					the final volume calculation, regardless of wave state 
				use => the volume value is dependant on the wave state and is included
					in the final volume calculation */

			logerror("selectable int ");
			switch ((data>>5) & 0x03)
			{
				case 0:
				{
					logerror("1khz\n");
				}
				break;
				
				case 1:
				{
					logerror("50hz\n");
				}
				break;

				case 2:
				{
					logerror("tone channel 0\n");
				}
				break;

				case 3:
				{
					logerror("tone channel 1\n");
				}
				break;
			}



			/* turn L.H audio output into D/A, outputting value in R8 */
			if (data & (1<<3))
			{
				/* force r8 value */
				dave.level_or[0] = 0x0ffff;
				dave.level_and[0] = 0x00;
			
				/* remove r9 value */
				dave.level_or[2] = 0x000;
				dave.level_and[2] = 0x00;

				/* remove r10 value */
				dave.level_or[4] = 0x000;
				dave.level_and[4] = 0x00;

				/* remove r11 value */
				dave.level_or[6] = 0x000;
				dave.level_and[6] = 0x00;
			}
			else
			{
				/* use r8 value */
				dave.level_or[0] = 0x000;
				dave.level_and[0] = 0xffff;
			
				/* use r9 value */
				dave.level_or[2] = 0x000;
				dave.level_and[2] = 0xffff;

				/* use r10 value */
				dave.level_or[4] = 0x000;
				dave.level_and[4] = 0xffff;

				/* use r11 value */
				dave.level_or[6] = 0x000;
				dave.level_and[6] = 0xffff;
			}

			/* turn L.H audio output into D/A, outputting value in R12 */
			if (data & (1<<4))
			{
				/* force r12 value */
				dave.level_or[1] = 0x0ffff;
				dave.level_and[1] = 0x00;
			
				/* remove r13 value */
				dave.level_or[3] = 0x000;
				dave.level_and[3] = 0x00;

				/* remove r14 value */
				dave.level_or[5] = 0x000;
				dave.level_and[5] = 0x00;

				/* remove r15 value */
				dave.level_or[7] = 0x000;
				dave.level_and[7] = 0x00;
			}
			else
			{
				/* use r12 value */
				dave.level_or[1] = 0x000;
				dave.level_and[1] = 0xffff;
			
				/* use r13 value */
				dave.level_or[3] = 0x000;
				dave.level_and[3] = 0xffff;

				/* use r14 value */
				dave.level_or[5] = 0x000;
				dave.level_and[5] = 0xffff;

				/* use r15 value */
				dave.level_or[7] = 0x000;
				dave.level_and[7] = 0xffff;
			}
		}
		break;

		default:
			break;


	}



}


WRITE8_HANDLER (	Dave_reg_w )
{
	logerror("dave w: %04x %02x\n",offset,data);

	Dave_sound_w(offset,data);

	dave.Regs[offset & 0x01f] = data;

	switch (offset)
	{
		case 0x07:
		{
			dave_refresh_selectable_int();
		}
		break;

		case 0x014:
		{
			/* enabled ints */
			dave.int_enable = data & 0x055;
			/* clear latches */
			dave.int_latch &=~(data & 0x0aa);

			/* reset 1khz, 50hz latch */
			if (data & (1<<1))
			{
				dave.int_irq = 0;
			}

			/* refresh ints */
			dave_refresh_ints();
		}
		break;

		default:
			break;
	}

	if (dave_iface!=NULL)
	{
		dave_iface->reg_w(offset, data);
	}
}


WRITE8_HANDLER ( Dave_setreg )
{
	dave.Regs[offset & 0x01f] = data;
}

READ8_HANDLER (	Dave_reg_r )
{
	logerror("dave r: %04x\n",offset);

	if (dave_iface!=NULL)
	{
		dave_iface->reg_r(offset);
	}

	switch (offset)
	{
		case 0x000:
		case 0x001:
		case 0x002:
		case 0x003:
		case 0x004:
		case 0x005:
		case 0x006:
		case 0x007:
		case 0x008:
		case 0x009:
		case 0x00a:
		case 0x00b:
		case 0x00c:
		case 0x00d:
		case 0x00e:
		case 0x00f:
		case 0x018:
		case 0x019:
		case 0x01a:
		case 0x01b:
		case 0x01c:
		case 0x01d:
		case 0x01e:
		case 0x01f:
			return 0x0ff;

		case 0x014:
			return (dave.int_latch & 0x0aa) | (dave.int_input & 0x055);
	

		default:
			break;
	}

	return dave.Regs[offset & 0x01f];
}

/* negative edge triggered */
void	Dave_SetExternalIntState(int IntID, int State)
{
	switch (IntID)
	{
		/* connected to Nick virq */
		case DAVE_INT1_ID:
		{
			int previous_state;

			previous_state = dave.int_input;

			dave.int_input &=~(1<<4);

			if (State)
			{
				dave.int_input |=(1<<4);
			}

			if ((previous_state ^ dave.int_input) & (1<<4))
			{
				/* changed state */

				if (dave.int_input & (1<<4))
				{
					/* int request */
					dave.int_latch |= (1<<5);

					dave_refresh_ints();
				}
			}

		}
		break;
	
		case DAVE_INT2_ID:
		{
			int previous_state;

			previous_state = dave.int_input;

			dave.int_input &= ~(1<<6);

			if (State)
			{
				dave.int_input |=(1<<6);
			}

			if ((previous_state ^ dave.int_input) & (1<<6))
			{
				/* changed state */

				if (dave.int_input & (1<<6))
				{
					/* int request */
					dave.int_latch|=(1<<7);

					dave_refresh_ints();
				}
			}
		}
		break;

		default:
			break;
	}
}



int	Dave_getreg(int RegIndex)
{
	return dave.Regs[RegIndex & 0x01f];
}

void	Dave_SetIFace(struct DAVE_INTERFACE *newInterface)
{
	dave_iface = newInterface;
}


/*
Reg 4 READ:

b7 = 1: INT2 latch set
b6 = INT2 input pin
b5 = 1: INT1 latch set
b4 = INT1 input pin
b3 = 1: 1Hz latch set
b2 = 1hz input pin
b1 = 1: 1khz/50hz/TG latch set
b0 = 1khz/50hz/TG input

Reg 4 WRITE:

b7 = 1: Reset INT2 latch
b6 = 1: Enable INT2
b5 = 1: Reset INT1 latch
b4 = 1: Enable INT1
b3 = 1: Reset 1hz interrupt latch
b2 = 1: Enable 1hz interrupt
b1 = 1: Reset 1khz/50hz/TG latch
b0 = 1: Enable 1khz/50Hz/TG latch
*/

#if 0
static void	Dave_SetInterruptWanted(void)
{
}

void    Dave_UpdateInterruptLatches(int input_mask)
{
	if (((dave.previous_int_input^dave.int_input)&(input_mask))!=0)
	{
		/* it changed */

		if ((dave.int_input & (input_mask))==0)
		{
				/* negative edge */
				dave.int_latch |= (input_mask<<1);
		}
	}
}

#endif

