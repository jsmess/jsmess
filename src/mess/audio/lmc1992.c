#include "lmc1992.h"

#define LOG (0)

typedef struct _lmc1992_t lmc1992_t;
struct _lmc1992_t
{
	const lmc1992_interface *intf;	/* interface */

	int enable;						/* enable latch */
	int data;						/* data latch */
	int clock;						/* clock latch */
	UINT16 si;						/* serial in shift register */

	int input;						/* input select */
	int bass;						/* bass */
	int treble;						/* treble */
	int volume;						/* volume */
	int fader_rf;					/* right front fader */
	int fader_lf;					/* left front fader */
	int fader_rr;					/* right rear fader */
	int fader_lr;					/* left rear fader */
};

INLINE lmc1992_t *get_safe_token(device_t *device)
{
	assert(device != NULL);

	return (lmc1992_t *)downcast<legacy_device_base *>(device)->token();
}

static void lmc1992_command_w(device_t *device, int addr, int data)
{
	lmc1992_t *lmc1992 = get_safe_token(device);

	switch (addr)
	{
	case LMC1992_FUNCTION_INPUT_SELECT:
		if (data == LCM1992_INPUT_SELECT_OPEN)
		{
			if (LOG) logerror("LMC1992 Input Select : OPEN\n");
		}
		else
		{
			if (LOG) logerror("LMC1992 Input Select : INPUT%u\n", data);
		}
		lmc1992->input = data;
		break;

	case LMC1992_FUNCTION_BASS:
		if (LOG) logerror("LMC1992 Bass : %i dB\n", -40 + (data * 2));
		lmc1992->bass = data;
		break;

	case LMC1992_FUNCTION_TREBLE:
		if (LOG) logerror("LMC1992 Treble : %i dB\n", -40 + (data * 2));
		lmc1992->treble = data;
		break;

	case LMC1992_FUNCTION_VOLUME:
		if (LOG) logerror("LMC1992 Volume : %i dB\n", -80 + (data * 2));
		lmc1992->volume = data;
		break;

	case LMC1992_FUNCTION_RIGHT_FRONT_FADER:
		if (LOG) logerror("LMC1992 Right Front Fader : %i dB\n", -40 + (data * 2));
		lmc1992->fader_rf = data;
		break;

	case LMC1992_FUNCTION_LEFT_FRONT_FADER:
		if (LOG) logerror("LMC1992 Left Front Fader : %i dB\n", -40 + (data * 2));
		lmc1992->fader_lf = data;
		break;

	case LMC1992_FUNCTION_RIGHT_REAR_FADER:
		if (LOG) logerror("LMC1992 Right Rear Fader : %i dB\n", -40 + (data * 2));
		lmc1992->fader_rr = data;
		break;

	case LMC1992_FUNCTION_LEFT_REAR_FADER:
		if (LOG) logerror("LMC1992 Left Rear Fader : %i dB\n", -40 + (data * 2));
		lmc1992->fader_lr = data;
		break;
	}
}

void lmc1992_clock_w(device_t *device, int level)
{
	lmc1992_t *lmc1992 = get_safe_token(device);

	// clock data in on rising edge

	if ((lmc1992->enable == 0) && ((lmc1992->clock == 0) && (level == 1)))
	{
		lmc1992->si >>= 1;
		lmc1992->si = lmc1992->si & 0x7fff;

		if (lmc1992->data)
		{
			lmc1992->si &= 0x8000;
		}
	}

	lmc1992->clock = level;
}

void lmc1992_data_w(device_t *device, int level)
{
	lmc1992_t *lmc1992 = get_safe_token(device);

	lmc1992->data = level;
}

void lmc1992_enable_w(device_t *device, int level)
{
	lmc1992_t *lmc1992 = get_safe_token(device);

	if ((lmc1992->enable == 0) && (level == 1))
	{
		UINT8 device_addr = (lmc1992->si & 0xc000) >> 14;
		UINT8 addr = (lmc1992->si & 0x3800) >> 11;
		UINT8 data = (lmc1992->si & 0x07e0) >> 5;

		if (device_addr == LMC1992_MICROWIRE_DEVICE_ADDRESS)
		{
			lmc1992_command_w(device, addr, data);
		}
	}

	lmc1992->enable = level;
}

/* Device Interface */

static DEVICE_START( lmc1992 )
{
	lmc1992_t *lmc1992 = get_safe_token(device);

	/* validate arguments */
	assert(device != NULL);
	assert(device->tag() != NULL);

	lmc1992->intf = (const lmc1992_interface*)device->baseconfig().static_config();

//  assert(lmc1992->intf != NULL);

	/* register for state saving */

	device->save_item(NAME(lmc1992->enable));
	device->save_item(NAME(lmc1992->data));
	device->save_item(NAME(lmc1992->clock));
	device->save_item(NAME(lmc1992->si));
	device->save_item(NAME(lmc1992->input));
	device->save_item(NAME(lmc1992->bass));
	device->save_item(NAME(lmc1992->treble));
	device->save_item(NAME(lmc1992->volume));
	device->save_item(NAME(lmc1992->fader_rf));
	device->save_item(NAME(lmc1992->fader_lf));
	device->save_item(NAME(lmc1992->fader_rr));
	device->save_item(NAME(lmc1992->fader_lr));
}

DEVICE_GET_INFO( lmc1992 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(lmc1992_t);				break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(lmc1992);	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							/* Nothing */								break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "LMC1992");						break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "LMC1992");						break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);							break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright MESS Team");			break;
	}
}

DEFINE_LEGACY_DEVICE(LMC1992, lmc1992);
