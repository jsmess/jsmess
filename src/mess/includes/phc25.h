#ifndef __PHC25__
#define __PHC25__

#define SCREEN_TAG		"screen"
#define Z80_TAG			"z80"
#define AY8910_TAG		"ay8910"
#define MC6847_TAG		"mc6847"
#define CASSETTE_TAG	"cassette"
#define CENTRONICS_TAG	"centronics"

#define PHC25_VIDEORAM_SIZE		0x1800

class phc25_state : public driver_data_t
{
public:
	static driver_data_t *alloc(running_machine &machine) { return auto_alloc_clear(&machine, phc25_state(machine)); }

	phc25_state(running_machine &machine)
		: driver_data_t(machine) { }

	/* video state */
	UINT8 *video_ram;
	UINT8 *char_rom;
	UINT8 char_size;
	UINT8 char_correct;
	UINT8 char_substact;

	/* devices */
	running_device *mc6847;
	running_device *centronics;
	running_device *cassette;
};

#endif
