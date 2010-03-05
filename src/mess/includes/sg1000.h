#ifndef __SG1000__
#define __SG1000__

#define SCREEN_TAG		"screen"
#define Z80_TAG			"z80"
#define SN76489A_TAG	"sn76489a"
#define UPD765_TAG		"upd765"
#define CASSETTE_TAG	"cassette"
#define UPD8251_TAG		"upd8251"
#define UPD9255_TAG		"upd9255"
#define UPD9255_0_TAG	"upd9255_0"
#define UPD9255_1_TAG	"upd9255_1"
#define CENTRONICS_TAG	"centronics"

#define IS_CARTRIDGE_TV_DRAW(ptr) \
	(!strncmp("annakmn", (const char *)&ptr[0x13b3], 7))

#define IS_CARTRIDGE_THE_CASTLE(ptr) \
	(!strncmp("ASCII 1986", (const char *)&ptr[0x1cc3], 10))

#define IS_CARTRIDGE_BASIC_LEVEL_III(ptr) \
	(!strncmp("SC-3000 BASIC Level 3 ver 1.0", (const char *)&ptr[0x6a20], 29))

#define IS_CARTRIDGE_MUSIC_EDITOR(ptr) \
	(!strncmp("PIANO", (const char *)&ptr[0x0841], 5))

class sg1000_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, sg1000_state(machine)); }

	sg1000_state(running_machine &machine) { }

	/* keyboard state */
	UINT8 keylatch;

	/* TV Draw state */
	UINT8 tvdraw_data;

	/* floppy state */
	int fdc_irq;
	int fdc_index;

	/* devices */
	running_device *upd765;
	running_device *centronics;
	running_device *cassette;
};

#endif
