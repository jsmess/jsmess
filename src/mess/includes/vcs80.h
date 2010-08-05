#ifndef __VCS80__
#define __VCS80__

#define SCREEN_TAG		"screen"
#define Z80_TAG			"z80"
#define Z80PIO_TAG		"z80pio"

class vcs80_state : public driver_data_t
{
public:
	static driver_data_t *alloc(running_machine &machine) { return auto_alloc_clear(&machine, vcs80_state(machine)); }

	vcs80_state(running_machine &machine)
		: driver_data_t(machine) { }

	/* keyboard state */
	int keylatch;
	int keyclk;

	/* devices */
	running_device *z80pio;
};

#endif
