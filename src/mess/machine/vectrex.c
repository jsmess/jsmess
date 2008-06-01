#include "driver.h"
#include "deprecat.h"
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

#define PORTB 0
#define PORTA 1


/*********************************************************************

   Global variables

*********************************************************************/

unsigned char vectrex_via_out[2];
rgb_t vectrex_beam_color = RGB_WHITE;      /* the color of the vectrex beam */
int vectrex_imager_status = 0;     /* 0 = off, 1 = right eye, 2 = left eye */
double imager_freq;
emu_timer *imager_timer;
int vectrex_lightpen_port=0;
int vectrex_reset_refresh;


/*********************************************************************

   Local variables

*********************************************************************/

/* Colors for right and left eye */
static rgb_t imager_colors[6] =
{
	RGB_WHITE, RGB_WHITE, RGB_WHITE, RGB_WHITE, RGB_WHITE, RGB_WHITE
};

/* Starting points of the three colors */
/* Values taken from J. Nelson's drawings*/

static const double minestorm_3d_angles[3] = {0, 0.1692, 0.2086};
static const double narrow_escape_angles[3] = {0, 0.1631, 0.3305};
static const double crazy_coaster_angles[3] = {0, 0.1631, 0.3305};


static const double unknown_game_angles[3] = {0,0.16666666, 0.33333333};
static const double *vectrex_imager_angles = unknown_game_angles;
static unsigned char vectrex_imager_pinlevel;

static int vectrex_verify_cart (char *data)
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

DEVICE_IMAGE_LOAD( vectrex_cart )
{
	image_fread(image, memory_region(REGION_CPU1), 0x8000);

	/* check image! */
	if (vectrex_verify_cart((char*)memory_region(REGION_CPU1)) == IMAGE_VERIFY_FAIL)
	{
		logerror("Invalid image!\n");
		return INIT_FAIL;
	}

	/* If VIA T2 starts, reset refresh timer.
	   This is the best strategy for most games. */
	vectrex_reset_refresh = 1;

	vectrex_imager_angles = narrow_escape_angles;

	/* let's do this 3D detection with a strcmp using data inside the cart images */
	/* slightly prettier than having to hardcode CRCs */

	/* handle 3D Narrow Escape but skip the 2-d hack of it from Fred Taft */
	if (!memcmp(memory_region(REGION_CPU1)+0x11,"NARROW",6) && (((char*)memory_region(REGION_CPU1))[0x39] == 0x0c))
	{
		vectrex_imager_angles = narrow_escape_angles;
	}

	if (!memcmp(memory_region(REGION_CPU1)+0x11,"CRAZY COASTER",13))
	{
		vectrex_imager_angles = crazy_coaster_angles;
	}

	if (!memcmp(memory_region(REGION_CPU1)+0x11,"3D MINE STORM",13))
	{
		vectrex_imager_angles = minestorm_3d_angles;

		/* Don't reset T2 each time it's written. 
		   This would cause jerking in mine3. */
		vectrex_reset_refresh = 0;
	}

	return INIT_PASS;
}


/*********************************************************************

   Vectrex configuration (mainly 3D Imager)

*********************************************************************/

void vectrex_configuration(running_machine *machine)
{
	unsigned char cport = input_port_read(machine, "3DCONF");

	/* Vectrex 'dipswitch' configuration */

	/* Imager control */
	if (cport & 0x01) /* Imager enabled */
	{
		if (vectrex_imager_status == 0)
			vectrex_imager_status = cport & 0x01;

		vector_add_point_function = cport & 0x02 ? vectrex_add_point_stereo: vectrex_add_point;

		switch ((cport>>2) & 0x07)
		{
		case 0x00:
			imager_colors[0]=imager_colors[1]=imager_colors[2]=RGB_BLACK;
			break;
		case 0x01:
			imager_colors[0]=imager_colors[1]=imager_colors[2]=VC_DARKRED;
			break;
		case 0x02:
			imager_colors[0]=imager_colors[1]=imager_colors[2]=VC_GREEN;
			break;
		case 0x03:
			imager_colors[0]=imager_colors[1]=imager_colors[2]=VC_BLUE;
			break;
		case 0x04:
			/* mine3 has a different color sequence */
			if (vectrex_imager_angles == minestorm_3d_angles)
			{
				imager_colors[0]=VC_GREEN;
				imager_colors[1]=VC_RED;
			}
			else
			{
				imager_colors[0]=VC_RED;
				imager_colors[1]=VC_GREEN;
			}
			imager_colors[2]=VC_BLUE;
			break;
		}

		switch ((cport>>5) & 0x07)
		{
		case 0x00:
			imager_colors[3]=imager_colors[4]=imager_colors[5]=RGB_BLACK;
			break;
		case 0x01:
			imager_colors[3]=imager_colors[4]=imager_colors[5]=VC_DARKRED;
			break;
		case 0x02:
			imager_colors[3]=imager_colors[4]=imager_colors[5]=VC_GREEN;
			break;
		case 0x03:
			imager_colors[3]=imager_colors[4]=imager_colors[5]=VC_BLUE;
			break;
		case 0x04:
			if (vectrex_imager_angles == minestorm_3d_angles)
			{
				imager_colors[3]=VC_GREEN;
				imager_colors[4]=VC_RED;
			}
			else
			{
				imager_colors[3]=VC_RED;
				imager_colors[4]=VC_GREEN;
			}
			imager_colors[5]=VC_BLUE;
			break;
		}
	}
	else
	{
		vector_add_point_function = vectrex_add_point;
		vectrex_beam_color = RGB_WHITE;
		imager_colors[0]=imager_colors[1]=imager_colors[2]=imager_colors[3]=imager_colors[4]=imager_colors[5]=RGB_WHITE;
	}
	vectrex_lightpen_port = input_port_read(machine, "LPENCONF") & 0x03;
}

/*********************************************************************

   VIA interface functions

*********************************************************************/

void v_via_irq (running_machine *machine, int level)
{
	cpunum_set_input_line(machine, 0, M6809_IRQ_LINE, level);
}

READ8_HANDLER( v_via_pb_r )
{
	int pot;
	char port[8];

	switch (((vectrex_via_out[PORTB] & 0x6) >> 1))
	{
		case 0:
			sprintf(port, "CONTR1X");
			break;
		case 1:
			sprintf(port, "CONTR1Y");
			break;
		case 2:
			sprintf(port, "CONTR2X");
			break;
		case 3:
			sprintf(port, "CONTR2Y");
			break;
	}
	
	pot = input_port_read(machine, port) - 0x80; 
	
	if (pot > (signed char)vectrex_via_out[PORTA])
		vectrex_via_out[PORTB] |= 0x20;
	else
		vectrex_via_out[PORTB] &= ~0x20;

	return vectrex_via_out[PORTB];
}

READ8_HANDLER( v_via_pa_r )
{
	if ((!(vectrex_via_out[PORTB] & 0x10)) && (vectrex_via_out[PORTB] & 0x08))
		/* BDIR inactive, we can read the PSG. BC1 has to be active. */
	{
		vectrex_via_out[PORTA] = AY8910_read_port_0_r(machine, 0)
			& ~(vectrex_imager_pinlevel & 0x80);
	}
	return vectrex_via_out[PORTA];
}

READ8_HANDLER( s1_via_pb_r )
{
	return (vectrex_via_out[PORTB] & ~0x40) | (input_port_read(machine, "COIN") & 0x40);
}


/*********************************************************************

   3D Imager support

*********************************************************************/

static TIMER_CALLBACK(vectrex_imager_change_color)
{
	vectrex_beam_color = param;
}

static TIMER_CALLBACK(update_level)
{
	if (ptr)
		* (UINT8 *) ptr = param;
}

TIMER_CALLBACK(vectrex_imager_eye)
{
	int coffset;
	double rtime = (1.0 / imager_freq);

	if (vectrex_imager_status > 0)
	{
		vectrex_imager_status = param;
		coffset = param > 1? 3: 0;
		timer_set (double_to_attotime(rtime * vectrex_imager_angles[0]), NULL, imager_colors[coffset+2], vectrex_imager_change_color);
		timer_set (double_to_attotime(rtime * vectrex_imager_angles[1]), NULL, imager_colors[coffset+1], vectrex_imager_change_color);
		timer_set (double_to_attotime(rtime * vectrex_imager_angles[2]), NULL, imager_colors[coffset], vectrex_imager_change_color);

		if (param == 2)
		{
			timer_set (double_to_attotime(rtime * 0.50), NULL, 1, vectrex_imager_eye);

			/* Index hole sensor is connected to IO7 which triggers also CA1 of VIA */
			via_set_input_ca1(0, 1);
			via_set_input_ca1(0, 0);
			vectrex_imager_pinlevel |= 0x80;
			timer_set (double_to_attotime(rtime / 360.0), &vectrex_imager_pinlevel, 0, update_level);
		}
	}
}

#define DAMPC (-0.2)
#define MMI (5.0)

WRITE8_HANDLER ( vectrex_psg_port_w )
{
	static int state;
	static double sl, pwl;
	double wavel, ang_acc, tmp;
	int mcontrol;

	mcontrol = data & 0x40; /* IO6 controls the imager motor */

	if (!mcontrol && mcontrol ^ state)
	{
		state = mcontrol;
		tmp = attotime_to_double(timer_get_time());
		wavel = tmp - sl;
		sl = tmp;

		if (wavel < 1)
		{
			/* The Vectrex sends a stream of pulses which control the speed of
			   the motor using Pulse Width Modulation. Guessed parameters are MMI
			   (mass moment of inertia) of the color wheel, DAMPC (damping coefficient)
			   of the whole thing and some constants of the motor's torque/speed curve.
			   pwl is the negative pulse width and wavel is the whole wavelength. */

			ang_acc = (50.0 - 1.55 * imager_freq) / MMI;
			imager_freq += ang_acc * pwl + DAMPC * imager_freq / MMI * wavel;

			if (imager_freq > 1)
			{
				timer_adjust_periodic(imager_timer,
									  double_to_attotime(MIN(1.0 / imager_freq, attotime_to_double(timer_timeleft(imager_timer)))),
									  2,
									  double_to_attotime(1.0 / imager_freq));
			}
		}
	}
	if (mcontrol && mcontrol ^ state)
	{
		state = mcontrol;
		pwl = attotime_to_double(timer_get_time()) - sl;
	}
}

DRIVER_INIT( vectrex )
{
	/*
	 * Uninitialized RAM needs to return 0xff. Otherwise the mines in
	 * the first level of Minestorm are not evenly distributed.
	 */

	memset(vectorram, 0xff, vectorram_size);
}
