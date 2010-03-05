#ifndef __POLY880__
#define __POLY880__

#define SCREEN_TAG		"screen"
#define Z80_TAG			"i1"
#define Z80CTC_TAG		"i4"
#define Z80PIO1_TAG		"i2"
#define Z80PIO2_TAG		"i3"
#define CASSETTE_TAG	"cassette"

class poly880_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, poly880_state(machine)); }

	poly880_state(running_machine &machine) { }

	/* display state */
	UINT8 digit;
	UINT8 segment;

	/* devices */
	running_device *cassette;
};

#endif
