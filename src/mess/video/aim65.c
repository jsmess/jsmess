/******************************************************************************

    PeT mess@utanet.at Nov 2000
    Updated by Dan Boris, 3/4/2007

******************************************************************************/

#include "driver.h"

#include "includes/aim65.h"
#include "machine/6522via.h"

static int printer_x;
static int printer_y;
static int printer_dir;
static int flag_a;
static int flag_b;

UINT16 *printerRAM;

static mame_timer *print_timer;
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


void aim65_printer_inc(void)
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

void aim65_printer_cr(void) {
	printer_x=0;
	printer_y++;
	if (printer_y > 500) printer_y = 0;
	flag_a=flag_b=0;
}

TIMER_CALLBACK(aim65_printer_timer)
{
	via_0_cb1_w(0, printer_level);
	via_0_ca1_w(0, !printer_level);
	printer_level = !printer_level;
	aim65_printer_inc();
}


WRITE8_HANDLER( aim65_printer_on )
{
	if (!data)
	{
		aim65_printer_cr();
		mame_timer_adjust(print_timer, time_zero, 0, MAME_TIME_IN_USEC(10));
		via_0_cb1_w(0, 0);
		printer_level = 1;
	}
	else
		mame_timer_reset(print_timer, time_never);
}


void aim65_printer_data_a(UINT8 data) {
/*  if (flag_a == 0) {
        printerRAM[(printer_y * 20) + printer_x] |= data;
        flag_a = 1;
    }*/
}

void aim65_printer_data_b(UINT8 data) {
/*  data &= 0x03;

    if (flag_b == 0) {
        printerRAM[(printer_y * 20) + printer_x ] |= (data << 8);
        flag_b = 1;
    }*/
}

VIDEO_START( aim65 )
{
	print_timer = mame_timer_alloc(aim65_printer_timer);

	/*
    videoram_size = 600 * 10 * 2;
    printerRAM = (UINT16*)auto_malloc (videoram_size );
    memset(printerRAM, 0, videoram_size);
    printer_x = 0;
    printer_y = 0;
    printer_dir = 0;
    flag_a=0;
    flag_b=0;


    printer_level = 0;

    video_start_generic(machine);
    */
}


VIDEO_UPDATE( aim65 )
{
	/* Display printer output */
/*  dir = 1;
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
                color=Machine->pens[(data & 0x1)?2:0];
                plot_pixel(bitmap,700 - ((b * 10) + x), y,color);
                data = data >> 1;
            }
        }

        if (dir == 0)
            dir = 1;
        else
            dir = 0;
    } */

	return 0;
}

