#ifndef __PECOM__
#define __PECOM__

#include "cpu/cosmac/cosmac.h"

#define SCREEN_TAG	"screen"
#define CDP1802_TAG	"cdp1802"
#define CDP1869_TAG	"cdp1869"

#define PECOM_CHAR_RAM_SIZE	0x800

class pecom_state : public driver_device
{
public:
	pecom_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_cdp1802(*this, CDP1802_TAG),
		  m_cdp1869(*this, CDP1869_TAG)
	{ }

	required_device<cosmac_device> m_cdp1802;
	required_device<cdp1869_device> m_cdp1869;

	UINT8 *m_charram;			/* character generator ROM */
	int m_reset;				/* CPU mode */
	int m_dma;				/* memory refresh DMA */

	/* timers */
	emu_timer *m_reset_timer;	/* power on reset timer */

};

/*----------- defined in machine/pecom.c -----------*/

extern MACHINE_START( pecom );
extern MACHINE_RESET( pecom );
extern WRITE8_HANDLER( pecom_bank_w );
extern READ8_HANDLER (pecom_keyboard_r);

extern const cosmac_interface pecom64_cdp1802_config;

/* ---------- defined in video/pecom.c ---------- */

WRITE8_HANDLER( pecom_cdp1869_w );
MACHINE_CONFIG_EXTERN( pecom_video );

#endif
