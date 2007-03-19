/***************************************************************************

	mm58274c.c

	mm58274c emulation

	Reference:
	* National Semiconductor MM58274C Microprocessor Compatible Real Time Clock
		<http://www.national.com/ds/MM/MM58274C.pdf>

	Todo:
	* Clock initialization will only work with the BwG: we need to provide
	  a way to customize it.
	* Save the config to NVRAM?
	* Support interrupt pin output

	Raphael Nabet, 2002

***************************************************************************/

#include "mm58274c.h"

static void rtc_interrupt_callback(int which);
static void increment_rtc(int which);

#define MAX_58274 2

static struct
{
	int status;		/* status register (*read* from address 0 = control register) */
	int control;	/* control register (*write* to address 0) */

	int clk_set;	/* clock setting register */
	int int_ctl;	/* interrupt control register */


	int wday;		/* day of the week (1-7 (1=monday, 7=sunday)) */
	int years1;		/* years (BCD: 0-99) */
	int years2;
	int months1;	/* months (BCD: 1-12) */
	int months2;
	int days1;		/* days (BCD: 1-31) */
	int days2;
	int hours1;		/* hours (BCD : 0-23) */
	int hours2;
	int minutes1;	/* minutes (BCD : 0-59) */
	int minutes2;
	int seconds1;	/* seconds (BCD : 0-59) */
	int seconds2;
	int tenths;		/* tenths of second (BCD : 0-9) */

	void *interrupt_timer;
} rtc[MAX_58274];

enum
{
	st_dcf = 0x8,		/* data-changed flag */
	st_if = 0x1,		/* interrupt flag */

	ctl_test = 0x8,		/* test mode (0=normal, 1=test) (not emulated) */
	ctl_clkstop = 0x4,	/* clock start/stop (0=run, 1=stop) */
	ctl_intsel = 0x2,	/* interrupt select (0=clock setting register, 1=interrupt register) */
	ctl_intstop = 0x1,	/* interrupt start stop (0=interrupt run, 1=interrupt stop) */

	clk_set_leap = 0xc,		/* leap year counter (0 indicates a leap year) */
	clk_set_leap_inc = 0x4,	/* leap year increment */
	clk_set_pm = 0x2,		/* am/pm indicator (0 = am, 1 = pm, 0 in 24-hour mode) */
	clk_set_24 = 0x1,		/* 12/24-hour select bit (1= 24-hour mode) */

	int_ctl_rpt = 0x8,		/* 1 for repeated interrupt */
	int_ctl_dly = 0x7		/* 0 no interrupt, 1 = .1 second, 2=.5, 3=1, 4=5, 5=10, 6=30, 7=60 */
};

static const double interrupt_period_table[8] =
{
	0.,
	TIME_IN_SEC(.1),
	TIME_IN_SEC(.5),
	TIME_IN_SEC(1),
	TIME_IN_SEC(5),
	TIME_IN_SEC(10),
	TIME_IN_SEC(30),
	TIME_IN_SEC(60)
};


void mm58274c_init(int which, int mode24)
{
	memset(&rtc[which], 0, sizeof(rtc[which]));

	timer_pulse(TIME_IN_SEC(.1), which, increment_rtc);
	rtc[which].interrupt_timer = timer_alloc(rtc_interrupt_callback);

	{
		mame_system_time systime;
		
		/* get the current date/time from the core */
		mame_get_current_datetime(Machine, &systime);

		rtc[which].clk_set = systime.local_time.year & 3 << 2;
		if (mode24)
			rtc[which].clk_set |= clk_set_24;

		/* The clock count starts on 1st January 1900 */
		rtc[which].wday = systime.local_time.weekday ? systime.local_time.weekday : 7;
		rtc[which].years1 = (systime.local_time.year / 10) % 10;
		rtc[which].years2 = systime.local_time.year % 10;
		rtc[which].months1 = (systime.local_time.month + 1) / 10;
		rtc[which].months2 = (systime.local_time.month + 1) % 10;
		rtc[which].days1 = systime.local_time.mday / 10;
		rtc[which].days2 = systime.local_time.mday % 10;
		if (! mode24)
		{
			/* 12-hour mode */
			if (systime.local_time.hour > 12)
			{
				systime.local_time.hour -= 12;
				rtc[which].clk_set |= clk_set_pm;
			}
			if (systime.local_time.hour == 0)
				systime.local_time.hour = 12;
		}
		rtc[which].hours1 = systime.local_time.hour / 10;
		rtc[which].hours2 = systime.local_time.hour % 10;
		rtc[which].minutes1 = systime.local_time.minute / 10;
		rtc[which].minutes2 = systime.local_time.minute % 10;
		rtc[which].seconds1 = systime.local_time.second / 10;
		rtc[which].seconds2 = systime.local_time.second % 10;
		rtc[which].tenths = 0;
	}
}


int mm58274c_r(int which, int offset)
{
	int reply;


	offset &= 0xf;

	switch (offset)
	{
	case 0x0:	/* Control Register */
		reply = rtc[which].status;
		rtc[which].status = 0;
		break;

	case 0x1:	/* Tenths of Seconds */
		reply = rtc[which].tenths;
		break;

	case 0x2:	/* Units Seconds */
		reply = rtc[which].seconds2;
		break;

	case 0x3:	/* Tens Seconds */
		reply = rtc[which].seconds1;
		break;

	case 0x04:	/* Units Minutes */
		reply = rtc[which].minutes2;
		break;

	case 0x5:	/* Tens Minutes */
		reply = rtc[which].minutes1;
		break;

	case 0x6:	/* Units Hours */
		reply = rtc[which].hours2;
		break;

	case 0x7:	/* Tens Hours */
		reply = rtc[which].hours1;
		break;

	case 0x8:	/* Units Days */
		reply = rtc[which].days2;
		break;

	case 0x9:	/* Tens Days */
		reply = rtc[which].days1;
		break;

	case 0xA:	/* Units Months */
		reply = rtc[which].months2;
		break;

	case 0xB:	/* Tens Months */
		reply = rtc[which].months1;
		break;

	case 0xC:	/* Units Years */
		reply = rtc[which].years2;
		break;

	case 0xD:	/* Tens Years */
		reply = rtc[which].years1;
		break;

	case 0xE:	/* Day of Week */
		reply = rtc[which].wday;
		break;

	case 0xF:	/* Clock Setting & Interrupt Registers */
		if (rtc[which].control & ctl_intsel)
			/* interrupt register */
			reply = rtc[which].int_ctl;
		else
		{	/* clock setting register */
			if (rtc[which].clk_set & clk_set_24)
				/* 24-hour mode */
				reply = rtc[which].clk_set & ~clk_set_pm;
			else
				/* 12-hour mode */
				reply = rtc[which].clk_set;
		}
		break;

	default:
		reply = 0;
		break;
	}

	return reply;
}


void mm58274c_w(int which, int offset, int data)
{
	offset &= 0xf;
	data &= 0xf;

	switch (offset)
	{
	case 0x0:	/* Control Register (test mode and interrupt not emulated) */
		if ((! (rtc[which].control & ctl_intstop)) && (data & ctl_intstop))
			/* interrupt stop */
			timer_enable(rtc[which].interrupt_timer, 0);
		else if ((rtc[which].control & ctl_intstop) && (! (data & ctl_intstop)))
		{
			/* interrupt run */
			double period = interrupt_period_table[rtc[which].int_ctl & int_ctl_dly];

			timer_adjust(rtc[which].interrupt_timer, period, which, rtc[which].int_ctl & int_ctl_rpt ? period : 0.);
		}
		if (data & ctl_clkstop)
			/* stopping the clock clears the tenth counter */
			rtc[which].tenths = 0;
		rtc[which].control = data;
		break;

	case 0x1:	/* Tenths of Seconds: cannot be written */
		break;

	case 0x2:	/* Units Seconds */
		rtc[which].seconds2 = data;
		break;

	case 0x3:	/* Tens Seconds */
		rtc[which].seconds1 = data;
		break;

	case 0x4:	/* Units Minutes */
		rtc[which].minutes2 = data;
		break;

	case 0x5:	/* Tens Minutes */
		rtc[which].minutes1 = data;
		break;

	case 0x6:	/* Units Hours */
		rtc[which].hours2 = data;
		break;

	case 0x7:	/* Tens Hours */
		rtc[which].hours1 = data;
		break;

	case 0x8:	/* Units Days */
		rtc[which].days2 = data;
		break;

	case 0x9:	/* Tens Days */
		rtc[which].days1 = data;
		break;

	case 0xA:	/* Units Months */
		rtc[which].months2 = data;
		break;

	case 0xB:	/* Tens Months */
		rtc[which].months1 = data;
		break;

	case 0xC:	/* Units Years */
		rtc[which].years2 = data;
		break;

	case 0xD:	/* Tens Years */
		rtc[which].years1 = data;
		break;

	case 0xE:	/* Day of Week */
		rtc[which].wday = data;
		break;

	case 0xF:	/* Clock Setting & Interrupt Registers */
		if (rtc[which].control & ctl_intsel)
		{
			/* interrupt register (not emulated) */
			rtc[which].int_ctl = data;
			if (! (rtc[which].control & ctl_intstop))
			{
				/* interrupt run */
				double period = interrupt_period_table[rtc[which].int_ctl & int_ctl_dly];

				timer_adjust(rtc[which].interrupt_timer, period, which, rtc[which].int_ctl & int_ctl_rpt ? period : 0.);
			}
		}
		else
		{
			/* clock setting register */
			rtc[which].clk_set = data;
			#if 0
				if (rtc[which].clk_set & clk_set_24)
					/* 24-hour mode */
					rtc[which].clk_set &= ~clk_set_pm;
			#endif
		}
		break;
	}
}


/*
	Set RTC interrupt flag
*/
static void rtc_interrupt_callback(int which)
{
	rtc[which].status |= st_if;
}


/*
	Increment RTC clock (timed interrupt every 1/10s)
*/
static void increment_rtc(int which)
{
	if (! (rtc[which].control & ctl_clkstop))
	{
		rtc[which].status |= st_dcf;

		if ((++rtc[which].tenths) == 10)
		{
			rtc[which].tenths = 0;

			if ((++rtc[which].seconds2) == 10)
			{
				rtc[which].seconds2 = 0;

				if ((++rtc[which].seconds1) == 6)
				{
					rtc[which].seconds1 = 0;

					if ((++rtc[which].minutes2) == 10)
					{
						rtc[which].minutes2 = 0;

						if ((++rtc[which].minutes1) == 6)
						{
							rtc[which].minutes1 = 0;

							if ((++rtc[which].hours2) == 10)
							{
								rtc[which].hours2 = 0;

								rtc[which].hours1++;
							}

							/* handle wrap-around */
							if ((! (rtc[which].clk_set & clk_set_24))
									&& ((rtc[which].hours1*10 + rtc[which].hours2) == 12))
							{
								rtc[which].clk_set ^= clk_set_pm;
							}
							if ((! (rtc[which].clk_set & clk_set_24))
									&& ((rtc[which].hours1*10 + rtc[which].hours2) == 13))
							{
								rtc[which].hours1 = 0;
								rtc[which].hours2 = 1;
							}

							if ((rtc[which].clk_set & clk_set_24)
								&& ((rtc[which].hours1*10 + rtc[which].hours2) == 24))
							{
								rtc[which].hours1 = rtc[which].hours2 = 0;
							}

							/* increment day if needed */
							if ((rtc[which].clk_set & clk_set_24)
								? ((rtc[which].hours1*10 + rtc[which].hours2) == 0)
								: (((rtc[which].hours1*10 + rtc[which].hours2) == 12)
									&& (! (rtc[which].clk_set & clk_set_pm))))
							{
								int days_in_month;

								if ((++rtc[which].days2) == 10)
								{
									rtc[which].days2 = 0;

									rtc[which].days1++;
								}

								if ((++rtc[which].wday) == 8)
									rtc[which].wday = 1;

								{
									static const int days_in_month_array[] =
									{
										31,28,31, 30,31,30,
										31,31,30, 31,30,31
									};

									if (((rtc[which].months1*10 + rtc[which].months2) != 2) || (rtc[which].clk_set & clk_set_leap))
										days_in_month = days_in_month_array[rtc[which].months1*10 + rtc[which].months2 - 1];
									else
										days_in_month = 29;
								}


								if ((rtc[which].days1*10 + rtc[which].days2) == days_in_month+1)
								{
									rtc[which].days1 = 0;
									rtc[which].days2 = 1;

									if ((++rtc[which].months2) == 10)
									{
										rtc[which].months2 = 0;

										rtc[which].months1++;
									}

									if ((rtc[which].months1*10 + rtc[which].months2) == 13)
									{
										rtc[which].months1 = 0;
										rtc[which].months2 = 1;

										rtc[which].clk_set = (rtc[which].clk_set & ~clk_set_leap)
															| ((rtc[which].clk_set + clk_set_leap_inc) & clk_set_leap);

										if ((++rtc[which].years2) == 10)
										{
											rtc[which].years2 = 0;

											if ((++rtc[which].years1) == 10)
												rtc[which].years1 = 0;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
}
