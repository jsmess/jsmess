#include <math.h>
#include "driver.h"
#include "video/vector.h"
#include "video/generic.h"
#include "machine/6522via.h"
#include "cpu/m6809/m6809.h"
#include "sound/ay8910.h"
#include "sound/dac.h"
#include "mscommon.h"

#include "includes/vectrex.h"

#define RAMP_DELAY 6.333e-6
#define SH_DELAY RAMP_DELAY

#define VEC_SHIFT 16
#define INT_PER_CLOCK 600
#define VECTREX_CLOCK 1500000

#define PORTB 0
#define PORTA 1

#ifndef M_SQRT1_2
#define M_SQRT1_2 0.70710678118654752440
#endif

/*********************************************************************
  Local variables
 *********************************************************************/
static WRITE8_HANDLER ( v_via_pa_w );
static WRITE8_HANDLER( v_via_pb_w );
static WRITE8_HANDLER ( v_via_ca2_w );
static WRITE8_HANDLER ( v_via_cb2_w );

static struct via6522_interface vectrex_via6522_interface =
{
	v_via_pa_r, v_via_pb_r,         /* read PA/B */
	0, 0, 0, 0,                     /* read ca1, cb1, ca2, cb2 */
	v_via_pa_w, v_via_pb_w,         /* write PA/B */ 
	0, 0, v_via_ca2_w, v_via_cb2_w, /* write ca1, cb1, ca2, cb2 */
	v_via_irq,                      /* IRQ */
};

static int x_center, y_center, x_max, y_max;
static int x_int, y_int; /* X, Y integrators IC LF347*/
static int lightpen_down=0, pen_x, pen_y; /* Lightpen position */
static mame_timer *lp_t;
static int blank;

/* Analog signals: 0 : X-axis sample and hold IC LF347
 *                 coupled by the MUX CD4052
 *                 1 : Y-axis sample and hold IC LF347
 *                 2 : "0" reference charge capacitor
 *                 3 : Z-axis (brightness signal) sample and hold IC LF347
 *                 4 : MPU sound resistive netowrk
 */

#define ASIG_Y 0
#define ASIG_X 1
#define ASIG_ZR 2
#define ASIG_Z 3
#define ASIG_MPU 4

#define NVECT 10000

static int analog_sig[5];

static double start_time;
static int last_point_x, last_point_y, last_point_z, last_point=0;
static double last_point_starttime;
static float z_factor;
static int vectrex_point_index = 0;
static double vectrex_persistance = 0.05;

struct vectrex_point
{
	int x; int y;
	rgb_t col;
	int intensity;
	double time;
};

static struct vectrex_point vectrex_points[NVECT];
void (*vector_add_point_function) (int, int, rgb_t, int) = vectrex_add_point;

void vectrex_add_point (int x, int y, rgb_t color, int intensity)
{
	struct vectrex_point *newpoint;

	vectrex_point_index = (vectrex_point_index+1) % NVECT;
	newpoint = &vectrex_points[vectrex_point_index];

	newpoint->x = x;
	newpoint->y = y;
	newpoint->col = color;
	newpoint->intensity = intensity;
	newpoint->time = timer_get_time();
}

/*********************************************************************
  Lightpen
 *********************************************************************/
static void lightpen_trigger (int param)
{
	if (vectrex_lightpen_port & 1)
	{
		via_0_ca1_w (0, 1);
		via_0_ca1_w (0, 0);
	}
	if (vectrex_lightpen_port & 2)
	{
		cpunum_set_input_line(0, M6809_FIRQ_LINE, PULSE_LINE);
	}
}

static int lightpen_check (void)
{
	int dx,dy;
	if (lightpen_down)
	{
		dx=abs(pen_x-x_int);
		dy=abs(pen_y-y_int);
		if (dx<500000 && dy<500000)
			return 1;
	}
	return 0;
}

static void lightpen_show (void)
{
	int color;

	if (vectrex_lightpen_port != 0)
	{
		if (readinputport(6)&0x10)
		{
			lightpen_down=1;
			color=0x00ff0000;
		}
		else
		{
			lightpen_down=0;
			color=0x00ffffff;
		}
		
		pen_x = readinputport(8)*(x_max/0xff);
		pen_y = readinputport(7)*(y_max/0xff);

		vector_add_point(pen_x-250000,pen_y-250000,0,0xff);
		vector_add_point(pen_x+250000,pen_y+250000,color,0xff);
		vector_add_point(pen_x+250000,pen_y-250000,0,0xff);
		vector_add_point(pen_x-250000,pen_y+250000,color,0xff);
	}
	else
		lightpen_down=0;	
}

/*********************************************************************
  Screen updating
 *********************************************************************/
VIDEO_UPDATE( vectrex )
{
	int i, v;
	double starttime;

	vectrex_configuration();
	lightpen_show();

	starttime = timer_get_time() - vectrex_persistance;
	if (starttime < 0) starttime = 0;
	i = vectrex_point_index;

	/* Find the oldest vector we want to display */
	for (v=0; ((vectrex_points[i].time > starttime) && (v < NVECT)); v++)
	{
		i--;
		if (i<0) i = NVECT - 1;
	}

	/* start black */
	vector_add_point(vectrex_points[i].x, vectrex_points[i].y, vectrex_points[i].col, 0);

	while (i != vectrex_point_index)
	{
		vector_add_point(vectrex_points[i].x,vectrex_points[i].y,vectrex_points[i].col, vectrex_points[i].intensity);
		i = (i+1) % NVECT;
	}

	video_update_vector(machine, screen, bitmap, cliprect);
	vector_clear_list();
	return 0;
}

/*********************************************************************
  Vector functions
 *********************************************************************/
void vectrex_add_point_stereo (int x, int y, rgb_t color, int intensity)
{
	if (vectrex_imager_status == 2) /* left = 1, right = 2 */
		vectrex_add_point ((int)(y*M_SQRT1_2), (int)(((x_max-x)*M_SQRT1_2)+y_center), color, intensity);
	else
		vectrex_add_point ((int)(y*M_SQRT1_2), (int)((x_max-x)*M_SQRT1_2), color, intensity);
}

INLINE void vectrex_zero_integrators(void)
{
	if (last_point)
		vector_add_point_function (last_point_x, last_point_y, vectrex_beam_color,
		MIN((int)(last_point_z*((timer_get_time()-last_point_starttime)*3E4)),255));
	last_point = 0;

	x_int=x_center+(analog_sig[ASIG_ZR]*INT_PER_CLOCK);
	y_int=y_center+(analog_sig[ASIG_ZR]*INT_PER_CLOCK);
	vector_add_point_function (x_int, y_int, vectrex_beam_color, 0);
}

INLINE void vectrex_dot(void)
{
	if (!last_point)
	{
		last_point_x = x_int;
		last_point_y = y_int;
		last_point_z = analog_sig[ASIG_Z] > 0? (int)(analog_sig[ASIG_Z] * z_factor): 0;
		last_point_starttime = timer_get_time();
		last_point = 1;
	}
}

INLINE void vectrex_solid_line(double int_time, int blank)
{
	int z = analog_sig[ASIG_Z] > 0? (int)(analog_sig[ASIG_Z]*z_factor): 0;
    int length = (int)(VECTREX_CLOCK * INT_PER_CLOCK * int_time);

	/* The BIOS draws lines as follows: First put a pattern in the VIA SR (this causes a dot). Then
	 * turn on RAMP and let the integrators do their job (this causes a line to be drawn).
	 * Obviously this first dot isn't needed so we optimize it away :) but we have to take care
	 * not to optimize _all_ dots, that's why we do draw the dot if the following line is
	 * black (a move). */
	if (last_point && (!blank || !z))
		vector_add_point_function(last_point_x, last_point_y, vectrex_beam_color,
				  MIN((int)(last_point_z*((timer_get_time()-last_point_starttime)*3E4)),255));
	last_point = 0;

	x_int += (int)(length * (analog_sig[ASIG_X] - analog_sig[ASIG_ZR]));
	y_int += (int)(length * (analog_sig[ASIG_Y] + analog_sig[ASIG_ZR]));
	vector_add_point_function(x_int, y_int, vectrex_beam_color, z * blank);
}

/*********************************************************************
  Startup and stop
 *********************************************************************/
VIDEO_START( vectrex )
{
	int width, height;

	width = Machine->screen[0].width;
	height = Machine->screen[0].height;

	x_center=((Machine->screen[0].visarea.max_x
		  -Machine->screen[0].visarea.min_x) / 2) << VEC_SHIFT;
	y_center=((Machine->screen[0].visarea.max_y
		  -Machine->screen[0].visarea.min_y) / 2) << VEC_SHIFT;
	x_max = Machine->screen[0].visarea.max_x << VEC_SHIFT;
	y_max = Machine->screen[0].visarea.max_y << VEC_SHIFT;

	via_config(0, &vectrex_via6522_interface);
	via_reset();
	z_factor = 2;

	imager_freq = 1;

	imager_timer = timer_alloc(vectrex_imager_right_eye);
	if (!imager_timer)
		return 1;
	timer_adjust(imager_timer, 1.0/imager_freq, 2, 1.0/imager_freq);


	lp_t = mame_timer_alloc(lightpen_trigger);

	return video_start_vector(machine);
}


/*********************************************************************
  VIA interface functions
 *********************************************************************/
INLINE void vectrex_multiplexer (int mux)
{
	analog_sig[mux + 1]=(signed char)vectrex_via_out[PORTA];
	if (mux == 3)
		DAC_data_w(0,(signed char)vectrex_via_out[PORTA]+0x80);
}

static WRITE8_HANDLER ( v_via_pb_w )
{
	if (!(data & 0x80))
	{
		/* RAMP is active */
		if ((vectrex_via_out[PORTB] & 0x80))
		{
			/* RAMP was inactive before */
			start_time = timer_get_time()+RAMP_DELAY;

			if (lightpen_down)
			{
				/* Simple lin. algebra to check if pen is near
				 * the line defined by (ASIG_X,ASIG_Y).
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
				ab = (double)(pen_x-x_int)*analog_sig[ASIG_X]
					+(double)(pen_y-y_int)*analog_sig[ASIG_Y];
				if (ab>0)
				{
					a2 = (analog_sig[ASIG_X]*analog_sig[ASIG_X]
						  +analog_sig[ASIG_Y]*analog_sig[ASIG_Y]);
					b2 = (double)(pen_x-x_int)*(pen_x-x_int)
						+(double)(pen_y-y_int)*(pen_y-y_int);
					d2=b2-ab*ab/a2;
					if (d2<2e10 && analog_sig[ASIG_Z]*blank>0)
						timer_adjust(lp_t, ab/a2/(VECTREX_CLOCK*INT_PER_CLOCK),0,0);
				}
			}
		}

		if (!(data & 0x1) && (vectrex_via_out[PORTB] & 0x1))
			/* MUX has been enabled */
			/* This is a rare case used by some new games */
		{
			double time_now = timer_get_time()+SH_DELAY;
			vectrex_solid_line(time_now-start_time, blank);
			start_time = time_now;
		}
	}
	else
	{
		/* RAMP is inactive */
		if (!(vectrex_via_out[PORTB] & 0x80))
		{
			/* RAMP was active before - we can draw the line */
			vectrex_solid_line(timer_get_time()-start_time+RAMP_DELAY, blank);
			/* Cancel running timer, line already finished */
			if (lightpen_down)
				timer_adjust(lp_t,TIME_NEVER ,0,0);
		}
	}

	/* Sound */
	if (data & 0x10)
	{
		/* BDIR active, PSG latches */
		if (data & 0x08) /* BC1 (do we select a reg or write it ?) */
			AY8910_control_port_0_w (0, vectrex_via_out[PORTA]);
		else
			AY8910_write_port_0_w (0, vectrex_via_out[PORTA]);
	}

	if (!(data & 0x1) && (vectrex_via_out[PORTB] & 0x1))
		/* MUX has been enabled, so check with which signal the MUX
		 * coulpes the DAC output.  */
		vectrex_multiplexer ((data >> 1) & 0x3);

	vectrex_via_out[PORTB] = data;
}

static WRITE8_HANDLER ( v_via_pa_w )
{
	double time_now;

	if (!(vectrex_via_out[PORTB] & 0x80))  /* RAMP active (low) ? */
	{
		/* The game changes the sample and hold ICs (X/Y axis)
		 * during line draw (curved vectors)
		 * Draw the vector with the current settings
		 * before updating the signals.
		 */
		time_now = timer_get_time() + SH_DELAY;
		vectrex_solid_line(time_now - start_time, blank);
		start_time = time_now;
	}
	/* DAC output always goes into Y integrator */
	vectrex_via_out[PORTA] = analog_sig[ASIG_Y] = (signed char)data;

	if (!(vectrex_via_out[PORTB] & 0x1))
		/* MUX is enabled, so check with which signal the MUX
		 * coulpes the DAC output.  */
		vectrex_multiplexer ((vectrex_via_out[PORTB] >> 1) & 0x3);
}

static WRITE8_HANDLER ( v_via_ca2_w )
{
	if  (!data)    /* ~ZERO low ? Then zero integrators*/
		vectrex_zero_integrators();
}

static WRITE8_HANDLER ( v_via_cb2_w )
{
	double time_now;
	if (blank != data)
	{
		if (vectrex_via_out[PORTB] & 0x80)
		{
			/* RAMP inactive */
			/* This generates a dot (here we take the dwell time into account) */
			
			if (data)
				vectrex_dot();
		}
		else
		{
			/* RAMP active 
			 * Take MAX because RAMP is slower than BLANK
			 */
			time_now = MAX(timer_get_time(),start_time);
			vectrex_solid_line(time_now - start_time, blank);
			start_time = time_now;
		}
		if (data & lightpen_check())
			lightpen_trigger(0);

		blank = data;
	}
}

/*****************************************************************

  RA+A Spectrum I+

*****************************************************************/

static struct via6522_interface spectrum1_via6522_interface =
{
	/*inputs : A/B,CA/B1,CA/B2 */ v_via_pa_r, s1_via_pb_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B1,CA/B2 */ v_via_pa_w, v_via_pb_w, 0, 0, v_via_ca2_w, v_via_cb2_w,
	/*irq                      */ v_via_irq,
};

WRITE8_HANDLER( raaspec_led_w )
{
	logerror("Spectrum I+ LED: %i%i%i%i%i%i%i%i\n",
				 (data>>7)&0x1, (data>>6)&0x1, (data>>5)&0x1, (data>>4)&0x1,
				 (data>>3)&0x1, (data>>2)&0x1, (data>>1)&0x1, data&0x1);
}

VIDEO_START( raaspec )
{
	x_center=((Machine->screen[0].visarea.max_x
		  -Machine->screen[0].visarea.min_x)/2) << VEC_SHIFT;
	y_center=((Machine->screen[0].visarea.max_y
		  -Machine->screen[0].visarea.min_y)/2) << VEC_SHIFT;
	x_max = Machine->screen[0].visarea.max_x << VEC_SHIFT;
	y_max = Machine->screen[0].visarea.max_y << VEC_SHIFT;

	via_config(0, &spectrum1_via6522_interface);
	via_reset();
	z_factor = 2;

	raaspec_led_w (0, 0xff);
	return video_start_vector(machine);
}
