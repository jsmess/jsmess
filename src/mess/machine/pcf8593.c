/*

	Philips PCF8593 CMOS clock/calendar circuit

	(c) 2001-2007 Tim Schuerewegen

*/

#include "pcf8593.h"

#include <time.h>

#define LOG_LEVEL  1
#define _logerror(level,...)  if (LOG_LEVEL > level) logerror(__VA_ARGS__)

#define RTC_MODE_NONE  0
#define RTC_MODE_SEND  1
#define RTC_MODE_RECV  2

// get/set date
#define RTC_GET_DATE_YEAR       ((rtc.data[5] >> 6) & 3)
#define RTC_SET_DATE_YEAR(x)    rtc.data[5] = (rtc.data[5] & 0x3F) | (((x) % 4) << 6)
#define RTC_GET_DATE_MONTH      bcd_to_dec( rtc.data[6])
#define RTC_SET_DATE_MONTH(x)   rtc.data[6] = dec_to_bcd( x)
#define RTC_GET_DATE_DAY        (bcd_to_dec( rtc.data[5] & 0x3F))
#define RTC_SET_DATE_DAY(x)     rtc.data[5] = (rtc.data[5] & 0xC0) | dec_to_bcd( x)

// get/set time
#define RTC_GET_TIME_HOUR       bcd_to_dec( rtc.data[4])
#define RTC_SET_TIME_HOUR(x)    rtc.data[4] = dec_to_bcd( x)
#define RTC_GET_TIME_MINUTE     bcd_to_dec( rtc.data[3])
#define RTC_SET_TIME_MINUTE(x)  rtc.data[3] = dec_to_bcd( x)
#define RTC_GET_TIME_SECOND     bcd_to_dec( rtc.data[2])
#define RTC_SET_TIME_SECOND(x)  rtc.data[2] = dec_to_bcd( x)

static void pcf8593_clear_buffer_rx( void);
static TIMER_CALLBACK( pcf8593_timer_callback );

typedef struct
{
	UINT8 *data;
	UINT32 size;
	int pin_scl, pin_sda, inp;
	BOOL active;
	int bits;
	UINT8 data_recv_index, data_recv[50];
	UINT8 mode, pos;
	emu_timer *timer;
} PCF8593;

static PCF8593 rtc;

void pcf8593_init( void)
{
	_logerror( 0, "pcf8593_init\n");
	memset( &rtc, 0, sizeof( rtc));
	rtc.size = 16;
	rtc.data = malloc( rtc.size);
	rtc.timer = timer_alloc( pcf8593_timer_callback );
	timer_adjust( rtc.timer, ATTOTIME_IN_SEC(1), 0, ATTOTIME_IN_SEC(1));
	pcf8593_reset();
}

void pcf8593_exit( void)
{
	_logerror( 0, "pcf8593_exit\n");
	free( rtc.data);
}

void pcf8593_reset( void)
{
	_logerror( 0, "pcf8593_reset\n");
	rtc.pin_scl = 1;
	rtc.pin_sda = 1;
	rtc.active  = FALSE;
	rtc.inp     = 0;
	rtc.mode    = RTC_MODE_RECV;
	rtc.bits    = 0;
	pcf8593_clear_buffer_rx();
	rtc.pos     = 0;
}

void pcf8593_pin_scl( int data)
{
	// send bit
	if ((rtc.active) && (!rtc.pin_scl) && (data))
	{
		switch (rtc.mode)
		{
			// HOST -> RTC
			case RTC_MODE_RECV :
			{
				// get bit
	    		if (rtc.pin_sda) rtc.data_recv[rtc.data_recv_index] = rtc.data_recv[rtc.data_recv_index] | (0x80 >> rtc.bits);
				rtc.bits++;
				// bit 9 = end
				if (rtc.bits > 8)
				{
					_logerror( 2, "pcf8593_write_byte(%02X)\n", rtc.data_recv[rtc.data_recv_index]);
					// enter receive mode when 1st byte = 0xA3
					if ((rtc.data_recv[0] == 0xA3) && (rtc.data_recv_index == 0))
					{
  						rtc.mode = RTC_MODE_SEND;
					}
					// A2 + xx = "read from pos xx" command
					if ((rtc.data_recv[0] == 0xA2) && (rtc.data_recv_index == 1))
					{
						rtc.pos = rtc.data_recv[1];
					}
					// A2 + xx + .. = write byte
					if ((rtc.data_recv[0] == 0xA2) && (rtc.data_recv_index >= 2))
					{
						UINT8 rtc_pos, rtc_val;
						rtc_pos = rtc.data_recv[1] + (rtc.data_recv_index - 2);
						rtc_val = rtc.data_recv[rtc.data_recv_index];
						//if (rtc_pos == 0) rtc_val = rtc_val & 3; // what is this doing here?
						rtc.data[rtc_pos] = rtc_val;
					}
					// next byte
					rtc.bits = 0;
  					rtc.data_recv_index++;
				}
			}
			break;
			// RTC -> HOST
			case RTC_MODE_SEND :
			{
				// set bit
				rtc.inp = (rtc.data[rtc.pos] >> (7 - rtc.bits)) & 1;
				rtc.bits++;
				// bit 9 = end
				if (rtc.bits > 8)
				{
					_logerror( 2, "pcf8593_read_byte(%02X)\n", rtc.data[rtc.pos]);
					// end ?
  					if (rtc.pin_sda)
					{
						_logerror( 2, "pcf8593 end\n");
  						rtc.mode = RTC_MODE_RECV;
           				pcf8593_clear_buffer_rx();
					}
					// next byte
					rtc.bits = 0;
					rtc.pos++;
				}
			}
			break;
		}
	}
	// save scl
	rtc.pin_scl = data;
}

void pcf8593_pin_sda_w( int data)
{
	// clock is high
	if (rtc.pin_scl)
	{
		// log init I2C
		if (data) _logerror( 1, "pcf8593 init i2c\n");
		// start condition (high to low when clock is high)
		if ((!data) && (rtc.pin_sda))
		{
			_logerror( 1, "pcf8593 start condition\n");
			rtc.active          = TRUE;
			rtc.bits            = 0;
			rtc.data_recv_index = 0;
			pcf8593_clear_buffer_rx();
			//rtc.pos = 0;
		}
		// stop condition (low to high when clock is high)
		if ((data) && (!rtc.pin_sda))
		{
			_logerror( 1, "pcf8593 stop condition\n");
			rtc.active = FALSE;
		}
	}
	// save sda
	rtc.pin_sda = data;
}

int pcf8593_pin_sda_r( void)
{
	return rtc.inp;
}

void pcf8593_clear_buffer_rx( void)
{
	memset( &rtc.data_recv[0], 0, sizeof( rtc.data_recv));
	rtc.data_recv_index = 0;
}

static UINT8 dec_to_bcd( UINT8 data)
{
	return ((data / 10) << 4) | ((data % 10) << 0);
}

static UINT8 bcd_to_dec( UINT8 data)
{
	if ((data & 0x0F) >= 0x0A) data = data - 0x0A + 0x10;
	if ((data & 0xF0) >= 0xA0) data = data - 0xA0;
	return (data & 0x0F) + (((data & 0xF0) >> 4) * 10);
}

void pcf8593_set_time( int hour, int minute, int second)
{
	RTC_SET_TIME_HOUR( hour);
	RTC_SET_TIME_MINUTE( minute);
	RTC_SET_TIME_SECOND( second);
	rtc.data[1] = 0; // hundreds of a seconds
}

void pcf8593_set_date( int year, int month, int day)
{
	RTC_SET_DATE_YEAR( year);
	RTC_SET_DATE_MONTH( month);
	RTC_SET_DATE_DAY( day);
}

static int get_days_in_month( int year, int month)
{
	static const int table[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	if ((month == 2) && ((year & 0x03) == 0)) return 29;
	return table[month-1];
}

static TIMER_CALLBACK( pcf8593_timer_callback )
{
	int value;
	_logerror( 2, "pcf8593_timer_callback (%d)\n", param);
	// check if counting is enabled
	if (rtc.data[0] & 0x80) return;
	// increment second
	value = RTC_GET_TIME_SECOND;
	if (value < 59)
	{
		RTC_SET_TIME_SECOND( value + 1);
	}
	else
	{
		RTC_SET_TIME_SECOND( 0);
		// increment minute
		value = RTC_GET_TIME_MINUTE;
		if (value < 59)
		{
			RTC_SET_TIME_MINUTE( value + 1);
		}
		else
		{
			RTC_SET_TIME_MINUTE( 0);
			// increment hour
			value = RTC_GET_TIME_HOUR;
			if (value < 23)
			{
				RTC_SET_TIME_HOUR( value + 1);
			}
			else
			{
				RTC_SET_TIME_HOUR( 0);
				// increment day
				value = RTC_GET_DATE_DAY;
				if (value < get_days_in_month( RTC_GET_DATE_YEAR, RTC_GET_DATE_MONTH))
				{
					RTC_SET_DATE_DAY( value + 1);
				}
				else
				{
					RTC_SET_DATE_DAY( 1);
					// increase month
					value = RTC_GET_DATE_MONTH;
					if (value < 12)
					{
						RTC_SET_DATE_MONTH( value + 1);
					}
					else
					{
						RTC_SET_DATE_MONTH( 1);
						// increase year
						RTC_SET_DATE_YEAR( RTC_GET_DATE_YEAR + 1);
					}
				}
			}
		}
	}
}

void pcf8593_load( mame_file *file)
{
	mame_system_time systime;
	_logerror( 0, "pcf8593_load (%p)\n", file);
	mame_fread( file, rtc.data, rtc.size);
	mame_get_current_datetime( Machine, &systime);
	pcf8593_set_date( systime.local_time.year, systime.local_time.month + 1, systime.local_time.mday);
	pcf8593_set_time( systime.local_time.hour, systime.local_time.minute, systime.local_time.second);
}

void pcf8593_save( mame_file *file)
{
	_logerror( 0, "pcf8593_save (%p)\n", file);
	mame_fwrite( file, rtc.data, rtc.size);
}

/*
NVRAM_HANDLER( pcf8593 )
{
	_logerror( 0, "nvram_handler_pcf8593 (%p/%d)\n", file, read_or_write);
	if (read_or_write)
	{
		pcf8593_save( file);
	}
	else
	{
		if (file)
		{
			pcf8593_load( file);
		}
		else
		{
			memset( rtc.data, 0, rtc.size);
		}
	}
}
*/
