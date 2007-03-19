#ifndef __VECTREX_H__
#define __VECTREX_H__

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

/* From machine/vectrex.c */
DEVICE_LOAD( vectrex_cart );

/* From machine/vectrex.c */
extern int vectrex_imager_status;
extern UINT32 vectrex_beam_color;
extern unsigned char vectrex_via_out[2];
extern double imager_freq;
extern mame_timer *imager_timer;
extern int vectrex_lightpen_port;
extern UINT8 *vectrex_ram_base;
extern size_t vectrex_ram_size;

void vectrex_imager_right_eye (int param);
void vectrex_configuration(void);
 READ8_HANDLER (v_via_pa_r);
 READ8_HANDLER(v_via_pb_r );
void v_via_irq (int level);
WRITE8_HANDLER ( vectrex_psg_port_w );

DRIVER_INIT( vectrex );

/* for spectrum 1+ */
 READ8_HANDLER( s1_via_pb_r );

/* From vidhrdw/vectrex.c */
VIDEO_START( vectrex );
VIDEO_UPDATE( vectrex );

VIDEO_START( raaspec );
VIDEO_UPDATE( raaspec );

WRITE8_HANDLER  ( raaspec_led_w );


/* from vidhrdw/vectrex.c */
void vectrex_add_point_stereo (int x, int y, rgb_t color, int intensity);
void vectrex_add_point (int x, int y, rgb_t color, int intensity);
extern void (*vector_add_point_function) (int, int, rgb_t, int);

#endif
