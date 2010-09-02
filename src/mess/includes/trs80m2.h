#ifndef __TRS80M2__
#define __TRS80M2__

#define SCREEN_TAG		"screen"
#define Z80_TAG			"u12"
#define M68000_TAG		"m68000"
#define I8021_TAG		"z4"
#define Z80CTC_TAG		"u19"
#define Z80DMA_TAG		"u20"
#define Z80PIO_TAG		"u22"
#define Z80SIO_TAG		"u18"
#define FD1791_TAG		"u6"
#define MC6845_TAG		"u11"
#define CENTRONICS_TAG	"j2"

class trs80m2_state : public driver_device
{
public:
	trs80m2_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* memory state */
	int boot_rom;
	int bank;
	int msel;

	/* floppy state */
	int fdc_intrq;

	/* keyboard state */
	UINT8 key_latch;
	UINT8 key_data;
	int key_bit;
	int kbclk;
	int kbdata;
	int kbirq;

	/* video state */
	UINT8 *video_ram;
	UINT8 *char_rom;
	int blnkvid;
	int _80_40_char_en;
	int de;
	int rtc_int;
	int enable_rtc_int;

	/* devices */
	running_device *z80ctc;
	running_device *z80pio;
	running_device *mc6845;
	running_device *centronics;
	running_device *floppy;
};

#endif
