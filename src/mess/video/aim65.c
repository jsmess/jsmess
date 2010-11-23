/******************************************************************************

    PeT mess@utanet.at Nov 2000
    Updated by Dan Boris, 3/4/2007

******************************************************************************/

#include "emu.h"

#include "includes/aim65.h"
#include "machine/6522via.h"



//UINT16 *printerRAM;




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


static void aim65_printer_inc(aim65_state *state)
{
	if (state->printer_dir)
	{
		if (state->printer_x > 0) {
			state->printer_x--;
		}
		else {
			state->printer_dir = 0;
			state->printer_x++;
			state->printer_y++;
		}
	}
	else
	{
		if (state->printer_x < 9) {
			state->printer_x++;
		}
		else {
			state->printer_dir = 1;
			state->printer_x--;
			state->printer_y++;
		}
	}

	if (state->printer_y > 500) state->printer_y = 0;

	state->flag_a=0;
	state->flag_b=0;
}

static void aim65_printer_cr(aim65_state *state)
{
	state->printer_x=0;
	state->printer_y++;
	if (state->printer_y > 500) state->printer_y = 0;
	state->flag_a=state->flag_b=0;
}

static TIMER_CALLBACK(aim65_printer_timer)
{
	aim65_state *state = machine->driver_data<aim65_state>();
	via6522_device *via_0 = machine->device<via6522_device>("via6522_0");

	via_0->write_cb1(state->printer_level);
	via_0->write_cb1(!state->printer_level);
	state->printer_level = !state->printer_level;
	aim65_printer_inc(state);
}


WRITE8_DEVICE_HANDLER( aim65_printer_on )
{
	aim65_state *state = device->machine->driver_data<aim65_state>();
	via6522_device *via_0 = device->machine->device<via6522_device>("via6522_0");
	if (!data)
	{
		aim65_printer_cr(state);
		timer_adjust_periodic(state->print_timer, attotime_zero, 0, ATTOTIME_IN_USEC(10));
		via_0->write_cb1(0);
		state->printer_level = 1;
	}
	else
		timer_reset(state->print_timer, attotime_never);
}


WRITE8_DEVICE_HANDLER( aim65_printer_data_a )
{
#if 0
	aim65_state *state = device->machine->driver_data<aim65_state>();
	if (state->flag_a == 0) {
		printerRAM[(state->printer_y * 20) + state->printer_x] |= data;
		state->flag_a = 1;
	}
#endif
}

WRITE8_DEVICE_HANDLER( aim65_printer_data_b )
{
#if 0
	aim65_state *state = device->machine->driver_data<aim65_state>();
	data &= 0x03;

	if (state->flag_b == 0) {
		printerRAM[(state->printer_y * 20) + state->printer_x ] |= (data << 8);
		state->flag_b = 1;
	}
#endif
}

VIDEO_START( aim65 )
{
	aim65_state *state = machine->driver_data<aim65_state>();
	state->print_timer = timer_alloc(machine, aim65_printer_timer, NULL);

#if 0
	videoram_size = 600 * 10 * 2;
	printerRAM = auto_alloc_array(machine, UINT16, videoram_size / 2);
	memset(printerRAM, 0, videoram_size);
	state->printer_x = 0;
	state->printer_y = 0;
	state->printer_dir = 0;
	state->flag_a=0;
	state->flag_b=0;


	state->printer_level = 0;

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

