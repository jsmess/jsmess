#ifndef __JTC__
#define __JTC__

#define SCREEN_TAG		"screen"
#define UB8830D_TAG		"ub8830d"
#define CASSETTE_TAG	"cassette"
#define SPEAKER_TAG		"speaker"
#define CENTRONICS_TAG	"centronics"

#define JTC_ES40_VIDEORAM_SIZE	0x2000

class jtc_state : public driver_data_t
{
public:
	static driver_data_t *alloc(running_machine &machine) { return auto_alloc_clear(&machine, jtc_state(machine)); }

	jtc_state(running_machine &machine)
		: driver_data_t(machine) { }

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
