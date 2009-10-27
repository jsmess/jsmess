#ifndef __BW12__
#define __BW12__

#define SCREEN_TAG		"screen"
#define Z80_TAG			"ic35"
#define MC6845_TAG		"ic14"
#define UPD765_TAG		"ic45"
#define Z80SIO_TAG		"ic15"
#define PIT8253_TAG		"ic34"
#define PIA6821_TAG		"ic16"
#define MC1408_TAG		"ic4"
#define AY3600_TAG		"ic74"
#define CENTRONICS_TAG	"centronics"

#define BW12_VIDEORAM_MASK	0x7ff
#define BW12_CHARROM_MASK	0xfff

typedef struct _bw12_state bw12_state;
struct _bw12_state
{
	/* memory state */
	int bank;

	/* video state */
	UINT8 *video_ram;
	UINT8 *char_rom;

	/* PIT state */
	int pit_out2;

	/* keyboard state */
	int key_data[9];
	int key_sin;
	int key_stb;
	int key_shift;

	/* floppy state */
	int fdcint;
	int motor_on;
	int motor0;
	int motor1;

	/* devices */
	const device_config *pia6821;
	const device_config *mc6845;
	const device_config *upd765;
	const device_config *centronics;

	/* timers */
	emu_timer	*floppy_motor_off_timer;
};

#endif
