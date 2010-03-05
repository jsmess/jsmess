#ifndef __ETI660__
#define __ETI660__

#include "cpu/cdp1802/cdp1802.h"

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

class eti660_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, eti660_state(machine)); }

	eti660_state(running_machine &machine) { }

	/* cpu state */
	cdp1802_control_mode cdp1802_mode;

	/* keyboard state */
	UINT8 keylatch;

	/* video state */
	int cdp1864_efx;
	UINT8 color_ram[0x100];
	UINT8 color;

	/* devices */
	running_device *cdp1864;
	running_device *cassette;
};

#endif
