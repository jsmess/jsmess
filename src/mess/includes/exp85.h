#ifndef __EXP85__
#define __EXP85__

#define SCREEN_TAG		"screen"
#define I8085A_TAG		"u100"
#define I8155_TAG		"u106"
#define I8355_TAG		"u105"
#define CASSETTE_TAG	"cassette"
#define SPEAKER_TAG		"speaker"

class exp85_state : public driver_device
{
public:
	exp85_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* cassette state */
	int tape_control;

	/* devices */
	device_t *cassette;
	device_t *speaker;
};

#endif
