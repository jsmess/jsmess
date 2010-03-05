#ifndef __EXP85__
#define __EXP85__

#define SCREEN_TAG		"screen"
#define I8085A_TAG		"u100"
#define I8155_TAG		"u106"
#define I8355_TAG		"u105"
#define CASSETTE_TAG	"cassette"
#define SPEAKER_TAG		"speaker"

class exp85_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, exp85_state(machine)); }

	exp85_state(running_machine &machine) { }

	/* cassette state */
	int tape_control;

	/* devices */
	running_device *cassette;
	running_device *speaker;
};

#endif
