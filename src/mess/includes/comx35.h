#ifndef __COMX35__
#define __COMX35__

#define SCREEN_TAG	"main"

#define CDP1870_TAG "U1"
#define CDP1869_TAG	"U2"
#define CDP1802_TAG "U3"
#define CDP1871_TAG	"U4"

typedef struct _comx35_state comx35_state;
struct _comx35_state
{
	/* processor state */
	int cdp1802_mode;		/* CPU mode */
	int iden;				/* interrupt enable */

	/* video state */
	int pal_ntsc;			/* PAL/NTSC */
	int cdp1869_prd;		/* predisplay */

	UINT8 *pageram;			/* page memory */
	UINT8 *charram;			/* character memory */

	/* keyboard state */
	int cdp1871_efxa;		/* keyboard data available */
	int cdp1871_efxb;		/* keyboard repeat */

	/* timers */
	emu_timer *reset_timer;	/* power on reset timer */
	emu_timer *dma_timer;	/* memory refresh timer */
};

/* ---------- defined in video/comx35.c ---------- */

MACHINE_DRIVER_EXTERN( comx35p_video );
MACHINE_DRIVER_EXTERN( comx35n_video );

#endif
