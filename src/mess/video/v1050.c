#include "driver.h"
#include "includes/v1050.h"
#include "video/mc6845.h"

#define V1050_ATTR_BRIGHT	0x01
#define V1050_ATTR_BLINKING	0x02
#define V1050_ATTR_ATTEN	0x04
#define V1050_ATTR_REVERSE	0x10
#define V1050_ATTR_BLANK	0x20
#define V1050_ATTR_BOLD		0x40
#define V1050_ATTR_BLINK	0x80

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

	if (offset >= 0x2000)
	{
		state->attr = (state->attr & 0xfc) | (state->attr_ram[offset] & 0x03);
	}

	return state->video_ram[offset];
}

WRITE8_HANDLER( v1050_videoram_w )
{
	v1050_state *state = space->machine->driver_data;

	state->video_ram[offset] = data;

	if (offset >= 0x2000 && BIT(state->attr, 2))
	{
		state->attr_ram[offset] = state->attr & 0x03;
	}
}

/* MC6845 Interface */

static MC6845_UPDATE_ROW( v1050_update_row )
{
	v1050_state *state = device->machine->driver_data;

	int column, bit;
	UINT8 data;

	for (column = 0; column < x_count; column++)
	{
		UINT16 address = (((ra & 0x03) + 1) << 13) | ((ma & 0x1fff) + column);

		data = state->video_ram[address & V1050_VIDEORAM_MASK];

		for (bit = 0; bit < 8; bit++)
		{
			int x = (column * 8) + bit;
			int color = BIT(data, 7);

			/* blinking */
			if ((state->attr_ram[address] & V1050_ATTR_BLINKING) && !(state->attr & V1050_ATTR_BLINK)) color = 0;

			/* reverse video */
			color ^= BIT(state->attr, 4);

			/* blank video */
			if (state->attr & V1050_ATTR_BLANK) color = 0;

			/* bright */
			if (color && ((state->attr & V1050_ATTR_BOLD) ^ (state->attr_ram[address] & V1050_ATTR_BRIGHT))) color = 2;

			*BITMAP_ADDR16(bitmap, y, x) = color;

			data <<= 1;
		}
	}
}

static WRITE_LINE_DEVICE_HANDLER( v1050_vsync_changed )
{
	if (state)
	{
		cputag_set_input_line(device->machine, M6502_TAG, INPUT_LINE_IRQ0, ASSERT_LINE);
		v1050_set_int(device->machine, INT_VSYNC, 1);
	}
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

/* Palette */

static PALETTE_INIT( v1050 )
{
	palette_set_color(machine, 0, RGB_BLACK); /* black */
	palette_set_color_rgb(machine, 1, 0x00, 0x80, 0x00); /* green */
	palette_set_color_rgb(machine, 2, 0x00, 0xff, 0x00); /* bright green */
}

/* Video Start */

static VIDEO_START( v1050 )
{
	v1050_state *state = machine->driver_data;
	
	/* find devices */
	state->mc6845 = devtag_get_device(machine, H46505_TAG);

	/* allocate memory */
	state->attr_ram = auto_alloc_array(machine, UINT8, V1050_VIDEORAM_SIZE);

	/* register for state saving */
	state_save_register_global(machine, state->attr);
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
	MDRV_MC6845_ADD(H46505_TAG, H46505, XTAL_15_36MHz/8, v1050_mc6845_intf)

	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MDRV_SCREEN_SIZE(640, 400)
	MDRV_SCREEN_VISIBLE_AREA(0,640-1, 0, 400-1)

	MDRV_PALETTE_LENGTH(3)
    MDRV_PALETTE_INIT(v1050)

	MDRV_VIDEO_START(v1050)
	MDRV_VIDEO_UPDATE(v1050)
MACHINE_DRIVER_END
