#ifndef __MPF1__
#define __MPF1__

#define Z80_TAG			"u1"
#define Z80CTC_TAG		"u11"
#define Z80PIO_TAG		"u10"
#define I8255A_TAG		"u14"
#define TMS5220_TAG		"tms5220"
#define CASSETTE_TAG	"cassette"
#define SPEAKER_TAG		"speaker"

class mpf1_state : public driver_device
{
public:
	mpf1_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	int _break;
	int m1;

	UINT8 lednum;

	emu_timer *led_refresh_timer;

	/* devices */
	running_device *z80ctc;
	running_device *speaker;
	running_device *cassette;
};

#endif
