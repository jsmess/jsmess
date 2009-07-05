#ifndef __VCS80__
#define __VCS80__

#define SCREEN_TAG		"screen"
#define Z80_TAG			"z80"
#define Z80PIO_TAG		"z80pio"

typedef struct _vcs80_state vcs80_state;
struct _vcs80_state
{
	/* keyboard state */
	int keylatch;
	int keyclk;

	/* devices */
	const device_config *z80pio;
};

#endif
