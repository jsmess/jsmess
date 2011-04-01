/***************************************************************************

        Irisha machine driver by Miodrag Milanovic

        27/03/2008 Preliminary driver.

****************************************************************************/


#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "machine/i8255a.h"
#include "machine/pit8253.h"
#include "machine/pic8259.h"
#include "machine/msm8251.h"
#include "includes/irisha.h"


/* Driver initialization */
DRIVER_INIT(irisha)
{
	irisha_state *state = machine.driver_data<irisha_state>();
	state->m_keyboard_mask = 0;
}



static TIMER_CALLBACK( irisha_key )
{
	irisha_state *state = machine.driver_data<irisha_state>();
	state->m_keypressed = 1;
	state->m_keyboard_cnt = 0;
}

MACHINE_START( irisha )
{
	machine.scheduler().timer_pulse(attotime::from_msec(30), FUNC(irisha_key));
}

MACHINE_RESET( irisha )
{
	irisha_state *state = machine.driver_data<irisha_state>();
	state->m_keypressed = 0;
}

static const char *const keynames[] = {
							"LINE0", "LINE1", "LINE2", "LINE3",
							"LINE4", "LINE5", "LINE6", "LINE7",
							"LINE8", "LINE9"
};

static READ8_DEVICE_HANDLER (irisha_8255_portb_r )
{
	irisha_state *state = device->machine().driver_data<irisha_state>();
  if (state->m_keypressed==1) {
	state->m_keypressed =0;
	return 0x80;
  }

	return 0x00;
}

static READ8_DEVICE_HANDLER (irisha_8255_portc_r )
{
	logerror("irisha_8255_portc_r\n");
	return 0;
}

READ8_HANDLER (irisha_keyboard_r)
{
	irisha_state *state = space->machine().driver_data<irisha_state>();
	UINT8 keycode;
	if (state->m_keyboard_cnt!=0 && state->m_keyboard_cnt<11) {
		keycode = input_port_read(space->machine(), keynames[state->m_keyboard_cnt-1]) ^ 0xff;
	} else {
		keycode = 0xff;
	}
	state->m_keyboard_cnt++;
	return keycode;
}

static WRITE8_DEVICE_HANDLER (irisha_8255_porta_w )
{
	logerror("irisha_8255_porta_w %02x\n",data);
}

static WRITE8_DEVICE_HANDLER (irisha_8255_portb_w )
{
	logerror("irisha_8255_portb_w %02x\n",data);
}

static WRITE8_DEVICE_HANDLER (irisha_8255_portc_w )
{
	//logerror("irisha_8255_portc_w %02x\n",data);
}

I8255A_INTERFACE( irisha_ppi8255_interface )
{
	DEVCB_NULL,
	DEVCB_HANDLER(irisha_8255_portb_r),
	DEVCB_HANDLER(irisha_8255_portc_r),
	DEVCB_HANDLER(irisha_8255_porta_w),
	DEVCB_HANDLER(irisha_8255_portb_w),
	DEVCB_HANDLER(irisha_8255_portc_w),
};

static WRITE_LINE_DEVICE_HANDLER( irisha_pic_set_int_line )
{
	cputag_set_input_line(device->machine(), "maincpu", 0, state ? HOLD_LINE : CLEAR_LINE);
}

const struct pic8259_interface irisha_pic8259_config =
{
	DEVCB_LINE(irisha_pic_set_int_line)
};

const struct pit8253_config irisha_pit8253_intf =
{
	{
		{
			0,
			DEVCB_NULL,
			DEVCB_NULL
		},
		{
			0,
			DEVCB_NULL,
			DEVCB_NULL
		},
		{
			2000000,
			DEVCB_NULL,
			DEVCB_NULL
		}
	}
};


