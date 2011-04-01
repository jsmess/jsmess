#ifndef __MICRONIC__
#define __MICRONIC__

#include "video/hd61830.h"

#define SCREEN_TAG		"screen"
#define Z80_TAG			"z80"
#define MC146818_TAG	"mc146818"
#define HD61830_TAG		"hd61830"

class micronic_state : public driver_device
{
public:
	micronic_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *m_ram;
	UINT8 m_kp_matrix;
	UINT8 m_lcd_contrast;
	UINT8 m_lcd_light;
	UINT8 m_status_flag;

	/* rtc-146818 */
	UINT8 m_rtc_address;
	UINT8 m_periodic_irq;
	UINT8 m_irq_flags;

	emu_timer *m_rtc_periodic_irq;

	/* devices */
	hd61830_device *m_hd61830;
	device_t *m_speaker;

};

#endif
