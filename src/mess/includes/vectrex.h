/*****************************************************************************
 *
 * includes/vectrex.h
 *
 ****************************************************************************/

#ifndef VECTREX_H_
#define VECTREX_H_


/*----------- defined in machine/vectrex.c -----------*/

DEVICE_IMAGE_LOAD( vectrex_cart );

extern int vectrex_imager_status;
extern UINT32 vectrex_beam_color;
extern unsigned char vectrex_via_out[2];
extern double imager_freq;
extern emu_timer *imager_timer;
extern int vectrex_lightpen_port;
extern int vectrex_reset_refresh;

TIMER_CALLBACK(vectrex_imager_eye);
void vectrex_configuration(running_machine *machine);
READ8_HANDLER (v_via_pa_r);
READ8_HANDLER(v_via_pb_r );
void v_via_irq (running_machine *machine, int level);
WRITE8_HANDLER ( vectrex_psg_port_w );

DRIVER_INIT( vectrex );

/* for spectrum 1+ */
READ8_HANDLER( s1_via_pb_r );


/*----------- defined in video/vectrex.c -----------*/

VIDEO_START( vectrex );
VIDEO_UPDATE( vectrex );

VIDEO_START( raaspec );

WRITE8_HANDLER  ( raaspec_led_w );
WRITE8_HANDLER ( vectrex_via_w ); 

void vectrex_add_point_stereo (int x, int y, rgb_t color, int intensity);
void vectrex_add_point (int x, int y, rgb_t color, int intensity);
extern void (*vector_add_point_function) (int, int, rgb_t, int);

#endif /* VECTREX_H_ */
