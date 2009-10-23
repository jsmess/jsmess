/*****************************************************************************
 *
 * includes/vtech2.h
 *
 ****************************************************************************/

#ifndef VTECH2_H_
#define VTECH2_H_


/*----------- defined in machine/vtech2.c -----------*/

extern int laser_latch;

DRIVER_INIT(laser);
MACHINE_RESET( laser350 );
MACHINE_RESET( laser500 );
MACHINE_RESET( laser700 );

DEVICE_IMAGE_LOAD( laser_cart );
DEVICE_IMAGE_UNLOAD( laser_cart );

extern  READ8_HANDLER ( laser_fdc_r );
extern WRITE8_HANDLER ( laser_fdc_w );
extern WRITE8_HANDLER ( laser_bank_select_w );


/*----------- defined in video/vtech2.c -----------*/

extern char laser_frame_message[64+1];
extern int laser_frame_time;

extern VIDEO_START( laser );
extern VIDEO_UPDATE( laser );
extern WRITE8_HANDLER ( laser_bg_mode_w );
extern WRITE8_HANDLER ( laser_two_color_w );


#endif /* VTECH2_H_ */
