/**********************************************************************

    ADC0808/ADC0809 8-Bit A/D Converter emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "emu.h"
#include "adc080x.h"

typedef struct _adc080x_t adc080x_t;
struct _adc080x_t
{
	const adc080x_interface *intf;		/* interface */

	int address;						/* analog channel address */
	int ale;							/* address latch enable pin */
	int start;							/* start conversion pin */
	int eoc;							/* end of conversion pin */
	int next_eoc;						/* next value end of conversion pin */

	UINT8 sar;							/* successive approximation register */

	int cycle;							/* clock cycle counter */
	int bit;							/* bit counter */

	/* timers */
	emu_timer *cycle_timer;				/* cycle timer */
};

INLINE adc080x_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == ADC0808 || device->type() == ADC0809);

	return (adc080x_t *)downcast<legacy_device_base *>(device)->token();
}

/* Approximation Cycle */

static TIMER_CALLBACK( cycle_tick )
{
	device_t *device = (device_t *)ptr;
	adc080x_t *adc080x = get_safe_token(device);

	if (!adc080x->start)
	{
		if (adc080x->cycle == 7)
		{
			adc080x->bit++;

			if (adc080x->bit == 8)
			{
				/* sample input */
				double vref_pos = adc080x->intf->vref_pos_r(device);
				double vref_neg = adc080x->intf->vref_neg_r(device);
				double input = adc080x->intf->input_r(device, adc080x->address);

				adc080x->sar = (255 * (input - vref_neg)) / (vref_pos - vref_neg);

				/* trigger end of conversion */
				adc080x->next_eoc = 1;
			}
		}
	}

	if (adc080x->cycle == 0)
	{
		/* set end of conversion pin */
		if (adc080x->next_eoc != adc080x->eoc)
		{
			adc080x->intf->on_eoc_changed(device, adc080x->next_eoc);
			adc080x->eoc = adc080x->next_eoc;
		}
	}

	adc080x->cycle++;

	if (adc080x->cycle == 8)
	{
		adc080x->cycle = 0;
	}
}

/* Address Latch Enable */

void adc080x_ale_w(device_t *device, int level, int address)
{
	adc080x_t *adc080x = get_safe_token(device);

	if (adc080x->ale && !level) // falling edge
	{
		adc080x->address = address;
	}

	adc080x->ale = level;
}

/* Start Conversion */

void adc080x_start_w(device_t *device, int level)
{
	adc080x_t *adc080x = get_safe_token(device);

	if (!adc080x->start && level) // rising edge
	{
		// reset registers

		adc080x->sar = 0;
		adc080x->bit = 0;
	}
	else if (adc080x->start && !level) // falling edge
	{
		// start conversion

		adc080x->next_eoc = 0;
	}

	adc080x->start = level;
}

/* Conversion Data */

READ8_DEVICE_HANDLER( adc080x_data_r )
{
	adc080x_t *adc080x = get_safe_token(device);

	return adc080x->sar;
}

/* Device Interface */

static DEVICE_START( adc080x )
{
	adc080x_t *adc080x = get_safe_token(device);

	/* validate arguments */
	assert(device != NULL);
	assert(device->tag() != NULL);

	adc080x->intf = (const adc080x_interface*)device->baseconfig().static_config();

	assert(adc080x->intf != NULL);
	assert(device->clock() > 0);

	/* set initial values */
	adc080x->eoc = 1;

	/* allocate cycle timer */
	adc080x->cycle_timer = device->machine().scheduler().timer_alloc(FUNC(cycle_tick), (void *)device);
	adc080x->cycle_timer->adjust(attotime::zero, 0, attotime::from_hz(device->clock()));

	/* register for state saving */
	state_save_register_item(device->machine(), "adc080x", device->tag(), 0, adc080x->address);
	state_save_register_item(device->machine(), "adc080x", device->tag(), 0, adc080x->ale);
	state_save_register_item(device->machine(), "adc080x", device->tag(), 0, adc080x->start);
	state_save_register_item(device->machine(), "adc080x", device->tag(), 0, adc080x->eoc);
	state_save_register_item(device->machine(), "adc080x", device->tag(), 0, adc080x->next_eoc);
	state_save_register_item(device->machine(), "adc080x", device->tag(), 0, adc080x->sar);
	state_save_register_item(device->machine(), "adc080x", device->tag(), 0, adc080x->cycle);
	state_save_register_item(device->machine(), "adc080x", device->tag(), 0, adc080x->bit);
}

DEVICE_GET_INFO( adc0808 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(adc080x_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(adc080x);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/* Nothing */								break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "National Semiconductor ADC0808");	break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "National Semiconductor ADC080");	break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);							break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");			break;
	}
}

DEVICE_GET_INFO( adc0809 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "National Semiconductor ADC0809");	break;

		default:										DEVICE_GET_INFO_CALL(adc0808);				break;
	}
}

DEFINE_LEGACY_DEVICE(ADC0808, adc0808);
DEFINE_LEGACY_DEVICE(ADC0809, adc0809);
