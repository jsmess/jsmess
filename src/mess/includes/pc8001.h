#ifndef __PC8001__
#define __PC8001__

#define Z80_TAG			"z80"
#define I8251_TAG		"i8251"
#define I8255A_TAG		"i8255"
#define I8257_TAG		"i8257"
#define UPD1990A_TAG	"upd1990a"
#define UPD3301_TAG		"upd3301"
#define CASSETTE_TAG	"cassette"
#define CENTRONICS_TAG	"centronics"
#define SCREEN_TAG		"screen"
#define SPEAKER_TAG		"speaker"

typedef struct _pc8001_state pc8001_state;
struct _pc8001_state
{
	/* video state */
	UINT8 *char_rom;
	int width80;
	int color;

	/* devices */
	const device_config *i8257;
	const device_config *upd1990a;
	const device_config *upd3301;
	const device_config *cassette;
	const device_config *centronics;
	const device_config *speaker;
};

#endif
