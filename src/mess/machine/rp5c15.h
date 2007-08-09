/*
 *  Ricoh RP5C15 RTC header
 */

#ifndef RP5C15_H_
#define RP5C15_H_

#include "driver.h"

struct rp5c15_interface
{
	void (*alarm_irq_callback)(int state);
};

struct rp5c15
{
	struct
	{
		unsigned char sec_1;
		unsigned char sec_10;
		unsigned char min_1;
		unsigned char min_10;
		unsigned char hour_1;
		unsigned char hour_10;
		unsigned char dayofweek;
		unsigned char day_1;
		unsigned char day_10;
		unsigned char month_1;
		unsigned char month_10;
		unsigned char year_1;
		unsigned char year_10;
	} systime;
	struct
	{
		unsigned char min_1;
		unsigned char min_10;
		unsigned char hour_1;
		unsigned char hour_10;
		unsigned char dayofweek;
		unsigned char day_1;
		unsigned char day_10;
	} alarm;
	int clkout;
	int adjust;
	int hour24;  // 12/24 hour clock
	int leap;  // Years until next leap year (0-3, 0 = this year is leap year)
	int mode;
	int test;
	int reset;
	int pulse_count;
	int pulse1_state;
	int pulse16_state;
	void (*timer_callback)(int state);
	void (*alarm_callback)(int state);
};  //  Ricoh RP5C15

void rp5c15_init(struct rp5c15_interface*);

void rtc_add_second(void);
void rtc_add_minute(void);
void rtc_add_day(void);
void rtc_add_month(void);

TIMER_CALLBACK(rtc_alarm_pulse);

READ16_HANDLER( rp5c15_r );
WRITE16_HANDLER( rp5c15_w );

#endif /*RP5C15_H_*/
