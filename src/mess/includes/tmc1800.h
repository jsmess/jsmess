#ifndef __TMC1800__
#define __TMC1800__

#define SCREEN_TAG		"screen"
#define CDP1802_TAG		"cdp1802"
#define CDP1861_TAG		"cdp1861"
#define CDP1864_TAG		"m3"
#define CASSETTE_TAG	"cassette"

class tmc1800_state : public driver_device
{
public:
	tmc1800_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* keyboard state */
	int keylatch;			/* key latch */

	/* devices */
	device_t *cdp1861;
	device_t *cassette;
};

class osc1000b_state : public driver_device
{
public:
	osc1000b_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* keyboard state */
	int keylatch;			/* key latch */

	/* devices */
	device_t *cassette;
};

class tmc2000_state : public driver_device
{
public:
	tmc2000_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* video state */
	UINT8 *colorram;		/* color memory */
	UINT8 color;

	/* keyboard state */
	int keylatch;			/* key latch */

	/* devices */
	device_t *cdp1864;
	device_t *cassette;
};

class nano_state : public driver_device
{
public:
	nano_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* keyboard state */
	int keylatch;			/* key latch */

	/* timers */
	emu_timer *ef4_timer;	/* EF4 line RC timer */

	/* devices */
	device_t *cdp1864;
	device_t *cassette;
};

/* ---------- defined in video/tmc1800.c ---------- */

MACHINE_CONFIG_EXTERN( tmc1800_video );
MACHINE_CONFIG_EXTERN( osc1000b_video );
MACHINE_CONFIG_EXTERN( tmc2000_video );
MACHINE_CONFIG_EXTERN( nano_video );

#endif
