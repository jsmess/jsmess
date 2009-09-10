#include "driver.h"
#include "e0516.h"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

enum
{
	E0516_REGISTER_SECOND = 0,
	E0516_REGISTER_MINUTE,
	E0516_REGISTER_HOUR,
	E0516_REGISTER_DAY,
	E0516_REGISTER_MONTH,
	E0516_REGISTER_DAY_OF_WEEK,
	E0516_REGISTER_YEAR,
	E0516_REGISTER_ALL
};

typedef struct _e0516_t e0516_t;
struct _e0516_t
{
	int cs;							/* chip select */
	int data_latch;					/* data latch */
	int reg_latch;					/* register latch */
	int read_write;					/* read/write data */
	int state;						/* state */
	int bits;						/* number of bits transferred */
	int dio;						/* data pin */

	UINT8 reg[8];					/* registers */

	/* timers */
	emu_timer *clock_timer;		/* clock timer */
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE e0516_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	return (e0516_t *)device->token;
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    TIMER_CALLBACK( clock_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( clock_tick )
{
	const device_config *device = ptr;
	e0516_t *e0516 = get_safe_token(device);

	e0516->reg[E0516_REGISTER_SECOND]++;

	if (e0516->reg[E0516_REGISTER_SECOND] == 60)
	{
		e0516->reg[E0516_REGISTER_SECOND] = 0;
		e0516->reg[E0516_REGISTER_MINUTE]++;
	}

	if (e0516->reg[E0516_REGISTER_MINUTE] == 60)
	{
		e0516->reg[E0516_REGISTER_MINUTE] = 0;
		e0516->reg[E0516_REGISTER_HOUR]++;
	}

	if (e0516->reg[E0516_REGISTER_HOUR] == 24)
	{
		e0516->reg[E0516_REGISTER_HOUR] = 0;
		e0516->reg[E0516_REGISTER_DAY]++;
		e0516->reg[E0516_REGISTER_DAY_OF_WEEK]++;
	}

	if (e0516->reg[E0516_REGISTER_DAY_OF_WEEK] == 8)
	{
		e0516->reg[E0516_REGISTER_DAY_OF_WEEK] = 1;
	}

	if (e0516->reg[E0516_REGISTER_DAY] == 32)
	{
		e0516->reg[E0516_REGISTER_DAY] = 1;
		e0516->reg[E0516_REGISTER_MONTH]++;
	}

	if (e0516->reg[E0516_REGISTER_MONTH] == 13)
	{
		e0516->reg[E0516_REGISTER_MONTH] = 1;
		e0516->reg[E0516_REGISTER_YEAR]++;
	}
}

/*-------------------------------------------------
    e0516_dio_r - data out
-------------------------------------------------*/

READ_LINE_DEVICE_HANDLER( e0516_dio_r )
{
	e0516_t *e0516 = get_safe_token(device);

	return e0516->dio;
}

/*-------------------------------------------------
    e0516_dio_w - data in
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( e0516_dio_w )
{
	e0516_t *e0516 = get_safe_token(device);

	e0516->dio = state;
}

/*-------------------------------------------------
    e0516_clk_w - clock in
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( e0516_clk_w )
{
	e0516_t *e0516 = get_safe_token(device);

	if (e0516->cs == 1) return;

	if (state == 0)
	{
		e0516->bits++;

		if (e0516->state == 0)
		{
			// command

			e0516->reg_latch |= e0516->dio << 3;
			e0516->reg_latch >>= 1;

			if (e0516->bits == 4)
			{
				e0516->state = 1;
				e0516->bits = 0;

				if (BIT(e0516->reg_latch, 0))
				{
					// load register value to data latch
					e0516->data_latch = e0516->reg[e0516->reg_latch >> 1];
				}
			}
		}
		else
		{
			// data

			if (BIT(e0516->reg_latch, 0))
			{
				// read

				e0516->dio = BIT(e0516->data_latch, 0);
				e0516->data_latch >>= 1;
			}
			else
			{
				// write

				e0516->data_latch |= e0516->dio << 7;
				e0516->data_latch >>= 1;
			}

			if (e0516->bits == 8)
			{
				e0516->state = 0;
				e0516->bits = 0;

				if (!BIT(e0516->reg_latch, 0))
				{
					// write latched data to register
					e0516->reg[e0516->reg_latch >> 1] = e0516->data_latch;
				}
			}
		}
	}
}

/*-------------------------------------------------
    e0516_clk_w - chip select
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( e0516_cs_w )
{
	e0516_t *e0516 = get_safe_token(device);

	e0516->cs = state;

	if (state == 1)
	{
		e0516->data_latch = 0;
		e0516->reg_latch = 0;
		e0516->bits = 0;
		e0516->state = 0;
	}
}

/*-------------------------------------------------
    DEVICE_START( e0516 )
-------------------------------------------------*/

static DEVICE_START( e0516 )
{
	e0516_t *e0516 = get_safe_token(device);

	/* create the timers */
	e0516->clock_timer = timer_alloc(device->machine, clock_tick, (void *)device);
	timer_adjust_periodic(e0516->clock_timer, attotime_zero, 0, ATTOTIME_IN_HZ(device->clock / 32768));

	/* register for state saving */
	state_save_register_device_item(device, 0, e0516->cs);
	state_save_register_device_item(device, 0, e0516->data_latch);
	state_save_register_device_item(device, 0, e0516->reg_latch);
	state_save_register_device_item(device, 0, e0516->read_write);
	state_save_register_device_item(device, 0, e0516->state);
	state_save_register_device_item(device, 0, e0516->bits);
	state_save_register_device_item(device, 0, e0516->dio);
	state_save_register_device_item_array(device, 0, e0516->reg);
}

/*-------------------------------------------------
    DEVICE_GET_INFO( e0516 )
-------------------------------------------------*/

DEVICE_GET_INFO( e0516 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(e0516_t);					break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(e0516);		break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/* Nothing */								break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "E05-16");					break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "E05-16");					break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");		break;
	}
}
