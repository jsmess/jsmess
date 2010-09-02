#ifndef __STUDIO2__
#define __STUDIO2__

#include "cpu/cdp1802/cdp1802.h"

#define CDP1802_TAG "ic1"
#define CDP1861_TAG "ic2"
#define CDP1864_TAG "cdp1864"
#define SCREEN_TAG	"screen"

class studio2_state : public driver_device
{
public:
	studio2_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* cpu state */
	cdp1802_control_mode cdp1802_mode;

	/* keyboard state */
	UINT8 keylatch;

	/* video state */
	int cdp1861_efx;
	int cdp1864_efx;
	UINT8 *color_ram;
	UINT8 *color_ram1;
	UINT8 color;

	/* devices */
	running_device *cdp1861;
	running_device *cdp1864;
};

#endif
