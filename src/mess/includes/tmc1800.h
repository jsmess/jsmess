#ifndef __TMC1800__
#define __TMC1800__

#define SCREEN_TAG "main"

#define CDP1861_TAG "cdp1861"
#define CDP1864_TAG "M3"

typedef struct _tmc1800_state tmc1800_state;
struct _tmc1800_state
{
	/* video state */
	int cdp1861_efx;		/* EFx */

	/* keyboard state */
	int keylatch;			/* key latch */
	int reset;				/* reset activated */
};

typedef struct _osc1000b_state osc1000b_state;
struct _osc1000b_state
{
	/* video state */
	int cdp1861_efx;		/* EFx */

	/* keyboard state */
	int keylatch;			/* key latch */
	int reset;				/* reset activated */
};

typedef struct _tmc2000_state tmc2000_state;
struct _tmc2000_state
{
	/* video state */
	int cdp1864_efx;		/* EFx */

	UINT8 *colorram;		/* color memory */

	/* keyboard state */
	int keylatch;			/* key latch */
	int reset;				/* reset activated */
};

typedef struct _oscnano_state oscnano_state;
struct _oscnano_state
{
	/* cpu state */
	int monitor_ef4;		/* EF4 line */

	/* video state */
	int cdp1864_efx;		/* EFx */

	/* keyboard state */
	int keylatch;			/* key latch */
	int reset;				/* reset activated */

	/* timers */
	emu_timer *ef4_timer;	/* EF4 line RC timer */
};

/* ---------- defined in video/tmc1800.c ---------- */

MACHINE_DRIVER_EXTERN( tmc1800_video );
MACHINE_DRIVER_EXTERN( osc1000b_video );
MACHINE_DRIVER_EXTERN( tmc2000_video );
MACHINE_DRIVER_EXTERN( oscnano_video );

#endif
