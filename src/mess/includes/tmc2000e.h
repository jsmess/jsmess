#ifndef __TMC2000E__
#define __TMC2000E__

#define SCREEN_TAG		"screen"
#define CDP1802_TAG		"cdp1802"
#define CDP1864_TAG		"cdp1864"
#define CASSETTE_TAG	"cassette"

#define TMC2000E_COLORRAM_SIZE 0x100 // ???

class tmc2000e_state : public driver_device
{
public:
	tmc2000e_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* video state */
	int cdp1864_efx;		/* EFx */
	UINT8 *colorram;		/* color memory */
	UINT8 color;

	/* keyboard state */
	int keylatch;			/* key latch */
	int reset;				/* reset activated */

	/* devices */
	device_t *cdp1864;
	device_t *cassette;
};

#endif
