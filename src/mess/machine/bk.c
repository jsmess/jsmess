/***************************************************************************

        BK machine driver by Miodrag Milanovic

        10/03/2008 Preliminary driver.

****************************************************************************/


#include "emu.h"
#include "imagedev/cassette.h"
#include "machine/i8255a.h"
#include "includes/bk.h"


static TIMER_CALLBACK(keyboard_callback)
{
	bk_state *state = machine.driver_data<bk_state>();
	UINT8 code, i, j;
	static const char *const keynames[] = {
		"LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6",
		"LINE7", "LINE8", "LINE9", "LINE10", "LINE11"
	};

	for(i = 1; i < 12; i++)
	{
		code =	input_port_read(machine, keynames[i-1]);
		if (code != 0)
		{
			for(j = 0; j < 8; j++)
			{
				if (code == (1 << j))
				{
					state->m_key_code = j + i*8;
					break;
				}
			}
			if ((input_port_read(machine, "LINE0") & 4) == 4)
			{
				if (i==6 || i==7)
				{
					state->m_key_code -= 16;
				}

			}
			if ((input_port_read(machine, "LINE0") & 4) == 4)
			{
				if (i>=8 && i<=11)
				{
					state->m_key_code += 32;
				}
			}
			state->m_key_pressed = 0x40;
			if ((input_port_read(machine, "LINE0") & 2) == 0)
			{
				state->m_key_irq_vector = 0x30;
			}
			else
			{
				state->m_key_irq_vector = 0xBC;
			}
			cputag_set_input_line(machine, "maincpu", 0, ASSERT_LINE);
			break;
		}
	}
}


MACHINE_START(bk0010)
{
	machine.scheduler().timer_pulse(attotime::from_hz(2400), FUNC(keyboard_callback));
}

static IRQ_CALLBACK(bk0010_irq_callback)
{
	bk_state *state = device->machine().driver_data<bk_state>();
	device_set_input_line(device, 0, CLEAR_LINE);
	return state->m_key_irq_vector;
}

MACHINE_RESET( bk0010 )
{
	bk_state *state = machine.driver_data<bk_state>();
	device_set_irq_callback(machine.device("maincpu"), bk0010_irq_callback);

	state->m_kbd_state = 0;
	state->m_scrool = 01330;
}

READ16_HANDLER (bk_key_state_r)
{
	bk_state *state = space->machine().driver_data<bk_state>();
	return state->m_kbd_state;
}
READ16_HANDLER (bk_key_code_r)
{
	bk_state *state = space->machine().driver_data<bk_state>();
	state->m_kbd_state &= ~0x80; // mark reading done
	state->m_key_pressed = 0;
	return state->m_key_code;
}
READ16_HANDLER (bk_vid_scrool_r)
{
	bk_state *state = space->machine().driver_data<bk_state>();
	return state->m_scrool;
}

READ16_HANDLER (bk_key_press_r)
{
	bk_state *state = space->machine().driver_data<bk_state>();
	double level = cassette_input(space->machine().device("cassette"));
	UINT16 cas;
	if (level < 0)
	{
		cas = 0x00;
	}
	else
	{
		cas = 0x20;
	}

	return 0x8080 | state->m_key_pressed | cas;
}

WRITE16_HANDLER(bk_key_state_w)
{
	bk_state *state = space->machine().driver_data<bk_state>();
	state->m_kbd_state = (state->m_kbd_state & ~0x40) | (data & 0x40);
}

WRITE16_HANDLER(bk_vid_scrool_w)
{
	bk_state *state = space->machine().driver_data<bk_state>();
	state->m_scrool = data;
}

WRITE16_HANDLER(bk_key_press_w)
{
}

READ16_HANDLER (bk_floppy_cmd_r)
{
	return 0;
}

WRITE16_HANDLER(bk_floppy_cmd_w)
{
	bk_state *state = space->machine().driver_data<bk_state>();
	if ((data & 1) == 1)
	{
		state->m_drive = 0;
	}
	if ((data & 2) == 2)
	{
		state->m_drive = 1;
	}
	if ((data & 4) == 4)
	{
		state->m_drive = 2;
	}
	if ((data & 8) == 8)
	{
		state->m_drive = 3;
	}
	if (data == 0)
	{
		state->m_drive = -1;
	}
}

READ16_HANDLER (bk_floppy_data_r)
{
	return 0;
}

WRITE16_HANDLER(bk_floppy_data_w)
{
}

