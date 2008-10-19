/*****************************************************************************
 *
 * video/abc800.c
 *
 ****************************************************************************/

/*

	TODO:

	- add proper screen parameters to startup
	- abc800m high resolution (HR)
	- abc800c row update
	- abc800c palette
	- abc800c high resolution (HR)

*/

#include "driver.h"
#include "includes/abc80x.h"
#include "machine/z80dart.h"
#include "video/mc6845.h"

/* Palette Initialization */

static PALETTE_INIT( abc800m )
{
	palette_set_color_rgb(machine, 0, 0x00, 0x00, 0x00); // black
	palette_set_color_rgb(machine, 1, 0xff, 0xff, 0x00); // yellow (really white, but blue signal is disconnected from monitor)
}

static PALETTE_INIT( abc800c )
{
	palette_set_color_rgb(machine, 0, 0x00, 0x00, 0x00); // black
	palette_set_color_rgb(machine, 1, 0x00, 0x00, 0xff); // blue
	palette_set_color_rgb(machine, 2, 0xff, 0x00, 0x00); // red
	palette_set_color_rgb(machine, 3, 0xff, 0x00, 0xff); // magenta
	palette_set_color_rgb(machine, 4, 0x00, 0xff, 0x00); // green
	palette_set_color_rgb(machine, 5, 0x00, 0xff, 0xff); // cyan
	palette_set_color_rgb(machine, 6, 0xff, 0xff, 0x00); // yellow
	palette_set_color_rgb(machine, 7, 0xff, 0xff, 0xff); // white
}

/* External Interface */

WRITE8_HANDLER( abc800m_hrs_w )
{
}

WRITE8_HANDLER( abc800m_hrc_w )
{
}

WRITE8_HANDLER( abc800c_hrs_w )
{
}

WRITE8_HANDLER( abc800c_hrc_w )
{
}

/* MC6845 Row Update */

static MC6845_UPDATE_ROW( abc800m_update_row )
{
	abc800_state *state = device->machine->driver_data;

	int column;

	for (column = 0; column < x_count; column++)
	{
		int bit;

		UINT16 address = (videoram[(ma + column) & 0x7ff] << 4) | (ra & 0x0f);
		UINT8 data = (state->char_rom[address & 0x7ff] & 0x3f);

		if (column == cursor_x)
		{
			data = 0x3f;
		}

		data <<= 2;

		for (bit = 0; bit < ABC800_CHAR_WIDTH; bit++)
		{
			int x = (column * ABC800_CHAR_WIDTH) + bit;
			int color = BIT(data, 7);

			*BITMAP_ADDR16(bitmap, y, x) = color;

			data <<= 1;
		}
	}
}

static MC6845_ON_VSYNC_CHANGED(abc800_vsync_changed)
{
	abc800_state *state = device->machine->driver_data;

	z80dart_set_ri(state->z80dart, 1, vsync);
}

static MC6845_UPDATE_ROW( abc800c_update_row )
{
}

/* MC6845 Interfaces */

static const mc6845_interface abc800m_mc6845_interface = {
	SCREEN_TAG,
	ABC800_CCLK,
	ABC800_CHAR_WIDTH,
	NULL,
	abc800m_update_row,
	NULL,
	NULL,
	NULL,
	abc800_vsync_changed
};

static const mc6845_interface abc800c_mc6845_interface = {
	SCREEN_TAG,
	ABC800_CCLK,
	ABC800_CHAR_WIDTH,
	NULL,
	abc800c_update_row,
	NULL,
	NULL,
	NULL,
	abc800_vsync_changed
};

/* Video Start */

static VIDEO_START( abc800 )
{
	abc800_state *state = machine->driver_data;

	/* find devices */

	state->mc6845 = devtag_get_device(machine, MC6845, MC6845_TAG);

	/* find memory regions */

	state->char_rom = memory_region(machine, "chargen");
}

/* Video Update */

static VIDEO_UPDATE( abc800m )
{
	abc800_state *state = screen->machine->driver_data;
	
	mc6845_update(state->mc6845, bitmap, cliprect);
	
	return 0;
}

static VIDEO_UPDATE( abc800c )
{
	abc800_state *state = screen->machine->driver_data;
	
	mc6845_update(state->mc6845, bitmap, cliprect);
	
	return 0;
}

/* Machine Drivers */

MACHINE_DRIVER_START( abc800m_video )
	// device interface
	MDRV_DEVICE_ADD(MC6845_TAG, MC6845)
	MDRV_DEVICE_CONFIG(abc800m_mc6845_interface)

	// video hardware
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MDRV_SCREEN_SIZE(640, 400)
	MDRV_SCREEN_VISIBLE_AREA(0,640-1, 0, 400-1)

	MDRV_PALETTE_LENGTH(2)

	MDRV_PALETTE_INIT(abc800m)
	MDRV_VIDEO_START(abc800)
	MDRV_VIDEO_UPDATE(abc800m)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( abc800c_video )
	// device interface
	MDRV_DEVICE_ADD(MC6845_TAG, MC6845)
	MDRV_DEVICE_CONFIG(abc800c_mc6845_interface)

	// video hardware
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MDRV_SCREEN_SIZE(640, 400)
	MDRV_SCREEN_VISIBLE_AREA(0,640-1, 0, 400-1)

	MDRV_PALETTE_LENGTH(2)

	MDRV_PALETTE_INIT(abc800c)
	MDRV_VIDEO_START(abc800)
	MDRV_VIDEO_UPDATE(abc800c)
MACHINE_DRIVER_END
