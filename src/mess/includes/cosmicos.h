#ifndef __COSMICOS__
#define __COSMICOS__

#include "cpu/cosmac/cosmac.h"

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
		: driver_device(machine, config),
		  m_maincpu(*this, CDP1802_TAG)
	{ }

	required_device<cosmac_device> m_maincpu;

	/* CPU state */
	int wait;
	int clear;
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
	device_t *dm9368;
	device_t *cdp1864;
	device_t *cassette;
	device_t *speaker;
};

#endif
