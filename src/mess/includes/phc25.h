#ifndef __PHC25__
#define __PHC25__

#define SCREEN_TAG		"screen"
#define Z80_TAG			"z80"
#define AY8910_TAG		"ay8910"
#define MC6847_TAG		"mc6847"
#define CASSETTE_TAG	"cassette"
#define CENTRONICS_TAG	"centronics"

#define PHC25_VIDEORAM_SIZE		0x1800

typedef struct _phc25_state phc25_state;
struct _phc25_state
{
	/* video state */
	UINT8 *video_ram;
	UINT8 *char_rom;

	/* devices */
	running_device *mc6847;
	running_device *centronics;
	running_device *cassette;
};

#endif
