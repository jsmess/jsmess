#include "driver.h"
#include "includes/v1050.h"
#include "video/mc6845.h"

/*

	TODO:

	- row update
	- vsync interrupt to Z80

*/

/* Video RAM Access */

READ8_HANDLER( v1050_attr_r )
{
	v1050_state *state = space->machine->driver_data;

	return state->attr;
}

WRITE8_HANDLER( v1050_attr_w )
{
	v1050_state *state = space->machine->driver_data;

	state->attr = data;
}

READ8_HANDLER( v1050_videoram_r )
{
	v1050_state *state = space->machine->driver_data;

	state->attr = (state->attr & 0xfc) | (state->attr_ram[offset] & 0x03);

	return state->video_ram[offset];
}

WRITE8_HANDLER( v1050_videoram_w )
{
	v1050_state *state = space->machine->driver_data;

	state->video_ram[offset] = data;

	if (BIT(state->attr, 2))
	{
		state->attr_ram[offset] = state->attr & 0x03;
	}
}

/* MC6845 Interface */

static MC6845_UPDATE_ROW( v1050_update_row )
{
	v1050_state *state = device->machine->driver_data;

	int column, bit;

	for (column = 0; column < x_count; column++)
	{
		UINT16 address = ((ra & 0x03) << 13) | (ma + column);
		UINT8 data = state->video_ram[address & V1050_VIDEORAM_MASK];

		for (bit = 0; bit < 8; bit++)
		{
			int x = (column * 8) + bit;
			int color = BIT(data, 7);

			*BITMAP_ADDR16(bitmap, y, x) = color;

			data <<= 1;
		}

		//logerror("MA %04x RA %01x addr %04x y %u\n", ma, ra, address, y);
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

	/* allocate memory */
	state->attr_ram = auto_alloc_array(machine, UINT8, V1050_VIDEORAM_SIZE);

	/* register for state saving */
//	state_save_register_global(machine, state->);
	state_save_register_global_pointer(machine, state->attr_ram, V1050_VIDEORAM_SIZE);
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
