/*****************************************************************************
 *
 * includes/galaxy.h
 *
 ****************************************************************************/

#ifndef GALAXY_H_
#define GALAXY_H_

#include "devices/snapquik.h"


/*----------- defined in machine/galaxy.c -----------*/

extern int galaxy_interrupts_enabled;

extern DRIVER_INIT( galaxy );
extern MACHINE_RESET( galaxy );
extern INTERRUPT_GEN( galaxy_interrupt );
extern SNAPSHOT_LOAD( galaxy );
extern READ8_HANDLER( galaxy_keyboard_r );
extern WRITE8_HANDLER( galaxy_latch_w );

extern MACHINE_RESET( galaxyp );

extern UINT8 galaxy_latch_value;

extern DRIVER_INIT( galaxyp );

/*----------- defined in video/galaxy.c -----------*/

extern UINT32 gal_cnt;
extern VIDEO_UPDATE( galaxy );
extern TIMER_CALLBACK( gal_video );

extern emu_timer *gal_video_timer;

#endif /* GALAXY_H_ */
