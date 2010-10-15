#ifndef __ETI660__
#define __ETI660__

#include "cpu/cosmac/cosmac.h"

#define SCREEN_TAG		"screen"
#define CDP1802_TAG		"ic3"
#define CDP1864_TAG		"ic4"
#define MC6821_TAG		"ic5"
#define CASSETTE_TAG	"cassette"

enum
{
	LED_POWER = 0,
	LED_PULSE
};

class eti660_state : public driver_device
{
public:
	eti660_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* keyboard state */
	UINT8 keylatch;

	/* video state */
	UINT8 color_ram[0x100];
	UINT8 color;

	/* devices */
	running_device *cdp1864;
	running_device *cassette;
};

#endif
