#ifndef __BW2__
#define __BW2__

#define SCREEN_TAG	"screen"
#define Z80_TAG		"ic1"
#define I8255A_TAG	"ic4"
#define WD2797_TAG	"ic5"
#define PIT8253_TAG	"ic6"
#define MSM8251_TAG	"ic7"
#define MSM6255_TAG	"ic49"
#define CENTRONICS_TAG	"centronics"

#define BW2_VIDEORAM_SIZE	0x4000
#define BW2_RAMCARD_SIZE	0x80000

enum {
	BANK_RAM1 = 0,
	BANK_VRAM,
	BANK_RAM2, BANK_RAMCARD_ROM = BANK_RAM2,
	BANK_RAM3,
	BANK_RAM4,
	BANK_RAM5, BANK_RAMCARD_RAM = BANK_RAM5,
	BANK_RAM6,
	BANK_ROM
};

typedef struct _bw2_state bw2_state;
struct _bw2_state
{
	/* keyboard state */
	UINT8 keyboard_row;

	/* memory state */
	UINT8 *work_ram;
	UINT8 *ramcard_ram;
	UINT8 bank;

	/* floppy state */
	int selected_drive;
	int mtron;
	int mfdbk;

	/* video state */
	UINT8 *video_ram;

	/* devices */
	const device_config *msm8251;
	const device_config *msm6255;
	const device_config *centronics;
};

#endif
