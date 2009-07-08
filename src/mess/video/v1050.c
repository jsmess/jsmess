#include "driver.h"
#include "includes/v1050.h"
#include "video/mc6845.h"

/*

	TODO:

	- row update
	- vsync interrupt
	- attribute RAM

*/

/* MC6845 Interface */

static MC6845_UPDATE_ROW( v1050_update_row )
{
	v1050_state *state = device->machine->driver_data;

	int column, bit;

	for (column = 0; column < x_count; column++)
	{
		UINT16 address = ((ma + column) << 5) | ra;
		UINT8 data = state->video_ram[address & 0x5fff];

		for (bit = 0; bit < 8; bit++)
		{
			int x = (column * 8) + bit;
			int color = BIT(data, 7);

			*BITMAP_ADDR16(bitmap, y, x) = color;

			data <<= 1;
		}
	}
}

static WRITE_LINE_DEVICE_HANDLER( v1050_vsync_changed )
{
	//v1050_state *driver_state = device->machine->driver_data;

	if (state)
	{
		cputag_set_input_line(device->machine, M6502_TAG, INPUT_LINE_IRQ0, ASSERT_LINE);
	}

	//upb8214_r3_w(driver_state->upb8214, state);
}

static const mc6845_interface v1050_mc6845_intf = 
{
	SCREEN_TAG,
	8,
	NULL,
	v1050_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(v1050_vsync_changed),
	NULL
};

/* Video Start */

static VIDEO_START( v1050 )
{
	v1050_state *state = machine->driver_data;
	
	/* find devices */
	state->mc6845 = devtag_get_device(machine, HD6845_TAG);

	/* register for state saving */
//	state_save_register_global(machine, state->);
}

/* Video Update */

static VIDEO_UPDATE( v1050 )
{
	v1050_state *state = screen->machine->driver_data;

	mc6845_update(state->mc6845, bitmap, cliprect);
	
	return 0;
}

/* Machine Drivers */

MACHINE_DRIVER_START( v1050_video )
	MDRV_MC6845_ADD(HD6845_TAG, HD6845, XTAL_15_36MHz/8, v1050_mc6845_intf) /* HD6845SP */

	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MDRV_SCREEN_SIZE(640, 400)
	MDRV_SCREEN_VISIBLE_AREA(0,640-1, 0, 400-1)

	MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(monochrome_green)

	MDRV_VIDEO_START(v1050)
	MDRV_VIDEO_UPDATE(v1050)
MACHINE_DRIVER_END
