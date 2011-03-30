/**********************************************************************

    OKI MSM58321RS Real Time Clock emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

    TODO:

    - count
    - stop
    - reset
    - reference registers

*/

#include "emu.h"
#include "msm58321.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

enum
{
	REGISTER_S1 = 0,
	REGISTER_S10,
	REGISTER_MI1,
	REGISTER_MI10,
	REGISTER_H1,
	REGISTER_H10,
	REGISTER_W,
	REGISTER_D1,
	REGISTER_D10,
	REGISTER_MO1,
	REGISTER_MO10,
	REGISTER_Y1,
	REGISTER_Y10,
	REGISTER_RESET,
	REGISTER_REF0,
	REGISTER_REF1
};

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _msm58321_t msm58321_t;
struct _msm58321_t
{
	devcb_resolved_write_line	out_busy_func;

	int cs1;					/* chip select 1 */
	int cs2;					/* chip select 2 */
	int busy;					/* busy flag */
	int read;					/* read data */
	int write;					/* write data */
	int address_write;			/* write address */

	UINT8 reg[13];				/* registers */
	UINT8 latch;				/* data latch (not present in real chip) */
	UINT8 address;				/* address latch */

	/* timers */
	emu_timer *busy_timer;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE msm58321_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == MSM58321RS);

	return (msm58321_t *)downcast<legacy_device_base *>(device)->token();
}

INLINE const msm58321_interface *get_interface(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == MSM58321RS);

	return (const msm58321_interface *) device->baseconfig().static_config();
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    msm58321_r - read data
-------------------------------------------------*/

READ8_DEVICE_HANDLER( msm58321_r )
{
	msm58321_t *msm58321 = get_safe_token(device);

	UINT8 data = 0;

	if (msm58321->cs1 && msm58321->cs2)
	{
		if (msm58321->read)
		{
			system_time systime;

			device->machine().current_datetime(systime);

			switch (msm58321->latch)
			{
			case REGISTER_S1:	data = systime.local_time.second % 10; break;
			case REGISTER_S10:	data = systime.local_time.second / 10; break;
			case REGISTER_MI1:	data = systime.local_time.minute % 10; break;
			case REGISTER_MI10: data = systime.local_time.minute / 10; break;
			case REGISTER_H1:	data = systime.local_time.hour % 10; break;
			case REGISTER_H10:	data = (systime.local_time.hour / 10) | 0x08; break;
			case REGISTER_W:	data = systime.local_time.weekday; break;
			case REGISTER_D1:	data = systime.local_time.mday % 10; break;
			case REGISTER_D10:	data = (systime.local_time.mday / 10) | ((systime.local_time.year % 4) ? 0 : 0x04); break;
			case REGISTER_MO1:	data = (systime.local_time.month + 1) % 10; break;
			case REGISTER_MO10: data = (systime.local_time.month + 1) / 10; break;
			case REGISTER_Y1:	data = systime.local_time.year % 10; break;
			case REGISTER_Y10:	data = (systime.local_time.year / 10) % 10;	break;

			case REGISTER_RESET:
				break;

			case REGISTER_REF0:
			case REGISTER_REF1:
				break;

			default:
				data = msm58321->reg[offset];
				break;
			}
		}

		if (msm58321->write)
		{
			if (msm58321->address >= REGISTER_REF0)
			{
				// TODO: output reference values
			}
		}
	}

	return data;
}

/*-------------------------------------------------
    msm58321_w - write data
-------------------------------------------------*/

WRITE8_DEVICE_HANDLER( msm58321_w )
{
	msm58321_t *msm58321 = get_safe_token(device);

	/* latch data for future use */
	msm58321->latch = data & 0x0f;

	if (!msm58321->cs1 || !msm58321->cs2) return;

	if (msm58321->address_write)
	{
		/* latch address */
		msm58321->address = msm58321->latch;
	}

	if (msm58321->write)
	{
		switch(msm58321->latch)
		{
		case REGISTER_RESET:
			if (LOG) logerror("MSM58321 '%s' Reset\n", device->tag());
			break;

		case REGISTER_REF0:
		case REGISTER_REF1:
			if (LOG) logerror("MSM58321 '%s' Reference Signal\n", device->tag());
			break;

		default:
			if (LOG) logerror("MSM58321 '%s' Register %01x = %01x\n", device->tag(), offset, data & 0x0f);
			msm58321->reg[offset] = msm58321->latch & 0x0f;
			break;
		}
	}
}

/*-------------------------------------------------
    msm58321_cs1_w - chip select 1
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( msm58321_cs1_w )
{
	msm58321_t *msm58321 = get_safe_token(device);

	if (LOG) logerror("MSM58321 '%s' CS1: %u\n", device->tag(), state);

	msm58321->cs1 = state;
}

/*-------------------------------------------------
    msm58321_cs2_w - chip select 2
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( msm58321_cs2_w )
{
	msm58321_t *msm58321 = get_safe_token(device);

	if (LOG) logerror("MSM58321 '%s' CS2: %u\n", device->tag(), state);

	msm58321->cs2 = state;
}

/*-------------------------------------------------
    msm58321_busy_r - busy flag read
-------------------------------------------------*/

READ_LINE_DEVICE_HANDLER( msm58321_busy_r )
{
	msm58321_t *msm58321 = get_safe_token(device);

	return msm58321->busy;
}

/*-------------------------------------------------
    msm58321_read_w - data read handshake
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( msm58321_read_w )
{
	msm58321_t *msm58321 = get_safe_token(device);

	if (LOG) logerror("MSM58321 '%s' READ: %u\n", device->tag(), state);

	msm58321->read = state;
}

/*-------------------------------------------------
    msm58321_write_w - data write handshake
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( msm58321_write_w )
{
	msm58321_t *msm58321 = get_safe_token(device);

	if (LOG) logerror("MSM58321 '%s' WRITE: %u\n", device->tag(), state);

	msm58321->write = state;
}

/*-------------------------------------------------
    msm58321_address_write_w - adddress write
    handshake
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( msm58321_address_write_w )
{
	msm58321_t *msm58321 = get_safe_token(device);

	if (LOG) logerror("MSM58321 '%s' ADDRESS WRITE: %u\n", device->tag(), state);

	msm58321->address_write = state;
}

/*-------------------------------------------------
    msm58321_stop_w - stop count
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( msm58321_stop_w )
{
//  msm58321_t *msm58321 = get_safe_token(device);

	if (LOG) logerror("MSM58321 '%s' STOP: %u\n", device->tag(), state);
}

/*-------------------------------------------------
    msm58321_test_w - test
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( msm58321_test_w )
{
//  msm58321_t *msm58321 = get_safe_token(device);

	if (LOG) logerror("MSM58321 '%s' TEST: %u\n", device->tag(), state);
}

/*-------------------------------------------------
    TIMER_CALLBACK( busy_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( busy_tick )
{
	device_t *device = (device_t *)ptr;
	msm58321_t *msm58321 = get_safe_token(device);

	devcb_call_write_line(&msm58321->out_busy_func, msm58321->busy);

	msm58321->busy = !msm58321->busy;
}

/*-------------------------------------------------
    DEVICE_START( msm58321 )
-------------------------------------------------*/

static DEVICE_START( msm58321 )
{
	msm58321_t *msm58321 = get_safe_token(device);
	const msm58321_interface *intf = get_interface(device);

	/* resolve callbacks */
	devcb_resolve_write_line(&msm58321->out_busy_func, &intf->out_busy_func, device);

	/* create busy timer */
	msm58321->busy_timer = device->machine().scheduler().timer_alloc(FUNC(busy_tick), (void *)device);
	msm58321->busy_timer->adjust(attotime::zero, 0, attotime::from_hz(2));

	/* register for state saving */
	device->save_item(NAME(msm58321->cs1));
	device->save_item(NAME(msm58321->cs2));
	device->save_item(NAME(msm58321->busy));
	device->save_item(NAME(msm58321->read));
	device->save_item(NAME(msm58321->write));
	device->save_item(NAME(msm58321->address_write));
	device->save_item(NAME(msm58321->reg));
	device->save_item(NAME(msm58321->latch));
	device->save_item(NAME(msm58321->address));
}

/*-------------------------------------------------
    DEVICE_GET_INFO( msm58321rs )
-------------------------------------------------*/

DEVICE_GET_INFO( msm58321rs )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(msm58321_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(msm58321);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/* Nothing */								break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "OKI MSM58321RS");			break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "OKI MSM58321RS");			break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");		break;
	}
}

DEFINE_LEGACY_DEVICE(MSM58321RS, msm58321rs);
