#ifndef __STUDIO2__
#define __STUDIO2__

#include "cpu/cosmac/cosmac.h"

#define CDP1802_TAG "ic1"
#define CDP1861_TAG "ic2"
#define CDP1864_TAG "cdp1864"
#define SCREEN_TAG	"screen"

class studio2_state : public driver_device
{
public:
	studio2_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, CDP1802_TAG),
		  m_vdc(*this, CDP1861_TAG),
		  m_cti(*this, CDP1864_TAG)
	{ }

	required_device<cosmac_device> m_maincpu;
	optional_device<device_t> m_vdc;
	optional_device<device_t> m_cti;

	virtual void machine_start();
	virtual void machine_reset();

	DECLARE_READ8_MEMBER(dispon_r);

	DECLARE_WRITE8_MEMBER(keylatch_w);
	DECLARE_WRITE8_MEMBER(dispon_w);

	/* keyboard state */
	UINT8 m_keylatch;

	/* video state */
	UINT8 *m_color_ram;
	UINT8 *m_color_ram1;
	UINT8 m_color;
};

#endif
