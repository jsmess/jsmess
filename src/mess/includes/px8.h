#ifndef __PX8__
#define __PX8__

#define UPD70008_TAG	"4a"
#define UPD7508_TAG		"2e"
#define HD6303_TAG		"13d"
#define SED1320_TAG		"7c"
#define I8251_TAG		"13e"
#define UPD7001_TAG		"1d"
#define CASSETTE_TAG	"cassette"
#define SCREEN_TAG		"screen"

#define PX8_VIDEORAM_MASK	0x17ff

class px8_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, px8_state(machine)); }

	px8_state(running_machine &machine) { }

	/* keyboard state */
	int ksc;

	/* video state */
	UINT8 *video_ram;		/* LCD video RAM */

	/* devices */
	running_device *sed1320;
	running_device *cassette;
};

#endif
