#ifndef __STUDIO2__
#define __STUDIO2__

#include "cpu/cdp1802/cdp1802.h"

#define SCREEN_TAG "main"
#define CDP1802_TAG "cdp1802"
#define CDP1861_TAG "cdp1861"
#define CDP1864_TAG "cdp1864"

#define ST2_HEADER_SIZE		256

typedef struct _studio2_state studio2_state;
struct _studio2_state
{
	/* cpu state */
	cdp1802_control_mode cdp1802_mode;

	/* keyboard state */
	UINT8 keylatch;

	/* video state */
	int cdp1861_efx;
	int cdp1864_efx;
	UINT8 *color_ram;

	/* devices */
	const device_config *cdp1861;
	const device_config *cdp1864;
};

#endif
