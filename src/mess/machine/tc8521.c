/* Toshiba TC8521 Real-Time Clock or RICOH RF5C01A*/

#include "mame.h"
#include "includes/tc8521.h"

/* Registers:

Page 0/Mode 0 (timer):

0x00: 1-second counter
0x01: 10-second counter
0x02: 1-minute counter
0x03: 10-minute counter
0x04: 1-hour counter
0x05: 10-hour counter
0x06: day-of-the-week counter
0x07: 1-day counter
0x08: 10-day counter
0x09: 1-month counter
0x0A: 10-month counter
0x0b: 1-year counter
0x0c: 10-year counter

Page 1/Mode 1 (alarm):
0x00: unused
0x01: unused
0x02: 1-minute alarm
0x03: 10-minute alarm
0x04: 1-hour alarm
0x05: 10-hour alarm
0x06: day-of-the-week alarm
0x07: 1-day alarm
0x08: 10-day alarm
0x09: unused
0x0a: 12/24 hour select register
0x0b: leap year counter
0x0c: unused

Page 2/Mode 2 (ram block 1) and
Page 3/Mode 3 (ram block 2):
0x00: ram
0x01: ram
0x02: ram
0x03: ram
0x04: ram
0x05: ram
0x06: ram
0x07: ram
0x08: ram
0x09: ram
0x0a: ram
0x0b: ram
0x0c: ram


registers common to all modes:

0x0d: MODE register
	bit 3: timer enable
	bit 2: alarm enable
	bit 1,0: mode/page select
0x0e: test register
	bit 3: test 3
	bit 2: test 2
	bit 1: test 1
	bit 0: test 0
0x0f: RESET etc
	bit 3: 1hz enable
	bit 2: 16hz enable
	bit 1: timer reset
	bit 0: alarm reset
*/

#define TC8521_TIMER_1_SECOND_COUNTER 0x00
#define TC8521_TIMER_10_SECOND_COUNTER 0x01
#define TC8521_TIMER_1_MINUTE_COUNTER 0x02
#define TC8521_TIMER_10_MINUTE_COUNTER 0x03
#define TC8521_TIMER_1_HOUR_COUNTER 0x04
#define TC8521_TIMER_10_HOUR_COUNTER 0x05
#define TC8521_TIMER_DAY_OF_THE_WEEK_COUNTER 0x06
#define TC8521_TIMER_1_DAY_COUNTER 0x07
#define TC8521_TIMER_10_DAY_COUNTER 0x08
#define TC8521_TIMER_1_MONTH_COUNTER 0x09
#define TC8521_TIMER_10_MONTH_COUNTER 0x0a
#define TC8521_TIMER_1_YEAR_COUNTER 0x0b
#define TC8521_TIMER_10_YEAR_COUNTER 0x0c

#define TC8521_MODE_REGISTER 0x0d
#define TC8521_TEST_REGISTER 0x0e
#define TC8521_RESET_REGISTER 0x0f

#define TC8521_ALARM_1_MINUTE_REGISTER 0x012
#define TC8521_ALARM_10_MINUTE_REGISTER 0x013
#define TC8521_ALARM_1_HOUR_REGISTER 0x014
#define TC8521_ALARM_10_HOUR_REGISTER 0x015
#define TC8521_ALARM_DAY_OF_THE_WEEK_REGISTER 0x016
#define TC8521_ALARM_1_DAY_REGISTER 0x017
#define TC8521_ALARM_10_DAY_REGISTER 0x018

/* uncomment for verbose debugging information */
//#define VERBOSE

/* mask data with these values when writing */
static unsigned char rtc_write_masks[16*4]=
{
	0x0f,0x07,0x0f,0x07,0x0f,0x03,0x07,0x0f,0x03,0x0f,0x03,0x0f,0x0f,0x0f, 0x0f, 0x0f,
	0x00,0x00,0x0f,0x07,0x0f,0x03,0x07,0x0f,0x03,0x00,0x01,0x03,0x00,0x0f, 0x0f, 0x0f,
	0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f, 0x0f, 0x0f,
	0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f, 0x0f, 0x0f,
};

#define ALARM_OUTPUT_16HZ	0x01
#define ALARM_OUTPUT_1HZ	0x02
#define ALARM_OUTPUT_ALARM	0X04

struct tc8521
{
	/* 4 register pages (timer, alarm, ram1 and ram2) */
    unsigned char registers[16*4];
	/* interface for 1hz/16hz and alarm outputs */
    struct tc8521_interface interface;

	unsigned long alarm_outputs;
	unsigned long thirty_two_hz_counter;
};

static struct tc8521 rtc;


/* read tc8521 data from supplied file */
void tc8521_load_stream(mame_file *file)
{
	if (file)
	{
		mame_fread(file, &rtc.registers[0], sizeof(unsigned char)*16*4);
		mame_fread(file, &rtc.alarm_outputs, sizeof(unsigned long));
		mame_fread(file, &rtc.thirty_two_hz_counter, sizeof(unsigned long));
	}
}

/* write tc8521 data to supplied file */
void tc8521_save_stream(mame_file *file)
{
	if (file)
	{
		mame_fwrite(file, &rtc.registers[0], sizeof(unsigned char)*16*4);
		mame_fwrite(file, &rtc.alarm_outputs, sizeof(unsigned long));
		mame_fwrite(file, &rtc.thirty_two_hz_counter, sizeof(unsigned long));
	}
}

static void tc8521_set_alarm_output(void)
{
	unsigned char alarm_output;
	
	alarm_output = 0;

	/* what happens when all are enabled? I assume they are all or'd together */

	/* 16hz enabled? */
    if ((rtc.registers[TC8521_RESET_REGISTER] & (1<<2))==0)
    {
		/* yes */

		/* add in state of 16hz output */
		alarm_output |= (rtc.alarm_outputs & ALARM_OUTPUT_16HZ);
	}

	if ((rtc.registers[TC8521_RESET_REGISTER] & (1<<3))==0)
	{
		/* yes */
		/* add in stat of 1hz output */
		alarm_output |= ((rtc.alarm_outputs & ALARM_OUTPUT_1HZ)>>1);
	}

	alarm_output |= ((rtc.alarm_outputs & ALARM_OUTPUT_ALARM)>>2);

	/* if it's not enabled, then there is no output */
	if (!((rtc.registers[TC8521_MODE_REGISTER] & (1<<2))!=0))
	{
		/* alarm output enabled? */
		alarm_output &= ~1;
	}
	
    if (rtc.interface.alarm_output_callback!=NULL)
    {
		rtc.interface.alarm_output_callback(alarm_output);
	}
}

static void tc8521_alarm_check(void)
{

	rtc.alarm_outputs &= ~ALARM_OUTPUT_ALARM;
	if (
		(rtc.registers[TC8521_TIMER_1_MINUTE_COUNTER]==rtc.registers[TC8521_ALARM_1_MINUTE_REGISTER]) &&
		(rtc.registers[TC8521_TIMER_10_MINUTE_COUNTER]==rtc.registers[TC8521_ALARM_10_MINUTE_REGISTER]) &&
		(rtc.registers[TC8521_TIMER_1_HOUR_COUNTER]==rtc.registers[TC8521_ALARM_1_HOUR_REGISTER]) &&
		(rtc.registers[TC8521_TIMER_10_HOUR_COUNTER]==rtc.registers[TC8521_ALARM_10_HOUR_REGISTER]) &&
		(rtc.registers[TC8521_TIMER_1_DAY_COUNTER]==rtc.registers[TC8521_ALARM_1_DAY_REGISTER]) &&
		(rtc.registers[TC8521_TIMER_10_DAY_COUNTER]==rtc.registers[TC8521_ALARM_10_DAY_REGISTER]) &&
		(rtc.registers[TC8521_TIMER_DAY_OF_THE_WEEK_COUNTER]==rtc.registers[TC8521_ALARM_DAY_OF_THE_WEEK_REGISTER])
		)
	{
		rtc.alarm_outputs |= ALARM_OUTPUT_ALARM;
	}

	tc8521_set_alarm_output();
}

static void tc8521_timer_callback(int dummy)
{
	/* Assumption how it works */
	/* 16hz output = 16 cycles per second, 16 cycles of high-low from counter */

	/* toggle 16hz wave state */
	rtc.alarm_outputs ^= ALARM_OUTPUT_16HZ;
	/* set in alarm output */
	tc8521_set_alarm_output();

	rtc.thirty_two_hz_counter++;

	if (rtc.thirty_two_hz_counter==16)
	{
		rtc.thirty_two_hz_counter = 0;

		/* toggle 1hz output */
		rtc.alarm_outputs ^= ALARM_OUTPUT_1HZ;
		/* set in alarm output */
		tc8521_set_alarm_output();

		if ((rtc.alarm_outputs & ALARM_OUTPUT_1HZ)==0)
		{
			/* timer enable? */
			if ((rtc.registers[0x0d] & (1<<3))!=0)
			{
			   /* seconds; units */
			   rtc.registers[0]++;

			   if (rtc.registers[0]==10)
			   {
				   rtc.registers[0] = 0;

				   /* seconds; tens */
				   rtc.registers[1]++;

				   if (rtc.registers[1]==6)
				   {
					  rtc.registers[1] = 0;

					  /* minutes; units */
					  rtc.registers[2]++;

					  if (rtc.registers[2]==10)
					  {
						  rtc.registers[2] = 0;

						  /* minutes; tens */
						  rtc.registers[3]++;

						  if (rtc.registers[3] == 6)
						  {
							 rtc.registers[3] = 0;

							 /* hours; units */
							 rtc.registers[4]++;

							 if (rtc.registers[4] == 10)
							 {
								rtc.registers[4] = 0;

								/* hours; tens */
								rtc.registers[4]++;

								if (rtc.registers[4] == 24)
								{
									/* TODO: finish rest of increments here! */
									rtc.registers[4] = 0;
								}
							 }
						  }
						}

						tc8521_alarm_check();
					}
				}
			}
		}
	}
}



void tc8521_init(struct tc8521_interface *intf)
{
	memset(&rtc, 0, sizeof(struct tc8521));
	memset(&rtc.interface, 0, sizeof(struct tc8521_interface));
	if (intf)
		memcpy(&rtc.interface, intf, sizeof(struct tc8521_interface));
	timer_pulse(TIME_IN_HZ(32), 0, tc8521_timer_callback);
}

 READ8_HANDLER(tc8521_r)
{
		unsigned long register_index;

		switch (offset)
		{
			/* control registers */
			case 0x0d:
			case 0x0e:
			case 0x0f:
#ifdef VERBOSE
				logerror("8521 RTC R: %04x %02x\n", offset, rtc.registers[offset]);
#endif
        		return rtc.registers[offset];
		
			default:
				break;
		}

		/* register in selected page */
        register_index = ((rtc.registers[TC8521_MODE_REGISTER] & 0x03)<<4) | (offset & 0x0f);
#ifdef VERBOSE
		logerror("8521 RTC R: %04x %02x\n", offset, rtc.registers[register_index]);
#endif
		/* data from selected page */
		return rtc.registers[register_index];
}


WRITE8_HANDLER(tc8521_w)
{
	unsigned long register_index;

#ifdef VERBOSE
        logerror("8521 RTC W: %04x %02x\n", offset, data);

		switch (offset)
        {
                case 0x0D:
                {
                        if (data & 0x08)
                        {
                            logerror("timer enable\n");
                        }

                        if (data & 0x04)
                        {
                            logerror("alarm enable\n");
                        }

						logerror("page %02x selected\n", data & 0x03);
	              }
                break;

                case 0x0e:
                {
                        if (data & 0x08)
                        {
                            logerror("test 3\n");
                        }

                        if (data & 0x04)
                        {
                            logerror("test 2\n");
                        }

                        if (data & 0x02)
                        {
                            logerror("test 1\n");
                        }

                        if (data & 0x01)
                        {
                            logerror("test 0\n");
                        }
                }
                break;

                case 0x0f:
                {
                        if ((data & 0x08)==0)
                        {
                           logerror("1hz enable\n");
                        }

                        if ((data & 0x04)==0)
                        {
                           logerror("16hz enable\n");
                        }

                        if (data & 0x02)
                        {
                           logerror("reset timer\n");
                        }

                        if (data & 0x01)
                        {
                           logerror("reset alarm\n");
                        }
                }
                break;

                default:
                  break;
        }
#endif
		switch (offset)
		{
			/* control registers */
			case 0x0d:
			case 0x0e:
			case 0x0f:
				rtc.registers[offset] = data & rtc_write_masks[offset];
				return;
			default:
				break;
		}

		/* register in selected page */
        register_index = ((rtc.registers[TC8521_MODE_REGISTER] & 0x03)<<4) | (offset & 0x0f);

		/* write and mask data */
		rtc.registers[register_index] = data & rtc_write_masks[register_index];
}



