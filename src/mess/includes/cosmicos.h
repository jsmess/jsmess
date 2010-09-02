#ifndef __COSMICOS__
#define __COSMICOS__

#include "cpu/cdp1802/cdp1802.h"

#define CDP1802_TAG		"ic19"
#define CDP1864_TAG		"ic3"
#define DM9368_TAG		"ic10"
#define CASSETTE_TAG	"cassette"
#define SCREEN_TAG		"screen"
#define SPEAKER_TAG		"speaker"

enum
{
	LED_RUN = 0,
	LED_LOAD,
	LED_PAUSE,
	LED_RESET,
	LED_D7,
	LED_D6,
	LED_D5,
	LED_D4,
	LED_D3,
	LED_D2,
	LED_D1,
	LED_D0,
	LED_Q,
	LED_CASSETTE
};

class cosmicos_state : public driver_device
{
public:
	cosmicos_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* CPU state */
	cdp1802_control_mode cdp1802_mode;
	int sc1;

	/* memory state */
	UINT8 data;
	int boot;
	int ram_protect;
	int ram_disable;

	/* keyboard state */
	UINT8 keylatch;

	/* display state */
	UINT8 segment;
	int digit;
	int counter;
	int q;
	int dmaout;
	int efx;
	int video_on;

	/* devices */
	running_device *dm9368;
	running_device *cdp1864;
	running_device *cassette;
	running_device *speaker;
};

#endif
