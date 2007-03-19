#include "driver.h"
#include "video/vector.h"
#include "machine/6522via.h"
#include "cpu/m6809/m6809.h"
#include "sound/ay8910.h"
#include "image.h"

#include "includes/vectrex.h"

#define VC_BLACK 0x00000000
#define VC_RED   0x00ff0000
#define VC_GREEN 0x0000ff00
#define VC_BLUE  0x000000ff
#define VC_WHITE VC_RED|VC_GREEN|VC_BLUE
#define VC_DARKRED 0x00800000

#define PORTB 0
#define PORTA 1

/*********************************************************************
  Global variables
 *********************************************************************/
unsigned char vectrex_via_out[2];
UINT32 vectrex_beam_color = VC_WHITE;	   /* the color of the vectrex beam */
int vectrex_imager_status = 0;	   /* 0 = off, 1 = right eye, 2 = left eye */
double imager_freq;
mame_timer *imager_timer;
int vectrex_lightpen_port=0;
UINT8 *vectrex_ram_base;
size_t vectrex_ram_size;

/*********************************************************************
  Local variables
 *********************************************************************/

/* Colors for right and left eye */
static UINT32 imager_colors[6] = {VC_WHITE,VC_WHITE,VC_WHITE,VC_WHITE,VC_WHITE,VC_WHITE};

/* Starting points of the three colors */
/* Values taken from J. Nelson's drawings*/
//static const double narrow_escape_angles[3] = {0,0.15277778, 0.34444444};
//static const double minestorm_3d_angles[3] = {0,0.16111111, 0.18888888};
//static const double crazy_coaster_angles[3] = {0,0.15277778, 0.34444444};

static const double minestorm_3d_angles[3] = {0,0.1692, 0.2086};
static const double narrow_escape_angles[3] = {0,0.1631, 0.3305};
static const double crazy_coaster_angles[3] = {0,0.1631, 0.3305};


static const double unknown_game_angles[3] = {0,0.16666666, 0.33333333};
static const double *vectrex_imager_angles = unknown_game_angles;
static unsigned char vectrex_imager_pinlevel=0x00;

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
DEVICE_LOAD( vectrex_cart )
{
	image_fread(image, memory_region(REGION_CPU1), 0x8000);

	/* check image! */
	if (vectrex_verify_cart((char*)memory_region(REGION_CPU1)) == IMAGE_VERIFY_FAIL)
	{
		logerror("Invalid image!\n");
		return INIT_FAIL;
	}

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
	}

	return INIT_PASS;
}

/*********************************************************************
  Vectrex configuration (mainly 3D Imager)
 *********************************************************************/
void vectrex_configuration(void)
{
	unsigned char cport = input_port_5_r (0);

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
			imager_colors[0]=imager_colors[1]=imager_colors[2]=VC_BLACK;
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
			imager_colors[3]=imager_colors[4]=imager_colors[5]=VC_BLACK;
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
		vectrex_beam_color = VC_WHITE;
		imager_colors[0]=imager_colors[1]=imager_colors[2]=imager_colors[3]=imager_colors[4]=imager_colors[5]=VC_WHITE;
	}
	vectrex_lightpen_port = (input_port_6_r (0) & 0x03);
}

/*********************************************************************
  VIA interface functions
 *********************************************************************/
void v_via_irq (int level)
{
	cpunum_set_input_line(0, M6809_IRQ_LINE, level);
}

 READ8_HANDLER( v_via_pb_r )
{
	int pot;
	pot = readinputport(((vectrex_via_out[PORTB] & 0x6)>>1) + 1) - 0x80;

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
		vectrex_via_out[PORTA] = AY8910_read_port_0_r (0)
			& ~(vectrex_imager_pinlevel & 0x80);
		vectrex_imager_pinlevel &= ~0x80;
	}
	return vectrex_via_out[PORTA];
}

 READ8_HANDLER( s1_via_pb_r )
{
	return (vectrex_via_out[PORTB] & ~0x40) | ((input_port_1_r(0) & 0x1)<<6);
}

/*********************************************************************
  3D Imager support
 *********************************************************************/
static void vectrex_imager_change_color (int i)
{
	vectrex_beam_color = i;
}

void vectrex_imager_right_eye (int param)
{
	int coffset;
	double rtime = (1.0/imager_freq);

	if (vectrex_imager_status > 0)
	{
		vectrex_imager_status = param;
		coffset = param>1?3:0;
		timer_set (rtime * vectrex_imager_angles[0], imager_colors[coffset+2], vectrex_imager_change_color);
		timer_set (rtime * vectrex_imager_angles[1], imager_colors[coffset+1], vectrex_imager_change_color);
		timer_set (rtime * vectrex_imager_angles[2], imager_colors[coffset], vectrex_imager_change_color);

		if (param == 2)
		{
			timer_set (rtime * 0.50, 1, vectrex_imager_right_eye);

			/* Index hole sensor is connected to IO7 which triggers also CA1 of VIA */
			via_0_ca1_w (0, 1);
			via_0_ca1_w (0, 0);
			vectrex_imager_pinlevel |= 0x80;
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
		tmp = timer_get_time();
		wavel = tmp - sl;
		sl = tmp;

		if (wavel < 1)
		{
			/* The Vectrex sends a stream of pulses which controls the speed of
			   the motor using Pulse Width Modulation. Guessed parameters are MMI
			   (mass moment of inertia) of the color wheel, DAMPC (damping coefficient)
			   of the whole thing and some constants of the motor's torque/speed curve.
			   pwl is the negative pulse width and wavel is the whole wavelength. */

			ang_acc = (50.0 - 1.55 * imager_freq) / MMI;
			imager_freq += ang_acc * pwl + DAMPC*imager_freq/MMI * wavel;

			if (imager_freq > 1)
				timer_adjust (imager_timer, MIN(1.0/imager_freq, timer_timeleft(imager_timer)), 2, 1.0/imager_freq);
		}
	}
	if (mcontrol && mcontrol ^ state)
	{
		state = mcontrol;
		pwl = timer_get_time() - sl;
	}
}

DRIVER_INIT( vectrex )
{
	int i;

	/* Set the whole cart ROM area to 1. This is needed to work around a bug (?)
	 * in Minestorm where the exec-rom attempts to access a vector list here.
	 * 1 signals the end of the vector list.
	 */
	if (vectrex_verify_cart((char*)memory_region(REGION_CPU1)) == IMAGE_VERIFY_FAIL)
	{
		memset (memory_region(REGION_CPU1), 1, 0x8000);
	}

	artwork_use_device_art(image_from_devtype_and_index(IO_CARTSLOT, 0), "mine");
	
	for (i = 0; i < vectrex_ram_size; i++)
		vectrex_ram_base[i] = rand();
}
