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

class pc8001_state : public driver_device
{
public:
	pc8001_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* video state */
	UINT8 *char_rom;
	int width80;
	int color;

	/* devices */
	device_t *i8257;
	device_t *upd1990a;
	device_t *upd3301;
	device_t *cassette;
	device_t *centronics;
	device_t *speaker;
};

#endif
