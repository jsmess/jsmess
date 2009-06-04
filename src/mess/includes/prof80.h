#ifndef __PROF80__
#define __PROF80__

#define SCREEN_TAG		"screen"

#define Z80_TAG			"z1"
#define NEC765_TAG		"z38"
#define UPD1990A_TAG	"z43"
#define MC6845_TAG		"z30"
#define PPI8255_TAG		"z6"
#define Z80STI_TAG		"z9"

typedef struct _prof80_state prof80_state;
struct _prof80_state
{
	/* clock state */
	int rtc_data;			/* RTC data output */

	const device_config *upd1990a;
};

#endif
