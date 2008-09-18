#ifndef __VIP__
#define __VIP__

#define SCREEN_TAG "main"

#define CDP1802_TAG "u1"
#define CDP1861_TAG "u2"
#define CDP1862_TAG "cdp1862"

#define VP590_COLOR_RAM_SIZE	0x100

#define VIP_BANK_RAM		0
#define VIP_BANK_ROM		1

#define VIP_VIDEO_CDP1861	0
#define VIP_VIDEO_CDP1862	1

typedef struct _vip_state vip_state;
struct _vip_state
{
	/* video state */
	int cdp1861_efx;		/* EFx */
	UINT8 *colorram;		/* CDP1862 color RAM */

	/* keyboard state */
	int keylatch;			/* key latch */
	int reset;				/* reset activated */

	/* devices */
	const device_config *cdp1861;
	const device_config *cdp1862;
};

#endif
