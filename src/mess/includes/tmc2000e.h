#ifndef __TMC2000E__
#define __TMC2000E__

#define SCREEN_TAG		"screen"
#define CDP1802_TAG		"cdp1802"
#define CDP1864_TAG		"cdp1864"
#define CASSETTE_TAG	"cassette"

#define TMC2000E_COLORRAM_SIZE 0x100 // ???

class tmc2000e_state : public driver_data_t
{
public:
	static driver_data_t *alloc(running_machine &machine) { return auto_alloc_clear(&machine, tmc2000e_state(machine)); }

	tmc2000e_state(running_machine &machine)
		: driver_data_t(machine) { }

	/* video state */
	int cdp1864_efx;		/* EFx */
	UINT8 *colorram;		/* color memory */
	UINT8 color;

	/* keyboard state */
	int keylatch;			/* key latch */
	int reset;				/* reset activated */

	/* devices */
	running_device *cdp1864;
	running_device *cassette;
};

#endif
