#ifndef __MIKROMIK__
#define __MIKROMIK__

#define SCREEN_TAG		"screen"
#define I8085A_TAG		"ic40"
#define I8212_TAG		"ic12"
#define I8237_TAG		"ic45"
#define I8253_TAG		"ic6"
#define UPD765_TAG		"ic15"
#define I8275_TAG		"ic59"
#define UPD7201_TAG		"ic11"
#define UPD7220_TAG		"ic101"
#define SPEAKER_TAG		"speaker"

class mm1_state : public driver_data_t
{
public:
	static driver_data_t *alloc(running_machine &machine) { return auto_alloc_clear(&machine, mm1_state(machine)); }

	mm1_state(running_machine &machine)
		: driver_data_t(machine) { }

	/* keyboard state */
	int sense;
	int drive;
	UINT8 keydata;
	UINT8 *key_rom;

	/* video state */
	UINT8 *char_rom;
	int llen;

	/* serial state */
	int intc;
	int rx21;
	int tx21;
	int rcl;

	/* floppy state */
	int recall;
	int dack3;
	int tc;

	/* devices */
	running_device *i8212;
	running_device *i8237;
	running_device *i8275;
	running_device *upd765;
	running_device *upd7201;
	running_device *upd7220;
	running_device *speaker;
};

#endif
