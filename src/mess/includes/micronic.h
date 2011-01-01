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

	UINT8 *ram;
	UINT8 kp_matrix;
	UINT8 lcd_contrast;
	UINT8 lcd_light;
	UINT8 status_flag;

	/* rtc-146818 */
	UINT8 rtc_address;
	UINT8 periodic_irq;
	UINT8 irq_flags;

	emu_timer *rtc_periodic_irq;

	/* devices */
	hd61830_device *hd61830;
	device_t *speaker;

};

#endif
