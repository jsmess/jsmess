#ifndef __TMC1800__
#define __TMC1800__

#define SCREEN_TAG		"screen"
#define CDP1802_TAG		"cdp1802"
#define CDP1861_TAG		"cdp1861"
#define CDP1864_TAG		"m3"
#define CASSETTE_TAG	"cassette"

class tmc1800_state : public driver_data_t
{
public:
	static driver_data_t *alloc(running_machine &machine) { return auto_alloc_clear(&machine, tmc1800_state(machine)); }

	tmc1800_state(running_machine &machine)
		: driver_data_t(machine) { }

	/* cpu state */
	int reset;				/* reset activated */

	/* video state */
	int cdp1861_efx;		/* EFx */

	/* keyboard state */
	int keylatch;			/* key latch */

	/* devices */
	running_device *cdp1861;
	running_device *cassette;
};

class osc1000b_state : public driver_data_t
{
public:
	static driver_data_t *alloc(running_machine &machine) { return auto_alloc_clear(&machine, osc1000b_state(machine)); }

	osc1000b_state(running_machine &machine)
		: driver_data_t(machine) { }

	/* cpu state */
	int reset;				/* reset activated */

	/* keyboard state */
	int keylatch;			/* key latch */

	/* devices */
	running_device *cassette;
};

class tmc2000_state : public driver_data_t
{
public:
	static driver_data_t *alloc(running_machine &machine) { return auto_alloc_clear(&machine, tmc2000_state(machine)); }

	tmc2000_state(running_machine &machine)
		: driver_data_t(machine) { }

	/* video state */
	int cdp1864_efx;		/* EFx */
	int reset;				/* reset activated */

	UINT8 *colorram;		/* color memory */
	UINT8 color;

	/* keyboard state */
	int keylatch;			/* key latch */

	/* devices */
	running_device *cdp1864;
	running_device *cassette;
};

class oscnano_state : public driver_data_t
{
public:
	static driver_data_t *alloc(running_machine &machine) { return auto_alloc_clear(&machine, oscnano_state(machine)); }

	oscnano_state(running_machine &machine)
		: driver_data_t(machine) { }

	/* cpu state */
	int monitor_ef4;		/* EF4 line */
	int reset;				/* reset activated */

	/* video state */
	int cdp1864_efx;		/* EFx */

	/* keyboard state */
	int keylatch;			/* key latch */

	/* timers */
	emu_timer *ef4_timer;	/* EF4 line RC timer */

	/* devices */
	running_device *cdp1864;
	running_device *cassette;
};

/* ---------- defined in video/tmc1800.c ---------- */

MACHINE_CONFIG_EXTERN( tmc1800_video );
MACHINE_CONFIG_EXTERN( osc1000b_video );
MACHINE_CONFIG_EXTERN( tmc2000_video );
MACHINE_CONFIG_EXTERN( oscnano_video );

#endif
