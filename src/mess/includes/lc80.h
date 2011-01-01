#ifndef __LC80__
#define __LC80__

#define SCREEN_TAG		"screen"
#define Z80_TAG			"d201"
#define Z80CTC_TAG		"d208"
#define Z80PIO1_TAG		"d206"
#define Z80PIO2_TAG		"d207"
#define CASSETTE_TAG	"cassette"
#define SPEAKER_TAG		"b237"

class lc80_state : public driver_device
{
public:
	lc80_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* display state */
	UINT8 digit;
	UINT8 segment;

	/* devices */
	device_t *z80pio2;
	device_t *speaker;
	device_t *cassette;
};

#endif
