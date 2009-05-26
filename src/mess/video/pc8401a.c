#include "driver.h"
#include "includes/pc8401a.h"
#include "video/sed1330.h"
#include "pc8500.lh"

static PALETTE_INIT( pc8401a )
{
	palette_set_color(machine, 0, MAKE_RGB(39, 108, 51));
	palette_set_color(machine, 1, MAKE_RGB(16, 37, 84));
}

static VIDEO_START( pc8401a )
{
	pc8401a_state *state = machine->driver_data;

	/* allocate video memory */
	state->video_ram = auto_alloc_array(machine, UINT8, PC8401A_LCD_VIDEORAM_SIZE);

	/* register for state saving */
	state_save_register_global_pointer(machine, state->video_ram, PC8401A_LCD_VIDEORAM_SIZE);
}

static VIDEO_UPDATE( pc8401a )
{
	return 0;
}

static VIDEO_START( pc8500 )
{
	pc8401a_state *state = machine->driver_data;

	/* find devices */
	state->sed1330 = devtag_get_device(machine, SED1330_TAG);

	/* allocate video memory */
	state->video_ram = auto_alloc_array(machine, UINT8, PC8500_LCD_VIDEORAM_SIZE);

	/* register for state saving */
	state_save_register_global_pointer(machine, state->video_ram, PC8500_LCD_VIDEORAM_SIZE);
}

static VIDEO_UPDATE( pc8500 )
{
	pc8401a_state *state = screen->machine->driver_data;

	sed1330_update(state->sed1330, bitmap, cliprect);

	return 0;
}

static READ8_HANDLER( pc8500_sed1330_vd_r )
{
	pc8401a_state *state = space->machine->driver_data;

	return state->video_ram[offset & PC8500_LCD_VIDEORAM_MASK];
}

static WRITE8_HANDLER( pc8500_sed1330_vd_w )
{
	pc8401a_state *state = space->machine->driver_data;

	state->video_ram[offset & PC8500_LCD_VIDEORAM_MASK] = data;
}

static SED1330_INTERFACE( pc8500_sed1330_config )
{
	SCREEN_TAG,
	DEVCB_MEMORY_HANDLER(Z80_TAG, PROGRAM, pc8500_sed1330_vd_r),
	DEVCB_MEMORY_HANDLER(Z80_TAG, PROGRAM, pc8500_sed1330_vd_w)
};

MACHINE_DRIVER_START( pc8401a_video )
	MDRV_SCREEN_ADD(SCREEN_TAG, LCD)
	MDRV_SCREEN_REFRESH_RATE(44)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(480, 128)
	MDRV_SCREEN_VISIBLE_AREA(0, 480-1, 0, 128-1)

//	MDRV_DEFAULT_LAYOUT(layout_pc8401a)
	
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(pc8401a)

	MDRV_VIDEO_START(pc8401a)
	MDRV_VIDEO_UPDATE(pc8401a)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( pc8500_video )
	MDRV_SCREEN_ADD(SCREEN_TAG, LCD)
	MDRV_SCREEN_REFRESH_RATE(44)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(480, 208)
	MDRV_SCREEN_VISIBLE_AREA(0, 480-1, 0, 200-1)

	MDRV_DEFAULT_LAYOUT(layout_pc8500)
	
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(pc8401a)

	MDRV_VIDEO_START(pc8500)
	MDRV_VIDEO_UPDATE(pc8500)

	MDRV_SED1330_ADD(SED1330_TAG, 0, pc8500_sed1330_config)
MACHINE_DRIVER_END
