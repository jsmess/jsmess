/*****************************************************************************
 *
 * includes/galaxy.h
 *
 ****************************************************************************/

#ifndef GALAXY_H_
#define GALAXY_H_

#include "devices/snapquik.h"


class galaxy_state : public driver_device
{
public:
	galaxy_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	int interrupts_enabled;
	UINT8 latch_value;
	UINT32 gal_cnt;
	UINT8 code;
	UINT8 first;
	UINT32 start_addr;
	emu_timer *gal_video_timer;
};


/*----------- defined in machine/galaxy.c -----------*/

DRIVER_INIT( galaxy );
MACHINE_RESET( galaxy );
INTERRUPT_GEN( galaxy_interrupt );
SNAPSHOT_LOAD( galaxy );
READ8_HANDLER( galaxy_keyboard_r );
WRITE8_HANDLER( galaxy_latch_w );

MACHINE_RESET( galaxyp );


DRIVER_INIT( galaxyp );

/*----------- defined in video/galaxy.c -----------*/

VIDEO_START( galaxy );
VIDEO_UPDATE( galaxy );

void galaxy_set_timer(running_machine *machine);

#endif /* GALAXY_H_ */
