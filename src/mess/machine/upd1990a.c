/*********************************************************************

	upd1990a.c

	NEC uPD1990AC CMOS Digital Integrated Circuit
	Serial I/O Calendar & Clock
	CMOS LSI
	
	Very preliminary emulation
	
	TODO:
		- implement TP settings to 64Hz, 256Hz, 2048Hz (started implementation)
		- implement Set Clock mode
		- implement TEST Mode
		- what about data_in?

*********************************************************************/

#include "driver.h"
#include "upd1990a.h"

#define UPD1990AC_XTAL 32768

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _upd1990a_t upd1990a_t;
struct _upd1990a_t
{
	UINT8 shift_reg[5];	/* 40 Bit Shift Register */
	UINT8 data_in;
	UINT8 data_out;
	UINT8 shift_mode;
	UINT16 freq_divider;
    emu_timer *tp;
};


/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE upd1990a_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == UPD1990A);

	return (upd1990a_t *)device->token;
}


/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

static UINT8 upd1990a_bcd( UINT8 val )
{
	return ((val / 10) << 4) | (val % 10);
}

WRITE8_DEVICE_HANDLER( upd1990a_w )
{
	UINT8 command = data & 0x07;
	upd1990a_t *upd1990a = get_safe_token(device);
	mame_system_time curtime, *systime = &curtime;
	int i;

	if (data & ~0x07)
		logerror("uPD1990AC expects only 3 bits data, found %d > 7\n", data);

	mame_get_current_datetime(device->machine, &curtime);

	/* 
       c0 = command bit 0
       c1 = command bit 1
       c2 = command bit 2

       c2   c1   c0  |  Function
      ----------------------------
        0    0    0  |  Register Hold (TEST Mode released)
        0    0    1  |  Register Shift
        0    1    0  |  Timer Set & Counter Hold
        0    1    1  |  Timer Read
        1    0    0  |  TP = 64Hz Set (TEST Mode released)
        1    0    1  |  TP = 256Hz Set (TEST Mode released)
        1    1    0  |  TP = 2048z Set (TEST Mode released)
        1    1    1  |  TEST Mode Set
	*/

	switch (command)
	{
		case 0x00:
			upd1990a->shift_mode = 0;
			break;
		case 0x01:
			upd1990a->shift_mode = 1;
			upd1990a->data_out = upd1990a->shift_reg[0] & 0x01;
			break;
		case 0x02:	/* Set Clock */
			upd1990a->shift_mode = 0;
			logerror("RTC Set Clock attempt\n");
			/* Not implemented yet */
		case 0x03:	/* Read Clock */
			upd1990a->shift_mode = 0;
			upd1990a->shift_reg[0] = upd1990a_bcd(systime->local_time.second);
			upd1990a->shift_reg[1] = upd1990a_bcd(systime->local_time.minute);
			upd1990a->shift_reg[2] = upd1990a_bcd(systime->local_time.hour);
			upd1990a->shift_reg[3] = upd1990a_bcd(systime->local_time.mday);
			upd1990a->shift_reg[4] = systime->local_time.weekday;
			upd1990a->shift_reg[4] |= systime->local_time.month << 4;
			for(i=0; i<5; i++)
				logerror("%d     ", (upd1990a->shift_reg[i] & 0x0f) + ((upd1990a->shift_reg[i] & 0xf0)>>4)*10);
			logerror("\n");
			break;
		case 0x04:
			upd1990a->freq_divider = 512;
			logerror("RTC Set TP 64Hz attempt\n");
			break;
		case 0x05:
			upd1990a->freq_divider = 128;
			logerror("RTC Set TP 256Hz attempt\n");
			break;
		case 0x06:
			upd1990a->freq_divider = 16;
			logerror("RTC Set TP 2048Hz attempt\n");
			break;
		case 0x07:
			logerror("RTC TEST Mode attempt\n");
			/* Not implemented yet */
			break;
	}

	if (command & 0x04 )
	{	
		UINT16 frequency = UPD1990AC_XTAL / upd1990a->freq_divider;

//		logerror("TP freq %d \n", frequency);

		timer_adjust_periodic(upd1990a->tp, attotime_zero, 0, ATTOTIME_IN_HZ(frequency));
	}
}


WRITE8_DEVICE_HANDLER( upd1990a_clk_w )
{
	upd1990a_t *upd1990a = get_safe_token(device);

	if (upd1990a->shift_mode)
	{
	/* Shift Register */
		upd1990a->shift_reg[0] >>= 1;
		upd1990a->shift_reg[0] |= upd1990a->shift_reg[1] & 1 ? 0x80 : 0;
		upd1990a->shift_reg[1] >>= 1;
		upd1990a->shift_reg[1] |= upd1990a->shift_reg[2] & 1 ? 0x80 : 0;
		upd1990a->shift_reg[2] >>= 1;
		upd1990a->shift_reg[2] |= upd1990a->shift_reg[3] & 1 ? 0x80 : 0;
		upd1990a->shift_reg[3] >>= 1;
		upd1990a->shift_reg[3] |= upd1990a->shift_reg[4] & 1 ? 0x80 : 0;
		upd1990a->shift_reg[4] >>= 1;
		upd1990a->shift_reg[4] |= data & 0x10 ? 0x80 : 0;
		upd1990a->data_out = upd1990a->shift_reg[0] & 1;
	}
}

READ8_DEVICE_HANDLER( upd1990a_data_out_r )
{
	upd1990a_t *upd1990a = get_safe_token(device);

	return upd1990a->data_out;
}

static TIMER_CALLBACK( rtc_tick )
{
	const device_config *device = devtag_get_device(machine, "rtc");
	upd1990a_t *upd1990a = get_safe_token(device);
	UINT16 frequency = UPD1990AC_XTAL / upd1990a->freq_divider;

	logerror("TP freq %d \n", frequency);

//	take system time here?
}

/* Untested save state support */
static void upd1990a_register_state_save(const device_config *device)
{
    upd1990a_t* upd1990a = get_safe_token( device );

    state_save_register_global(device->machine, upd1990a->shift_reg[0]);
    state_save_register_global(device->machine, upd1990a->shift_reg[1]);
    state_save_register_global(device->machine, upd1990a->shift_reg[2]);
    state_save_register_global(device->machine, upd1990a->shift_reg[3]);
    state_save_register_global(device->machine, upd1990a->shift_reg[4]);
    state_save_register_global(device->machine, upd1990a->data_in);
    state_save_register_global(device->machine, upd1990a->data_out);
    state_save_register_global(device->machine, upd1990a->shift_mode);
    state_save_register_global(device->machine, upd1990a->freq_divider);
}


static DEVICE_START( upd1990a )
{
	upd1990a_t *upd1990a = get_safe_token(device);

	upd1990a->tp = timer_alloc(device->machine, rtc_tick, 0);
	upd1990a_register_state_save(device);
}

static DEVICE_RESET( upd1990a )
{
	upd1990a_t *upd1990a = get_safe_token(device);

	upd1990a->shift_reg[0] = 0x00;
	upd1990a->shift_reg[1] = 0x00;
	upd1990a->shift_reg[2] = 0x00;
	upd1990a->shift_reg[3] = 0x00;
	upd1990a->shift_reg[4] = 0x00;
	upd1990a->data_in = 0;
	upd1990a->data_out = 0;
	upd1990a->shift_mode = 0;
	upd1990a->freq_divider = 0;
}

DEVICE_GET_INFO( upd1990a )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(upd1990a_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_TIMER;				break;

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
