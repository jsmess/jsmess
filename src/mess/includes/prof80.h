#ifndef __PROF80__
#define __PROF80__

#define SCREEN_TAG		"screen"

#define Z80_TAG			"z1"
#define NEC765_TAG		"z38"
#define UPD1990A_TAG	"z43"
#define MC6845_TAG		"z30"
#define PPI8255_TAG		"z6"
#define Z80STI_TAG		"z9"

#define GRIP_VIDEORAM_SIZE	0x10000
#define GRIP_VIDEORAM_MASK	0xffff

typedef struct _prof80_state prof80_state;
struct _prof80_state
{
	/* memory state */
	UINT8 mmu[16];			/* MMU block register */
	int mme;				/* MMU enable */

	/* video state */
	UINT8 *video_ram;		/* video RAM */
	int vsync;				/* vertical sync */
	int lps;				/* light pen sense */

	/* clock state */
	int rtc_data;			/* RTC data output */

	/* floppy state */
	int	fdc_index;			/* floppy index hole sensor */

	/* devices */
	const device_config *nec765;
	const device_config *upd1990a;
	const device_config *mc6845;
	const device_config *centronics;
};

#endif
