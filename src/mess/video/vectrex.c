#include <math.h>
#include "driver.h"
#include "includes/vectrex.h"
#include "video/vector.h"
#include "machine/6522via.h"
#include "cpu/m6809/m6809.h"
#include "sound/ay8910.h"
#include "sound/dac.h"


#define ANALOG_DELAY 7800
#define NVECT 10000

#define INT_PER_CLOCK 550

#ifndef M_SQRT1_2
#define M_SQRT1_2 0.70710678118654752440
#endif

/*********************************************************************

   Enums and typedefs

*********************************************************************/

enum {
	PORTB = 0,
	PORTA
};

enum {
	A_X = 0,
	A_ZR,
	A_Z,
	A_AUDIO,
	A_Y,
};

typedef struct _vectrex_point
{
	int x; int y;
	rgb_t col;
	int intensity;
} vectrex_point;


/*********************************************************************

   Prototypes

*********************************************************************/

static WRITE8_HANDLER (v_via_pa_w);
static WRITE8_HANDLER(v_via_pb_w);
static WRITE8_HANDLER (v_via_ca2_w);
static WRITE8_HANDLER (v_via_cb2_w);


/*********************************************************************

   Local variables

*********************************************************************/

static const struct via6522_interface vectrex_via6522_interface =
{
	v_via_pa_r, v_via_pb_r,         /* read PA/B */
	0, 0, 0, 0,                     /* read ca1, cb1, ca2, cb2 */
	v_via_pa_w, v_via_pb_w,         /* write PA/B */
	0, 0, v_via_ca2_w, v_via_cb2_w, /* write ca1, cb1, ca2, cb2 */
	v_via_irq,                      /* IRQ */
};

static int x_center, y_center, x_max, y_max;
static int x_int, y_int; /* X, Y integrators */
static int lightpen_down, pen_x, pen_y; /* Lightpen position */
static emu_timer *lp_t;
static emu_timer *refresh;
static UINT8 blank;
static UINT8 ramp;
static INT8 analog[5];
static int vectrex_point_index;
static int display_start, display_end;
static vectrex_point vectrex_points[NVECT];
static UINT16 via_timer2;
static attotime vector_start_time;

void (*vector_add_point_function) (int, int, rgb_t, int) = vectrex_add_point;


/*********************************************************************

   Lightpen

*********************************************************************/

static TIMER_CALLBACK(lightpen_trigger)
{
	if (vectrex_lightpen_port & 1)
	{
		via_set_input_ca1(machine, 0, 1);
		via_set_input_ca1(machine, 0, 0);
	}

	if (vectrex_lightpen_port & 2)
	{
		cpu_set_input_line(machine->cpu[0], M6809_FIRQ_LINE, PULSE_LINE);
	}
}


/*********************************************************************

   VIA T2 configuration

   We need to snoop the frequency of VIA timer 2 here since most
   vectrex games use that timer for steady screen refresh. Even if the
   game stops T2 we continue refreshing the screen with the last known
   frequency. Note that we quickly get out of sync in this case and the
   screen will start flickering (see cut scenes in Spike).

   Note that the timer can be adjusted to the full period each time T2
   is restarted. This behaviour avoids flicker in most games. Some
   games like mine 3d don't work well with this scheme though and show
   severe jerking. So the second option is to leave the current period
   alone (if the new period isn't shorter) and change only the next
   full period.

*********************************************************************/

WRITE8_HANDLER(vectrex_via_w) 
{ 
	attotime period;

	switch (offset)
	{
	case 8:
		via_timer2 = (via_timer2 & 0xff00) | data;
		break;

	case 9:
		via_timer2 = (via_timer2 & 0x00ff) | (data << 8);

		period = attotime_mul(ATTOTIME_IN_HZ(cpu_get_clock(space->machine->cpu[0])), via_timer2);

		if (vectrex_reset_refresh)
			timer_adjust_periodic(refresh, period, 0, period);
		else
			timer_adjust_periodic(refresh,
								  attotime_min(period, timer_timeleft(refresh)),
								  0,
								  period);
		break;
	}
	via_write(space->machine, 0, offset, data); 
}


/*********************************************************************

   Screen refresh

*********************************************************************/

static TIMER_CALLBACK(vectrex_refresh)
{
	/* Refresh only marks the range of vectors which will be drawn
	 * during the next VIDEO_UPDATE. */
	display_start = display_end;
	display_end = vectrex_point_index;
}


VIDEO_UPDATE(vectrex)
{
	int i;

	vectrex_configuration(screen->machine);

	/* start black */
	vector_add_point(vectrex_points[display_start].x, 
					 vectrex_points[display_start].y, 
					 vectrex_points[display_start].col,
					 0);

	for (i = display_start; i != display_end; i = (i + 1) % NVECT)
	{
		vector_add_point(vectrex_points[i].x,
						 vectrex_points[i].y,
						 vectrex_points[i].col,
						 vectrex_points[i].intensity);
	}

	VIDEO_UPDATE_CALL(vector);
	vector_clear_list();
	return 0;
}


/*********************************************************************

   Vector functions

*********************************************************************/

void vectrex_add_point(int x, int y, rgb_t color, int intensity)
{
	vectrex_point *newpoint;

	vectrex_point_index = (vectrex_point_index+1) % NVECT;
	newpoint = &vectrex_points[vectrex_point_index];

	newpoint->x = x;
	newpoint->y = y;
	newpoint->col = color;
	newpoint->intensity = intensity;
}


void vectrex_add_point_stereo(int x, int y, rgb_t color, int intensity)
{
	if (vectrex_imager_status == 2) /* left = 1, right = 2 */
		vectrex_add_point ((int)(y * M_SQRT1_2)+ x_center, 
						   (int)(((x_max - x) * M_SQRT1_2)), 
						   color, 
						   intensity);
	else
		vectrex_add_point ((int)(y * M_SQRT1_2),
						   (int)((x_max - x) * M_SQRT1_2),
						   color,
						   intensity);
}


static TIMER_CALLBACK(vectrex_zero_integrators)
{
	x_int = x_center + (analog[A_ZR] * INT_PER_CLOCK);
	y_int = y_center + (analog[A_ZR] * INT_PER_CLOCK);
	vector_add_point_function (x_int, y_int, vectrex_beam_color, 0);
}


/*********************************************************************

   Delayed signals

   The RAMP signal is delayed wrt. beam blanking. Getting this right
   is important for text placement, the maze in Clean Sweep and many
   other games.

*********************************************************************/

static TIMER_CALLBACK(update_signal)
{
	int length;

	if (!ramp)
	{
		length = cpu_get_clock(machine->cpu[0]) * INT_PER_CLOCK 
			* attotime_to_double(attotime_sub(timer_get_time(), vector_start_time));

		x_int += length * (analog[A_X] + analog[A_ZR]);
		y_int += length * (analog[A_Y] + analog[A_ZR]);

		vector_add_point_function(x_int, y_int, vectrex_beam_color, 2 * analog[A_Z] * blank);
	}
	else
	{
		if (blank)
			vector_add_point_function(x_int, y_int, vectrex_beam_color, 2 * analog[A_Z]);
	}

	vector_start_time = timer_get_time();

	if (ptr)
		* (UINT8 *) ptr = param;
}


/*********************************************************************

   Startup

*********************************************************************/

VIDEO_START(vectrex)
{
	const device_config *screen = video_screen_first(machine->config);
	const rectangle *visarea = video_screen_get_visible_area(screen);

	x_center=((visarea->max_x - visarea->min_x) / 2) << 16;
	y_center=((visarea->max_y - visarea->min_y) / 2) << 16;
	x_max = visarea->max_x << 16;
	y_max = visarea->max_y << 16;

	via_config(0, &vectrex_via6522_interface);
	via_reset();

	imager_freq = 1;

	imager_timer = timer_alloc(vectrex_imager_eye, NULL);
	timer_adjust_periodic(imager_timer, 
						  ATTOTIME_IN_HZ(imager_freq),
						  2,
						  ATTOTIME_IN_HZ(imager_freq));

	lp_t = timer_alloc(lightpen_trigger, NULL);

	/* Switch off crosshairs since most games don't use lightpen */
	crosshair_toggle(machine);

	refresh = timer_alloc(vectrex_refresh, NULL);

	VIDEO_START_CALL(vector);
}


/*********************************************************************

   VIA interface functions

*********************************************************************/

static void vectrex_multiplexer(int mux)
{
	timer_set(ATTOTIME_IN_NSEC(ANALOG_DELAY), &analog[mux], vectrex_via_out[PORTA], update_signal);

	if (mux == A_AUDIO)
		dac_data_w(0, vectrex_via_out[PORTA]);
}


static WRITE8_HANDLER(v_via_pb_w)
{
	if (!(data & 0x80))
	{
		/* RAMP is active */
		if ((ramp & 0x80))
		{
			/* RAMP was inactive before */

			if (lightpen_down)
			{
				/* Simple lin. algebra to check if pen is near
				 * the line defined by (A_X,A_Y).
				 * If that is the case, set a timer which goes
				 * off when the beam reaches the pen. Exact
				 * timing is important here.
				 *
				 *    lightpen
				 *       ^
				 *  _   /|
				 *  b  / |
				 *    /  |
				 *   /   |d
				 *  /    |
				 * /     |
				 * ------+---------> beam path
				 *    l  |    _
				 *            a
				 */
				double a2, b2, ab, d2;
				ab = (pen_x - x_int) * analog[A_X]
					+(pen_y - y_int) * analog[A_Y];
				if (ab > 0)
				{
					a2 = (double)(analog[A_X] * analog[A_X]
								  +(double)analog[A_Y] * analog[A_Y]);
					b2 = (double)(pen_x - x_int) * (pen_x - x_int)
						+(double)(pen_y - y_int) * (pen_y - y_int);
					d2 = b2 - ab * ab / a2;
					if (d2 < 2e10 && analog[A_Z] * blank > 0)
						timer_adjust_oneshot(lp_t, double_to_attotime(ab / a2 / (cpu_get_clock(space->machine->cpu[0]) * INT_PER_CLOCK)), 0);
				}
			}
		}

		if (!(data & 0x1) && (vectrex_via_out[PORTB] & 0x1))
		{
			/* MUX has been enabled */
			timer_set(ATTOTIME_IN_NSEC(ANALOG_DELAY), NULL, 0, update_signal);
		}
	}
	else
	{
		/* RAMP is inactive */
		if (!(ramp & 0x80))
		{
			/* Cancel running timer, line already finished */
			if (lightpen_down)
				timer_adjust_oneshot(lp_t, attotime_never, 0);
		}
	}

	/* Sound */
	if (data & 0x10)
	{
		if (data & 0x08) /* BC1 (do we select a reg or write it ?) */
			ay8910_control_port_0_w(space, 0, vectrex_via_out[PORTA]);
		else
			ay8910_write_port_0_w(space, 0, vectrex_via_out[PORTA]);
	}

	if (!(data & 0x1) && (vectrex_via_out[PORTB] & 0x1))
		vectrex_multiplexer ((data >> 1) & 0x3);

	vectrex_via_out[PORTB] = data;
	timer_set(ATTOTIME_IN_NSEC(ANALOG_DELAY), &ramp, data & 0x80, update_signal);
}


static WRITE8_HANDLER(v_via_pa_w)
{
	/* DAC output always goes to Y integrator */
	vectrex_via_out[PORTA] = data;
	timer_set(ATTOTIME_IN_NSEC(ANALOG_DELAY), &analog[A_Y], data, update_signal);

	if (!(vectrex_via_out[PORTB] & 0x1))
		vectrex_multiplexer ((vectrex_via_out[PORTB] >> 1) & 0x3);
}


static WRITE8_HANDLER(v_via_ca2_w)
{
	if (data == 0)
		timer_set(ATTOTIME_IN_NSEC(ANALOG_DELAY), NULL, 0, vectrex_zero_integrators);
}


static WRITE8_HANDLER(v_via_cb2_w)
{
	int dx, dy;
	static UINT8 cb2 = 0;

	if (cb2 != data)
	{

		/* Check lightpen */
		if (vectrex_lightpen_port != 0)
		{
			lightpen_down = input_port_read(space->machine, "LPENCONF") & 0x10;
			
			if (lightpen_down)
			{
				pen_x = input_port_read(space->machine, "LPENX") * (x_max / 0xff);
				pen_y = input_port_read(space->machine, "LPENY") * (y_max / 0xff);
				
				dx = abs(pen_x - x_int);
				dy = abs(pen_y - y_int);
				if (dx < 500000 && dy < 500000 && data > 0)
					timer_set(attotime_zero, NULL, 0, lightpen_trigger);
			}
		}

		timer_set(attotime_zero, &blank, data, update_signal);
		cb2 = data;
	}
}


/*****************************************************************

   RA+A Spectrum I+

*****************************************************************/

static const struct via6522_interface spectrum1_via6522_interface =
{
	/*inputs : A/B,CA/B1,CA/B2 */ v_via_pa_r, s1_via_pb_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B1,CA/B2 */ v_via_pa_w, v_via_pb_w, 0, 0, v_via_ca2_w, v_via_cb2_w,
	/*irq                      */ v_via_irq,
};


WRITE8_HANDLER(raaspec_led_w)
{
	logerror("Spectrum I+ LED: %i%i%i%i%i%i%i%i\n",
			 (data>>7)&0x1, (data>>6)&0x1, (data>>5)&0x1, (data>>4)&0x1,
			 (data>>3)&0x1, (data>>2)&0x1, (data>>1)&0x1, data&0x1);
}


VIDEO_START(raaspec)
{
	const device_config *screen = video_screen_first(machine->config);
	const rectangle *visarea = video_screen_get_visible_area(screen);

	x_center=((visarea->max_x - visarea->min_x) / 2) << 16;
	y_center=((visarea->max_y - visarea->min_y) / 2) << 16;
	x_max = visarea->max_x << 16;
	y_max = visarea->max_y << 16;

	via_config(0, &spectrum1_via6522_interface);
	via_reset();
	refresh = timer_alloc(vectrex_refresh, NULL);

	VIDEO_START_CALL(vector);
}
