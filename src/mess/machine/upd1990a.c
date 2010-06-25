/**********************************************************************

    NEC uPD1990AC Serial I/O Calendar & Clock emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

/*

    TODO:

    - test mode

*/

#include "emu.h"
#include "upd1990a.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

enum
{
	UPD1990A_MODE_REGISTER_HOLD = 0,
	UPD1990A_MODE_SHIFT,
	UPD1990A_MODE_TIME_SET,
	UPD1990A_MODE_TIME_READ,
	UPD1990A_MODE_TP_64HZ_SET,
	UPD1990A_MODE_TP_256HZ_SET,
	UPD1990A_MODE_TP_2048HZ_SET,
	UPD1990A_MODE_TEST,
};

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _upd1990a_t upd1990a_t;
struct _upd1990a_t
{
	devcb_resolved_write_line	out_data_func;
	devcb_resolved_write_line	out_tp_func;

	UINT8 time_counter[5];		/* time counter */
	UINT8 shift_reg[5];			/* shift register */

	int oe;						/* output enable */
	int cs;						/* chip select */
	int stb;					/* strobe */
	int data_in;				/* data in */
	int data_out;				/* data out */
	int c;						/* command */
	int clk;					/* shift clock */
	int tp;						/* time pulse */

	/* timers */
	emu_timer *clock_timer;
	emu_timer *data_out_timer;
	emu_timer *tp_timer;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE upd1990a_t *get_safe_token(running_device *device)
{
	assert(device != NULL);
	assert(device->type() == UPD1990A);
	return (upd1990a_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE const upd1990a_interface *get_interface(running_device *device)
{
	assert(device != NULL);
	assert((device->type() == UPD1990A));
	return (const upd1990a_interface *) device->baseconfig().static_config();
}

INLINE UINT8 convert_to_bcd(int val)
{
	return ((val / 10) << 4) | (val % 10);
}

INLINE int bcd_to_integer(UINT8 val)
{
	return (((val & 0xf0) >> 4) * 10) + (val & 0x0f);
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    upd1990a_oe_w - output enable write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( upd1990a_oe_w )
{
	upd1990a_t *upd1990a = get_safe_token(device);

	upd1990a->oe = state;

	if (LOG) logerror("UPD1990A OE %u\n", state);
}

/*-------------------------------------------------
    upd1990a_oe_w - output enable write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( upd1990a_cs_w )
{
	upd1990a_t *upd1990a = get_safe_token(device);

	upd1990a->cs = state;

	if (LOG) logerror("UPD1990A CS %u\n", state);
}

/*-------------------------------------------------
    upd1990a_stb_w - strobe write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( upd1990a_stb_w )
{
	upd1990a_t *upd1990a = get_safe_token(device);

	if (LOG) logerror("UPD1990A STB %u\n", state);

	upd1990a->stb = state;

	if (upd1990a->cs && upd1990a->stb && !upd1990a->clk)
	{
		switch (upd1990a->c)
		{
		case UPD1990A_MODE_REGISTER_HOLD:
			if (LOG) logerror("UPD1990A Register Hold Mode\n");

			/* enable time counter */
			timer_enable(upd1990a->clock_timer, 1);

			/* 1 Hz data out pulse */
			upd1990a->data_out = 1;
			timer_adjust_periodic(upd1990a->data_out_timer, attotime_zero, 0, ATTOTIME_IN_HZ(1*2));

			/* 64 Hz time pulse */
			timer_adjust_periodic(upd1990a->tp_timer, attotime_zero, 0, ATTOTIME_IN_HZ(64*2));
			break;

		case UPD1990A_MODE_SHIFT:
			if (LOG) logerror("UPD1990A Shift Mode\n");

			/* enable time counter */
			timer_enable(upd1990a->clock_timer, 1);

			/* disable data out pulse */
			timer_enable(upd1990a->data_out_timer, 0);

			/* output LSB of shift register */
			upd1990a->data_out = BIT(upd1990a->shift_reg[0], 0);
			devcb_call_write_line(&upd1990a->out_data_func, upd1990a->data_out);

			/* 32 Hz time pulse */
			timer_adjust_periodic(upd1990a->tp_timer, attotime_zero, 0, ATTOTIME_IN_HZ(32*2));
			break;

		case UPD1990A_MODE_TIME_SET:
			{
			int i;

			if (LOG) logerror("UPD1990A Time Set Mode\n");
			if (LOG) logerror("UPD1990A Shift Register %02x%02x%02x%02x%02x\n", upd1990a->shift_reg[4], upd1990a->shift_reg[3], upd1990a->shift_reg[2], upd1990a->shift_reg[1], upd1990a->shift_reg[0]);

			/* disable time counter */
			timer_enable(upd1990a->clock_timer, 0);

			/* disable data out pulse */
			timer_enable(upd1990a->data_out_timer, 0);

			/* output LSB of shift register */
			upd1990a->data_out = BIT(upd1990a->shift_reg[0], 0);
			devcb_call_write_line(&upd1990a->out_data_func, upd1990a->data_out);

			/* load shift register data into time counter */
			for (i = 0; i < 5; i++)
			{
				upd1990a->time_counter[i] = upd1990a->shift_reg[i];
			}

			/* 32 Hz time pulse */
			timer_adjust_periodic(upd1990a->tp_timer, attotime_zero, 0, ATTOTIME_IN_HZ(32*2));
			}
			break;

		case UPD1990A_MODE_TIME_READ:
			{
			int i;

			if (LOG) logerror("UPD1990A Time Read Mode\n");

			/* enable time counter */
			timer_enable(upd1990a->clock_timer, 1);

			/* load time counter data into shift register */
			for (i = 0; i < 5; i++)
			{
				upd1990a->shift_reg[i] = upd1990a->time_counter[i];
			}

			if (LOG) logerror("UPD1990A Shift Register %02x%02x%02x%02x%02x\n", upd1990a->shift_reg[4], upd1990a->shift_reg[3], upd1990a->shift_reg[2], upd1990a->shift_reg[1], upd1990a->shift_reg[0]);

			/* 512 Hz data out pulse */
			upd1990a->data_out = 1;
			timer_adjust_periodic(upd1990a->data_out_timer, attotime_zero, 0, ATTOTIME_IN_HZ(512*2));

			/* 32 Hz time pulse */
			timer_adjust_periodic(upd1990a->tp_timer, attotime_zero, 0, ATTOTIME_IN_HZ(32*2));
			}
			break;

		case UPD1990A_MODE_TP_64HZ_SET:
			if (LOG) logerror("UPD1990A TP = 64 Hz Set Mode\n");

			/* 64 Hz time pulse */
			timer_adjust_periodic(upd1990a->tp_timer, attotime_zero, 0, ATTOTIME_IN_HZ(64*2));
			break;

		case UPD1990A_MODE_TP_256HZ_SET:
			if (LOG) logerror("UPD1990A TP = 256 Hz Set Mode\n");

			/* 256 Hz time pulse */
			timer_adjust_periodic(upd1990a->tp_timer, attotime_zero, 0, ATTOTIME_IN_HZ(256*2));
			break;

		case UPD1990A_MODE_TP_2048HZ_SET:
			if (LOG) logerror("UPD1990A TP = 2048 Hz Set Mode\n");

			/* 2048 Hz time pulse */
			timer_adjust_periodic(upd1990a->tp_timer, attotime_zero, 0, ATTOTIME_IN_HZ(2048*2));
			break;

		case UPD1990A_MODE_TEST:
			if (LOG) logerror("UPD1990A Test Mode not supported!\n");

			if (upd1990a->oe)
			{
				/* time counter is advanced at 1024 Hz from "Second" counter input */
			}
			else
			{
				/* each counter is advanced at 1024 Hz in parallel, overflow carry does not affect next counter */
			}

			break;
		}
	}
}

/*-------------------------------------------------
    upd1990a_clk_w - shift clock write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( upd1990a_clk_w )
{
	upd1990a_t *upd1990a = get_safe_token(device);

	if (LOG) logerror("UPD1990A CLK %u\n", state);

	if (!upd1990a->clk && state) // rising edge
	{
		if (upd1990a->c == UPD1990A_MODE_SHIFT)
		{
			upd1990a->shift_reg[0] >>= 1;
			upd1990a->shift_reg[0] |= (BIT(upd1990a->shift_reg[1], 0) << 7);

			upd1990a->shift_reg[1] >>= 1;
			upd1990a->shift_reg[1] |= (BIT(upd1990a->shift_reg[2], 0) << 7);

			upd1990a->shift_reg[2] >>= 1;
			upd1990a->shift_reg[2] |= (BIT(upd1990a->shift_reg[3], 0) << 7);

			upd1990a->shift_reg[3] >>= 1;
			upd1990a->shift_reg[3] |= (BIT(upd1990a->shift_reg[4], 0) << 7);

			upd1990a->shift_reg[4] >>= 1;
			upd1990a->shift_reg[4] |= (upd1990a->data_in << 7);

			if (upd1990a->oe)
			{
				upd1990a->data_out = BIT(upd1990a->shift_reg[0], 0);

				if (LOG) logerror("UPD1990A DATA OUT %u\n", upd1990a->data_out);

				devcb_call_write_line(&upd1990a->out_data_func, upd1990a->data_out);
			}
		}
	}

	upd1990a->clk = state;
}

/*-------------------------------------------------
    upd1990a_c0_w - command bit 0 write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( upd1990a_c0_w )
{
	upd1990a_t *upd1990a = get_safe_token(device);

	if (LOG) logerror("UPD1990A C0 %u\n", state);

	upd1990a->c = (upd1990a->c & 0x06) | state;
}

/*-------------------------------------------------
    upd1990a_c1_w - command bit 1 write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( upd1990a_c1_w )
{
	upd1990a_t *upd1990a = get_safe_token(device);

	if (LOG) logerror("UPD1990A C1 %u\n", state);

	upd1990a->c = (upd1990a->c & 0x05) | (state << 1);
}

/*-------------------------------------------------
    upd1990a_c2_w - command bit 2 write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( upd1990a_c2_w )
{
	upd1990a_t *upd1990a = get_safe_token(device);

	if (LOG) logerror("UPD1990A C2 %u\n", state);

	upd1990a->c = (upd1990a->c & 0x03) | (state << 2);
}

/*-------------------------------------------------
    upd1990a_data_in_w - data input write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( upd1990a_data_in_w )
{
	upd1990a_t *upd1990a = get_safe_token(device);

	if (LOG) logerror("UPD1990A DATA IN %u\n", state);

	upd1990a->data_in = state;
}

/*-------------------------------------------------
    upd1990a_data_out_r - data output read
-------------------------------------------------*/

READ_LINE_DEVICE_HANDLER( upd1990a_data_out_r )
{
	upd1990a_t *upd1990a = get_safe_token(device);

	return upd1990a->data_out;
}

/*-------------------------------------------------
    upd1990a_tp_r - timing pulse read
-------------------------------------------------*/

READ_LINE_DEVICE_HANDLER( upd1990a_tp_r )
{
	upd1990a_t *upd1990a = get_safe_token(device);

	return upd1990a->tp;
}

/*-------------------------------------------------
    advance_seconds - advance seconds counter
-------------------------------------------------*/

static void advance_seconds(upd1990a_t *upd1990a)
{
	int days_per_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	int seconds = bcd_to_integer(upd1990a->time_counter[0]);
	int minutes = bcd_to_integer(upd1990a->time_counter[1]);
	int hours = bcd_to_integer(upd1990a->time_counter[2]);
	int days = bcd_to_integer(upd1990a->time_counter[3]);
	int day_of_week = upd1990a->time_counter[4] & 0x0f;
	int month = (upd1990a->time_counter[4] & 0xf0) >> 4;

	seconds++;

	if (seconds > 59)
	{
		seconds = 0;
		minutes++;
	}

	if (minutes > 59)
	{
		minutes = 0;
		hours++;
	}

	if (hours > 23)
	{
		hours = 0;
		days++;
		day_of_week++;
	}

	if (day_of_week > 6)
	{
		day_of_week++;
	}

	if (days > days_per_month[month - 1])
	{
		days = 1;
		month++;
	}

	if (month > 12)
	{
		month = 1;
	}

	upd1990a->time_counter[0] = convert_to_bcd(seconds);
	upd1990a->time_counter[1] = convert_to_bcd(minutes);
	upd1990a->time_counter[2] = convert_to_bcd(hours);
	upd1990a->time_counter[3] = convert_to_bcd(days);
	upd1990a->time_counter[4] = (month << 4) | day_of_week;
}

/*-------------------------------------------------
    TIMER_CALLBACK( clock_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( clock_tick )
{
	running_device *device = (running_device *)ptr;
	upd1990a_t *upd1990a = get_safe_token(device);

	advance_seconds(upd1990a);
}

/*-------------------------------------------------
    TIMER_CALLBACK( tp_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( tp_tick )
{
	running_device *device = (running_device *)ptr;
	upd1990a_t *upd1990a = get_safe_token(device);

	upd1990a->tp = !upd1990a->tp;

	if (LOG) logerror("UPD1990A TP %u\n", upd1990a->tp);

	devcb_call_write_line(&upd1990a->out_tp_func, upd1990a->tp);
}

/*-------------------------------------------------
    TIMER_CALLBACK( data_out_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( data_out_tick )
{
	running_device *device = (running_device *)ptr;
	upd1990a_t *upd1990a = get_safe_token(device);

	upd1990a->data_out = !upd1990a->data_out;

	if (LOG) logerror("UPD1990A DATA OUT TICK %u\n", upd1990a->data_out);

	devcb_call_write_line(&upd1990a->out_data_func, upd1990a->data_out);
}

/*-------------------------------------------------
    DEVICE_START( upd1990a )
-------------------------------------------------*/

static DEVICE_START( upd1990a )
{
	upd1990a_t *upd1990a = get_safe_token(device);
	const upd1990a_interface *intf = get_interface(device);

	/* resolve callbacks */
	devcb_resolve_write_line(&upd1990a->out_data_func, &intf->out_data_func, device);
	devcb_resolve_write_line(&upd1990a->out_tp_func, &intf->out_tp_func, device);

	/* create the timers */
	upd1990a->clock_timer = timer_alloc(device->machine, clock_tick, (void *)device);
	timer_adjust_periodic(upd1990a->clock_timer, attotime_zero, 0, ATTOTIME_IN_HZ(1));

	upd1990a->tp_timer = timer_alloc(device->machine, tp_tick, (void *)device);

	upd1990a->data_out_timer = timer_alloc(device->machine, data_out_tick, (void *)device);

	/* register for state saving */
    state_save_register_global_array(device->machine, upd1990a->time_counter);
    state_save_register_global_array(device->machine, upd1990a->shift_reg);
    state_save_register_global(device->machine, upd1990a->oe);
    state_save_register_global(device->machine, upd1990a->cs);
    state_save_register_global(device->machine, upd1990a->stb);
	state_save_register_global(device->machine, upd1990a->data_in);
    state_save_register_global(device->machine, upd1990a->data_out);
    state_save_register_global(device->machine, upd1990a->c);
    state_save_register_global(device->machine, upd1990a->clk);
    state_save_register_global(device->machine, upd1990a->tp);
}

static DEVICE_RESET( upd1990a )
{
	upd1990a_t *upd1990a = get_safe_token(device);

	mame_system_time curtime, *systime = &curtime;

	mame_get_current_datetime(device->machine, &curtime);

	/* HACK: load time counter from system time */
	upd1990a->time_counter[0] = convert_to_bcd(systime->local_time.second);
	upd1990a->time_counter[1] = convert_to_bcd(systime->local_time.minute);
	upd1990a->time_counter[2] = convert_to_bcd(systime->local_time.hour);
	upd1990a->time_counter[3] = convert_to_bcd(systime->local_time.mday);
	upd1990a->time_counter[4] = systime->local_time.weekday;
	upd1990a->time_counter[4] |= (systime->local_time.month + 1) << 4;
}

/*-------------------------------------------------
    DEVICE_GET_INFO( upd1990a )
-------------------------------------------------*/

DEVICE_GET_INFO( upd1990a )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(upd1990a_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(upd1990a);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(upd1990a);	break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "NEC uPD1990AC");			break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "uPD1990AC RTC");			break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						/* Nothing */								break;
	}
}

DEFINE_LEGACY_DEVICE(UPD1990A, upd1990a);
