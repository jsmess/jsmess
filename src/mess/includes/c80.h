#ifndef __C80__
#define __C80__

#define SCREEN_TAG		"screen"
#define Z80_TAG			"d2"
#define Z80PIO1_TAG		"d11"
#define Z80PIO2_TAG		"d12"
#define CASSETTE_TAG	"cassette"

class c80_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, c80_state(machine)); }

	c80_state(running_machine &machine) { }

	/* keyboard state */
	int keylatch;

	/* display state */
	int digit;
	int pio1_a5;
	int pio1_brdy;

	/* devices */
	running_device *z80pio;
	running_device *cassette;
};

#endif
