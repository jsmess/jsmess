#ifndef __PROF80__
#define __PROF80__

#define SCREEN_TAG				"screen"

#define Z80_TAG					"z1"
#define NEC765_TAG				"z38"
#define UPD1990A_TAG			"z43"
#define MC6845_TAG				"z30"
#define I8255A_TAG				"z6"
#define Z80STI_TAG				"z9"

#define GRIP_Z80_TAG			"grip_z1"

#define UNIO_Z80STI_TAG			"z5"
#define UNIO_Z80SIO_TAG			"z15"
#define UNIO_Z80PIO_TAG			"z13"
#define UNIO_CENTRONICS1_TAG	"n3"
#define UNIO_CENTRONICS2_TAG	"n4"

#define GRIP_VIDEORAM_SIZE	0x10000
#define GRIP_VIDEORAM_MASK	0xffff

typedef struct _prof80_state prof80_state;
struct _prof80_state
{
	/* memory state */
	UINT8 mmu[16];			/* MMU block register */
	int mme;				/* MMU enable */

	/* keyboard state */
	UINT8 keydata;			/* keyboard data */
	int kbf;				/* keyboard buffer full */

	/* video state */
	UINT8 *video_ram;		/* video RAM */
	int lps;				/* light pen sense */
	int page;				/* video page */
	int flash;				/* flash */

	/* clock state */
	int rtc_data;			/* RTC data output */

	/* floppy state */
	int	fdc_index;			/* floppy index hole sensor */
	int motor;				/* floppy motor */

	/* GRIP state */
	UINT8 gripd;			/* GRIP data */
	UINT8 gripc;			/* GRIP status */

	/* devices */
	const device_config *nec765;
	const device_config *upd1990a;
	const device_config *mc6845;
	const device_config *ppi8255;
	const device_config *z80sti;
	const device_config *centronics;

	/* timers */
	emu_timer	*floppy_motor_off_timer;
};

#endif
