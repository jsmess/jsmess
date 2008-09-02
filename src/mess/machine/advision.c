/***************************************************************************

  advision.c

  Machine file to handle emulation of the AdventureVision.

***************************************************************************/

#include "driver.h"
#include "includes/advision.h"
#include "cpu/mcs48/mcs48.h"
#include "devices/cartslot.h"
#include "sound/dac.h"

/*
    8048 Ports:
    
    P1  Bit 0..1  - RAM bank select
        Bit 3..7  - Keypad input

    P2  Bit 0..3  - A8-A11
        Bit 4..7  - Sound control/Video write address

    T1  Mirror sync pulse
*/

/* Machine Initialization */

MACHINE_START( advision )
{
	/* configure EA banking */
	memory_configure_bank(MCS48_INTERNAL_ROMBANK, 0, 1, memory_region(machine, "bios"), 0);
	memory_configure_bank(MCS48_INTERNAL_ROMBANK, 1, 1, memory_region(machine, "main"), 0);
}

MACHINE_RESET( advision )
{
	advision_state *state = machine->driver_data;

	/* allocate external RAM */
	state->extram = auto_malloc(0x400);

	/* enable internal ROM */
	cpunum_set_input_line(machine, 0, MCS48_INPUT_EA, CLEAR_LINE);

	/* reset sound CPU */
	cpunum_set_input_line(machine, 1, INPUT_LINE_RESET, ASSERT_LINE);

	state->rambank = 0x300;
	state->frame_start = 0;
	state->video_enable = 0;
	state->sound_cmd = 0;
}

/* Bank Switching */

WRITE8_HANDLER( advision_bankswitch_w )
{
	advision_state *state = machine->driver_data;

	cpunum_set_input_line(machine, 0, MCS48_INPUT_EA, (data & 0x04) ? ASSERT_LINE : CLEAR_LINE);

	state->rambank = (data & 0x03) << 8;
}

/* External RAM */

READ8_HANDLER( advision_extram_r )
{
	advision_state *state = machine->driver_data;

	UINT8 data = state->extram[state->rambank + offset];

	if (!state->video_enable)
	{
		/* the video hardware interprets reads as writes */
		advision_vh_write(machine, data);
	}

	if (state->video_bank == 0x06)
	{
		cpunum_set_input_line(machine, 1, INPUT_LINE_RESET, (data & 0x01) ? CLEAR_LINE : ASSERT_LINE);
	}

	return data;
}

WRITE8_HANDLER( advision_extram_w )
{
	advision_state *state = machine->driver_data;

	state->extram[state->rambank + offset] = data;
}

/* Sound */

READ8_HANDLER( advision_sound_cmd_r )
{
	advision_state *state = machine->driver_data;

	return state->sound_cmd;
}

static void update_dac(running_machine *machine)
{
	advision_state *state = machine->driver_data;

	if (state->sound_g == 0 && state->sound_d == 0)
		dac_data_w(0, 0xff);
	else if (state->sound_g == 1 && state->sound_d == 1)
		dac_data_w(0, 0x80);
	else
		dac_data_w(0, 0x00);
}

WRITE8_HANDLER( advision_sound_g_w )
{
	advision_state *state = machine->driver_data;
	state->sound_g = data & 0x01;
	update_dac(machine);
}


WRITE8_HANDLER( advision_sound_d_w )
{
	advision_state *state = machine->driver_data;
	state->sound_d = data & 0x01;
	update_dac(machine);
}

/* Video */

WRITE8_HANDLER( advision_av_control_w )
{
	advision_state *state = machine->driver_data;

	state->sound_cmd = data >> 4;

	if ((state->video_enable == 0x00) && (data & 0x10))
	{
		advision_vh_update(machine, state->video_hpos);
		
		state->video_hpos++;
		
		if (state->video_hpos > 255)
		{
			state->video_hpos = 0;
			logerror("HPOS OVERFLOW\n");
		}
	}
	
	state->video_enable = data & 0x10;
	state->video_bank = (data & 0xe0) >> 5;
}

READ8_HANDLER( advision_vsync_r )
{
	advision_state *state = machine->driver_data;

	if (state->frame_start)
	{
		state->frame_start = 0;

		return 0;
	}
	else
	{
		return 1;
	}
}

/* Input */

READ8_HANDLER( advision_controller_r )
{
	// Get joystick switches
	UINT8 in = input_port_read(machine, "joystick");
	UINT8 data = in | 0x0f;

	// Get buttons
	if (in & 0x02) data = data & 0xf7; /* Button 3 */
	if (in & 0x08) data = data & 0xcf; /* Button 1 */
	if (in & 0x04) data = data & 0xaf; /* Button 2 */
	if (in & 0x01) data = data & 0x6f; /* Button 4 */

	return data & 0xf8;
}
