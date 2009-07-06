#ifndef __HUEBLER__
#define __HUEBLER__

#define SCREEN_TAG		"screen"
#define Z80_TAG			"z80"
#define Z80CTC_TAG		"z80ctc"
#define Z80SIO_TAG		"z80sio"
#define Z80PIO1_TAG		"z80pio1"
#define Z80PIO2_TAG		"z80pio2"
#define CASSETTE_TAG	"cassette"

typedef struct _huebler_state huebler_state;
struct _huebler_state
{
	/* video state */
	UINT8 *video_ram;
	UINT8 *char_rom;
};

#endif
