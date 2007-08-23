#include "lmc1992.h"

static struct LMC1992
{
	const lmc1992_interface *intf;

	int enable;
	int data;
	int clock;
	UINT16 si;

	int input;
	int bass, treble;
	int volume;
	int fader_rf, fader_lf, fader_rr, fader_lr;
} lmc;

static void lmc1992_command_w(int addr, int data)
{
	switch (addr)
	{
	case LMC1992_FUNCTION_INPUT_SELECT:
		if (data == LCM1992_INPUT_SELECT_OPEN)
		{
			logerror("LMC1992 Input Select : OPEN\n");
		}
		else
		{
			logerror("LMC1992 Input Select : INPUT%u\n", data);
		}
		lmc.input = data;
		break;
	case LMC1992_FUNCTION_BASS:
		logerror("LMC1992 Bass : %i dB\n", -40 + (data * 2));
		lmc.bass = data;
		break;
	case LMC1992_FUNCTION_TREBLE:
		logerror("LMC1992 Treble : %i dB\n", -40 + (data * 2));
		lmc.treble = data;
		break;
	case LMC1992_FUNCTION_VOLUME:
		logerror("LMC1992 Volume : %i dB\n", -80 + (data * 2));
		lmc.volume = data;
		break;
	case LMC1992_FUNCTION_RIGHT_FRONT_FADER:
		logerror("LMC1992 Right Front Fader : %i dB\n", -40 + (data * 2));
		lmc.fader_rf = data;
		break;
	case LMC1992_FUNCTION_LEFT_FRONT_FADER:
		logerror("LMC1992 Left Front Fader : %i dB\n", -40 + (data * 2));
		lmc.fader_lf = data;
		break;
	case LMC1992_FUNCTION_RIGHT_REAR_FADER:
		logerror("LMC1992 Right Rear Fader : %i dB\n", -40 + (data * 2));
		lmc.fader_rr = data;
		break;
	case LMC1992_FUNCTION_LEFT_REAR_FADER:
		logerror("LMC1992 Left Rear Fader : %i dB\n", -40 + (data * 2));
		lmc.fader_lr = data;
		break;
	}
}

void lmc1992_clock_w(int level)
{
	// clock data in on rising edge

	if ((lmc.enable == 0) && ((lmc.clock == 0) && (level == 1)))
	{
		lmc.si >>= 1;
		lmc.si = lmc.si & 0x7fff;

		if (lmc.data)
		{
			lmc.si &= 0x8000;
		}
	}

	lmc.clock = level;
}

void lmc1992_data_w(int level)
{
	lmc.data = level;
}

void lmc1992_enable_w(int level)
{
	if ((lmc.enable == 0) && (level == 1))
	{
		UINT8 device = (lmc.si & 0xc000) >> 14;
		UINT8 addr = (lmc.si & 0x3800) >> 11;
		UINT8 data = (lmc.si & 0x07e0) >> 5;

		if (device == LMC1992_MICROWIRE_DEVICE_ADDRESS)
		{
			lmc1992_command_w(addr, data);
		}
	}

	lmc.enable = level;
}

void lmc1992_config(const lmc1992_interface *intf)
{
	memset(&lmc, 0, sizeof(lmc));

	lmc.intf = intf;

	state_save_register_global(lmc.enable);
	state_save_register_global(lmc.data);
	state_save_register_global(lmc.clock);
	state_save_register_global(lmc.si);
	state_save_register_global(lmc.input);
	state_save_register_global(lmc.bass);
	state_save_register_global(lmc.treble);
	state_save_register_global(lmc.volume);
	state_save_register_global(lmc.fader_rf);
	state_save_register_global(lmc.fader_lf);
	state_save_register_global(lmc.fader_rr);
	state_save_register_global(lmc.fader_lr);
}
