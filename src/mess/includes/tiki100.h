#ifndef __TIKI100__
#define __TIKI100__

#define SCREEN_TAG		"screen"
#define Z80_TAG			"z80"
#define Z80DART_TAG		"z80dart"
#define Z80PIO_TAG		"z80pio"
#define Z80CTC_TAG		"z80ctc"
#define FD1797_TAG		"fd1797"
#define AY8912_TAG		"ay8912"

#define TIKI100_VIDEORAM_SIZE	0x8000
#define TIKI100_VIDEORAM_MASK	0x7fff

#define BANK_ROM		0
#define BANK_RAM		1
#define BANK_VIDEO_RAM	2

class tiki100_state : public driver_device
{
public:
	tiki100_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* memory state */
	int rome;
	int vire;

	/* video state */
	UINT8 *video_ram;
	UINT8 scroll;
	UINT8 mode;
	UINT8 palette;

	/* keyboard state */
	int keylatch;

	/* devices */
	running_device *fd1797;
	running_device *z80ctc;
};

#endif
