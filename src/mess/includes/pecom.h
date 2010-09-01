#ifndef __PECOM__
#define __PECOM__

#define SCREEN_TAG	"screen"
#define CDP1869_TAG	"cdp1869"

#define PECOM_PAGE_RAM_SIZE	0x400
#define PECOM_PAGE_RAM_MASK	0x3ff

class pecom_state : public driver_data_t
{
public:
	static driver_data_t *alloc(running_machine &machine) { return auto_alloc_clear(&machine, pecom_state(machine)); }

	pecom_state(running_machine &machine)
		: driver_data_t(machine) { }

	UINT8 *page_ram;		/* page memory */
	UINT8 *charram;			/* character generator ROM */
	int cdp1802_mode;		/* CPU mode */
	int dma;				/* memory refresh DMA */
	running_device *cdp1869;

	/* timers */
	emu_timer *reset_timer;	/* power on reset timer */

};

/*----------- defined in machine/pecom.c -----------*/

extern MACHINE_START( pecom );
extern MACHINE_RESET( pecom );
extern WRITE8_HANDLER( pecom_bank_w );
extern READ8_HANDLER (pecom_keyboard_r);

extern const cdp1802_interface pecom64_cdp1802_config;

/* ---------- defined in video/pecom.c ---------- */

MACHINE_CONFIG_EXTERN( pecom_video );

#endif
