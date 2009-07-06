#ifndef __POLY880__
#define __POLY880__

#define SCREEN_TAG		"screen"
#define Z80_TAG			"i1"
#define Z80CTC_TAG		"i4"
#define Z80PIO1_TAG		"i2"
#define Z80PIO2_TAG		"i3"
#define CASSETTE_TAG	"cassette"

typedef struct _poly880_state poly880_state;
struct _poly880_state
{
	/* display state */
	UINT8 digit;
	UINT8 segment;
};

#endif
