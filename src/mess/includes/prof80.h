#ifndef __PROF80__
#define __PROF80__

#define Z80_TAG					"z1"
#define UPD765_TAG				"z38"
#define UPD1990A_TAG			"z43"

/* ------------------------------------------------------------------------ */

#define SCREEN_TAG				"screen"
#define GRIP_Z80_TAG			"grip_z1"
#define MC6845_TAG				"z30"
#define I8255A_TAG				"z6"
#define Z80STI_TAG				"z9"
#define SPEAKER_TAG				"speaker"
#define CENTRONICS_TAG			"centronics"

#define GRIP_VIDEORAM_SIZE	0x10000
#define GRIP_VIDEORAM_MASK	0xffff

/* ------------------------------------------------------------------------ */

#define UNIO_Z80STI_TAG			"z5"
#define UNIO_Z80SIO_TAG			"z15"
#define UNIO_Z80PIO_TAG			"z13"
#define UNIO_CENTRONICS1_TAG	"n3"
#define UNIO_CENTRONICS2_TAG	"n4"

class prof80_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, prof80_state(machine)); }

	prof80_state(running_machine &machine) { }

	/* memory state */
	UINT8 mmu[16];			/* MMU block register */
	int init;				/* MMU enable */

	/* RTC state */
	int c0;
	int c1;
	int c2;

	/* floppy state */
	int	fdc_index;			/* floppy index hole sensor */
	int motor;				/* floppy motor */

	/* GRIP state */
	UINT8 gripd;			/* GRIP data */
	UINT8 gripc;			/* GRIP status */

	/* devices */
	running_device *upd765;
	running_device *upd1990a;

	/* timers */
	emu_timer	*floppy_motor_off_timer;

/* ------------------------------------------------------------------------ */

	/* sound state */
	int vol0;
	int vol1;

	/* keyboard state */
	UINT8 keydata;			/* keyboard data */
	int kbf;				/* keyboard buffer full */

	/* video state */
	UINT8 *video_ram;		/* video RAM */
	int lps;				/* light pen sense */
	int page;				/* video page */
	int flash;				/* flash */

	/* devices */
	running_device *mc6845;
	running_device *ppi8255;
	running_device *z80sti;
	running_device *centronics;
};

#endif
