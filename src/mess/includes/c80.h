#ifndef __C80__
#define __C80__

#define SCREEN_TAG		"screen"
#define Z80_TAG			"d2"
#define Z80PIO1_TAG		"d11"
#define Z80PIO2_TAG		"d12"
#define CASSETTE_TAG	"cassette"

typedef struct _c80_state c80_state;
struct _c80_state
{
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
