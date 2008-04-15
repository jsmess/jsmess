#include "driver.h"
#include "e0516.h"

enum {
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
	const e0516_interface *intf;	/* interface */
	
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

INLINE e0516_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);

	return (e0516_t *)device->token;
}

/* Timer Callbacks */

static TIMER_CALLBACK(clock_tick)
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

/* Serial Data Input/Output */

int e0516_dio_r(const device_config *device)
{
	e0516_t *e0516 = get_safe_token(device);

	return e0516->dio;
}

void e0516_dio_w(const device_config *device, int level)
{
	e0516_t *e0516 = get_safe_token(device);

	e0516->dio = level;
}

/* Clock */

void e0516_clk_w(const device_config *device, int level)
{
	e0516_t *e0516 = get_safe_token(device);

	if (e0516->cs == 1) return;

	if (level == 0)
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

/* Chip Select */

void e0516_cs_w(const device_config *device, int level)
{
	e0516_t *e0516 = get_safe_token(device);

	e0516->cs = level;

	if (level == 1)
	{
		e0516->data_latch = 0;
		e0516->reg_latch = 0;
		e0516->bits = 0;
		e0516->state = 0;
	}
}

/* Device Interface */

static DEVICE_START( e0516 )
{
	e0516_t *e0516 = get_safe_token(device);
	char unique_tag[30];

	/* validate arguments */
	assert(device != NULL);
	assert(device->tag != NULL);
	assert(strlen(device->tag) < 20);

	e0516->intf = device->static_config;

	assert(e0516->intf != NULL);
	assert(e0516->intf->clock > 0);

	/* create the timers */
	e0516->clock_timer = timer_alloc(clock_tick, (void *)device);
	timer_adjust_periodic(e0516->clock_timer, attotime_zero, 0, ATTOTIME_IN_HZ(e0516->intf->clock / 32768));

	/* register for state saving */
	state_save_combine_module_and_tag(unique_tag, "E0516", device->tag);

	state_save_register_item(unique_tag, 0, e0516->cs);
	state_save_register_item(unique_tag, 0, e0516->data_latch);
	state_save_register_item(unique_tag, 0, e0516->reg_latch);
	state_save_register_item(unique_tag, 0, e0516->read_write);
	state_save_register_item(unique_tag, 0, e0516->state);
	state_save_register_item(unique_tag, 0, e0516->bits);
	state_save_register_item(unique_tag, 0, e0516->dio);

	state_save_register_item_array(unique_tag, 0, e0516->reg);
}

static DEVICE_SET_INFO( e0516 )
{
	switch (state)
	{
		/* no parameters to set */
	}
}

DEVICE_GET_INFO( e0516 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(e0516_t);					break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_SET_INFO:						info->set_info = DEVICE_SET_INFO_NAME(e0516); break;
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(e0516);		break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/* Nothing */								break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							info->s = "E05-16";							break;
		case DEVINFO_STR_FAMILY:						info->s = "E05-16";							break;
		case DEVINFO_STR_VERSION:						info->s = "1.0";							break;
		case DEVINFO_STR_SOURCE_FILE:					info->s = __FILE__;							break;
		case DEVINFO_STR_CREDITS:						info->s = "Copyright MESS Team";			break;
	}
}
