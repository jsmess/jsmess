#ifndef __TMC2000E__
#define __TMC2000E__

#define SCREEN_TAG		"screen"
#define CDP1802_TAG		"cdp1802"
#define CDP1864_TAG		"cdp1864"
#define CASSETTE_TAG	"cassette"

#define TMC2000E_COLORRAM_SIZE 0x100 // ???

typedef struct _tmc2000e_state tmc2000e_state;
struct _tmc2000e_state
{
	/* video state */
	int cdp1864_efx;		/* EFx */
	UINT8 *colorram;		/* color memory */
	UINT8 color;

	/* keyboard state */
	int keylatch;			/* key latch */
	int reset;				/* reset activated */

	/* devices */
	const device_config *cdp1864;
	const device_config *cassette;
};

#endif
