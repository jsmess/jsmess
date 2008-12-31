/*
 *  Ricoh RP5C15 RTC header
 */

#ifndef RP5C15_H_
#define RP5C15_H_

#include "driver.h"

typedef struct rp5c15_interface rp5c15_intf;
struct rp5c15_interface
{
	void (*alarm_irq_callback)(int state);
};

typedef struct _rp5c15_t rp5c15_t;
struct _rp5c15_t
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
	void (*timer_fired_func)(int state);
	void (*alarm_callback)(int state);
	const rp5c15_intf* intf;
};  //  Ricoh RP5C15

DEVICE_START(rp5c15);
DEVICE_GET_INFO(rp5c15);

void rtc_add_second(const device_config*);
void rtc_add_minute(const device_config*);
void rtc_add_day(const device_config*);
void rtc_add_month(const device_config*);

TIMER_CALLBACK(rtc_alarm_pulse);

#define RP5C15 DEVICE_GET_INFO_NAME(rp5c15)

#define MDRV_RP5C15_ADD(_tag, _config) \
	MDRV_DEVICE_ADD(_tag, RP5C15, 0) \
	MDRV_DEVICE_CONFIG(_config)

READ16_DEVICE_HANDLER( rp5c15_r );
WRITE16_DEVICE_HANDLER( rp5c15_w );

#endif /*RP5C15_H_*/
