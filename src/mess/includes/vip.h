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

class vip_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, vip_state(machine)); }

	vip_state(running_machine &machine) { }

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
	running_device *cdp1861;
	running_device *cdp1862;
	running_device *cdp1863;
	running_device *cassette;
	running_device *beeper;
	running_device *vp595;
	running_device *vp550;
	running_device *vp551;
};

#endif
