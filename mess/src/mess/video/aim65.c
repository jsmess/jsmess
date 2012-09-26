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
	if (state->m_printer_dir)
	{
		if (state->m_printer_x > 0) {
			state->m_printer_x--;
		}
		else {
			state->m_printer_dir = 0;
			state->m_printer_x++;
			state->m_printer_y++;
		}
	}
	else
	{
		if (state->m_printer_x < 9) {
			state->m_printer_x++;
		}
		else {
			state->m_printer_dir = 1;
			state->m_printer_x--;
			state->m_printer_y++;
		}
	}

	if (state->m_printer_y > 500) state->m_printer_y = 0;

	state->m_flag_a=0;
	state->m_flag_b=0;
}

static void aim65_printer_cr(aim65_state *state)
{
	state->m_printer_x=0;
	state->m_printer_y++;
	if (state->m_printer_y > 500) state->m_printer_y = 0;
	state->m_flag_a=state->m_flag_b=0;
}

static TIMER_CALLBACK(aim65_printer_timer)
{
	aim65_state *state = machine.driver_data<aim65_state>();
	via6522_device *via_0 = machine.device<via6522_device>("via6522_0");

	via_0->write_cb1(state->m_printer_level);
	via_0->write_cb1(!state->m_printer_level);
	state->m_printer_level = !state->m_printer_level;
	aim65_printer_inc(state);
}


WRITE8_DEVICE_HANDLER( aim65_printer_on )
{
	aim65_state *state = device->machine().driver_data<aim65_state>();
	via6522_device *via_0 = device->machine().device<via6522_device>("via6522_0");
	if (!data)
	{
		aim65_printer_cr(state);
		state->m_print_timer->adjust(attotime::zero, 0, attotime::from_usec(10));
		via_0->write_cb1(0);
		state->m_printer_level = 1;
	}
	else
		state->m_print_timer->reset();
}


WRITE8_DEVICE_HANDLER( aim65_printer_data_a )
{
#if 0
	aim65_state *state = device->machine().driver_data<aim65_state>();
	if (state->m_flag_a == 0) {
		printerRAM[(state->m_printer_y * 20) + state->m_printer_x] |= data;
		state->m_flag_a = 1;
	}
#endif
}

WRITE8_DEVICE_HANDLER( aim65_printer_data_b )
{
#if 0
	aim65_state *state = device->machine().driver_data<aim65_state>();
	data &= 0x03;

	if (state->m_flag_b == 0) {
		printerRAM[(state->m_printer_y * 20) + state->m_printer_x ] |= (data << 8);
		state->m_flag_b = 1;
	}
#endif
}

VIDEO_START( aim65 )
{
	aim65_state *state = machine.driver_data<aim65_state>();
	state->m_print_timer = machine.scheduler().timer_alloc(FUNC(aim65_printer_timer));

#if 0
	videoram_size = 600 * 10 * 2;
	printerRAM = auto_alloc_array(machine, UINT16, videoram_size / 2);
	memset(printerRAM, 0, videoram_size);
	state->m_printer_x = 0;
	state->m_printer_y = 0;
	state->m_printer_dir = 0;
	state->m_flag_a=0;
	state->m_flag_b=0;


	state->m_printer_level = 0;

	VIDEO_START_CALL(generic);
#endif
}


#ifdef UNUSED_FUNCTION
SCREEN_UPDATE( aim65 )
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
				color=machine.pens[(data & 0x1)?2:0];
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

