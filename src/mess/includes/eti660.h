#ifndef __ETI660__
#define __ETI660__

#include "cpu/cdp1802/cdp1802.h"

#define SCREEN_TAG	"screen"
#define CDP1802_TAG "ic3"
#define CDP1864_TAG "ic4"
#define MC6821_TAG	"ic5"
#define CASSETTE_TAG	"cassette"

enum
{
	LED_POWER = 0,
	LED_PULSE
};

typedef struct _eti660_state eti660_state;
struct _eti660_state
{
	/* cpu state */
	cdp1802_control_mode cdp1802_mode;

	/* keyboard state */
	UINT8 keylatch;

	/* video state */
	int cdp1864_efx;
	UINT8 *color_ram;
	UINT8 color;

	/* devices */
	const device_config *cdp1864;
	const device_config *cassette;
};

#endif
