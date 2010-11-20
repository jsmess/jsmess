/*****************************************************************************
 *
 * includes/vectrex.h
 *
 ****************************************************************************/

#ifndef VECTREX_H_
#define VECTREX_H_

#include "machine/6522via.h"

/*----------- defined in drivers/vectrex.c -----------*/

extern UINT8 *gce_vectorram;
extern size_t gce_vectorram_size;


/*----------- defined in machine/vectrex.c -----------*/

DEVICE_IMAGE_LOAD( vectrex_cart );

extern int vectrex_imager_status;
extern UINT32 vectrex_beam_color;
extern unsigned char vectrex_via_out[2];
extern double vectrex_imager_freq;
extern emu_timer *vectrex_imager_timer;
extern int vectrex_lightpen_port;
extern int vectrex_reset_refresh;

TIMER_CALLBACK(vectrex_imager_eye);
void vectrex_configuration(running_machine *machine);
READ8_DEVICE_HANDLER (vectrex_via_pa_r);
READ8_DEVICE_HANDLER(vectrex_via_pb_r );
void vectrex_via_irq (running_device *device, int level);
WRITE8_HANDLER ( vectrex_psg_port_w );

DRIVER_INIT( vectrex );

/* for spectrum 1+ */
READ8_DEVICE_HANDLER( vectrex_s1_via_pb_r );


/*----------- defined in video/vectrex.c -----------*/

extern const via6522_interface vectrex_via6522_interface;
extern const via6522_interface spectrum1_via6522_interface;

VIDEO_START( vectrex );
VIDEO_UPDATE( vectrex );

VIDEO_START( raaspec );

WRITE8_HANDLER  ( raaspec_led_w );

READ8_HANDLER ( vectrex_via_r );
WRITE8_HANDLER ( vectrex_via_w );

void vectrex_add_point_stereo (int x, int y, rgb_t color, int intensity);
void vectrex_add_point (int x, int y, rgb_t color, int intensity);
extern void (*vector_add_point_function) (int, int, rgb_t, int);

#endif /* VECTREX_H_ */
