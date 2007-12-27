#ifndef MBEE_H
#define MBEE_H

/*----------- defined in machine/mbee.c -----------*/

MACHINE_RESET( mbee );
MACHINE_START( mbee );
MACHINE_START( mbee56 );

extern UINT8 *mbee_workram;
READ8_HANDLER( mbee_lowram_r );

void mbee_interrupt(void);

DEVICE_LOAD( mbee_cart );

 READ8_HANDLER ( mbee_pio_r );
WRITE8_HANDLER ( mbee_pio_w );

 READ8_HANDLER ( mbee_fdc_status_r );
WRITE8_HANDLER ( mbee_fdc_motor_w );


/*----------- defined in video/mbee.c -----------*/

extern char mbee_frame_message[128+1];
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

 READ8_HANDLER ( mbee_videoram_r );
WRITE8_HANDLER ( mbee_videoram_w );

 READ8_HANDLER ( mbee_pcg_color_r );
WRITE8_HANDLER ( mbee_pcg_color_w );

VIDEO_START( mbee );
VIDEO_UPDATE( mbee );

#endif /* MBEE_H */
