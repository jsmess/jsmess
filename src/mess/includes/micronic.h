#ifndef __MICRONIC__
#define __MICRONIC__

#define SCREEN_TAG		"screen"
#define Z80_TAG			"z80"
#define MC146818_TAG	"mc146818"

class micronic_state : public driver_data_t
{
public:
	static driver_data_t *alloc(running_machine &machine) { return auto_alloc_clear(&machine, micronic_state(machine)); }

	micronic_state(running_machine &machine)
		: driver_data_t(machine) { }

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
	running_device *hd61830;
	running_device *speaker;

};

#endif
