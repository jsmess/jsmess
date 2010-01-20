/***************************************************************************

        BK machine driver by Miodrag Milanovic

        10/03/2008 Preliminary driver.

****************************************************************************/


#include "emu.h"
#include "devices/cassette.h"
#include "machine/i8255a.h"
#include "includes/bk.h"

static UINT16 kbd_state;
UINT16 bk_scrool;
static UINT16 key_code;
static UINT16 key_pressed;
static UINT16 key_irq_vector;
static UINT16 bk_drive;

static TIMER_CALLBACK(keyboard_callback)
{
	UINT8 code, i, j;
	static const char *const keynames[] = {
		"LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6",
		"LINE7", "LINE8", "LINE9", "LINE10", "LINE11"
	};

	for(i = 1; i < 12; i++)
	{
		code = 	input_port_read(machine, keynames[i-1]);
		if (code != 0)
		{
			for(j = 0; j < 8; j++)
			{
				if (code == (1 << j))
				{
					key_code = j + i*8;
					break;
				}
			}
			if ((input_port_read(machine, "LINE0") & 4) == 4)
			{
				if (i==6 || i==7)
				{
					key_code -= 16;
				}

			}
			if ((input_port_read(machine, "LINE0") & 4) == 4)
			{
				if (i>=8 && i<=11)
				{
					key_code += 32;
				}
			}
			key_pressed = 0x40;
			if ((input_port_read(machine, "LINE0") & 2) == 0)
			{
				key_irq_vector = 0x30;
			}
			else
			{
				key_irq_vector = 0xBC;
			}
			cputag_set_input_line(machine, "maincpu", 0, ASSERT_LINE);
			break;
		}
	}
}


MACHINE_START(bk0010)
{
	timer_pulse(machine, ATTOTIME_IN_HZ(2400), NULL, 0, keyboard_callback);
}

static IRQ_CALLBACK(bk0010_irq_callback)
{
	cpu_set_input_line(device, 0, CLEAR_LINE);
	return key_irq_vector;
}

MACHINE_RESET( bk0010 )
{
	cpu_set_irq_callback(devtag_get_device(machine, "maincpu"), bk0010_irq_callback);

	kbd_state = 0;
	bk_scrool = 01330;
}

READ16_HANDLER (bk_key_state_r)
{
	return kbd_state;
}
READ16_HANDLER (bk_key_code_r)
{
	kbd_state &= ~0x80; // mark reading done
	key_pressed = 0;
	return key_code;
}
READ16_HANDLER (bk_vid_scrool_r)
{
	return bk_scrool;
}

READ16_HANDLER (bk_key_press_r)
{
	double level = cassette_input(devtag_get_device(space->machine, "cassette"));
	UINT16 cas;
	if (level < 0)
	{
	 	cas = 0x00;
 	}
	else
	{
		cas = 0x20;
	}

	return 0x8080 | key_pressed | cas;
}

WRITE16_HANDLER(bk_key_state_w)
{
	kbd_state = (kbd_state & ~0x40) | (data & 0x40);
}

WRITE16_HANDLER(bk_vid_scrool_w)
{
	bk_scrool = data;
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
	if ((data & 1) == 1)
	{
		bk_drive = 0;
	}
	if ((data & 2) == 2)
	{
		bk_drive = 1;
	}
	if ((data & 4) == 4)
	{
		bk_drive = 2;
	}
	if ((data & 8) == 8)
	{
		bk_drive = 3;
	}
	if (data == 0)
	{
		bk_drive = -1;
	}
}

READ16_HANDLER (bk_floppy_data_r)
{
	return 0;
}

WRITE16_HANDLER(bk_floppy_data_w)
{
}

