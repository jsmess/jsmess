#include "driver.h"
#include "includes/kyocera.h"
#include "video/hd44102.h"
#include "video/hd61830.h"


READ8_HANDLER( kc85_lcd_status_r )
{
	kc85_state *state = space->machine->driver_data;

	UINT8 data = 0;
	int i;

	for (i = 0; i < 10; i++)
	{
		if (state->lcd_cs2[i])
		{
			data |= hd44102_status_r(state->hd44102[i], 0);
		}
	}

	return data;
}

READ8_HANDLER( kc85_lcd_data_r )
{
	kc85_state *state = space->machine->driver_data;
	UINT8 data = 0;
	int i;

	for (i = 0; i < 10; i++)
	{
		if (state->lcd_cs2[i])
		{
			data |= hd44102_data_r(state->hd44102[i], 0);
		}
	}

	return data;
}

WRITE8_HANDLER( kc85_lcd_command_w )
{
	kc85_state *state = space->machine->driver_data;
	int i;

	for (i = 0; i < 10; i++)
	{
		if (state->lcd_cs2[i])
		{
			hd44102_control_w(state->hd44102[i], 0, data);
		}
	}
}

WRITE8_HANDLER( kc85_lcd_data_w )
{
	kc85_state *state = space->machine->driver_data;
	int i;

	for (i = 0; i < 10; i++)
	{
		if (state->lcd_cs2[i])
		{
			hd44102_data_w(state->hd44102[i], 0, data);
		}
	}
}

static PALETTE_INIT( kc85 )
{
	palette_set_color(machine, 0, MAKE_RGB(138, 146, 148));
	palette_set_color(machine, 1, MAKE_RGB(92, 83, 88));
}

static VIDEO_START( kc85 )
{
	kc85_state *state = machine->driver_data;

	/* find devices */
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
	state_save_register_global_array(machine, state->lcd_cs2);
}

static VIDEO_UPDATE( kc85 )
{
	kc85_state *state = screen->machine->driver_data;

	int i;

	for (i = 0; i < 10; i++)
	{
		hd44102_update(state->hd44102[i], bitmap, cliprect);
	}

	return 0;
}

static VIDEO_START( tandy200 )
{
	tandy200_state *state = machine->driver_data;

	/* find devices */
	state->hd61830 = devtag_get_device(machine, HD61830_TAG);

	/* allocate video memory */
	state->video_ram = auto_alloc_array(machine, UINT8, TANDY200_VIDEORAM_SIZE);

	/* register for state saving */
	state_save_register_global_pointer(machine, state->video_ram, TANDY200_VIDEORAM_SIZE);
}

static VIDEO_UPDATE( tandy200 )
{
	tandy200_state *state = screen->machine->driver_data;

	hd61830_update(state->hd61830, bitmap, cliprect);

	return 0;
}

static READ8_HANDLER( tandy200_rd_r )
{
	tandy200_state *state = space->machine->driver_data;

	return state->video_ram[offset & TANDY200_VIDEORAM_MASK];
}

static WRITE8_HANDLER( tandy200_rd_w )
{
	tandy200_state *state = space->machine->driver_data;

	state->video_ram[offset & TANDY200_VIDEORAM_MASK] = data;
}

static HD61830_INTERFACE( tandy200_hd61830_intf )
{
	SCREEN_TAG,
	DEVCB_MEMORY_HANDLER(I8085_TAG, PROGRAM, tandy200_rd_r),
	DEVCB_MEMORY_HANDLER(I8085_TAG, PROGRAM, tandy200_rd_w)
};

MACHINE_DRIVER_START( kc85_video )
	MDRV_SCREEN_ADD(SCREEN_TAG, LCD)
	MDRV_SCREEN_REFRESH_RATE(44)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(240, 64)
	MDRV_SCREEN_VISIBLE_AREA(0, 240-1, 0, 64-1)

	MDRV_DEFAULT_LAYOUT(layout_lcd)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(kc85)

	MDRV_VIDEO_START(kc85)
	MDRV_VIDEO_UPDATE(kc85)

	MDRV_HD44102_ADD( "m1", SCREEN_TAG,   0,  0)
	MDRV_HD44102_ADD( "m2", SCREEN_TAG,  50,  0)
	MDRV_HD44102_ADD( "m3", SCREEN_TAG, 100,  0)
	MDRV_HD44102_ADD( "m4", SCREEN_TAG, 150,  0)
	MDRV_HD44102_ADD( "m5", SCREEN_TAG, 200,  0)
	MDRV_HD44102_ADD( "m6", SCREEN_TAG,   0, 32)
	MDRV_HD44102_ADD( "m7", SCREEN_TAG,  50, 32)
	MDRV_HD44102_ADD( "m8", SCREEN_TAG, 100, 32)
	MDRV_HD44102_ADD( "m9", SCREEN_TAG, 150, 32)
	MDRV_HD44102_ADD("m10", SCREEN_TAG, 200, 32)

//	MDRV_HD44103_MASTER_ADD("m11", SCREEN_TAG, CAP_P(18), RES_K(100), HD44103_FS_HIGH, HD44103_DUTY_1_32)
//	MDRV_HD44103_SLAVE_ADD( "m12", "m11", SCREEN_TAG, HD44103_FS_HIGH, HD44103_DUTY_1_32)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( tandy200_video )
	MDRV_SCREEN_ADD(SCREEN_TAG, LCD)
	MDRV_SCREEN_REFRESH_RATE(80)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(240, 128)
	MDRV_SCREEN_VISIBLE_AREA(0, 240-1, 0, 128-1)

	MDRV_DEFAULT_LAYOUT(layout_lcd)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(kc85)

	MDRV_VIDEO_START(tandy200)
	MDRV_VIDEO_UPDATE(tandy200)

	MDRV_HD61830_ADD(HD61830_TAG, XTAL_4_9152MHz/2/2, tandy200_hd61830_intf)
MACHINE_DRIVER_END
