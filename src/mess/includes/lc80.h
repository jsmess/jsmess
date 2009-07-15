#ifndef __LC80__
#define __LC80__

#define SCREEN_TAG		"screen"
#define Z80_TAG			"d201"
#define Z80CTC_TAG		"d208"
#define Z80PIO1_TAG		"d206"
#define Z80PIO2_TAG		"d207"
#define CASSETTE_TAG	"cassette"
#define SPEAKER_TAG		"b237"

typedef struct _lc80_state lc80_state;
struct _lc80_state
{
	/* display state */
	UINT8 digit;
	UINT8 segment;

	/* devices */
	const device_config *z80pio2;
	const device_config *speaker;
	const device_config *cassette;
};

#endif
