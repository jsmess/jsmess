#ifndef __CRVISION__
#define __CRVISION__

#define SCREEN_TAG		"screen"
#define M6502_TAG		"u2"
#define TMS9929_TAG		"u3"
#define PIA6821_TAG		"u21"
#define SN76489_TAG		"u22"
#define CASSETTE_TAG	"cassette"
#define CENTRONICS_TAG	"centronics"

#define BANK_ROM1		"bank1"
#define BANK_ROM2		"bank2"

class crvision_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, crvision_state(machine)); }

	crvision_state(running_machine &machine) { }

	/* keyboard state */
	int keylatch;

	/* devices */
	running_device *psg;
	running_device *cassette;
};

#endif
