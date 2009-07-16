#ifndef __EXP85__
#define __EXP85__

#define SCREEN_TAG		"screen"
#define I8085A_TAG		"u100"
#define I8155_TAG		"u106"
#define I8355_TAG		"u105"
#define CASSETTE_TAG	"cassette"
#define SPEAKER_TAG		"speaker"

typedef struct _exp85_state exp85_state;
struct _exp85_state
{
	/* cassette state */
	int tape_control;

	/* devices */
	const device_config *cassette;
	const device_config *speaker;
};

#endif
