#include "driver.h"
#include "includes/kyocera.h"
#include "video/hd44102.h"

void kyo85_set_lcd_bank(running_machine *machine, UINT16 data)
{
	kyocera_state *state = machine->driver_data;

	state->lcd_cs2 = data;
}

READ8_HANDLER( kyo85_lcd_status_r )
{
	kyocera_state *state = space->machine->driver_data;

	UINT8 data = 0;
	int i;

	for (i = 0; i < 10; i++)
	{
		if (BIT(state->lcd_cs2, i))
		{
			data |= hd44102_status_r(state->hd44102[i], 0);
		}
	}

	return data;
}

READ8_HANDLER( kyo85_lcd_data_r )
{
	kyocera_state *state = space->machine->driver_data;
	UINT8 data = 0;
	int i;

	for (i = 0; i < 10; i++)
	{
		if (BIT(state->lcd_cs2, i))
		{
			data |= hd44102_data_r(state->hd44102[i], 0);
		}
	}

	return data;
}

WRITE8_HANDLER( kyo85_lcd_command_w )
{
	kyocera_state *state = space->machine->driver_data;
	int i;

	for (i = 0; i < 10; i++)
	{
		if (BIT(state->lcd_cs2, i))
		{
			hd44102_control_w(state->hd44102[i], 0, data);
		}
	}
}

WRITE8_HANDLER( kyo85_lcd_data_w )
{
	kyocera_state *state = space->machine->driver_data;
	int i;
	
	for (i = 0; i < 10; i++)
	{
		if (BIT(state->lcd_cs2, i))
		{
			hd44102_data_w(state->hd44102[i], 0, data);
		}
	}
}

static PALETTE_INIT( kyo85 )
{
	palette_set_color(machine, 0, MAKE_RGB(138, 146, 148));
	palette_set_color(machine, 1, MAKE_RGB(92, 83, 88));
}

static VIDEO_START( kyo85 )
{
	kyocera_state *state = machine->driver_data;
	
	state->hd44102[0] = devtag_get_device(machine, "m1");
	state->hd44102[1] = devtag_get_device(machine, "m2");
	state->hd44102[2] = devtag_get_device(machine, "m3");
	state->hd44102[3] = devtag_get_device(machine, "m4");
	state->hd44102[4] = devtag_get_device(machine, "m5");
	state->hd44102[5] = devtag_get_device(machine, "m6");
	state->hd44102[6] = devtag_get_device(machine, "m7");
	state->hd44102[7] = devtag_get_device(machine, "m8");
	state->hd44102[8] = devtag_get_device(machine, "m9");
	state->hd44102[9] = devtag_get_device(machine, "m10");

	/* register for state saving */
	state_save_register_global(machine, state->lcd_cs2);
}

static VIDEO_UPDATE( kyo85 )
{
	kyocera_state *state = screen->machine->driver_data;

	int i;

	for (i = 0; i < 10; i++)
	{
		hd44102_update(state->hd44102[i], bitmap, cliprect);
	}

	return 0;
}

MACHINE_DRIVER_START( kyo85_video )
	MDRV_SCREEN_ADD(SCREEN_TAG, LCD)
	MDRV_SCREEN_REFRESH_RATE(44)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(240, 64)
	MDRV_SCREEN_VISIBLE_AREA(0, 240-1, 0, 64-1)

//	MDRV_DEFAULT_LAYOUT(layout_kyo85)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(kyo85)

	MDRV_VIDEO_START(kyo85)
	MDRV_VIDEO_UPDATE(kyo85)

	MDRV_HD44102_ADD( "m1", 0, SCREEN_TAG,   0,  0)
	MDRV_HD44102_ADD( "m2", 0, SCREEN_TAG,  50,  0)
	MDRV_HD44102_ADD( "m3", 0, SCREEN_TAG, 100,  0)
	MDRV_HD44102_ADD( "m4", 0, SCREEN_TAG, 150,  0)
	MDRV_HD44102_ADD( "m5", 0, SCREEN_TAG, 200,  0)
	MDRV_HD44102_ADD( "m6", 0, SCREEN_TAG,   0, 32)
	MDRV_HD44102_ADD( "m7", 0, SCREEN_TAG,  50, 32)
	MDRV_HD44102_ADD( "m8", 0, SCREEN_TAG, 100, 32)
	MDRV_HD44102_ADD( "m9", 0, SCREEN_TAG, 150, 32)
	MDRV_HD44102_ADD("m10", 0, SCREEN_TAG, 200, 32)
MACHINE_DRIVER_END
