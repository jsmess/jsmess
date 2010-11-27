#include "emu.h"
#include "video/vector.h"
#include "machine/6522via.h"
#include "cpu/m6809/m6809.h"
#include "sound/ay8910.h"
#include "image.h"

#include "includes/vectrex.h"


#define VC_RED      MAKE_RGB(0xff, 0x00, 0x00)
#define VC_GREEN    MAKE_RGB(0x00, 0xff, 0x00)
#define VC_BLUE     MAKE_RGB(0x00, 0x00, 0xff)
#define VC_DARKRED  MAKE_RGB(0x80, 0x00, 0x00)

#define DAMPC (-0.2)
#define MMI (5.0)

enum {
	PORTB = 0,
	PORTA
};


/*********************************************************************

   Global variables

*********************************************************************/



/*********************************************************************

   Local variables

*********************************************************************/

/* Colors for right and left eye */

/* Starting points of the three colors */
/* Values taken from J. Nelson's drawings*/

static const double minestorm_3d_angles[3] = {0, 0.1692, 0.2086};
static const double narrow_escape_angles[3] = {0, 0.1631, 0.3305};
static const double crazy_coaster_angles[3] = {0, 0.1631, 0.3305};


static const double unknown_game_angles[3] = {0,0.16666666, 0.33333333};


static int vectrex_verify_cart(char *data)
{
	/* Verify the file is accepted by the Vectrex bios */
	if (!memcmp(data,"g GCE", 5))
		return IMAGE_VERIFY_PASS;
	else
		return IMAGE_VERIFY_FAIL;
}


/*********************************************************************

   ROM load and id functions

*********************************************************************/

DEVICE_IMAGE_LOAD(vectrex_cart)
{
	vectrex_state *state = image.device().machine->driver_data<vectrex_state>();
	UINT8 *mem = memory_region(image.device().machine, "maincpu");
	if (image.software_entry() == NULL)
	{
		image.fread( mem, 0x8000);
	} else {
		int size = image.get_software_region_length("rom");
		memcpy(mem, image.get_software_region("rom"), size);
	}

	/* check image! */
	if (vectrex_verify_cart((char*)mem) == IMAGE_VERIFY_FAIL)
	{
		logerror("Invalid image!\n");
		return IMAGE_INIT_FAIL;
	}

	/* If VIA T2 starts, reset refresh timer.
       This is the best strategy for most games. */
	state->reset_refresh = 1;

	state->imager_angles = narrow_escape_angles;

	/* let's do this 3D detection with a strcmp using data inside the cart images */
	/* slightly prettier than having to hardcode CRCs */

	/* handle 3D Narrow Escape but skip the 2-d hack of it from Fred Taft */
	if (!memcmp(mem + 0x11,"NARROW",6) && (((char*)mem)[0x39] == 0x0c))
	{
		state->imager_angles = narrow_escape_angles;
	}

	if (!memcmp(mem + 0x11,"CRAZY COASTER", 13))
	{
		state->imager_angles = crazy_coaster_angles;
	}

	if (!memcmp(mem + 0x11,"3D MINE STORM", 13))
	{
		state->imager_angles = minestorm_3d_angles;

		/* Don't reset T2 each time it's written.
           This would cause jerking in mine3. */
		state->reset_refresh = 0;
	}

	return IMAGE_INIT_PASS;
}


/*********************************************************************

   Vectrex configuration (mainly 3D Imager)

*********************************************************************/

void vectrex_configuration(running_machine *machine)
{
	vectrex_state *state = machine->driver_data<vectrex_state>();
	unsigned char cport = input_port_read(machine, "3DCONF");

	/* Vectrex 'dipswitch' configuration */

	/* Imager control */
	if (cport & 0x01) /* Imager enabled */
	{
		if (state->imager_status == 0)
			state->imager_status = cport & 0x01;

		vector_add_point_function = cport & 0x02 ? vectrex_add_point_stereo: vectrex_add_point;

		switch ((cport >> 2) & 0x07)
		{
		case 0x00:
			state->imager_colors[0] = state->imager_colors[1] = state->imager_colors[2] = RGB_BLACK;
			break;
		case 0x01:
			state->imager_colors[0] = state->imager_colors[1] = state->imager_colors[2] = VC_DARKRED;
			break;
		case 0x02:
			state->imager_colors[0] = state->imager_colors[1] = state->imager_colors[2] = VC_GREEN;
			break;
		case 0x03:
			state->imager_colors[0] = state->imager_colors[1] = state->imager_colors[2] = VC_BLUE;
			break;
		case 0x04:
			/* mine3 has a different color sequence */
			if (state->imager_angles == minestorm_3d_angles)
			{
				state->imager_colors[0] = VC_GREEN;
				state->imager_colors[1] = VC_RED;
			}
			else
			{
				state->imager_colors[0] = VC_RED;
				state->imager_colors[1] = VC_GREEN;
			}
			state->imager_colors[2]=VC_BLUE;
			break;
		}

		switch ((cport >> 5) & 0x07)
		{
		case 0x00:
			state->imager_colors[3] = state->imager_colors[4] = state->imager_colors[5] = RGB_BLACK;
			break;
		case 0x01:
			state->imager_colors[3] = state->imager_colors[4] = state->imager_colors[5] = VC_DARKRED;
			break;
		case 0x02:
			state->imager_colors[3] = state->imager_colors[4] = state->imager_colors[5] = VC_GREEN;
			break;
		case 0x03:
			state->imager_colors[3] = state->imager_colors[4] = state->imager_colors[5] = VC_BLUE;
			break;
		case 0x04:
			if (state->imager_angles == minestorm_3d_angles)
			{
				state->imager_colors[3] = VC_GREEN;
				state->imager_colors[4] = VC_RED;
			}
			else
			{
				state->imager_colors[3] = VC_RED;
				state->imager_colors[4] = VC_GREEN;
			}
			state->imager_colors[5]=VC_BLUE;
			break;
		}
	}
	else
	{
		vector_add_point_function = vectrex_add_point;
		state->beam_color = RGB_WHITE;
		state->imager_colors[0] = state->imager_colors[1] = state->imager_colors[2] = state->imager_colors[3] = state->imager_colors[4] = state->imager_colors[5] = RGB_WHITE;
	}
	state->lightpen_port = input_port_read(machine, "LPENCONF") & 0x03;
}


/*********************************************************************

   VIA interface functions

*********************************************************************/

void vectrex_via_irq(running_device *device, int level)
{
	cputag_set_input_line(device->machine, "maincpu", M6809_IRQ_LINE, level);
}


READ8_DEVICE_HANDLER(vectrex_via_pb_r)
{
	vectrex_state *state = device->machine->driver_data<vectrex_state>();
	int pot;
	static const char *const ctrlnames[] = { "CONTR1X", "CONTR1Y", "CONTR2X", "CONTR2Y" };

	pot = input_port_read(device->machine, ctrlnames[(state->via_out[PORTB] & 0x6) >> 1]) - 0x80;

	if (pot > (signed char)state->via_out[PORTA])
		state->via_out[PORTB] |= 0x20;
	else
		state->via_out[PORTB] &= ~0x20;

	return state->via_out[PORTB];
}


READ8_DEVICE_HANDLER(vectrex_via_pa_r)
{
	vectrex_state *state = device->machine->driver_data<vectrex_state>();
	if ((!(state->via_out[PORTB] & 0x10)) && (state->via_out[PORTB] & 0x08))
		/* BDIR inactive, we can read the PSG. BC1 has to be active. */
	{
		running_device *ay = device->machine->device("ay8912");

		state->via_out[PORTA] = ay8910_r(ay, 0)
			& ~(state->imager_pinlevel & 0x80);
	}
	return state->via_out[PORTA];
}


READ8_DEVICE_HANDLER(vectrex_s1_via_pb_r)
{
	vectrex_state *state = device->machine->driver_data<vectrex_state>();
	return (state->via_out[PORTB] & ~0x40) | (input_port_read(device->machine, "COIN") & 0x40);
}


/*********************************************************************

   3D Imager support

*********************************************************************/

static TIMER_CALLBACK(vectrex_imager_change_color)
{
	vectrex_state *state = machine->driver_data<vectrex_state>();
	state->beam_color = param;
}


static TIMER_CALLBACK(update_level)
{
	if (ptr)
		* (UINT8 *) ptr = param;
}


TIMER_CALLBACK(vectrex_imager_eye)
{
	vectrex_state *state = machine->driver_data<vectrex_state>();
	via6522_device *via_0 = machine->device<via6522_device>("via6522_0");
	int coffset;
	double rtime = (1.0 / state->imager_freq);

	if (state->imager_status > 0)
	{
		state->imager_status = param;
		coffset = param > 1? 3: 0;
		timer_set (machine, double_to_attotime(rtime * state->imager_angles[0]), NULL, state->imager_colors[coffset+2], vectrex_imager_change_color);
		timer_set (machine, double_to_attotime(rtime * state->imager_angles[1]), NULL, state->imager_colors[coffset+1], vectrex_imager_change_color);
		timer_set (machine, double_to_attotime(rtime * state->imager_angles[2]), NULL, state->imager_colors[coffset], vectrex_imager_change_color);

		if (param == 2)
		{
			timer_set (machine, double_to_attotime(rtime * 0.50), NULL, 1, vectrex_imager_eye);

			/* Index hole sensor is connected to IO7 which triggers also CA1 of VIA */
			via_0->write_ca1(1);
			via_0->write_ca1(0);
			state->imager_pinlevel |= 0x80;
			timer_set (machine, double_to_attotime(rtime / 360.0), &state->imager_pinlevel, 0, update_level);
		}
	}
}


WRITE8_HANDLER(vectrex_psg_port_w)
{
	vectrex_state *state = space->machine->driver_data<vectrex_state>();
	double wavel, ang_acc, tmp;
	int mcontrol;

	mcontrol = data & 0x40; /* IO6 controls the imager motor */

	if (!mcontrol && mcontrol ^ state->old_mcontrol)
	{
		state->old_mcontrol = mcontrol;
		tmp = attotime_to_double(timer_get_time(space->machine));
		wavel = tmp - state->sl;
		state->sl = tmp;

		if (wavel < 1)
		{
			/* The Vectrex sends a stream of pulses which control the speed of
               the motor using Pulse Width Modulation. Guessed parameters are MMI
               (mass moment of inertia) of the color wheel, DAMPC (damping coefficient)
               of the whole thing and some constants of the motor's torque/speed curve.
               pwl is the negative pulse width and wavel is the whole wavelength. */

			ang_acc = (50.0 - 1.55 * state->imager_freq) / MMI;
			state->imager_freq += ang_acc * state->pwl + DAMPC * state->imager_freq / MMI * wavel;

			if (state->imager_freq > 1)
			{
				timer_adjust_periodic(state->imager_timer,
									  double_to_attotime(MIN(1.0 / state->imager_freq, attotime_to_double(timer_timeleft(state->imager_timer)))),
									  2,
									  double_to_attotime(1.0 / state->imager_freq));
			}
		}
	}
	if (mcontrol && mcontrol ^ state->old_mcontrol)
	{
		state->old_mcontrol = mcontrol;
		state->pwl = attotime_to_double(timer_get_time(space->machine)) - state->sl;
	}
}

DRIVER_INIT(vectrex)
{
	vectrex_state *state = machine->driver_data<vectrex_state>();
	int i;

	state->imager_angles = unknown_game_angles;
	state->beam_color = RGB_WHITE;
	for (i=0; i<ARRAY_LENGTH(state->imager_colors); i++)
		state->imager_colors[i] = RGB_WHITE;

	/*
     * Uninitialized RAM needs to return 0xff. Otherwise the mines in
     * the first level of Minestorm are not evenly distributed.
     */

	memset(state->gce_vectorram, 0xff, state->gce_vectorram_size);
}
