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
extern READ8_HANDLER( galaxy_latch_r );
extern WRITE8_HANDLER( galaxy_latch_w );

extern MACHINE_RESET( galaxyp );

extern UINT32 gal_cnt;
extern UINT8 gal_latch_value;

extern DRIVER_INIT( galaxyp );
extern MACHINE_RESET( galaxyp );
/*----------- defined in video/galaxy.c -----------*/

extern const gfx_layout galaxy_charlayout;
extern const unsigned char galaxy_palette[2*3];
extern PALETTE_INIT( galaxy );

extern VIDEO_START( galaxy );
extern VIDEO_UPDATE( galaxy );
extern TIMER_CALLBACK( gal_video );

extern emu_timer *gal_video_timer;

#endif /* GALAXY_H_ */
