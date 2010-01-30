#ifndef __CRVISION__
#define __CRVISION__

#define SCREEN_TAG		"screen"
#define M6502_TAG		"u2"
#define TMS9929_TAG		"u3"
#define PIA6821_TAG		"u21"
#define SN76489_TAG		"u22"
#define CASSETTE_TAG	"cassette"
#define CENTRONICS_TAG	"centronics"

#define BANK_ROM1	"bank1"
#define BANK_ROM2	"bank2"

typedef struct _crvision_state crvision_state;
struct _crvision_state
{
	/* keyboard state */
	int keylatch;

	/* devices */
	running_device *sn76489;
	running_device *cassette;
};

#endif
