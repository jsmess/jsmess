/*****************************************************************************
 *
 * includes/galaxy.h
 *
 ****************************************************************************/

#ifndef GALAXY_H_
#define GALAXY_H_

#include "imagedev/snapquik.h"


class galaxy_state : public driver_device
{
public:
	galaxy_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

	int m_interrupts_enabled;
	UINT8 m_latch_value;
	UINT32 m_gal_cnt;
	UINT8 m_code;
	UINT8 m_first;
	UINT32 m_start_addr;
	emu_timer *m_gal_video_timer;
	bitmap_ind16 m_bitmap;
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
SCREEN_UPDATE_IND16( galaxy );

void galaxy_set_timer(running_machine &machine);

#endif /* GALAXY_H_ */
