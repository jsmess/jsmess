#ifndef __JTC__
#define __JTC__

#define SCREEN_TAG		"screen"
#define UB8830D_TAG		"ub8830d"
#define CASSETTE_TAG	"cassette"
#define SPEAKER_TAG		"speaker"

typedef struct _jtc_state jtc_state;
struct _jtc_state
{
	/* video state */
	UINT8 *video_ram;

	/* devices */
	const device_config *cassette;
	const device_config *speaker;
};

#endif
