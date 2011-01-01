/*****************************************************************************
 *
 * includes/vectrex.h
 *
 ****************************************************************************/

#ifndef VECTREX_H_
#define VECTREX_H_

#include "machine/6522via.h"

#define NVECT 10000

typedef struct _vectrex_point
{
	int x; int y;
	rgb_t col;
	int intensity;
} vectrex_point;

class vectrex_state : public driver_device
{
public:
	vectrex_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *gce_vectorram;
	size_t gce_vectorram_size;
	int imager_status;
	UINT32 beam_color;
	unsigned char via_out[2];
	double imager_freq;
	emu_timer *imager_timer;
	int lightpen_port;
	int reset_refresh;
	rgb_t imager_colors[6];
	const double *imager_angles;
	unsigned char imager_pinlevel;
	int old_mcontrol;
	double sl;
	double pwl;
	int x_center;
	int y_center;
	int x_max;
	int y_max;
	int x_int;
	int y_int;
	int lightpen_down;
	int pen_x;
	int pen_y;
	emu_timer *lp_t;
	emu_timer *refresh;
	UINT8 blank;
	UINT8 ramp;
	INT8 analog[5];
	int point_index;
	int display_start;
	int display_end;
	vectrex_point points[NVECT];
	UINT16 via_timer2;
	attotime vector_start_time;
	UINT8 cb2;
};


/*----------- defined in machine/vectrex.c -----------*/

DEVICE_IMAGE_LOAD( vectrex_cart );


TIMER_CALLBACK(vectrex_imager_eye);
void vectrex_configuration(running_machine *machine);
READ8_DEVICE_HANDLER (vectrex_via_pa_r);
READ8_DEVICE_HANDLER(vectrex_via_pb_r );
void vectrex_via_irq (device_t *device, int level);
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

void vectrex_add_point_stereo (running_machine *machine, int x, int y, rgb_t color, int intensity);
void vectrex_add_point (running_machine *machine, int x, int y, rgb_t color, int intensity);
extern void (*vector_add_point_function) (running_machine *, int, int, rgb_t, int);

#endif /* VECTREX_H_ */
