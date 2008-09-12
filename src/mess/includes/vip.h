#ifndef __VIP__
#define __VIP__

#define SCREEN_TAG "main"

#define CDP1802_TAG "u1"
#define CDP1861_TAG "u2"

#define VIP_BANK_RAM		0
#define VIP_BANK_ROM		1

typedef struct _vip_state vip_state;
struct _vip_state
{
	/* video state */
	int cdp1861_efx;		/* EFx */

	/* keyboard state */
	int keylatch;			/* key latch */
	int reset;				/* reset activated */
};

#endif
