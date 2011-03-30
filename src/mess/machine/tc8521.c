/*********************************************************************

    tc8521.h

    Toshiba TC8251 Real Time Clock code

    (or RICOH RF5C01A)

    Registers:

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
            bit 3: 1 Hz enable
            bit 2: 16 Hz enable
            bit 1: timer reset
            bit 0: alarm reset

*********************************************************************/

#include "emu.h"
#include "tc8521.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

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

#define VERBOSE 0
#define LOG(x) do { if (VERBOSE) logerror x; } while (0)

#define ALARM_OUTPUT_16HZ	0x01
#define ALARM_OUTPUT_1HZ	0x02
#define ALARM_OUTPUT_ALARM	0X04


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _tc8521_t tc8521_t;
struct _tc8521_t
{
	/* 4 register pages (timer, alarm, ram1 and ram2) */
    unsigned char registers[16*4];
	UINT32 alarm_outputs;
	UINT32 thirty_two_hz_counter;
};


/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE tc8521_t *get_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == TC8521);
	return (tc8521_t *) downcast<legacy_device_base *>(device)->token();
}


INLINE const tc8521_interface *get_interface(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == TC8521);
	return (tc8521_interface *) device->baseconfig().static_config();
}


/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

const tc8521_interface default_tc8521_interface = {0, };


/***************************************************************************
    LOCAL VARIABLES
***************************************************************************/

/* mask data with these values when writing */
static const UINT8 rtc_write_masks[16*4]=
{
	0x0f,0x07,0x0f,0x07,0x0f,0x03,0x07,0x0f,0x03,0x0f,0x03,0x0f,0x0f,0x0f, 0x0f, 0x0f,
	0x00,0x00,0x0f,0x07,0x0f,0x03,0x07,0x0f,0x03,0x00,0x01,0x03,0x00,0x0f, 0x0f, 0x0f,
	0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f, 0x0f, 0x0f,
	0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f, 0x0f, 0x0f,
};



/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    tc8521_load_stream - read tc8521 data from
    supplied file
-------------------------------------------------*/

void tc8521_load_stream(device_t *device, emu_file *file)
{
	tc8521_t *rtc = get_token(device);

	if (file)
	{
		file->read(&rtc->registers[0], sizeof(unsigned char)*16*4);
		file->read(&rtc->alarm_outputs, sizeof(UINT32));
		file->read(&rtc->thirty_two_hz_counter, sizeof(UINT32));
	}
}



/*-------------------------------------------------
    tc8521_save_stream - write tc8521 data to
    supplied file
-------------------------------------------------*/

void tc8521_save_stream(device_t *device, emu_file *file)
{
	tc8521_t *rtc = get_token(device);

	if (file)
	{
		file->write(&rtc->registers[0], sizeof(unsigned char)*16*4);
		file->write(&rtc->alarm_outputs, sizeof(UINT32));
		file->write(&rtc->thirty_two_hz_counter, sizeof(UINT32));
	}
}



/*-------------------------------------------------
    tc8521_set_alarm_output
-------------------------------------------------*/

static void tc8521_set_alarm_output(device_t *device)
{
	tc8521_t *rtc = get_token(device);
	const tc8521_interface *rtc_interface = get_interface(device);
	unsigned char alarm_output;

	alarm_output = 0;

	/* what happens when all are enabled? I assume they are all or'd together */

	/* 16 Hz enabled? */
    if ((rtc->registers[TC8521_RESET_REGISTER] & (1<<2))==0)
    {
		/* yes */

		/* add in state of 16 Hz output */
		alarm_output |= (rtc->alarm_outputs & ALARM_OUTPUT_16HZ);
	}

	if ((rtc->registers[TC8521_RESET_REGISTER] & (1<<3))==0)
	{
		/* yes */
		/* add in stat of 1 Hz output */
		alarm_output |= ((rtc->alarm_outputs & ALARM_OUTPUT_1HZ)>>1);
	}

	alarm_output |= ((rtc->alarm_outputs & ALARM_OUTPUT_ALARM)>>2);

	/* if it's not enabled, then there is no output */
	if (!((rtc->registers[TC8521_MODE_REGISTER] & (1<<2))!=0))
	{
		/* alarm output enabled? */
		alarm_output &= ~1;
	}

    if (rtc_interface->alarm_output_callback != NULL)
    {
		(*rtc_interface->alarm_output_callback)(device, alarm_output);
	}
}



/*-------------------------------------------------
    tc8521_alarm_check
-------------------------------------------------*/

static void tc8521_alarm_check(device_t *device)
{
	tc8521_t *rtc = get_token(device);

	rtc->alarm_outputs &= ~ALARM_OUTPUT_ALARM;
	if (
		(rtc->registers[TC8521_TIMER_1_MINUTE_COUNTER]==rtc->registers[TC8521_ALARM_1_MINUTE_REGISTER]) &&
		(rtc->registers[TC8521_TIMER_10_MINUTE_COUNTER]==rtc->registers[TC8521_ALARM_10_MINUTE_REGISTER]) &&
		(rtc->registers[TC8521_TIMER_1_HOUR_COUNTER]==rtc->registers[TC8521_ALARM_1_HOUR_REGISTER]) &&
		(rtc->registers[TC8521_TIMER_10_HOUR_COUNTER]==rtc->registers[TC8521_ALARM_10_HOUR_REGISTER]) &&
		(rtc->registers[TC8521_TIMER_1_DAY_COUNTER]==rtc->registers[TC8521_ALARM_1_DAY_REGISTER]) &&
		(rtc->registers[TC8521_TIMER_10_DAY_COUNTER]==rtc->registers[TC8521_ALARM_10_DAY_REGISTER]) &&
		(rtc->registers[TC8521_TIMER_DAY_OF_THE_WEEK_COUNTER]==rtc->registers[TC8521_ALARM_DAY_OF_THE_WEEK_REGISTER])
		)
	{
		rtc->alarm_outputs |= ALARM_OUTPUT_ALARM;
	}

	tc8521_set_alarm_output(device);
}



/*-------------------------------------------------
    tc8521_timer_callback
-------------------------------------------------*/

static TIMER_CALLBACK(tc8521_timer_callback)
{
	device_t *device = (device_t *)ptr;
	tc8521_t *rtc = get_token(device);

	/* Assumption how it works */
	/* 16 Hz output = 16 cycles per second, 16 cycles of high-low from counter */

	/* toggle 16 Hz wave state */
	rtc->alarm_outputs ^= ALARM_OUTPUT_16HZ;
	/* set in alarm output */
	tc8521_set_alarm_output(device);

	rtc->thirty_two_hz_counter++;

	if (rtc->thirty_two_hz_counter==16)
	{
		rtc->thirty_two_hz_counter = 0;

		/* toggle 1 Hz output */
		rtc->alarm_outputs ^= ALARM_OUTPUT_1HZ;
		/* set in alarm output */
		tc8521_set_alarm_output(device);

		if ((rtc->alarm_outputs & ALARM_OUTPUT_1HZ)==0)
		{
			/* timer enable? */
			if ((rtc->registers[0x0d] & (1<<3))!=0)
			{
			   /* seconds; units */
			   rtc->registers[0]++;

			   if (rtc->registers[0]==10)
			   {
				   rtc->registers[0] = 0;

				   /* seconds; tens */
				   rtc->registers[1]++;

				   if (rtc->registers[1]==6)
				   {
					  rtc->registers[1] = 0;

					  /* minutes; units */
					  rtc->registers[2]++;

					  if (rtc->registers[2]==10)
					  {
						  rtc->registers[2] = 0;

						  /* minutes; tens */
						  rtc->registers[3]++;

						  if (rtc->registers[3] == 6)
						  {
							 rtc->registers[3] = 0;

							 /* hours; units */
							 rtc->registers[4]++;

							 if (rtc->registers[4] == 10)
							 {
								rtc->registers[4] = 0;

								/* hours; tens */
								rtc->registers[4]++;

								if (rtc->registers[4] == 24)
								{
									/* TODO: finish rest of increments here! */
									rtc->registers[4] = 0;
								}
							 }
						  }
					  }

					  tc8521_alarm_check(device);
				   }
			   }
			}
		}
	}
}



/*-------------------------------------------------
    DEVICE_START( tc8521 )
-------------------------------------------------*/

static DEVICE_START( tc8521 )
{
	tc8521_t *rtc = get_token(device);
	memset(rtc, 0, sizeof(*rtc));
	device->machine().scheduler().timer_pulse(attotime::from_hz(32), FUNC(tc8521_timer_callback), 0, (void *) device);
}



/*-------------------------------------------------
    tc8521_r
-------------------------------------------------*/

READ8_DEVICE_HANDLER(tc8521_r)
{
	tc8521_t *rtc = get_token(device);
	UINT32 register_index;

	switch (offset)
	{
		/* control registers */
		case 0x0d:
		case 0x0e:
		case 0x0f:
			LOG(("8521 RTC R: %04x %02x\n", offset, rtc->registers[offset]));
			return rtc->registers[offset];

		default:
			break;
	}

	/* register in selected page */
	register_index = ((rtc->registers[TC8521_MODE_REGISTER] & 0x03)<<4) | (offset & 0x0f);
	LOG(("8521 RTC R: %04x %02x\n", offset, rtc->registers[register_index]));
	/* data from selected page */
	return rtc->registers[register_index];
}



/*-------------------------------------------------
    tc8521_w
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER(tc8521_w)
{
	tc8521_t *rtc = get_token(device);
	UINT32 register_index;

	LOG(("8521 RTC W: %04x %02x\n", offset, data));

	switch (offset)
	{
		case 0x0D:
		{
			if (data & 0x08)
			{
				LOG(("timer enable\n"));
			}

			if (data & 0x04)
			{
				LOG(("alarm enable\n"));
			}

			logerror("page %02x selected\n", data & 0x03);
		}
		break;

		case 0x0e:
		{
			if (data & 0x08)
			{
				LOG(("test 3\n"));
			}

			if (data & 0x04)
			{
				LOG(("test 2\n"));
			}

			if (data & 0x02)
			{
				LOG(("test 1\n"));
			}

			if (data & 0x01)
			{
				LOG(("test 0\n"));
			}
		}
		break;

		case 0x0f:
		{
			if ((data & 0x08)==0)
			{
			   LOG(("1 Hz enable\n"));
			}

			if ((data & 0x04)==0)
			{
			   LOG(("16 Hz enable\n"));
			}

			if (data & 0x02)
			{
			   LOG(("reset timer\n"));
			}

			if (data & 0x01)
			{
			   LOG(("reset alarm\n"));
			}
		}
		break;

		default:
			break;
	}

	switch (offset)
	{
		/* control registers */
		case 0x0d:
		case 0x0e:
		case 0x0f:
			rtc->registers[offset] = data & rtc_write_masks[offset];
			return;
		default:
			break;
	}

	/* register in selected page */
	register_index = ((rtc->registers[TC8521_MODE_REGISTER] & 0x03)<<4) | (offset & 0x0f);

	/* write and mask data */
	rtc->registers[register_index] = data & rtc_write_masks[register_index];
}



/*-------------------------------------------------
    DEVICE_GET_INFO( tc8521 )
-------------------------------------------------*/

DEVICE_GET_INFO( tc8521 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(tc8521_t);					break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(tc8521);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/* Nothing */								break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Toshiba 8521 RTC");		break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Toshiba 8521 RTC");		break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						/* Nothing */								break;
	}
}

DEFINE_LEGACY_DEVICE(TC8521, tc8521);
