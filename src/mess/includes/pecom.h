#ifndef __PECOM__
#define __PECOM__

#define SCREEN_TAG	"screen"
#define CDP1869_TAG	"cdp1869"

#define PECOM_PAGE_RAM_SIZE	0x400
#define PECOM_PAGE_RAM_MASK	0x3ff

typedef struct _pecom_state pecom_state;

struct _pecom_state
{
	UINT8 *page_ram;		/* page memory */
	UINT8 *charram;			/* character generator ROM */
	int cdp1802_mode;		/* CPU mode */
	int dma;				/* memory refresh DMA */
	const device_config *cdp1869;

	/* timers */
	emu_timer *reset_timer;	/* power on reset timer */

};

/*----------- defined in machine/pecom.c -----------*/

extern DRIVER_INIT( pecom );
extern MACHINE_START( pecom );
extern MACHINE_RESET( pecom );
extern WRITE8_HANDLER( pecom_bank_w );
extern READ8_HANDLER (pecom_keyboard_r);
extern const cdp1802_interface ( pecom64_cdp1802_config );

/* ---------- defined in video/pecom.c ---------- */

MACHINE_DRIVER_EXTERN( pecom_video );

#endif
