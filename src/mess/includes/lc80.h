#ifndef __LC80__
#define __LC80__

#define SCREEN_TAG		"screen"
#define Z80_TAG			"d201"
#define Z80CTC_TAG		"d208"
#define Z80PIO1_TAG		"d206"
#define Z80PIO2_TAG		"d207"
#define CASSETTE_TAG	"cassette"
#define SPEAKER_TAG		"b237"

class lc80_state : public driver_data_t
{
public:
	static driver_data_t *alloc(running_machine &machine) { return auto_alloc_clear(&machine, lc80_state(machine)); }

	lc80_state(running_machine &machine)
		: driver_data_t(machine) { }

	/* display state */
	UINT8 digit;
	UINT8 segment;

	/* devices */
	running_device *z80pio2;
	running_device *speaker;
	running_device *cassette;
};

#endif
