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

typedef struct _cosmicos_state cosmicos_state;
struct _cosmicos_state
{
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
	const device_config *dm9368;
	const device_config *cdp1864;
	const device_config *cassette;
	const device_config *speaker;
};

#endif
