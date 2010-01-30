#ifndef __TMC600__
#define __TMC600__

#define SCREEN_TAG		"screen"
#define CDP1802_TAG		"cdp1802"
#define CDP1869_TAG		"cdp1869"
#define CASSETTE_TAG	"cassette"
#define CENTRONICS_TAG	"centronics"

#define TMC600_PAGE_RAM_SIZE	0x400
#define TMC600_PAGE_RAM_MASK	0x3ff

typedef struct _tmc600_state tmc600_state;
struct _tmc600_state
{
	/* video state */
	int vismac_reg_latch;	/* video register latch */
	int vismac_color_latch;	/* color latch */
	int vismac_bkg_latch;	/* background color latch */
	int blink;				/* cursor blink */

	UINT8 *page_ram;		/* page memory */
	UINT8 *color_ram;		/* color memory */
	UINT8 *char_rom;		/* character generator ROM */

	/* keyboard state */
	int keylatch;			/* key latch */

	/* devices */
	running_device *cdp1869;
	running_device *cassette;
};

/* ---------- defined in video/tmc600.c ---------- */

WRITE8_HANDLER( tmc600_vismac_register_w );
WRITE8_DEVICE_HANDLER( tmc600_vismac_data_w );

MACHINE_DRIVER_EXTERN( tmc600_video );

#endif
