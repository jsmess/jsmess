/*****************************************************************************
 *
 * includes/mbee.h
 *
 ****************************************************************************/

#ifndef MBEE_H_
#define MBEE_H_

#include "cpu/z80/z80daisy.h"

/*----------- defined in machine/mbee.c -----------*/

MACHINE_RESET( mbee );
MACHINE_START( mbee );

extern UINT8 *mbee_workram;

DEVICE_IMAGE_LOAD( mbee_cart );

 READ8_HANDLER ( mbee_fdc_status_r );
WRITE8_HANDLER ( mbee_fdc_motor_w );


/*----------- defined in video/mbee.c -----------*/

extern int mbee_frame_counter;
extern UINT8 *pcgram;

 READ8_HANDLER ( m6545_status_r );
WRITE8_HANDLER ( m6545_index_w );
 READ8_HANDLER ( m6545_data_r );
WRITE8_HANDLER ( m6545_data_w );

 READ8_HANDLER ( mbee_color_bank_r );
WRITE8_HANDLER ( mbee_color_bank_w );
 READ8_HANDLER ( mbee_video_bank_r );
WRITE8_HANDLER ( mbee_video_bank_w );

 READ8_HANDLER ( mbee_pcg_color_latch_r );
WRITE8_HANDLER ( mbee_pcg_color_latch_w );

WRITE8_HANDLER ( mbee_videoram_w );
WRITE8_HANDLER ( mbee_pcg_color_w );
WRITE8_HANDLER ( mbee_pcg_w );

VIDEO_START( mbee );
VIDEO_UPDATE( mbee );

VIDEO_START( mbeeic );
VIDEO_UPDATE( mbeeic );


#endif /* MBEE_H_ */
