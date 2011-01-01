#include <math.h>
#include "emu.h"
#include "includes/vectrex.h"
#include "video/vector.h"
#include "machine/6522via.h"
#include "cpu/m6809/m6809.h"
#include "sound/ay8910.h"
#include "sound/dac.h"


#define ANALOG_DELAY 7800

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


/*********************************************************************

   Prototypes

*********************************************************************/

static WRITE8_DEVICE_HANDLER (v_via_pa_w);
static WRITE8_DEVICE_HANDLER(v_via_pb_w);
static WRITE8_DEVICE_HANDLER (v_via_ca2_w);
static WRITE8_DEVICE_HANDLER (v_via_cb2_w);


/*********************************************************************

   Local variables

*********************************************************************/

const via6522_interface vectrex_via6522_interface =
{
	DEVCB_HANDLER(vectrex_via_pa_r), DEVCB_HANDLER(vectrex_via_pb_r),         /* read PA/B */
	DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL,                     /* read ca1, cb1, ca2, cb2 */
	DEVCB_HANDLER(v_via_pa_w), DEVCB_HANDLER(v_via_pb_w),         /* write PA/B */
	DEVCB_NULL, DEVCB_NULL, DEVCB_HANDLER(v_via_ca2_w), DEVCB_HANDLER(v_via_cb2_w), /* write ca1, cb1, ca2, cb2 */
	DEVCB_LINE(vectrex_via_irq),                      /* IRQ */
};


typedef void (*vector_add_point_fn)(running_machine *, int, int, rgb_t, int);
vector_add_point_fn vector_add_point_function;


/*********************************************************************

   Lightpen

*********************************************************************/

static TIMER_CALLBACK(lightpen_trigger)
{
	vectrex_state *state = machine->driver_data<vectrex_state>();
	if (state->lightpen_port & 1)
	{
		via6522_device *via_0 = machine->device<via6522_device>("via6522_0");
		via_0->write_ca1(1);
		via_0->write_ca1(0);
	}

	if (state->lightpen_port & 2)
	{
		cputag_set_input_line(machine, "maincpu", M6809_FIRQ_LINE, PULSE_LINE);
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

READ8_HANDLER(vectrex_via_r)
{
	via6522_device *via = space->machine->device<via6522_device>("via6522_0");
	return via->read(*space, offset);
}

WRITE8_HANDLER(vectrex_via_w)
{
	vectrex_state *state = space->machine->driver_data<vectrex_state>();
	via6522_device *via = space->machine->device<via6522_device>("via6522_0");
	attotime period;

	switch (offset)
	{
	case 8:
		state->via_timer2 = (state->via_timer2 & 0xff00) | data;
		break;

	case 9:
		state->via_timer2 = (state->via_timer2 & 0x00ff) | (data << 8);

		period = attotime_mul(ATTOTIME_IN_HZ(cputag_get_clock(space->machine, "maincpu")), state->via_timer2);

		if (state->reset_refresh)
			timer_adjust_periodic(state->refresh, period, 0, period);
		else
			timer_adjust_periodic(state->refresh,
								  attotime_min(period, timer_timeleft(state->refresh)),
								  0,
								  period);
		break;
	}
	via->write(*space, offset, data);
}


/*********************************************************************

   Screen refresh

*********************************************************************/

static TIMER_CALLBACK(vectrex_refresh)
{
	vectrex_state *state = machine->driver_data<vectrex_state>();
	/* Refresh only marks the range of vectors which will be drawn
     * during the next VIDEO_UPDATE. */
	state->display_start = state->display_end;
	state->display_end = state->point_index;
}


VIDEO_UPDATE(vectrex)
{
	vectrex_state *state = screen->machine->driver_data<vectrex_state>();
	int i;

	vectrex_configuration(screen->machine);

	/* start black */
	vector_add_point(screen->machine,
					 state->points[state->display_start].x,
					 state->points[state->display_start].y,
					 state->points[state->display_start].col,
					 0);

	for (i = state->display_start; i != state->display_end; i = (i + 1) % NVECT)
	{
		vector_add_point(screen->machine,
						 state->points[i].x,
						 state->points[i].y,
						 state->points[i].col,
						 state->points[i].intensity);
	}

	VIDEO_UPDATE_CALL(vector);
	vector_clear_list();
	return 0;
}


/*********************************************************************

   Vector functions

*********************************************************************/

void vectrex_add_point(running_machine *machine, int x, int y, rgb_t color, int intensity)
{
	vectrex_state *state = machine->driver_data<vectrex_state>();
	vectrex_point *newpoint;

	state->point_index = (state->point_index+1) % NVECT;
	newpoint = &state->points[state->point_index];

	newpoint->x = x;
	newpoint->y = y;
	newpoint->col = color;
	newpoint->intensity = intensity;
}


void vectrex_add_point_stereo(running_machine *machine, int x, int y, rgb_t color, int intensity)
{
	vectrex_state *state = machine->driver_data<vectrex_state>();
	if (state->imager_status == 2) /* left = 1, right = 2 */
		vectrex_add_point(machine, (int)(y * M_SQRT1_2)+ state->x_center,
						   (int)(((state->x_max - x) * M_SQRT1_2)),
						   color,
						   intensity);
	else
		vectrex_add_point(machine, (int)(y * M_SQRT1_2),
						   (int)((state->x_max - x) * M_SQRT1_2),
						   color,
						   intensity);
}


static TIMER_CALLBACK(vectrex_zero_integrators)
{
	vectrex_state *state = machine->driver_data<vectrex_state>();
	state->x_int = state->x_center + (state->analog[A_ZR] * INT_PER_CLOCK);
	state->y_int = state->y_center + (state->analog[A_ZR] * INT_PER_CLOCK);
	vector_add_point_function(machine, state->x_int, state->y_int, state->beam_color, 0);
}


/*********************************************************************

   Delayed signals

   The RAMP signal is delayed wrt. beam blanking. Getting this right
   is important for text placement, the maze in Clean Sweep and many
   other games.

*********************************************************************/

static TIMER_CALLBACK(update_signal)
{
	vectrex_state *state = machine->driver_data<vectrex_state>();
	int length;

	if (!state->ramp)
	{
		length = cputag_get_clock(machine, "maincpu") * INT_PER_CLOCK
			* attotime_to_double(attotime_sub(timer_get_time(machine), state->vector_start_time));

		state->x_int += length * (state->analog[A_X] + state->analog[A_ZR]);
		state->y_int += length * (state->analog[A_Y] + state->analog[A_ZR]);

		vector_add_point_function(machine, state->x_int, state->y_int, state->beam_color, 2 * state->analog[A_Z] * state->blank);
	}
	else
	{
		if (state->blank)
			vector_add_point_function(machine, state->x_int, state->y_int, state->beam_color, 2 * state->analog[A_Z]);
	}

	state->vector_start_time = timer_get_time(machine);

	if (ptr)
		* (UINT8 *) ptr = param;
}


/*********************************************************************

   Startup

*********************************************************************/

VIDEO_START(vectrex)
{
	vectrex_state *state = machine->driver_data<vectrex_state>();
	screen_device *screen = machine->first_screen();
	const rectangle &visarea = screen->visible_area();

	state->x_center=((visarea.max_x - visarea.min_x) / 2) << 16;
	state->y_center=((visarea.max_y - visarea.min_y) / 2) << 16;
	state->x_max = visarea.max_x << 16;
	state->y_max = visarea.max_y << 16;

	state->imager_freq = 1;

	vector_add_point_function = vectrex_add_point;
	state->imager_timer = timer_alloc(machine, vectrex_imager_eye, NULL);
	timer_adjust_periodic(state->imager_timer,
						  ATTOTIME_IN_HZ(state->imager_freq),
						  2,
						  ATTOTIME_IN_HZ(state->imager_freq));

	state->lp_t = timer_alloc(machine, lightpen_trigger, NULL);

	state->refresh = timer_alloc(machine, vectrex_refresh, NULL);

	VIDEO_START_CALL(vector);
}


/*********************************************************************

   VIA interface functions

*********************************************************************/

static void vectrex_multiplexer(running_machine *machine, int mux)
{
	vectrex_state *state = machine->driver_data<vectrex_state>();
	device_t *dac_device = machine->device("dac");

	timer_set(machine, ATTOTIME_IN_NSEC(ANALOG_DELAY), &state->analog[mux], state->via_out[PORTA], update_signal);

	if (mux == A_AUDIO)
		dac_data_w(dac_device, state->via_out[PORTA]);
}


static WRITE8_DEVICE_HANDLER(v_via_pb_w)
{
	vectrex_state *state = device->machine->driver_data<vectrex_state>();
	if (!(data & 0x80))
	{
		/* RAMP is active */
		if ((state->ramp & 0x80))
		{
			/* RAMP was inactive before */

			if (state->lightpen_down)
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
				ab = (state->pen_x - state->x_int) * state->analog[A_X]
					+(state->pen_y - state->y_int) * state->analog[A_Y];
				if (ab > 0)
				{
					a2 = (double)(state->analog[A_X] * state->analog[A_X]
								  +(double)state->analog[A_Y] * state->analog[A_Y]);
					b2 = (double)(state->pen_x - state->x_int) * (state->pen_x - state->x_int)
						+(double)(state->pen_y - state->y_int) * (state->pen_y - state->y_int);
					d2 = b2 - ab * ab / a2;
					if (d2 < 2e10 && state->analog[A_Z] * state->blank > 0)
						timer_adjust_oneshot(state->lp_t, double_to_attotime(ab / a2 / (cputag_get_clock(device->machine, "maincpu") * INT_PER_CLOCK)), 0);
				}
			}
		}

		if (!(data & 0x1) && (state->via_out[PORTB] & 0x1))
		{
			/* MUX has been enabled */
			timer_set(device->machine, ATTOTIME_IN_NSEC(ANALOG_DELAY), NULL, 0, update_signal);
		}
	}
	else
	{
		/* RAMP is inactive */
		if (!(state->ramp & 0x80))
		{
			/* Cancel running timer, line already finished */
			if (state->lightpen_down)
				timer_adjust_oneshot(state->lp_t, attotime_never, 0);
		}
	}

	/* Sound */
	if (data & 0x10)
	{
		device_t *ay8912 = device->machine->device("ay8912");

		if (data & 0x08) /* BC1 (do we select a reg or write it ?) */
			ay8910_address_w(ay8912, 0, state->via_out[PORTA]);
		else
			ay8910_data_w(ay8912, 0, state->via_out[PORTA]);
	}

	if (!(data & 0x1) && (state->via_out[PORTB] & 0x1))
		vectrex_multiplexer (device->machine, (data >> 1) & 0x3);

	state->via_out[PORTB] = data;
	timer_set(device->machine, ATTOTIME_IN_NSEC(ANALOG_DELAY), &state->ramp, data & 0x80, update_signal);
}


static WRITE8_DEVICE_HANDLER(v_via_pa_w)
{
	vectrex_state *state = device->machine->driver_data<vectrex_state>();
	/* DAC output always goes to Y integrator */
	state->via_out[PORTA] = data;
	timer_set(device->machine, ATTOTIME_IN_NSEC(ANALOG_DELAY), &state->analog[A_Y], data, update_signal);

	if (!(state->via_out[PORTB] & 0x1))
		vectrex_multiplexer (device->machine, (state->via_out[PORTB] >> 1) & 0x3);
}


static WRITE8_DEVICE_HANDLER(v_via_ca2_w)
{
	if (data == 0)
		timer_set(device->machine, ATTOTIME_IN_NSEC(ANALOG_DELAY), NULL, 0, vectrex_zero_integrators);
}


static WRITE8_DEVICE_HANDLER(v_via_cb2_w)
{
	vectrex_state *state = device->machine->driver_data<vectrex_state>();
	int dx, dy;

	if (state->cb2 != data)
	{

		/* Check lightpen */
		if (state->lightpen_port != 0)
		{
			state->lightpen_down = input_port_read(device->machine, "LPENCONF") & 0x10;

			if (state->lightpen_down)
			{
				state->pen_x = input_port_read(device->machine, "LPENX") * (state->x_max / 0xff);
				state->pen_y = input_port_read(device->machine, "LPENY") * (state->y_max / 0xff);

				dx = abs(state->pen_x - state->x_int);
				dy = abs(state->pen_y - state->y_int);
				if (dx < 500000 && dy < 500000 && data > 0)
					timer_set(device->machine, attotime_zero, NULL, 0, lightpen_trigger);
			}
		}

		timer_set(device->machine, attotime_zero, &state->blank, data, update_signal);
		state->cb2 = data;
	}
}


/*****************************************************************

   RA+A Spectrum I+

*****************************************************************/

const via6522_interface spectrum1_via6522_interface =
{
	/*inputs : A/B,CA/B1,CA/B2 */ DEVCB_HANDLER(vectrex_via_pa_r), DEVCB_HANDLER(vectrex_s1_via_pb_r), DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL,
	/*outputs: A/B,CA/B1,CA/B2 */ DEVCB_HANDLER(v_via_pa_w), DEVCB_HANDLER(v_via_pb_w), DEVCB_NULL, DEVCB_NULL, DEVCB_HANDLER(v_via_ca2_w), DEVCB_HANDLER(v_via_cb2_w),
	/*irq                      */ DEVCB_LINE(vectrex_via_irq),
};


WRITE8_HANDLER(raaspec_led_w)
{
	logerror("Spectrum I+ LED: %i%i%i%i%i%i%i%i\n",
			 (data>>7)&0x1, (data>>6)&0x1, (data>>5)&0x1, (data>>4)&0x1,
			 (data>>3)&0x1, (data>>2)&0x1, (data>>1)&0x1, data&0x1);
}


VIDEO_START(raaspec)
{
	vectrex_state *state = machine->driver_data<vectrex_state>();
	screen_device *screen = machine->first_screen();
	const rectangle &visarea = screen->visible_area();

	state->x_center=((visarea.max_x - visarea.min_x) / 2) << 16;
	state->y_center=((visarea.max_y - visarea.min_y) / 2) << 16;
	state->x_max = visarea.max_x << 16;
	state->y_max = visarea.max_y << 16;

	vector_add_point_function = vectrex_add_point;
	state->refresh = timer_alloc(machine, vectrex_refresh, NULL);

	VIDEO_START_CALL(vector);
}
