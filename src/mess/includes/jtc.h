#ifndef __JTC__
#define __JTC__

#define SCREEN_TAG		"screen"
#define UB8830D_TAG		"ub8830d"
#define CASSETTE_TAG	"cassette"
#define SPEAKER_TAG		"speaker"
#define CENTRONICS_TAG	"centronics"

#define JTC_ES40_VIDEORAM_SIZE	0x2000

class jtc_state : public driver_device
{
public:
	jtc_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* video state */
	UINT8 video_bank;
	UINT8 *video_ram;
	UINT8 *color_ram_r;
	UINT8 *color_ram_g;
	UINT8 *color_ram_b;

	/* devices */
	running_device *cassette;
	running_device *speaker;
	running_device *centronics;
};

#endif
