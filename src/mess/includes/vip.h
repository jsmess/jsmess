#ifndef __VIP__
#define __VIP__

#define SCREEN_TAG		"screen"
#define CDP1802_TAG		"u1"
#define CDP1861_TAG		"u2"
#define CDP1862_TAG		"cdp1862"
#define CASSETTE_TAG	"cassette"
#define DISCRETE_TAG	"discrete"

#define VP590_COLOR_RAM_SIZE	0x100

#define VIP_BANK_RAM			0
#define VIP_BANK_ROM			1

#define VIP_VIDEO_CDP1861		0
#define VIP_VIDEO_CDP1862		1

#define VIP_SOUND_SPEAKER		0
#define VIP_SOUND_VP595			1
#define VIP_SOUND_VP550			2
#define VIP_SOUND_VP551			3

#define VIP_KEYBOARD_KEYPAD		0
#define VIP_KEYBOARD_VP580		1
#define VIP_KEYBOARD_DUAL_VP580	2
#define VIP_KEYBOARD_VP601		3
#define VIP_KEYBOARD_VP611		4

#define VIP_LED_POWER			0
#define VIP_LED_Q				1
#define VIP_LED_TAPE			2

typedef struct _vip_state vip_state;
struct _vip_state
{
	/* cpu state */
	int reset;						/* reset activated */

	/* video state */
	int cdp1861_efx;				/* EFx */
	int a12;						/* latched address line 12 */
	UINT8 *colorram;				/* CDP1862 color RAM */
	UINT8 color;					/* color RAM data */

	/* keyboard state */
	int keylatch;					/* key latch */

	/* devices */
	const device_config *cdp1861;
	const device_config *cdp1862;
	const device_config *cdp1863;
	const device_config *cassette;
	const device_config *beeper;
	const device_config *vp550;
	const device_config *vp595;
};

#endif
