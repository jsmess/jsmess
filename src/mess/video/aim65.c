/******************************************************************************

    PeT mess@utanet.at Nov 2000
    Updated by Dan Boris, 3/4/2007

******************************************************************************/

#include "emu.h"

#include "includes/aim65.h"
#include "machine/6522via.h"


static int printer_x;
static int printer_y;
static int printer_dir;
static int flag_a;
static int flag_b;

//UINT16 *printerRAM;

static emu_timer *print_timer;
static int printer_level;



/******************************************************************************
 Printer
******************************************************************************/

/*
  aim65 thermal printer (20 characters)
  10 heat elements (place on 1 line, space between 2 characters(about 14dots))
  (pa0..pa7,pb0,pb1 1 heat element on)

  cb2 0 motor, heat elements on
  cb1 output start!?
  ca1 input

  normally printer 5x7 characters
  (horizontal movement limits not known, normally 2 dots between characters)

  3 dots space between lines?
 */


static void aim65_printer_inc(void)
{
	if (printer_dir)
	{
		if (printer_x > 0) {
			printer_x--;
		}
		else {
			printer_dir = 0;
			printer_x++;
			printer_y++;
		}
	}
	else
	{
		if (printer_x < 9) {
			printer_x++;
		}
		else {
			printer_dir = 1;
			printer_x--;
			printer_y++;
		}
	}

	if (printer_y > 500) printer_y = 0;

	flag_a=0;
	flag_b=0;
}

static void aim65_printer_cr(void) {
	printer_x=0;
	printer_y++;
	if (printer_y > 500) printer_y = 0;
	flag_a=flag_b=0;
}

static TIMER_CALLBACK(aim65_printer_timer)
{
	running_device *via_0 = devtag_get_device(machine, "via6522_0");

	via_cb1_w(via_0, printer_level);
	via_ca1_w(via_0, !printer_level);
	printer_level = !printer_level;
	aim65_printer_inc();
}


WRITE8_DEVICE_HANDLER( aim65_printer_on )
{
	if (!data)
	{
		aim65_printer_cr();
		timer_adjust_periodic(print_timer, attotime_zero, 0, ATTOTIME_IN_USEC(10));
		via_cb1_w(device, 0);
		printer_level = 1;
	}
	else
		timer_reset(print_timer, attotime_never);
}


WRITE8_DEVICE_HANDLER( aim65_printer_data_a )
{
#if 0
	if (flag_a == 0) {
		printerRAM[(printer_y * 20) + printer_x] |= data;
		flag_a = 1;
	}
#endif
}

WRITE8_DEVICE_HANDLER( aim65_printer_data_b )
{
#if 0
	data &= 0x03;

	if (flag_b == 0) {
		printerRAM[(printer_y * 20) + printer_x ] |= (data << 8);
		flag_b = 1;
	}
#endif
}

VIDEO_START( aim65 )
{
	print_timer = timer_alloc(machine, aim65_printer_timer, NULL);

#if 0
	videoram_size = 600 * 10 * 2;
	printerRAM = auto_alloc_array(machine, UINT16, videoram_size / 2);
	memset(printerRAM, 0, videoram_size);
	printer_x = 0;
	printer_y = 0;
	printer_dir = 0;
	flag_a=0;
	flag_b=0;


	printer_level = 0;

	VIDEO_START_CALL(generic);
#endif
}


#ifdef UNUSED_FUNCTION
VIDEO_UPDATE( aim65 )
{
	/* Display printer output */
#if 0
	dir = 1;
	for(y = 0; y<500;y++)
	{
		for(x = 0; x< 10; x++)
		{
			if (dir == 1) {
				data = printerRAM[y * 10  + x];
			}
			else {
				data = printerRAM[(y * 10) + (9 - x)];
			}


			for (b = 0; b<10; b++)
			{
				color=machine->pens[(data & 0x1)?2:0];
				plot_pixel(bitmap,700 - ((b * 10) + x), y,color);
				data = data >> 1;
			}
		}

		if (dir == 0)
			dir = 1;
		else
			dir = 0;
	}
#endif

	return 0;
}
#endif

