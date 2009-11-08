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

typedef struct _mm1_state mm1_state;
struct _mm1_state
{
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

	int recall;

	/* devices */
	const device_config		*i8212;
	const device_config		*i8237;
	const device_config		*i8275;
	const device_config		*upd765;
	const device_config		*upd7201;
	const device_config		*upd7220;
	const device_config		*speaker;
};

#endif
